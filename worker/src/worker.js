// PostLive auth worker.
//
// Implements the POST /api protocol from docs/api.md on Cloudflare Workers:
//   D1 (DB) stores users with PBKDF2-SHA256 salted password hashes;
//   KV (KV) stores issued tokens (30 d TTL), per-account login-failure
//   counters and short-lived "online" markers written by heartbeat.
//
// The desktop client transmits MD5(password) (uppercase hex) over HTTPS —
// that string is treated as the password-equivalent and never stored in
// the clear: only PBKDF2(hash, per-user salt + server pepper) is persisted.

const PBKDF2_ITERATIONS = 50_000; // stays inside the free plan's 10 ms CPU budget
const PBKDF2_KEY_BITS = 256;
const SALT_BYTES = 16;
const TOKEN_BYTES = 32;
const TOKEN_TTL_SECONDS = 30 * 24 * 3600;
const ONLINE_TTL_SECONDS = 15 * 60;
const LOGIN_LOCKOUT_ATTEMPTS = 10;
const LOGIN_LOCKOUT_TTL_SECONDS = 15 * 60;
const ACCOUNT_PATTERN = /^[A-Za-z][A-Za-z0-9_-]{3,15}$/;
const PASSWORD_MD5_PATTERN = /^[0-9A-F]{32}$/;

function jsonResponse(body, status = 200)
{
	return new Response(JSON.stringify(body), {
		status,
		headers: { "content-type": "application/json" },
	});
}

function replyTemplate(body)
{
	return {
		account: typeof body.account === "string" ? body.account : "",
		type: typeof body.type === "string" ? body.type : "",
	};
}

function bytesToHex(bytes)
{
	return [...bytes].map((b) => b.toString(16).padStart(2, "0")).join("");
}

function hexToBytes(hex)
{
	const bytes = new Uint8Array(hex.length / 2);
	for (let i = 0; i < bytes.length; ++i)
		bytes[i] = parseInt(hex.substr(i * 2, 2), 16);
	return bytes;
}

function randomHex(byteCount)
{
	const bytes = new Uint8Array(byteCount);
	crypto.getRandomValues(bytes);
	return bytesToHex(bytes);
}

function randomToken()
{
	const bytes = new Uint8Array(TOKEN_BYTES);
	crypto.getRandomValues(bytes);
	let binary = "";
	for (const b of bytes)
		binary += String.fromCharCode(b);
	return btoa(binary).replaceAll("+", "-").replaceAll("/", "_").replace(/=+$/, "");
}

function timingSafeEqual(a, b)
{
	const left = String(a);
	const right = String(b);
	if (left.length !== right.length)
		return false;
	let diff = 0;
	for (let i = 0; i < left.length; ++i)
		diff |= left.charCodeAt(i) ^ right.charCodeAt(i);
	return diff === 0;
}

async function derivePasswordHash(env, saltHex, password, iterations)
{
	const encoder = new TextEncoder();
	const key = await crypto.subtle.importKey(
		"raw", encoder.encode(password), "PBKDF2", false, ["deriveBits"]);

	const pepper = env.PEPPER ? encoder.encode(env.PEPPER) : new Uint8Array();
	const salt = new Uint8Array([...hexToBytes(saltHex), ...pepper]);
	const bits = await crypto.subtle.deriveBits(
		{ name: "PBKDF2", hash: "SHA-256", salt, iterations },
		key, PBKDF2_KEY_BITS);
	return bytesToHex(new Uint8Array(bits));
}

// ---------------------------------------------------------------------------
// Storage access. env.DB follows the D1 API (prepare/bind/first/run),
// env.KV the Workers KV API (get/put) — both are easy to stub in tests.
// ---------------------------------------------------------------------------

async function getUser(env, account)
{
	if (!env.DB)
		throw new Error("D1 binding missing");
	const result = await env.DB
		.prepare("SELECT account, password_salt, password_hash, password_iterations, gender, avatar FROM users WHERE account = ?1")
		.bind(account)
		.first();
	return result ?? null;
}

