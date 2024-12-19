#include <Arduino.h>

#include "WiFi.h"
#include "wifi_manual.hpp"
#include "utils.hpp"
#include "user_config.hpp"
#include "logger.hpp"

// method for connection to Wifi via credentials
int wifi_manual_setup()
{
    char buffer[64] = {0};
    sprintf(buffer, "attempting to connect to WIFI: %s: \n", WIFI_SSID);
    serial_logger_print(buffer, LOG_LEVEL_INFO);

    WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
    return 0; // success
}