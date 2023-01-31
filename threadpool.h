// 半同步半异步线程池 (一个客户连接上的所有任务始终由一个线程来处理)
// epoll
#ifndef THREADPOOL_H
#define THREADPOOL_H

#include "pvz_server.h"

// 专门用一个线程负责定时任务？， 或者就让主线程负责定时任务，反正主线程也要向所有用户发信息，统一事件源，把定时任务通过sig_pipe发给处理定时任务的线程

template <typename T>
class Thread {
  public:
	Thread();
	~Thread();
	// 创建用户数组
	void Init(T *users, int listenfd, int thread_idx);
	// 关闭一个用户连接 user_idx是该用户在users数组中的索引
	void CloseConnection(int user_idx); 	
	// 线程工作函数（静态函数）
    static void *ThreadWork(void *arg);

	// 处理主线程传过来的信息
    int ProcessThreadMessage(const Message& message);

	pthread_t tid_ = -1; // 线程id
	int pipefd_[2]; // 监听线程与线程池中的线程通信,管道是线程安全的（一个线程从一侧写，另一个线程从另一侧读）主线程是0,其余线程是1, 只注册读，不注册写(写是直接写)
	int thread_idx_ = -1; // 线程编号
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
    ThreadPool(const int& listenfd, const int& thread_num);
    ~ThreadPool();
    // 运行主线程
    void Run();
	// 初始化锁
	void MutexInit();
	// 处理子线程传过来的信息
	int ProcessThreadMessage(const Message& message);
  private:

    Thread<T> *threads_ = nullptr;
	T *users_ = nullptr; // 以连接socket的值作为索引

    int thread_num_ = 0; // 线程数
	int current_thread_ = 0; // 该分配哪个线程了
    int listenfd_ = -1;

	// pthread_mutex_t  mutex_graph_;
	int graph_[LINE_NUM][COLUMN_NUM]; // 只由主线程访问，似乎不用上锁
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
    AddEpollIn(epollfd_, pipefd_[1], false);
}

template <typename T>
Thread<T>::~Thread() {
	close(pipefd_[0]);
	close(pipefd_[1]);
	close(epollfd_);

	// 把所有连接断开
	int user_num = user_num_;
	for(int i = 0; i < user_num; ++i) {
		CloseConnection(fd_table_[0]); // 删除第一个之后，后面的会顶上来
	}
}

template <typename T>
void Thread<T>::Init(T *users, int listenfd, int thread_idx) {
	users_ = users;
	listenfd_ = listenfd;
	thread_idx_ = thread_idx;
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
				// 管道中的数据就正常读即可， 不用怕一次没读完
				Message message;
				int ret = recv(sockfd, (char *)(&message), sizeof(message), 0);
				if(ret < 0 && errno != EAGAIN) {
					throw std::runtime_error("Thread: recv failue");
				} else if(ret > 0) {
					assert(ret == sizeof(message));
					if(strcmp(message.magic, magic_str)) {
						throw std::runtime_error("Thread: magic failure");
					}
					if(!ProcessThreadMessage(message)) {
						return; // 失败就结束（也可以不结束，我的有点极端，防止出错误）
					}
				} else if(ret == 0) {
					// 关闭管道是在析构函数中实现的
					continue;
				}
			} else if(events[i].events & EPOLLIN) { // 其他可读数据，只能是用户发来的
				if(!users_[sockfd].Read() || !users_[sockfd].ProcessRead()) {
					// 出错，或者对方已经关闭连接,就断开这个用户的连接
					if(user_num_ > 0 && users_[sockfd].sockfd_ != -1 && users_[sockfd].user_idx_ != -1) {
						assert(fd_table_[users_[sockfd].user_idx_] == users_[sockfd].sockfd_);
						CloseConnection(users_[sockfd].user_idx_);
					}
				} 
			}
		}
	}
}

// 不能随便用const int& , 要保证原变量在执行函数时不可以改变， 传入的实参 users_[sockfd].user_idx_ 可能改变了
template <typename T>
void Thread<T>::CloseConnection(int user_idx) {  
	assert(user_num_ > 0);
	assert(user_idx >= 0 && user_idx < user_num_);
	assert(users_[fd_table_[user_idx]].user_idx_ == user_idx);
	int sockfd = users_[fd_table_[user_idx]].sockfd_;    
	if(sockfd != -1) {
		epoll_ctl(epollfd_, EPOLL_CTL_DEL, sockfd, 0);
		close(sockfd);  
		users_[fd_table_[user_idx]].sockfd_ = -1; 
		users_[fd_table_[user_idx]].user_idx_ = -1;  
		fd_table_[user_idx] = fd_table_[--user_num_]; // 把后面的顶上来
		users_[fd_table_[user_idx]].user_idx_ = user_idx;                   
	}
	std::cout<<"thread "<<thread_idx_<<": close a user connection(socket: "<<sockfd<<"), current users num = "<<user_num_<<"\n";
}

