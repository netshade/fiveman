#ifndef FIVEMAN_PROCESS_STATE_H
#define FIVEMAN_PROCESS_STATE_H

#include <time.h>
#include <sys/stat.h>
#include <unistd.h>

#include "fiveman_types.h"

fiveman_process_state * fiveman_process_state_allocate(fiveman_instruction * instr, int port);
void fiveman_process_state_deallocate(fiveman_process_state * state);
void fiveman_process_state_initialize(fiveman_process_state * state);
pid_t fiveman_process_state_start(fiveman_process_state * state, char * directory);
pid_t fiveman_process_state_signal(fiveman_process_state * state, int signal);
pid_t fiveman_process_state_stop(fiveman_process_state * state);
const char * fiveman_get_pager();
void fiveman_process_state_child_process_status(fiveman_process_state * state);
void fiveman_process_state_page_file(fiveman_process_state * state, char * file);
int fiveman_process_state_lifetime(fiveman_process_state * state);
void fiveman_process_state_lifetime_str(fiveman_process_state * state, char * buf, size_t buf_size);
void fiveman_process_state_page_stdout(fiveman_process_state * state, fiveman_pager_fork * fork);
void fiveman_process_state_page_stderr(fiveman_process_state * state, fiveman_pager_fork * fork);
void fiveman_process_state_change_intent(fiveman_process_state * state, FIVEMAN_INTENT intent);
int fiveman_process_state_is_alive(fiveman_process_state * state);
void fiveman_process_state_converge_start(fiveman_process_state * state, char * directory);
void fiveman_process_state_converge_signal(fiveman_process_state * state, int signal);
void fiveman_process_state_converge_stop(fiveman_process_state * state);
void fiveman_process_state_converge_none(fiveman_process_state * state);
int fiveman_process_state_stderr_has_new_entries(fiveman_process_state * state);
int fiveman_process_state_stdout_has_new_entries(fiveman_process_state * state);
int fiveman_process_state_status_string(char * buffer, size_t buf_len, int index, fiveman_process_state * state);
void fiveman_process_state_converge(fiveman_process_state * state, char * directory);
void fiveman_process_state_desires_intent(fiveman_process_state * state, FIVEMAN_INTENT intent);
void fiveman_process_state_reflect_desired_intent(fiveman_process_state * state);
void fiveman_process_state_reap_zombie_processes(fiveman_process_state * state);
void fiveman_process_state_sample_process(fiveman_process_state * state);
void fiveman_process_state_clear_sample(fiveman_process_state * state);
void fiveman_process_state_set_sample(fiveman_process_state * state, fiveman_process_statistics_sample * sample);
FIVEMAN_PROCESS_CURRENT_ACTIVITY fiveman_process_state_current_activity(fiveman_process_state * state);

#endif
