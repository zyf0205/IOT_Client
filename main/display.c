#include "display.h"
#include "driver/i2c.h"
#include "ssd1306.h"
#include "ssd1306_fonts.h"
#include "esp_log.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include <stdio.h>

static const char *TAG = "DISPLAY";

// ================= 配置区域 =================
// 可以在这里自由选择引脚 (ESP32 硬件I2C支持任意引脚映射)
#define I2C_MASTER_SCL_IO 22      /*!< SCL 引脚 */
#define I2C_MASTER_SDA_IO 23      /*!< SDA 引脚 */
#define I2C_MASTER_NUM 0          /*!< I2C 端口号 */
#define I2C_MASTER_FREQ_HZ 400000 /*!< I2C 频率 */
#define SSD1306_ADDR 0x3C         /*!< OLED 地址 */
// ===========================================

// 全局系统数据实例
system_data_t sys_data = {
    .temperature = 0.0f,
    .humidity = 0.0f,
    .lux = 0.0f,
    .led_status = false,
    .is_connected = false};

static ssd1306_handle_t ssd1306_dev = NULL;

/**
 * @brief 初始化 I2C 总线 (硬件 I2C，但引脚可自定义)
 */
static void i2c_bus_init(void)
{
  i2c_config_t conf = {
      .mode = I2C_MODE_MASTER,
      .sda_io_num = I2C_MASTER_SDA_IO,
      .scl_io_num = I2C_MASTER_SCL_IO,
      .sda_pullup_en = GPIO_PULLUP_ENABLE,
      .scl_pullup_en = GPIO_PULLUP_ENABLE,
      .master.clk_speed = I2C_MASTER_FREQ_HZ,
  };
  ESP_ERROR_CHECK(i2c_param_config(I2C_MASTER_NUM, &conf));
  ESP_ERROR_CHECK(i2c_driver_install(I2C_MASTER_NUM, conf.mode, 0, 0, 0));
}


static void display_task(void *arg)
{
  char str_buf[32];

  // 1. 初始化 I2C
  i2c_bus_init();
  ESP_LOGI(TAG, "I2C initialized on SDA:%d SCL:%d", I2C_MASTER_SDA_IO, I2C_MASTER_SCL_IO);

  // 2. 创建 SSD1306 设备
  ssd1306_dev = ssd1306_create(I2C_MASTER_NUM, SSD1306_ADDR);
  ssd1306_refresh_gram(ssd1306_dev);
  ssd1306_clear_screen(ssd1306_dev, 0x00);

  while (1)
  {

    ssd1306_clear_screen(ssd1306_dev, 0x00);

    // 标题栏
    ssd1306_draw_string(ssd1306_dev, 0, 0, (const uint8_t *)"IOT Monitor", 16, 1);

    // 网络状态
    if (sys_data.is_connected)
    {
      ssd1306_draw_string(ssd1306_dev, 90, 0, (const uint8_t *)"LINK", 12, 1);
    }
    else
    {
      ssd1306_draw_string(ssd1306_dev, 90, 0, (const uint8_t *)"----", 12, 1);
    }

    // 温度
    snprintf(str_buf, sizeof(str_buf), "Temp: %.1f C", sys_data.temperature);
    ssd1306_draw_string(ssd1306_dev, 0, 18, (const uint8_t *)str_buf, 12, 1);

    // 湿度
    snprintf(str_buf, sizeof(str_buf), "Humi: %.1f %%", sys_data.humidity);
    ssd1306_draw_string(ssd1306_dev, 0, 30, (const uint8_t *)str_buf, 12, 1);

    // 光照
    snprintf(str_buf, sizeof(str_buf), "Lux : %.0f", sys_data.lux);
    ssd1306_draw_string(ssd1306_dev, 0, 42, (const uint8_t *)str_buf, 12, 1);

    // LED 状态
    snprintf(str_buf, sizeof(str_buf), "LED : %s", sys_data.led_status ? "ON" : "OFF");
    ssd1306_draw_string(ssd1306_dev, 0, 54, (const uint8_t *)str_buf, 12, 1);

    // 刷新显存到屏幕
    ssd1306_refresh_gram(ssd1306_dev);

    // 1秒刷新一次
    vTaskDelay(pdMS_TO_TICKS(1000));
  }
}

void display_task_start(void)
{
  xTaskCreate(display_task, "display_task", 4096, NULL, 5, NULL);
}