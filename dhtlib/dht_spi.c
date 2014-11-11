/************************************************************************

  This file is part of the libdht "DHT Temperature & Humidity Sensor"
  library.
  
  This is the implementation of the DHT sensor reading functions using
  SPI as communication bus. It depends on the SPI kernel driver which 
  needs to be installed and loaded on the target system.  

  Author: Ondrej Wisniewski
  
  Features:
  - Support for DHT11 and DHT22/AM2302/RHT03
  - Auto detect sensor model

  Based on the work from Daniel Perron posted on the RaspberryPi Forum:
  http://www.raspberrypi.org/forums/viewtopic.php?p=506650#p506650
  
  Changelog:
   11-11-2014: Initial version (porting and integrating Daniels code)

************************************************************************/

/************************************************************************

  Sensor cabling:
  
          HOST                                    SENSOR
  
          3.3V o---------------------+----------o VCC
                                     |
                                    |X| R1
                                     |
      SPI MISO o-----+---------------+----------o DATA 
                     |
                     V D1
                     |
      SPI MOSI o-----+
  
           GND o--------------------------------o GND

************************************************************************/


#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <unistd.h>
#include <fcntl.h>
#include <malloc.h>
#include <string.h>
#include <errno.h>
#include <sys/ioctl.h>
#include <linux/types.h>
#include <linux/spi/spidev.h>

#include "dht.h"

#define RSP_DATA_SIZE 5
#define MAX_PULSE_LENGTH_ZERO 40 // 26-28us
#define MAX_PULSE_LENGTH_ONE  80 // 70us
#define MIN_BIT_LENGTH 5
#define MAX_BIT_LENGTH MAX_PULSE_LENGTH_ONE

/* Imported variables */
extern float temperature;
extern float humidity;
extern DHT_MODEL_t sensor_model;
extern DHT_ERROR_t error_code;
extern uint32_t last_read_time;

/* SPI protocol settings */
static const char *device = "/dev/spidev0.0";
static uint8_t  mode  = SPI_MODE_0;
static uint8_t  bits  = 8;
static uint32_t speed = 500000;
static uint16_t delay = 0;

static int fd=0;


/*********************************************************************
 * INTERNAL FUNCTIONS
 ********************************************************************/

/*********************************************************************
 * Function:    spi_data_transfer()
 * 
 * Description: Passes the data buffer to the SPI device and receives
 *              the response. 
 * 
 * Parameters:  int fd            - file handle for SPI device
 *              uint8_t *data_buf - pointer to data buffer (tx/rx)
 *              int len           - data buffer length
 * 
 ********************************************************************/
static int spi_data_transfer(int fd, uint8_t *data_buf, int len)
{
   int ret;

   struct spi_ioc_transfer tr = {
      .tx_buf = (unsigned long)data_buf,
      .rx_buf = (unsigned long)data_buf,
      .len = len,
      .delay_usecs = delay,
      .speed_hz = speed,
      .bits_per_word = bits,
   };

   ret = ioctl(fd, SPI_IOC_MESSAGE(1), &tr);
   
   return ret;
}


/*********************************************************************
 * Function:    get_bit()
 * 
 * Description: Get the value of a bit in the data buffer
 * 
 * Parameters:  uint8_t *data_buf - pointer to data buffer 
 *              int bit_idx       - bit index in buffer
 * 
 ********************************************************************/
static int get_bit(uint8_t* data_buf, int bit_idx)
{
   if ((data_buf[bit_idx/8] & 0x80>>(bit_idx%8)) == 0)
     return 0;
   else
     return 1;
}


/*********************************************************************
 * Function:    get_pulse_length()
 * 
 * Description: Detects an edge in the bit stream of continuous periods
 *              of 1s or 0s contained in the data buffer and calculates 
 *              the pulse length from the start position to the edge.
 *              The pulse length and the bit position of the detected 
 *              edge is returned.
 * 
 * Parameters:  uint8_t *data_buf - pointer to data buffer 
 *              int *bit_idx      - pointer to bit index in buffer
 *              int max_bit       - max bit index
 * 
 ********************************************************************/
static int get_pulse_length(uint8_t* data_buf, int* bit_idx, int max_bit)
{
   int i;
   int last_bit;
   int bit_delta;
   int start_bit_idx=*bit_idx;

   if (start_bit_idx >= max_bit) return 0;

   /* Search for first changed bit in data buffer */
   last_bit = get_bit(data_buf, start_bit_idx);
   for (i=start_bit_idx; i<max_bit; i++)
   {
      if(get_bit(data_buf, i) != last_bit)
      { 
         bit_delta = i-start_bit_idx;
         
         /* Return current bit index and pulse length in usec */
         *bit_idx = i;
         return ((int)(1000000.0 * bit_delta / speed));
      }
   }
   
   /* No bit change detected */
   return 0;
}


/*********************************************************************
 * Function:    decode_data()
 * 
 * Description: Decodes the actual sensor data contained in the bit 
 *              stream in the data buffer.              
 * 
 * Parameters:  uint8_t *data_in  - pointer to input data buffer 
 *                                  (bit stream)
 *              uint8_t *data_out - pointer to output data buffer 
 *                                  (decoded sensor data)
 *              int max_bit       - max bit index for bit stream
 *  
 ********************************************************************/
