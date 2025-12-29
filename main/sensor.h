#ifndef SENSOR_H
#define SENSOR_H

#include "esp_err.h"

// 启动传感器上报任务
esp_err_t sensor_task_start(void);

#endif // SENSOR_H