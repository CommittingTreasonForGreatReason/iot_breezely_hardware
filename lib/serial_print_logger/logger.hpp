#pragma once

typedef enum serial_log_level
{
    LOG_LEVEL_DEBUG,
    LOG_LEVEL_INFO,
    LOG_LEVEL_WARNING,
    LOG_LEVEL_ERROR
} serial_log_level_t;

void set_global_log_level(serial_log_level_t global_log_level_);
serial_log_level_t get_global_log_level();

void serial_logger_print(const char *message, serial_log_level_t log_level);