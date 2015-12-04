
#include <assert.h>
#include <pthread.h>
#include <stdio.h>
#include <strings.h>
#include <unistd.h>

#include "fiveman_process_state.h"
#include "fiveman_process_state_table.h"
#include "fiveman_status_thread.h"
#include "ncurses_screen.h"


void * fiveman_update_thread_main(void * arg){
  fiveman_status_thread * thread_info = (fiveman_status_thread *) arg;
  fiveman_process_state * state = thread_info->table;
  fiveman_ncurses_screen_entry cur_draw_entries[thread_info->num_entries];
  fiveman_ncurses_screen_entry prev_draw_entries[thread_info->num_entries];
  size_t entry_size = sizeof(fiveman_ncurses_screen_entry) * thread_info->num_entries;
  bzero(cur_draw_entries, entry_size);
  bzero(prev_draw_entries, entry_size);
  int cur_row = 0;
  while(state != NULL){
    initialize_screen_entry(state, &cur_draw_entries[cur_row]);
    initialize_screen_entry(state, &prev_draw_entries[cur_row]);
    state = state->next;
    cur_row ++;
  }
  for(;;){
    pthread_mutex_lock(&thread_info->mutex);
    fiveman_process_state_table_reflect_desired_intent(thread_info->table);
    pthread_mutex_unlock(&thread_info->mutex);
    fiveman_process_state_table_reap_zombie_processes(thread_info->table);
    fiveman_process_state_table_converge(thread_info->table, thread_info->directory);
    memcpy(prev_draw_entries, cur_draw_entries, entry_size);
    state = thread_info->table;
    cur_row = 0;
    while(state != NULL){
      fiveman_process_state_child_process_status(state);
      update_screen_entry(state, &cur_draw_entries[cur_row]);
      state = state->next;
      cur_row ++;
    }
    pthread_mutex_lock(&thread_info->mutex);
    memcpy(thread_info->cur_entries, cur_draw_entries, entry_size);
    int exit = thread_info->exit;
    pthread_mutex_unlock(&thread_info->mutex);
    if(exit == 1){
      break;
    }
    sleep(1);
  }
  pthread_exit(NULL);
  return NULL;
}

fiveman_status_thread * fiveman_status_thread_create(fiveman_process_state * table, int num_entries, char * directory) {
  fiveman_status_thread * thread_info = calloc(1, sizeof(fiveman_status_thread));
  assert(thread_info != NULL);
  thread_info->num_entries = num_entries;
  thread_info->cur_entries = calloc(num_entries, sizeof(fiveman_ncurses_screen_entry));
  assert(thread_info->cur_entries != NULL);
  thread_info->prev_entries = calloc(num_entries, sizeof(fiveman_ncurses_screen_entry));
  assert(thread_info->prev_entries != NULL);
  fiveman_process_state * state = table;
  int cur_row = 0;
  while(state != NULL){
    initialize_screen_entry(state, thread_info->cur_entries + cur_row);
    initialize_screen_entry(state, thread_info->prev_entries + cur_row);
    state = state->next;
    cur_row ++;
  }
  thread_info->directory = directory;
  thread_info->table = table;
  thread_info->exit = 0;
  assert(pthread_attr_init(&thread_info->thread_attr) == 0);
  assert(pthread_mutexattr_init(&thread_info->mutex_attr) == 0);
  assert(pthread_mutex_init(&thread_info->mutex, &thread_info->mutex_attr) == 0);
  int status = pthread_create(&thread_info->thread, &thread_info->thread_attr, fiveman_update_thread_main, thread_info);
  assert(status == 0);
  return thread_info;
}

void fiveman_status_thread_destroy(fiveman_status_thread * thread){
  pthread_mutex_lock(&thread->mutex);
  thread->exit = 1;
  pthread_mutex_unlock(&thread->mutex);
  pthread_join(thread->thread, NULL);
  pthread_attr_destroy(&thread->thread_attr);
  pthread_mutex_destroy(&thread->mutex);
  pthread_mutexattr_destroy(&thread->mutex_attr);
  free(thread->cur_entries);
  free(thread->prev_entries);
  free(thread);
}
