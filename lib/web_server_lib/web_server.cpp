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
#include "breezely_persistency.hpp"

#ifdef __USE_DATA_FABRICATION
#include "data_fabricator.hpp"
#else
#include "dht_sensor.hpp"
#endif

// define http server instance on default port 80
AsyncWebServer server(80);

bool cloud_connected = false;

int64_t identify_start_ms = 0;
int64_t last_identify_toggle_ms = 0;
#define IDENTIFY_DURATION_MS 5000

void set_cloud_connection_status(bool connected)
{
    cloud_connected = connected;
}

void identify_loop()
{
    int64_t current_time_ms = esp_timer_get_time() / 1000;
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
    if (try_get_stored_device_name() != nullptr && try_get_stored_device_name_extension() != nullptr)
    {
        char buffer[64] = {0};
        sprintf(buffer, "%s_%s", try_get_stored_device_name(), try_get_stored_device_name_extension());
        jsonDoc["device-name"] = buffer;
    }
    else
    {
        jsonDoc["device-name"] = "not set";
    }

    uint64_t secsRemaining = millis() / 1000;
    int32_t runDays = secsRemaining / (3600 * 24);
    secsRemaining = secsRemaining % (3600 * 24);
    int32_t runHours = secsRemaining / 3600;
    secsRemaining = secsRemaining % 3600;
    int32_t runMinutes = secsRemaining / 60;
    int32_t runSeconds = secsRemaining % 60;

    char uptime_formatted_str[21];
    sprintf(uptime_formatted_str, "%02d:%02d:%02d:%02d", runDays, runHours, runMinutes, runSeconds);

    jsonDoc["uptime"] = String(uptime_formatted_str);
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
    if (try_get_stored_device_name() != nullptr && try_get_stored_device_name_extension() != nullptr)
    {
        char buffer[64] = {0};
        sprintf(buffer, "%s_%s", try_get_stored_device_name(), try_get_stored_device_name_extension());
        jsonDoc["device-name"] = buffer;
    }
    else
    {
        jsonDoc["device-name"] = "not set";
    }
    jsonDoc["token"] = (try_get_stored_token() != nullptr) ? try_get_stored_token() : "not set";
    // ...

    send_http_response_json_format(request, 200, &jsonDoc);
}

void on_http_fetch_cloud_connection_status(AsyncWebServerRequest *request)
{
    Serial.println("--> cloud connection status request from client");

    JsonDocument jsonDoc;
    jsonDoc["cloud-connection-status"] = cloud_connected ? "cloud connection made" : "unable to connect to cloud";
    char configured_host_name[128] = {0};
    if (try_get_stored_device_name() != nullptr)
    {
        sprintf(configured_host_name, "%s-%s", HOSTNAME, try_get_stored_device_name());
    }
    else
    {
        sprintf(configured_host_name, "%s", HOSTNAME);
    }

    jsonDoc["configured-hostname"] = cloud_connected ? configured_host_name : "";

    char old_hyperlink[128] = {0};
    char new_hyperlink[128] = {0};
    sprintf(old_hyperlink, "http://%s", HOSTNAME);
    sprintf(new_hyperlink, "http://%s", configured_host_name);
    // ...
    jsonDoc["configured-hyperlink"] = cloud_connected ? new_hyperlink : old_hyperlink;

    send_http_response_json_format(request, 200, &jsonDoc);
}

char allowed_characters[] = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz123456789";
String breezely_get_random_string()
{
    uint8_t random_numbers[8 + 1];
    esp_fill_random(random_numbers, 8);
    for (uint8_t i = 0; i < 8;)
    {
        if (strchr(allowed_characters, random_numbers[i]) != NULL)
            i++;
        else
            random_numbers[i] = (uint8_t)esp_random();
    }
    random_numbers[8] = 0; // end string
    return String((char *)random_numbers);
}

// callback for things board token authorization request
void on_http_set_device_name(AsyncWebServerRequest *request)
{
    Serial.println("--> set device name request from client");
    // check for token paramater
    String input_device_name = "";
    if (request->hasParam(DEVICE_NAME_INPUT_NAME))
    {
        input_device_name = request->getParam(DEVICE_NAME_INPUT_NAME)->value();
    }

    String generated_device_name_extension = breezely_get_random_string();

    String extended_device_name = input_device_name + "_" + generated_device_name_extension;
    char buffer[64] = {0};
    sprintf(buffer, "extended_device_name: %s", extended_device_name.c_str());
    serial_logger_print(buffer, LOG_LEVEL_DEBUG);
    if (input_device_name.length() <= DEVICE_NAME_SIZE_MIN) // input names to short
    {
        request->send(200, "text/html", "Invalid device name OR customer name length!!! <br><a href=\"/\">Return to Home Page</a>");
        return;
    }

    if (extended_device_name.length() > DEVICE_NAME_SIZE_MAX) // input names to short
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
    serial_logger_print("starting thingsboard client", LOG_LEVEL_DEBUG);
    // set a new hostname (<default_hostname>-<device_name>)
    bool success = (things_board_client_setup_provisioning(input_device_name.c_str(), generated_device_name_extension.c_str(), true) >= 0); // true for force profisioning
    // respond with seperate cloud connectiosn status display page
    File html_file = SPIFFS.open("/webdir/cloud_connection.html");
    uint32_t size = html_file.size();
    char file_buffer[size] = {0};
    html_file.readBytes(file_buffer, size);
    request->send(200, "text/html", file_buffer);
    cloud_connected = success;
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
    server.on("/cloud_connection_status", HTTP_GET, on_http_fetch_cloud_connection_status);

    server.onNotFound(on_http_not_found);
    server.begin();

    return 0;
}
