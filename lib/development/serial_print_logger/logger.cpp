#include <Arduino.h>
#include "logger.hpp"

serial_log_level_t global_log_level = LOG_LEVEL_INFO;

void set_global_log_level(serial_log_level_t global_log_level_)
{
    global_log_level = global_log_level_;
}
serial_log_level_t get_global_log_level()
{
    return global_log_level;
}

void serial_logger_print_telemetry_float(const char *telemetry_name, float value)
{
    char buffer[64] = {0};
    sprintf(buffer, "sent telemetry %s: %f", telemetry_name, value);
    serial_logger_print(buffer, LOG_LEVEL_DEBUG);
}

void serial_logger_print(const char *message, serial_log_level_t log_level = LOG_LEVEL_INFO)
{
    String log_level_string = "";
    switch (log_level)
    {
    case LOG_LEVEL_INFO:
        log_level_string = "INFO: ";
        break;
    case LOG_LEVEL_DEBUG:
        log_level_string = "DEBUG: ";
        break;
    case LOG_LEVEL_WARNING:
        log_level_string = "WARNING: ";
        break;
    default:
        log_level_string = "ERROR: ";
        break;
    }
    if (log_level >= global_log_level)
    {
        Serial.printf("%s: %s \n", log_level_string, message);
    }
}