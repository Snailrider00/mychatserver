#ifndef OFFLINEMESSAGEMODEL_H
#define OFFLINEMESSAGEMODEL_H
#include <vector>
#include <string>
class OfflineMsgModel
{
public:
  // 存储离线消息
  void insert(int userid, std::string msg);
  // 删除离线消息
  void remove(int userid);
  // 查询用户的离线消息
  std::vector<std::string> query(int userid);
};

#endif