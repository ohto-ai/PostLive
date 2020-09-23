#include <iostream>

#include "MySQLConnectionPool.h"
std::string db_name = "LiveUser";
std::string db_ip = "thatboy.info";
std::string db_user = "LiveClient";
std::string db_passwd = "liveclient%007";
int db_port = 3306;
std::string db_charset = "utf8";

int main()
{
	thatboy::sql::ConnectionPool db_pool(db_name, db_ip, db_user, db_passwd, db_port, db_charset);

	try {
		thatboy::sql::ScopedConnection conn(db_pool);
		auto query = conn->query("select * from LiveUser.live_user");

		auto ret = query.store();

		for (auto it : ret)
		{
			for (auto it : it)
			{
				std::cout << it << std::endl;
			}
			std::cout << std::endl;
		}
	}
	catch (const mysqlpp::Exception& e) {
		std::cout << "error:" << e.what() << "\n";
		return false;
	}
	Sleep(2000);
	return 0;
}