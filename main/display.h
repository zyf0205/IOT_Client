#ifndef DISPLAY_H
#define DISPLAY_H

#include <stdbool.h>
#include <stdint.h>

// 定义系统状态数据结构
typedef struct
{
  float temperature;
  float humidity;
  float lux;
  bool led_status;
  bool is_connected; // WebSocket连接状态
} system_data_t;

// 声明全局变量，供其他文件访问
extern system_data_t sys_data;

// 初始化并启动显示任务
void display_task_start(void);

#endif