#ifndef MESSAGE_H
#define MESSAGE_H

static const char* magic_str = "yuriyuri";

enum MessageType {
    CONNECT_SOCKET = 0, 
    CLOSE_CONNECTION,
    SIGNAL_ALARM,

    
    SIGNAL_CREATE_PLANT, RESPOND_CREATE_PLANT, // 放置一个植物的事件，（点击graphblock块）
    SIGNAL_DESTROY_PLANT, RESPOND_DESTROY_PLANT, 
    CREATE_ZOMBIE, PRODUCE_SUN,
    VICTORY, DEFEAT, GAME_START

};

// 客户端和服务器之间的通信报文(发送各种事件)
struct Message {
    char magic[16];
    int message_type;
    // 事件发生的行，列
    union {
        int line;
        int y;
    };
    union {
        int column; 
        int x;
    };
    union {
        int plant_type;
        int zombie_type;
    };
    int sockfd; // 客户端发送者的sockfd,用于区分，方便设置回应(respond)字段
    int seq; // 所要销毁（或者创建）的植物的序号
    bool respond; // 是不是事件发生的回应报文
};

#endif