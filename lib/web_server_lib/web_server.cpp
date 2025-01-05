#include <WiFi.h>
#include <Arduino.h>
#include <AsyncTCP.h>
#include <ESPAsyncWebServer.h>
#include <SPIFFS.h>
#include <ArduinoJson.h>

#include "web_server.hpp"
#include "things_board_client.hpp"
#include "user_config.hpp"
#include "logger.hpp"

#ifdef __USE_DATA_FABRICATION
#include "data_fabricator.hpp"
#else
#include "dht_sensor.hpp"
#endif

// define http server instance on default port 80
AsyncWebServer server(80);

char configured_token[32] = {0};
char configured_device_name[32] = {0};
char configured_customer_name[32] = {0};
bool cloud_connected = false;

uint16_t identify_start_ms = 0;
uint16_t last_identify_toggle_ms = 0;
#define IDENTIFY_DURATION_MS 5000

String device_name = "";

char *get_configured_device_name()
{
    return configured_device_name;
}

void identify_loop()
{
    uint64_t current_time_ms = esp_timer_get_time() / 1000;
    if (identify_start_ms + IDENTIFY_DURATION_MS > current_time_ms)
    {
        if (current_time_ms - last_identify_toggle_ms > 100)
        {
            digitalWrite(WINDOW_STATUS_LED_PIN, !digitalRead(WINDOW_STATUS_LED_PIN)); // toggle
            last_identify_toggle_ms = current_time_ms;
        }
    }
    else
    {
        digitalWrite(WINDOW_STATUS_LED_PIN, LOW);
    }
}

String outputState(int output)
{
    return digitalRead(output) ? "checked" : "";
}

void send_http_response_json_format(AsyncWebServerRequest *request, int code, JsonDocument *jsonDoc, bool printResponse = false)
{
    String jsonResponse;
    serializeJson(*jsonDoc, jsonResponse);
    request->send(code, "application/json", jsonResponse);
    if (printResponse)
    {
        serial_logger_print(jsonResponse.c_str(), LOG_LEVEL_DEBUG);
    }
}

// callback for http requests on undefined routes
void on_http_not_found(AsyncWebServerRequest *request)
{
    request->send(404, "text/plain", "Not found");
}

// callback function to write the digital output pin state via GET request
void on_http_identify(AsyncWebServerRequest *request)
{
    serial_logger_print("identify invoked", LOG_LEVEL_DEBUG);
    identify_start_ms = esp_timer_get_time() / 1000;

    File html_file = SPIFFS.open("/webdir/index.html");
    uint32_t size = html_file.size();
    char buffer[size] = {0};
    html_file.readBytes(buffer, size);
    request->send(200, "text/html", buffer);
}

// callback function for sending sensor measurements to the browser on demand
void on_http_sensor_read(AsyncWebServerRequest *request)
{
    serial_logger_print("-- > sensor read request from client", LOG_LEVEL_DEBUG);

    // store sensor data as dictionary of key-value-pairs
    JsonDocument jsonDoc;

#ifdef __USE_DATA_FABRICATION
    jsonDoc["window-state"] = data_fabricator_get_window_status() ? "open" : "closed";
    jsonDoc["air-temperature"] = data_fabricator_get_temperature();
    jsonDoc["air-humidity"] = data_fabricator_get_humidity();
#else
    jsonDoc["window-state"] = digitalRead(MAGNET_INPUT_PIN) == HIGH ? "open" : "closed";
    jsonDoc["air-temperature"] = dht_sensor_get_temperature();
    jsonDoc["air-humidity"] = dht_sensor_get_humidity();
#endif

    // ...

    // send http response with json encoded payload
    send_http_response_json_format(request, 200, &jsonDoc);
}

// callback function for handling fetch device info request
void on_http_fetch_device_info(AsyncWebServerRequest *request)
{
    Serial.println("--> device info request from client");

    JsonDocument jsonDoc;
    jsonDoc["device-name"] = device_name;
    jsonDoc["uptime"] = "dd hh:mm:ss";
    jsonDoc["wifi-ssid"] = WiFi.SSID();
    jsonDoc["ipv4-address"] = WiFi.localIP().toString();
    jsonDoc["mac-address"] = WiFi.macAddress();
    jsonDoc["hostname"] = WiFi.getHostname();
    jsonDoc["cloud-connection-status"] = cloud_connected ? "online" : "offline";
    // ...

    send_http_response_json_format(request, 200, &jsonDoc);
}

