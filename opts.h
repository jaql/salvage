#ifndef JETSAM_OPTS_H
#define JETSAM_OPTS_H

#define MIN_HEAP_SIZE 1 * 1024 * 1024
#define DEFAULT_HEAP_SIZE 10 * 1024 * 1024
#define DEFAULT_QUIESCE_SECS 10
#define DEFAULT_METHOD "PUT"
#define DEFAULT_MAX_ATTEMPTS 3

#define MAX_HEADERS   128
#define MAX_FILES     128
#define MAX_EXEC_ARGS 128

/**
 * The outcome of parsing CLI options.
 */
enum OptsParseResult {
    OPTS_PARSE_SUCCESS = 0,

    OPTS_PARSE_SYNTAX,
    OPTS_PARSE_NO_FILES,
    OPTS_PARSE_FILE_OVERFLOW,
    OPTS_PARSE_HEADER_OVERFLOW,
    OPTS_PARSE_BAD_HEAP_SIZE,
    OPTS_PARSE_BAD_URL,
    OPTS_PARSE_BAD_METHOD,
    OPTS_PARSE_BAD_QUIESCE_SECS,
    OPTS_PARSE_NO_EXEC,
    OPTS_PARSE_EXEC_ARGS_OVERFLOW,
    OPTS_PARSE_BAD_MAX_ATTEMPTS
};

/**
 * CLI options structure.
 */
struct Options {
    /**
     * Size of heap in bytes.
     */
    int heap_size;

    /**
     * Quiesce time for subprocess in seconds.
     */
    int quiesce_secs;

    /**
     * Maximum retries for uploading a file.
     */
    int max_attempts;

    /**
     * Base URL for uploading a file.
     */
    char* url;

    /**
     * HTTP method for uploading a file.
     */
    char* method;

    /**
     * Pinned certificate filename or PEM.
     */
    char* certificate;

    /**
     * Program to execute.
     */
    char* exec_pathname;

    /**
     * Number of HTTP headers when uploading a file.
     */
    int nheaders;

    /**
     * HTTP headers when uploading a file.
     */
    char* headers[MAX_HEADERS];

    /**
     * Number of files to upload.
     */
    int nfiles;

    /**
     * Files to upload (filenames)
     */
    char* files[MAX_FILES];

    /**
     * Number of arguments for the executed program.
     */
    int exec_nargs;

    /**
     * Arguments for the executed program.
     */
    char* exec_args[MAX_EXEC_ARGS];
};

/**
 * Actual options instance.
 */
extern struct Options* g_opts;

/**
 * Initialize options instance.
 */
void g_opts_init();

/**
 * Parse CLI options into global options instance.
 * @param argc number of CLI arguments
 * @param argv CLI arguments
 * @return OptsParseResult value
 */
int g_opts_parse(int argc, char* argv[]);

/**
 * Print usage information
 * @param image_name the program binary name, usually argv[0]
 * @param opts_parse_result the result from g_opts_parse
 */
void g_opts_usage(char* image_name, int opts_parse_result);

/**
 * Destroy options instance.
 */
void g_opts_destroy();

#endif