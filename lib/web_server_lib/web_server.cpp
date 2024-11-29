#include <WiFi.h>
#include <Arduino.h>
#include <AsyncTCP.h>
#include <ESPAsyncWebServer.h>
#include <SPIFFS.h>

#include "web_server.hpp"
#include "dht_sensor.hpp"

// define http server instance on default port 80
AsyncWebServer server(80);

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

void on_http_temperature_read(AsyncWebServerRequest *request)
{
    Serial.println("Temperature was just requested from client");
    float temperature = dht_sensor_get_temperature();
    // print_temperature(temperature);
    request->send_P(200, "text/plain", String(temperature).c_str());
}

void on_http_humidity_read(AsyncWebServerRequest *request)
{
    Serial.println("Humidity was just requested from client");
    float humidity = dht_sensor_get_humidity();
    // print_humidity(humidity);
    request->send_P(200, "text/plain", String(humidity).c_str());
}

void on_window_state_read(AsyncWebServerRequest *request)
{
    Serial.println("Window sensor state was just requested from client");
    String state = digitalRead(18) == HIGH ? "open" : "closed";
    request->send_P(200, "text/plain", state.c_str());
}

// setup function for the local async webserver
int web_server_setup()
{
    // serve static index.html stored in SPIFFS
    server.serveStatic("/", SPIFFS, "/").setDefaultFile("index.html");

    // define the async callback functions for different routes (= http paths)
    server.on("/login", HTTP_GET, on_http_login);
    server.on("/gpio_write", HTTP_GET, on_http_gpio_write);
    server.on("/temperature", HTTP_GET, on_http_temperature_read);
    server.on("/humidity", HTTP_GET, on_http_humidity_read);
    server.on("/window_state", HTTP_GET, on_window_state_read);
    server.onNotFound(on_http_not_found);

    server.begin();

    return 0;
}
