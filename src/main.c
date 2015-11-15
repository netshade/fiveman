#include <assert.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include "options.h"
#include "fiveman_instruction.h"
#include "fiveman_process_state.h"
#include "fiveman_process_state_table.h"
#include "ncurses_screen.h"
#include "procfile.h"
#include "signal_handlers.h"

int main(int argc, char ** argv){
  install_signal_handlers();

  char * directory                   = NULL;
  char * procfile                    = NULL;
  fiveman_command * cmd              = NULL;
  int num_instructions               = 0;
  fiveman_instruction * instructions = NULL;
  fiveman_process_state * pstate     = NULL;
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

  setup_screen();
  draw_screen(procfile, directory, pstate, num_instructions);
  teardown_screen();

  fiveman_process_state_table_deallocate(pstate);
  fiveman_instruction_deallocate(instructions);
  free(directory);
  free(procfile);
  return 0;
}
