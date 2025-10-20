#ifndef DB_H
#define DB_H
#include <string>
#include <mysql/mysql.h>
using namespace std;
// 数据库配置信息
static string server = "127.0.0.1";
static string user = "root";
static string password = "WALUOLANmt00";
static string dbname = "chat";

class MySQL
{
public:
  // 连接数据库
  MySQL();
  ~MySQL();
  // 连接数据库
  bool connect();
  // 更新操作
  bool update(string sql);
  // 查询操作,其实和update差不多，但是会返回MYSQL_RES结果集
  MYSQL_RES *query(string sql);

  // 获取连接
  MYSQL *getConnection();

private:
  MYSQL *_conn;
};
#endif