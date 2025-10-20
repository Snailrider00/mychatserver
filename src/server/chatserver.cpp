#include "chatserver.hpp"
#include <functional>
#include <iostream>
#include "json.hpp"
#include <string>
#include "chatservice.hpp"
using namespace std;
using namespace placeholders;
using namespace nlohmann;
ChatServer::ChatServer(EventLoop *loop,
                       const InetAddress &listenAddr,
                       const string &nameArg) : _server(loop, listenAddr, nameArg), _loop(loop)
{
  // 设置两个回调函数
  _server.setConnectionCallback(bind(&ChatServer::onConnection, this, _1));
  _server.setMessageCallback(bind(&ChatServer::onMessage, this, _1, _2, _3));

  // 设置线程数目
  _server.setThreadNum(4);
}

void ChatServer::start()
{
  _server.start();
}

void ChatServer::onConnection(const TcpConnectionPtr &conn)
{
  // 客户端断开连接
  if (!conn->connected())
  {
    // 客户端异常关闭
    ChatService::instance()->clientCloseException(conn);
    conn->shutdown();
  }
}
// 上报读写事件相关信息的回调函数
void ChatServer::onMessage(const TcpConnectionPtr &conn,
                           Buffer *buffer,
                           Timestamp time)
{
  string buf = buffer->retrieveAllAsString();
  // 数据的反序列化
  json js = json::parse(buf);

  // 网络模块，要注册两个回调函数，进行解耦，把网络模块和业务模块分开
  // 通过ChatService业务（单例模式）得到对应消息的对应回调
  MsgHandler msgHandler = ChatService::instance()->getHandler(js["msgid"].get<int>());
  // 执行回调
  msgHandler(conn, js, time);
}