#ifndef _MYSQL_CONNECTION_POOL_H_
#define _MYSQL_CONNECTION_POOL_H_

#include <mysql++/mysql++.h>
#include <string>

namespace thatboy
{
	namespace sql
	{
		class ConnectionPool : public mysqlpp::ConnectionPool
		{
		public:
			ConnectionPool(std::string dbname
				, std::string serverip
				, std::string user
				, std::string passwd
				, int port = 3306
				, std::string charset = std::string("utf8"))
				: m_dbname(dbname)
				, m_server_ip(serverip)
				, m_user(user)
				, m_password(passwd)
				, m_charset(charset)
				, m_port(port)
			{
				m_max_idle_time = 300;
			}

			virtual ~ConnectionPool()
			{
				clear();
			}

			const std::string& getDBName() const { return m_dbname; }
			const std::string& getServerIP() const { return m_server_ip; }
			const std::string& getUser() const { return m_user; }
			const std::string& getPassword() const { return m_password; }
			const std::string& getCharset() const { return m_charset; }
			int getPort() const { return m_port; }

			void setMaxIdleTime(int max_idle)
			{
				m_max_idle_time = max_idle;
			}
		protected:
			virtual mysqlpp::Connection* create()
			{
				mysqlpp::Connection* conn = new mysqlpp::Connection(true);
				mysqlpp::SetCharsetNameOption* pOpt = new  mysqlpp::SetCharsetNameOption(m_charset.c_str());
				conn->set_option(pOpt);

				conn->connect(m_dbname.empty() ? 0 : m_dbname.c_str(),
					m_server_ip.empty() ? 0 : m_server_ip.c_str(),
					m_user.empty() ? 0 : m_user.c_str(),
					m_password.empty() ? "" : m_password.c_str(),
					m_port);

				return conn;
			}

			virtual void destroy(mysqlpp::Connection* cp)
			{
				delete cp;
			}

			virtual unsigned int max_idle_time()
			{
				return m_max_idle_time;
			}

		private:
			std::string m_dbname;
			std::string m_server_ip;
			std::string m_user;
			std::string m_password;
			std::string m_charset;
			int         m_port;
			int         m_max_idle_time;
		};
		using mysqlpp::ScopedConnection;
		using mysqlpp::Query;
	}
}
#endif