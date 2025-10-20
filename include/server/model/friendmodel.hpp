#ifndef FRIENDMODEL_H
#define FRIENDMODEL_H
#include <vector>
#include "user.hpp"
class FriendModel
{
public:
  // 添加好友
  void insert(int userid, int friendid);

  // 返回用户的好友列表
  std::vector<User> query(int userid);
};

#endif