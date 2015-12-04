#include "ncurses_screen.h"

#include <math.h>
#include <pthread.h>
#include <signal.h>

#include "fiveman.h"
#include "fiveman_process_state.h"
#include "fiveman_process_state_table.h"


bool exit_fiveman = FALSE;
bool exit_fiveman_immediately = FALSE;

int byte_measurement_string(long long measurement_in_bytes, char * buffer, size_t buf_size) {
  char * suffix = "B";
  double amount = (double) measurement_in_bytes;
  if( (measurement_in_bytes / BYTES_PER_EXABYTE) >= 1 ){
    amount = (double)measurement_in_bytes / (double)BYTES_PER_EXABYTE;
    suffix = "EB";
  } else if ( (measurement_in_bytes / BYTES_PER_PETABYTE) >= 1){
    amount = (double)measurement_in_bytes / (double)BYTES_PER_PETABYTE;
    suffix = "PB";
  } else if ( (measurement_in_bytes / BYTES_PER_TERABYTE) >= 1){
    amount = (double)measurement_in_bytes / (double)BYTES_PER_TERABYTE;
    suffix = "TB";
  } else if ( (measurement_in_bytes / BYTES_PER_GIGABYTE) >= 1){
    amount = (double)measurement_in_bytes / (double)BYTES_PER_GIGABYTE;
    suffix = "GB";
  } else if ( (measurement_in_bytes / BYTES_PER_MEGABYTE) >= 1){
    amount = (double)measurement_in_bytes / (double)BYTES_PER_MEGABYTE;
    suffix = "MB";
  } else if ( (measurement_in_bytes / BYTES_PER_KILOBYTE) >= 1){
    amount = (double)measurement_in_bytes / (double)BYTES_PER_KILOBYTE;
    suffix = "kB";
  }
  amount = round(amount * 100.0) / 100.0;
  return snprintf(buffer, buf_size, "%g%s", amount, suffix);
}

int readwrite_byte_measurement_string(long long read_measurement, long long write_measurement, char * buffer, size_t buf_size){
  int written = byte_measurement_string(read_measurement, buffer, buf_size);
  if(written < buf_size){
    buffer[written]='/';
    written += 1;
    size_t remainder = buf_size - written;
    if(remainder > 0){
      written += byte_measurement_string(write_measurement, &buffer[written], remainder);
    }
  }
  return written;
}

void update_screen_extents(fiveman_ncurses_screen_extents * extents, int maxy, int maxx) {
  int total = maxx;
  extents->spacing_size = 1;

  extents->state_size = 20;
  total -= extents->state_size + extents->spacing_size;

  extents->pid_size = 10;
  total -= extents->pid_size + extents->spacing_size;

  extents->age_size = 30;
  total -= extents->age_size + extents->spacing_size;

  extents->cpu_size = 5;
  total -= extents->cpu_size + extents->spacing_size;

  extents->mem_size = 12;
  total -= extents->mem_size + extents->spacing_size;

  extents->net_size = 12;
  total -= extents->net_size + extents->spacing_size;

  extents->io_size = 12;
  total -= extents->io_size + extents->spacing_size;

  extents->fs_size = 12;
  total -= extents->fs_size + extents->spacing_size;

  extents->stdout_size = 6;
  total -= extents->stdout_size + extents->spacing_size;

  extents->stderr_size = 6;
  total -= extents->stderr_size + extents->spacing_size;

  if(total < 0){
    total = 0;
  }
  extents->name_size = total;
}

void initialize_screen_entry(fiveman_process_state * state, fiveman_ncurses_screen_entry * screen_entry) {
  bzero(screen_entry, sizeof(fiveman_ncurses_screen_entry));
  screen_entry->state = state;
  update_screen_entry(state, screen_entry);
}

void update_screen_entry(fiveman_process_state * state, fiveman_ncurses_screen_entry * screen_entry){
  screen_entry->activity      = fiveman_process_state_current_activity(state);
  screen_entry->pid           = state->pid;
  screen_entry->age           = fiveman_process_state_lifetime(state);
  screen_entry->stdout_active = fiveman_process_state_stdout_has_new_entries(state);
  screen_entry->stderr_active = fiveman_process_state_stderr_has_new_entries(state);

  memcpy(&screen_entry->sample, &state->sample, sizeof(fiveman_process_statistics_sample));
}

