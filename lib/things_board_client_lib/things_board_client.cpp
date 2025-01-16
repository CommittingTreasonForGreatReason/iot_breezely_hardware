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
char debug_buffer[512] = {0};

// Maximum size packets will ever be sent or received by the underlying MQTT client,
// if the size is to small messages might not be sent or received messages will be discarded
constexpr uint32_t MAX_MESSAGE_SIZE = 1024U;
constexpr int32_t thingsboard_connection_timeout_ms = 5000;

WiFiClient wifiClient;

// Initalize the Mqtt client instance
Arduino_MQTT_Client mqttClient(wifiClient);

Server_Side_RPC<3U, 5U> rpc;

const std::array<IAPI_Implementation *, 2U> apis = {
    &rpc};

// Initalize thingsboard instance based on mqtt client
ThingsBoard tb(mqttClient, MAX_MESSAGE_SIZE, MAX_MESSAGE_SIZE, Default_Max_Stack_Size, apis);

constexpr uint64_t REQUEST_TIMEOUT_MICROSECONDS = 5000U * 1000U;

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
bool things_board_client_routine(float temperature, float humidity, bool window_status)
{
    if (!try_send_temperature(&tb, temperature))
        return false;
    if (!try_send_humidity(&tb, humidity))
        return false;
    if (!try_send_window_status(&tb, window_status))
        return false;
    return true;
}

int provision_http(const char *device_name, const char *device_name_extension)
{
    serial_logger_print("provisioning via http...", LOG_LEVEL_DEBUG);
    // HTTP

    WiFiClient wifiClient_temp;
    Arduino_HTTP_Client httpClient(wifiClient_temp, THINGSBOARD_SERVER, THINGSBOARD_HTTP_PORT);
    if (!httpClient.connect(THINGSBOARD_SERVER, THINGSBOARD_HTTP_PORT))
    {
        sprintf(debug_buffer, "Failed to connect via http with %s:%d", THINGSBOARD_SERVER, THINGSBOARD_HTTP_PORT);
        serial_logger_print(debug_buffer, LOG_LEVEL_ERROR);
        return -1;
    }
    sprintf(debug_buffer, "Successfully connecte via http with %s:%d", THINGSBOARD_SERVER, THINGSBOARD_HTTP_PORT);
    serial_logger_print(debug_buffer, LOG_LEVEL_DEBUG);

    httpClient.set_keep_alive(true);
    JsonDocument jsonDoc;

    char extended_device_name[64] = {0};
    sprintf(extended_device_name, "%s_%s", device_name, device_name_extension);
    jsonDoc["deviceName"] = String(extended_device_name);
    jsonDoc["provisionDeviceKey"] = PROVISION_DEVICE_KEY;
    jsonDoc["provisionDeviceSecret"] = PROVISION_DEVICE_SECRET;

    String jsonString;
    serializeJson(jsonDoc, jsonString);

    sprintf(debug_buffer, "provisioning with device_name %s", extended_device_name);
    serial_logger_print(debug_buffer, LOG_LEVEL_DEBUG);

    if (httpClient.post("/api/v1/provision", "application/json", jsonString.c_str()))
    {
        serial_logger_print("Failed to post provisioning", LOG_LEVEL_ERROR);
        return -1;
    }

    int status_code = httpClient.get_response_status_code();
    std::string body = httpClient.get_response_body().c_str();

    sprintf(debug_buffer, "provisioning response: code[%d] - body[%s]", status_code, body.c_str());
    serial_logger_print(debug_buffer, LOG_LEVEL_DEBUG);

    JsonDocument newJsonDoc;
    String body_temp = body.c_str();
    deserializeJson(newJsonDoc, body_temp);
    String success = newJsonDoc["status"];
    if (strcmp(success.c_str(), "SUCCESS") == 0)
    {
        serial_logger_print("provisioned device successfully", LOG_LEVEL_INFO);
        String token_str = newJsonDoc["credentialsValue"];
        try_set_stored_device_name(device_name);
        try_set_stored_device_name_extension(device_name_extension);
        try_set_stored_token(token_str.c_str());
        store_config_to_flash();
        httpClient.stop();
        return 0;
    }
    else
    {
        serial_logger_print("failed to provision device", LOG_LEVEL_ERROR);
        httpClient.stop();
        return 0;
    }
}

// setup function to establish connection for new device (initial device provision process)
int things_board_client_setup_provisioning(const char *device_name, const char *device_name_extension, bool force)
{
    if (try_get_stored_token() == nullptr || try_get_stored_device_name() == nullptr || try_get_stored_device_name_extension() == nullptr || force)
    {
        if (provision_http(device_name, device_name_extension) < 0)
            return -1;
        load_config_from_flash();
    }
    int64_t timeout_deadline_ms = millis() + thingsboard_connection_timeout_ms;
    uint32_t delay_ms = 500;

    if (tb.connected())
    {
        tb.disconnect();
    }

    while (!tb.connect(THINGSBOARD_SERVER, try_get_stored_token(), THINGSBOARD_MQTT_PORT))
    {
        if (millis() >= timeout_deadline_ms)
        {
            sprintf(debug_buffer, "Failed to connect to: %s:%d with token[%s] after timeout_ms[%d]", THINGSBOARD_SERVER, THINGSBOARD_MQTT_PORT, try_get_stored_token(), thingsboard_connection_timeout_ms);
            serial_logger_print(debug_buffer, LOG_LEVEL_ERROR);
            return -1;
        }
        delay(delay_ms);
        delay_ms += 1000;
    }
    sprintf(debug_buffer, "Successfully connected with %s:%d with token[%s]", THINGSBOARD_SERVER, THINGSBOARD_MQTT_PORT, try_get_stored_token());
    serial_logger_print(debug_buffer, LOG_LEVEL_DEBUG);

    /*
        Serial.println("Subscribing for RPC...");
        // Perform a subscription. All consequent data processing will happen in
        if (!rpc.RPC_Subscribe(callbacks.cbegin(), callbacks.cend()))
        {
            Serial.println("Failed to subscribe for RPC");
            return -1;
        }*/
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
