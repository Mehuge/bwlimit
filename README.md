#bwlimit man Page

###NAME
*bwlimit* - bandwidth limiter for pipelines

###SYNOPSIS

    bwlimit [-v] [-g<granularity>] [[schedule,]limit [...]]

###DESCRIPTION

*bwlimit* is a command line utility intended to provide a simple means of bandwidth limiting a stream of data in a pipeline.  It is intended to be used in conjunction with utilities like *ssh(1)* to limit the amount of data transmitted across the network, though it can be used anywhere a pipeline might require bandwidth limiting, for instance writing to disk.  

bwlimit supports limiting bandwidth on a schedule, that is different limits can be set at different times of the day, such that long transfers will slow down or speed up while still running, depending on what time of day it is.

###USAGE

The schedule is internally held a 24 integers which represent the respective bandwidth specified in KiB/s units, for each of the 24 hours in a day.

Each schedule argument is parsed and used to add to the current 24 hour schedule.  Each successive schedule argument overwrites the previous for the hours it specified.  The _schedule:_ part of the argument can be omitted, in which case the bandwidth argument applies to the entire 24 hour schedule (and overwrites all previous schedules).  If schedule is specified it is either a single hour, or a range of hours separated with a hyphen, for example _7-21:_ means hours 7 thru 21.

The default schedule is unlimited 24 hrs a day, so *bwlimit* with no arguments is effectively the same as *cat(1)*. 

###Examples

*bwlimit*
 This simply copies standard input to standard output without applying any limits.

*bwlimit 100*
 This sets the bandwidth limit to 100 KiB/s for the whole 24 hours.

*bwlimit 100 7-21,50*
 This sets the default limit to 100 KiB/s and then sets a limit of 50KiB/s for the hours 7 thru 21.

*bwlimit 0 7,100 8,100 9-17,50 18-21,100*
 This sets the default limit as unlimited but then sets a limit of 100 KiB/s between 7-8:59:59am and 18-20:59:59pm, and a limit of 50 KiB/s for the hours 9am thru 5pm.

*bwlimit 7-21,100 9-17,50*
 This example is exactly the same as the previous example, just shorter.  It uses the fact that the default limit is unlimited to drop the first argument.  It also uses the fact that subsequent schedules overwrite previous ones and sets the 100KiB/s limit for hours 7 thru 21 and then overwrites the hours 9 thru 17 with a limit of 50KiB/s

###Options

*`-v`* Verbose mode.  Causes *bwlimit* to print details of bandwidth usage and limits to standard error.

*`-g<granularity>`*
 Set the number of times a second *bwlimit* will check and limit bandwidth.  The finer the granularity the smoother the bandwidth usage.  The default is to check bandwidth ever 1ms interval which in most cases will be fine enough granularity.  Setting granularity to 0 means that bandwidth is checked after every read, which given the internal buffer size for the read is 16k means every 16k transfered.

###AUTHOR
*bwlimit* was originally written by Austin France.

The *bwlimit* project is open source and can be found at http://bwlimit.googlecode.com/. It is release under the [http://www.opensource.org/licenses/bsd-license.php New BSD license].

###SEE ALSO

*[bwlimitAPI bwlimit_*() API]*

# HOW-TO checkout, build and install bwlimit

###CHECK OUT

    $ git clone https://github.com/Mehuge/bwlimit

###BUILD

    $ cd bwlimit
    $ make

###INSTALL

    $ cd bwlimit
    $ make install

#bwlimit_*() API

###Introduction

*bwlimit* is implemented as an API as well as a command line tool.  This page documents the API.

###SYNOPSIS

    #include "bwlimit.h"
    void bwlimit_msleep(long ms); 
    long long bwlimit_timer(void); 
    void bwlimit_init(struct bwlstate *state);
    void bwlimit_args(struct bwlstate *state, int argc, char **argv); 
    void bwlimit_start(struct bwlstate *state); 
    void bwlimit_limit(struct bwlstate *state, size_t size); 
    void bwlimit_end(struct bwlstate *state); 

###DESCRIPTION

The main sequence of calls required to bandwidth limit a data stream is _bwlimit_init()_ -> _bwlimit_start()_ -> _bwlimit_limit()_ -> _bwlimit_end()_.  _bwlimit_args()_ can be called between init and start to set the bandwidth limits per hour based on *[bwlimit bwlimit(1)]* compatible command line arguments.

