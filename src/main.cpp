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

#include "web_server.hpp"
#include "user_config.hpp"
#include "logger.hpp"
#include "esp_timer.h"

#ifdef __USE_DATA_FABRICATION
#include "data_fabricator.hpp"
#else
#include "dht_sensor.hpp"
#endif

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

// State definitions for FSM
enum class State
{
    WAITING_ON_WIFI,  // wifi connecteion yet to be made
    ONLY_SERVER_IDLE, // wifi conection made! but only the local server is active (no connection to cloud)
    CLOUD_CLIENT_IDLE // server active and cloud connected
};

// set initial state
State current_state = State::WAITING_ON_WIFI;

// ------------ startup routine ------------ //
void setup()
{
    // current_state = State::WAITING_ON_WIFI;

    // configure GPIO pins
    pinMode(WIFI_STATUS_PIN, OUTPUT);
    pinMode(MAGNET_INPUT_PIN, INPUT);

#ifdef __USE_DATA_FABRICATION
    data_fabricator_setup();
#else
    // setup DHT11 sensor for air temperature and humidity
    dht_sensor_setup();
#endif

    set_global_log_level(LOG_LEVEL_DEBUG); // <-------------------------------------------------------------------------------- SET LOG LEVEL HERE

    // initialize SPIFFS to access non-volatile flash memory
    if (!SPIFFS.begin(true))
    {
        serial_logger_print("An error occurred while mounting SPIFFS", LOG_LEVEL_ERROR);
        return;
    }

    // launch serial console
    delay(100);
    Serial.begin(115200);

#ifdef __NO_WPS
    // manual wifi setup (SSID & password hardcoded)
    wifi_manual_setup(); // does a manuel setup by using hardcoded SSID and password (see more under lib/user_specific)
#else
    // initial wifi setup via WPS (release mode)
    wifi_wps_setup();
#endif
}
// ------------------------------------------------- //

int64_t delta_time_ms = 1000;
int64_t things_board_routine_deadline_ms = esp_timer_get_time() / 1000.0 + delta_time_ms;
char buffer[128] = {0};

// ----------- MAIN APPLICATION LOOP ------------ //
void loop()
{
    // run the global finite state machine in iterative manner
    switch (current_state)
    {
    case State::WAITING_ON_WIFI:
    {
        // ENTRY
        serial_logger_print("[FSM] Waiting for Wifi state", LOG_LEVEL_DEBUG);
        // ...

        // DO
        // wait indefinetely for a wifi connection
        while (WiFi.status() != WL_CONNECTED)
        {
            // blink the wifi status LED (green) slowly while waiting for connection
            dot_dot_dot_loop_increment();
            digitalWrite(WIFI_STATUS_PIN, !digitalRead(WIFI_STATUS_PIN));
            delay(1000);
        }

        // EXIT
        serial_logger_print("~~~ WIFI SETUP COMPLETE ~~~", LOG_LEVEL_INFO);
        log_wifi_info(LOG_LEVEL_DEBUG);

        // keep wifi status LED (green) constantly on to signal active connection
        digitalWrite(WIFI_STATUS_PIN, HIGH);

        // start mDNS discovery service and set timeout
        start_mDNS_timeout_us(1000 * 1000);

        // continue to launch the webinterface
        web_server_setup();
        serial_logger_print("\n~~~ SERVER SETUP COMPLETE ~~~", LOG_LEVEL_INFO);
        sprintf(buffer, "Access your breezely at http://%s ", HOSTNAME);
        serial_logger_print(buffer, LOG_LEVEL_INFO);

        // transition into ONLY_SERVER_IDLE state
        current_state = State::ONLY_SERVER_IDLE;
        break;
    }

    case State::ONLY_SERVER_IDLE:
    {
        // ENTRY
        serial_logger_print("[FSM] Only server idle state", LOG_LEVEL_DEBUG);
        // ...

        // DO
        while (true)
        {
            // readout the magnetic reed switch and control output pin accordingly
            static int last_pin_status = LOW;
            int pin_status = digitalRead(MAGNET_INPUT_PIN);

            // log window state changes to the serial monitor
            if (pin_status != last_pin_status)
            {
                sprintf(buffer, "updated pin status: %d", pin_status);
                serial_logger_print(buffer, LOG_LEVEL_DEBUG);
                last_pin_status = pin_status;
            }

            // EXIT
            if (WiFi.status() != WL_CONNECTED)
            {
#ifdef __NO_WPS
                // manual wifi setup (SSID & password hardcoded)
                wifi_manual_setup(); // does a manuel setup by using hardcoded SSID and password (see more under lib/user_specific)
#else
                // initial wifi setup via WPS (release mode)
                wifi_wps_setup();
#endif
                current_state = State::WAITING_ON_WIFI;
                break;
            }
            else if (get_things_board_connected())
            {
                current_state = State::CLOUD_CLIENT_IDLE;
                break;
            }
            delay(2000);
        }

        break;
    }

    case State::CLOUD_CLIENT_IDLE:
    {
        // ENTRY
        serial_logger_print("[FSM] Cloud client idle state", LOG_LEVEL_DEBUG);
        delta_time_ms = 5000;
        things_board_routine_deadline_ms = esp_timer_get_time() / 1000 + delta_time_ms;
        serial_logger_print("~~~ CLOUD CONNECTION MADE ~~~", LOG_LEVEL_INFO);

        // DO
        while (get_things_board_connected())
        {
#ifdef __USE_DATA_FABRICATION
            float temperature = data_fabricator_get_temperature();
            float humidity = data_fabricator_get_humidity();
            bool window_status = data_fabricator_get_window_status();
#else
            // readout the magnetic reed switch and control output pin accordingly
            static int last_pin_status = LOW;
            int pin_status = digitalRead(MAGNET_INPUT_PIN);
            if (pin_status != last_pin_status)
            {
                sprintf(buffer, "updated pin status: %d", pin_status);
                serial_logger_print(buffer, LOG_LEVEL_DEBUG);
                last_pin_status = pin_status;
            }

            float temperature = dht_sensor_get_temperature();
            float humidity = dht_sensor_get_humidity();
            bool window_status = (pin_status == 1);

#endif
            if (things_board_routine_deadline_ms <= esp_timer_get_time() / 1000)
            {
                things_board_client_routine(temperature, humidity, window_status);
                things_board_routine_deadline_ms = esp_timer_get_time() / 1000 + delta_time_ms;
            }
            delay(100);
        }

        // cloud connection lost
        current_state = State::ONLY_SERVER_IDLE;

        // EXIT
        break;
    }

    default:
    {
        // ENTRY
        // ...

        // DO
        serial_logger_print("\n[FSM] unknown state!", LOG_LEVEL_WARNING);

        // EXIT
        // ...
        break;
    }
    }

    delay(100);
}