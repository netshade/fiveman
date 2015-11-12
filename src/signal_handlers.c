#include "signal_handlers.h"

#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/wait.h>

#include "ncurses_screen.h"

void handle_sigchld(int sig) {
  while (waitpid((pid_t)(-1), 0, WNOHANG) > 0) {}
}

void handle_sigint(int sig){
  teardown_screen();
  exit(1);
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
