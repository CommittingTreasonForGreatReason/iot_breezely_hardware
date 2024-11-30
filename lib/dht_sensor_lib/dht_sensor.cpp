#include "dht_sensor.hpp"
#include <DHT.h>

static DHT dht(DHT_DATA_PIN, DHT11);

int dht_sensor_setup()
{
    dht.begin();
    return 0;
}

float dht_sensor_get_temperature()
{
    return dht.readTemperature();
}

float dht_sensor_get_humidity()
{
    return dht.readHumidity();
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
