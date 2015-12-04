#ifndef FIVEMAN_STATUS_THREAD_H
#define FIVEMAN_STATUS_THREAD_H

#include "fiveman_types.h"

fiveman_status_thread * fiveman_status_thread_create(fiveman_process_state * table, int num_entries, char * directory);
void fiveman_status_thread_destroy(fiveman_status_thread * thread);

#endif
