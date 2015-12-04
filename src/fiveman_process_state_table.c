#include "fiveman_process_state_table.h"

fiveman_process_state * application_state_table = NULL;

fiveman_process_state * fiveman_process_state_table_allocate(fiveman_instruction * instr, int starting_port){
  fiveman_process_state * head       = NULL;
  fiveman_process_state * cur_state  = NULL;
  fiveman_process_state * prev_state = NULL;
  fiveman_instruction * cur_instr    = instr;
  int port                           = starting_port;

  while(cur_instr != NULL){
    cur_state = fiveman_process_state_allocate(cur_instr, port);
    if(head == NULL){
      head = cur_state;
    }
    if(prev_state != NULL){
      prev_state->next = cur_state;
    }
    prev_state = cur_state;
    cur_instr = cur_instr->next;
    port ++;
  }

  return head;
}

void fiveman_process_state_table_deallocate(fiveman_process_state * state){
  fiveman_process_state_deallocate(state);
}

void fiveman_process_state_table_initialize(fiveman_process_state * state){
  fiveman_process_state * cur_state = state;
  while(cur_state != NULL){
    fiveman_process_state_initialize(cur_state);
    cur_state = cur_state->next;
  }
}

void fiveman_process_state_table_converge(fiveman_process_state * state, char * directory){
  fiveman_process_state * cur_state = state;
  while(cur_state != NULL){
    fiveman_process_state_converge(cur_state, directory);
    cur_state = cur_state->next;
  }
}

void fiveman_process_state_table_change_intent(fiveman_process_state * state, FIVEMAN_INTENT intent){
  fiveman_process_state * cur_state = state;
  while(cur_state != NULL){
    fiveman_process_state_change_intent(cur_state, intent);
    cur_state = cur_state->next;
  }
}

void fiveman_process_state_table_reflect_desired_intent(fiveman_process_state * state) {
  fiveman_process_state * cur_state = state;
  while(cur_state != NULL){
    fiveman_process_state_reflect_desired_intent(cur_state);
    cur_state = cur_state->next;
  }
}

void fiveman_process_state_table_desires_intent(fiveman_process_state * state, FIVEMAN_INTENT intent) {
  fiveman_process_state * cur_state = state;
  while(cur_state != NULL){
    fiveman_process_state_desires_intent(cur_state, intent);
    cur_state = cur_state->next;
  }
}

void fiveman_process_state_table_signal(fiveman_process_state * state, int signal) {
  fiveman_process_state * cur_state = state;
  while(cur_state != NULL){
    fiveman_process_state_signal(cur_state, signal);
    cur_state = cur_state->next;
  }
}

int fiveman_process_state_table_num_alive(fiveman_process_state * state){
  int n = 0;
  fiveman_process_state * cur_state = state;
  while(cur_state != NULL){
    if(fiveman_process_state_is_alive(cur_state)){
      n++;
    }
    cur_state = cur_state->next;
  }
  return n;
}

void fiveman_process_state_table_mark_as_application_table(fiveman_process_state * table) {
  application_state_table = table;
}

void fiveman_process_state_table_reap_zombie_processes(fiveman_process_state * table){
  fiveman_process_state * cur_state = table;
  while(cur_state != NULL){
    fiveman_process_state_reap_zombie_processes(cur_state);
    cur_state = cur_state->next;
  }
}
