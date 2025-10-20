#ifndef USER_H
#define USER_H
#include <string>
using namespace std;
// user的orm类
class User
{
public:
  User(int id = 0, string name = "", string password = "", string state = "offline")
      : id(id), name(name), password(password), state(state) {}
  int getId() const
  {
    return id;
  }
  string getName() const
  {
    return name;
  }
  string getPassword() const
  {
    return password;
  }
  string getState() const
  {
    return state;
  }
  void setId(int id)
  {
    this->id = id;
  }
  void setName(const string &name)
  {
    this->name = name;
  }
  void setPassword(const string &password)
  {
    this->password = password;
  }
  void setState(const string &state)
  {
    this->state = state;
  }

private:
  int id;
  string name;
  string password;
  string state;
};

#endif