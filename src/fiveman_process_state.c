#include <assert.h>
#include <errno.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "fiveman.h"
#include "fiveman_instruction.h"
#include "fiveman_process_intent.h"
#include "fiveman_process_state.h"
#include "ncurses_screen.h"
#include "signal_handlers.h"

fiveman_process_state * fiveman_process_state_allocate(fiveman_instruction * instr, int port){
  fiveman_process_state * state = calloc(1, sizeof(fiveman_process_state));
  assert(state != NULL);
  state->intent.intent = INTENT_NONE;
  state->desired_port = port;
  state->instruction = instr;
  return state;
}

void fiveman_process_state_deallocate(fiveman_process_state * state){
  fiveman_process_state * next_ptr = NULL;
  while(state != NULL){
    next_ptr = state->next;
    unlink(state->stdout);
    unlink(state->stderr);
    free(state->stdout);
    free(state->stderr);
    state->stdout = NULL;
    state->stderr = NULL;
    state = next_ptr;
  }
}

void fiveman_process_state_initialize(fiveman_process_state * state){
  state->intent.intent = INTENT_START;

  const size_t buf_len = 8192;
  char tmp_buf[buf_len];
  int written = 0;
  written = snprintf(tmp_buf, buf_len, "/tmp/fiveman-%s-stdout.XXXXX", state->instruction->name);

  state->stdout = calloc(written + 1, sizeof(char));
  assert(state->stdout != NULL);
  strncpy(state->stdout, tmp_buf, written);
  mktemp(state->stdout);
  assert(state->stdout != NULL);

  written = snprintf(tmp_buf, buf_len, "/tmp/fiveman-%s-stderr.XXXXX", state->instruction->name);

  state->stderr = calloc(written + 1, sizeof(char));
  assert(state->stderr != NULL);
  strncpy(state->stderr, tmp_buf, written);
  mktemp(state->stderr);
  assert(state->stderr != NULL);
}

pid_t fiveman_process_state_start(fiveman_process_state * state, char * directory){
  suspend_screen();
  const char * existing_path = getenv("PATH");
  const char * path_assignment = "PATH=";
  const size_t new_path_buf_size = strlen(path_assignment) + strlen(existing_path) + strlen(directory) + 2;
  char new_path_buf[new_path_buf_size];
  snprintf(new_path_buf, new_path_buf_size, "%s%s:%s", path_assignment, directory, existing_path);

  const char * port_assignment = "PORT=";
  const size_t new_port_buf_size = strlen(port_assignment) + 128;
  char new_port_buf[new_port_buf_size];
  snprintf(new_port_buf, new_port_buf_size, "%s%i", port_assignment, state->desired_port);

  pid_t child = fork();
  if(child == 0){ // in child
    assert(setpgid(0,0) == 0);
    reset_signal_handlers();
    assert(freopen(state->stderr, "a", stderr) != NULL);
    assert(freopen(state->stdout, "a", stdout) != NULL);
    assert(freopen("/dev/null", "r", stdin) != NULL);
    assert(putenv(new_path_buf) == 0);
    assert(putenv(new_port_buf) == 0);


    assert(chdir(directory) == 0);
    execl("/bin/sh", "/bin/sh", "-c", state->instruction->exec, NULL);
    // If execution proceeds here, we should print error message and exit immediately
    printf("Couldn't execute instruction %s, %i\n", state->instruction->exec, errno);
    exit(errno);
  } else if(child > 0){ // in parent
    if( setpgid(child, child) != 0) {
      assert(errno == EACCES);
		}
    resume_screen();
    return child;
  } else {
    assert(child >= 0);
    exit(-1);
  }
}

pid_t fiveman_process_state_signal(fiveman_process_state * state, int signal){
  kill(state->pid, signal);
  return state->pid;
}

pid_t fiveman_process_state_stop(fiveman_process_state * state){
  if(state->pid > 0){
    kill(state->pid, SIGTERM);
  }
  return state->pid;
}

const char * fiveman_get_pager(){
  char * env_pager     = getenv("PAGER");
  char * default_pager = "more";
  char * pager         = default_pager;
  if(env_pager != NULL && strlen(env_pager) > 0){
    pager = env_pager;
  }
  return pager;
}

void fiveman_process_state_page_file(fiveman_process_state * state, char * file){
  suspend_screen();
  pid_t child = fork();
  reset_signal_handlers();
  install_ignore_sigint_handler();
  if(child == 0){ // in child, paging process
    const char * pager = fiveman_get_pager();
    execlp(pager, pager, file, NULL);
    printf("Couldn't page file %s, %i", file, errno);
    exit(errno);
  } else if(child > 0){ // in parent
    // Pass control of the TTY to the child process
    tcsetpgrp(STDIN_FILENO, child);
    tcsetpgrp(STDOUT_FILENO, child);
    tcsetpgrp(STDERR_FILENO, child);
    int status;
    waitpid(child, &status, 0);
    reset_signal_handlers();
    install_signal_handlers();
    // reap any waiting children that are now zombies
    handle_sigchld(SIGCHLD);
    resume_screen();
  } else {
    assert(child >= 0);
    exit(-1);
  }
}

