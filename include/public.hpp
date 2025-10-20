#ifndef PUBLIC_H
#define PUBLIC_H

enum EnMsgType
{
  LOGIN_MSG = 1,
  LOGIN_MSG_ACK,
  LOGINOUT_MSG, // 注销消息
  REG_MSG,
  REG_MSG_ACK,
  ONE_CHAT_MSG,   // 聊天消息
  ADD_FRIEND_MSG, // 添加好友消息

  // 群组相关
  CREATE_GROUP_MSG,
  ADD_GROUP_MSG,
  GROUP_CHAT_MSG
};
#endif