*bwlimit_init(struct bwlstate `*`state)*
 This must be called at the start of the process.  It initialises the passed struct bwlstate structure with the default settings and prepares it for use.

*bwlimit_args(struct bwlstate `*`state, int argc, char `*``*`argv)*
 This is optionally called after init and before start to set the bandwidth limit schedule based on command line arguments compatible with *[bwlimit bwlimit(1)]*.  Bandwidth limits can be set directly in the bwlimit state structure through the *schedule[]* member, which is a 24 element integer array containing the bandwidth limits in KiB/s for each of the 24 hours.

*bwlimit_start(struct bwlstate `*``*`state)*
 This starts the bandwidth limit process.  Essentially it resets the totals and sets the start time.

*bwlimit_limit(struct bwlstate `*`state, size_t size)*
 This is the guts of the bandwidth limiting logic.  It should be called somewhere in the main loop after a read and before the write.  It is passed the size of the data just read and will effect a delay based on the amount of data passed through so far to limit the throughput to the desired bandwidth for the current hour.

*bwlimit_end(struct bwlstate `*`state)*
 This should be called once processing is complete to perform cleanup and to output totals in verbose mode.

*bwlimit_timer()*
 This is a timer routine used internally to return the number of nanoseconds since an unknown point in time.  The granularity of the timer may not be as fine as a nanosecond, and will depend upon the platform on which it is running.

*bwlimit_msleep(long ms)*
 This is a sleep routine used internally to sleep for a specified number of miliseconds.

*struct bwlstate*

    struct bwlstate {
        int schedule[24];   /* bandwidth schedule (KiB/s/hour) */
        int grain;          /* grain size */
        int kbs;            /* currently selected bandwidth limit */
        long long btot;     /* total bytes transfered */
        long long start;    /* start */
        long long lt;       /* time stamp of last limit */
        long long tot;      /* total bytes between limiting */
        int verbose;        /* verbose mode */
    };

Most of the members of this structure should not be modified, however three members may be modified before _bwlimit_start()_ is called.

*schedule*
 This is an array of 24 integers representing the bandwidth limits for each hour of the day.  This can be initialised by the application any way it sees fit.  A bandwidth limit of 0 means unlimited which is what this array is initialised to by default.

*grain*
 This is the grain size, or the number of times per second, that bandwidth is checked.  This is set to 1000 (ie once every 1ms) by default.

*verbose*
 If set to a non-zero value, the _bwlimit`_``*`() API_ will output to stderr, diagnostic information showing bandwidth and limiting progress.

###AUTHOR
*bwlimit* was originally written by Austin France.

The *bwlimit* project is open source and can be found at http://bwlimit.googlecode.com/. It is release under the [http://www.opensource.org/licenses/bsd-license.php New BSD license].

###SEE ALSO
*[bwlimit bwlimit(1)]*

#Example Usage

###Pull Examples

In these examples, note that bandwidth is only limited in the return direction (output from the command run on the server), this is because we want to limit the bandwidth of the data we are pulling from that server.

###rdiff-backup

    rdiff-backup --remote-schema="ssh -C %s 'rdiff-backup --server | bwlimit 100 9-17,20'" <remote-path> <local-path>

###rsync

rsync is a little more difficult, bwlimit cannot be used with rsync directly, we need to use a little *bash(1)* script to run *bwlimit*

    #/bin/bash
    # bwssh - run rsync server with bandwidth limit on returned data
    HOST=$1 ; shift
    exec ssh $HOST "$@ | bwlimit 100 9-17,20"

    $ rsync -ebwssh <remote-path> <local-path>

###Push Examples

If we were pushing data to a server then the limit needs to be placed before the remote command.

###rdiff-backup

    rdiff-backup --remote-schema="bwlimit 100 9-17,20 | ssh -C %s 'rdiff-backup --server'" <local-path> <remote-path>

###rsync

rsync is a little more difficult, bwlimit cannot be used with rsync directly, we need to use a little *bash(1)* script to run *bwlimit*

    #/bin/bash
    # bwssh - run rsync server with bandwidth limit on data sent to server
    HOST=$1 ; shift
    exec bwlimit 100 9-17,20 | ssh $HOST "$@"

    $ rsync -ebwssh <remote-path> <local-path>
