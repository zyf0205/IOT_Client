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
#include "dht.h"

#define DHT_SENSOR_GPIO GPIO_NUM_4

static const char *TAG = "SENSOR";

// 温湿度上报
static void sensor_task(void *pvParameters)
{
  float temperature = 0;
  float humidity = 0;
  while (1)
  {
    vTaskDelay(pdMS_TO_TICKS(3000));
    esp_err_t res = dht_read_float_data(DHT_TYPE_DHT11, DHT_SENSOR_GPIO, &humidity, &temperature);
    if (res == ESP_OK)
    {
      ESP_LOGI(TAG, "湿度: %.0f%%, 温度: %.0f°C", humidity, temperature);
    }
    else
    {
      ESP_LOGE(TAG, "读取传感器失败: %s", esp_err_to_name(res));
    }

    if (ws_is_connected())
    {
      // 构造 Payload: Temp(4) + Hum(4) + Time(8) = 16 Bytes
      uint8_t payload[16];
      memset(payload, 0, 16);

      write_u32_le(&payload[0], temperature);
      write_u32_le(&payload[4], humidity);

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