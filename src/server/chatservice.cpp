#include "chatservice.hpp"
#include "public.hpp"
#include <muduo/base/Logging.h>
#include "user.hpp"
#include "usermodel.hpp"
using namespace placeholders;
using namespace muduo;
ChatService *ChatService::instance()
{
  static ChatService service;
  return &service;
}

// 注册消息和对应的handler回调操作
// 就是在消息队列里加入对应的函数指针
ChatService::ChatService()
{
  _msgHandlerMap.insert(make_pair(LOGIN_MSG, std::bind(&ChatService::login, this, _1, _2, _3)));
  _msgHandlerMap.insert({LOGINOUT_MSG, std::bind(&ChatService::loginout, this, _1, _2, _3)});
  _msgHandlerMap.insert(make_pair(REG_MSG, std::bind(&ChatService::reg, this, _1, _2, _3)));
  _msgHandlerMap.insert(make_pair(ONE_CHAT_MSG, std::bind(&ChatService::oneChat, this, _1, _2, _3)));
  _msgHandlerMap.insert(make_pair(ADD_FRIEND_MSG, std::bind(&ChatService::addFriend, this, _1, _2, _3)));

  // 群组相关的回调
  _msgHandlerMap.insert({CREATE_GROUP_MSG, std::bind(&ChatService::createGroup, this, _1, _2, _3)});
  _msgHandlerMap.insert({ADD_GROUP_MSG, std::bind(&ChatService::addGroup, this, _1, _2, _3)});
  _msgHandlerMap.insert({GROUP_CHAT_MSG, std::bind(&ChatService::groupChat, this, _1, _2, _3)});

  // 连接redis服务器
  if (_redis.connect())
  {
    // 设置上报消息的回调
    _redis.init_notify_handler(std::bind(&ChatService::handleRedisSubscribeMessage, this, _1, _2));
  }
}

// 获取消息对应的处理器
MsgHandler ChatService::getHandler(int msgid)
{
  // 记录错误的日志，msgid没有对应的事件处理回调的话
  auto it = _msgHandlerMap.find(msgid);
  if (it == _msgHandlerMap.end())
  {
    // 返回一个默认的处理器，空操作
    return [=](const TcpConnectionPtr &conn, json &js, Timestamp)
    {
      LOG_ERROR << "msgid: " << msgid << " can not find handler!";
    };
  }
  else
  {
    return _msgHandlerMap[msgid];
  }
}

// 处理登录的业务
void ChatService::login(const TcpConnectionPtr &conn, json &js, Timestamp time)
{
  int id = js["id"].get<int>();
  string password = js["password"];
  User user = _userModel.query(id);

  if (user.getId() == id && user.getPassword() == password)
  {
    // 登录成功两种情况，以在线和未在线
    if (user.getState() == "online")
    {
      json response;
      // 已经在线了
      response["msgid"] = LOGIN_MSG_ACK;
      response["errno"] = 2; // 2表示重复登录
      response["errmsg"] = "该账号已经登录，请重新登录新账号";
      conn->send(response.dump());
    }
    else
    {

      // 登录成功，记录长连接状态
      // 由于是muduo库默认是在多线程下进行，stl是不保证线程安全的,手动加锁
      {
        lock_guard<mutex> lock(_connMutex);
        _userConnMap.insert(make_pair(id, conn));
      }

      // id用户登录成功后，向redis订阅channel(id)的消息
      _redis.subscribe(id);

      // mysql数据库的线程安全由mysqlservice自己保证，我们不管
      // 登录成功，把状态修改成online
      user.setState("online");
      _userModel.updateState(user);

      json response;
      // 已经在线了
      response["msgid"] = LOGIN_MSG_ACK;
      response["errno"] = 0; // 2表示重复登录
      response["id"] = user.getId();
      response["name"] = user.getName();

      // 登录成功要查询是否有离线消息
      vector<string> vec = _offlineMsgModel.query(id);
      if (!vec.empty())
      {
        // 如果有离线消息
        response["offlinemsg"] = vec;
        _offlineMsgModel.remove(id); // 删除离线消息
      }
      // 登录成功后，要查询该用户的好友列表信息
      vector<User> friends = _friendModel.query(id);
      if (!friends.empty())
      {
        // 有好友
        vector<string> vec2;
        for (User &user : friends)
        {
          json js;
          js["id"] = user.getId();
          js["name"] = user.getName();
          js["state"] = user.getState();
          vec2.push_back(js.dump());
        }
        response["friends"] = vec2;
      }

      // 登录成功后查询是否有群组
      vector<Group> groups = _groupModel.queryGroups(id);
      if (!groups.empty())
      {
        vector<string> groupV;
        for (Group &group : groups)
        {
          json grpjson;
          grpjson["id"] = group.getId();
          grpjson["groupname"] = group.getName();
          grpjson["groupdesc"] = group.getDesc();
          vector<string> userV;
          for (GroupUser &user : group.getUsers())
          {
            json usrjson;
            usrjson["id"] = user.getId();
            usrjson["name"] = user.getName();
            usrjson["state"] = user.getState();
            usrjson["role"] = user.getRole();
            userV.push_back(usrjson.dump());
          }
          grpjson["users"] = userV;
          groupV.push_back(grpjson.dump());
        }
        response["groups"] = groupV;
      }
      conn->send(response.dump());
    }
  }
  else
  {
    // 登录失败
    json response;
    response["msgid"] = LOGIN_MSG_ACK;
    response["errno"] = 1; // 2表示重复登录
    response["errmsg"] = "用户名或密码错误";
    conn->send(response.dump());
  }
}
// 处理注册业务
void ChatService::reg(const TcpConnectionPtr &conn, json &js, Timestamp time)
{
  // 处理注册业务
  string name = js["name"];
  string password = js["password"];

  // 创建ORM对象
  User user;
  user.setName(name);
  user.setPassword(password);
  // 通过业务类进行操作
  bool state = _userModel.insert(user);

  if (state)
  {
    // 如果注册成功
    json response;
    response["msgid"] = REG_MSG_ACK;
    response["errno"] = 0; // 0表示成功
    response["id"] = user.getId();
    conn->send(response.dump());
  }
  else
  {
    json response;
    response["msgid"] = REG_MSG_ACK;
    response["errno"] = 1;
    conn->send(response.dump());
  }
}

