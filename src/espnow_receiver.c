#include "espnow_receiver.h"
#include "esp_wifi.h"
#include "esp_now.h"
#include "esp_log.h"
#include "esp_err.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include <string.h>

static const char *TAG = "ESPNOW_RECEIVER";

// Global variables to store sensor data
static float g_temperature = 0.0f;
static float g_humidity = 0.0f;
static uint32_t g_last_update_time = 0;
static bool g_data_received = false;

// Callback function that will be executed when data is received
void esp_now_recv_cb(const esp_now_recv_info_t *recv_info, const uint8_t *data, int len)
{
    // Log sender MAC address for debugging
    if (recv_info != NULL)
    {
        ESP_LOGI(TAG, "ESP-NOW data received from: %02X:%02X:%02X:%02X:%02X:%02X", 
                 recv_info->src_addr[0], recv_info->src_addr[1], recv_info->src_addr[2],
                 recv_info->src_addr[3], recv_info->src_addr[4], recv_info->src_addr[5]);
    }
    
    ESP_LOGI(TAG, "Received ESP-NOW data, length: %d bytes (expected %d)", len, sizeof(struct_message));
    
    if (len == sizeof(struct_message))
    {
        struct_message *sensorData = (struct_message *)data;
        
        g_temperature = sensorData->temperature;
        g_humidity = sensorData->humidity;
        g_last_update_time = xTaskGetTickCount() * portTICK_PERIOD_MS;
        g_data_received = true;
        
        ESP_LOGI(TAG, "Received data - Temp: %.2f°C, Humidity: %.2f%%", 
                 g_temperature, g_humidity);
    }
    else
    {
        ESP_LOGW(TAG, "Received data with incorrect length: %d (expected %d)", 
                 len, sizeof(struct_message));
    }
}

void espnow_receiver_init(void)
{
    // Wait a bit for WiFi to be fully started
    vTaskDelay(pdMS_TO_TICKS(1000));
    
    // Check if WiFi is started
    wifi_mode_t wifi_mode;
    if (esp_wifi_get_mode(&wifi_mode) != ESP_OK)
    {
        ESP_LOGE(TAG, "Failed to get WiFi mode");
        return;
    }
    
    ESP_LOGI(TAG, "WiFi mode: %d (0=null, 1=STA, 2=AP, 3=APSTA)", wifi_mode);
    
    // Initialize ESP-NOW
    esp_err_t ret = esp_now_init();
    if (ret == ESP_ERR_ESPNOW_NOT_INIT)
    {
        ESP_LOGE(TAG, "ESP-NOW init failed: WiFi not initialized yet");
        return;
    }
    else if (ret == ESP_ERR_ESPNOW_EXIST)
    {
        ESP_LOGW(TAG, "ESP-NOW already initialized, deinitializing first");
        esp_now_deinit();
        vTaskDelay(pdMS_TO_TICKS(100));
        ret = esp_now_init();
        if (ret != ESP_OK)
        {
            ESP_LOGE(TAG, "ESP-NOW reinit failed: %s", esp_err_to_name(ret));
            return;
        }
    }
    else if (ret != ESP_OK)
    {
        ESP_LOGE(TAG, "ESP-NOW init failed: %s", esp_err_to_name(ret));
        return;
    }
    
    // Note: We don't set a fixed channel here to avoid interfering with router operation
    // ESP-NOW will use the current WiFi channel
    // Both ESP8266 and ESP32 should be on the same channel for communication
    
    // Register receive callback
    esp_err_t cb_ret = esp_now_register_recv_cb(esp_now_recv_cb);
    if (cb_ret != ESP_OK)
    {
        ESP_LOGE(TAG, "Failed to register ESP-NOW receive callback: %s", esp_err_to_name(cb_ret));
        esp_now_deinit();
        return;
    }
    
    // Get and log ESP32 MAC address for reference
    uint8_t mac[6];
    if (esp_wifi_get_mac(WIFI_IF_STA, mac) == ESP_OK)
    {
        ESP_LOGI(TAG, "ESP32 STA MAC Address: %02X:%02X:%02X:%02X:%02X:%02X", 
                 mac[0], mac[1], mac[2], mac[3], mac[4], mac[5]);
    }
    if (esp_wifi_get_mac(WIFI_IF_AP, mac) == ESP_OK)
    {
        ESP_LOGI(TAG, "ESP32 AP MAC Address: %02X:%02X:%02X:%02X:%02X:%02X", 
                 mac[0], mac[1], mac[2], mac[3], mac[4], mac[5]);
    }
    
    ESP_LOGI(TAG, "ESP-NOW receiver initialized successfully");
    ESP_LOGI(TAG, "Waiting for data from ESP8266...");
}

float espnow_get_temperature(void)
{
    return g_temperature;
}

float espnow_get_humidity(void)
{
    return g_humidity;
}

bool espnow_is_data_valid(void)
{
    if (!g_data_received)
    {
        return false;
    }
    
    // Check if data was received within last 30 seconds
    uint32_t current_time = xTaskGetTickCount() * portTICK_PERIOD_MS;
    uint32_t time_diff = current_time - g_last_update_time;
    
    return (time_diff < 30000); // 30 seconds
}

uint32_t espnow_get_last_update_time(void)
{
    return g_last_update_time;
}

