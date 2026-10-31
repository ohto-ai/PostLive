#include <cpp-httplib/httplib.h>
#include <nlohmann/json.hpp>
#include <QCryptographicHash>
#include <QMessageAuthenticationCode>
#include <QRandomGenerator>

#include <fstream>
#include <iostream>
#include <string>

namespace
{
	// Dev-only speed-up. The real worker (worker/src/worker.js) uses 50 000
	// iterations; the users file format and the algorithm are identical.
	constexpr int kDefaultPbkdf2Iterations = 1000;

	std::string md5(const std::string& value)
	{
		return QCryptographicHash::hash(QByteArray::fromStdString(value), QCryptographicHash::Md5)
			.toHex()
			.toUpper()
			.toStdString();
	}

	std::string bytesToHex(const QByteArray& bytes)
	{
		static const char* digits = "0123456789abcdef";
		std::string out;
		out.reserve(size_t(bytes.size()) * 2);
		for (char c : bytes)
		{
			out.push_back(digits[(c >> 4) & 0xF]);
			out.push_back(digits[c & 0xF]);
		}
		return out;
	}

	QByteArray hexToBytes(const std::string& hex)
	{
		QByteArray bytes;
		bytes.reserve(int(hex.size() / 2));
		for (size_t i = 0; i + 1 < hex.size(); i += 2)
			bytes.push_back(char(std::stoi(hex.substr(i, 2), nullptr, 16)));
		return bytes;
	}

	// PBKDF2-HMAC-SHA256 (RFC 2898). dkLen is 32 bytes, so a single block.
	QByteArray pbkdf2HmacSha256(const std::string& password, const QByteArray& salt, int iterations)
	{
		const QByteArray key = QByteArray::fromStdString(password);
		QByteArray u = QMessageAuthenticationCode::hash(salt + QByteArray("\0\0\0\1", 4), key, QCryptographicHash::Sha256);
		QByteArray accumulator = u;
		for (int i = 1; i < iterations; ++i)
		{
			u = QMessageAuthenticationCode::hash(u, key, QCryptographicHash::Sha256);
			for (int j = 0; j < u.size(); ++j)
				accumulator[j] = char(accumulator[j] ^ u[j]);
		}
		return accumulator;
	}

	std::string randomSaltHex()
	{
		QByteArray salt(16, '\0');
		for (int i = 0; i < salt.size(); ++i)
			salt[i] = char(QRandomGenerator::system()->generate() & 0xFF);
		return bytesToHex(salt);
	}

	bool timingSafeEqual(const std::string& a, const std::string& b)
	{
		if (a.size() != b.size())
			return false;
		unsigned char diff = 0;
		for (size_t i = 0; i < a.size(); ++i)
			diff |= unsigned char(a[i]) ^ unsigned char(b[i]);
		return diff == 0;
	}

	void sendJson(httplib::Response& response, const nlohmann::json& body)
	{
		response.set_content(body.dump(4), "application/json");
	}

	nlohmann::json loadUsers(const std::string& path)
	{
		std::ifstream input(path);
		if (!input)
			return nlohmann::json();

		auto users = nlohmann::json::parse(input, nullptr, false);
		if (users.is_discarded() || !users.is_object())
			return nlohmann::json();
		return users;
	}

	bool tokenMatches(const nlohmann::json& user, const std::string& token)
	{
		return user.contains("token") && user["token"].is_string()
			&& user["token"].get<std::string>() == token;
	}

	nlohmann::json profileOf(const nlohmann::json& user)
	{
		nlohmann::json profile = nlohmann::json::object();
		if (user.contains("avatar") && user["avatar"].is_string())
			profile["avatar"] = user["avatar"];
		if (user.contains("gender") && user["gender"].is_string())
			profile["gender"] = user["gender"];
		return profile;
	}
}

