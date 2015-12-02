#include "signal_handlers.h"

#include <assert.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/time.h>
#include <sys/wait.h>
#include <unistd.h>

#include "fiveman_process_state.h"
#include "fiveman_process_statistics.h"
#include "fiveman_process_state_table.h"
#include "ncurses_screen.h"


void handle_sigchld(int sig) {
  fiveman_process_state_table_reap_zombie_processes(application_state_table);
}

void handle_sigint(int sig){
  exit_fiveman = TRUE;
}

void handle_childkill(int sig) {
  if(state_in_fork != NULL){
    fiveman_teardown_sampling(state_in_fork->pid);
    kill(state_in_fork->pid, sig);
  }
  exit(0);
}

void reset_signal_handlers(){
  struct sigaction sa;
  sa.sa_handler = SIG_DFL;
  sigemptyset(&sa.sa_mask);
  sa.sa_flags = SA_RESETHAND;
  if (sigaction(SIGCHLD, &sa, 0) == -1) {
    perror(0);
    exit(1);
  }

  struct sigaction intsa;
  intsa.sa_handler = SIG_DFL;
  sigemptyset(&intsa.sa_mask);
  sa.sa_flags = SA_RESETHAND;
  if(sigaction(SIGINT, &intsa, 0) == -1) {
    perror(0);
    exit(1);
  }
}

void install_ignore_sigint_handler(){
  struct sigaction intsa;
  intsa.sa_handler = SIG_IGN;
  sigemptyset(&intsa.sa_mask);
  if(sigaction(SIGINT, &intsa, 0) == -1) {
    perror(0);
    exit(1);
  }
}

void install_signal_handlers(){
  struct sigaction sa;
  sa.sa_handler = &handle_sigchld;
  sigemptyset(&sa.sa_mask);
  sa.sa_flags = SA_RESTART | SA_NOCLDSTOP | SA_NOCLDWAIT;
  if (sigaction(SIGCHLD, &sa, 0) == -1) {
    perror(0);
    exit(1);
  }

  struct sigaction intsa;
  intsa.sa_handler = &handle_sigint;
  sigemptyset(&intsa.sa_mask);
  if(sigaction(SIGINT, &intsa, 0) == -1) {
    perror(0);
    exit(1);
  }
}


void install_child_kill_handler() {
  struct sigaction sa;
  sa.sa_handler = &handle_childkill;
  sigemptyset(&sa.sa_mask);
  if(sigaction(SIGTERM, &sa, 0) == -1) {
    perror(0);
    exit(1);
  }
}

void ignore_sigpipe() {
  signal(SIGPIPE, SIG_IGN);
}

void restore_sigpipe() {
  signal(SIGPIPE, SIG_DFL);
}

void reap_zombie_processes() {
  while (waitpid((pid_t) -1, 0, WNOHANG) > 0) {}
}
