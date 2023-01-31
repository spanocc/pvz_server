// 不采用ET模式，而是每次读，只读一个message大小的报文，不一次全读完，因为有多个事件，不像http服务器那样，一次就一个事件，之后的数据直接不管了
// 对于写数组，采用非阻塞写，假设都能写成功，不会写一半

// 每个玩家单独的东西，就在本地实现，比如阳光，CD, 植物残影，植物卡片的亮度
// 共同的东西，由服务器安排，比如说地图上植物的分布，僵尸， 子弹
// 先把不需要发送的事件在本地过滤掉，比如在重复的地方放植物，铲子铲空气，点击未冷却的的植物卡片等（这些在本地的东西）

// 像僵尸，阳光，子弹这种东西可以放在一个数组里，数组元素是这些对象的指针，传送报文时，传送这些对象的数组序号，来操作这些对象 （服务器和每个客户端都要有这样一个数组）
// 由服务器来给这些对象分配数组下标，可以将可用的序号放在队列里，每分配出一个序号，就把这个元素从队列中移除，对象销毁，就把这个元素序号放回去

// 僵尸，阳光，子弹的位置放在报文里发送过来， 一个报文只含有一个对象的一个位置，有几个对象，定时器一到，就发几个报文    （不用这种方法了，感觉不可行）

// 在地图上种植物这些需要给所有玩家同步的操作，由主线程去做，然后发给每个子线程，每个子线程再发给每个用户


// ************************************************************
// 只需要做到，产生僵尸和阳光的时间一样，种下植物的时间一样，植物和僵尸销毁的时间一样 即可实现同步，其他的都靠客户端自己来实现，因为客户端是完全一致的
// 僵尸的产生实现用定时器+信号+统一事件源，就作为一个练习

// 似乎用半同步半反应堆来实现，更加合适，同步操作直接就由主线程做完，其他操作再放到任务队列里

#ifndef PVZ_SERVER_H
#define PVZ_SERVER_H

#include "message.h"
#include "socket_config.h"
#include "pvz.h"

extern const char *plant_name[];

// 由线程负责关闭连接

class PVZServer {
  public:
    ~PVZServer();
    void Init(int sockfd, int user_idx, int pipefd[2]);

    // 恢复状态
    void Reset();
    // 从套接字中读数据
    int Read();
    // 处理读进来的数据
    int ProcessRead();
    // 发送数据
    int Write();
    // 处理将要发送的报文
    int ProcessWrite(const Message& message);

    int user_idx_ = -1; // 该用户在线程的users数组中的编号
    int sockfd_ = -1;
    int pipefd_;

  private:

    Message read_message_;
    int read_message_offset_; // 读进来的数据相较于message_首部字段的偏移量
    Message write_message_;
    int write_message_offset_;

};


#endif