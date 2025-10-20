#include "db.hpp"
#include <mysql/mysql.h>
#include <muduo/base/Logging.h>
using namespace muduo;
MySQL::MySQL()
{
  // 实例化一个连接
  _conn = mysql_init(nullptr);
}
MySQL::~MySQL()
{
  // 释放连接资源
  if (_conn != nullptr)
  {
    mysql_close(_conn);
  }
}
// 连接数据库
bool MySQL::connect()
{
  MYSQL *p = mysql_real_connect(_conn, server.c_str(),
                                user.c_str(), password.c_str(),
                                dbname.c_str(), 3306, nullptr, 0);
  if (p != nullptr)
  {
    mysql_query(_conn, "set names gbk");
    LOG_INFO << "connect mysql success!";
  }
  else
  {
    LOG_ERROR << "connect mysql failed!";
  }
  return p;
}
// 更新操作
bool MySQL::update(string sql)
{
  if (mysql_query(_conn, sql.c_str()))
  {
    LOG_ERROR << "更新失败:" << mysql_error(_conn);
    return false;
  }
  return true;
}

MYSQL_RES *MySQL::query(string sql)
{
  if (mysql_query(_conn, sql.c_str()))
  {
    LOG_ERROR << "查询失败:" << mysql_error(_conn);
    return nullptr;
  }
  return mysql_store_result(_conn);
}

MYSQL *MySQL::getConnection()
{
  return _conn;
}
