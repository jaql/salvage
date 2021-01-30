#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <strings.h>

#include "log.h"
#include "opts.h"

/**
 * The options instance.
 */
struct Options g_opts_instance;

/**
 * Pointer to the options instance, as we may allocate differently in future.
 */
struct Options* g_opts = NULL;

void g_opts_init() {
    TRACE("g_opts_init()");
    g_opts = &g_opts_instance;
    bzero(g_opts, sizeof(struct Options));
}

int g_opts_parse(int argc, char* argv[]) {
    int opt;

    TRACEV("g_opts_parse(%d, %p)", argc, argv);

    bzero(g_opts, sizeof(struct Options));

    g_opts->heap_size = DEFAULT_HEAP_SIZE;
    g_opts->quiesce_secs = DEFAULT_QUIESCE_SECS;
    g_opts->max_attempts = DEFAULT_MAX_ATTEMPTS;

    while ((opt = getopt(argc, argv, "s:m:u:c:f:h:q:")) != -1) {
        DEBUGV("Encountered option %c", opt);

        switch (opt) {
            case 's':
                g_opts->heap_size = atoi(optarg);
                INFOV("Heap size is: %s", optarg);
                break;
            case 'm':
                g_opts->method = optarg;
                INFOV("HTTP method is: %s", optarg);
                break;
            case 'u':
                g_opts->url = optarg;
                INFOV("HTTP base URL is: %s", optarg);
                break;
            case 'c':
                g_opts->certificate = optarg;
                INFOV("Certificate is: %s", optarg);
                break;
            case 'h':
                g_opts->headers[g_opts->nheaders++] = optarg;
                if (g_opts->nheaders == MAX_HEADERS) {
                    return OPTS_PARSE_HEADER_OVERFLOW;
                }
                INFOV("Header %d is: %s", g_opts->nheaders, optarg);
                break;
            case 'f':
                g_opts->files[g_opts->nfiles++] = optarg;
                if (g_opts->nfiles == MAX_FILES) {
                    return OPTS_PARSE_FILE_OVERFLOW;
                }
                INFOV("File %d is: %s", g_opts->nfiles, optarg);
                break;
            case 'q':
                g_opts->quiesce_secs = atoi(optarg);
                INFOV("Quiesce is: %s", optarg);
                break;
            case '?':
            default:
                return OPTS_PARSE_SYNTAX;
        }
    }

    if (argc - optind < 1) {
        DEBUG("No exec pathname provided");
        return OPTS_PARSE_NO_EXEC;
    }

    g_opts->exec_pathname = argv[optind];
    INFOV("Exec pathname is: %s", g_opts->exec_pathname);

    for (optind = optind + 1; optind < argc; optind++) {
        g_opts->exec_args[g_opts->exec_nargs++] = argv[optind];
        if (g_opts->exec_nargs == MAX_EXEC_ARGS) {
            return OPTS_PARSE_EXEC_ARGS_OVERFLOW;
        }
        INFOV("Exec arg %d is: %s", g_opts->exec_nargs, argv[optind]);
    }
    g_opts->exec_args[g_opts->exec_nargs] = NULL;

    if (g_opts->max_attempts < 1) {
        DEBUG("Illegal max attempts");
        return OPTS_PARSE_BAD_MAX_ATTEMPTS;
    }

    if (g_opts->heap_size < MIN_HEAP_SIZE) {
        DEBUG("Illegal heap size");
        return OPTS_PARSE_BAD_HEAP_SIZE;
    }

    if (g_opts->quiesce_secs < 0) {
        DEBUG("Illegal quiesce secs");
        return OPTS_PARSE_BAD_QUIESCE_SECS;
    }

    if (g_opts->url == NULL || strlen(g_opts->url) == 0) {
        DEBUG("Illegal URL");
        return OPTS_PARSE_BAD_URL;
    }

    if (g_opts->method == NULL || strlen(g_opts->method) == 0) {
        DEBUG("Illegal method");
        return OPTS_PARSE_BAD_METHOD;
    }

    if (g_opts->exec_pathname == NULL || strlen(g_opts->exec_pathname) == 0) {
        DEBUG("Illegal exec path");
        return OPTS_PARSE_NO_EXEC;
    }

    if (g_opts->nfiles == 0) {
        DEBUG("No files");
        return OPTS_PARSE_NO_FILES;
    }

    DEBUG("Success");
    return OPTS_PARSE_SUCCESS;
}

