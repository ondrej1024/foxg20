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

  Build command:
  gcc -o dht -lrt dht.c 
  
  Changelog:
   18-10-2013: Initial version (porting from arduino-DHT)

 ******************************************************************
   
  This program is free software; you can redistribute it and/or
  modify it under the terms of the GNU General Public License
  as published by the Free Software Foundation; either version 2
  of the License, or (at your option) any later version.

  This program is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
  GNU General Public License for more details.

  You should have received a copy of the GNU General Public License
  along with this program; if not, write to the Free Software Foundation,
  Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.
   
 ******************************************************************/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <errno.h>
#include <fcntl.h>
#include <time.h>
#include <poll.h>

#include "dht.h"

#define DEBUG 0

#define EXPORT_FILE    "/sys/class/gpio/export"
#define UNEXPORT_FILE  "/sys/class/gpio/unexport"
#define GPIO_BASE_FILE "/sys/class/gpio/gpio"

// timing parameters for serial bit detection
// (numbers are in microseconds)
#define MAX_PULSE_LENGTH_ZERO 50 // 26-28us
#define MAX_PULSE_LENGTH_ONE 120 // 70us
#define MAX_BIT_LENGTH MAX_PULSE_LENGTH_ONE
#define MAX_RESPONSE_BITS 40     // 5 bytes
#define MAX_RESPONSE_EDGES MAX_RESPONSE_BITS*2
#define INIT_DELAY 500000
#define DHT11_START_DELAY 20*1000  // min 18ms
#define DHT22_START_DELAY 1000     // min 800us

static int value_fd;
static int direction_fd;


/*********************************************************************
 * INTERNAL FUNCTIONS
 ********************************************************************/

/*********************************************************************
 * Function:    pinMode()
 * 
 * Description: Set the direction mode for the data pin
 * 
 * Parameters:  iomode - direction (IN|OUT)
 * 
 ********************************************************************/
static void pinMode(DHT_IOMODE_t iomode)
{
  int fd=direction_fd;
  int res;
    
  if (iomode == INPUT)
    res = pwrite(fd, "in", 3, 0);
  else
    res = pwrite(fd, "out", 4, 0);
  
  if (res < 0) {
    fprintf(stderr, "Unable to pwrite to gpio direction for pin %d: %s\n",
            data_pin, strerror(errno));
  }
}

/*********************************************************************
 * Function:    digitalWrite()
 * 
 * Description: Write to the data pin
 * 
 * Parameters:  value - outpur value (HIGF|LOW)
 * 
 ********************************************************************/
static void digitalWrite(PIN_STATE_t value)
{
  int fd=value_fd;
  char d[1];
  
  d[0] = (value == LOW ? '0' : '1');
  if (pwrite(fd, d, 1, 0) != 1) {
    fprintf(stderr, "Unable to pwrite %c to gpio value: %s\n",
                     d[0], strerror(errno));
  }
}

/*********************************************************************
 * Function:    digitalRead()
 * 
 * Description: Read from the data pin
 * 
 * Parameters:  none
 * 
 ********************************************************************/
static PIN_STATE_t digitalRead(void)
{
  int fd=value_fd;
  char d[1];

  if (pread(fd, d, 1, 0) != 1) {
    fprintf(stderr, "Unable to pread gpio value: %s\n",
                     strerror(errno));
  } 
  
  return (d[0] == '0' ? LOW : HIGH);
}

/*********************************************************************
 * Function:    micros()
 * 
 * Description: Reads the current time
 * 
 * Parameters:  none
 * 
 * Return:      The microsseconds part of the current time 
 * 
 ********************************************************************/
static long micros(void)
{
  static long first = -1;
  long nsec;
  struct timespec now_ts;
  

  /* clock_gettime() needs '-lrt' on the link line */
  if (clock_gettime(CLOCK_REALTIME, &now_ts) < 0) {
      fprintf(stderr, "clock_gettime(CLOCK_REALTIME) failed: %s\n",
              strerror(errno));
      return 0;
  }
  
  // store first reading
  if (first == -1) first=now_ts.tv_nsec;
  
  // check for zero crossing of nano seconds
  if (now_ts.tv_nsec < first) 
    nsec=now_ts.tv_nsec+1000000;
  else
    nsec=now_ts.tv_nsec;
  
  // convert to micro seconds
  return nsec/1000;
}

/*********************************************************************
 * PUBLIC FUNCTIONS
 ********************************************************************/

/*********************************************************************
 * Function: dhtSetup()
 * 
 * Description: Setup of globally used resources
 * 
 * Parameters: pin - GPIO Kernel Id of used IO pin
 *             model - sensors model
 * 
 ********************************************************************/
