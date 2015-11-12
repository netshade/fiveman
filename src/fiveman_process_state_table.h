#ifndef FIVEMAN_PROCESS_STATE_TABLE_H
#define FIVEMAN_PROCESS_STATE_TABLE_H

#include "fiveman_process_state.h"

fiveman_process_state * fiveman_process_state_table_allocate(fiveman_instruction * instr);
void fiveman_process_state_table_deallocate(fiveman_process_state * state);
void fiveman_process_state_table_initialize(fiveman_process_state * state);
void fiveman_process_state_table_converge(fiveman_process_state * state, char * directory);

#endif
