#pragma once

bool get_things_board_connected();
void things_board_client_routine(float temperature, float humidity, bool window_status);
int things_board_client_setup(char const *device_token);
int things_board_client_setup_provisioning(char const *device_name, char const *customer_name, char const *used_token);
int things_board_client_teardown();