#ifndef FIVEMAN_INSTRUCTION_H
#define FIVEMAN_INSTRUCTION_H

#include "fiveman_types.h"



fiveman_instruction * fiveman_instruction_allocate();
void fiveman_instruction_deallocate(fiveman_instruction * instr);

#endif
