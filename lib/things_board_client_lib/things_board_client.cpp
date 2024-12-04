#include <Arduino.h>

#include <Arduino_MQTT_Client.h>
#include <Server_Side_RPC.h>
#include <Attribute_Request.h>
#include <Shared_Attribute_Update.h>
#include <ThingsBoard.h>
#include <WiFi.h>

#include "things_board_client.hpp"

// Maximum size packets will ever be sent or received by the underlying MQTT client,
// if the size is to small messages might not be sent or received messages will be discarded
constexpr uint32_t MAX_MESSAGE_SIZE = 1024U;

WiFiClient wifiClient;

// Initalize the Mqtt client instance
Arduino_MQTT_Client mqttClient(wifiClient);

ThingsBoard tb(mqttClient, MAX_MESSAGE_SIZE, Default_Max_Stack_Size);

bool things_board_connected = false;

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
    Serial.printf("sent %s: %f \n", TELEMETRY_NAME_TEMPERATURE, temperature);
    Serial.printf("sent %s: %f \n", TELEMETRY_NAME_HUMIDITY, humidity);
    Serial.printf("sent %s: %f \n", TELEMETRY_NAME_WINDOW_STATUS, window_status);
}

// setup function to establish connection 
int things_board_client_setup(char const *device_token)
{
    // validate present token
    if (strlen(device_token) < 10)
    {
        Serial.printf("invalid token length: %d", device_token);
        return -1;
    }

    // only in disconnected state a reconnection attempt can be made
    if (tb.connected())
    {
        Serial.println("Things Board already connected");
        return -1;
    }

    // attempt to connect to the ThingsBoard
    Serial.printf("Connecting to: %s", THINGSBOARD_SERVER);
    Serial.printf(" with token %s", device_token);
    if (!tb.connect(THINGSBOARD_SERVER, device_token, THINGSBOARD_PORT))
    {
        Serial.println("Failed to connect");
        return -1;
    }
    else
    {
        Serial.printf("Successfully connected with %s\n", THINGSBOARD_SERVER);
        things_board_connected = true;
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
