#ifndef ESPNOW_RECEIVER_H
#define ESPNOW_RECEIVER_H

#include <stdint.h>
#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

// Structure to receive data (must match ESP8266 sender)
typedef struct struct_message {
    float temperature;
    float humidity;
} struct_message;

// Initialize ESP-NOW receiver
void espnow_receiver_init(void);

// Get latest temperature reading
float espnow_get_temperature(void);

// Get latest humidity reading
float espnow_get_humidity(void);

// Check if data is valid (received within last 30 seconds)
bool espnow_is_data_valid(void);

// Get timestamp of last received data
uint32_t espnow_get_last_update_time(void);

#ifdef __cplusplus
}
#endif

#endif // ESPNOW_RECEIVER_H

