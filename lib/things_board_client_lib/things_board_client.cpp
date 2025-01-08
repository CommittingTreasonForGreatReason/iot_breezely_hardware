#include <Arduino.h>

#include <Arduino_Http_Client.h>
#include <Arduino_MQTT_Client.h>
#include <Server_Side_RPC.h>
#include <Attribute_Request.h>
#include <Shared_Attribute_Update.h>
#include <ThingsBoard.h>
#include <ThingsBoardHttp.h>
#include <WiFi.h>

#include "things_board_client.hpp"
#include "provision.h"
#include "user_config.hpp"
#include "logger.hpp"
#include "data_communications.hpp"

#include "breezely_persistency.hpp"

#define PROVISION_DEVICE_KEY "oyjtd3506udhrt7afv1p"
#define PROVISION_DEVICE_SECRET "1v1202a059muuxfmp3f5"

// Struct for client connecting after provisioning
struct Credentials
{
    std::string client_id;
    std::string username;
    std::string password;
} credentials;

// Maximum size packets will ever be sent or received by the underlying MQTT client,
// if the size is to small messages might not be sent or received messages will be discarded
constexpr uint32_t MAX_MESSAGE_SIZE = 1024U;

WiFiClient wifiClient;

// Initalize the Mqtt client instance
Arduino_MQTT_Client mqttClient(wifiClient);

Server_Side_RPC<3U, 5U> rpc;

Provision<> prov;
const std::array<IAPI_Implementation *, 2U> apis = {
    &rpc,
    &prov};

// Initalize thingsboard instance based on mqtt client
ThingsBoard tb(mqttClient, MAX_MESSAGE_SIZE, MAX_MESSAGE_SIZE, Default_Max_Stack_Size, apis);

constexpr uint64_t REQUEST_TIMEOUT_MICROSECONDS = 5000U * 1000U;

// device provisioning related stuff
bool provisionRequestSent = false;
bool provisionResponseProcessed = false;

void processRpcIdentify(const JsonVariantConst &data, JsonDocument &response)
{
    Serial.println("Received the rpc identify state RPC method");
}
const std::array<RPC_Callback, 1U> callbacks = {
    RPC_Callback{"rpcIdentify", processRpcIdentify}};

// getter for the cloud connection status
bool get_things_board_connected()
{
    return tb.connected();
}

// routine is called periodically to send telemetry date to the things board server
void things_board_client_routine(float temperature, float humidity, bool window_status)
{
    try_send_temperature(&tb, temperature);
    try_send_humidity(&tb, humidity);
    try_send_window_status(&tb, window_status);
}

// setup function to establish connection with existing device token (provision already completed)
int things_board_client_setup(char const *access_token)
{
    // validate present token
    if (strlen(access_token) < 10)
    {
        char buffer[64] = {0};
        sprintf(buffer, "invalid token length: %d", access_token);
        serial_logger_print(buffer, LOG_LEVEL_ERROR);
        return -1;
    }

    // only in disconnected state a reconnection attempt can be made
    if (tb.connected())
    {
        char buffer[64] = "Things Board already connected";
        serial_logger_print(buffer, LOG_LEVEL_ERROR);
        return -1;
    }

    // attempt to connect to the ThingsBoard
    char buffer[64] = {0};
    sprintf(buffer, "Connecting to: %s with token %s", THINGSBOARD_SERVER, access_token);
    serial_logger_print(buffer, LOG_LEVEL_DEBUG);

    if (!tb.connect(THINGSBOARD_SERVER, access_token, THINGSBOARD_PORT))
    {
        char buffer[64] = {0};
        sprintf(buffer, "Failed to connect to: %s", THINGSBOARD_SERVER);
        serial_logger_print(buffer, LOG_LEVEL_ERROR);
        return -1;
    }
    else
    {
        char buffer[64] = {0};
        sprintf(buffer, "Successfully connected with %s", THINGSBOARD_SERVER);
        serial_logger_print(buffer, LOG_LEVEL_DEBUG);
    }

    Serial.println("Subscribing for RPC...");
    // Perform a subscription. All consequent data processing will happen in
    if (!rpc.RPC_Subscribe(callbacks.cbegin(), callbacks.cend()))
    {
        Serial.println("Failed to subscribe for RPC");
        return -1;
    }

    // Sending a MAC address as an attribute
    tb.sendAttributeData("macAddress", WiFi.macAddress().c_str());
    return 0;
}

int connect_thingsboard_timeout_ms(const int64_t timeout_ms)
{
    int64_t timeout_deadline_us = esp_timer_get_time() / 1000 + timeout_ms;

    while (!tb.connect(THINGSBOARD_SERVER, PROV_ACCESS_TOKEN, THINGSBOARD_PORT))
    {
        if (esp_timer_get_time() >= timeout_deadline_us)
        {
            char buffer[64] = {0};
            sprintf(buffer, "Failed to connect to: %s", THINGSBOARD_SERVER);
            serial_logger_print(buffer, LOG_LEVEL_ERROR);
            return -1;
        }
    }
    char buffer[64] = {0};
    sprintf(buffer, "Successfully connected with %s", THINGSBOARD_SERVER);
    serial_logger_print(buffer, LOG_LEVEL_DEBUG);
}

