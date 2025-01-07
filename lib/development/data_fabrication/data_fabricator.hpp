#include <Arduino.h>

typedef struct float_data_timestamp_pair
{
    float value;
    int64_t activation_time_ms;
} float_data_timestamp_pair_t;

typedef struct bool_data_timestamp_pair
{
    bool value;
    int64_t activation_time_ms;
} bool_data_timestamp_pair_t;

void data_fabricator_setup();
float data_fabricator_get_temperature();
float data_fabricator_get_humidity();
bool data_fabricator_get_window_status();