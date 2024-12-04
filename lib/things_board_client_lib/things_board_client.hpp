#pragma once

// Things board related env parameters
#define THINGSBOARD_SERVER "server-malte.duckdns.org"
#define THINGSBOARD_PORT 5869 // 1883U

// name definitions of the measurement datapoints 
#define TELEMETRY_NAME_TEMPERATURE "temperature"
#define TELEMETRY_NAME_HUMIDITY "humidity"
#define TELEMETRY_NAME_WINDOW_STATUS "window_status"

bool get_things_board_connected();
void things_board_client_routine(float temperature, float humidity, bool window_status);
int things_board_client_setup(char const *device_token);
int things_board_client_teardown();