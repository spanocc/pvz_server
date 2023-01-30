// 不采用ET模式，而是每次读，只读一个message大小的报文，不一次全读完，因为有多个事件，不像http服务器那样，一次就一个事件，之后的数据直接不管了
// 对于写数组，采用非阻塞写，假设都能写成功，不会写一半

// 每个玩家单独的东西，就在本地实现，比如阳光，CD, 植物残影，植物卡片的亮度
// 共同的东西，由服务器安排，比如说地图上植物的分布，僵尸， 子弹
// 先把不需要发送的事件在本地过滤掉，比如在重复的地方放植物，铲子铲空气，点击未冷却的的植物卡片等（这些在本地的东西）

// 像僵尸，阳光，子弹这种东西可以放在一个数组里，数组元素是这些对象的指针，传送报文时，传送这些对象的数组序号，来操作这些对象 （服务器和每个客户端都要有这样一个数组）
// 由服务器来给这些对象分配数组下标，可以将可用的序号放在队列里，每分配出一个序号，就把这个元素从队列中移除，对象销毁，就把这个元素序号放回去

#ifndef PVZ_SERVER_H
#define PVZ_SERVER_H

#include "message.h"
#include "socket_config.h"

// 由线程负责关闭连接

class PVZServer {
  public:
    ~PVZServer();
    void Init(int sockfd, int user_idx);

    // 恢复状态
    void Reset();
    // 从套接字中读数据
    int Read();
    // 处理读进来的数据
    int ProcessRead();
    // 发送数据
    int Write();

    int user_idx_ = -1; // 该用户在线程的users数组中的编
    int sockfd_ = -1;

  private:

    Message read_message_;
    int read_message_offset_; // 读进来的数据相较于message_首部字段的偏移量
    Message write_message_;
    int write_message_offset_;

};


#endif