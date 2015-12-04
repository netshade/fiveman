#ifndef FIVEMAN_PROCESS_STATISTICS_H
#define FIVEMAN_PROCESS_STATISTICS_H




#include "fiveman_types.h"

void fiveman_init_sampling(fiveman_process_state * state);
void fiveman_teardown_sampling(fiveman_process_state * state);
void fiveman_sample_info(fiveman_process_state * state, fiveman_process_statistics_sample * previous_sample, fiveman_process_statistics_sample * sample);

#endif
