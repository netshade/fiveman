#ifndef FIVEMAN_PROCESS_STATE_TABLE_H
#define FIVEMAN_PROCESS_STATE_TABLE_H

#include "fiveman_process_state.h"

fiveman_process_state * fiveman_process_state_table_allocate(fiveman_instruction * instr);
void fiveman_process_state_table_deallocate(fiveman_process_state * state);
void fiveman_process_state_table_initialize(fiveman_process_state * state);
void fiveman_process_state_table_converge(fiveman_process_state * state, char * directory);
void fiveman_process_state_table_signal(fiveman_process_state * state, int signal);
void fiveman_process_state_table_change_intent(fiveman_process_state * state, FIVEMAN_INTENT intent);
int fiveman_process_state_table_num_alive(fiveman_process_state * state);

#endif
