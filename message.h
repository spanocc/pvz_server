#ifndef MESSAGE_H
#define MESSAGE_H

static const char* magic_str = "yuriyuri";

enum MessageType {
    CONNECT_SOCKET = 0, 
    CLOSE_CONNECTION,

    
    SIGNAL_CREATE_PLANT, RESPOND_CREATE_PLANT, // 放置一个植物的事件，（点击graphblock块）
    SIGNAL_DESTROY_PLANT, RESPOND_DESTROY_PLANT, 

};

// 客户端和服务器之间的通信报文(发送各种事件)
struct Message {
    char magic[16];
    int message_type;
    int line, column; // 事件发生的行，列
    int plant_type;
    int sockfd; // 发送者的sockfd,用于区分，方便设置回应字段
    int seq; // 所要销毁（或者创建）的植物的序号
    bool respond; // 是不是事件发生的回应报文
};

#endif