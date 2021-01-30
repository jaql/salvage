#include "init.h"
#include "heap.h"
#include "http.h"
#include "log.h"
#include "opts.h"

void g_init(int argc, char* argv[]) {
    int opts_parse_result;

    TRACEV("g_init(%d, %p)", argc, argv);

    g_opts_init();
    TRACE("Options initalized");

    opts_parse_result = g_opts_parse(argc, argv);
    if (opts_parse_result) {
        g_opts_usage(argv[0], opts_parse_result);
        FATAL(FATAL_ERROR_OPTS_PARSE, "Could not parse options");
    }
    TRACE("Options parsed");

    g_heap_init(g_opts->heap_size);
    TRACE("Heap initialized");

    g_http_init();
    TRACE("HTTP initialized");
}

void g_destroy() {
    TRACE("g_destroy()");

    g_http_destroy();
    TRACE("HTTP destroyed");

    g_heap_destroy();
    TRACE("Heap destroyed");

    g_opts_destroy();
    TRACE("Options destroyed");
}