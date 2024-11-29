#include "emul_sensors.h"

static const float temp[] = {
	1.3, 1.5, 1.8, 1.8, 1.6, 1.3, 0.0, 0.3,-0.3, 0.2, 1.6, 2.8, 
    3.6, 5.1, 6.3, 5.9, 5.3, 4.7, 3.8, 3.4, 2.8, 2.4, 2.0, 1.6
};

static const float humidity[]= { 
   84.0, 81.0, 79.0, 79.0, 81.0, 81.0, 85.0, 86.0, 87.0, 85.0, 79.0, 75.0,
   74.0, 72.0, 71.0, 72.0, 72.0, 73.0, 74.0, 75.0, 75.0, 76.0, 78.0, 79.0};

constexpr int sz = sizeof(temp)/sizeof(float);

void get_sensor_value(float &t, float &h)
{
	static int idx=0;
	
	t = temp[idx];
	h = humidity[idx];
	idx++; 
	if (idx==sz) idx=0;
}