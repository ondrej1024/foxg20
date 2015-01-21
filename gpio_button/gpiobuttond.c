/*
 *  Filename: gpiobuttond.c
 * 
 *  Author: Ondrej Wisniewski (2015)
 *
 *  Description:
 *  Detect button press events of a push button connected
 *  to a GPIO line and execute an arbitrary shell command.
 * 
 *  Build:
 *  gcc -Wall -o gpiobuttond gpiobuttond.c
 * 
 */

/*
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or 
 * (at your option) any later version.
 * 
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 * 
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA 02111-1307 USA
 * 
 */
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <unistd.h>
#include <errno.h>
#include <fcntl.h>
#include <poll.h>
#include <time.h>
#include <signal.h>

#define PUSHBUTTON_PIN 81
#define GPIO_BASE_DIR "/sys/class/gpio"
#define EXPORT_FILE "/sys/class/gpio/export"
#define UNEXPORT_FILE "/sys/class/gpio/unexport"

/* define the timeout of a short button press, everything 
 * longer than that will be a long button press 
 */
#define SHORT_TIMEOUT 3

typedef enum {
   LOW,
   HIGH
}
PIN_STATE_t;

static uint8_t gpio_pin=0;


static void sysfs_filename(char *filename, int len, int pin, const char *function)
{
   snprintf(filename, len, "%s/pio%c%d/%s", GPIO_BASE_DIR, 'A'+pin/32, pin%32, function);
}


/*********************************************************************
 * Function: setup()
 *
 * Description: 
 *
 * Parameters: pin - Kernel ID of GPIO pin to set up
 *
 ********************************************************************/
static int setup(uint8_t pin)
{
   int fd;
   char b[64];
   
   // Prepare GPIO pin connected to sensors data pin to be used with GPIO sysfs
   // (export to user space)
   fd = open(EXPORT_FILE, O_WRONLY);
   if (fd < 0) {
      perror(EXPORT_FILE);
      return 1;
   }
   snprintf(b, sizeof(b), "%d", pin);
   if (pwrite(fd, b, strlen(b), 0) < 0) {
      fprintf(stderr, "Unable to export pin=%d (already in use?): %s\n",
              pin, strerror(errno));
      return 2;
   }
   close(fd);
   
   // Define edge interrupt (both edge)
   // to be used later with the poll() function for edge detection
   sysfs_filename(b, sizeof(b), pin, "edge");
   fd = open(b, O_RDWR);
   if (fd < 0) {
      fprintf(stderr, "Open %s: %s", b, strerror(errno));
      return 3;
   }
   if (pwrite(fd, "both", 4, 0) < 0) {
      fprintf(stderr, "Unable to write 'both' to %s: %s",
             b, strerror(errno));
      return 4;
   }
   close(fd);
   
   // Define gpio direction as input
   sysfs_filename(b, sizeof(b), pin, "direction");
   fd = open(b, O_RDWR);
   if (fd < 0) {
      fprintf(stderr, "Open %s: %s", b, strerror(errno));
      return 5;
   }
   if (pwrite(fd, "in", 2, 0) < 0) {
      fprintf(stderr, "Unable to write 'in' to %s: %s",
             b, strerror(errno));
      return 6;
   }
   close(fd);
   
   return 0;
}


/*********************************************************************
 * Function:    cleanup()
 * 
 * Description: Cleanup of globally used resources
 * 
 * Parameters: pin - Kernel ID of GPIO pin
 * 
 ********************************************************************/
static int cleanup(uint8_t pin)
{
   int fd;
   char b[8];
   
   // Free GPIO pin (unexport)
   fd = open(UNEXPORT_FILE, O_WRONLY);
   if (fd < 0) {
      perror(UNEXPORT_FILE);
      return 1;
   }
   snprintf(b, sizeof(b), "%d", pin);
   if (pwrite(fd, b, strlen(b), 0) < 0) {
      fprintf(stderr,  "Unable to unexport pin=%d: %s",
              pin, strerror(errno));
      return 2;
   }
   close(fd);
   return 0;
}


/*********************************************************************
 * Function: digitalRead()
 *
 * Description: Read from the data pin
 *
 * Parameters: fd - file descriptor of GPIO sysfs value file
 *
 ********************************************************************/
