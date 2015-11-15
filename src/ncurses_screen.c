#include "ncurses_screen.h"

#include <signal.h>

#include "fiveman.h"
#include "fiveman_process_state.h"
#include "fiveman_process_state_table.h"


bool exit_fiveman = FALSE;
bool exit_fiveman_immediately = FALSE;

void setup_screen(){
  exit_fiveman = FALSE;
  exit_fiveman_immediately = FALSE;
  initscr();
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
  int term_timeout              = 100;
  bool is_terminating           = FALSE;

  time_t term_issued_at;

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
    fiveman_process_state_table_reap_zombie_processes(process_state);
    fiveman_process_state_table_converge(process_state, directory);
    cur_row = 0;
    while(state != NULL){
      fiveman_process_state_child_process_status(state);
      states[cur_row] = state;
      fiveman_process_state_status_string(buffer, buffer_len, inum, state);
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

    switch(c){
    case 'q':
      exit_fiveman = TRUE;
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
    if(is_terminating){
      if(fiveman_process_state_table_num_alive(process_state) > 0){
        time_t current;
        time(&current);
        int diff = current - term_issued_at;
        exit_fiveman_immediately = diff >= term_timeout;
        if(exit_fiveman_immediately){
          fiveman_process_state_table_signal(process_state, SIGKILL);
        } else {
          fiveman_process_state_table_signal(process_state, SIGTERM);
        }
      } else {
        exit_fiveman_immediately = TRUE;
      }
    }
    if(exit_fiveman_immediately){
      break;
    }
    if(exit_fiveman){
      time(&term_issued_at);
      fiveman_process_state_table_change_intent(process_state, INTENT_STOP);
      exit_fiveman   = FALSE;
      is_terminating = TRUE;
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