void draw_headers(fiveman_ncurses_screen_extents * extents, int row) {
  int col_head = 0;
  clrtoeol();

  move(row, col_head);
  addstr("PROCESS");
  col_head += extents->name_size + extents->spacing_size;

  move(row, col_head);
  addstr("PID");
  col_head += extents->pid_size + extents->spacing_size;

  move(row, col_head);
  addstr("STATE");
  col_head += extents->state_size + extents->spacing_size;

  move(row, col_head);
  addstr("AGE");
  col_head += extents->age_size + extents->spacing_size;

  move(row, col_head);
  addstr("CPU");
  col_head += extents->cpu_size + extents->spacing_size;

  move(row, col_head);
  addstr("MEM");
  col_head += extents->mem_size + extents->spacing_size;

  move(row, col_head);
  addstr("NET R/W");
  col_head += extents->net_size + extents->spacing_size;

  move(row, col_head);
  addstr("IO R/W");
  col_head += extents->io_size + extents->spacing_size;

  move(row, col_head);
  addstr("FS R/W");
  col_head += extents->fs_size + extents->spacing_size;

  move(row, col_head);
  addstr("STDOUT");
  col_head += extents->stdout_size + extents->spacing_size;

  move(row, col_head);
  addstr("STDERR");
  col_head += extents->stderr_size + extents->spacing_size;

}

void draw_screen_entry(fiveman_ncurses_screen_entry * entry, fiveman_ncurses_screen_entry * prev_entry, int row, bool highlighted, fiveman_ncurses_screen_extents * extents, bool force_redraw){
  char name_str[extents->name_size + 1];
  bzero(name_str, extents->name_size + 1);
  char pid_str[extents->pid_size + 1];
  bzero(pid_str, extents->pid_size + 1);
  char state_str[extents->state_size + 1];
  bzero(state_str, extents->state_size + 1);
  char age_str[extents->age_size + 1];
  bzero(age_str, extents->age_size + 1);
  char cpu_str[extents->cpu_size + 1];
  bzero(cpu_str, extents->cpu_size + 1);
  char mem_str[extents->mem_size + 1];
  bzero(mem_str, extents->mem_size + 1);
  char net_str[extents->net_size + 1];
  bzero(net_str, extents->net_size + 1);
  char io_str[extents->io_size + 1];
  bzero(io_str, extents->io_size + 1);
  char fs_str[extents->io_size + 1];
  bzero(fs_str, extents->fs_size + 1);
  char stdout_str[extents->stdout_size + 1];
  bzero(stdout_str, extents->stdout_size + 1);
  char stderr_str[extents->stderr_size + 1];
  bzero(stderr_str, extents->stderr_size + 1);

  snprintf(name_str, extents->name_size, "%s: %s", entry->state->instruction->name, entry->state->instruction->exec);
  switch(fiveman_process_state_current_activity(entry->state)){
  case FIVEMAN_PROCESS_STOPPED:
    snprintf(state_str, extents->state_size, "STOPPED");
    break;
  case FIVEMAN_PROCESS_RUNNING:
    snprintf(state_str, extents->state_size, "ACTIVE");
    break;
  case FIVEMAN_PROCESS_STARTING_UP:
    snprintf(state_str, extents->state_size, "LAUNCHING");
    break;
  case FIVEMAN_PROCESS_SHUTTING_DOWN:
    snprintf(state_str, extents->state_size, "STOPPING");
    break;
  case FIVEMAN_PROCESS_UNKNOWN:
    snprintf(state_str, extents->state_size, "UNKNOWN");
    break;
  }
  snprintf(pid_str, extents->pid_size, "%i", entry->pid);
  fiveman_process_state_lifetime_str(entry->state, age_str, extents->age_size);
  snprintf(cpu_str, extents->cpu_size, "%i%%", entry->sample.cpu_usage);
  byte_measurement_string(entry->sample.memory_usage, mem_str, extents->mem_size);
  readwrite_byte_measurement_string(entry->sample.io_read_rate, entry->sample.io_write_rate, io_str, extents->io_size);
  readwrite_byte_measurement_string(entry->sample.net_read_rate, entry->sample.net_write_rate, net_str, extents->net_size);
  readwrite_byte_measurement_string(entry->sample.fs_read_rate, entry->sample.fs_write_rate, fs_str, extents->fs_size);

  if(entry->stdout_active){
    snprintf(stdout_str, extents->stdout_size, "*");
  } else {
    snprintf(stdout_str, extents->stdout_size, "");
  }
  if(entry->stderr_active){
    snprintf(stderr_str, extents->stderr_size, "*");
  } else {
    snprintf(stderr_str, extents->stderr_size, "");
  }

  int col_head = 0;
  move(row, col_head);
  clrtoeol();

  if(highlighted){
    attron(A_BOLD);
  }

  move(row, col_head);
  addstr(name_str);
  col_head += extents->name_size + extents->spacing_size;

  move(row, col_head);
  addstr(pid_str);
  col_head += extents->pid_size + extents->spacing_size;

  move(row, col_head);
  addstr(state_str);
  col_head += extents->state_size + extents->spacing_size;

  move(row, col_head);
  addstr(age_str);
  col_head += extents->age_size + extents->spacing_size;

  move(row, col_head);
  addstr(cpu_str);
  col_head += extents->cpu_size + extents->spacing_size;

  move(row, col_head);
  addstr(mem_str);
  col_head += extents->mem_size + extents->spacing_size;

  move(row, col_head);
  addstr(net_str);
  col_head += extents->net_size + extents->spacing_size;

  move(row, col_head);
  addstr(io_str);
  col_head += extents->io_size + extents->spacing_size;

  move(row, col_head);
  addstr(fs_str);
  col_head += extents->fs_size + extents->spacing_size;

  move(row, col_head);
  addstr(stdout_str);
  col_head += extents->stdout_size + extents->spacing_size;

  move(row, col_head);
  addstr(stderr_str);
  col_head += extents->stderr_size + extents->spacing_size;

  if(highlighted){
    attroff(A_BOLD);
  }
}

