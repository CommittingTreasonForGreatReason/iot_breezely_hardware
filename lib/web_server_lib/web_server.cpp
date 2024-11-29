#include <WiFi.h>
#include <Arduino.h>
#include <AsyncTCP.h>
#include <ESPAsyncWebServer.h>
#include <SPIFFS.h>

#include "web_server.hpp"
#include "dht_sensor.hpp"

// define http server instance on default port 80
AsyncWebServer server(80);

// html template reused with placeholders
const char index_html[] PROGMEM = R"rawliteral(
<!DOCTYPE HTML>
<html>
<head>
  <title>ESP Web Server</title>
  <meta name="viewport" content="width=device-width, initial-scale=1">
  <link rel="icon" href="data:,">
</head>
<body>
  <form action="/login">
    username: <input type="text" name="username">
    password: <input type="text" name="password">
    <input type="submit" value="Submit">
  </form><br>
  %BUTTONPLACEHOLDER%
  <p>
    <i class="fas fa-thermometer-half" style="color:#059e8a;"></i> 
    <span class="dht-labels">Temperature</span> 
    <span id="temperature">%TEMPERATURE%</span>
    <sup class="units">&deg;C</sup>
  </p>
  <p>
    <i class="fas fa-tint" style="color:#00add6;"></i> 
    <span class="dht-labels">Humidity</span>
    <span id="humidity">%HUMIDITY%</span>
    <sup class="units">&percnt;</sup>
  </p>

  <script>
  function toggleCheckbox(element) {
    var xhr = new XMLHttpRequest();
    if(element.checked){ xhr.open("GET", "/gpio_update?output="+element.id+"&state=1", true); }
    else { xhr.open("GET", "/gpio_update?output="+element.id+"&state=0", true); }
    xhr.send();
    }

  setInterval(function ( ) {
    var xhttp = new XMLHttpRequest();
    xhttp.onreadystatechange = function() {
        if (this.readyState == 4 && this.status == 200) {
        document.getElementById("temperature").innerHTML = this.responseText;
        }
    };
    xhttp.open("GET", "/temperature", true);
    xhttp.send();
    }, 10000 ) ;

  setInterval(function ( ) {
    var xhttp = new XMLHttpRequest();
    xhttp.onreadystatechange = function() {
        if (this.readyState == 4 && this.status == 200) {
        document.getElementById("humidity").innerHTML = this.responseText;
        }
    };
    xhttp.open("GET", "/humidity", true);
    xhttp.send();
    }, 10000 ) ;
   </script>
</body></html>)rawliteral";

String outputState(int output)
{
    return digitalRead(output) ? "checked" : "";
}

// Replaces placeholder
String processor(const String &var)
{
    if (var == "BUTTONPLACEHOLDER")
    {
        String buttons = "";
        buttons += "Output - GPIO " + String(HTTP_OUTPUT_PIN) + "<label class=\"switch\"><input type=\"checkbox\" onchange=\"toggleCheckbox(this)\" id=\"" + String(HTTP_OUTPUT_PIN) + "\" " + outputState(HTTP_OUTPUT_PIN) + "><span class=\"slider\"></span></label>";
        return buttons;
    }
    else if (var == "TEMPERATURE")
    {
        return String(dht_sensor_get_temperature());
    }
    else if (var == "HUMIDITY")
    {
        return String(dht_sensor_get_humidity());
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

void on_http_gpio_update(AsyncWebServerRequest *request)
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

    Serial.print("GPIO: ");
    Serial.print(output_message);
    Serial.print(" -> ");
    Serial.println(state_message);
    request->send(200, "text/plain", "OK");
}

void on_http_temperature(AsyncWebServerRequest *request)
{
    Serial.println("Temperature was just requested from client");
    float temperature = dht_sensor_get_temperature();
    // print_temperature(temperature);
    request->send_P(200, "text/plain", String(temperature).c_str());
}

void on_http_humidity(AsyncWebServerRequest *request)
{
    Serial.println("Humidity was just requested from client");
    float humidity = dht_sensor_get_humidity();
    // print_humidity(humidity);
    request->send_P(200, "text/plain", String(humidity).c_str());
}

// List of GPIO pins to monitor
const int gpioPins[] = { 18 }; 
const int numPins = sizeof(gpioPins) / sizeof(gpioPins[0]);
String html = "<html><body><h1>ESP32 GPIO States</h1><table border='1'><tr><th>Pin</th><th>State</th></tr>";

void on_http_gpio_read_all(AsyncWebServerRequest *request) 
{
    
    for (int i = 0; i < numPins; i++) {
      int state = digitalRead(gpioPins[i]);
      html += "<tr><td>GPIO " + String(gpioPins[i]) + "</td><td>" + String(state) + "</td></tr>";
    }
    html += "</table></body></html>";
    request->send(200, "text/html", html);
}

int web_server_setup()
{
    // serve static index.html stored in SPIFFS
    server.serveStatic("/", SPIFFS, "/").setDefaultFile("index.html");

    // define the async callback functions for different routes (= http paths)
    // server.on("/", HTTP_GET, on_http_root);
    server.on("/login", HTTP_GET, on_http_login);
    server.on("/gpio_update", HTTP_GET, on_http_gpio_update);
    server.on("/temperature", HTTP_GET, on_http_temperature);
    server.on("/humidity", HTTP_GET, on_http_humidity);
    server.on("/gpio_read_all", HTTP_GET, on_http_gpio_read_all);

    server.onNotFound(not_http_found);
    server.begin();

    Serial.print("Web server is setup and reachable via ipv4: ");
    Serial.println(WiFi.localIP());
    return 0;
}
