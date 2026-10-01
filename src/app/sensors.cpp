#include "sensors.h"
#include <Arduino.h>
#include <Wire.h>
#include <Adafruit_AHTX0.h>
#include <ScioSense_ENS16x.h>

// I2C pins
// ----------------------------
#define I2C_SDA 22
#define I2C_SCL 27
#define I2C_ADDRESS 0x53  // ENS160-sensor I2C address

Adafruit_AHTX0 aht;
ENS160 ens160;

extern float temp_offs;   //individual offset values for temperature(+-°C) and humidity(+-%rH)
extern float humi_offs;   //to calibrate the AHT20 sensor readings to match a known accurate reference
uint16_t t_comp, h_comp;  // NOTE: the ENS160 gas sensor compensation receives unbiased data from sensor
                          // as this represents the temp and humidity which ENS160 is exposed to
extern float temp_sensor;
extern float humi_sensor;
extern int32_t aqi_sensor;
extern int32_t eco2_sensor;
extern int32_t tvoc_sensor;

extern float Aco2;  // Alpha CO2 for EMA (Exponential Moving Average)
extern float Avoc;  // Alpha TVOC for EMA (Exponential Moving Average)

sensors_event_t humidity, temp;

// ================== Setup I2C and init sensors ===================
void SetupSensors() {
Wire.begin(I2C_SDA, I2C_SCL, 100000);
aht.begin();
delay(100);
ens160.begin(&Wire, I2C_ADDRESS);
delay(1000);
ens160.startStandardMeasure();

//Aco2 = 0.2f;     // set fixed smoothing factor for EMA (Exponential Moving Average)
//Avoc = 0.2f;     // set fixed smoothing factor for EMA (Exponential Moving Average)
eco2_sensor = 0;  // initialize EMA for eCO2
tvoc_sensor = 0; // initialize EMA for TVOC
}
// ======== Get Current Sensor Data ==========
void GetSensorData() {
  aht.getEvent(&humidity,&temp);
  temp_sensor = temp_offs+round(temp.temperature*10)/10.0;  // apply offset and round to 1 decimal place for temperature; whole number for humidity
  humi_sensor = humi_offs+round(humidity.relative_humidity);

  // convert the temperature and humidity values for the ENS160 gas sensor compensation algorithm
  t_comp = (uint16_t)((temp.temperature + 273.15f) * 64.0f);
  h_comp = (uint16_t)(humidity.relative_humidity * 512.0f);
  ens160.writeCompensation(t_comp, h_comp);
  if (ens160.update() == RESULT_OK) {
    if( ens160.hasNewData() ) {
      aqi_sensor = ens160.getAirQualityIndex_UBA();
      eco2_sensor = Aco2 * ens160.getEco2() + (1 - Aco2) * eco2_sensor;
      tvoc_sensor = Avoc * ens160.getTvoc() + (1 - Avoc) * tvoc_sensor;
      }
  }
}
  