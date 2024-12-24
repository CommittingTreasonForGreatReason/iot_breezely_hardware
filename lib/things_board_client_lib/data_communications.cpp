#include "data_communications.hpp"
#include "user_config.hpp"
#include "logger.hpp"

float last_sent_temperature = -1000;
float last_sent_humidity = -1000;
float last_sent_window_state = false;

void try_send_temperature(ThingsBoard *tb, float temperature)
{
    if (MIN_TEMPERATURE_DIFFERENCE > abs(temperature - last_sent_temperature))
    {
        return; // ignore fluctuation
    }
    tb->sendTelemetryData(TELEMETRY_NAME_TEMPERATURE, temperature);
    serial_logger_print_telemetry_float(TELEMETRY_NAME_TEMPERATURE, temperature);
    last_sent_temperature = temperature;
}

void try_send_humidity(ThingsBoard *tb, float humidity)
{
    if (MIN_TEMPERATURE_DIFFERENCE > abs(humidity - last_sent_humidity))
    {
        return; // ignore fluctuation
    }
    tb->sendTelemetryData(TELEMETRY_NAME_HUMIDITY, humidity);
    serial_logger_print_telemetry_float(TELEMETRY_NAME_HUMIDITY, humidity);
    last_sent_humidity = humidity;
}

void try_send_window_status(ThingsBoard *tb, bool window_status)
{
    if (window_status == last_sent_window_state)
    {
        return; // window status did not change
    }
    tb->sendTelemetryData(TELEMETRY_NAME_WINDOW_STATUS, window_status);
    serial_logger_print_telemetry_float(TELEMETRY_NAME_WINDOW_STATUS, window_status);
    last_sent_window_state = window_status;
}
