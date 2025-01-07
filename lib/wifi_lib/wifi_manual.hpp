#pragma once

#include "logger.hpp"

int wifi_manual_setup(char *wifi_ssid, char *wifi_password);
void log_wifi_info(serial_log_level_t log_level);