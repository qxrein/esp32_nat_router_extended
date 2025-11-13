#include "handler.h"
#include "espnow_receiver.h"
#include "temperature_storage.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include <time.h>
#include <sys/time.h>
#include <string.h>
#include <stdlib.h>

static const char *TAG = "TemperatureHandler";

esp_err_t temperature_get_handler(httpd_req_t *req)
{
    ESP_LOGI(TAG, "Requesting temperature page");
    if (isLocked())
    {
        return redirectToLock(req);
    }
    httpd_req_to_sockfd(req);

    extern const char temperature_start[] asm("_binary_temperature_html_start");
    extern const char temperature_end[] asm("_binary_temperature_html_end");
    const size_t temperature_html_size = (temperature_end - temperature_start);

    char *temperature_page = malloc(temperature_html_size + 1);
    memcpy(temperature_page, temperature_start, temperature_html_size);
    temperature_page[temperature_html_size] = '\0';

    closeHeader(req);

    esp_err_t ret = httpd_resp_send(req, temperature_page, HTTPD_RESP_USE_STRLEN);
    free(temperature_page);
    return ret;
}

esp_err_t temperature_api_post_handler(httpd_req_t *req)
{
    // Get content length
    size_t content_len = req->content_len;
    if (content_len > 256) {
        ESP_LOGE(TAG, "Content too long: %d", content_len);
        httpd_resp_set_status(req, "400 Bad Request");
        return httpd_resp_send(req, "Content too long", HTTPD_RESP_USE_STRLEN);
    }
    
    // Read POST data
    char buf[256];
    int ret = httpd_req_recv(req, buf, content_len);
    if (ret <= 0) {
        ESP_LOGE(TAG, "Failed to receive POST data");
        httpd_resp_set_status(req, "400 Bad Request");
        return httpd_resp_send(req, "Failed to receive data", HTTPD_RESP_USE_STRLEN);
    }
    buf[ret] = '\0';
    
    ESP_LOGI(TAG, "Received POST data: %s", buf);
    
    // Parse JSON data (simple parsing for temperature and humidity)
    float temp = 0.0f;
    float humidity = 0.0f;
    bool temp_parsed = false;
    bool humidity_parsed = false;
    
    // Try to parse JSON: {"temperature":23.20,"humidity":54.00}
    char *temp_str = strstr(buf, "\"temperature\"");
    char *humidity_str = strstr(buf, "\"humidity\"");
    
    if (temp_str != NULL) {
        temp_str = strchr(temp_str, ':');
        if (temp_str != NULL) {
            temp = strtof(temp_str + 1, NULL);
            temp_parsed = true;
        }
    }
    
    if (humidity_str != NULL) {
        humidity_str = strchr(humidity_str, ':');
        if (humidity_str != NULL) {
            humidity = strtof(humidity_str + 1, NULL);
            humidity_parsed = true;
        }
    }
    
    // Also try URL-encoded format: temperature=23.20&humidity=54.00
    if (!temp_parsed || !humidity_parsed) {
        char temp_param[32] = {0};
        char humidity_param[32] = {0};
        readUrlParameterIntoBuffer(buf, "temperature", temp_param, sizeof(temp_param));
        readUrlParameterIntoBuffer(buf, "humidity", humidity_param, sizeof(humidity_param));
        
        if (!temp_parsed && strlen(temp_param) > 0) {
            temp = strtof(temp_param, NULL);
            temp_parsed = true;
        }
        if (!humidity_parsed && strlen(humidity_param) > 0) {
            humidity = strtof(humidity_param, NULL);
            humidity_parsed = true;
        }
    }
    
    if (temp_parsed || humidity_parsed) {
        temperature_storage_update(temp, humidity);
        ESP_LOGI(TAG, "Updated temperature: %.2f°C, humidity: %.2f%%", temp, humidity);
        
        httpd_resp_set_type(req, "application/json");
        return httpd_resp_send(req, "{\"status\":\"ok\"}", HTTPD_RESP_USE_STRLEN);
    } else {
        ESP_LOGW(TAG, "Failed to parse temperature/humidity from data");
        httpd_resp_set_status(req, "400 Bad Request");
        return httpd_resp_send(req, "{\"status\":\"error\",\"message\":\"Invalid data\"}", HTTPD_RESP_USE_STRLEN);
    }
}

esp_err_t temperature_api_get_handler(httpd_req_t *req)
{
    ESP_LOGI(TAG, "API GET request received");
    
    // Try WiFi storage first, fallback to ESP-NOW
    float temp = temperature_storage_get_temperature();
    float humidity = temperature_storage_get_humidity();
    bool valid = temperature_storage_is_data_valid();
    uint32_t last_update = temperature_storage_get_last_update_time();
    
    // If WiFi storage has no valid data, try ESP-NOW
    if (!valid) {
        temp = espnow_get_temperature();
        humidity = espnow_get_humidity();
        valid = espnow_is_data_valid();
        last_update = espnow_get_last_update_time();
    }
    
    ESP_LOGI(TAG, "Returning data - Temp: %.2f°C, Humidity: %.2f%%, Valid: %s", temp, humidity, valid ? "true" : "false");
    
    // Convert timestamp to readable format
    char time_str[64];
    if (last_update > 0) {
        uint32_t current_time = xTaskGetTickCount() * portTICK_PERIOD_MS;
        uint32_t seconds_ago = (current_time - last_update) / 1000;
        
        if (seconds_ago < 60) {
            snprintf(time_str, sizeof(time_str), "%lu seconds ago", seconds_ago);
        } else if (seconds_ago < 3600) {
            snprintf(time_str, sizeof(time_str), "%lu minutes ago", seconds_ago / 60);
        } else {
            snprintf(time_str, sizeof(time_str), "%lu hours ago", seconds_ago / 3600);
        }
    } else {
        strcpy(time_str, "Never");
    }
    
    char json_response[256];
    snprintf(json_response, sizeof(json_response),
             "{\"temperature\":%.2f,\"humidity\":%.2f,\"valid\":%s,\"last_update\":\"%s\"}",
             temp, humidity, valid ? "true" : "false", time_str);
    
    ESP_LOGI(TAG, "Sending JSON response: %s", json_response);
    
    httpd_resp_set_type(req, "application/json");
    return httpd_resp_send(req, json_response, HTTPD_RESP_USE_STRLEN);
}

