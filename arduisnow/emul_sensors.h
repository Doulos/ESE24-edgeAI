#ifndef EMUL_SENSOR_H
#define EMUL_SENSOR_H

/* emulate sensors (temperature, relative humidity) */
void get_sensor_value(float &t, // OUT: temperature in °C
                      float &h  // OUT: relative humidity %
                     );
#endif
