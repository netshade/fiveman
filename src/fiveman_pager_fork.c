#include "fiveman_pager_fork.h"
#include "fiveman_process_state.h"

#include <assert.h>
#include <errno.h>
#include <signal.h>
#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

void fiveman_pager_execute(char * file, fiveman_pager_fork * rsrc){
  pid_t child = fork();
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
    write(rsrc->confirm_write_fd, "1", sizeof(char));
  }
}

void fiveman_pager_fork_main(fiveman_pager_fork * fork){
  int reading_size = 1;
  int reading_file = 0;
  int read_bytes = 0;
  uint32_t num_bytes = 0;
  size_t buffer_size = 65536;
  char buffer[buffer_size];
  size_t zero_size = sizeof(char) * buffer_size;
  bzero(buffer, zero_size);
  int buffer_pointer = 0;
  for(;;){
    int status;
    fd_set fds;
    FD_ZERO(&fds);
    FD_SET(fork->file_read_fd, &fds);
    status = select(fork->file_read_fd + 1, &fds, NULL, NULL, NULL);
    if(status > 0){
      if(reading_size){
        read_bytes = read(fork->file_read_fd, &buffer[buffer_pointer], sizeof(uint32_t));
        if(read_bytes > -1){
          buffer_pointer += read_bytes;
          if(buffer_pointer == sizeof(uint32_t)){
            uint32_t * lptr = (uint32_t *)&buffer[0];
            num_bytes = ntohl(*lptr);
            reading_size = 0;
            reading_file = 1;
            buffer_pointer = 0;
            bzero(buffer, zero_size);
          }
        }
      }
      if(reading_file){
        read_bytes = read(fork->file_read_fd, &buffer[buffer_pointer], num_bytes);
        if(read_bytes > -1){
          buffer_pointer += read_bytes;
          if(buffer_pointer == num_bytes){
            fiveman_pager_execute(buffer, fork);
            reading_size = 1;
            reading_file = 0;
            buffer_pointer = 0;
            bzero(buffer, zero_size);
          }
        }
      }
    }
  }
}

void fiveman_pager_fork_wait_for_confirm(fiveman_pager_fork * fork) {
  for(;;){
    int status;
    fd_set fds;
    FD_ZERO(&fds);
    FD_SET(fork->confirm_read_fd, &fds);
    int read_bytes;
    size_t buffer_size = 4096;
    char buffer[buffer_size];
    status = select(fork->confirm_read_fd + 1, &fds, NULL, NULL, NULL);
    if(status > 0){
      read_bytes = read(fork->confirm_read_fd, buffer, buffer_size);
      if(read_bytes > 0){
        break;
      }
    }
  }
}

fiveman_pager_fork * fiveman_pager_fork_create() {
  fiveman_pager_fork * rsrc = calloc(1, sizeof(fiveman_pager_fork));
  assert(rsrc != NULL);
  int file_filedes[2];
  int confirm_filedes[2];
  pipe(file_filedes);
  pipe(confirm_filedes);
  rsrc->file_read_fd = file_filedes[0];
  rsrc->file_write_fd = file_filedes[1];
  rsrc->confirm_read_fd = confirm_filedes[0];
  rsrc->confirm_write_fd = confirm_filedes[1];
  pid_t pid = fork();
  if(pid == 0){
    close(rsrc->file_write_fd);
    close(rsrc->confirm_read_fd);
    fiveman_pager_fork_main(rsrc);
    exit(0);
    return rsrc;
  } else {
    close(rsrc->file_read_fd);
    close(rsrc->confirm_write_fd);
    rsrc->pid = pid;
    return rsrc;
  }
}

void fiveman_pager_fork_page_file(fiveman_pager_fork * fork, char * file) {
  uint32_t strsize = strlen(file);
  uint32_t netsize = htonl(strsize);
  write(fork->file_write_fd, &netsize, sizeof(uint32_t));
  write(fork->file_write_fd, file, sizeof(char) * strsize);
  fiveman_pager_fork_wait_for_confirm(fork);
}

void fiveman_pager_fork_destroy(fiveman_pager_fork * fork) {
  if(fork->pid > 0){
    kill(fork->pid, SIGTERM);
    close(fork->file_write_fd);
    close(fork->confirm_read_fd);
    free(fork);
  }
}
