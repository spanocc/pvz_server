#ifndef SOCKET_CONFIG_H
#define SOCKET_CONFIG_H

#include <sys/types.h>
#include <sys/socket.h>
#include <sys/epoll.h>
#include <sys/wait.h>
#include <sys/stat.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <assert.h>
#include <errno.h>
#include <fcntl.h>
#include <pthread.h>
#include <cstdlib>
#include <cstdio>
#include <cstring>
#include <iostream>
#include <exception>
#include <stdexcept>

constexpr int MAX_THREAD_NUM = 8; 
constexpr int MAX_USER_NUM = 65536;	
constexpr int MAX_EVENT_NUM = 10000; // epoll能处理的最大事件数

// 设置fd为非阻塞,返回旧的选项
int SetNoBlocking(const int& fd);
// 把fd上的可读事件注册到事件表上,设置为ET模式
void AddEpollIn(const int& epollfd, const int& fd); 

#endif