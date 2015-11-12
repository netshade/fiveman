#ifndef PROCFILE_H
#define PROCFILE_H

#include "fiveman_instruction.h"

int parse_procfile(const char * const procfile_path, fiveman_instruction ** instructions);

#endif
