#include <ThingsBoard.h>

#define MIN_TEMPERATURE_DIFFERENCE 0.1
#define MIN_HUMIDITY_DIFFERENCE 1

void try_send_temperature(ThingsBoard *tb, float temperature);
void try_send_humidity(ThingsBoard *tb, float humidity);
void try_send_window_status(ThingsBoard *tb, bool window_status);