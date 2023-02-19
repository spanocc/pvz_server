#ifndef ZOMBIE_TIMER_H
#define ZOMBIE_TIMER_H

#include <time.h>
#include <random>

struct ZombieTimer {
    time_t timeout; // 距离游戏开始多长时间，出现该僵尸
    int zombie_type; // 该僵尸的类型
    int line; // 该僵尸出现的行数
};

#endif