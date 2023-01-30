#ifndef MESSAGE_H
#define MESSAGE_H

enum ThreadMessageType {
    CONNECT_SOCKET = 0, 
};

// 线程间通信发送的信息类
struct ThreadMessage {
    int message_type;
    union {
        
    };
};

// 客户端和服务器之间的通信报文(发送各种事件)
struct Message {
    char magic[16];
};

#endif