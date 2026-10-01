#pragma once

#include <Arduino.h>
#include <TFT_eSPI.h>
#include <time.h>

void drawClockStatic();
void drawClockFace();
void drawDateAndWeek( const struct tm *ti );
void drawNamedayAndHoliday();
void drawDigitalClock( int h, int m, int s );
void updateHands( int h, int m, int s );
void drawWeatherSection();
void drawSensorData( float temp_sensor, float humi_sensor, int32_t aqi_sensor, int32_t eco2_sensor, int32_t tvoc_sensor );
