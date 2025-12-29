#ifndef NET_TIME_H
#define NET_TIME_H

#include <stdint.h>

// 初始化 SNTP 服务
void net_time_init(void);

// 获取当前时间戳（毫秒级，Unix Epoch）
uint64_t net_get_time_ms(void);

#endif