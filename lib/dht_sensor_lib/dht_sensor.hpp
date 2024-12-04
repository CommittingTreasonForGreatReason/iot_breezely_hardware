#pragma once

int dht_sensor_setup();
float dht_sensor_get_temperature();
float dht_sensor_get_humidity();
void print_temperature(float temperature);
void print_humidity(float humidity);