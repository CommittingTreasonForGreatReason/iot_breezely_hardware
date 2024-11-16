#include <Arduino.h>
#include "WiFi.h"
#include "utils.hpp"
#include "wifi_manual.hpp"
#include "wifi_protect_setup.hpp"
#include "web_server.hpp"

#define MAGNET_INPUT_PIN 18
#define MAGNET_STATUS_PIN 22
#define WIFI_STATUS_PIN 21

void setup()
{
  pinMode(WIFI_STATUS_PIN, OUTPUT);
  pinMode(MAGNET_STATUS_PIN, OUTPUT);
  pinMode(HTTP_OUTPUT_PIN, OUTPUT);
  pinMode(MAGNET_INPUT_PIN, INPUT);
  Serial.begin(115200);
  Serial.println("");

  // wps setup
  // wifi_wps_setup();
  // manual setup
  wifi_manual_setup(); // does a manuel setup by using hardcoded SSID and password (see more under lib/user_specific)

  Serial.println("setup_complete");
  Serial.println("connecting to wifi");
}

bool is_connected = false;

void loop()
{
  if (!is_connected && WiFi.status() == WL_CONNECTED)
  {
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
    digitalWrite(WIFI_STATUS_PIN, HIGH);
    is_connected = true;

    Serial.println("starting web server");
    web_server_setup();
  }
  if (WiFi.status() != WL_CONNECTED)
  {
    dot_dot_dot_loop_increment();
    digitalWrite(WIFI_STATUS_PIN, !digitalRead(WIFI_STATUS_PIN));
    is_connected = false;
  }

  int pin_status = digitalRead(MAGNET_INPUT_PIN); // doesn't work for some reason ...
  digitalWrite(MAGNET_STATUS_PIN, pin_status);
  // Serial.print("pin status: ");
  // Serial.println(pin_status);
  delay(500);
}