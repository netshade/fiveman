#ifndef FIVEMAN_PAGER_FORK_H
#define FIVEMAN_PAGER_FORK_H

#include "fiveman_types.h"

fiveman_pager_fork * fiveman_pager_fork_create();
void fiveman_pager_fork_page_file(fiveman_pager_fork * fork, char * file);
void fiveman_pager_fork_destroy(fiveman_pager_fork * fork);
void fiveman_pager_fork_wait_for_confirm(fiveman_pager_fork * fork);

#endif
