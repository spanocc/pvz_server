#include "pvz_server.h"

PVZServer::~PVZServer() {
    
}

void PVZServer::Init(int sockfd, int user_idx, int pipefd[2]) {
    sockfd_ = sockfd;
    user_idx_ = user_idx;
    pipefd_ = pipefd[1];
    Reset();
}

void PVZServer::Reset() {
    read_message_offset_ = 0;
    write_message_offset_ = 0;
}

int PVZServer::Read() {
    int bytes_read = 0;
    while(1) { 
        if(read_message_offset_ >= sizeof(read_message_)) {
            return true; // 已经读进来一个报文了，直接返回
        }
        bytes_read = recv(sockfd_, &read_message_ + read_message_offset_, sizeof(read_message_) - read_message_offset_, 0);
        if(bytes_read == -1) {
            if(errno == EAGAIN) {
                return true; // 读完了
            }
            return false; // 
        } else if(bytes_read == 0) {
            return false; // 对方已经关闭连接
        }
        read_message_offset_ += bytes_read;
    }
}

int PVZServer::ProcessRead() {
    if(read_message_offset_ < sizeof(read_message_)) {
        return true; // 不够，继续读
    }
    if(strcmp(read_message_.magic, magic_str)) { // magic不对
        return false;
    }
    if(read_message_.message_type ==  SIGNAL_CREATE_PLANT) { // 通知要种植物， 就发给主线程
        read_message_.sockfd = sockfd_; // 把sockfd传过去，方便区分是不是回应报文
        int ret = send(pipefd_, (char *)(&read_message_), sizeof(read_message_), 0);
        assert(ret == sizeof(read_message_));
    } else if(read_message_.message_type == SIGNAL_DESTROY_PLANT) {
        read_message_.sockfd = sockfd_;
        int ret = send(pipefd_, (char *)(&read_message_), sizeof(read_message_), 0);
        assert(ret == sizeof(read_message_));
    }

    Reset();
    return true;
}

int PVZServer::Write() {
    strncpy(write_message_.magic, magic_str, sizeof(write_message_.magic) - 1);
    assert(write_message_offset_ == 0);
    int bytes_send = 0;
    while(1) {
        if(write_message_offset_ >= sizeof(write_message_)) {
            return true;
        }
        bytes_send = send(sockfd_, &write_message_ + write_message_offset_, sizeof(write_message_) - write_message_offset_, 0);
        if(bytes_send == -1) {
            if(errno == EAGAIN) { // 缓冲区暂时没有空间了， 继续等待有没有机会写
                continue;
            }
            return false;
        } else if(bytes_send == 0) {
            return false;
        }
        write_message_offset_ += bytes_send;
    }
}

int PVZServer::ProcessWrite(const Message& message) {
    assert(write_message_offset_ == 0);
    strncpy(write_message_.magic, "yuriyuri", sizeof(write_message_.magic) - 1);
    
    write_message_ = message;
    if(write_message_.sockfd == sockfd_) { // 说明是这个报文发来的请求，要加以回应
        write_message_.respond = true; 
    } else write_message_.respond = false;

    if(!Write()) { // 写失败了
        std::cout<<"write failue\n";
        return false;
    }
    Reset();
    return true;
}