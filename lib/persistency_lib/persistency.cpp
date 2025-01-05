#include <Arduino.h>
#include <SPIFFS.h>
#include <ArduinoJson.h>

#include "persistency.hpp"

bool load_config_from_flash()
{
    // open the json file from root directory of the flash memory
    File file = SPIFFS.open("/config.json", "r");
    if (!file)
    {
        Serial.println("Failed to open config file");
        return false;
    }

    // read in it's content and parse it
    String jsonString;
    while (file.available())
    {
        jsonString += char(file.read());
    }
    file.close();

    Serial.println("JSON file content:");
    Serial.println(jsonString);

    // Parse the JSON string
    DynamicJsonDocument doc(1024); // Adjust the size based on your JSON structure
    DeserializationError error = deserializeJson(doc, jsonString);
    if (error)
    {
        Serial.print("Failed to parse JSON: ");
        Serial.println(error.c_str());
        return false;
    }

    // extract all config parameters and store them in RAM
    device_name = doc["decvice_name"];
    wifi_ssid = doc["wifi_ssid"];
    wifi_pwd = doc["wifi_password"];
    things_board_access_token = doc["things_board_access_token"];

    // set flag to indicate that config has been loaded from flash memory
    config_loaded_from_flash = true;
    return true;
}

    // getters

    // setters
