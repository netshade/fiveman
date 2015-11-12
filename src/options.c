#include "options.h"

#include <assert.h>
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "fiveman.h"

// Caller frees directory and procfile
int parse_options(int argc, char ** argv, char ** directory, char ** procfile, int * port, fiveman_command ** cmd){
  int c;
  char * found_directory = NULL,
       * found_procfile  = NULL,
       * found_command   = NULL;
  int found_port         = 0;
  size_t opt_size        = 0;
  int index              = 0;
  while(1){
    int option_index = 0;
    c = getopt_long(argc, argv, "hd:f:p:", long_options, &option_index);
    if(c == -1){
      break;
    }
    switch(c){
    case 'f':
      opt_size = strlen(optarg);
      found_procfile = calloc(opt_size + 1, sizeof(char));
      assert(found_procfile != NULL);
      strncpy(found_procfile, optarg, opt_size);
      break;
    case 'd':
      opt_size = strlen(optarg);
      found_directory = calloc(opt_size + 1, sizeof(char));
      assert(found_directory != NULL);
      strncpy(found_directory, optarg, opt_size);
      break;
    case 'p':
      found_port = atoi(optarg);
      assert(found_port > 0);
      *port = found_port;
      break;
    case 'h':
      exit(0);
    case '?':
      break;
    default:
      exit(-1);
    }
  }
  if(found_directory == NULL){
    found_directory = getwd(NULL);
    assert(found_directory != NULL);
    assert(errno == 0);
  }

  if(found_procfile == NULL){
    size_t path_len = 0;
    int dir_is_root = strncmp(found_directory, DIR_SEPARATOR, strlen(found_directory)) == 0;
    if(dir_is_root){
      path_len = strlen(found_directory) + DEFAULT_FILENAME_LENGTH;
    } else {
      path_len = strlen(found_directory) + DIR_SEPARATOR_LENGTH + DEFAULT_FILENAME_LENGTH;
    }

    found_procfile = calloc(path_len + 1, sizeof(char));
    assert(found_procfile != NULL);
    int written = 0;

    if(dir_is_root){
      written = snprintf(found_procfile, path_len + 1, "%s%s", found_directory, DEFAULT_FILENAME);
    } else {
      written = snprintf(found_procfile, path_len + 1, "%s%s%s", found_directory, "/", DEFAULT_FILENAME);
    }
    assert(written == path_len);
  }

  index = optind;
  if(index >= argc){
    found_command = DEFAULT_COMMAND;
  } else {
    found_command = argv[index];
    index ++;
  }
  fiveman_command * cur_cmd = &commands[0];
  fiveman_command * selected_command = NULL;
  while(cur_cmd->name != NULL){
    if(strncmp(cur_cmd->name, found_command, strlen(found_command)) == 0){
      selected_command = cur_cmd;
      break;
    } else {
      cur_cmd++;
    }
  }
  *directory = found_directory;
  *procfile  = found_procfile;
  *cmd       = selected_command;

  return index;
};