void dhtSetup(uint8_t pin, DHT_MODEL_t model)
{
  int fd;
  char b[64];

  // store globals
  data_pin = pin;
  sensor_model = model;
  resetTimer(); // Make sure we do read the sensor in the next readSensor()

  
  // Prepare GPIO pin connected to sensors data pin to be used with GPIO sysfs
  // (export to user space)
  fd = open(EXPORT_FILE, O_WRONLY);
  if (fd < 0) {
    perror(EXPORT_FILE);
    error_code = ERROR_OTHER;
    return;
  }  
  snprintf(b, sizeof(b), "%d", pin);
  if (pwrite(fd, b, strlen(b), 0) < 0) {
    fprintf(stderr, "Unable to export pin=%d (already in use?): %s\n",
            pin, strerror(errno));
    error_code = ERROR_OTHER;
    return;
  }  
  close(fd);

  // Define edge interrupt (both edges)  
  // to be used later with the poll() function for fast edge detection
  snprintf(b, sizeof(b), "%s%d/edge", GPIO_BASE_FILE, pin);
  fd = open(b, O_RDWR);
  if (fd < 0) {
    fprintf(stderr, "Open %s: %s\n", b, strerror(errno));
    error_code = ERROR_OTHER;
    return;
  }
  if (pwrite(fd, "both", 4, 0) < 0) {
    fprintf(stderr, "Unable to write 'both' to edge_fd: %s\n",
            strerror(errno));
    error_code = ERROR_OTHER;
    return;
  }
  close(fd);
  
  // Open gpio direction file for fast reading/writing when requested
  snprintf(b, sizeof(b), "%s%d/direction", GPIO_BASE_FILE, pin);
  fd = open(b, O_RDWR);
  if (fd < 0) {
    fprintf(stderr, "Open %s: %s\n", b, strerror(errno));
    error_code = ERROR_OTHER;
    return;
  }
  direction_fd=fd;
  
  // Open gpio value file for fast reading/writing when requested
  snprintf(b, sizeof(b), "%s%d/value", GPIO_BASE_FILE, pin);
  fd = open(b, O_RDWR);
  if (fd < 0) {
    fprintf(stderr, "Open %s: %s\n", b, strerror(errno));
    error_code = ERROR_OTHER;
    return;
  }
  value_fd=fd;
   
  // sensor model handling
  if ( model == AM2302 || model == RHT03) {
     sensor_model = DHT22;
  }   
  else if ( model == AUTO_DETECT) {
    sensor_model = DHT22;
    readSensor();
    if ( error_code == ERROR_TIMEOUT ) {
      sensor_model = DHT11;
      // Warning: in case we auto detect a DHT11, you should wait at least 1000 msec
      // before your first read request. Otherwise you will get a time out error.
    }
  }
  
  error_code = ERROR_NONE;
}

/*********************************************************************
 * Function:    dhtCleanup()
 * 
 * Description: Cleanup of globally used resources
 * 
 * Parameters:  none
 * 
 ********************************************************************/
void dhtCleanup(void)
{
  int fd;
  char b[8];

  // close gpio value file
  close(value_fd);

  // close gpio direction file
  close(direction_fd);
  
  // free GPIO pin connected to sensors data pin to be used with GPIO sysfs  
  fd = open(UNEXPORT_FILE, O_WRONLY);
  if (fd < 0) {
    perror(UNEXPORT_FILE);
    error_code = ERROR_OTHER;
    return;
  } 
  snprintf(b, sizeof(b), "%d", data_pin);
  if (pwrite(fd, b, strlen(b), 0) < 0) {
    fprintf(stderr, "Unable to unexport pin=%d: %s\n",
            data_pin, strerror(errno));
    error_code = ERROR_OTHER;
    return;
  }  
  close(fd);
  error_code = ERROR_NONE;
}

/*********************************************************************
 * Function:    resetTimer()
 * 
 * Description: 
 * 
 * Parameters:  none
 * 
 * Return:
 * 
 ********************************************************************/
void resetTimer()
{
  last_read_time = micros()*1000 - 3000;
}

/*********************************************************************
 * Function:    getHumidity()
 * 
 * Description: get humidity value read with latest readSensor()
 * 
 * Parameters:  none
 * 
 * Return:      relative humidity in %
 * 
 ********************************************************************/
float getHumidity()
{
  return humidity;
}

/*********************************************************************
 * Function:    getTemperature()
 * 
 * Description: get temperature value read with latest readSensor()
 * 
 * Parameters:  none
 * 
 * Return:      temperature in °C
 * 
 ********************************************************************/
float getTemperature()
{
  return temperature;
}

/*********************************************************************
 * Function:    getStatus()
 * 
 * Description: get latest error code
 * 
 * Parameters:  none
 * 
 * Return:      error_code
 * 
 ********************************************************************/
DHT_ERROR_t getStatus() 
{ 
   return error_code; 
}

