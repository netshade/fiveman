#ifndef OPTIONS_H
#define OPTIONS_H

#include <getopt.h>

typedef struct {
  const char * name;
} fiveman_command;

static struct option long_options[] = {
  { "procfile", required_argument, NULL, 'f' },
  { "root",     required_argument, NULL, 'd' },
  { "help",     no_argument,       NULL, 'h' },
  { "port",     required_argument, NULL, 'p' },
  { NULL, 0, NULL, 0 }
};

static fiveman_command commands[] = {
  { .name = "start" },
  { .name = NULL }
};

int parse_options(int argc, char ** argv, char ** directory, char ** procfile, int * port, fiveman_command ** cmd);

#endif
