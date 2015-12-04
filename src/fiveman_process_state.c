#include <assert.h>
#include <errno.h>
#include <fcntl.h>
#include <signal.h>
#include <spawn.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "fiveman.h"
#include "fiveman_instruction.h"
#include "fiveman_pager_fork.h"
#include "fiveman_process_intent.h"
#include "fiveman_process_state.h"
#include "fiveman_process_state_table.h"
#include "fiveman_process_statistics.h"
#include "ncurses_screen.h"
#include "signal_handlers.h"

fiveman_process_state * fiveman_process_state_allocate(fiveman_instruction * instr, int port){
  fiveman_process_state * state = calloc(1, sizeof(fiveman_process_state));
  assert(state != NULL);

  state->intent.intent = INTENT_NONE;
  state->desired_port = port;
  state->instruction = instr;
  state->sample_ctxt = NULL;
  return state;
}

void fiveman_process_state_deallocate(fiveman_process_state * state){
  fiveman_process_state * next_ptr = NULL;
  while(state != NULL){
    next_ptr = state->next;
    fiveman_teardown_sampling(state);
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

  fiveman_process_state_clear_sample(state);
}



pid_t fiveman_process_state_start(fiveman_process_state * state, char * directory){
  fiveman_process_state_clear_sample(state);

  const char * existing_path = getenv("PATH");
  const char * path_assignment = "PATH=";
  const size_t new_path_buf_size = strlen(path_assignment) + strlen(existing_path) + strlen(directory) + 2;
  char new_path_buf[new_path_buf_size];
  snprintf(new_path_buf, new_path_buf_size, "%s%s:%s", path_assignment, directory, existing_path);

    const char * port_assignment = "PORT=";
  const size_t new_port_buf_size = strlen(port_assignment) + 128;
  char new_port_buf[new_port_buf_size];
  snprintf(new_port_buf, new_port_buf_size, "%s%i", port_assignment, state->desired_port);

  char * argv[] = { "/bin/sh", "-c", (char *) state->instruction->exec, NULL };

  int esize = 0;
  char * c = environ[0];
  for( int i = 0; c != NULL; c = environ[i++]){
    esize = i;
  }
  char ** new_environ = calloc(esize + 2, sizeof(char *));
  memcpy(new_environ, environ, esize * sizeof(char *));
  new_environ[esize - 1] = new_path_buf;
  new_environ[esize] = new_port_buf;

  pid_t subchild;
  sigset_t reset;
  sigaddset(&reset, SIGCHLD);
  sigaddset(&reset, SIGINT);
  sigaddset(&reset, SIGTERM);
  sigaddset(&reset, SIGPIPE);
  posix_spawnattr_t attr;
  posix_spawn_file_actions_t file_actions;
  posix_spawnattr_init(&attr);
  posix_spawn_file_actions_init(&file_actions);
  posix_spawnattr_setflags(&attr, POSIX_SPAWN_CLOEXEC_DEFAULT | POSIX_SPAWN_SETSIGDEF | POSIX_SPAWN_RESETIDS | POSIX_SPAWN_SETPGROUP);
  posix_spawnattr_setsigdefault(&attr, &reset);
  posix_spawnattr_setpgroup(&attr, 0);
  posix_spawn_file_actions_addopen(&file_actions, STDOUT_FILENO, state->stdout, O_WRONLY | O_APPEND | O_CREAT, 0755);
  posix_spawn_file_actions_addopen(&file_actions, STDERR_FILENO, state->stderr, O_WRONLY | O_APPEND | O_CREAT, 0755);
  posix_spawn_file_actions_addopen(&file_actions, STDIN_FILENO, "/dev/null", O_RDONLY, 0755);

  int posix_status = posix_spawn(&subchild, "/bin/sh", &file_actions, &attr, argv, new_environ);
  assert(posix_status == 0);
  posix_spawn_file_actions_destroy(&file_actions);
  posix_spawnattr_destroy(&attr);

  fiveman_teardown_sampling(state);

  state->pid = subchild;
  fiveman_init_sampling(state);

  return subchild;
}

pid_t fiveman_process_state_signal(fiveman_process_state * state, int signal){
  if(state->pid > 0){
    killpg(state->pid, signal);
  }
  return state->pid;
}

void fiveman_process_state_child_process_status(fiveman_process_state * state) {
  if(fiveman_process_state_is_alive(state)){
    fiveman_process_state_sample_process(state);
  }

}

pid_t fiveman_process_state_stop(fiveman_process_state * state){
  return fiveman_process_state_signal(state, SIGTERM);
}

const char * fiveman_default_pager = "/usr/bin/more";

const char * fiveman_get_pager(){
  const char * env_pager     = getenv("PAGER");
  const char * pager         = fiveman_default_pager;
  if(env_pager != NULL && strlen(env_pager) > 0){
    pager = env_pager;
  }
  return pager;
}

void fiveman_process_state_page_file(fiveman_process_state * state, char * file){
  suspend_screen();

  const char * pager = fiveman_get_pager();
  char * argv[] = { (char *)pager, file, NULL };

  pid_t subchild;
  sigset_t reset;
  sigaddset(&reset, SIGCHLD);
  sigaddset(&reset, SIGINT);
  sigaddset(&reset, SIGTERM);
  sigaddset(&reset, SIGPIPE);
  posix_spawnattr_t attr;
  posix_spawn_file_actions_t file_actions;
  posix_spawnattr_init(&attr);
  posix_spawn_file_actions_init(&file_actions);
  posix_spawnattr_setflags(&attr, POSIX_SPAWN_CLOEXEC_DEFAULT | POSIX_SPAWN_RESETIDS);
  posix_spawnattr_setsigdefault(&attr, &reset);
  posix_spawn_file_actions_addinherit_np(&file_actions, STDOUT_FILENO);
  posix_spawn_file_actions_addinherit_np(&file_actions, STDERR_FILENO);
  posix_spawn_file_actions_addinherit_np(&file_actions, STDIN_FILENO);

  int posix_status = posix_spawnp(&subchild, pager, &file_actions, &attr, argv, environ);
  assert(posix_status == 0);
  posix_spawn_file_actions_destroy(&file_actions);
  posix_spawnattr_destroy(&attr);

  tcsetpgrp(STDIN_FILENO, subchild);
  tcsetpgrp(STDOUT_FILENO, subchild);
  tcsetpgrp(STDERR_FILENO, subchild);
  int status;
  waitpid(subchild, &status, WNOHANG);
  printf("Resuming scren!\n");

  resume_screen();
}

int fiveman_process_state_lifetime(fiveman_process_state * state){
  time_t cur;
  time(&cur);
  int distance = cur - state->last_state_change;
  return distance;
}

void fiveman_process_state_lifetime_str(fiveman_process_state * state, char * buf, size_t buf_size){
  int distance = fiveman_process_state_lifetime(state);
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

void fiveman_process_state_page_stdout(fiveman_process_state * state, fiveman_pager_fork * fork){
  fiveman_pager_fork_page_file(fork, state->stdout);
  state->stdout_paged_at = state->stdout_stat.st_mtime;
}

void fiveman_process_state_page_stderr(fiveman_process_state * state, fiveman_pager_fork * fork){
  fiveman_pager_fork_page_file(fork, state->stderr);
  state->stderr_paged_at = state->stderr_stat.st_mtime;
}

void fiveman_process_state_change_intent(fiveman_process_state * state, FIVEMAN_INTENT intent){
  state->intent.intent = intent;
}

int fiveman_process_state_is_alive(fiveman_process_state * state){
  if(state->pid > 0){
    return (kill(state->pid, 0) == 0);
  } else {
    return 0;
  }
}

void fiveman_process_state_converge_start(fiveman_process_state * state, char * directory){
  if(state->pid > 0){
    if(fiveman_process_state_is_alive(state)){ // Process running
      // Do nothing, already started
    } else {
      fiveman_process_state_start(state, directory);
      time(&state->last_state_change);
    }
  } else {
    // Start process, never been started before
    fiveman_process_state_start(state, directory);
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
    if(fiveman_process_state_is_alive(state)){
      title = ACTIVE_STATUS_TITLE;
    } else {
      title = STOPPED_STATUS_TITLE;
    }
  } else {
    title = INACTIVE_STATUS_TITLE;
  }
  fiveman_process_state_lifetime_str(state, age_state_buf, age_state_buf_len);
  if(fiveman_process_state_stdout_has_new_entries(state)){
    stdout_info = "[stdout]";
  }
  if(fiveman_process_state_stderr_has_new_entries(state)){
    stderr_info = "[stderr]";
  }
  return snprintf(buffer, buf_len, "%s (%i for %s) %i. %s: %s %i%% %s %s", title, pid, age_state_buf, index, state->instruction->name, state->instruction->exec, state->sample.cpu_usage, stdout_info, stderr_info);
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

void fiveman_process_state_desires_intent(fiveman_process_state * state, FIVEMAN_INTENT intent){
  state->desired_intent.intent = intent;
}

void fiveman_process_state_reflect_desired_intent(fiveman_process_state * state){
  state->intent.intent = state->desired_intent.intent;
}

void fiveman_process_state_reap_zombie_processes(fiveman_process_state * state) {
  if(state->pid > 0){
    while (waitpid(state->pid, 0, WNOHANG) > 0) {}
  }
}

void fiveman_process_state_sample_process(fiveman_process_state * state) {
  time_t now;
  time(&now);
  if( (now - state->sample.sample_time) >= 1){
    fiveman_process_statistics_sample new_sample;
    bzero(&new_sample, sizeof(fiveman_process_statistics_sample));
    fiveman_sample_info(state, &state->sample, &new_sample);
    fiveman_process_state_set_sample(state, &new_sample);
  }
}

void fiveman_process_state_clear_sample(fiveman_process_state * state) {
  bzero(&state->sample, sizeof(fiveman_process_statistics_sample));
}

void fiveman_process_state_set_sample(fiveman_process_state * state, fiveman_process_statistics_sample * sample){
  memcpy(&state->sample, sample, sizeof(fiveman_process_statistics_sample));
}

FIVEMAN_PROCESS_CURRENT_ACTIVITY fiveman_process_state_current_activity(fiveman_process_state * state) {
  if(state->intent.intent == INTENT_START){
    if(fiveman_process_state_is_alive(state)){
      return FIVEMAN_PROCESS_RUNNING;
    } else {
      return FIVEMAN_PROCESS_STARTING_UP;
    }
  } else if(state->intent.intent == INTENT_STOP){
    if(fiveman_process_state_is_alive(state)){
      return FIVEMAN_PROCESS_SHUTTING_DOWN;
    } else {
      return FIVEMAN_PROCESS_STOPPED;
    }
  } else {
    return FIVEMAN_PROCESS_UNKNOWN;
  }
}