void g_opts_usage(char* image_name, int opts_parse_result) {
    #define EXPLAIN(message, ...) fprintf(stderr, message "\n")
    #define EXPLAINV(message, ...) fprintf(stderr, message "\n", __VA_ARGS__)

    switch (opts_parse_result) {
        case OPTS_PARSE_SYNTAX:
            ERROR("Invalid option provided");
            break;
        case OPTS_PARSE_BAD_HEAP_SIZE:
            ERRORV("Invalid heap size provided.  Must be a number of bytes bigger than %d", MIN_HEAP_SIZE);
            break;
        case OPTS_PARSE_BAD_URL:
            ERROR("No suitable URL provided");
            break;
        case OPTS_PARSE_BAD_METHOD:
            ERROR("No suitable HTTP method provided");
            break;
        case OPTS_PARSE_NO_FILES:
            ERROR("No suitable files provided");
            break;
        case OPTS_PARSE_FILE_OVERFLOW:
            ERRORV("Too many files parsed.  Only %d files are supported", MAX_FILES);
            break;
        case OPTS_PARSE_HEADER_OVERFLOW:
            ERRORV("Too many headers parsed.  Only %d headers are supported", MAX_HEADERS);
            break;
        case OPTS_PARSE_NO_EXEC:
            ERROR("No program given to exec");
            break;
        case OPTS_PARSE_EXEC_ARGS_OVERFLOW:
            ERRORV("Too many program arguments parsed.  Only %d arguments are supported", MAX_EXEC_ARGS);
            break;
        case OPTS_PARSE_BAD_QUIESCE_SECS:
            ERROR("Invalid quiesce time provided.  Must be 0 or more seconds");
            break;
        case OPTS_PARSE_BAD_MAX_ATTEMPTS:
            ERROR("Invalid max attempts provided.  Must be 1 or more");
    }

    EXPLAINV("Usage: %s -u URL -f FILE [-f ...] [-n MAX_ATTEMPTS] [-q QUIESCE_SECS] [-m METHOD] [-s HEAP_SIZE] [-c CERTIFICATE] [-h HEADER [-h ...]] PROGRAM ARG [...]", image_name);
    EXPLAIN("\t-u URL\tURL to upload to (required)");
    EXPLAINV("\t-f FILE\tFile to upload (required, multiple, up to %d files)", MAX_FILES);
    EXPLAINV("\t-n MAX_ATTEMPTS\tMax attempts to upload a file (optional, default %d)", DEFAULT_MAX_ATTEMPTS);
    EXPLAINV("\t-q QUIESCE_SECS\tSeconds to wait after abnormal termination before uploading (optional, default %d)", DEFAULT_QUIESCE_SECS);
    EXPLAINV("\t-m METHOD\tHTTP method to upload with (optional, default %s)", DEFAULT_METHOD);
    EXPLAINV("\t-s HEAP_SIZE\tHeap size (optional, default %dB)", DEFAULT_HEAP_SIZE);
    EXPLAINV("\t-h HEADER\tHeader to send with upload (optional, multiple, up to %d headers)", MAX_HEADERS);
    EXPLAIN("\tPROGRAM\tProgram to execute (required)");
    EXPLAIN("\tARG\tArguments for program to executed (optional, multiple, no limit)");
}

void g_opts_destroy() {
    g_opts = NULL;
}