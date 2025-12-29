#ifndef WIFI_MGR_H
#define WIFI_MGR_H

#include "esp_err.h"

// WiFi 配置
#define WIFI_SSID "20pro"
#define WIFI_PASS "12345678"

// 初始化并连接WiFi
esp_err_t wifi_mgr_init(void);

#endif // WIFI_MGR_H