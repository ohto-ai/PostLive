#include <json.hpp>
#include <httplib.h>
#include <MD5.h>

int main()
{
	httplib::Server server;
	server.bind_to_port("localhost", 8877);

	std::ifstream ifs("userdata.json");	
	nlohmann::json mysqlDB = nlohmann::json::parse(std::string{ (std::istreambuf_iterator<char>(ifs)), std::istreambuf_iterator<char>() });

	server.Post("/api", [&](const httplib::Request& req, httplib::Response& res)
		{
			const auto cont = nlohmann::json::parse(req.body);
			nlohmann::json body;

			std::cout << "=====================================================\n" << std::endl;

			std::cout << "Request:\n" << cont.dump(4) << "\n" << std::endl;

			body["account"] = cont["account"];
			body["type"] = cont["type"];

			std::string dbToken = mysqlDB["users"][cont["account"].get<std::string>()]["token"];
			if (cont["type"] != "login")
			{
				if (cont["token"] != dbToken)
				{
					body["success"] = false;
					body["info"] = "token verify failed.";
					res.set_content(body.dump(), "application/json");
					std::cout << "Response:\n" << body.dump(4) << "\n" << std::endl;
					return;
				}
			}
			else if (cont["login"] == "token")
			{
				if (cont["token"] != dbToken)
				{
					body["success"] = false;
					body["info"] = "token verify failed.";
					res.set_content(body.dump(), "application/json");
					std::cout << "Response:\n" << body.dump(4) << "\n" << std::endl;
					return;
				}
			}
			else if (md5(cont["password"]) != dbToken)
			{
				body["success"] = false;
				body["info"] = "token verify failed.";
				res.set_content(body.dump(), "application/json");
				std::cout << "Response:\n" << body.dump(4) << "\n" << std::endl;
				return;
			}

			body["token"] = dbToken;

			if (cont["type"] == "login")
			{
				body["success"] = true;
				body["info"] = "login success.";

			}
			else if (cont["type"] == "profile")
			{
				for (const std::string& profile : cont["profile"])
				{
					body["profile"][profile] = mysqlDB["users"][body["account"].get<std::string>()][profile];
				}
				body["success"] = true;
				body["info"] = "get profile success.";
			}

			res.set_content(body.dump(), "application/json");
			std::cout << "Response:\n" << body.dump(4) << "\n" << std::endl;
		});


	std::cout << "Verify server started." << std::endl;
	server.listen_after_bind();
	return 0;
}