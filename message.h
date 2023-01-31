#ifndef MESSAGE_H
#define MESSAGE_H

enum ThreadMessageType {
    CONNECT_SOCKET = 0, 
    CLOSE_CONNECTION,
};

enum MessageType {
    CREATE_PLANT = 0, // 放置一个植物的事件，（点击graphblock块） 
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
    int message_type;
    int line, column; // 事件发生的行，列
    union {
        int plant_type;
    };
};

// 客户端 主线程和子线程通信的参数
struct SignalMessage {
    int message_type;
    int line, column;
    union {
        int plant_type;
    };
};

#endif