//
// Created by dbyard on 08/12/2020.
//

#ifndef JETSAM_WAIT_H
#define JETSAM_WAIT_H

/**
 * Wait for a signal.  Registers temporary signal handlers to do so.
 * @param nsignal number of signals to wait for.
 * @param signals the signals to wait for.
 * #return the encountered signal.
 */
int g_wait_for_signal(int nsignal, int signal[]);

#endif //JETSAM_WAIT_H
