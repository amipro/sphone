/*
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 * 
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU Library General Public License for more details.
 * 
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor Boston, MA 02110-1301,  USA
 */

#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <linux/input.h>
#include <linux/uinput.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>
#include <dirent.h>
#include <stdio.h>

#include "utils.h"

 
int setup_proximity(){
	int err=0;
	DIR *dp;
	struct dirent *ep;
	char evtype_b[EV_MAX/8];
	int fd;
	char buf[21];
        char tmp[50];

	// loop for input devices in /dev/input
	dp = opendir ("/dev/input");
	if (dp != NULL){
		while ((ep=readdir(dp))){
			debug("Looping for input device %s\n",ep->d_name);
			if(!strncmp("event",ep->d_name,5)){
				// test device has keys but not a pointer
				sprintf(buf,"/dev/input/%s",ep->d_name);
				fd=open(buf,O_RDONLY|O_NDELAY);
				if(fd<0){
					error("Open error of evdev[%s]: %s\n",ep->d_name, strerror(errno));
					continue;
				}
				memset(evtype_b, 0, sizeof(evtype_b));
				if (ioctl(fd, EVIOCGBIT(0, EV_MAX), evtype_b) < 0) {
					error("evdev ioctl error of evdev[%s]: %s\n",ep->d_name, strerror(errno));
					close(fd);
					continue;
				}
				if (!TEST_BIT(EV_KEY, evtype_b) || TEST_BIT(EV_REL, evtype_b) || TEST_BIT(EV_ABS, evtype_b)){
					debug("Input device %s was ignored\n",ep->d_name);
					close(fd);
					continue;
				}
				evdev_fd_list[evdev_count++]=fd;
				debug("Input device %s was added\n",ep->d_name);
			}
		}
		closedir (dp);
	}
	else{
		error ("Couldn't open the directory\n");
		err=-1;
	}

	if(!evdev_count){
		error ("Can't find valid input device\n");
		err=-1;
	}

	// clearup incase of error
	if(err){
		clear_evdev();
	}

	return err;
}

 void proximity_test()
{
        
}
