#include <Arduino.h>
#include "WiFi.h"
#include "wifi_manual.hpp"
#include "utils.hpp"
#include "network_config.hpp"

int wifi_manual_setup()
{
    Serial.print("attempting to connect to WIFI: ");
    Serial.println(WIFI_SSID);

    WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
    return 0; // success
}