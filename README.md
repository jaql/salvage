# Salvage

These programs are to help with monitoring container deployments.  They will wait for a condition and when the process
terminates abnormally they will upload any support files specified to a given URL.

What makes them slightly interesting is that they implement a (naive, allocation only) replacement for malloc and
that works on a heap size given up front.  This means that even on a stressed container they should never crash or run
out of memory.

Therefore they could be considered a reference implementation of a dumb Linux malloc that handles alignment. 

## Support

These programs were created to help with a problem at my employer, and have not been tested.  A different solution was
found, and I was granted permission to release these as open source.  If they don't work, please leave a comment - if I
get users I will make sure to fix bugs.

Feature requests will likely be refused because of time pressures elsewhere.  I apologise for this.

## Requirements

* C11 compiler
* libcurl headers and libraries
* cmake

## Flotsam program

Flotsam will launch and wait for a SIGTERM signal.  Upon SIGTERM it will attempt to upload specified files (e.g. log files).

## Jetsam program

Jetsam will launch and run a child process.  Upon SIGTERM or child process termination it will ensure the child process
is terminated, it will attempt to upload files, and return with a non-zero exit code.

## Upload Strategy

Both binaries always try to upload as many files as possible via HTTP PUT before looping back to upload any failed files.
Upon upload, they will terminate with a non-zero exit code if they were abnormally interrupted.
