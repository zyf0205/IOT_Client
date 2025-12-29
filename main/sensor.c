#include "sensor.h"
#include "websocket_client.h"
#include "protocol.h"
#include "net_time.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_system.h"
#include "esp_random.h"
#include "esp_log.h"
#include <string.h>

static const char *TAG = "SENSOR";

// 任务：模拟温湿度上报
static void sensor_task(void *pvParameters)
{
  uint32_t temp = 25; // 25摄氏度
  uint32_t hum = 60;  // 60%

  while (1)
  {
    // 等待3秒
    vTaskDelay(pdMS_TO_TICKS(3000));

    if (ws_is_connected())
    {
      // 模拟数据波动
      temp = 20 + (esp_random() % 10);
      hum = 40 + (esp_random() % 30);

      // 构造 Payload: Temp(4) + Hum(4) + Time(8) = 16 Bytes
      uint8_t payload[16];
      memset(payload, 0, 16);

      write_u32_le(&payload[0], temp);
      write_u32_le(&payload[4], hum);

      uint64_t now_ms = net_get_time_ms();
      if (now_ms == 0)
      {
        ESP_LOGW(TAG, "Time not synced yet!");
      }
      // 将 64位 整数拆分为两个 32位 写入 (小端序)
      // 低32位
      write_u32_le(&payload[8], (uint32_t)(now_ms & 0xFFFFFFFF));
      // 高32位 (右移32位后强转)
      write_u32_le(&payload[12], (uint32_t)(now_ms >> 32));

      ws_send_packet(CMD_REPORT, payload, 16);
    }
  }
}

esp_err_t sensor_task_start(void)
{
  BaseType_t ret = xTaskCreate(sensor_task, "sensor_task", 4096, NULL, 5, NULL);
  if (ret != pdPASS)
  {
    ESP_LOGE(TAG, "Failed to create sensor task");
    return ESP_FAIL;
  }

  ESP_LOGI(TAG, "Sensor task started");
  return ESP_OK;
}