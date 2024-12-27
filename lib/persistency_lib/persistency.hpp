#pragma once

/*
    module for managing global config parameters
    stored in non-volatile flash memory
*/

#include "user_config.hpp"

#define CONF_FILE_PATH "config.json"

// config parameters default definitions //
char* device_name = DEVICE_NAME_INPUT_NAME;
char* wifi_ssid = WIFI_SSID;
char* wifi_pwd = WIFI_PASSWORD;
char* things_board_access_token = TB_ACCESS_TOKEN;
// ...
// ----------------------------------------

static bool config_loaded_from_flash = false;

bool load_config_from_flash();

// getters 

// setters
