#ifndef NCURSES_SCREEN_H
#define NCURSES_SCREEN_H

#include "fiveman_process_state.h"

void setup_screen();
void draw_screen(char * procfile, char * directory, fiveman_process_state * process_state, int total_states);
void teardown_screen();
void suspend_screen();
void resume_screen();

#endif
