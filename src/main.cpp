#include <Arduino.h>
#include "WiFi.h"
#include "utils.hpp"
#include "wifi_manual.hpp"
#include "wifi_protect_setup.hpp"

#define MAGNET_INPUT_PIN 23
#define MAGNET_STATUS_PIN 22
#define WIFI_STATUS_PIN 21

static WiFiServer wifi_server(12345);
void client_connect()
{
  static WiFiClient wifi_client;
  if (!wifi_client)
  {
    wifi_client = wifi_server.available();
  }
  if (wifi_client)
  {
    Serial.print("we have a client!!!");

    Serial.print("client local ip: ");
    Serial.println(wifi_client.localIP());
    Serial.print("client local port: ");
    Serial.println(wifi_client.localPort());

    int length = wifi_client.available();
    Serial.print("client data length: ");
    Serial.println(length);
    delay(1000);
  }
}

void setup()
{
  pinMode(WIFI_STATUS_PIN, OUTPUT);
  pinMode(MAGNET_STATUS_PIN, OUTPUT);
  pinMode(MAGNET_INPUT_PIN, INPUT);
  Serial.begin(921600);
  Serial.println("");
  wifi_manual_setup();
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
  }
  if (WiFi.status() != WL_CONNECTED)
  {
    dot_dot_dot_loop_increment();
    digitalWrite(WIFI_STATUS_PIN, !digitalRead(WIFI_STATUS_PIN));
    delay(500);
    is_connected = false;
  }

  int pin_status = digitalRead(MAGNET_INPUT_PIN);
  digitalWrite(MAGNET_STATUS_PIN, pin_status);
  // Serial.print("pin status: ");
  // Serial.println(pin_status);
}