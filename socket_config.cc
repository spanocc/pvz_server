#include "socket_config.h"
#include "message.h"

extern int sig_pipefd[2];

int SetNoBlocking(const int& fd) {
    const int& old_option = fcntl(fd, F_GETFL);
    const int& new_option = old_option | O_NONBLOCK;
    fcntl(fd, F_SETFL, new_option);
    return old_option;  // 返回旧的选项方便于恢复
}
// 在此函数中不调用SetNoBlocking
void AddEpollIn(const int& epollfd, const int& fd, bool set_et) {
    epoll_event event;
    event.data.fd = fd;
    event.events = EPOLLIN; // 注册可读事件，不设置ET模式，可以重复读(因为一次只读一个message大小，不全读完)，不用设置EPOLLONESHOT事件，因为线程池的实现中一个socket连接只由一个线程处理
    if(set_et) {
        event.events |= EPOLLET;
    }
    epoll_ctl(epollfd, EPOLL_CTL_ADD, fd, &event);
}

void SigHandler(int sig) {
    int save_errno = errno;
    Message message;
    message.message_type = SIGNAL_ALARM;
    strncpy(message.magic, magic_str, sizeof(message.magic) - 1);

	int ret = send(sig_pipefd[1], &message, sizeof(message), 0);
	assert(ret == sizeof(message));

    errno = save_errno;

}

void SetSig(int sig) {
    struct sigaction sa;
    memset(&sa, '\0', sizeof(sa));
    sa.sa_handler = SigHandler;
    sa.sa_flags |= SA_RESTART; // 重启被信号打断的处于阻塞状态的系统调用，eg : epoll_wait
    sigfillset(&sa.sa_mask); // 处理信号处理函数时，阻挡其他信号
    assert(sigaction(sig, &sa, NULL) != -1);
}