// 半同步半异步线程池 (一个客户连接上的所有任务始终由一个线程来处理)
// epoll
#ifndef THREADPOOL_H
#define THREADPOOL_H

#include "message.h"
#include "socket_config.h"


template <typename T>
class Thread {
  public:
	Thread();
	~Thread();
	// 创建用户数组
	void Init(T *users, int listenfd);
	// 线程工作函数（静态函数）
    static void *ThreadWork(void *arg);

	pthread_t tid_; // 线程id
	int pipefd_[2]; // 监听线程与线程池中的线程通信,管道是线程安全的（一个线程从一侧写，另一个线程从另一侧读）主线程是0,其余线程是1, 只注册读，不注册写
  private:
    // 实际线程最终调用
    void ThreadRun();

	T *users_ = nullptr; // 以连接socket的值作为索引

    int epollfd_ = -1; // 事件表id
	int user_num_ = 0; // 每个线程连接的用户数量
	int fd_table_[MAX_USER_NUM];  // 用户编号到该用户socket的映射
	int listenfd_ = -1;
};

template <typename T>
class ThreadPool {
  public:
    ThreadPool(const int& listenfd, const int& thread_num = 8);
    ~ThreadPool();
    // 运行主线程
    void Run();
  private:

    Thread<T> *threads_ = nullptr;
	T *users_ = nullptr; // 以连接socket的值作为索引

    int thread_num_ = 0; // 线程数
	int current_thread_ = 0; // 该分配哪个线程了
    int listenfd_ = -1;
};

// 模板类的函数定义要和声明放在同一个文件中
// 函数定义：

template <typename T>
Thread<T>::Thread() {
    // 初始化事件表
    epollfd_ = epoll_create(1024);
    assert(epollfd_ != -1);

    // 初始化线程间通信的管道
    int ret = socketpair(PF_UNIX, SOCK_STREAM, 0, pipefd_);
    assert(ret != -1);

    SetNoBlocking(pipefd_[0]);
    SetNoBlocking(pipefd_[1]);
    AddEpollIn(epollfd_, pipefd_[1]);
}

template <typename T>
Thread<T>::~Thread() {
	close(pipefd_[0]);
	close(pipefd_[1]);
	close(epollfd_);
}

template <typename T>
void Thread<T>::Init(T *users, int listenfd) {
	users_ = users;
	listenfd_ = listenfd;
}

template <typename T>
void *Thread<T>::ThreadWork(void *arg) {
	Thread *thread = static_cast<Thread<T> *>(arg);
	thread->ThreadRun();
	return thread;
}

template <typename T>
void Thread<T>::ThreadRun() {
	epoll_event events[MAX_EVENT_NUM];

	while(1) {
		int event_num = epoll_wait(epollfd_, events, MAX_EVENT_NUM, -1);
		if(event_num < 0 && errno != EINTR) { // EINTR是信号把epoll_wait系统调用中断了
			throw std::runtime_error("Thread: epoll failure");
		}

		for(int i = 0; i < event_num; ++i) {
			int sockfd = events[i].data.fd;

			if(sockfd == pipefd_[1] && (events[i].events & EPOLLIN)) {
				ThreadMessage message;
				int ret = recv(sockfd, (char *)(&message), sizeof(message), 0);
				if(ret < 0 && errno != EAGAIN) {
					throw std::runtime_error("Thread: recv failue");
				} else if(ret > 0) {
					if(message.message_type == CONNECT_SOCKET) { // 有新连接建立
						struct sockaddr_in client_address;
						socklen_t client_address_len = sizeof(client_address);
						int connfd = accept(listenfd_, (struct sockaddr*)(&client_address), &client_address_len);
						if(connfd < 0) {
							throw std::runtime_error("Thread: accept failure");
						}
						std::cout<<"connect successfully, add a user\n";
						AddEpollIn(epollfd_, connfd);
						fd_table_[user_num_++] = connfd;
					}
				} else if(ret == 0) {
					// 关闭管道是在析构函数中实现的
					continue;
				}
			} else if(events[i].events & EPOLLIN) { // 其他可读数据，只能是用户发来的
				char buf[65536];
				memset(buf, 0, sizeof(buf));
				int ret = recv(sockfd, buf, sizeof(buf) - 1, 0);
				if(ret == 0) {
					std::cout<<"ok\n";
				}
				std::cout<<buf<<std::endl;
				// send(sockfd, buf, 100, 0);
			}
		}
	}
}


template <typename T>
ThreadPool<T>::ThreadPool(const int& listenfd, const int& thread_num) 
    : thread_num_(thread_num),
      listenfd_(listenfd) {

    assert(thread_num_ > 0 && thread_num_ <= MAX_THREAD_NUM);
    if(thread_num_ <= 0 || thread_num_ > MAX_THREAD_NUM) {
		throw std::invalid_argument("ThreadPool: acquire the invalid argument(thread_num)");
    }
	users_ = new T [MAX_USER_NUM];
	assert(users_);
    threads_ = new Thread<T> [thread_num_];
    assert(threads_);

    for(int i = 0; i < thread_num_; ++i) {
		threads_[i].Init(users_, listenfd_);
        int ret = 0;
        ret = pthread_create(&(threads_[i].tid_), NULL, threads_[i].ThreadWork, &threads_[i]);
        assert(ret == 0);
        ret = pthread_detach(threads_[i].tid_);
        assert(ret == 0);
    }
}

template <typename T>
ThreadPool<T>::~ThreadPool() {
    delete [] threads_;
	delete [] users_;
}

template <typename T>
void ThreadPool<T>::Run() {
    // 主线程监听socket
    epoll_event events[MAX_EVENT_NUM];
    int epollfd = epoll_create(1024);
    assert(epollfd != -1);
    SetNoBlocking(epollfd);
    AddEpollIn(epollfd, listenfd_);
	// 注册从与各个线程通信的管道读的事件
	for(int i = 0; i < thread_num_; ++i) {
		AddEpollIn(epollfd, threads_[i].pipefd_[0]);
	}

    while(1) {
        int event_num = epoll_wait(epollfd, events, MAX_EVENT_NUM, -1);
		if(event_num < 0 && errno != EINTR) { // EINTR是信号把epoll_wait系统调用中断了
			throw std::runtime_error("ThreadPool: epoll failure");
		}
		for(int i = 0; i < event_num; ++i) {
			int sockfd = events[i].data.fd;

			if(sockfd == listenfd_) { // 有新的连接请求
				std::cout<<"receive a connect request\n";
				ThreadMessage message;
				message.message_type = CONNECT_SOCKET;
				int ret = send(threads_[current_thread_].pipefd_[0], (char *)(&message), sizeof(message), 0);
				if(ret < 0) {
					throw std::runtime_error("ThreadPool: send failure");
				}
				current_thread_ = (current_thread_ + 1) % thread_num_;
			} 
		}
    }

	close(epollfd);
}



#endif