void setup_screen(){
  exit_fiveman = FALSE;
  exit_fiveman_immediately = FALSE;
  initscr();
  keypad(stdscr, TRUE);
  nonl();
  nocbreak();
  noecho();
  raw();
  //nodelay(stdscr, TRUE);
  halfdelay(10);
  curs_set(FALSE);
}

void draw_ui_text(char * procfile, char * directory, int maxy, int maxx) {
  move(maxy - 1, 0);
  printw(BOTTOM_BAR_TEXT);
  move(0, 0);
  printw("Procfile: %s Directory: %s", procfile, directory);
}

void draw_screen(char * procfile, char * directory, fiveman_process_state * process_state, int total_states, fiveman_status_thread * thread_info, fiveman_pager_fork * pager_fork){
  fiveman_process_state * state = process_state;
  int header_row                = 1;
  int entry_start_row           = 2;
  int inum                      = entry_start_row;
  int highlighted_row           = 0;
  int cur_row                   = 0;
  int max_row                   = 0;
  int max_col                   = 0;
  int prev_max_row              = 0;
  int prev_max_col              = 0;
  int term_timeout              = 100;
  bool is_terminating           = FALSE;
  bool redraw                   = FALSE;

  fiveman_ncurses_screen_extents extents;
  time_t term_issued_at;

  getmaxyx(stdscr, max_row, max_col);
  update_screen_extents(&extents, max_row, max_col);

  fiveman_process_state * states[total_states];
  while(state != NULL){
    states[cur_row] = state;
    state = state->next;
    cur_row ++;
  }
  state = process_state;
  cur_row = 0;

  draw_ui_text(procfile, directory, max_row, max_col);
  draw_headers(&extents, header_row);
  for(;;){
    prev_max_row = max_row;
    prev_max_col = max_col;
    getmaxyx(stdscr, max_row, max_col);
    redraw = max_row != prev_max_row || max_col != prev_max_col;
    if(redraw){
      update_screen_extents(&extents, max_row, max_col);
      clear();
      draw_headers(&extents, header_row);
      draw_ui_text(procfile, directory, max_row, max_col);
    }
    cur_row = 0;
    for(int i = 0; i < thread_info->num_entries; i++){
      pthread_mutex_lock(&thread_info->mutex);
      draw_screen_entry(thread_info->cur_entries + i, thread_info->prev_entries + i, inum, highlighted_row == cur_row, &extents, redraw);
      pthread_mutex_unlock(&thread_info->mutex);

      cur_row ++;
      inum ++;
    }
    inum     = entry_start_row;
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
      pthread_mutex_lock(&thread_info->mutex);
      fiveman_process_state_desires_intent(states[highlighted_row], INTENT_STOP);
      pthread_mutex_unlock(&thread_info->mutex);
      break;
    case 'l':
      pthread_mutex_lock(&thread_info->mutex);
      fiveman_process_state_desires_intent(states[highlighted_row], INTENT_START);
      pthread_mutex_unlock(&thread_info->mutex);
      break;
    case 'o':
      suspend_screen();
      fiveman_process_state_page_stdout(states[highlighted_row], pager_fork);
      resume_screen();
      break;
    case 'e':
      suspend_screen();
      fiveman_process_state_page_stderr(states[highlighted_row], pager_fork);
      resume_screen();
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
      pthread_mutex_lock(&thread_info->mutex);
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
      pthread_mutex_unlock(&thread_info->mutex);
    }
    if(exit_fiveman_immediately){
      break;
    }
    if(exit_fiveman){
      pthread_mutex_lock(&thread_info->mutex);
      time(&term_issued_at);
      fiveman_process_state_table_desires_intent(process_state, INTENT_STOP);
      exit_fiveman   = FALSE;
      is_terminating = TRUE;
      pthread_mutex_unlock(&thread_info->mutex);
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
