#include <assert.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include "options.h"
#include "fiveman_instruction.h"
#include "fiveman_pager_fork.h"
#include "fiveman_process_state.h"
#include "fiveman_process_state_table.h"
#include "fiveman_process_statistics.h"
#include "fiveman_status_thread.h"
#include "ncurses_screen.h"
#include "procfile.h"
#include "signal_handlers.h"

char ** environ;

int main(int argc, char ** argv, char ** envp){

  fiveman_pager_fork * fork = fiveman_pager_fork_create();

  environ = envp;

  char * sudo_gid = getenv("SUDO_GID");
  char * sudo_uid = getenv("SUDO_UID");
  if(sudo_gid != NULL){
    gid_t gid = (gid_t) atoi(sudo_gid);
    setegid(gid);
  }
  if(sudo_uid != NULL){
    uid_t uid = (uid_t) atoi(sudo_uid);
    seteuid(uid);
  };

  install_signal_handlers();
  char * directory                   = NULL;
  char * procfile                    = NULL;
  fiveman_command * cmd              = NULL;
  int num_instructions               = 0;
  fiveman_instruction * instructions = NULL;
  fiveman_process_state * pstate     = NULL;
  fiveman_status_thread * thread     = NULL;
  int default_port                   = 5000;
  char * port_env                    = getenv("PORT");
  int port                           = default_port;
  if(port_env != NULL && strlen(port_env) > 0){
    int port_from_env = atoi(port_env);
    if(port_from_env > 0){
      port = port_from_env;
    }
  }

  parse_options(argc, argv, &directory, &procfile, &port, &cmd);
  assert(directory != NULL);
  assert(procfile != NULL);
  assert(cmd != NULL);
  assert(access(directory, R_OK) == 0);
  assert(access(procfile, R_OK) == 0);

  num_instructions = parse_procfile(procfile, &instructions);
  pstate = fiveman_process_state_table_allocate(instructions, port);
  fiveman_process_state_table_initialize(pstate);
  fiveman_process_state_table_mark_as_application_table(pstate);

  thread = fiveman_status_thread_create(pstate, num_instructions, directory);

  setup_screen();
  draw_screen(procfile, directory, pstate, num_instructions, thread, fork);
  teardown_screen();

  fiveman_status_thread_destroy(thread);
  fiveman_process_state_table_deallocate(pstate);
  fiveman_instruction_deallocate(instructions);
  free(directory);
  free(procfile);
  fiveman_pager_fork_destroy(fork);
  return 0;
}
