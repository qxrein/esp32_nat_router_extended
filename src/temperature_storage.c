#include "temperature_storage.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

// Global variables to store sensor data
static float g_temperature = 0.0f;
static float g_humidity = 0.0f;
static uint32_t g_last_update_time = 0;
static bool g_data_received = false;

void temperature_storage_update(float temperature, float humidity)
{
    g_temperature = temperature;
    g_humidity = humidity;
    g_last_update_time = xTaskGetTickCount() * portTICK_PERIOD_MS;
    g_data_received = true;
}

float temperature_storage_get_temperature(void)
{
    return g_temperature;
}

float temperature_storage_get_humidity(void)
{
    return g_humidity;
}

bool temperature_storage_is_data_valid(void)
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

uint32_t temperature_storage_get_last_update_time(void)
{
    return g_last_update_time;
}

