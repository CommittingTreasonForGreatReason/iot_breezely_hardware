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

void log_wifi_info(serial_log_level_t log_level)
{
    char buffer[128] = {0};
    serial_logger_print("\n######### WIFI INFO #########", log_level);

    sprintf(buffer, "SSID %s", WiFi.SSID().c_str());
    serial_logger_print(buffer, log_level);

    sprintf(buffer, "Gateway IPv4: %s", WiFi.gatewayIP().toString().c_str());
    serial_logger_print(buffer, log_level);
    serial_logger_print("------------------------------", log_level);

    sprintf(buffer, "mDNS Hostname: %s.local", HOSTNAME);
    serial_logger_print(buffer, log_level);

    sprintf(buffer, "Hostname: %s", WiFi.getHostname());
    serial_logger_print(buffer, log_level);

    sprintf(buffer, "Local IPv4: %s", WiFi.localIP().toString().c_str());
    serial_logger_print(buffer, log_level);

    sprintf(buffer, "Local IPv6: %s", WiFi.localIPv6().toString().c_str());
    serial_logger_print(buffer, log_level);

    sprintf(buffer, "MAC address: %s", WiFi.macAddress().c_str());
    serial_logger_print(buffer, log_level);

    serial_logger_print("#############################", log_level);
}