#include "socket_config.h"

int SetNoBlocking(const int& fd) {
    const int& old_option = fcntl(fd, F_GETFL);
    const int& new_option = old_option | O_NONBLOCK;
    fcntl(fd, F_SETFL, new_option);
    return old_option;  // 返回旧的选项方便于恢复
}
// 在此函数中不调用SetNoBlocking
void AddEpollIn(const int& epollfd, const int& fd) {
    epoll_event event;
    event.data.fd = fd;
    event.events = EPOLLIN; // 注册可读事件，不设置ET模式，可以重复读，不用设置EPOLLONESHOT事件，因为线程池的实现中一个socket连接只由一个线程处理
    epoll_ctl(epollfd, EPOLL_CTL_ADD, fd, &event);
}