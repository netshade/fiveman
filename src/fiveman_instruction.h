#ifndef FIVEMAN_INSTRUCTION_H
#define FIVEMAN_INSTRUCTION_H

typedef struct fiveman_instruction {
  const char * name;
  const char * exec;
  struct fiveman_instruction * next;
} fiveman_instruction;

fiveman_instruction * fiveman_instruction_allocate();
void fiveman_instruction_deallocate(fiveman_instruction * instr);

#endif
