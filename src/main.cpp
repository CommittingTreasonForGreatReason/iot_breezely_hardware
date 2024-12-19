// standard library includes
#include <Arduino.h>
#include <SPIFFS.h>
#include <WiFi.h>
#include <ESPmDNS.h>

// local includes
#include "utils.hpp"
#include "wifi_manual.hpp"
#include "wifi_protect_setup.hpp"
#include "things_board_client.hpp"
#include "dht_sensor.hpp"
#include "web_server.hpp"
#include "user_config.hpp"
#include "logger.hpp"
#include "esp_timer.h"

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

int start_mDNS_timeout_us(int64_t timeout_us)
{
    int64_t timeout_deadline_us = esp_timer_get_time() + timeout_us;
    // start mDNS service
    while (!MDNS.begin(HOSTNAME))
    {
        if (esp_timer_get_time() >= timeout_deadline_us)
        {
            serial_logger_print("Error starting mDNS service", LOG_LEVEL_ERROR);
            return -1;
        }
    }

    // add http service for mDNS discovery
    if (!MDNS.addService("http", "tcp", 80))
    {
        serial_logger_print("Unable to add MDNS", LOG_LEVEL_ERROR);
        return -1;
    }
    serial_logger_print("Successfully started MDNS", LOG_LEVEL_DEBUG);
    return 0;
}

// ------------ startup routine ------------ //
void setup()
{
    // configure GPIO pins
    pinMode(WIFI_STATUS_PIN, OUTPUT);
    pinMode(MAGNET_INPUT_PIN, INPUT);

    set_global_log_level(LOG_LEVEL_DEBUG); // <-------------------------------------------------------------------------------- SET LOG LEVEL HERE

    // initialize SPIFFS
    if (!SPIFFS.begin(true))
    {
        serial_logger_print("An error occurred while mounting SPIFFS", LOG_LEVEL_ERROR);
        return;
    }

    // setup DHT11 sensor for air temperature and humidity
    dht_sensor_setup();
    delay(100);

    // launch serial console
    Serial.begin(115200);

#ifdef __NO_WPS
    // manual wifi setup (SSID & password hardcoded)
    wifi_manual_setup(); // does a manuel setup by using hardcoded SSID and password (see more under lib/user_specific)
#else
    // initial wifi setup via WPS (release mode)
    wifi_wps_setup();
#endif

    serial_logger_print("~~~ WIFI SETUP COMPLETE ~~~", LOG_LEVEL_INFO);
}
// ------------------------------------------------- //

// connection status flag
bool wifi_is_connected = false;
enum States
{
    WAITING_ON_WIFI,
    WAITING_ON_SERVER,
    IDLE
};

// ----------- MAIN APPLICATION LOOP ------------ //
void loop()
{
    // detect when wifi connection is established and ready
    if (!wifi_is_connected && WiFi.status() == WL_CONNECTED)
    {
        log_wifi_info(LOG_LEVEL_DEBUG);

        // signal wifi connection status on GPIO pin
        digitalWrite(WIFI_STATUS_PIN, HIGH);
        wifi_is_connected = true;

        start_mDNS_timeout_us(1000 * 1000);

        // continue to launch the webinterface
        web_server_setup();
        serial_logger_print("\n~~~ SERVER SETUP COMPLETE ~~~", LOG_LEVEL_INFO);
        char buffer[64] = {0};
        sprintf(buffer, "Access your breezely at http://%s ", HOSTNAME);
        serial_logger_print(buffer, LOG_LEVEL_INFO);
    }
    // waiting for wifi state ...
    if (WiFi.status() != WL_CONNECTED)
    {
        dot_dot_dot_loop_increment();
        digitalWrite(WIFI_STATUS_PIN, !digitalRead(WIFI_STATUS_PIN));
        wifi_is_connected = false;
    }

    // readout the magnetic reed switch and control output pin accordingly
    static int last_pin_status = LOW;
    int pin_status = digitalRead(MAGNET_INPUT_PIN);
    if (pin_status != last_pin_status)
    {
        char buffer[64] = {0};
        sprintf(buffer, "updated pin status: %d", pin_status);
        serial_logger_print(buffer, LOG_LEVEL_DEBUG);
        last_pin_status = pin_status;
    }

    // publish measurements to the thingsboard cloud
    float temperature = dht_sensor_get_temperature();
    float humidity = dht_sensor_get_humidity();
    if (get_things_board_connected())
    {
        things_board_client_routine(temperature, humidity, (pin_status == 1));
    }

    delay(2000);
}