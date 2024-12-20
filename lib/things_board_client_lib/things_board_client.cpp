#include <Arduino.h>

#include <ArduinoHttpClient.h>
#include <Arduino_MQTT_Client.h>
#include <Server_Side_RPC.h>
#include <Attribute_Request.h>
#include <Shared_Attribute_Update.h>
#include <ThingsBoard.h>
#include <WiFi.h>

#include "things_board_client.hpp"
#include "user_config.hpp"
#include "logger.hpp"

// Maximum size packets will ever be sent or received by the underlying MQTT client,
// if the size is to small messages might not be sent or received messages will be discarded
constexpr uint32_t MAX_MESSAGE_SIZE = 1024U;

WiFiClient wifiClient;

// Initalize the Mqtt client instance
Arduino_MQTT_Client mqttClient(wifiClient);

// http client needed for device provisioning
HttpClient http_client = HttpClient(wifiClient, THINGSBOARD_SERVER, THINGSBOARD_PORT);

// Initalize thingsboard instance based on mqtt client
ThingsBoard tb(mqttClient, MAX_MESSAGE_SIZE, MAX_MESSAGE_SIZE, Default_Max_Stack_Size);

bool things_board_connected = false;
constexpr uint64_t REQUEST_TIMEOUT_MICROSECONDS = 5000U * 1000U;

// device provisioning related stuff
bool provision_completed = false;
char access_token[128] = {0};
/* const std::array<IAPI_Implementation *, 1U> apis = { &prov }; */

void requestTimedOut()
{
    Serial.printf("Provision request timed out did not receive a response in (%llu) microseconds. Ensure client is connected to the MQTT broker\n", REQUEST_TIMEOUT_MICROSECONDS);
}

String get_access_token() 
{
    return String(access_token);
}

void set_access_token(String token)
{
    memcpy(access_token, token.c_str(), token.length());
}

// getter for the cloud connection status
bool get_things_board_connected()
{
    return things_board_connected;
}

// routine is called periodically to send telemetry date to the things board server
void things_board_client_routine(float temperature, float humidity, bool window_status)
{
    tb.sendTelemetryData(TELEMETRY_NAME_TEMPERATURE, temperature);
    tb.sendTelemetryData(TELEMETRY_NAME_HUMIDITY, humidity);
    tb.sendTelemetryData(TELEMETRY_NAME_WINDOW_STATUS, window_status);

    // also print to the serial console for debugging purposes
    /*
    char buffer[64] = {0};
    sprintf(buffer, "sent telemetry %s: %f", TELEMETRY_NAME_TEMPERATURE, temperature);
    serial_logger_print(buffer, LOG_LEVEL_DEBUG);
    sprintf(buffer, "sent telemetry %s: %f", TELEMETRY_NAME_HUMIDITY, humidity);
    serial_logger_print(buffer, LOG_LEVEL_DEBUG);
    sprintf(buffer, "sent telemetry %s: %f", TELEMETRY_NAME_WINDOW_STATUS, window_status);
    serial_logger_print(buffer, LOG_LEVEL_DEBUG);
    */   
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
        things_board_connected = true;
        char buffer[64] = {0};
        sprintf(buffer, "Successfully connected with %s", THINGSBOARD_SERVER);
        serial_logger_print(buffer, LOG_LEVEL_DEBUG);
    }
    // Sending a MAC address as an attribute
    tb.sendAttributeData("macAddress", WiFi.macAddress().c_str());
    return 0;
}

// setup function to establish connection for new device (initial device provision process)
int things_board_client_setup_provisioning(char const *device_name)
{
    // only in disconnected state a reconnection attempt can be made
    if (tb.connected())
    {
        char buffer[64] = "Things Board already connected";
        serial_logger_print(buffer, LOG_LEVEL_ERROR);
        return -1;
    }

    // attempt to connect to the ThingsBoard
    char buffer[64] = {0};
    sprintf(buffer, "Connecting to: %s with device name %s", THINGSBOARD_SERVER, device_name);
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
        things_board_connected = true;
    }

    // now attempt to do device provision with provided device name and creds
    JsonDocument doc;
    doc["deviceName"] = device_name;
    doc["provisionDeviceKey"] = PROVISION_DEVICE_KEY;
    doc["provisionDeviceSecret"] = PROVISION_DEVICE_SECRET;

    String payload;
    serializeJson(doc, payload);
    
    // send HTTP post request for device provisioning
    // application/x-www-form-urlencoded
    http_client.post("/api/v1/provision", "application/json", payload);
    serial_logger_print("Sent out a provision request", LOG_LEVEL_INFO);

    // process response 
    serial_logger_print("Received response to provision request", LOG_LEVEL_DEBUG);
    int responseCode = http_client.responseStatusCode();
    String response = http_client.responseBody();
    serial_logger_print(response.c_str(), LOG_LEVEL_DEBUG);
    if(responseCode != HTTP_SUCCESS) {
        serial_logger_print("Provisioning failed!", LOG_LEVEL_ERROR);
        return -1;
    }

    JsonDocument docRes;
    DeserializationError error = deserializeJson(docRes, response);
    if(error) {
        Serial.println("Failed to deserialize json encoded provision response!");
        return -1;
    }

    // find access token in response and store it 
    if(docRes["credentialsType"] == "ACCESS_TOKEN" && docRes["credentialsValue"]) {
        strncpy(access_token, docRes["credentialsValue"], sizeof(access_token));
        Serial.printf("Provision Successfull. Access Token: %s", access_token);
    }

    // Sending a MAC address as an attribute
    tb.sendAttributeData("macAddress", WiFi.macAddress().c_str());
    
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
