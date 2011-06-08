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

#ifndef __BWLIM_H__
#define __BWLIM_H__

#define NanosecondsPerSecond 1000000000

struct bwlstate {
	int schedule[24];
	int grain;
	int kbs;			// current bandwidth
	long long btot;
	long long start;	// start
	long long lt;		// time stamp of last limit
	long long tot;		// total bytes between limiting
	int verbose;
};

extern void bwlimit_msleep(long ms);
extern long long bwlimit_timer(void);
extern void bwlimit_init(struct bwlstate *state);
extern void bwlimit_args(struct bwlstate *state, int argc, char **argv);
extern void bwlimit_start(struct bwlstate *state);
extern void bwlimit_limit(struct bwlstate *state, void *block, size_t l);
extern void bwlimit_end(struct bwlstate *state);

#endif /* __BWLIM_H__ */
