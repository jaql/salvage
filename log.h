#ifndef JETSAM_LOG_H
#define JETSAM_LOG_H

#include <stdlib.h>
#include <stdio.h>

/**
 * Exit codes for the application.
 */
enum ExitCodes {
    EXIT_NORMAL = 0,

    FATAL_ERROR_OPTS_PARSE,
    FATAL_ERROR_HEAP_MALLOC,
    FATAL_ERROR_HEAP_MUTEX_INIT,
    FATAL_ERROR_HTTP_CURL_INIT,
    FATAL_ERROR_SIGNAL_INIT,
    FATAL_ERROR_SIGNAL_EXEC,
    FATAL_ERROR_EXEC_FAILURE,
    FATAL_ERROR_UNREACHABLE_CODE_REACHED
};

#define LOG_LEVEL_FATAL  0
#define LOG_LEVEL_ERROR  1
#define LOG_LEVEL_INFO   2
#define LOG_LEVEL_DEBUG  3
#define LOG_LEVEL_TRACE  4
#define LOG_LEVEL_MEMLOG 5

#define LOG(level, message) \
    fprintf(stderr, "[salvage] %s(%d) %s#%d: " message "\n", #level, LOG_LEVEL_##level, __FILE__, __LINE__)
#define MEMLOG(message) LOG(MEMLOG, message)
#define TRACE(message) LOG(TRACE, message)
#define DEBUG(message) LOG(DEBUG, message)
#define INFO(message) LOG(INFO, message)
#define ERROR(message) LOG(ERROR, message)
#define FATAL(code, message) LOGV(FATAL, #code "(%d): " message, code), exit((code))

#define LOGV(level, message, ...) \
    fprintf(stderr, "[salvage] %s(%d) %s#%d: " message "\n", #level, LOG_LEVEL_##level, __FILE__, __LINE__, __VA_ARGS__)
#define MEMLOGV(message, ...) LOGV(MEMLOG, message, __VA_ARGS__)
#define TRACEV(message, ...) LOGV(TRACE, message, __VA_ARGS__)
#define DEBUGV(message, ...) LOGV(DEBUG, message, __VA_ARGS__)
#define INFOV(message, ...) LOGV(INFO, message, __VA_ARGS__)
#define ERRORV(message, ...) LOGV(ERROR, message, __VA_ARGS__)
#define FATALV(code, message, ...) LOGV(FATAL, #code "(%d): " message, code, __VA_ARGS__), exit((code))

#endif //JETSAM_LOG_H
