/************************************************************************
  DHT Temperature & Humidity Sensor library for use on 
  FoxG20 embedded Linux board (by ACME Systems).

  Author: Ondrej Wisniewski
  
  Features:
  - Support for DHT11 and DHT22/AM2302/RHT03
  - Auto detect sensor model

  Based on code for Arduino from Mark Ruys, mark@paracas.nl
  http://www.github.com/markruys/arduino-DHT

  Datasheets:
  - http://www.micro4you.com/files/sensor/DHT11.pdf
  - http://www.adafruit.com/datasheets/DHT22.pdf
  - http://dlnmh9ip6v2uc.cloudfront.net/datasheets/Sensors/Weather/RHT03.pdf
  - http://meteobox.tk/files/AM2302.pdf

  Changelog:
   18-10-2013: Initial version (porting from arduino-DHT)
   17-03-2014: Added function prototypes for sensor power switching
   
 ******************************************************************/

#ifndef dht_h
#define dht_h

typedef unsigned long  uint32_t;
typedef unsigned short uint16_t;
typedef unsigned char  uint8_t;

typedef enum {
   AUTO_DETECT,
   DHT11,
   DHT22,
   AM2302,  // Packaged DHT22
   RHT03    // Equivalent to DHT22
}
DHT_MODEL_t;

typedef enum {
   ERROR_NONE = 0,
   ERROR_TIMEOUT,
   ERROR_CHECKSUM,
   ERROR_OTHER
}
DHT_ERROR_t;

typedef enum {
   INPUT,
   OUTPUT
}
DHT_IOMODE_t;

typedef enum {
   LOW,
   HIGH
}
PIN_STATE_t;

static float temperature;
static float humidity;

static uint8_t data_pin;
static DHT_MODEL_t sensor_model;
static DHT_ERROR_t error_code;
static uint32_t last_read_time;

void dhtSetup(uint8_t pin, DHT_MODEL_t model);
void dhtCleanup();
void resetTimer();
void dhtPoweron(uint8_t pin);
void dhtPoweroff(uint8_t pin);
void dhtReset(uint8_t pin);

void readSensor();

float getTemperature();
float getHumidity();

DHT_ERROR_t getStatus();
const char* getStatusString();

#if 0
DHT_MODEL_t getModel() { return sensor_model; }

int getMinimumSamplingPeriod() { return sensor_model == DHT11 ? 1000 : 2000; }

int8_t getNumberOfDecimalsTemperature() { return sensor_model == DHT11 ? 0 : 1; };
int8_t getLowerBoundTemperature() { return sensor_model == DHT11 ? 0 : -40; };
int8_t getUpperBoundTemperature() { return sensor_model == DHT11 ? 50 : 125; };

int8_t getNumberOfDecimalsHumidity() { return 0; };
int8_t getLowerBoundHumidity() { return sensor_model == DHT11 ? 20 : 0; };
int8_t getUpperBoundHumidity() { return sensor_model == DHT11 ? 90 : 100; };

static float toFahrenheit(float fromCelcius) { return 1.8 * fromCelcius + 32.0; };
static float toCelsius(float fromFahrenheit) { return (fromFahrenheit - 32.0) / 1.8; };
#endif

#endif /*dht_h*/
