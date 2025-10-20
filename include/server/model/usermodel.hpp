#ifndef USERMODEL_H
#define USERMODEL_H
#include "user.hpp"
class UserModel
{
public:
  // User表的增加方法
  bool insert(User &user);

  User query(int id);

  bool updateState(User &user);

  // 当主机主动退出，重置所有用户状态为state
  void resetState();
};

#endif