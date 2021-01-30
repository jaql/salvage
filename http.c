#include <curl/curl.h>
#include <errno.h>
#include <string.h>
#include <sys/stat.h>

#include "heap.h"
#include "http.h"
#include "log.h"
#include "opts.h"

/**
 * Upload succeeded.
 */
#define UPLOAD_SUCCESS 0

/**
 * Upload failed unrecoverably.
 */
#define UPLOAD_UNRECOVERABLE_FAILURE 1

/**
 * Upload failed recoverably.
 */
#define UPLOAD_RECOVERABLE_FAILURE 2

/**
 * Buffer size for URLs.
 */
#define MAX_URL_LENGTH 2048

/**
 * CURL instance.
 */
CURL* curl = NULL;

/**
 * List of CURL error codes that are unrecoverable, terminated by CURLE_OK
 */
CURLcode unrecoverable_curl_errors[] = {
        CURLE_UNSUPPORTED_PROTOCOL,
        CURLE_FAILED_INIT,
        CURLE_URL_MALFORMAT,
        CURLE_NOT_BUILT_IN,
        CURLE_QUOTE_ERROR,
        CURLE_HTTP_RETURNED_ERROR,
        CURLE_WRITE_ERROR,
        CURLE_OUT_OF_MEMORY,
        CURLE_FUNCTION_NOT_FOUND,
        CURLE_BAD_FUNCTION_ARGUMENT,
        CURLE_UNKNOWN_OPTION,
        CURLE_SSL_ENGINE_NOTFOUND,
        CURLE_SSL_ENGINE_SETFAILED,
        CURLE_SSL_CIPHER,
        CURLE_PEER_FAILED_VERIFICATION,
        CURLE_USE_SSL_FAILED,
        CURLE_SSL_ENGINE_INITFAILED,
        CURLE_REMOTE_DISK_FULL,
        CURLE_CONV_FAILED,
        CURLE_CONV_REQD,
        CURLE_SSL_CACERT_BADFILE,
        CURLE_SSH,
        CURLE_SSL_CRL_BADFILE,
        CURLE_SSL_ISSUER_ERROR,
        CURLE_NO_CONNECTION_AVAILABLE,
        CURLE_SSL_PINNEDPUBKEYNOTMATCH,
        CURLE_SSL_INVALIDCERTSTATUS,
        CURLE_RECURSIVE_API_CALL,

        CURLE_OK // List terminator
};

/**
 * Returns whether a CURL error is unrecoverable.
 * @param res CURL error
 * @return 1 if and only if the error is unrecoverable.
 */
int is_unrecoverable_curl_error(CURLcode res) {
    for (int i = 0; unrecoverable_curl_errors[i] != CURLE_OK; i++) {
        if (res == unrecoverable_curl_errors[i]) {
            return 1;
        }
    }

    return 0;
}

/**
 * curl malloc implementation using the heap.
 */
void* curl_malloc_callback_fn(size_t size) {
    void* result;
    MEMLOGV("curl_malloc_callback_fn(%ld)", size);
    return g_heap_emulate_malloc(size);
}

/**
 * curl realloc implementation using the heap.
 */
void* curl_realloc_callback_fn(void* ptr, size_t size) {
    void* result;
    MEMLOGV("curl_realloc_callback_fn(%p, %ld)", ptr, size);
    return g_heap_emulate_realloc(ptr, size);
}

/**
 * curl free implementation using the heap.
 */
void curl_free_callback_fn(void* ptr) {
    MEMLOGV("curl_free_callback_fn(%p)", ptr);
    g_heap_emulate_free(ptr);
}

/**
 * curl strdup implementation using the heap.
 */
char* curl_strdup_callback_fn(const char* str) {
    char* result;
    MEMLOGV("curl_strdup_callback_fn(%p = \"%s\")", str, str);
    result = g_heap_allocate(strlen(str));
    strcpy(result, str);
    return result;
}

/**
 * curl calloc implementation using the heap.
 */
void* curl_calloc_callback_fn(size_t nmemb, size_t size) {
    void* result;
    MEMLOGV("curl_calloc_callback_fn(%ld, %ld)", nmemb, size);
    result = g_heap_emulate_calloc(nmemb, size);
    return result;
}