int provision_http(const char *device_name)
{
    // HTTP
    WiFiClient wifiClient_temp;

    Arduino_HTTP_Client httpClient(wifiClient_temp, THINGSBOARD_SERVER, 5868);
    int error = httpClient.connect(THINGSBOARD_SERVER, 5868);
    Serial.println("ERROR is");
    Serial.println(error);
    httpClient.set_keep_alive(true);
    JsonDocument jsonDoc;
    jsonDoc["deviceName"] = String(device_name);
    jsonDoc["provisionDeviceKey"] = PROVISION_DEVICE_KEY;
    jsonDoc["provisionDeviceSecret"] = PROVISION_DEVICE_SECRET;

    String jsonString;
    serializeJson(jsonDoc, jsonString);
    error = httpClient.post("/api/v1/provision", "application/json", jsonString.c_str());
    Serial.println("ERROR is");
    Serial.println(error);
    delay(1000);
    Serial.println("Status code is");
    Serial.println(httpClient.get_response_status_code());
    Serial.println("Body is");
    std::string body = httpClient.get_response_body().c_str();
    Serial.println(body.c_str());
    JsonDocument newJsonDoc;
    String body_temp = body.c_str();
    deserializeJson(newJsonDoc, body_temp);
    String success = newJsonDoc["status"];
    Serial.println(success.c_str());
    if (strcmp(success.c_str(), "SUCCESS") == 0)
    {
        Serial.println("Provisioning success!!!");
        String token_str = newJsonDoc["credentialsValue"];
        try_set_stored_device_name(device_name);
        try_set_stored_token(token_str.c_str());
        store_config_to_flash();
        httpClient.stop();
        return 0;
    }
    else
    {
        httpClient.stop();
        return 0;
    }
}

// setup function to establish connection for new device (initial device provision process)
int things_board_client_setup_provisioning(const char *device_name)
{
    if (try_get_stored_token() == nullptr || try_get_stored_device_name() == nullptr)
    {
        int error = provision_http(device_name);
        if (error < 0)
            return error;
        load_config_from_flash();
    }

    delay(1000);
    // only in disconnected state a reconnection attempt can be made
    /*
    if (tb.connected())
    {
        char buffer[64] = "Things Board already connected";
        serial_logger_print(buffer, LOG_LEVEL_ERROR);
        return -1;
    }

    // attempt to connect to the ThingsBoard
    char buffer[64] = {0};
    sprintf(buffer, "Connecting to: %s with device name %s and customer name %s", THINGSBOARD_SERVER, device_name, customer_name);
    serial_logger_print(buffer, LOG_LEVEL_DEBUG);
    if (!tb.connect(THINGSBOARD_SERVER, PROV_ACCESS_TOKEN, THINGSBOARD_PORT))
    {
        char buffer[64] = {0};
        sprintf(buffer, "Failed to connect to: %s", THINGSBOARD_SERVER);
        serial_logger_print(buffer, LOG_LEVEL_ERROR);
        return -1;
    }
    else
    {
        char buffer[64] = {0};
        sprintf(buffer, "Successfully connected with %s", THINGSBOARD_SERVER);
        serial_logger_print(buffer, LOG_LEVEL_DEBUG);
    }
    // const Provision_Callback provisionCallback(Access_Token(), &processProvisionResponse, PROVISION_DEVICE_KEY, PROVISION_DEVICE_SECRET, device_name, REQUEST_TIMEOUT_MICROSECONDS, &requestTimedOut); // doesn't work :(

    const Provision_Callback provisionCallback(Device_Access_Token(), &processProvisionResponse, PROVISION_DEVICE_KEY, PROVISION_DEVICE_SECRET, token, device_name, REQUEST_TIMEOUT_MICROSECONDS, &requestTimedOut);

    provisionRequestSent = prov.Provision_Request(provisionCallback);
    if (provisionRequestSent)
        serial_logger_print("send provision request", LOG_LEVEL_DEBUG);

    delay(2000);
    tb.disconnect();

    delay(2000);
    */

    Serial.println(try_get_stored_token());
    if (try_get_stored_token() == nullptr || try_get_stored_device_name() == nullptr)
    {
        return -1;
    }
    if (!tb.connect(THINGSBOARD_SERVER, try_get_stored_token(), THINGSBOARD_PORT))
    {
        char buffer[64] = {0};
        sprintf(buffer, "Failed to connect to: %s", THINGSBOARD_SERVER);
        serial_logger_print(buffer, LOG_LEVEL_ERROR);
        return -1;
    }
    else
    {
        char buffer[64] = {0};
        sprintf(buffer, "Successfully connected with %s", THINGSBOARD_SERVER);
        serial_logger_print(buffer, LOG_LEVEL_DEBUG);
    }

    Serial.println("Subscribing for RPC...");
    // Perform a subscription. All consequent data processing will happen in
    if (!rpc.RPC_Subscribe(callbacks.cbegin(), callbacks.cend()))
    {
        Serial.println("Failed to subscribe for RPC");
        return -1;
    }
    return 0;
}

// disconnect routine
int things_board_client_teardown()
{
    if (tb.connected())
    {
        tb.disconnect();
        return 0;
    }
    return -1; // not connected to begin with
}
