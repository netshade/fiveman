#ifndef FIVEMAN_TYPES_H
#define FIVEMAN_TYPES_H

#include <pthread.h>
#include <stdint.h>
#include <stdlib.h>
#include <sys/stat.h>
#include <time.h>

typedef enum {
  FIVEMAN_PROCESS_STOPPED = 0,
  FIVEMAN_PROCESS_RUNNING,
  FIVEMAN_PROCESS_STARTING_UP,
  FIVEMAN_PROCESS_SHUTTING_DOWN,
  FIVEMAN_PROCESS_UNKNOWN
} FIVEMAN_PROCESS_CURRENT_ACTIVITY;

typedef struct fiveman_instruction {
  const char * name;
  const char * exec;
  struct fiveman_instruction * next;
} fiveman_instruction;

typedef enum {
  INTENT_START,
  INTENT_SIG,
  INTENT_STOP,
  INTENT_NONE
} FIVEMAN_INTENT;

typedef struct fiveman_process_intent {
  FIVEMAN_INTENT intent;
  int data;
} fiveman_process_intent;

typedef struct {
  uint64_t sample_time;
  uint64_t total_time;
  uint32_t cpu_usage;
  uint64_t memory_usage;
  uint64_t io_read_rate;
  uint64_t io_write_rate;
  uint64_t net_read_rate;
  uint64_t net_write_rate;
  uint64_t fs_read_rate;
  uint64_t fs_write_rate;
} fiveman_process_statistics_sample;

typedef struct fiveman_process_state {
  const fiveman_instruction * instruction;
  fiveman_process_intent intent; // what the process will converge to next
  fiveman_process_intent desired_intent; // what the user desires the process to become
  fiveman_process_statistics_sample sample;
  int desired_port;
  pid_t pid;
  int host_read_fd;
  int child_write_fd;
  time_t last_state_change;
  char * stdout;
  struct stat stdout_stat;
  time_t stdout_paged_at;
  char * stderr;
  struct stat stderr_stat;
  time_t stderr_paged_at;
  void * sample_ctxt;
  struct fiveman_process_state * next;
} fiveman_process_state;

typedef struct {
  fiveman_process_state * state;
  FIVEMAN_PROCESS_CURRENT_ACTIVITY activity;
  pid_t pid;
  int age;
  fiveman_process_statistics_sample sample;
  int stdout_active;
  int stderr_active;
} fiveman_ncurses_screen_entry;

typedef struct {
  int name_size;
  int state_size;
  int pid_size;
  int age_size;
  int cpu_size;
  int mem_size;
  int net_size;
  int io_size;
  int fs_size;
  int stdout_size;
  int stderr_size;
  int spacing_size;
} fiveman_ncurses_screen_extents;

typedef struct {
  const char * name;
} fiveman_command;

typedef struct {
  pthread_t thread;
  pthread_attr_t thread_attr;
  pthread_mutex_t mutex;
  pthread_mutexattr_t mutex_attr;
  int exit;
  fiveman_process_state * table;
  int num_entries;
  fiveman_ncurses_screen_entry * cur_entries;
  fiveman_ncurses_screen_entry * prev_entries;
  char * directory;
} fiveman_status_thread;

typedef struct {
  pid_t pid;
  int file_write_fd;
  int file_read_fd;
  int confirm_write_fd;
  int confirm_read_fd;
} fiveman_pager_fork;

#endif
