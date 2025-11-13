#ifndef TEMPERATURE_STORAGE_H
#define TEMPERATURE_STORAGE_H

#include <stdint.h>
#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

// Update temperature and humidity data
void temperature_storage_update(float temperature, float humidity);

// Get latest temperature reading
float temperature_storage_get_temperature(void);

// Get latest humidity reading
float temperature_storage_get_humidity(void);

// Check if data is valid (received within last 30 seconds)
bool temperature_storage_is_data_valid(void);

// Get timestamp of last received data
uint32_t temperature_storage_get_last_update_time(void);

#ifdef __cplusplus
}
#endif

#endif // TEMPERATURE_STORAGE_H