// callback function for handling fetch settings request
void on_http_fetch_settings(AsyncWebServerRequest *request)
{
    Serial.println("--> settings request from client");

    JsonDocument jsonDoc;
    jsonDoc["token"] = (strlen(configured_token) >= 10) ? configured_token : "not configured";
    jsonDoc["device-name"] = (strlen(configured_device_name) >= 5) ? configured_device_name : "not set";
    jsonDoc["customer-name"] = (strlen(configured_customer_name) >= 5) ? configured_customer_name : "not set";
    // ...

    send_http_response_json_format(request, 200, &jsonDoc);
}
/*
// callback for things board token authorization request
void on_http_set_token(AsyncWebServerRequest *request)
{
    // check for token paramater
    String input_token = "";
    if (request->hasParam(TOKEN_INPUT_NAME))
    {
        input_token = request->getParam(TOKEN_INPUT_NAME)->value();
    }

    char buffer[64] = {0};
    sprintf(buffer, "token: %s", input_token);
    serial_logger_print(buffer, LOG_LEVEL_DEBUG);

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
        set_access_token(input_token);      // save the actually valid token
        // memcpy(configured_token, input_token.c_str(), input_token.length());
        // ...
        return;
    }

    // cloud connection has NOT been made :-(
    request->send(200, "text/html", "Tried to make cloud connection with token: " + input_token + " but failed.<br><a href=\"/\">Return to Home Page</a>");
}*/

// callback for things board token authorization request
void on_http_set_device_name(AsyncWebServerRequest *request)
{
    // check for token paramater
    String input_device_name = "";
    if (request->hasParam(DEVICE_NAME_INPUT_NAME))
    {
        input_device_name = request->getParam(DEVICE_NAME_INPUT_NAME)->value();
    }
    String input_customer_name = "";
    if (request->hasParam(CUSTOMER_NAME_INPUT_NAME))
    {
        input_customer_name = request->getParam(CUSTOMER_NAME_INPUT_NAME)->value();
    }

    char buffer[64] = {0};
    sprintf(buffer, "device_name: %s", input_device_name);
    serial_logger_print(buffer, LOG_LEVEL_DEBUG);
    sprintf(buffer, "customer_name: %s", input_customer_name);
    serial_logger_print(buffer, LOG_LEVEL_DEBUG);
    if (input_device_name.length() <= 5 || input_customer_name.length() <= 5) // input names to short
    {
        request->send(200, "text/html", "Invalid device name OR customer name length!!! <br><a href=\"/\">Return to Home Page</a>");
        return;
    }

    // when token is changed then force to reconnect with new one
    if (cloud_connected)
    {
        // tear down connection if already connected with a token
        things_board_client_teardown();
    }
    const char *token = WiFi.macAddress().c_str();
    cloud_connected = things_board_client_setup_provisioning(input_device_name.c_str(), input_customer_name.c_str(), token) >= 0; //  using mac address as token for now (kindof problematic but oh well (͡ ° ͜ʖ ͡ °) )
    if (cloud_connected)                                                                                                          // cloud connection has been made :-)
    {
        request->send(200, "text/html", "Successfully made cloud connection with device name: " + input_device_name + " | customer name: " + input_customer_name + ".<br><a href=\"/\">Return to Home Page</a>");
        memcpy(configured_device_name, input_device_name.c_str(), input_device_name.length());
        memcpy(configured_customer_name, input_customer_name.c_str(), input_customer_name.length());
        memcpy(configured_token, token, strlen(token));
        // ...
        return;
    }

    // cloud connection has NOT been made :-(
    request->send(200, "text/html", "Tried to make cloud connection with device name: " + input_device_name + " | customer name: " + input_customer_name + " but failed.<br><a href=\"/\">Return to Home Page</a>");
}

// setup function for the local async webserver
int web_server_setup()
{
    // serve static index.html stored in SPIFFS
    server.serveStatic("/", SPIFFS, "/webdir").setDefaultFile("index.html");

    // define the async callback functions for different routes (= http paths)
    server.on("/identify", HTTP_GET, on_http_identify);

    server.on("/sensor_read", HTTP_GET, on_http_sensor_read);
    server.on("/device_info", HTTP_GET, on_http_fetch_device_info);

    // server.on("/set_token", HTTP_GET, on_http_set_token);
    server.on("/set_device_name", HTTP_GET, on_http_set_device_name);
    server.on("/settings", HTTP_GET, on_http_fetch_settings);

    server.onNotFound(on_http_not_found);
    server.begin();

    return 0;
}
