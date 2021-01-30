#include "log.h"
#include "exec.h"
#include "init.h"
#include "http.h"
#include "opts.h"

int main(int argc, char* argv[]) {
    int exit_code, nuploaded;

    TRACEV("main(%d, %p)", argc, argv);
    INFO("Initializing...");
    g_init(argc, argv);

    INFO("Executing child process...");
    exit_code = g_exec_child_process();

    if (exit_code != 0) {
        INFO("Uploading files...");
        nuploaded = g_http_upload_files();
        if (nuploaded < g_opts->nfiles) {
            ERRORV("Only uploaded %d of %d files", nuploaded, g_opts->nfiles);
        }
    }

    INFO("Shutting down...");
    g_destroy();

    INFOV("Terminating with code %d", exit_code);
    return exit_code;
}
