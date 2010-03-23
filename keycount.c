/*
 * $Id: evtest.c,v 1.23 2005/02/06 13:51:42 vojtech Exp $
 *
 *  Copyright (c) 1999-2000 Vojtech Pavlik
 *
 *  Event device test program
 *
 * 2010-03-23: Mangled into a keycount / mouse distance monitor
 *   svensven@gmail.com
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
 * Should you need to contact me, the author, you can do so either by
 * e-mail - mail your message to <vojtech@ucw.cz>, or by paper mail:
 * Vojtech Pavlik, Simunkova 1594, Prague 8, 182 00 Czech Republic
 */

#include <linux/input.h>
#include <fcntl.h>
#include <stdio.h>
#include <math.h>
#include <time.h>
#include <poll.h>
#include <errno.h>
#include <unistd.h>

#ifndef EV_SYN
#define EV_SYN 0
#endif

#define DPI 106.
#define MPI .0254 /* meters per inch */

int main (int argc, char **argv)
{
	FILE *log;
	int fd, rd, i;
	struct input_event ev[64];
	time_t start = time(NULL);
	unsigned keycount = 0;
	int deltax = 0;
	int deltay = 0;
	double distance = 0.;

	if (argc < 3) {
		printf("Usage: evtest <inputdev> <logfile>\n");
		return 1;
	}

	if ((fd = open(argv[1], O_RDONLY)) < 0) {
		perror(argv[1]);
		return 1;
	}

	/* prepare for poll loop */
	int newdata = 1;
	int nfds;
	struct pollfd pfd[1];
	int timeout = 10000;

	/* stuff to poll */
	pfd[0].fd = fd;
	pfd[0].events = POLLIN;
	pfd[0].revents = 0;

	for(;;) {
		time_t now;
		nfds = poll(pfd, 1, timeout);
		if (nfds == -1 || (pfd[0].revents & (POLLERR|POLLHUP|POLLNVAL))) {
			if (errno != EINTR)
			{
				perror("Poll error");
				return 1;
			}
		}

		/* Handle event read */
		if (pfd[0].revents) {
			rd = read(fd, ev, sizeof(struct input_event) * 64);
			if (rd < (int) sizeof(struct input_event)) {
				perror("Read error");
				return 1;
			}
			for (i = 0; i < rd / (int) sizeof(struct input_event); i++)
				if (ev[i].type == EV_SYN && (deltax || deltay)) {
					/* Handle queued mouse movement */
					distance += sqrt(deltax*deltax+deltay*deltay) /
							DPI * MPI;
					deltax = deltay = 0;
				} else if (ev[i].type == EV_KEY) {
					if (ev[i].value == 1) {
						keycount++;
						newdata = 1;
					}
				} else if (ev[i].type == EV_REL) {
					if (ev[i].code == REL_X)
						deltax = ev[i].value;
					if (ev[i].code == REL_Y)
						deltay = ev[i].value;
					newdata = 1;
				}
		}

		now = time(NULL);
		if (now/10 != start/10) {
			log = fopen(argv[2], "w");
			fprintf(log, "%u %.5f\n",
					keycount, distance);
			fclose(log);
			start = now;
		}
	}
}
