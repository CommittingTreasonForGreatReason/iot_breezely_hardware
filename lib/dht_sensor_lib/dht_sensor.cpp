#include <DHT.h>

#include "dht_sensor.hpp"
#include "user_config.hpp"

static DHT dht(DHT_DATA_PIN, DHTTYPE);

// always store last valid measurements to ensure consistency 
static float lastValidTemperature = 20.0f;
static float lastValidHumidity = 50.0f;

int dht_sensor_setup()
{
    dht.begin();
    return 0;
}

float dht_sensor_get_temperature()
{
    // obtain latest measurement
    float value = dht.readTemperature();

    // validate measurement value
    if(isnan(value) || value < -15.0f || value > 50.0f) {
        return lastValidTemperature;
    }

    // round to two decimals and store as last valid value
    value = ((int)round(value * 100.0f)) / 100.0f;
    lastValidTemperature = value;

    return value;
}

float dht_sensor_get_humidity()
{
    // obtain latest measurement
    float value = dht.readHumidity();

    // validate measurement value
    if(isnan(value) || value < 1.0f || value > 99.0f) {
        return lastValidHumidity;
    }

    // round to two decimals and store as last valid value
    value = ((int)round(value * 100.0f)) / 100.0f;
    lastValidHumidity = value;

    return value;
}

void print_temperature(float temperature)
{
    Serial.print("Temperature: ");
    Serial.print(temperature);
    Serial.println(" C");
}

void print_humidity(float humidity)
{
    Serial.print("Humidity: ");
    Serial.print(humidity);
    Serial.println(" %");
}