#include <errno.h>
#include <string.h>
#include <sys/wait.h>
#include <unistd.h>

#include "log.h"
#include "opts.h"
#include "signal.h"

/**
 * Flag as to whether a signal has been received.
 */
volatile int signal_received = 0;

/**
 * The last signal number received.
 */
volatile int last_signal = 0;

/**
 * Simple handler for signals.
 */
void signal_handler(int signum) {
    TRACE("sigterm_handler()");
    INFO("SIGTERM received");
    signal_received = 1;
    last_signal = signum;
}

/**
 * Fork and execute a program.
 * @param pathname program to execute
 * @param argv arguments for the program, null terminated.
 * @return child process pid
 */
pid_t fork_exec(char* pathname, char** argv) {
    TRACEV("fork_exec(%p = \"%s\", %p)", pathname, pathname, argv);

    int exec_result;
    pid_t child_pid = fork();
    if (child_pid != 0) {
        return child_pid;
    }

    exec_result = execv(pathname, argv);
    if (exec_result == -1) {
        FATALV(FATAL_ERROR_EXEC_FAILURE, "Could not exec %s: %s", pathname, strerror(errno));
    }
    // Never reached.
    FATAL(FATAL_ERROR_UNREACHABLE_CODE_REACHED, "execv() returned something other than -1");
}

/**
 * Run configured child process, and send SIGTERM if we are interrupted in any way.
 * @return status of child process as per waitpid()
 */
int run_child_process() {
    int stat;

    TRACE("run_child_process()");

    int child_pid = fork_exec(g_opts->exec_pathname, g_opts->exec_args);
    INFOV("Child PID: %d", child_pid);

    if (waitpid(child_pid, &stat, 0) == -1) {
        INFOV("Waiting on child failed: %s", strerror(errno));
        if (signal_received && last_signal == SIGTERM) {
            INFOV("SIGTERM received, terminating child PID: %d", child_pid);
            kill(child_pid, SIGTERM);
        }
    }

    return stat;
}

/**
 * Check if process exited abnormally
 * @param stat status from waitpid()
 * @return non-zero if, and only if, the process exited abnormally
 */
int is_abnormal_termination(int stat) {
    TRACEV("is_abnormal_termination(%d)", stat);

    if (!WIFEXITED(stat)) {
        return 1;
    }

    if (WEXITSTATUS(stat) != 0) {
        return 1;
    }

    return 0;
}

/**
 * Execute the child process as provided in the CLI options.
 * @return 1 if and only if the child process terminated abnormally.
 */
int g_exec_child_process() {
    int stat;

    TRACE("g_exec_child_process()");

    INFO("Registering SIGTERM...");
    if (signal(SIGTERM, &signal_handler) != 0) {
        FATALV(FATAL_ERROR_SIGNAL_INIT, "Error initializing signal handler: %s", strerror(errno));
    }

    INFOV("Running %s...", g_opts->exec_pathname);
    stat = run_child_process();

    DEBUGV("Child process status: %d", stat);

    if (!is_abnormal_termination(stat)) {
        INFO("Normal termination detected, finished");
        return 0;
    }

    if (g_opts->quiesce_secs > 0) {
        INFOV("Waiting %d seconds to quiesce", g_opts->quiesce_secs);
        sleep(g_opts->quiesce_secs);
    }

    DEBUG("Abmornal termination");
    return 1;
}