int main(int argc, char** argv)
{
	const std::vector<std::string> args(argv + 1, argv + argc);

	// PostLiveMockServer.exe --hash <password> prints the JSON fragment to
	// paste into a users file. The stored password is the exact string the
	// desktop client sends (the uppercase MD5 hex of the real password).
	if (!args.empty() && args.front() == "--hash")
	{
		if (args.size() != 2)
		{
			std::cerr << "usage: PostLiveMockServer.exe --hash <password-as-sent-by-client>\n";
			return 2;
		}
		const std::string salt = randomSaltHex();
		const std::string hash = bytesToHex(pbkdf2HmacSha256(
			args[1], hexToBytes(salt), kDefaultPbkdf2Iterations));
		std::cout << nlohmann::json{
			{ "password_salt", salt },
			{ "password_hash", hash },
			{ "password_iterations", kDefaultPbkdf2Iterations },
		}.dump(4) << "\n";
		return 0;
	}

	const std::string usersPath = !args.empty() ? args[0] : "users.example.json";
	int port = 8877;
	if (args.size() > 1)
	{
		try
		{
			port = std::stoi(args[1]);
		}
		catch (...)
		{
			std::cerr << "Invalid port: " << args[1] << "\n";
			return 2;
		}
	}

	auto database = loadUsers(usersPath);
	if (!database.is_object())
	{
		std::cerr << "Failed to load users file: " << usersPath << "\n";
		return 1;
	}

	httplib::Server server;
	server.Post("/api", [&](const httplib::Request& request, httplib::Response& response)
		{
			auto body = nlohmann::json::parse(request.body, nullptr, false);
			std::cout << "Request:\n" << request.body << "\n";

			if (body.is_discarded() || !body.is_object())
			{
				sendJson(response, { { "success", false }, { "info", "invalid json" } });
				return;
			}

			const std::string account = body.value("account", std::string());
			const std::string type = body.value("type", std::string());
			nlohmann::json reply;
			reply["account"] = account;
			reply["type"] = type;

			if (!database.contains("users") || !database["users"].is_object()
				|| !database["users"].contains(account)
				|| !database["users"][account].is_object())
			{
				reply["success"] = false;
				reply["info"] = "account not found";
				std::cout << "Response:\n" << reply.dump(4) << "\n";
				sendJson(response, reply);
				return;
			}

			const auto& user = database["users"][account];
			const std::string savedToken = user.value("token", std::string());

			if (type == "login")
			{
				const std::string login = body.value("login", std::string("password"));
				bool authenticated = false;
				if (login == "token")
					authenticated = body.value("token", std::string()) == savedToken;
				else if (user.contains("password_salt") && user.contains("password_hash")
					&& user["password_salt"].is_string() && user["password_hash"].is_string())
				{
					// Salted storage (same algorithm as worker/src/worker.js).
					const int iterations = user.value("password_iterations", kDefaultPbkdf2Iterations);
					const std::string derived = bytesToHex(pbkdf2HmacSha256(
						body.value("password", std::string()),
						hexToBytes(user["password_salt"].get<std::string>()), iterations));
					authenticated = timingSafeEqual(derived, user["password_hash"].get<std::string>());
				}
				else
				{
					// Legacy users file without salted fields: compare against the token.
					authenticated = md5(body.value("password", std::string())) == savedToken;
				}

				reply["success"] = authenticated;
				reply["info"] = authenticated ? "login success." : "invalid credentials";
				if (authenticated)
					reply["token"] = savedToken;
			}
			else if (type == "profile")
			{
				const bool authenticated = tokenMatches(user, body.value("token", std::string()));
				reply["success"] = authenticated;
				reply["info"] = authenticated ? "get profile success." : "token verify failed";
				if (authenticated)
					reply["profile"] = profileOf(user);
			}
			else if (type == "heartbeat")
			{
				const bool authenticated = tokenMatches(user, body.value("token", std::string()));
				reply["success"] = authenticated;
				reply["info"] = authenticated ? "heartbeat success." : "token verify failed";
			}
			else if (type == "query")
			{
				const bool authenticated = tokenMatches(user, body.value("token", std::string()));
				reply["success"] = authenticated;
				reply["info"] = authenticated ? "query success." : "token verify failed";
				reply["query"] = nlohmann::json::array();

				const auto requested = body.value("query", nlohmann::json::array());
				if (authenticated && requested.is_array())
				{
					for (const auto& item : requested)
					{
						if (!item.is_object())
							continue;
						const std::string queryAccount = item.value("account", std::string());
						nlohmann::json entry;
						entry["account"] = queryAccount;
						entry["profile"] = nlohmann::json::object();
						if (database["users"].contains(queryAccount)
							&& database["users"][queryAccount].is_object())
						{
							const auto& queryUser = database["users"][queryAccount];
							const auto fields = item.value("profile", nlohmann::json::array());
							if (fields.is_array())
							{
								for (const auto& field : fields)
								{
									if (!field.is_string())
										continue;
									const std::string key = field.get<std::string>();
									if (queryUser.contains(key))
										entry["profile"][key] = queryUser[key];
								}
							}
						}
						reply["query"].push_back(entry);
					}
				}
			}
			else
			{
				reply["success"] = false;
				reply["info"] = "unknown type";
			}

			std::cout << "Response:\n" << reply.dump(4) << "\n";
			sendJson(response, reply);
		});

	std::cout << "PostLive mock auth server\n"
		<< "  listening: http://127.0.0.1:" << port << "\n"
		<< "  users:     " << usersPath << "\n";
	if (!server.listen("127.0.0.1", port))
	{
		std::cerr << "Failed to listen on 127.0.0.1:" << port << "\n";
		return 1;
	}
	return 0;
}
