#pragma once

bool get_things_board_connected();
bool things_board_client_routine(float temperature, float humidity, bool window_status);
int things_board_client_setup_provisioning(const char *device_name, const char *device_name_extension, bool force = false);
int things_board_client_teardown();
