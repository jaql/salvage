#ifndef JETSAM_HTTP_H
#define JETSAM_HTTP_H

#include <curl/curl.h>

#include "heap.h"

/**
 * Initialize HTTP subsystem.
 */
void g_http_init();

/**
 * Upload files provided in CLI options.
 * @return number of successfully uploaded files.
 */
int g_http_upload_files();

/**
 * Clean up HTTP subsystem.
 */
void g_http_destroy();

#endif //JETSAM_HTTP_H
