#ifndef FIVEMAN_PROCESS_STATE_TABLE_H
#define FIVEMAN_PROCESS_STATE_TABLE_H

#include "fiveman_process_state.h"

fiveman_process_state * fiveman_process_state_table_allocate(fiveman_instruction * instr, int starting_port);
void fiveman_process_state_table_deallocate(fiveman_process_state * state);
void fiveman_process_state_table_initialize(fiveman_process_state * state);
void fiveman_process_state_table_converge(fiveman_process_state * state, char * directory);
void fiveman_process_state_table_signal(fiveman_process_state * state, int signal);
void fiveman_process_state_table_change_intent(fiveman_process_state * state, FIVEMAN_INTENT intent);
void fiveman_process_state_table_reflect_desired_intent(fiveman_process_state * state);
void fiveman_process_state_table_desires_intent(fiveman_process_state * state, FIVEMAN_INTENT intent);
int fiveman_process_state_table_num_alive(fiveman_process_state * state);
void fiveman_process_state_table_mark_as_application_table(fiveman_process_state * table);
void fiveman_process_state_table_reap_zombie_processes(fiveman_process_state * table);

extern fiveman_process_state * application_state_table;

#endif
