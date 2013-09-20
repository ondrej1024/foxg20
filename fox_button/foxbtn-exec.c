/*
 *  Filename: foxbtn-exec.c
 * 
 *  Copyright (c) 2011 Ondrej Wisniewski
 *
 *  Detect button press events of the FoxBoard on-board push button
 *  and execute an arbitrary shell command.
 * 
 *  This program needs the gpio-keys kernel driver to be running
 *  which provides the button events on device /dev/input/event0
 * 
 *  Based on code in evtest.c from Vojtech Pavlik
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

#include <stdint.h>

#include <linux/input.h>

#include <string.h>
#include <fcntl.h>
#include <unistd.h>
#include <stdio.h>
#include <time.h>

/* define the timeout of a short button press, everything 
 * longer than that will be a long button press 
 */
#define SHORT_TIMEOUT 3

int main (int argc, char **argv)
{
	int fd, rd, i;
	struct input_event ev[64];
	int version;
	unsigned short id[4];
	char name[256] = "Unknown";
	time_t start_time;
	time_t duration;

	if (argc < 2) {
		printf("Usage: %s /dev/input/event<X> [cmd1] [cmd2]\n", argv[0]);
		printf("  X    = input device number\n");
		printf("  cmd1 = shell command to execute in case of short button press (less than %ds)\n", SHORT_TIMEOUT);
		printf("  cmd2 = shell command to execute in case of long button press (more than %ds)\n", SHORT_TIMEOUT);
		return 1;
	}

	if ((fd = open(argv[1], O_RDONLY)) < 0) {
		perror("failed to open input device");
		return 1;
	}

	if (ioctl(fd, EVIOCGVERSION, &version)) {
		perror("can't get version");
		return 1;
	}

	printf("Input driver version is %d.%d.%d\n",
		version >> 16, (version >> 8) & 0xff, version & 0xff);

	ioctl(fd, EVIOCGID, id);
	printf("Input device ID: bus 0x%x vendor 0x%x product 0x%x version 0x%x\n",
		id[ID_BUS], id[ID_VENDOR], id[ID_PRODUCT], id[ID_VERSION]);

	ioctl(fd, EVIOCGNAME(sizeof(name)), name);
	printf("Input device name: \"%s\"\n", name);

	printf("Waiting for event from BTN_1 ... (interrupt to exit)\n");

	while (1) {
		rd = read(fd, ev, sizeof(struct input_event) * 64);

		if (rd < (int) sizeof(struct input_event)) {
			printf("yyy\n");
			perror("\nerror reading from device");
			return 1;
		}

		for (i = 0; i < rd / sizeof(struct input_event); i++)

			if ((ev[i].type == EV_KEY) && (ev[i].code == BTN_1)) {
				
				/* event for on board push button detected */
			  
				if (ev[i].value == 1) {
					
					/* button pressed, start time measurement */
					start_time = time(NULL);
				}
				
				if (ev[i].value == 0) {
					
					/* button released, calculate duration */
					duration = time(NULL)-start_time;

					if (duration < SHORT_TIMEOUT) {
					  
						/* execute first command */
						if (argc > 2) {
							printf("Executing shell command \"%s\" \n", argv[2]);
							system(argv[2]);
						}
						else
							printf("FoxBoard push button pressed for %d s (no cmd1 specified)\n", duration);
					}
					else { 
						/* execute second command */
						if (argc > 3) {
							printf("Executing shell command \"%s\" \n", argv[3]);
							system(argv[3]);
						}
						else
							printf("FoxBoard push button pressed for %d s (no cmd2 specified)\n", duration);
					}
				}
			}
			
	}
}
