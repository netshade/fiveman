#ifndef NCURSES_SCREEN_H
#define NCURSES_SCREEN_H

#include <curses.h>

#include "fiveman_process_state.h"

void setup_screen();
void draw_screen(char * procfile, char * directory, fiveman_process_state * process_state, int total_states);
void teardown_screen();
void suspend_screen();
void resume_screen();

extern bool exit_fiveman;
extern bool exit_fiveman_immediately;

#endif