static int decode_data(uint8_t* data_in, uint8_t* data_out, int max_bit)
{
   int byte_idx, bit_idx;
   int bit_num=0;
   uint32_t pulse_len;

   
   /* Skip host request sequence (low part) */
   pulse_len = get_pulse_length(data_in, &bit_num, max_bit);
  
   /* Skip host request sequence (high part) */
   pulse_len = get_pulse_length(data_in, &bit_num, max_bit);

   /* Skip sensor init response (low part) */
   pulse_len = get_pulse_length(data_in, &bit_num, max_bit);

   /* Skip sensor init response (high part) */
   pulse_len = get_pulse_length(data_in, &bit_num, max_bit);

   /* 
    * Now the actual data bits follow
    * Start bit detection (data decoding)
    */
   for (byte_idx=0; byte_idx<RSP_DATA_SIZE; byte_idx++)
   {
      data_out[byte_idx]=0;
      for(bit_idx=0; bit_idx<8; bit_idx++)
      {
         /* Skip low level (start tx) */
         pulse_len = get_pulse_length(data_in, &bit_num, max_bit);
         
         /* Measure high level duration */
         pulse_len = get_pulse_length(data_in, &bit_num, max_bit);
         
         /* Check for invalid pulses */
         if ((pulse_len < MIN_BIT_LENGTH) || (pulse_len > MAX_BIT_LENGTH))
            return 1;
         
         /* Detect bit value according to the pulse length */
         if (pulse_len > MAX_PULSE_LENGTH_ZERO)
            data_out[byte_idx] |= 0x80>>(bit_idx);
      }
   }
   return 0;
}


/*********************************************************************
 * PUBLIC FUNCTIONS
 ********************************************************************/

/*********************************************************************
 * Function: dhtSetup_spi()
 * 
 * Description: Setup of globally used resources
 * 
 * Parameters: pin - GPIO Kernel Id of used IO pin
 *             model - sensors model
 * 
 ********************************************************************/
void dhtSetup_spi(DHT_MODEL_t model)
{
   int ret = 0;
   
   /* Open SPI device */
   fd = open(device, O_RDWR);
   if (fd < 0)
   {
      fprintf(stderr, "ERROR: Can't open device %s: %s\n",
                       device, strerror(errno));
      error_code = ERROR_OTHER;
      return;
   }
   
   /* Set SPI mode */
   ret = ioctl(fd, SPI_IOC_WR_MODE, &mode);
   if (ret == -1)
   {
      fprintf(stderr, "ERROR: Can't set spi mode (%02X): %s\n",
                       mode, strerror(errno));
      error_code = ERROR_OTHER;
      return;
   }
      
   /* Set bits per word */
   ret = ioctl(fd, SPI_IOC_WR_BITS_PER_WORD, &bits);
   if (ret == -1)
   {
      fprintf(stderr, "ERROR: Can't set bits per word (%d): %s\n",
                       bits, strerror(errno));
      error_code = ERROR_OTHER;
      return;
   }
   
   /* Set max speed in Hz */
   ret = ioctl(fd, SPI_IOC_WR_MAX_SPEED_HZ, &speed);
   if (ret == -1)
   {
      fprintf(stderr, "ERROR: Can't set max speed (%d Hz): %s\n",
                       speed, strerror(errno));
      error_code = ERROR_OTHER;
      return;
   }
   
   error_code = ERROR_NONE;  
}

/*********************************************************************
 * Function: dhtCleanup_spi()
 * 
 * Description: Setup of globally used resources
 * 
 * Parameters: None
 * 
 ********************************************************************/
void dhtCleanup_spi(DHT_MODEL_t model)
{
   if (fd) close(fd);
   error_code = ERROR_NONE;  
}

/*********************************************************************
 * Function:    readSensor_spi()
 * 
 * Description: handles the communication with the sensor and reads
 *              the current sensor data via SPI interface
 * 
 * Parameters:  none
 * 
 * Return:      sets the following global variables:
 *              - error_code
 *              - temperature
 *              - humidity
 ********************************************************************/
void readSensor_spi(int argc, char *argv[])
{
   uint8_t sensor_data[RSP_DATA_SIZE];
   uint8_t checksum=0;
   int i;
   int ret;
      
   
   if (sensor_model==AUTO_DETECT)
   { /* Do something */ }
      
   if (last_read_time == 0)
   { /* Do something */ }
   
   temperature = 0;
   humidity = 0;
   
   /* 
    * The whole communication process with the sensor should not 
    * exceed 6ms (init sequence + 5ms of data response)
    * To be on the save side we create a byte array for 8ms of data 
    */
   int num_bits  =  (int)(0.008 * speed);
   int num_bytes =  num_bits / bits;
   uint8_t *spi_data = (uint8_t *)malloc(num_bytes);

   /* 
    * Define data request to DHT22:
    *   - start for 1.5ms with 0 (min 1ms),
    *   - then switch to 1 to wait for the response
    */
   int start_offset = (int)(0.0015 * speed) / bits;
   memset(spi_data, 0, start_offset);
   memset(&spi_data[start_offset], 0xff, num_bytes-start_offset);

   /* Perform the data transfer */
   spi_data_transfer(fd, spi_data, num_bytes);
        
   /* Decode the sensor response */
   ret = decode_data(spi_data, sensor_data, num_bits);
   free(spi_data);
   
   /* Check decoding result */
   if (ret == 1)
   {
      error_code = ERROR_TIMEOUT;
      return;
   }
   
   /* Checksum validation */
   for (i=0; i<RSP_DATA_SIZE-1; i++)
      checksum += sensor_data[i];
   if (checksum != sensor_data[4])
   {
      error_code = ERROR_CHECKSUM;
      return;
   }
   
   /* Calculate temperature and humidity values from raw data */
   humidity = ((uint16_t)sensor_data[0]<<8 | sensor_data[1])/10.0;
   temperature = ((uint16_t)(sensor_data[2] & 0x7f)<<8 | sensor_data[3])/10.0;
   if((sensor_data[2] & 0x80)== 0x80)
      temperature = -(temperature);

   error_code = ERROR_NONE;
}
