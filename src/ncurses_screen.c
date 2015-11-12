#include "ncurses_screen.h"

#include <curses.h>

#include "fiveman.h"
#include "fiveman_process_state.h"
#include "fiveman_process_state_table.h"

void setup_screen(){
  WINDOW * w = initscr();
  keypad(stdscr, TRUE);
  nonl();
  nocbreak();
  noecho();
  raw();
  halfdelay(10);
  curs_set(FALSE);
}

void draw_screen(char * procfile, char * directory, fiveman_process_state * process_state, int total_states){
  fiveman_process_state * state = process_state;
  int inum                      = 1;
  const size_t buffer_len       = 8192;
  int highlighted_row           = 0;
  int cur_row                   = 0;
  int max_row                   = 0;
  int max_col                   = 0;

  char buffer[buffer_len];
  fiveman_process_state * states[total_states];
  while(state != NULL){
    states[cur_row] = state;
    state = state->next;
  }
  state = process_state;

  for(;;){
    clear();
    move(max_row, max_col);
    getmaxyx(stdscr, max_row, max_col);
    move(max_row - 1, 0);
    printw(BOTTOM_BAR_TEXT);
    move(0, 0);
    printw("Procfile: %s Directory: %s", procfile, directory);
    fiveman_process_state_table_converge(process_state, directory);
    cur_row = 0;
    while(state != NULL){
      states[cur_row] = state;
      int written = fiveman_process_state_status_string(buffer, buffer_len, inum, state);
      move(inum, 0);
      if(highlighted_row == cur_row){
        attron(A_BOLD);
      }
      addstr(buffer);
      if(highlighted_row == cur_row){
        attroff(A_BOLD);
      }
      state = state->next;
      cur_row ++;
      inum ++;
    }
    inum     = 1;
    state = process_state;
    refresh();
    int c = getch();
    bool is_exit = FALSE;
    switch(c){
    case 'q':
      is_exit = TRUE;
      break;
    case 'r':
      refresh();
      break;
    case 's':
      fiveman_process_state_change_intent(states[highlighted_row], INTENT_STOP);
      break;
    case 'l':
      fiveman_process_state_change_intent(states[highlighted_row], INTENT_START);
      break;
    case 'o':
      fiveman_process_state_page_stdout(states[highlighted_row]);
      break;
    case 'e':
      fiveman_process_state_page_stderr(states[highlighted_row]);
      break;
    case KEY_UP:
      highlighted_row --;
      if(highlighted_row < 0){
        highlighted_row = 0;
      }
      break;
    case KEY_DOWN:
      highlighted_row ++;
      if(highlighted_row >= total_states){
        highlighted_row = total_states - 1;
      }
      if(highlighted_row < 0){
        highlighted_row = 0;
      }
      break;
    case ERR:
      // hit delay
      break;
    }
    if(is_exit){
      break;
    }


  }
}

void teardown_screen(){
  endwin();
}

void suspend_screen(){
  def_prog_mode();		/* Save the tty modes		  */
	endwin(); /* Stop ncurses interaction */
}

void resume_screen(){
  reset_prog_mode();
  refresh();
}
