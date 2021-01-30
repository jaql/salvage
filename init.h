//
// Created by dbyard on 03/12/2020.
//

#ifndef JETSAM_INIT_H
#define JETSAM_INIT_H

/**
 * Initialize all global subsystems.
 * @param argc program argument count
 * @param argv program arguments
 */
void g_init(int argc, char* argv[]);

/**
 * Destroy all global subsystems.
 */
void g_destroy();

#endif //JETSAM_INIT_H
