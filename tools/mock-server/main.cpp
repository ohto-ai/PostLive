#include <cpp-httplib/httplib.h>
#include <nlohmann/json.hpp>
#include <QCryptographicHash>

#include <fstream>
#include <iostream>
#include <string>

namespace
{
	std::string md5(const std::string& value)
	{
		return QCryptographicHash::hash(QByteArray::fromStdString(value), QCryptographicHash::Md5)
			.toHex()
			.toUpper()
			.toStdString();
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
	const std::string usersPath = argc > 1 ? argv[1] : "users.example.json";
	int port = 8877;
	if (argc > 2)
	{
		try
		{
			port = std::stoi(argv[2]);
		}
		catch (...)
		{
			std::cerr << "Invalid port: " << argv[2] << "\n";
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
				else
					authenticated = md5(body.value("password", std::string())) == savedToken;

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
