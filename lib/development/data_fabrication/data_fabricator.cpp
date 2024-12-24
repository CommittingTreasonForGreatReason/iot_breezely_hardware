#include "data_fabricator.hpp"
#include "esp_timer.h"

// ################################################################################################################
// This file is mostly used to fabricate synthetic/fake data for testing purposes
// Choose a data series  that shall be activate at specific time periods to test the system
// ################################################################################################################
int64_t temperature_start_time_ms = 0;
uint16_t last_activated_temperature_index = 0;
float_data_timestamp_pair_t temperatures[] = {
    {20, 0},
    {22, 1000},
    {25, 2000},
    {30, 3000},
    {33, 4000},
    {20, 5000}, // must loop again to start
};

int64_t humidity_start_time_ms = 0;
uint16_t last_activated_humidity_index = 0;
float_data_timestamp_pair_t humidities[] = {
    {10, 0},
    {35, 1000},
    {40, 2000},
    {45, 3000},
    {60, 4000},
    {50, 5000},
    {22, 6000},
    {10, 7000}, // must loop again to start
};

int64_t window_status_start_time_ms = 0;
uint16_t last_activated_window_status_index = 0;
float_data_timestamp_pair_t window_statuses[] = {
    {true, 0},
    {false, 3000},
    {true, 6000},
    {false, 10000},
    {true, 11000}, // must loop again to start
};

void data_fabricator_setup()
{
    temperature_start_time_ms = esp_timer_get_time();
    humidity_start_time_ms = esp_timer_get_time();
    window_status_start_time_ms = esp_timer_get_time();
    last_activated_temperature_index = 0;
    last_activated_humidity_index = 0;
    last_activated_window_status_index = 0;
}

float data_fabricator_get_temperature()
{
    int64_t next_activation_time_ms = temperature_start_time_ms + temperatures[last_activated_temperature_index].activation_time_ms;
    if (esp_timer_get_time() >= next_activation_time_ms)
    {
        last_activated_temperature_index++;
    }
    uint16_t temperature_values = sizeof(temperatures) / sizeof(float_data_timestamp_pair_t);
    if (last_activated_temperature_index >= temperature_values)
    {
        last_activated_temperature_index = 0;
        temperature_start_time_ms = esp_timer_get_time();
    }
    return temperatures[last_activated_temperature_index].value;
}

float data_fabricator_get_humidity()
{
    int64_t next_activation_time_ms = humidity_start_time_ms + humidities[last_activated_humidity_index].activation_time_ms;
    if (esp_timer_get_time() >= next_activation_time_ms)
    {
        last_activated_humidity_index++;
    }
    uint16_t humidity_values = sizeof(humidities) / sizeof(float_data_timestamp_pair_t);
    if (last_activated_humidity_index >= humidity_values)
    {
        last_activated_humidity_index = 0;
        humidity_start_time_ms = esp_timer_get_time();
    }
    return humidities[last_activated_humidity_index].value;
}

bool data_fabricator_get_window_status()
{
    int64_t next_activation_time_ms = window_status_start_time_ms + window_statuses[last_activated_window_status_index].activation_time_ms;
    if (esp_timer_get_time() >= next_activation_time_ms)
    {
        last_activated_window_status_index++;
    }
    uint16_t window_status_values = sizeof(window_statuses) / sizeof(float_data_timestamp_pair_t);
    if (last_activated_window_status_index >= window_status_values)
    {
        last_activated_window_status_index = 0;
        window_status_start_time_ms = esp_timer_get_time();
    }
    return window_statuses[last_activated_window_status_index].value;
}