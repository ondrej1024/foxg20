/************************************************************************
  This is an example program which uses the DHT Temperature & Humidity 
  Sensor library for use on FoxG20 embedded Linux board (by ACME Systems).

  Author: Ondrej Wisniewski
  
  Build command (make sure to have dhtlib built and installed):
  gcc -o dhtsensor dhtsensor.c -ldht
  
************************************************************************/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include "dht.h"

#define DEFAULT_DATA_PIN_ID 60
#define MAX_RETRIES 3


int main(int argc, char* argv[])
{
   uint8_t data_pin  = DEFAULT_DATA_PIN_ID;
   DHT_MODEL_t model = DHT22;
   int retry = MAX_RETRIES;
 
   
   /* Parse command line */
   switch (argc)
   {   
      case 1:  /* no parameters, use defaults */
      break;
      
      case 3:  /* 2 paramters provided */
         /* Get sensor type */ 
         if (strcmp(argv[1], "DHT11")==0) model = DHT11;

         /* Get Kernel Id of data pin */
         data_pin = atoi(argv[2]);
      break;
      
      default: /* print help message */
         printf("dhtsensor - read temperature and humidity data from DHT11 and DHT22 sensors\n");
         printf(" usage: dhtsensor [<sensor type>] [<data pin>]\n");
         printf("          sensor type: DHT11|DHT22 (default DHT22)\n");
         printf("          data pin: Kernel Id of GPIO data pin (default %u)\n", DEFAULT_DATA_PIN_ID);
         return -1;
   }
   
   /* Init sensor communication */
   dhtSetup(data_pin, model);
   if (getStatus() != ERROR_NONE)
   {
      printf("Error during setup: %s\n", getStatusString());
      return -1;
   }

   /* Read sensor with retry */
   do    
   {
      readSensor();
   
      if (getStatus() == ERROR_NONE)
      {
         printf("Rel. Humidity: %3.1f %%\n", getHumidity());
         printf("Temperature:   %3.1f °C\n", getTemperature());
      }
      else
      {
         usleep(1000000);
      }
   }
   while ((getStatus() != ERROR_NONE) && retry--);
   
   if (getStatus() != ERROR_NONE)
   {
      printf("Error reading sensor: %s\n", getStatusString());
   }
   
   /* Cleanup */
   dhtCleanup();
   
   return 0;
}
