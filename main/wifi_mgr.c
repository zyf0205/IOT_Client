#include "wifi_mgr.h"
#include "esp_wifi.h"
#include "esp_event.h"
#include "esp_log.h"
#include "freertos/event_groups.h"
#include "net_time.h"

static const char *TAG = "WIFI_MGR";
static EventGroupHandle_t s_wifi_event_group;
const int WIFI_CONNECTED_BIT = BIT0;

static void wifi_event_handler(void *arg, esp_event_base_t event_base, int32_t event_id, void *event_data)
{
  /*开启sta*/
  if (event_base == WIFI_EVENT && event_id == WIFI_EVENT_STA_START)
  {
    esp_wifi_connect(); /*连接wifi*/
  }
  /*wifi断开*/
  else if (event_base == WIFI_EVENT && event_id == WIFI_EVENT_STA_DISCONNECTED)
  {
    esp_wifi_connect(); /*重连wifi*/
    ESP_LOGI(TAG, "Retrying to connect to the AP");
  }
  /*连接wifi后自动获取到ip*/
  else if (event_base == IP_EVENT && event_id == IP_EVENT_STA_GOT_IP)
  {
    ip_event_got_ip_t *event = (ip_event_got_ip_t *)event_data;
    ESP_LOGI(TAG, "Got IP: " IPSTR, IP2STR(&event->ip_info.ip));
    net_time_init();
    /*置位WIFI_CONNECTED_BIT，让wifi_mgr_init返回*/
    xEventGroupSetBits(s_wifi_event_group, WIFI_CONNECTED_BIT);
  }
}

esp_err_t wifi_mgr_init(void)
{
  /*创建wifi事件组*/
  s_wifi_event_group = xEventGroupCreate();

  /*初始化tcp/ip协议栈*/
  ESP_ERROR_CHECK(esp_netif_init());

  /*创建默认事件循环，让注册的各类回调函数可以调用*/
  ESP_ERROR_CHECK(esp_event_loop_create_default());

  /*创建默认sta网络接口*/
  esp_netif_create_default_wifi_sta();

  /*初始化wifi*/
  wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();
  ESP_ERROR_CHECK(esp_wifi_init(&cfg));

  esp_event_handler_instance_t instance_any_id; /*保存注册得到的实例句柄*/
  esp_event_handler_instance_t instance_got_ip;

  /*为所有 WiFi 事件注册回调*/
  ESP_ERROR_CHECK(esp_event_handler_instance_register(WIFI_EVENT, ESP_EVENT_ANY_ID, &wifi_event_handler, NULL, &instance_any_id));

  /*为"获取 IP"事件注册同一个回调*/
  ESP_ERROR_CHECK(esp_event_handler_instance_register(IP_EVENT, IP_EVENT_STA_GOT_IP, &wifi_event_handler, NULL, &instance_got_ip));

  /*wifi参数设置*/
  wifi_config_t wifi_config = {
      .sta = {
          .ssid = WIFI_SSID,
          .password = WIFI_PASS,
      },
  };

  ESP_ERROR_CHECK(esp_wifi_set_mode(WIFI_MODE_STA));               /*sta模式*/
  ESP_ERROR_CHECK(esp_wifi_set_config(WIFI_IF_STA, &wifi_config)); /*应用wifi参数*/
  ESP_ERROR_CHECK(esp_wifi_start());                               /*启动wifi*/

  ESP_LOGI(TAG, "WiFi Init Finished. Waiting for IP...");

  /*等待wifi连接完成*/
  xEventGroupWaitBits(s_wifi_event_group, WIFI_CONNECTED_BIT, pdFALSE, pdFALSE, portMAX_DELAY);

  return ESP_OK;
}