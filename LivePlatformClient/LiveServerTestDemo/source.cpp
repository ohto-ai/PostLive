#include <json.hpp>
#include <httplib.h>
#include <MD5.h>

int main()
{
	httplib::Server server;
	server.bind_to_port("localhost", 8080);

	nlohmann::json mysqlDB = nlohmann::json::parse(R"(
{
    "user_list":[
        "thatboy","jaychou","kongjh"
    ],
    "users":{
        "thatboy":{
            "token":"385B3D8302A4B0E7570D6158CD2FBCA8",
            "gender":"male",
            "avatar":"https://i.loli.net/2020/09/25/YyGpnNmq5fCbEd6.png"
        },
        "jaychou":{
            "token":"E10ADC3949BA59ABBE56E057F20F883E",
            "gender":"female",
            "avatar":"https://cdn.jsdelivr.net/gh/zhouhuan666/BlogAssets@0.11/SettingPic/avatar.JPG"
        },
        "kongjh":{
            "token":"E80B5017098950FC58AAD83C8C14978E",
            "gender":"male",
            "avatar":"http://thatboy.info:5120/uploads/big/c180974952931b1433afc32c6e8522cf.jpg"
        }
    }
}
)");


	server.Post("/api", [&](const httplib::Request& req, httplib::Response& res)
		{
			const auto cont = nlohmann::json::parse(req.body);
			nlohmann::json body;

			std::cout << "Request:\n" << cont.dump(4) << "\n" << std::endl;

			body["account"] = cont["account"];
			body["type"] = cont["type"];

			// 身份校验
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
			else if (md5(cont["password"]) != dbToken)// 生成一个token比对
			{
				body["success"] = false;
				body["info"] = "token verify failed.";
				res.set_content(body.dump(), "application/json");
				std::cout << "Response:\n" << body.dump(4) << "\n" << std::endl;
				return;
			}

			// 身份验证通过
			body["token"] = dbToken;
			// 登录
			if (cont["type"] == "login")
			{
				body["success"] = true;
				body["info"] = "login success.";
				// 标记用户在线
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



	server.listen_after_bind();
	return 0;
}