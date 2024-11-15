#include <Arduino.h>
#include "WiFi.h"
#include "wifi_manual.hpp"
#include "utils.hpp"

char wifi_password[PASSWORD_LENGTH] = {0};

int wifi_manual_setup()
{
    Serial.print("attempting to connect to WIFI: ");
    Serial.println(WIFI_SSID);

    Serial.println("please input the wifi password");
    text_input_blocking(wifi_password, sizeof(wifi_password));
    Serial.print("text input: ");
    Serial.println(wifi_password);

    WiFi.begin(WIFI_SSID, (const char *)wifi_password);
    return 0; // success
}