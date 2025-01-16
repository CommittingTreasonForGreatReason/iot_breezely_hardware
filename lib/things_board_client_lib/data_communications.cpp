#include "data_communications.hpp"
#include "user_config.hpp"
#include "logger.hpp"

float last_sent_temperature = -1000;
float last_sent_humidity = -1000;
float last_sent_window_state = false;

bool try_send_temperature(ThingsBoard *tb, float temperature)
{
    if (MIN_TEMPERATURE_DIFFERENCE > abs(temperature - last_sent_temperature))
    {
        serial_logger_print("temperature did not change enough-> did not send telemetry!", LOG_LEVEL_DEBUG);
        return true; // ignore fluctuation
    }
    bool success = tb->sendTelemetryData(TELEMETRY_NAME_TEMPERATURE, temperature);
    serial_logger_print_telemetry_float(TELEMETRY_NAME_TEMPERATURE, temperature);
    if (!success)
    {
        serial_logger_print("Failed to send!", LOG_LEVEL_DEBUG);
        return false;
    }
    last_sent_temperature = temperature;
    return true;
}

bool try_send_humidity(ThingsBoard *tb, float humidity)
{
    if (MIN_TEMPERATURE_DIFFERENCE > abs(humidity - last_sent_humidity))
    {
        serial_logger_print("humidity did not change enough-> did not send telemetry!", LOG_LEVEL_DEBUG);
        return true; // ignore fluctuation
    }
    bool success = tb->sendTelemetryData(TELEMETRY_NAME_HUMIDITY, humidity);
    serial_logger_print_telemetry_float(TELEMETRY_NAME_HUMIDITY, humidity);
    if (!success)
    {
        serial_logger_print("Failed to send!", LOG_LEVEL_DEBUG);
        return false;
    }
    last_sent_humidity = humidity;
    return true;
}

bool try_send_window_status(ThingsBoard *tb, bool window_status)
{
    if (window_status == last_sent_window_state)
    {
        serial_logger_print("window_status did not change -> did not send telemetry!", LOG_LEVEL_DEBUG);
        return true; // window status did not change
    }
    bool success = tb->sendTelemetryData(TELEMETRY_NAME_WINDOW_STATUS, window_status);
    serial_logger_print_telemetry_float(TELEMETRY_NAME_WINDOW_STATUS, window_status);
    if (!success)
    {
        serial_logger_print("Failed to send!", LOG_LEVEL_DEBUG);
        return false;
    }
    last_sent_window_state = window_status;
    return true;
}
