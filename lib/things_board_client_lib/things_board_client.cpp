#include <Arduino.h>

#include <Arduino_MQTT_Client.h>
#include <Server_Side_RPC.h>
#include <Attribute_Request.h>
#include <Shared_Attribute_Update.h>
#include <ThingsBoard.h>
#include <WiFi.h>

#define TOKEN_PAUL_TEST_DEVICE "2aq5mz3oTq3pGyp4f64M"
#define THINGSBOARD_SERVER "server-malte.duckdns.org"
#define THINGSBOARD_PORT 5869 //1883U

// Maximum size packets will ever be sent or received by the underlying MQTT client,
// if the size is to small messages might not be sent or received messages will be discarded
constexpr uint32_t MAX_MESSAGE_SIZE = 1024U;

WiFiClient wifiClient;

// Initalize the Mqtt client instance
Arduino_MQTT_Client mqttClient(wifiClient);

ThingsBoard tb(mqttClient,MAX_MESSAGE_SIZE, Default_Max_Stack_Size);

void things_board_routine(float temperature, float humidity){

    char temperature_str[32] = {0};
    char humidity_str[32] = {0};

    snprintf(temperature_str,32,"%f",temperature);
    snprintf(humidity_str,32,"%f",humidity_str);
    tb.sendTelemetryData("temperature", temperature);
    tb.sendTelemetryData("humidity", humidity);

    Serial.printf("sent temperature: %s \n", temperature_str);
    Serial.printf("sent humidity: %s \n", humidity_str);
}

int things_board_client_setup()
{
    if (!tb.connected()) {
        // Connect to the ThingsBoard
        Serial.print("Connecting to: ");
        Serial.print(THINGSBOARD_SERVER);
        Serial.print(" with token ");
        Serial.println(TOKEN_PAUL_TEST_DEVICE);
        if (!tb.connect(THINGSBOARD_SERVER, TOKEN_PAUL_TEST_DEVICE, THINGSBOARD_PORT)) {
        Serial.println("Failed to connect");
        return -1;
        }
        // Sending a MAC address as an attribute
        tb.sendAttributeData("macAddress", WiFi.macAddress().c_str());
    }
    return 0;
}
