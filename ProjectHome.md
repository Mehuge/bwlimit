Allows limiting bandwidth based on an hourly schedule.  The command line tool implements a standard input/output bandwidth limiter which can be used as part of an **ssh(1)** pipeline to limit bandwidth used across the network.  It can be used wherever a pipeline is used and bandwidth limiting is required.

  * **[bwlimit(1)](bwlimit.md)** - Manual Page
  * **[bwlimit\_\*() API](bwlimitAPI.md)** - bwlimit API documentation.
  * **[Build](build.md)** - Build Instructions

The advantage of this tool is that it can define a schedule of limits so that long transfers can adjust their bandwidth limit on the fly according to the current time, whereas the bandwidth limiting options of the common transfer commands such as **scp(1)**, **rsync(1)** and **rdiff-backup(1)** all specify a single limit at the start of the transfer, so if that transfer takes a long time and crosses into a period where a different limit is required, then, well, tough.

**bwlimit** is not intended to replace linux traffic control, it is intended to be more an ad-hoc and simple (relatively) solution to a very specific problem.

  * **[Examples](examples.md)** - Some examples of how to use bwlimit