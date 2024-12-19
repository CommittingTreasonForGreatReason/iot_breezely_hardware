#include <Arduino.h>

#include <Arduino_MQTT_Client.h>
#include <Server_Side_RPC.h>
#include <Attribute_Request.h>
#include <Shared_Attribute_Update.h>
#include <ThingsBoard.h>
#include <WiFi.h>

#include "things_board_client.hpp"
#include "user_config.hpp"
#include "logger.hpp"

constexpr char CREDENTIALS_TYPE[] = "credentialsType";
constexpr char CREDENTIALS_VALUE[] = "credentialsValue";
constexpr char CLIENT_ID[] = "clientId";
constexpr char CLIENT_PASSWORD[] = "password";
constexpr char CLIENT_USERNAME[] = "userName";
constexpr char ACCESS_TOKEN_CRED_TYPE[] = "ACCESS_TOKEN";
constexpr char MQTT_BASIC_CRED_TYPE[] = "MQTT_BASIC";

#define PROVISION_DEVICE_KEY "rmrx8zgkpa2tqwpptvxw"
#define PROVISION_DEVICE_SECRET "k9kmvbklxxxpclk59aui"
#define HARDCODED_TOKEN "12345token" // for some reason the other provisioning function never calls the callback so just set the token hard and maybe try to find solution later ???

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

/*
const std::array<IAPI_Implementation *, 1U> apis = {
    &prov};*/
bool provisionRequestSent = false;
bool provisionResponseProcessed = false;

ThingsBoard tb(mqttClient, MAX_MESSAGE_SIZE, MAX_MESSAGE_SIZE, Default_Max_Stack_Size);

bool things_board_connected = false;
constexpr uint64_t REQUEST_TIMEOUT_MICROSECONDS = 5000U * 1000U;

void requestTimedOut()
{
    Serial.printf("Provision request timed out did not receive a response in (%llu) microseconds. Ensure client is connected to the MQTT broker\n", REQUEST_TIMEOUT_MICROSECONDS);
}

// for some reason never gets called ???
void processProvisionResponse(const JsonDocument &data)
{
    const size_t jsonSize = Helper::Measure_Json(data);
    char buffer[jsonSize];
    serializeJson(data, buffer, jsonSize);
    Serial.printf("Received device provision response (%s)\n", buffer);
    provisionResponseProcessed = true;
    if (strncmp(data["status"], "SUCCESS", strlen("SUCCESS")) != 0)
    {
        Serial.printf("Provision response contains the error: (%s)\n", data["errorMsg"].as<const char *>());
        return;
    }

    if (strncmp(data[CREDENTIALS_TYPE], ACCESS_TOKEN_CRED_TYPE, strlen(ACCESS_TOKEN_CRED_TYPE)) == 0)
    {
        credentials.client_id = "";
        credentials.username = data[CREDENTIALS_VALUE].as<std::string>();
        credentials.password = "";
    }
    else if (strncmp(data[CREDENTIALS_TYPE], MQTT_BASIC_CRED_TYPE, strlen(MQTT_BASIC_CRED_TYPE)) == 0)
    {
        auto credentials_value = data[CREDENTIALS_VALUE].as<JsonObjectConst>();
        credentials.client_id = credentials_value[CLIENT_ID].as<std::string>();
        credentials.username = credentials_value[CLIENT_USERNAME].as<std::string>();
        credentials.password = credentials_value[CLIENT_PASSWORD].as<std::string>();
    }
    else
    {
        char buffer[64] = {0};
        sprintf(buffer, "Unexpected provision credentialsType: (%s)\n", data[CREDENTIALS_TYPE].as<const char *>());
        serial_logger_print(buffer, LOG_LEVEL_ERROR);
        return;
    }

    if (tb.connected())
    {
        tb.disconnect();
    }
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
    char buffer[64] = {0};
    sprintf(buffer, "sent telemetry %s: %f", TELEMETRY_NAME_TEMPERATURE, temperature);
    serial_logger_print(buffer, LOG_LEVEL_DEBUG);
    sprintf(buffer, "sent telemetry %s: %f", TELEMETRY_NAME_HUMIDITY, humidity);
    serial_logger_print(buffer, LOG_LEVEL_DEBUG);
    sprintf(buffer, "sent telemetry %s: %f", TELEMETRY_NAME_WINDOW_STATUS, window_status);
    serial_logger_print(buffer, LOG_LEVEL_DEBUG);
}

// setup function to establish connection
int things_board_client_setup(char const *device_token)
{
    // validate present token
    if (strlen(device_token) < 10)
    {
        char buffer[64] = {0};
        sprintf(buffer, "invalid token length: %d", device_token);
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
    sprintf(buffer, "Connecting to: %s with token %s", THINGSBOARD_SERVER, device_token);
    serial_logger_print(buffer, LOG_LEVEL_DEBUG);

    if (!tb.connect(THINGSBOARD_SERVER, device_token, THINGSBOARD_PORT))
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
    // Sending a MAC address as an attribute
    tb.sendAttributeData("macAddress", WiFi.macAddress().c_str());
    // const Provision_Callback provisionCallback(Access_Token(), &processProvisionResponse, PROVISION_DEVICE_KEY, PROVISION_DEVICE_SECRET, device_name, REQUEST_TIMEOUT_MICROSECONDS, &requestTimedOut); // doesn't work :(
    /*
    const Provision_Callback provisionCallback(Device_Access_Token(), &processProvisionResponse, PROVISION_DEVICE_KEY, PROVISION_DEVICE_SECRET, HARDCODED_TOKEN, device_name, REQUEST_TIMEOUT_MICROSECONDS, &requestTimedOut);

    provisionRequestSent = prov.Provision_Request(provisionCallback);
    if (provisionRequestSent)
        Serial.println("send provision request");*/
    return provisionRequestSent ? 0 : -1;
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
