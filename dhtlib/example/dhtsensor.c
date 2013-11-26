/*
 * build command:
 * gcc -o dhtsensors dhtsensor.c -ldht
 */

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
   if((argc==2) || (argc>3))
   {
      if ((strcmp(argv[1], "-h")==0) || (strcmp(argv[1], "--help")==0))
      {
         printf("dhtsensor - read temperature and humidity data from DHT11 and DHT22 sensors\n");
         printf(" usage: dhtsensor [<sensor type>] [<data pin>]\n");
         printf("          sensor type: DHT11|DHT22 (default DHT22)\n");
         printf("          data pin: Kernel Id of GPIO data pin (default %u)\n", DEFAULT_DATA_PIN_ID);
         return -1;
      }
   }
   else if(argc==3)
   {
      /* Get sensor type */ 
      if (strcmp(argv[1], "DHT11")==0) model = DHT11;

      /* Get Kernel Id of data pin */
      data_pin = atoi(argv[1]);
   }
   
   /* Init sensor communication */
   dhtSetup(data_pin, model);
   if (getStatus() != ERROR_NONE)
   {
      printf("Error code: (%d), %s\n", getStatus(), getStatusString());
      return -1;
   }

   do    
   {
      /* Read sensor */
      readSensor();
   
      if (getStatus() == ERROR_NONE)
      {
         printf("RH: %3.1f\n", getHumidity());
         printf("T:  %3.1f\n", getTemperature());
      }
      else
      {
         usleep(1000000);
      }
   }
   while ((getStatus() != ERROR_NONE) && retry--);
   
   if (getStatus() != ERROR_NONE)
   {
      printf("Error code: %s\n", getStatusString());
   }
   
   /* Cleanup */
   dhtCleanup();
   
   return 0;
}
