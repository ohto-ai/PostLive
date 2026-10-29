// Unit tests for the PostLive auth worker, run with `node --test worker/test/`.
// D1 and KV are stubbed with the minimal API surface the worker uses.

import test from "node:test";
import assert from "node:assert/strict";

import { handleApi } from "../src/worker.js";

const ADMIN_TOKEN = "admin-secret";
const MD5_123456 = "E10ADC3949BA59ABBE56E057F20F883E"; // md5("123456") uppercase

// Single-table D1 fake: INSERT ... ON CONFLICT upserts, SELECT ... WHERE account = ?1 reads.
function makeFakeDb()
{
	const rows = new Map();
	return {
		rows,
		prepare(sql)
		{
			let boundArgs;
			return {
				bind(...args)
				{
					boundArgs = args;
					return this;
				},
				async first()
				{
					assert.match(sql, /WHERE account = \?1/);
					return rows.get(boundArgs[0]) ?? null;
				},
				async run()
				{
					assert.match(sql, /INSERT INTO users/);
					const [account, salt, hash, iterations, gender, avatar] = boundArgs;
					const existing = rows.get(account) ?? { created_at: 0 };
					rows.set(account, {
						account, password_salt: salt, password_hash: hash,
						password_iterations: iterations, gender, avatar,
						created_at: existing.created_at, updated_at: 0,
					});
					return { success: true };
				},
			};
		},
	};
}

function makeEnv()
{
	return {
		DB: makeFakeDb(),
		KV: kvApi(new Map()),
		PEPPER: "test-pepper",
		REGISTER_SECRET: ADMIN_TOKEN,
	};
}

// Minimal KV API over a Map (TTLs are ignored; the tests do not depend on expiry).
function kvApi(map)
{
	return {
		get: (key) => Promise.resolve(map.get(key) ?? null),
		put: (key, value) => (map.set(key, value), Promise.resolve()),
	};
}

async function callJson(env, body, headers = {})
{
	const request = new Request("https://worker.test/api", {
		method: "POST",
		body: JSON.stringify(body),
		headers,
	});
	const response = await handleApi(body, env, request);
	return { status: response.status, body: await response.json() };
}

async function registerUser(env, account = "tester", password = MD5_123456)
{
	const { body } = await callJson(env, {
		type: "register", account, password, gender: "male", avatar: "https://a/p.png",
	}, { "x-admin-token": ADMIN_TOKEN });
	assert.equal(body.success, true, `register failed: ${body.info}`);
}

test("register requires the admin token header", async () =>
{
	const env = makeEnv();
	const { status, body } = await callJson(env, { type: "register", account: "tester", password: MD5_123456 });
	assert.equal(status, 403);
	assert.equal(body.success, false);
});

test("register rejects malformed accounts and passwords", async () =>
{
	const cases = [
		{ account: "a", password: MD5_123456 },                          // too short
		{ account: "9lead", password: MD5_123456 },                      // must start with a letter
		{ account: "tester", password: "plaintext" },                    // must be MD5 hex
		{ account: "tester", password: MD5_123456.toLowerCase() },       // must be uppercase
	];
	for (const { account, password } of cases)
	{
		const env = makeEnv();
		const { body } = await callJson(env, { type: "register", account, password },
			{ "x-admin-token": ADMIN_TOKEN });
		assert.equal(body.success, false, `${account} / ${password}`);
	}
});

test("password login issues a token and profile works with it", async () =>
{
	const env = makeEnv();
	await registerUser(env);

	const login = await callJson(env, { type: "login", account: "tester", login: "password", password: MD5_123456 });
	assert.equal(login.body.success, true, login.body.info);
	assert.ok(typeof login.body.token === "string" && login.body.token.length > 20);

	const profile = await callJson(env, {
		type: "profile", account: "tester", token: login.body.token, profile: ["avatar", "gender"],
	});
	assert.equal(profile.body.success, true, profile.body.info);
	assert.deepEqual(profile.body.profile, { avatar: "https://a/p.png", gender: "male" });
});

