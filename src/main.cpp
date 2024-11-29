// standard library includes
#include <Arduino.h>
#include <SPIFFS.h>
#include <WiFi.h>

// local includes
#include "utils.hpp"
#include "wifi_manual.hpp"
#include "wifi_protect_setup.hpp"
#include "web_server.hpp"
#include "web_client.hpp"
#include "dht_sensor.hpp"

// GPIO pin mapping definitions
#define MAGNET_INPUT_PIN 18
#define MAGNET_STATUS_PIN 22
#define WIFI_STATUS_PIN 21

// ------------ startup routine ------------ //
void setup()
{
    // configure GPIO pins
    pinMode(WIFI_STATUS_PIN, OUTPUT);
    pinMode(MAGNET_STATUS_PIN, OUTPUT);
    pinMode(HTTP_OUTPUT_PIN, OUTPUT);
    pinMode(MAGNET_INPUT_PIN, INPUT);

    // Initialize SPIFFS
    if (!SPIFFS.begin(true)) {
        Serial.println("An error occurred while mounting SPIFFS");
        return;
    }

    // setup DH11 sensor for air temperature and humidity    
    dht_sensor_setup();
    delay(2000);

    // launch serial console
    Serial.begin(115200);
    Serial.println("");

    // WIFI setup: WPS
    // wifi_wps_setup();

    // WIFI setup: Manual (SSID & password hardcoded)
    wifi_manual_setup(); // does a manuel setup by using hardcoded SSID and password (see more under lib/user_specific)

    Serial.println("setup complete");
    Serial.println("connecting to wifi ...");
}
// ------------------------------------------------- //

// connection status flag
bool is_connected = false;

// ----------- MAIN APPLICATION LOOP ------------ //
void loop()
{
    if (!is_connected && WiFi.status() == WL_CONNECTED)
    {
        // As soon as Wifi connection is established print some debug info to serial console
        Serial.println("wifi connection was made :-)");
        Serial.print("SSID: ");
        Serial.println(WiFi.SSID());
        Serial.print("gateway ip: ");
        Serial.println(WiFi.gatewayIP());
        Serial.println("***********************************");

        Serial.print("host name: ");
        Serial.println(WiFi.getHostname());
        Serial.print("local ip: ");
        Serial.println(WiFi.localIP());
        Serial.print("local ipv6: ");
        Serial.println(WiFi.localIPv6());
        Serial.print("mac address: ");
        Serial.println(WiFi.macAddress());

        // signal wifi connection status on GPIO pin
        digitalWrite(WIFI_STATUS_PIN, HIGH);
        is_connected = true;

        // continue to launch the webinterface 
        Serial.println("starting web server");
        web_server_setup();

        // at last init the http client for communication with cloud backend
        Serial.println("starting web client");
        web_client_setup();
    }
    if (WiFi.status() != WL_CONNECTED)
    {
        dot_dot_dot_loop_increment();
        digitalWrite(WIFI_STATUS_PIN, !digitalRead(WIFI_STATUS_PIN));
        is_connected = false;
    }

    // readout the magnetic reed switch and control output pin accordingly
    int pin_status = digitalRead(MAGNET_INPUT_PIN); // doesn't work for some reason ...
    digitalWrite(MAGNET_STATUS_PIN, pin_status);

    Serial.print("pin status: ");
    Serial.println(pin_status);

    delay(500);
}