void ChatService::clientCloseException(const TcpConnectionPtr &conn)
{
  // 客户端异常退出
  User user;
  // 注意线程同步
  {
    lock_guard<mutex> lock(_connMutex);
    for (auto it = _userConnMap.begin(); it != _userConnMap.end(); it++)
    {
      if (it->second == conn)
      {
        // 从map表中删除用户的连接信息
        user.setId(it->first);
        _userConnMap.erase(it);
        LOG_INFO << "clientCloseException userId=" << user.getId();
        break;
      }
    }
  }

  if (user.getId() != -1)
  {
    user.setState("offline");
    _userModel.updateState(user);
  }
}
void ChatService::oneChat(const TcpConnectionPtr &conn, json &js, Timestamp time)
{
  int toid = js["toid"].get<int>();
  // 如果对方在线直接转发
  // 处理线程同步问题
  {
    lock_guard<mutex> lock(_connMutex);
    auto it = _userConnMap.find(toid);
    if (it != _userConnMap.end())
    {
      it->second->send(js.dump());
      return;
    }
  }
  // 查询toid是否在线
  User user = _userModel.query(toid);
  if (user.getState() == "online")
  {
    _redis.publish(toid, js.dump());
    return;
  }
  // toid不在线，存储离线消息
  _offlineMsgModel.insert(toid, js.dump());
}
void ChatService::addFriend(const TcpConnectionPtr &conn, json &js, Timestamp time)
{
  int userid = js["id"].get<int>();
  int friendid = js["friendid"].get<int>();
  _friendModel.insert(userid, friendid);
}

void ChatService::reset()
{
  _userModel.resetState();
}

// 创建群组业务
void ChatService::createGroup(const TcpConnectionPtr &conn, json &js, Timestamp time)
{
  int userid = js["id"].get<int>();
  string name = js["groupname"];
  string desc = js["groupdesc"];
  // 存储新创建的群组消息
  Group group(-1, name, desc);
  if (_groupModel.createGroup(group))
  {
    // 存储群组创建人信息
    _groupModel.addGroup(userid, group.getId(), "creator");
  }
}
// 加入群组业务
void ChatService::addGroup(const TcpConnectionPtr &conn, json &js, Timestamp time)
{
  int userid = js["id"].get<int>();
  int groupid = js["groupid"].get<int>();
  _groupModel.addGroup(userid, groupid, "normal");
}
// 群组聊天业务
void ChatService::groupChat(const TcpConnectionPtr &conn, json &js, Timestamp time)
{
  int userid = js["id"].get<int>();
  int groupid = js["groupid"].get<int>();
  // 查询群组其他用户的id列表，然后把消息推送给在线的，不在线的存储离线消息
  vector<int> useridVec = _groupModel.queryGroupUsers(userid, groupid);
  lock_guard<mutex> lock(_connMutex);
  for (int id : useridVec)
  {
    auto it = _userConnMap.find(id);
    if (it != _userConnMap.end())
    {
      // 找的到说明在线，直接转发
      it->second->send(js.dump());
    }
    else
    {

      // 查询toid是否在线
      User user = _userModel.query(id);
      if (user.getState() == "online")
      {
        _redis.publish(id, js.dump());
      }
      else
      {
        // 存储离线群消息
        _offlineMsgModel.insert(id, js.dump());
      }
    }
  }
}

void ChatService::loginout(const TcpConnectionPtr &conn, json &js, Timestamp time)
{
  int userid = js["id"].get<int>();

  {
    lock_guard<mutex> lock(_connMutex);
    auto it = _userConnMap.find(userid);
    if (it != _userConnMap.end())
    {
      _userConnMap.erase(it);
    }
  }

  // 用户注销，相当于就是下线，在redis中取消订阅通道
  _redis.unsubscribe(userid);

  // 更新用户的状态信息
  User user(userid, "", "", "offline");
  _userModel.updateState(user);
}

void ChatService::handleRedisSubscribeMessage(int userid, string msg)
{
  lock_guard<mutex> lock(_connMutex);
  auto it = _userConnMap.find(userid);
  if (it != _userConnMap.end())
  {
    it->second->send(msg);
    return;
  }
  // 存储该用户的离线消息
  _offlineMsgModel.insert(userid, msg);
}