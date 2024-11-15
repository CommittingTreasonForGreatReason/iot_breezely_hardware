#include <Arduino.h>
#include "WiFi.h"

#define WIFI_SSID "hotspot_1234"

#define ENTER_ASCII 13
#define DELETE_ASCII 8

#define MAGNET_INPUT_PIN 23
#define MAGNET_STATUS_PIN 22
#define WIFI_STATUS_PIN 21

// char wifi_ssid[16] = {0};
#define PASSWORD_LENGTH 12
char wifi_password[PASSWORD_LENGTH] = {0};

void text_input_blocking(char *input_buffer, uint8_t size)
{
  bool confirmed_input = false;
  uint8_t text_input_index = 0;
  while (!confirmed_input)
  {
    if (Serial.available() > 0)
    {
      // read the incoming byte:
      uint8_t read_byte = Serial.read();

      if (text_input_index + 1 >= size)
      {
        Serial.print("input too long");
      }
      else
      {
        if (read_byte == DELETE_ASCII)
        {
          text_input_index--;
          input_buffer[text_input_index] = 0x00;
        }
        else if (read_byte == ENTER_ASCII)
        {
          input_buffer[text_input_index + 1] = 0x00;
          confirmed_input = true;
          Serial.println("");
        }
        else
        {
          Serial.print((char)read_byte);
          input_buffer[text_input_index] = read_byte;
          text_input_index++;
        }
      }
    }
  }
}

uint8_t dot_dot_dot_index = 0;
void dot_dot_dot_loop_increment()
{
  dot_dot_dot_index++;
  if (dot_dot_dot_index >= 3)
  {
    dot_dot_dot_index = 0;
    Serial.println("\r");
  }
  Serial.print(".");
}
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
  Serial.print("attempting to connect to WIFI: ");
  Serial.println(WIFI_SSID);
  
  Serial.println("please input the wifi password");
  text_input_blocking(wifi_password, sizeof(wifi_password));
  Serial.print("text input: ");
  Serial.println(wifi_password);

  WiFi.begin(WIFI_SSID, (const char *)wifi_password);
  Serial.println("setup_complete");
  Serial.println("connecting to wifi");
}

bool is_connected = false;

void loop()
{
  if (!is_connected && WiFi.status() == WL_CONNECTED)
  {
    Serial.println("connected :)");

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
  Serial.print("pin status: ");
  Serial.println(pin_status);
}