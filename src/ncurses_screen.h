#ifndef NCURSES_SCREEN_H
#define NCURSES_SCREEN_H

#include <curses.h>

#include "fiveman_process_state.h"
#include "fiveman_process_statistics.h"

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
  int stdout_size;
  int stderr_size;
  int spacing_size;
} fiveman_ncurses_screen_extents;


void byte_measurement_string(long long measurement_in_bytes, char * buffer, size_t buf_size);
void initialize_screen_entry(fiveman_process_state * state, fiveman_ncurses_screen_entry * screen_entry);
void update_screen_entry(fiveman_process_state * state, fiveman_ncurses_screen_entry * screen_entry);
void draw_screen_entry(fiveman_ncurses_screen_entry * entry, fiveman_ncurses_screen_entry * prev_entry, int row, bool highlighted, fiveman_ncurses_screen_extents * extents, bool force_redraw);
void draw_headers(fiveman_ncurses_screen_extents * extents, int row);
void update_screen_extents(fiveman_ncurses_screen_extents * extents, int maxy, int maxx);
void draw_ui_text(char * procfile, char * directory, int maxy, int maxx);

void setup_screen();
void draw_screen(char * procfile, char * directory, fiveman_process_state * process_state, int total_states);
void teardown_screen();
void suspend_screen();
void resume_screen();

extern bool exit_fiveman;
extern bool exit_fiveman_immediately;



#endif
