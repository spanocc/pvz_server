#include "pvz_server.h"

PVZServer::~PVZServer() {
    
}

void PVZServer::Init(int sockfd, int user_idx) {
    sockfd_ = sockfd;
    user_idx_ = user_idx;
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
    } else {
        // std::cout<<read_message_.magic<<std::endl;
        Write();
        Reset();
    }
    return true;
}

// 经过证明，在槽函数执行过程中，不会被其他信号打断
// 一次发就全发完
int PVZServer::Write() {
    strcpy(write_message_.magic, "yuriyuri");
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