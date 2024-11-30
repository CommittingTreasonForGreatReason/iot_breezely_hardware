#include "web_client.hpp"
#include <WiFi.h>
#include <HTTPClient.h>

unsigned long lastTime = 0;
// Set timer to 5 seconds (5000)
unsigned long timerDelay = 5000;

String serverName = "http://192.168.0.10:8080/";
WiFiClient client;
HTTPClient http;

void web_client_routine()
{
}

int web_client_setup()
{
    // Check WiFi connection status
    if (WiFi.status() == WL_CONNECTED)
    {
        Serial.print("beginning client");
        http.begin(client, serverName);

        // Specify content-type header
        http.addHeader("Content-Type", "text/plain");

        int httpResponseCode = http.POST("Hello, World!");

        Serial.print("HTTP Response code: ");
        Serial.println(httpResponseCode);

        http.end();

        return 0;
    }
    return -1;
}
