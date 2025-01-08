#pragma once

/*
    module for managing global config parameters
    stored in non-volatile flash memory
*/
#include "user_config.hpp"

#define DEVICE_NAME_SIZE_MIN 6
#define DEVICE_NAME_SIZE_MAX 32

#define DEVICE_NAME_EXTENSION_SIZE 8

#define WIFI_SIZE_MIN 4
#define WIFI_SIZE_MAX 32

#define TOKEN_SIZE_MIN 4
#define TOKEN_SIZE_MAX 32

#define CONF_FILE_PATH "config.json"

static bool config_loaded_from_flash = false;

bool load_config_from_flash();
bool store_config_to_flash();

// getters
char *try_get_stored_device_name();
char *try_get_stored_device_name_extension();
char *try_get_stored_wifi_ssid();
char *try_get_stored_wifi_pwd();
char *try_get_stored_token();

// setters
bool try_set_stored_device_name(const char *new_device_name);
bool try_set_stored_device_name_extension(const char *new_device_name_extension);
bool try_set_stored_wifi_ssid(const char *new_wifi_ssid);
bool try_set_stored_wifi_pwd(const char *new_wifi_pwd);
bool try_set_stored_token(const char *new_token);
