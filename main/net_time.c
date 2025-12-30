#include <time.h>
#include <sys/time.h>
#include "esp_sntp.h" // 确保包含这个头文件
#include "esp_log.h"

static const char *TAG = "NTP_TIME";

void net_time_init(void)
{
  ESP_LOGI(TAG, "Initializing SNTP");
  
  // 修改前: sntp_setoperatingmode(SNTP_OPMODE_POLL);
  esp_sntp_setoperatingmode(SNTP_OPMODE_POLL);

  // 修改前: sntp_setservername(0, "ntp.aliyun.com");
  esp_sntp_setservername(0, "ntp.aliyun.com");

  // 修改前: sntp_setservername(1, "pool.ntp.org");
  esp_sntp_setservername(1, "pool.ntp.org");

  // 修改前: sntp_init();
  esp_sntp_init();
}

uint64_t net_get_time_ms(void)
{
  struct timeval tv_now;
  gettimeofday(&tv_now, NULL);

  // 如果时间没同步（比如还是1970年），返回0或者做特殊处理
  if (tv_now.tv_sec < 1600000000)
  {
    return 0;
  }

  // 转换为毫秒: 秒*1000 + 微秒/1000
  return (uint64_t)tv_now.tv_sec * 1000 + (uint64_t)tv_now.tv_usec / 1000;
}