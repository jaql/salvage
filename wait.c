#include <errno.h>
#include <semaphore.h>
#include <string.h>

#include "log.h"
#include "signal.h"

struct Wait {
    sem_t sem;
    int last_signal;
};

struct Wait g_wait_instance;
struct Wait* g_wait = &g_wait_instance;

void wait_signal_handler(int signal) {
    g_wait->last_signal = signal;
    if (sem_post(&g_wait->sem) != 0) {
        FATALV(FATAL_ERROR_SIGNAL_EXEC, "Fatal error posting semaphore: %s", strerror(errno));
    }
}

int g_wait_for_signal(int nsignals, int signals[]) {
    TRACEV("g_wait_for_signals(%d, %p)", nsignals, signals);

    g_wait->last_signal = SIGTERM;

    if (sem_init(&g_wait->sem, 0, 1) != 0) {
        FATALV(FATAL_ERROR_SIGNAL_INIT, "Error initializing semaphore: %s", strerror(errno));
    }

    for (int i = 0; i < nsignals; i++) {
        if (signal(signals[i], &wait_signal_handler) != 0) {
            FATALV(FATAL_ERROR_SIGNAL_INIT, "Error initializing signal handler for signal %d: %s", signals[i], strerror(errno));
        }
    }

    if (sem_wait(&g_wait->sem) != 0) {
        ERRORV("Failed to wait for semaphore: %s", strerror(errno));
    }

    if (sem_destroy(&g_wait->sem) != 0) {
        ERRORV("Failed to destroy semaphore: %s", strerror(errno));
    }

    return g_wait->last_signal;
}
