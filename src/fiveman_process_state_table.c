#include "fiveman_process_state_table.h"


fiveman_process_state * fiveman_process_state_table_allocate(fiveman_instruction * instr){
  fiveman_process_state * head       = NULL;
  fiveman_process_state * cur_state  = NULL;
  fiveman_process_state * prev_state = NULL;
  fiveman_instruction * cur_instr    = instr;

  while(cur_instr != NULL){
    cur_state = fiveman_process_state_allocate(cur_instr);
    if(head == NULL){
      head = cur_state;
    }
    if(prev_state != NULL){
      prev_state->next = cur_state;
    }
    prev_state = cur_state;
    cur_instr = cur_instr->next;
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
