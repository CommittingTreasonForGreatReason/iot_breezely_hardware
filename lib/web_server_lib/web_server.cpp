#include <WiFi.h>
#include <Arduino.h>
#include <AsyncTCP.h>
#include <ESPAsyncWebServer.h>
#include <SPIFFS.h>
#include <ArduinoJson.h>

#include "web_server.hpp"
#include "dht_sensor.hpp"
#include "things_board_client.hpp"

// define http server instance on default port 80
AsyncWebServer server(80);

char configured_token[32] = {0};
bool cloud_connected = false;

String outputState(int output)
{
    return digitalRead(output) ? "checked" : "";
}

// callback for http requests on undefined routes
void on_http_not_found(AsyncWebServerRequest *request)
{
    request->send(404, "text/plain", "Not found");
}

// callback for login attempts
void on_http_login(AsyncWebServerRequest *request)
{
    String input_username = "No message sent";
    String input_password = "No message sent";
    // GET username value on <ESP_IP>/get?username=<inputMessage>
    if (request->hasParam(USERNAME_INPUT_NAME))
    {
        input_username = request->getParam(USERNAME_INPUT_NAME)->value();
    }
    // GET password value on <ESP_IP>/get?password=<inputMessage>
    if (request->hasParam(PASSWORD_INPUT_NAME))
    {
        input_password = request->getParam(PASSWORD_INPUT_NAME)->value();
    }

    Serial.print("username: ");
    Serial.println(input_username);
    Serial.print("password: ");
    Serial.println(input_password);

    request->send(200, "text/html", "HTTP GET request with user login send to esp32 (username: " + input_username + " | password: " + input_password + ")<br><a href=\"/\">Return to Home Page</a>");
}

// callback function to write the digital output pin state via GET request
void on_http_gpio_write(AsyncWebServerRequest *request)
{
    String output_message;
    String state_message;
    // GET input1 value on <ESP_IP>/gpio_update?output=<output_message>&state=<state_message>
    if (request->hasParam(OUTPUT_NAME) && request->hasParam(STATE_NAME))
    {
        output_message = request->getParam(OUTPUT_NAME)->value();
        state_message = request->getParam(STATE_NAME)->value();
        digitalWrite(output_message.toInt(), state_message.toInt());
    }
    else
    {
        output_message = "No message sent";
        state_message = "No message sent";
    }

    // print info to serial console
    Serial.print("GPIO: ");
    Serial.print(output_message);
    Serial.print(" -> ");
    Serial.println(state_message);

    request->send(200, "text/plain", "OK");
}

// callback function for sending sensor measurements to the browser on demand
void on_http_sensor_read(AsyncWebServerRequest *request)
{
    Serial.println("--> sensor read request from client");

    // store sensor data as dictionary of key-value-pairs
    JsonDocument jsonDoc;
    jsonDoc["window-state"] = digitalRead(18) == HIGH ? "open" : "closed";
    jsonDoc["air-temperature"] = dht_sensor_get_temperature();
    jsonDoc["air-humidity"] = dht_sensor_get_humidity();
    // ...

    // send http response with json encoded payload
    String jsonResponse;
    serializeJson(jsonDoc, jsonResponse);
    Serial.println(jsonResponse);
    request->send(200, "application/json", jsonResponse);
}

// callback function for handling fetch device info request
void on_http_fetch_device_info(AsyncWebServerRequest *request)
{
    Serial.println("--> device info request from client");

    JsonDocument jsonDoc;
    jsonDoc["device-name"] = "name";
    jsonDoc["uptime"] = "dd hh:mm:ss";
    jsonDoc["wifi-ssid"] = WiFi.SSID();
    jsonDoc["ipv4-address"] = WiFi.localIP().toString();
    jsonDoc["mac-address"] = WiFi.macAddress();
    jsonDoc["hostname"] = WiFi.getHostname();
    jsonDoc["cloud-connection-status"] = cloud_connected ? "online" : "offline";
    // ...

    // send http response with json encoded payload
    String jsonResponse;
    serializeJson(jsonDoc, jsonResponse);
    request->send(200, "application/json", jsonResponse);
}

// callback function for handling fetch settings request
void on_http_fetch_settings(AsyncWebServerRequest *request)
{
    Serial.println("--> settings request from client");

    JsonDocument jsonDoc;
    jsonDoc["token"] = (strlen(configured_token) >= 10) ? configured_token : "not configured";
    // ...

    // send http response with json encoded payload
    String jsonResponse;
    serializeJson(jsonDoc, jsonResponse);
    request->send(200, "application/json", jsonResponse);
}

// callback for things board token authorization request
void on_http_set_token(AsyncWebServerRequest *request)
{
    // check for token paramater
    String input_token = "";
    if (request->hasParam(TOKEN_INPUT_NAME))
    {
        input_token = request->getParam(TOKEN_INPUT_NAME)->value();
    }

    Serial.print("token: ");
    Serial.println(input_token);

    // when token is changed then force to reconnect with new one
    if (cloud_connected)
    {
        // tear down connection if already connected with a token
        things_board_client_teardown();
    }

    cloud_connected = (things_board_client_setup(input_token.c_str()) >= 0);
    if (cloud_connected) // cloud connection has been made :-)
    {
        request->send(200, "text/html", "Successfully made cloud connection with token: " + input_token + ".<br><a href=\"/\">Return to Home Page</a>");
        memcpy(configured_token, input_token.c_str(), input_token.length()); // save the actually valid token
        // ...
        return;
    }

    // cloud connection has NOT been made :-(
    request->send(200, "text/html", "Tried to make cloud connection with token: " + input_token + " but failed.<br><a href=\"/\">Return to Home Page</a>");
}

// setup function for the local async webserver
int web_server_setup()
{
    // serve static index.html stored in SPIFFS
    server.serveStatic("/", SPIFFS, "/").setDefaultFile("index.html");

    // define the async callback functions for different routes (= http paths)
    server.on("/login", HTTP_GET, on_http_login);
    server.on("/gpio_write", HTTP_GET, on_http_gpio_write);

    server.on("/sensor_read", HTTP_GET, on_http_sensor_read);
    server.on("/device_info", HTTP_GET, on_http_fetch_device_info);

    server.on("/set_token", HTTP_GET, on_http_set_token);
    server.on("/settings", HTTP_GET, on_http_fetch_settings);

    server.onNotFound(on_http_not_found);
    server.begin();

    return 0;
}
