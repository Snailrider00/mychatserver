#ifndef CHATSERVICE_H
#define CHATSERVICE_H

#include <muduo/net/TcpClient.h>
#include <unordered_map>
#include <functional>
#include <mutex>
#include "offlinemessagemodel.hpp"
#include "usermodel.hpp"
#include "friendmodel.hpp"
#include "groupmodel.hpp"
#include "redis.hpp"
using namespace std;
using namespace muduo;
using namespace muduo::net;

#include "json.hpp"
using json = nlohmann::json;
using MsgHandler = std::function<void(const TcpConnectionPtr &conn, json &js, Timestamp)>;
// 单例模式
class ChatService
{
public:
  // 获取单例模式唯一入口
  static ChatService *instance();
  void login(const TcpConnectionPtr &conn, json &js, Timestamp time);
  // 处理注册业务
  void reg(const TcpConnectionPtr &conn, json &js, Timestamp time);
  // 处理客户端异常离线
  void clientCloseException(const TcpConnectionPtr &conn);
  // 一对一聊天业务
  void oneChat(const TcpConnectionPtr &conn, json &js, Timestamp time);
  // 添加好友业务
  void addFriend(const TcpConnectionPtr &conn, json &js, Timestamp time);
  // 创建群组业务
  void createGroup(const TcpConnectionPtr &conn, json &js, Timestamp time);
  // 加入群组业务
  void addGroup(const TcpConnectionPtr &conn, json &js, Timestamp time);
  // 群组聊天业务
  void groupChat(const TcpConnectionPtr &conn, json &js, Timestamp time);
  // 退出业务
  void loginout(const TcpConnectionPtr &conn, json &js, Timestamp time);
  // 重置信息
  void reset();
  MsgHandler getHandler(int msgid);

  // 从redis消息队列中获取订阅的消息
  void handleRedisSubscribeMessage(int, string);

private:
  ChatService();
  // 存储消息id和对应的业务处理方法
  unordered_map<int, MsgHandler> _msgHandlerMap;

  // 存储在线用户的通信连接
  // 当用户1向用户2发消息，是把消息传到服务器，服务器主动向用户2发送
  // 我们如何判断用户而在线？就要用一个数据结构来判断
  unordered_map<int, TcpConnectionPtr> _userConnMap;

  // 数据操作类对象
  UserModel _userModel;
  OfflineMsgModel _offlineMsgModel;
  FriendModel _friendModel;
  GroupModel _groupModel;

  mutex _connMutex;

  // redis操作对象
  Redis _redis;
};
#endif