void g_http_init() {
    #define G_HTTP_INIT_SET_CURL_OPTION(option, value)                                        \
        curl_code = curl_easy_setopt(curl, (option), (value));                              \
        if (curl_code != CURLE_OK) {                                                          \
           FATALV(FATAL_ERROR_HTTP_CURL_INIT, "failed curl_easy_setopt(%s, %s) with code %d", \
                #option, #value, curl_code);                                                  \
        }

    CURLcode curl_code;
    struct curl_slist* headers = NULL;

    TRACE("g_http_init()");

    curl_code = curl_global_init_mem(0,
                                     &curl_malloc_callback_fn,
                                     &curl_free_callback_fn,
                                     &curl_realloc_callback_fn,
                                     &curl_strdup_callback_fn,
                                     &curl_calloc_callback_fn);
    if (curl_code != CURLE_OK) {
        FATALV(FATAL_ERROR_HTTP_CURL_INIT, "failed CURL global init with code %d", curl_code);
    }

    curl = curl_easy_init();
    if (curl == NULL) {
        FATAL(FATAL_ERROR_HTTP_CURL_INIT, "failed CURL easy init");
    }
    TRACEV("created CURL %p", curl);

    for (int i = 0; i < g_opts->nheaders; i++) {
        headers = curl_slist_append(headers, g_opts->headers[i]);
        if (headers == NULL) {
            FATALV(FATAL_ERROR_HTTP_CURL_INIT, "failed adding header: %s", g_opts->headers[i]);
        }
        INFOV("added header %s", g_opts->headers[i]);
    }

    if (g_opts->certificate != NULL) {
        G_HTTP_INIT_SET_CURL_OPTION(CURLOPT_PINNEDPUBLICKEY, g_opts->certificate);
        DEBUGV("Pinned public key: %s", g_opts->certificate);
    }

    G_HTTP_INIT_SET_CURL_OPTION(CURLOPT_HTTPHEADER, headers);
    TRACE("Set headers");

    G_HTTP_INIT_SET_CURL_OPTION(CURLOPT_VERBOSE, 1L);
    TRACE("Set verbose");

    if (headers != NULL) {
        curl_slist_free_all(headers);
        TRACE("Freed headers");
    }
}

/**
 * Upload a file to the configured location.
 * @param filename file to upload
 * @return UPLOAD_SUCCESS, UPLOAD_UNRECOVERABLE_FAILURE, UPLOAD_RECOVERABLE_FAILURE
 */
int http_upload(char* filename) {
    #define G_HTTP_UPLOAD_SET_CURL_OPTION(option, value)                        \
        curl_code = curl_easy_setopt(curl, (option), (value));                  \
        if (curl_code != CURLE_OK) {                                            \
           ERRORV("failed curl_easy_setopt(%s, %s) with code %d",               \
                #option, #value, curl_code);                                    \
           return UPLOAD_UNRECOVERABLE_FAILURE;                                 \
        }

    char full_url[MAX_URL_LENGTH];
    FILE* fd;
    CURLcode curl_code;
    struct stat fd_stat;

    TRACEV("http_upload(%p = \"%s\")", filename, filename);

    if (snprintf(full_url, MAX_URL_LENGTH,"%s/%s", g_opts->url, filename) == MAX_URL_LENGTH) {
        ERRORV("%s/%s is too long an URL, max URL size is %d", g_opts->url, filename, MAX_URL_LENGTH);
        return UPLOAD_UNRECOVERABLE_FAILURE;
    }
    INFOV("Uploading %s to %s", filename, full_url);

    fd = fopen(filename, "rb");
    if (fd == NULL) {
        ERRORV("%s could not be opened: %s", filename, strerror(errno));
        return UPLOAD_UNRECOVERABLE_FAILURE;
    }
    TRACE("File opened");

    if (fstat(fileno(fd), &fd_stat) != 0) {
        ERRORV("%s could not be examined for size: %s", filename, strerror(errno));

        if (fclose(fd) != 0) {
            ERRORV("%s could not be closed on stat failure: %s", filename, strerror(errno));
        }

        return UPLOAD_UNRECOVERABLE_FAILURE;
    }
    DEBUGV("File size: %d", fd_stat.st_size);

    G_HTTP_UPLOAD_SET_CURL_OPTION(CURLOPT_URL, full_url);
    G_HTTP_UPLOAD_SET_CURL_OPTION(CURLOPT_UPLOAD, 1L);
    G_HTTP_UPLOAD_SET_CURL_OPTION(CURLOPT_READDATA, fd);
    G_HTTP_UPLOAD_SET_CURL_OPTION(CURLOPT_INFILESIZE_LARGE,(curl_off_t)fd_stat.st_size);

    curl_code = curl_easy_perform(curl);
    if (curl_code != CURLE_OK) {
        ERRORV("Upload failed: %s", curl_easy_strerror(curl_code));
    }
    DEBUGV("curl result: %d", curl_code);

    if (is_unrecoverable_curl_error(curl_code)) {
        ERROR("curl result was unrecoverable");
        return UPLOAD_UNRECOVERABLE_FAILURE;
    }

    return curl_code == CURLE_OK ? UPLOAD_SUCCESS : UPLOAD_RECOVERABLE_FAILURE;
}

int g_http_upload_files() {
    char* file;
    int upload_result;
    int ndone = 0;
    int nuploaded = 0;
    struct {
        int done;
        int attempts;
    } uploads[g_opts->nfiles];

    TRACE("g_http_upload_files()");

    bzero(uploads, sizeof(uploads));

    while (ndone < g_opts->nfiles) {
        DEBUGV("%d/%d files done", ndone, g_opts->nfiles);

        for (int i = 0; i < g_opts->nfiles; i++) {
            file = g_opts->files[i];

            if (uploads[i].done) {
                TRACEV("%s done, skipping", g_opts->files[i]);
                continue;
            }

            uploads[i].attempts++;

            INFOV("Attempt %d/%d of upload of file %s", uploads[i].attempts, g_opts->max_attempts, file);
            upload_result = http_upload(file);

            if (upload_result == UPLOAD_RECOVERABLE_FAILURE && uploads[i].attempts < g_opts->max_attempts) {
                ERRORV("Recoverable error encountered uploading %s, trying again", file);
                continue;
            }

            if (upload_result == UPLOAD_RECOVERABLE_FAILURE && uploads[i].attempts == g_opts->max_attempts) {
                ERRORV("Recoverable error encountered uploading %s, attempts exhausted so not trying again", file);
                uploads[i].done = 1;
                ndone++;
                continue;
            }

            if (upload_result == UPLOAD_UNRECOVERABLE_FAILURE) {
                ERRORV("Unrecoverable error encountered uploading %s", file);
            }

            if (upload_result == UPLOAD_SUCCESS) {
                INFOV("Success uploading %s", file);
                nuploaded++;
            }

            INFOV("Done uploading %s", file);
            uploads[i].done = 1;
            ndone++;
        }
    }

    INFOV("%d files uploaded", ndone);
    return nuploaded;
}

void g_http_destroy() {
    TRACE("g_http_destroy()");
    curl_easy_cleanup(curl);
    curl_global_cleanup();
}
