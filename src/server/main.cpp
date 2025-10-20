#include "chatserver.hpp"
#include <iostream>
#include <signal.h>
#include "chatservice.hpp"
using namespace std;

void resetHandler(int)
{
  ChatService::instance()->reset();
  exit(0);
}
int main(int argc, char **argv)
{
  // 我们退出服务器的时候使用的是一个信号
  signal(SIGINT, resetHandler);
  if (argc < 3)
  {
    cerr << "command invalid! example: ./ChatServer 127.0.0.1 6000" << endl;
    exit(-1);
  }

  // 解析通过命令行参数传递的ip和port
  char *ip = argv[1];
  uint16_t port = atoi(argv[2]);

  EventLoop loop;
  InetAddress addr(ip, port);
  ChatServer server(&loop, addr, "ChatServer");
  server.start();
  loop.loop();
  return 0;
}