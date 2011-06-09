/*
* Copyright (c) 2011, Austin David France, RedSky IT
* All rights reserved.
* 
* - Redistribution and use in source and binary forms, with or without 
*	modification, are permitted provided that the following conditions are met:
*
* - Redistributions of source code must retain the above copyright notice, this
*	list of conditions and the following disclaimer.
*
* - Redistributions in binary form must reproduce the above copyright notice,
*	this list of conditions and the following disclaimer in the documentation 
*	and/or other materials provided with the distribution.
*
* THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
* AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE 
* IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
* ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE
* LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
* CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
* SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
* INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
* CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
* ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
* POSSIBILITY OF SUCH DAMAGE.
*/

#include <time.h>
#include <stdio.h>
#include <sys/times.h>
#include <sys/types.h>
#include <sys/select.h>
#include <unistd.h>
#include <string.h>
#include <errno.h>
#include <signal.h>

#include "bwlimit.h"

#ifdef LIB

void bwlimit_msleep(long ms){
	struct timeval timeout;
	timeout.tv_sec = ms / 1000;
	timeout.tv_usec = (ms % 1000) * 1000;
	(void) select (0, 0, 0, 0, &timeout);
}

long long bwlimit_timer() {
#ifdef CLOCK_REALTIME
	struct timespec tsp;
	int t = clock_gettime(CLOCK_REALTIME, &tsp);
	long long ts = tsp.tv_sec;
	ts *= NanosecondsPerSecond;
	ts += tsp.tv_nsec;
#else
	struct timeval tv;
	gettimeofday(&tv, NULL);
	long long ts = tv.tv_sec;
	ts *= NanosecondsPerSecond;
	ts += tv.tv_usec*1000;
#endif
	return ts;
}
void bwlimit_init(struct bwlstate *state) {
	int n;
	memset(state, 0, sizeof(struct bwlstate));
	state->grain = 1000;					/* check every milisecond */
}

void bwlimit_args(struct bwlstate *state, int argc, char **argv) {
	int n;
	while (argc>1) {
		char *p = strchr(argv[1],',');
		if (p) {
			int start = atoi(argv[1]);
			int kbs = atoi(p+1);
			char *q = strchr(argv[1],'-');
			int end = q ? atoi(q+1) : start;
			for (n = start; n <= end; n++) {
				if (n >= 0 && n <= 23) state->schedule[n] = kbs;
			}
		} else {
			int kbs = atoi(argv[1]);
			for (n = 0; n <= 23; n++) {
				state->schedule[n] = kbs;
			}
		}
		argc--;argv++;
	}

	/* Dump schedule in verbose mode */
	if (state->verbose) {
		for (n = 0; n < 24; n++) {
			if ((n % 12) == 0) fprintf(stderr,"SCHEDULE:");
			fprintf(stderr," %02d:%05d", n, state->schedule[n]);
			if (n == 11) fprintf(stderr,"\n");
		}
		fprintf(stderr,"\n");
	}
}

void bwlimit_start(struct bwlstate *state) {
	state->lt = state->start = bwlimit_timer();
	state->tot = state->btot = 0;
	state->kbs = -1;				/* force pickup from schedule */
}

void bwlimit_limit(struct bwlstate *state, size_t l) {

	time_t tnow;
	struct tm now; 
	long long ticks;	/* now in nanosecond ticks */

	/* Get the current timer (nanoseconds) */
	ticks = bwlimit_timer();

	/* Work out the current bandwidth based on the current localtime hour */
	tnow = time(0);
	localtime_r(&tnow, &now);
	if (state->schedule[now.tm_hour] != state->kbs) {
		state->kbs = state->schedule[now.tm_hour];
		if (state->verbose) {
			fprintf(stderr,"USE BANDWIDTH RATE: %02d:%05d\n",now.tm_hour,state->kbs);
			fflush(stderr);
		}
	}

	/* track totals */
	state->tot += l;
	state->btot += l;

	/* bandwidth throttle */
	if (state->kbs > 0 && ticks > state->lt && (state->grain == 0 || (ticks - state->lt) >= (NanosecondsPerSecond/state->grain))) {
		double secs = (double)(ticks-state->lt)/NanosecondsPerSecond;			/* seconds since last check */
		double kbps = state->tot/secs/1024.0;						/* KB/s */
		long delayed = 0;
		while (kbps > state->kbs) {						/* if KB/s > target KB/s the we need to slow it down */
			bwlimit_msleep(1);
			delayed += 1;
			ticks = bwlimit_timer();
			secs = (double)(ticks-state->lt)/NanosecondsPerSecond;
			kbps = state->tot/secs/1024.0;
		}
		if (state->verbose) {
			fprintf(stderr,"%09lld: %7lld bytes in %9.6fs [msleep %3ld] (%8.3fKB/s) %6lldMB total\n",
				(ticks-state->start)/1000,
				state->tot,
				secs,
				delayed,
				kbps,
				state->btot/1024/1024);
		}
		state->lt = ticks;
		state->tot = 0;
	}
}

void bwlimit_end(struct bwlstate *state) {
	if (state->verbose) {
		double secs,kbps;
		long long ticks = bwlimit_timer();
		secs = (double)(ticks-state->start)/NanosecondsPerSecond;
		kbps = state->btot/1024.0/secs;
		fprintf(stderr, "%09lld: Total: wrote %8.3fKB in %8.3fs (%8.3fKB/s)\n", (ticks-state->start)/1000, state->btot/1024.0, secs, kbps);
	}
}

#else

int main(int argc, char **argv) {
	char block[16384];
	int bail = 0;
	int l,n;

	struct bwlstate state;

	/* Ignore sigpipe (return a write error instead) */
	signal(SIGPIPE, SIG_IGN);

	/* Initialise bwlimit state */
	bwlimit_init(&state);

	/* Parse arguments and set bwlimit options */
	while (argc>1&&argv[1][0]=='-') {
		if (argv[1][1] == 'v') {
			state.verbose++;
		}
		else if (argv[1][1] == 'g') {					/* Granularity of bandwidth check (n times a second) */
			state.grain = atoi(argv[1]+2);
		}
		argc--;argv++;
	}

	/* Parse bandwidth schedule arguments */
	bwlimit_args(&state, argc, argv);

	/* Start bandwidth measurement */
	bwlimit_start(&state);

	while (!bail && (l = read(0,block,sizeof(block)))>0) {

		/* Limit bandwidth */
		bwlimit_limit(&state, l);

		/* Write data to standard output (probably a socket) */
		{	int s = 0,w;
			while (l > 0) {								/* loop til done */
				do { 
					w = write(1,block+s,l); 
					if (w == -1 && errno == EAGAIN) {
						bwlimit_msleep(10);			/* resource not available, wait for a bit before trying again */
					}
				} while (w == -1 && (errno == EINTR || errno == EAGAIN));		/* EINTR or EAGAIN, try again */
				if (w == -1) {							/* other error, bail */
					if (state.verbose) { 
						int e = errno;
						perror("write");
						fprintf(stderr,"errno %d\n", e);
					}
					bail = 1;
					break;
				}
				/* track data written so far */
				s += w;
				l -= w;
			}
		}
	}

	/* Finished */
	bwlimit_end(&state);

	return 0;
}

#endif /* LIB */
