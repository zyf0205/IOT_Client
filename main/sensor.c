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
// [新增] ADC 驱动
#include "esp_adc/adc_oneshot.h"

#define DHT_SENSOR_GPIO GPIO_NUM_4
// [新增] 光敏传感器 GPIO (GPIO 34 对应 ADC1_CHANNEL_6)
#define LIGHT_SENSOR_ADC_CHANNEL ADC_CHANNEL_6 

static const char *TAG = "SENSOR";

// 温湿度上报
static void sensor_task(void *pvParameters)
{
  // --- [新增] ADC 初始化开始 ---
  adc_oneshot_unit_handle_t adc1_handle;
  adc_oneshot_unit_init_cfg_t init_config1 = {
      .unit_id = ADC_UNIT_1, // Wi-Fi 开启时必须用 ADC1
  };
  ESP_ERROR_CHECK(adc_oneshot_new_unit(&init_config1, &adc1_handle));

  adc_oneshot_chan_cfg_t config = {
      .bitwidth = ADC_BITWIDTH_DEFAULT, // 默认 12位 (0-4095)
      .atten = ADC_ATTEN_DB_12,         // 12dB 衰减，支持 0-3.3V 电压
  };
  ESP_ERROR_CHECK(adc_oneshot_config_channel(adc1_handle, LIGHT_SENSOR_ADC_CHANNEL, &config));
  // --- [新增] ADC 初始化结束 ---

  float temperature = 0;
  float humidity = 0;
  int light_raw = 0; // [新增] 存储光照值

  while (1)
  {
    vTaskDelay(pdMS_TO_TICKS(3000));

    // 1. 读取温湿度
    esp_err_t res = dht_read_float_data(DHT_TYPE_DHT11, DHT_SENSOR_GPIO, &humidity, &temperature);
    
    // 2. [新增] 读取光照 (ADC)
    ESP_ERROR_CHECK(adc_oneshot_read(adc1_handle, LIGHT_SENSOR_ADC_CHANNEL, &light_raw));

    if (res == ESP_OK)
    {
      // 打印包含光照数据
      ESP_LOGI(TAG, "湿度: %.1f%%, 温度: %.1f°C, 光照: %d", humidity, temperature, light_raw);
    }
    else
    {
      ESP_LOGE(TAG, "读取传感器失败: %s", esp_err_to_name(res));
    }

    if (ws_is_connected())
    {
      // 构造 Payload: Temp(4) + Hum(4) + Light(4) + Time(8) = 20 Bytes
      uint8_t payload[20];
      memset(payload, 0, 20);

      // [修改] 使用 memcpy 发送浮点数，保留精度
      memcpy(&payload[0], &temperature, sizeof(float));
      memcpy(&payload[4], &humidity, sizeof(float));

      // [新增] 写入光照数据 (int -> 4 bytes)
      write_u32_le(&payload[8], (uint32_t)light_raw);

      uint64_t now_ms = net_get_time_ms();
      if (now_ms == 0)
      {
        ESP_LOGW(TAG, "Time not synced yet!");
      }
      
      // [修改] 时间戳偏移量向后移动 4 字节 (从 index 12 开始)
      write_u32_le(&payload[12], (uint32_t)(now_ms & 0xFFFFFFFF));
      write_u32_le(&payload[16], (uint32_t)(now_ms >> 32));

      // 发送长度改为 20
      ws_send_packet(CMD_REPORT, payload, 20);
    }
  }
  
  // 任务清理 (虽然通常不会执行到这里)
  adc_oneshot_del_unit(adc1_handle);
  vTaskDelete(NULL);
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