/*********************************************************************
 * Function:    getStatusString()
 * 
 * Description: get latest error string
 * 
 * Parameters:  none
 * 
 * Return:      error desciption
 * 
 ********************************************************************/
const char* getStatusString()
{
  switch ( error_code ) 
  {
    case ERROR_TIMEOUT:
      return "TIMEOUT";

    case ERROR_CHECKSUM:
      return "CHECKSUM";
      
    case ERROR_OTHER:
      return "OTHER";

    default:
      return "OK";
  }
}

/*********************************************************************
 * Function:    readSensor()
 * 
 * Description: handles the communication with the sensor and reads
 *              the current sensor data
 * 
 * Parameters:  none
 * 
 * Return:      sets the following global variables:
 *              - error_code
 *              - temperature
 *              - humidity
 ********************************************************************/
void readSensor()
{
  long startTime = micros();
  int8_t   i; 
  uint32_t k;
  uint8_t  age;
  uint16_t rawHumidity=0;
  uint16_t rawTemperature=0;
  uint16_t data=0;
  long t1, t2, t3, t4; // debug info

#if 0
  // Make sure we don't poll the sensor too often
  // - Max sample rate DHT11 is 1 Hz   (duty cicle 1000 ms)
  // - Max sample rate DHT22 is 0.5 Hz (duty cicle 2000 ms)
  unsigned long startTime = micros();
  if ( (unsigned long)(startTime - last_read_time) < (model == DHT11 ? 999L : 1999L) ) {
    return;
  }
  last_read_time = startTime;
#endif

  temperature = 0;
  humidity = 0;
 
  // Request sample
  pinMode(OUTPUT);  
  digitalWrite(HIGH); // Init
  usleep(INIT_DELAY);
  
  digitalWrite(LOW); // Send start signal
  t1 = micros(); 
  if ( sensor_model == DHT11 ) {
    usleep(DHT11_START_DELAY);
  }
  else {
    // This will fail for a DHT11 - that's how we can detect such a device
    usleep(DHT22_START_DELAY);
  }
  
  digitalWrite(HIGH); // Switch bus to receive data
  t2 = micros(); 
  pinMode(INPUT);
  t3 = micros(); 

  // We're going to read 83 edges:
  // - First a FALLING, RISING, and FALLING edge for the start bit
  // - Then 40 bits: RISING and then a FALLING edge per bit
  // To keep our code simple, we accept any HIGH or LOW reading if it's max 85 usecs long
  
  for ( i = -3 ; i < MAX_RESPONSE_EDGES; i++ ) {
    startTime = micros();

    // wait for edge change and measure pulse length
    k=0;
    do {
      k++;
      age = (uint8_t)(micros() - startTime);
      if ( age > MAX_BIT_LENGTH ) {
        // pulse length for single bit has timed out
        t4 = micros(); 
#if DEBUG
        printf("i=%d, k=%lu, age=%u, data_pin=%u, data=0x%08X\n", 
                i, k, age, digitalRead(), data);
        printf("dt2=%ld, dt3=%ld, dt4=%ld\n", t2-t1, t3-t2, t4-t3);
#endif
        error_code = ERROR_TIMEOUT;
        return;
      }
      // sleep 10us
      //usleep(10);
    }
    while ( digitalRead() == (i & 1) ? HIGH : LOW );
    
    if ( i >= 0 && (i & 1) ) {
      // Now we are being fed our 40 bits
      data <<= 1;

      // A zero lasts max 30 usecs, a one at least 68 usecs.
      if ( age > MAX_PULSE_LENGTH_ZERO ) {
        data |= 1; // we got a one
      }
    }

    switch ( i ) {
      case 31:
        rawHumidity = data;
        data = 0;
        break;
      case 63:
        rawTemperature = data;
        data = 0;
        break;
    }
  }
  
  // Verify checksum
  if ( (uint8_t)(((uint8_t)rawHumidity) + (rawHumidity >> 8) + ((uint8_t)rawTemperature) + (rawTemperature >> 8)) != data ) {
#if DEBUG
    printf("data_pin=%d, data=0x%04X%04X%02X\n", 
            digitalRead(), rawHumidity, rawTemperature, data);
#endif
    error_code = ERROR_CHECKSUM;
    return;
  }

  // Convert raw readings and store in global variables
  if ( sensor_model == DHT11 ) {
    humidity = rawHumidity >> 8;
    temperature = rawTemperature >> 8;
  }
  else {
    humidity = rawHumidity * 0.1;

    if ( rawTemperature & 0x8000 ) {
      rawTemperature = -(int16_t)(rawTemperature & 0x7FFF);
    }
    temperature = ((int16_t)rawTemperature) * 0.1;
  }

  error_code = ERROR_NONE;
}
