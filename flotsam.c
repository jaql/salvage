#include <signal.h>

#include "log.h"
#include "init.h"
#include "http.h"
#include "opts.h"
#include "wait.h"

/**
 * Number of signals to wait for.
 */
const int nsignums = 2;

/**
 * Signals to wait for (see above).
 */
const int signums[] = { SIGTERM, SIGUSR1 };

int main(int argc, char* argv[]) {
    int looping = 1;
    int nuploaded;
    int signum;

    TRACEV("main(%d, %p)", argc, argv);
    INFO("Initializing...");
    g_init(argc, argv);

    INFO("Waiting for signal...");
    while (looping) {
        signum = g_wait_for_signal(nsignums, signums);
        switch (signum) {
            case SIGUSR1:
                INFO("SIGUSR1 received, uploading");
                nuploaded = g_http_upload_files();
                if (nuploaded < g_opts->nfiles) {
                    ERRORV("Only uploaded %d of %d files", nuploaded, g_opts->nfiles);
                }

                // Purposefully falls through to default
            default:
                ERRORV("Signal %d received, breaking", signal);
                looping = 0;
        }
    }

    INFO("Shutting down...");
    g_destroy();

    INFO("Terminating with code 0.");
    return EXIT_SUCCESS;
}
