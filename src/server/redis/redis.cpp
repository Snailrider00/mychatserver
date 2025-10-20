#include "redis.hpp"
#include <string>
#include <iostream>
using namespace std;
Redis::Redis()
    : _publish_context(nullptr), _subscribe_context(nullptr)
{
}

Redis::~Redis()
{
  if (_publish_context != nullptr)
  {
    redisFree(_publish_context);
  }

  if (_subscribe_context != nullptr)
  {
    redisFree(_subscribe_context);
  }
}

// 连接redis服务器
bool Redis::connect()
{
  // 负责publish发布消息的上下文连接
  _publish_context = redisConnect("127.0.0.1", 6379);
  if (nullptr == _publish_context)
  {
    cerr << "connect redis failed!" << endl;
    return false;
  }

  // 负责subscribe订阅消息的上下文连接
  _subscribe_context = redisConnect("127.0.0.1", 6379);
  if (nullptr == _subscribe_context)
  {
    cerr << "connect redis failed!" << endl;
    return false;
  }

  // 在单独的线程中，监听通道上的事件，有消息给业务层进行上报
  thread t([&]()
           { observer_channel_message(); });
  t.detach();

  cout << "connect redis server success!" << endl;
  return true;
}

// 向redis指定的通道channel发布消息
bool Redis::publish(int channel, string message)
{
  redisReply *reply = (redisReply *)redisCommand(_publish_context, "PUBLISH %d %s", channel, message.c_str());
  if (nullptr == reply)
  {
    cerr << "publish command failed!" << endl;
    return false;
  }
  freeReplyObject(reply);
  return true;
}

// 向redis指定的通道subscribe订阅消息
bool Redis::subscribe(int channel)
{
  // SUBSCRIBE命令本身会造成线程阻塞等待通道里发生消息，这里只做订阅通道，不接收通道消息
  // 通道消息的接收专门在observer_channel_message函数中的单独线程执行
  // 只负责发送命令，不阻塞接收redis server响应消息，不然和notifyMsg线程抢占响应资源
  if (REDIS_ERR == redisAppendCommand(this->_subscribe_context, "SUBSCRIBE %d", channel))
  {
    cerr << "subscribe command failed!" << endl;
    return false;
  }
  // redisBufferWrite函数会把命令写入到socket缓冲区，并不会阻塞等待服务器响应
  // done必须初始化，当done=1时，表示命令已经全部写入底层socket缓冲区
  int done = 0;
  while (!done)
  {
    if (REDIS_ERR == redisBufferWrite(this->_subscribe_context, &done))
    {
      cerr << "subscribe command failed!" << endl;
      return false;
    }
  }
  return true;
}

// 向redis指定的通道unsubscribe取消订阅消息
bool Redis::unsubscribe(int channel)
{
  if (REDIS_ERR == redisAppendCommand(this->_subscribe_context, "UNSUBSCRIBE %d", channel))
  {
    cerr << "unsubscribe command failed!" << endl;
    return false;
  }
  // redisBufferWrite可以循环发送缓冲区，直到缓冲区数据发送完毕（done被置为1）
  int done = 0;
  while (!done)
  {
    if (REDIS_ERR == redisBufferWrite(this->_subscribe_context, &done))
    {
      cerr << "unsubscribe command failed!" << endl;
      return false;
    }
  }
  return true;
}

// 在独立线程中接收订阅通道中的消息
void Redis::observer_channel_message()
{
  redisReply *reply = nullptr;
  // 阻塞的读取一条消息
  while (REDIS_OK == redisGetReply(this->_subscribe_context, (void **)&reply))
  {
    // 订阅收到的消息是一个带三元素的数组
    // ["message", <channel:string>, <payload:string>]
    // 线判断消息不为空，在判断第三个元素指针存在防止访问空指针，再判断字符串不为空
    if (reply != nullptr && reply->element[2] != nullptr && reply->element[2]->str != nullptr)
    {
      // 给业务层上报通道收到的消息
      _notify_message_handler(atoi(reply->element[1]->str), string(reply->element[2]->str));
    }
  }
}

// 初始化向业务层上报通道消息的回调对象
void Redis::init_notify_handler(function<void(int, string)> fn)
{
  this->_notify_message_handler = fn;
}