static PIN_STATE_t digitalRead(int fd)
{
   char d[1];
   if (pread(fd, d, 1, 0) != 1) {
      fprintf(stderr, "Unable to pread gpio value: %s\n",
              strerror(errno));
   }
   return (d[0] == '0' ? LOW : HIGH);
}


/*********************************************************************
 * Function:    waitForEdge()
 * 
 * Description: Waits for an edge change on the GPIO pin
 * 
 * Parameters:  pin   - Kernel ID of GPIO pin 
 *              value - pin value after edge detection
 * 
 ********************************************************************/
static int waitForEdge(uint8_t pin, PIN_STATE_t* value)
{
   struct pollfd a_poll;
   int res;
   int fd;
   char b[64];
   
   // Open gpio value file for reading
   sysfs_filename(b, sizeof(b), pin, "value");
   fd = open(b, O_RDWR);
   if (fd < 0) {
      fprintf(stderr, "Open %s: %s", b, strerror(errno));
      return 1;
   }
   
   a_poll.fd = fd;
   a_poll.events = POLLPRI;
   a_poll.revents = 0;
   
   digitalRead(fd);
   res = poll(&a_poll, 1, -1);
   if (res < 0) {   // error
      fprintf(stderr, "poll() failed: %s", strerror(errno));
      return 2;
   }
   
   if (a_poll.revents & (POLLPRI | POLLERR)) {
      *value=digitalRead(fd);
      return 0;
   }
   else {
      fprintf(stderr, "poll() detected unknown event");
      return 3;
   }
}

/*********************************************************************
 * Function:    doExit()
 * 
 * Description: Signal handler function to do a clean shutdown
 * 
 * Parameters:  the received signal
 * 
 ********************************************************************/
static void doExit(int signum)
{
   //syslog(LOG_DAEMON | LOG_NOTICE, "caught signal %d in process %d", signum, pid);
  
   cleanup(gpio_pin);
   kill(getpid(), SIGKILL);
  
}


int main(int argc, char* argv[])
{
   PIN_STATE_t value;
   time_t start_time;
   time_t duration;
   
   if (argc < 2) {
      printf("Usage: %s <pin> [cmd1] [cmd2]\n", argv[0]);
      printf("  pin  = Kernel Id of GPIO pin\n");
      printf("  cmd1 = shell command to execute in case of short button press (less than %ds)\n", SHORT_TIMEOUT);
      printf("  cmd2 = shell command to execute in case of long button press (more than %ds)\n", SHORT_TIMEOUT);
      return 1;
   }

   /* Install signal handler for SIGTERM and SIGINT ("CTRL C") 
    * to be used to cleanly terminate parent annd child  processes
    */
   signal(SIGTERM, doExit);
   signal(SIGINT, doExit);
   
   gpio_pin = atoi(argv[1]);
   if (gpio_pin == 0) {
      fprintf(stderr, "Invalid GPIO pin\n");
      return 2;
   }
   
   printf("using GPIO pin %d\n", gpio_pin);
   
   setup(gpio_pin);
   
   while (1) {
      
      waitForEdge(gpio_pin, &value);
      
      switch (value)
      {
         case HIGH:
            /* button released, calculate duration */
            duration = time(NULL)-start_time;
            
            if (duration < SHORT_TIMEOUT) {
               
               /* execute first command */
               if (argc > 2) {
                  fprintf(stderr, "Executing shell command \"%s\" \n", argv[2]);
                  system(argv[2]);
               }
               else
                  fprintf(stderr, "Push button pressed for %u s (no cmd1 specified)\n", (unsigned int)duration);
            }
            else { 
               
               /* execute second command */
               if (argc > 3) {
                  fprintf(stderr, "Executing shell command \"%s\" \n", argv[3]);
                  system(argv[3]);
               }
               else
                  fprintf(stderr, "Push button pressed for %u s (no cmd2 specified)\n", (unsigned int)duration);
            }
            break;
            
         case LOW:
            fprintf(stderr, "Push butten press started\n");
            start_time = time(NULL);
            break;          
      }
   }

   return 0;   
}
