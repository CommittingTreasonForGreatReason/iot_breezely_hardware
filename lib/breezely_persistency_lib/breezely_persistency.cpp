#include <Arduino.h>
#include <SPIFFS.h>
#include <ArduinoJson.h>
#include "breezely_persistency.hpp"

// config parameters default definitions //
char stored_device_name[DEVICE_NAME_SIZE_MAX + 1] = {0};
char stored_wifi_ssid[WIFI_SIZE_MAX + 1] = {0};
char stored_wifi_pwd[WIFI_SIZE_MAX + 1] = {0};
char stored_token[TOKEN_SIZE_MAX + 1] = {0};
// ...
// ----------------------------------------

void print_config_json_doc(JsonDocument jsonDoc)
{
    String jsonString;
    String temp = jsonDoc["wifi_password"];
    jsonDoc["wifi_password"] = "<REDACTED>"; // before printing the file in console override the password for obvious security reasons :) (since its not encrypted)
    serializeJson(jsonDoc, jsonString);
    Serial.println("loaded JSON file content:");
    Serial.println(jsonString);
    jsonDoc["wifi_password"] = temp; // set it back to original value
}

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

    // Parse the JSON string
    JsonDocument jsonDoc;
    DeserializationError error = deserializeJson(jsonDoc, jsonString);
    if (error)
    {
        Serial.print("Failed to parse JSON: ");
        Serial.println(error.c_str());
        return false;
    }
    // extract all config parameters and store them in RAM
    print_config_json_doc(jsonDoc);

    // extract all config parameters and store them in RAM
    String device_name_str = jsonDoc["device_name"];
    String wifi_ssid_str = jsonDoc["wifi_ssid"];
    String wifi_password_str = jsonDoc["wifi_password"];
    String token_str = jsonDoc["token"];
    memcpy(stored_device_name, device_name_str.c_str(), device_name_str.length());
    memcpy(stored_wifi_ssid, wifi_ssid_str.c_str(), wifi_ssid_str.length());
    memcpy(stored_wifi_pwd, wifi_password_str.c_str(), wifi_password_str.length());
    memcpy(stored_token, token_str.c_str(), token_str.length());

    // set flag to indicate that config has been loaded from flash memory
    config_loaded_from_flash = true;
    return true;
}

bool store_config_to_flash()
{
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

    // Parse the JSON string
    JsonDocument jsonDoc;
    DeserializationError error = deserializeJson(jsonDoc, jsonString);
    if (error)
    {
        Serial.print("Failed to parse JSON: ");
        Serial.println(error.c_str());
        return false;
    }

    // extract all config parameters and store them in RAM
    jsonDoc["device_name"] = stored_device_name;
    jsonDoc["wifi_ssid"] = stored_wifi_ssid;
    jsonDoc["wifi_password"] = stored_wifi_pwd;
    jsonDoc["token"] = stored_token;

    int32_t err = serializeJson(jsonDoc, jsonString);

    file = SPIFFS.open("/config.json", "w");
    if (!file)
    {
        Serial.println("Failed to open config file");
        return false;
    }

    // read in it's content and parse it
    file.print(jsonString);
    file.flush();
    file.close();

    print_config_json_doc(jsonDoc);

    return true;
}

// getters
char *try_get_stored_device_name()
{
    if (strlen(stored_device_name) < DEVICE_NAME_SIZE_MIN || strlen(stored_device_name) > DEVICE_NAME_SIZE_MAX)
        return nullptr;

    return stored_device_name;
}
char *try_get_stored_wifi_ssid()
{
    if (strlen(stored_wifi_ssid) < WIFI_SIZE_MIN || strlen(stored_wifi_ssid) > WIFI_SIZE_MAX)
        return nullptr;

    return stored_wifi_ssid;
}
char *try_get_stored_wifi_pwd()
{
    if (strlen(stored_wifi_pwd) < WIFI_SIZE_MIN || strlen(stored_wifi_pwd) > WIFI_SIZE_MAX)
        return nullptr;

    return stored_wifi_pwd;
}
char *try_get_stored_token()
{
    if (strlen(stored_token) < TOKEN_SIZE_MIN || strlen(stored_token) > TOKEN_SIZE_MAX)
        return nullptr;

    return stored_token;
}
// setters
bool try_set_stored_device_name(const char *new_device_name)
{
    if (strlen(new_device_name) >= DEVICE_NAME_SIZE_MIN) //&& strlen(new_device_name) <= DEVICE_NAME_SIZE_MAX)
    {
        memcpy(stored_device_name, new_device_name, strlen(new_device_name));
        return true;
    }
    return false;
}
bool try_set_stored_wifi_ssid(const char *new_wifi_ssid)
{
    if (strlen(new_wifi_ssid) >= WIFI_SIZE_MIN && strlen(new_wifi_ssid) <= WIFI_SIZE_MAX)
    {
        memcpy(stored_wifi_ssid, new_wifi_ssid, strlen(new_wifi_ssid));
        return true;
    }
    return false;
}
bool try_set_stored_wifi_pwd(const char *new_wifi_pwd)
{
    if (strlen(new_wifi_pwd) >= WIFI_SIZE_MIN && strlen(new_wifi_pwd) <= WIFI_SIZE_MAX)
    {
        memcpy(stored_wifi_pwd, new_wifi_pwd, strlen(new_wifi_pwd));
        return true;
    }
    return false;
}

bool try_set_stored_token(const char *new_token)
{
    if (strlen(new_token) >= TOKEN_SIZE_MIN && strlen(new_token) <= TOKEN_SIZE_MAX)
    {
        memcpy(stored_token, new_token, strlen(new_token));
        return true;
    }
    return false;
}