template<typename T>
int Thread<T>::ProcessThreadMessage(const Message& message) {

	if(message.message_type == CONNECT_SOCKET) { // 有新连接建立
		struct sockaddr_in client_address;
		socklen_t client_address_len = sizeof(client_address);
		int connfd = accept(listenfd_, (struct sockaddr*)(&client_address), &client_address_len);
		if(connfd < 0) {
			throw std::runtime_error("Thread: accept failure");
		}
		AddEpollIn(epollfd_, connfd, false);
		users_[connfd].Init(connfd, user_num_, pipefd_);
		fd_table_[user_num_++] = connfd;
		std::cout<<"thread "<<thread_idx_<<": connect successfully, add a user(socket: "<<connfd<<"), current users num = "<<user_num_<<"\n";
	} else if(message.message_type == RESPOND_CREATE_PLANT){
		// 收到回应放置植物的信息，就给本线程的所有客户发送一个报文
		for(int i = 0; i < user_num_; ++i) {
			if(!users_[fd_table_[i]].ProcessWrite(message)) {
				throw std::runtime_error("Thread: prosess write failure");
			}
		}
	} 

	return true;
}



// 线程池

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
		threads_[i].Init(users_, listenfd_, i);
        int ret = 0;
        ret = pthread_create(&(threads_[i].tid_), NULL, threads_[i].ThreadWork, &threads_[i]);
        assert(ret == 0);
        ret = pthread_detach(threads_[i].tid_);
        assert(ret == 0);
    }

	for(int i = 0; i < LINE_NUM; ++i) {
		for(int j= 0; j < COLUMN_NUM; ++j) {
			graph_[i][j] = 0; // 初始化地图为空
		}
	}
	MutexInit();
}

template <typename T>
ThreadPool<T>::~ThreadPool() {
	//pthread_mutex_destroy(&mutex_graph_);
    delete [] threads_;
	delete [] users_;
}

template <typename T>
void ThreadPool<T>::MutexInit() {
	// pthread_mutex_init(&mutex_graph_, NULL);
}

template <typename T>
void ThreadPool<T>::Run() {
    // 主线程监听socket
    epoll_event events[MAX_EVENT_NUM];
    int epollfd = epoll_create(1024);
    assert(epollfd != -1);
    SetNoBlocking(epollfd);
    AddEpollIn(epollfd, listenfd_, true); // 监听套接字要设置为ET,因为事件不会马上处理，所以让他只响应一次，防止同一个连接被多个线程处理
	// 注册从与各个线程通信的管道读的事件
	for(int i = 0; i < thread_num_; ++i) {
		AddEpollIn(epollfd, threads_[i].pipefd_[0], false);
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
				Message message;
				strncpy(message.magic, magic_str, sizeof(message.magic) - 1);
				message.message_type = CONNECT_SOCKET;
				int ret = send(threads_[current_thread_].pipefd_[0], (char *)(&message), sizeof(message), 0);
				if(ret != sizeof(message)) {
					throw std::runtime_error("ThreadPool: send failure");
				}
				std::cout<<"signal thread "<<current_thread_<<" to accept the connect request\n";
				current_thread_ = (current_thread_ + 1) % thread_num_;
			} else if(events[i].events & EPOLLIN){
				// 否则就是从子线程发送过来的信息
				Message message;
				int ret = recv(sockfd, &message, sizeof(message), 0);
				assert(ret == sizeof(message));
				if(strcmp(message.magic, magic_str)) {
					throw std::runtime_error("Thread: magic failure");
				}
				if(!ProcessThreadMessage(message)) {
					break;
				}
			}
		}
    }

	close(epollfd);
}


template <typename T>
int ThreadPool<T>::ProcessThreadMessage(const Message& message) {  
	if(message.message_type == SIGNAL_CREATE_PLANT) {
		// pthread_mutex_lock(&mutex_graph_);
		int line = message.line, column = message.column;
		if(line >= 0 && line < LINE_NUM && column >= 0 && column < COLUMN_NUM) {
			if(graph_[line][column] == 0) {
				graph_[line][column] = 1;
				Message new_message = message;
				new_message.message_type = RESPOND_CREATE_PLANT; 
				for(int i = 0; i < thread_num_; ++i) {
					int ret = send(threads_[i].pipefd_[0], &new_message, sizeof(new_message), 0);
					assert(ret == sizeof(new_message));
				}
				std::cout<<"create a "<<plant_name[new_message.plant_type]<<" at ("<<new_message.line<<", "<<new_message.column<<")\n";
			}
		}
		// pthread_mutex_unlock(&mutex_graph_);
	} else {

	}

	return true;
}


#endif