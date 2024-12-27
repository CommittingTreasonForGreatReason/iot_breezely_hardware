#pragma once

/*
    module for managing global config parameters
    stored in non-volatile flash memory
*/

#define CONF_FILE_PATH "config.json"

static bool config_loaded_from_flash = false;

bool load_config_from_flash();

// getters 


// setters