test("wrong password fails and does not issue a token", async () =>
{
	const env = makeEnv();
	await registerUser(env);

	const login = await callJson(env, { type: "login", account: "tester", login: "password", password: "0".repeat(32) });
	assert.equal(login.body.success, false);
	assert.equal(login.body.token, undefined);
});

test("token login returns the same token and is account-bound", async () =>
{
	const env = makeEnv();
	await registerUser(env);
	const login = await callJson(env, { type: "login", account: "tester", login: "password", password: MD5_123456 });

	const tokenLogin = await callJson(env, { type: "login", account: "tester", login: "token", token: login.body.token });
	assert.equal(tokenLogin.body.success, true, tokenLogin.body.info);
	assert.equal(tokenLogin.body.token, login.body.token);

	// The same token must not authenticate a different account.
	const foreign = await callJson(env, { type: "login", account: "intruder", login: "token", token: login.body.token });
	assert.equal(foreign.body.success, false);
});

test("unknown account reports failure without leaking state", async () =>
{
	const env = makeEnv();
	const reply = await callJson(env, { type: "login", account: "ghost", login: "password", password: MD5_123456 });
	assert.equal(reply.body.success, false);
	assert.equal(reply.body.info, "account not found");
});

test("profile only returns requested fields and fails without a valid token", async () =>
{
	const env = makeEnv();
	await registerUser(env);

	const bad = await callJson(env, { type: "profile", account: "tester", token: "nope", profile: ["avatar"] });
	assert.equal(bad.body.success, false);

	const login = await callJson(env, { type: "login", account: "tester", login: "password", password: MD5_123456 });
	const partial = await callJson(env, {
		type: "profile", account: "tester", token: login.body.token, profile: ["avatar"],
	});
	assert.deepEqual(partial.body.profile, { avatar: "https://a/p.png" });
});

test("heartbeat marks the account online for query", async () =>
{
	const env = makeEnv();
	await registerUser(env);
	const login = await callJson(env, { type: "login", account: "tester", login: "password", password: MD5_123456 });
	const token = login.body.token;

	const query = await callJson(env, {
		type: "query", account: "tester", token,
		query: [{ account: "tester", profile: ["gender", "online"] }],
	});
	assert.equal(query.body.success, true, query.body.info);
	assert.deepEqual(query.body.query, [{ account: "tester", profile: { gender: "male", online: false } }]);

	const beat = await callJson(env, { type: "heartbeat", account: "tester", token });
	assert.equal(beat.body.success, true, beat.body.info);

	const queryAgain = await callJson(env, {
		type: "query", account: "tester", token,
		query: [{ account: "tester", profile: ["online"] }],
	});
	assert.deepEqual(queryAgain.body.query, [{ account: "tester", profile: { online: true } }]);
});

test("heartbeat with a foreign token fails", async () =>
{
	const env = makeEnv();
	await registerUser(env);
	const login = await callJson(env, { type: "login", account: "tester", login: "password", password: MD5_123456 });

	const beat = await callJson(env, { type: "heartbeat", account: "other", token: login.body.token });
	assert.equal(beat.body.success, false);
	assert.equal(beat.body.info, "token verify failed");
});

test("repeated failed logins trigger a temporary lockout", async () =>
{
	const env = makeEnv();
	await registerUser(env);

	let reply;
	for (let i = 0; i < 10; ++i)
		reply = await callJson(env, { type: "login", account: "tester", login: "password", password: "0".repeat(32) });
	assert.equal(reply.body.success, false);

	const locked = await callJson(env, { type: "login", account: "tester", login: "password", password: MD5_123456 });
	assert.equal(locked.body.success, false);
	assert.match(locked.body.info, /too many failed attempts/);
});

test("unknown request types are rejected; register upserts on re-run", async () =>
{
	const env = makeEnv();
	const unknown = await callJson(env, { type: "dance", account: "tester" });
	assert.equal(unknown.body.success, false);
	assert.equal(unknown.body.info, "unknown type");

	await registerUser(env);
	await registerUser(env); // must not throw on conflict
	const login = await callJson(env, { type: "login", account: "tester", login: "password", password: MD5_123456 });
	assert.equal(login.body.success, true, login.body.info);
});
