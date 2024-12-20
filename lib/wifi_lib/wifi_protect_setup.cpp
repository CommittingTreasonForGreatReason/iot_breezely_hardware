#include "WiFi.h"
#include <Arduino.h>

#include "wifi_protect_setup.hpp"
#include "esp_wps.h"
#include "logger.hpp"

void wps_start()
{
    esp_wps_config_t config;
    memset(&config, 0, sizeof(esp_wps_config_t));
    // Same as config = WPS_CONFIG_INIT_DEFAULT(ESP_WPS_MODE);
    config.wps_type = ESP_WPS_MODE;
    strcpy(config.factory_info.manufacturer, ESP_MANUFACTURER);
    strcpy(config.factory_info.model_number, CONFIG_IDF_TARGET);
    strcpy(config.factory_info.model_name, ESP_MODEL_NAME);
    strcpy(config.factory_info.device_name, ESP_DEVICE_NAME);
    esp_err_t err = esp_wifi_wps_enable(&config);
    if (err != ESP_OK)
    {
        Serial.printf("wps enable failed: 0x%x: %s\n", err, esp_err_to_name(err));
        return;
    }

    err = esp_wifi_wps_start(0);
    if (err != ESP_OK)
    {
        Serial.printf("wps start failed: 0x%x: %s\n", err, esp_err_to_name(err));
    }
}

void wps_stop()
{
    esp_err_t err = esp_wifi_wps_disable();
    if (err != ESP_OK)
    {
        Serial.printf("wps disable failed: 0x%x: %s\n", err, esp_err_to_name(err));
    }
}

void WiFiEvent(WiFiEvent_t event)
{
    switch (event)
    {
    case ARDUINO_EVENT_WIFI_STA_START:
        Serial.println("wifi event: station startup");
        break;
    case ARDUINO_EVENT_WIFI_STA_GOT_IP:
        Serial.println("wifi event: got ip from -> network: " + String(WiFi.SSID()) + " | ip: " + String(WiFi.localIP()));
        break;
    case ARDUINO_EVENT_WIFI_STA_DISCONNECTED:
        Serial.println("wifi event: disconnected from station -> reconnecting");
        WiFi.reconnect();
        break;
    case ARDUINO_EVENT_WPS_ER_SUCCESS:
        Serial.println("wifi event: wps successful -> stopping wps and connection to " + String(WiFi.SSID()));
        wps_stop();
        delay(10);
        WiFi.begin();
        break;
    case ARDUINO_EVENT_WPS_ER_FAILED:
        Serial.println("wifi event: wps failed -> retrying");
        wps_stop();
        wps_start();
        break;
    case ARDUINO_EVENT_WPS_ER_TIMEOUT:
        Serial.println("wifi event: wps timeout -> retrying");
        wps_stop();
        wps_start();
        break;
    case ARDUINO_EVENT_WPS_ER_PIN:
        Serial.println("WPS_PIN = ...");
        break;
    default:
        break;
    }
}

int wifi_wps_setup()
{
    // Start Wi-Fi in station mode
    WiFi.onEvent(WiFiEvent);
    WiFi.mode(WIFI_MODE_STA);
    Serial.println("starting wifi protect setup (wps)");
    wps_start();

    return 0; // success
}