async function upsertUser(env, { account, passwordSalt, passwordHash, passwordIterations, gender, avatar })
{
	const now = Math.floor(Date.now() / 1000);
	await env.DB
		.prepare(`INSERT INTO users (account, password_salt, password_hash, password_iterations, gender, avatar, created_at, updated_at)
		          VALUES (?1, ?2, ?3, ?4, ?5, ?6, ?7, ?7)
		          ON CONFLICT(account) DO UPDATE SET
		            password_salt = ?2, password_hash = ?3, password_iterations = ?4,
		            gender = ?5, avatar = ?6, updated_at = ?7`)
		.bind(account, passwordSalt, passwordHash, passwordIterations, gender ?? null, avatar ?? null, now)
		.run();
}

function tokenKey(token)
{
	return `token:${token}`;
}

async function issueToken(env, account)
{
	const token = randomToken();
	await env.KV.put(tokenKey(token), JSON.stringify({ account }), { expirationTtl: TOKEN_TTL_SECONDS });
	return token;
}

// The token must exist and belong to the account named in the request.
async function tokenOwner(env, token)
{
	if (typeof token !== "string" || token.length === 0)
		return null;
	const value = await env.KV.get(tokenKey(token));
	if (!value)
		return null;
	try {
		return JSON.parse(value).account ?? null;
	} catch {
		return null;
	}
}

async function getFailCount(env, account)
{
	const value = await env.KV.get(`fail:${account}`);
	return value ? parseInt(value, 10) || 0 : 0;
}

async function bumpFailCount(env, account)
{
	const next = (await getFailCount(env, account)) + 1;
	await env.KV.put(`fail:${account}`, String(next), { expirationTtl: LOGIN_LOCKOUT_TTL_SECONDS });
	return next;
}

// ---------------------------------------------------------------------------
// Handlers
// ---------------------------------------------------------------------------

async function handleLogin(env, body)
{
	const reply = replyTemplate(body);
	const account = body.account;
	const user = (typeof account === "string" && account)
		? await getUser(env, account) : null;
	if (!user)
	{
		reply.success = false;
		reply.info = "account not found";
		return reply;
	}

	const failCount = await getFailCount(env, account);
	if (failCount >= LOGIN_LOCKOUT_ATTEMPTS)
	{
		reply.success = false;
		reply.info = "too many failed attempts, retry later";
		return reply;
	}

	const login = body.login === "token" ? "token" : "password";
	let authenticated = false;
	if (login === "token")
	{
		authenticated = timingSafeEqual(await tokenOwner(env, body.token), account);
	}
	else
	{
		const password = typeof body.password === "string" ? body.password : "";
		const derived = await derivePasswordHash(env, user.password_salt, password, user.password_iterations);
		authenticated = timingSafeEqual(derived, user.password_hash);
	}

	if (!authenticated)
	{
		await bumpFailCount(env, account);
		reply.success = false;
		reply.info = "invalid credentials";
		return reply;
	}

	reply.success = true;
	reply.info = "login success.";
	reply.token = login === "token" ? body.token : await issueToken(env, account);
	return reply;
}

async function tokenAuthenticated(env, body)
{
	const account = typeof body.account === "string" ? body.account : "";
	return account !== "" && timingSafeEqual(await tokenOwner(env, body.token), account);
}

async function handleProfile(env, body)
{
	const reply = replyTemplate(body);
	if (!(await tokenAuthenticated(env, body)))
	{
		reply.success = false;
		reply.info = "token verify failed";
		return reply;
	}

	const user = await getUser(env, body.account);
	const requested = Array.isArray(body.profile) ? body.profile : [];
	const profile = {};
	if (user)
	{
		for (const field of requested)
		{
			if (typeof field === "string" && user[field] != null)
				profile[field] = user[field];
		}
	}
	reply.success = true;
	reply.info = "get profile success.";
	reply.profile = profile;
	return reply;
}

async function handleHeartbeat(env, body)
{
	const reply = replyTemplate(body);
	if (!(await tokenAuthenticated(env, body)))
	{
		reply.success = false;
		reply.info = "token verify failed";
		return reply;
	}

	// One KV write per heartbeat; free-plan writes (1 000/day) bound the
	// number of concurrent users, see docs/cloudflare.md.
	await env.KV.put(`online:${body.account}`, String(Math.floor(Date.now() / 1000)),
		{ expirationTtl: ONLINE_TTL_SECONDS });
	reply.success = true;
	reply.info = "heartbeat success.";
	return reply;
}

