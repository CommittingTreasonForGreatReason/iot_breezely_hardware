#pragma once

bool get_things_board_connected();
void things_board_client_routine(float temperature, float humidity, bool window_status);
int things_board_client_setup(char const *device_token);
int things_board_client_setup_provisioning(char const *device_name);
int things_board_client_teardown();

String get_access_token();
void set_access_token(String token);