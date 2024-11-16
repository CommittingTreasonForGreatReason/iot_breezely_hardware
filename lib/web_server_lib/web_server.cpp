#include "web_server.hpp"
#include <WiFi.h>
#include <AsyncTCP.h>
#include <ESPAsyncWebServer.h>
#include <Arduino.h>

AsyncWebServer server(80);

const char index_html[] PROGMEM = R"rawliteral(
<!DOCTYPE HTML>
<html>
<head>
  <title>ESP Web Server</title>
  <meta name="viewport" content="width=device-width, initial-scale=1">
  <link rel="icon" href="data:,">
</head>
<body>
  <form action="/get">
    username: <input type="text" name="username">
    password: <input type="text" name="password">
    <input type="submit" value="Submit">
  </form><br>
  %BUTTONPLACEHOLDER%
  <script>function toggleCheckbox(element) {
    var xhr = new XMLHttpRequest();
    if(element.checked){ xhr.open("GET", "/update?output="+element.id+"&state=1", true); }
    else { xhr.open("GET", "/update?output="+element.id+"&state=0", true); }
    xhr.send();
    }
   </script>
</body></html>)rawliteral";

String outputState(int output)
{
    return digitalRead(output) ? "checked" : "";
}

// Replaces placeholder with button section in your web page
String processor(const String &var)
{
    if (var == "BUTTONPLACEHOLDER")
    {
        String buttons = "";
        buttons += "Output - GPIO " + String(HTTP_OUTPUT_PIN) + "<label class=\"switch\"><input type=\"checkbox\" onchange=\"toggleCheckbox(this)\" id=\"" + String(HTTP_OUTPUT_PIN) + "\" " + outputState(HTTP_OUTPUT_PIN) + "><span class=\"slider\"></span></label>";
        return buttons;
    }
    return String();
}

void not_http_found(AsyncWebServerRequest *request)
{
    request->send(404, "text/plain", "Not found");
}

void on_http_root(AsyncWebServerRequest *request)
{
    request->send_P(200, "text/html", index_html, processor);
}

void on_http_get(AsyncWebServerRequest *request)
{
    String input_username;
    String input_password;
    // GET username value on <ESP_IP>/get?username=<inputMessage>
    if (request->hasParam(USERNAME_INPUT_NAME))
    {
        input_username = request->getParam(USERNAME_INPUT_NAME)->value();
    }
    else
    {
        input_username = "No message sent";
    }
    // GET password value on <ESP_IP>/get?password=<inputMessage>
    if (request->hasParam(PASSWORD_INPUT_NAME))
    {
        input_password = request->getParam(PASSWORD_INPUT_NAME)->value();
    }
    else
    {
        input_password = "No message sent";
    }

    Serial.print("username: ");
    Serial.println(input_username);
    Serial.print("password: ");
    Serial.println(input_password);

    request->send(200, "text/html", "HTTP GET request with user login send to esp32 (username: " + input_username + " | password: " + input_password + ")<br><a href=\"/\">Return to Home Page</a>");
}

void on_http_update(AsyncWebServerRequest *request)
{
    String output_message;
    String state_message;
    // GET input1 value on <ESP_IP>/update?output=<output_message>&state=<state_message>
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

    Serial.print("GPIO: ");
    Serial.print(output_message);
    Serial.print(" -> ");
    Serial.println(state_message);
    request->send(200, "text/plain", "OK");
}

int web_server_setup()
{
    server.on("/", HTTP_GET, on_http_root);
    server.on("/get", HTTP_GET, on_http_get);
    server.on("/update", HTTP_GET, on_http_update);
    server.onNotFound(not_http_found);
    server.begin();

    Serial.print("Web server is setup and reachable via ipv4: ");
    Serial.println(WiFi.localIP());
    return 0;
}