async function handleQuery(env, body)
{
	const reply = replyTemplate(body);
	if (!(await tokenAuthenticated(env, body)))
	{
		reply.success = false;
		reply.info = "token verify failed";
		return reply;
	}

	reply.success = true;
	reply.info = "query success.";
	reply.query = [];
	const requested = Array.isArray(body.query) ? body.query : [];
	for (const item of requested)
	{
		if (!item || typeof item !== "object")
			continue;
		const queryAccount = typeof item.account === "string" ? item.account : "";
		const entry = { account: queryAccount, profile: {} };
		const user = queryAccount ? await getUser(env, queryAccount) : null;
		const fields = Array.isArray(item.profile) ? item.profile : [];
		for (const field of fields)
		{
			if (typeof field !== "string")
				continue;
			if (field === "online")
				entry.profile.online = (await env.KV.get(`online:${queryAccount}`)) != null;
			else if (user && user[field] != null)
				entry.profile[field] = user[field];
		}
		reply.query.push(entry);
	}
	return reply;
}

// Registration / password reset, for the operator only: requires the
// REGISTER_SECRET header. `password` must be the exact string the desktop
// client sends — the uppercase MD5 hex of the real password.
async function handleRegister(env, body, request)
{
	const reply = replyTemplate(body);
	const provided = request.headers.get("x-admin-token") ?? "";
	if (!env.REGISTER_SECRET || !timingSafeEqual(provided, env.REGISTER_SECRET))
	{
		reply.success = false;
		reply.info = "admin token missing or wrong";
		return jsonResponse(reply, 403);
	}

	const account = typeof body.account === "string" ? body.account : "";
	const password = typeof body.password === "string" ? body.password : "";
	if (!ACCOUNT_PATTERN.test(account))
	{
		reply.success = false;
		reply.info = "invalid account";
		return jsonResponse(reply);
	}
	if (!PASSWORD_MD5_PATTERN.test(password))
	{
		reply.success = false;
		reply.info = "password must be the 32-char uppercase MD5 hex the client sends";
		return jsonResponse(reply);
	}

	const passwordSalt = randomHex(SALT_BYTES);
	reply.success = true;
	reply.info = "account stored.";
	await upsertUser(env, {
		account,
		passwordSalt,
		passwordHash: await derivePasswordHash(env, passwordSalt, password, PBKDF2_ITERATIONS),
		passwordIterations: PBKDF2_ITERATIONS,
		gender: typeof body.gender === "string" ? body.gender : undefined,
		avatar: typeof body.avatar === "string" ? body.avatar : undefined,
	});
	return jsonResponse(reply);
}

// Entry point shared by fetch() and the tests: takes the parsed JSON body,
// the bindings and the incoming request (for headers).
export async function handleApi(body, env, request)
{
	if (!body || typeof body !== "object" || Array.isArray(body))
		return jsonResponse({ success: false, info: "invalid json" });

	const type = body.type;
	if (type === "login")
		return jsonResponse(await handleLogin(env, body));
	if (type === "profile")
		return jsonResponse(await handleProfile(env, body));
	if (type === "heartbeat")
		return jsonResponse(await handleHeartbeat(env, body));
	if (type === "query")
		return jsonResponse(await handleQuery(env, body));
	if (type === "register")
		return handleRegister(env, body, request);
	return jsonResponse({ ...replyTemplate(body), success: false, info: "unknown type" });
}

export default {
	async fetch(request, env)
	{
		if (request.method !== "POST")
			return jsonResponse({ success: false, info: "method not allowed" }, 405);
		const url = new URL(request.url);
		if (url.pathname !== "/api")
			return jsonResponse({ success: false, info: "not found" }, 404);

		let body;
		try {
			body = await request.json();
		} catch {
			return jsonResponse({ success: false, info: "invalid json" });
		}
		return handleApi(body, env, request);
	},
};