void fiveman_process_state_lifetime(fiveman_process_state * state, char * buf, size_t buf_size){
  time_t cur;
  time(&cur);
  int distance = cur - state->last_state_change;
  double ratio;
  char * label;
  if( (distance / SECONDS_PER_YEAR) >= 1 ) {
    ratio = (double)distance / (double)SECONDS_PER_YEAR;
    label = "years";
  } else if( (distance / SECONDS_PER_MONTH) >= 1 ){
    ratio = (double)distance / (double)SECONDS_PER_MONTH;
    label = "months";
  } else if( (distance / SECONDS_PER_DAY) >= 1 ){
    ratio = (double)distance / (double)SECONDS_PER_DAY;
    label = "days";
  } else if( (distance / SECONDS_PER_HOUR) >= 1 ){
    ratio = (double)distance / (double)SECONDS_PER_HOUR;
    label = "hours";
  } else if( (distance / SECONDS_PER_MINUTE) >= 1){
    ratio = (double)distance / (double)SECONDS_PER_MINUTE;
    label = "minutes";
  } else {
    ratio = (double)distance;
    label = "seconds";
  }
  snprintf(buf, buf_size, "%.2f %s", ratio, label);
}

void fiveman_process_state_page_stdout(fiveman_process_state * state){
  fiveman_process_state_page_file(state, state->stdout);
  state->stdout_paged_at = state->stdout_stat.st_mtime;
}

void fiveman_process_state_page_stderr(fiveman_process_state * state){
  fiveman_process_state_page_file(state, state->stderr);
  state->stderr_paged_at = state->stderr_stat.st_mtime;
}

void fiveman_process_state_change_intent(fiveman_process_state * state, FIVEMAN_INTENT intent){
  state->intent.intent = intent;
}

int fiveman_process_state_is_alive(fiveman_process_state * state){
  return (kill(state->pid, 0) == 0);
}

void fiveman_process_state_converge_start(fiveman_process_state * state, char * directory){
  if(state->pid > 0){
    if(fiveman_process_state_is_alive(state)){ // Process running
      // Do nothing, already started
    } else {
      // Start process, had previously stopped and should be started again
      state->pid = fiveman_process_state_start(state, directory);
      time(&state->last_state_change);

    }
  } else {
    // Start process, never been started before
    state->pid = fiveman_process_state_start(state, directory);
    time(&state->last_state_change);
  }
}

void fiveman_process_state_converge_signal(fiveman_process_state * state, int signal){
  // Ephemeral convergence, just send signal if process is running
  //   Send signal, transition to INTENT_NONE
  if(state->pid > 0){
    if(fiveman_process_state_is_alive(state)){
      fiveman_process_state_signal(state, signal);
    }
  }
  state->intent.intent = INTENT_NONE;
}

void fiveman_process_state_converge_stop(fiveman_process_state * state){
  if(state->pid > 0){
    if(fiveman_process_state_is_alive(state)){ // Process alive, send SIGTERM
      fiveman_process_state_stop(state);
      time(&state->last_state_change);
    } else {
      // Process stopped
    }
  } else {
    // Process cannot stop, never started
  }
}

void fiveman_process_state_converge_none(fiveman_process_state * state){
  // no-op
}

int fiveman_process_state_stderr_has_new_entries(fiveman_process_state * state){
  return (state->stderr_paged_at != state->stderr_stat.st_mtime && state->stderr_stat.st_size > 0);
}

int fiveman_process_state_stdout_has_new_entries(fiveman_process_state * state){
  return (state->stdout_paged_at != state->stdout_stat.st_mtime && state->stdout_stat.st_size > 0);
}

int fiveman_process_state_status_string(char * buffer, size_t buf_len, int index, fiveman_process_state * state){
  char * title                   = NULL;
  pid_t pid                      = 0;
  char * stdout_info             = "";
  char * stderr_info             = "";
  const size_t age_state_buf_len = 1024;

  char age_state_buf[age_state_buf_len];

  if(state->pid > 0){
    pid = state->pid;
    if(kill(state->pid, 0) == 0){
      title = ACTIVE_STATUS_TITLE;
    } else {
      title = STOPPED_STATUS_TITLE;
    }
  } else {
    title = INACTIVE_STATUS_TITLE;
  }
  fiveman_process_state_lifetime(state, age_state_buf, age_state_buf_len);
  if(fiveman_process_state_stdout_has_new_entries(state)){
    stdout_info = "[stdout]";
  }
  if(fiveman_process_state_stderr_has_new_entries(state)){
    stderr_info = "[stderr]";
  }
  return snprintf(buffer, buf_len, "%s (%i for %s) %i. %s: %s %s %s", title, pid, age_state_buf, index, state->instruction->name, state->instruction->exec, stdout_info, stderr_info);
}

void fiveman_process_state_converge(fiveman_process_state * state, char * directory){
  if(stat(state->stdout, &state->stdout_stat) != 0){
    assert(errno != EACCES);
  }
  if(stat(state->stderr, &state->stderr_stat) != 0){
    assert(errno != EACCES);
  }
  switch(state->intent.intent){
  case INTENT_START:
    fiveman_process_state_converge_start(state, directory);
    break;
  case INTENT_SIG:
    fiveman_process_state_converge_signal(state, state->intent.data);
    break;
  case INTENT_STOP:
    fiveman_process_state_converge_stop(state);
    break;
  case INTENT_NONE:
    fiveman_process_state_converge_none(state);
    break;
  default:
    exit(-1);
  }
}
