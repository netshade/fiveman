#include "fiveman_instruction.h"

#include <assert.h>
#include <stdlib.h>

fiveman_instruction * fiveman_instruction_allocate(){
  fiveman_instruction * ret = calloc(1, sizeof(fiveman_instruction));
  assert(ret != NULL);
  ret->next = NULL;
  return ret;
}

void fiveman_instruction_deallocate(fiveman_instruction * instr){
  fiveman_instruction * next_ptr = NULL;
  while(instr != NULL){
    next_ptr = instr->next;
    free(instr);
    instr = next_ptr;
  }
}
