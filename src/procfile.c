#include "procfile.h"

#include <assert.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>

// Caller frees instructions
int parse_procfile(const char * const procfile_path, fiveman_instruction ** instructions){
  FILE * file = fopen(procfile_path, "r");
  assert(file != NULL);

  const int line_size         = 8192;
  char line[line_size];

  char expecting              = ':';
  int total_read              = 0;
  int read                    = 0;
  int parse_head              = 0;
  char * token                = NULL;
  size_t str_len              = 0;
  int found                   = 0;
  fiveman_instruction * head  = NULL;
  fiveman_instruction * cur   = NULL;
  fiveman_instruction * prev  = NULL;

  while(!feof(file)){
    read = fread(line, sizeof(char), line_size, file);
    if(read == -1){
      break;
    }
    parse_head = 0;
    for(int i = 0; i < read; i++){
      if(line[i] == expecting){
        // Consume all leading whitespace
        for(int j = parse_head; j < i; j++){
          if(line[j] != ' ' &&
             line[j] != '\t'){
            parse_head = j;
            break;
          }
        }
        str_len = i - parse_head;
        token = calloc(str_len + 1, sizeof(char));
        assert(token != NULL);
        strncpy(token, &line[parse_head], str_len);
        switch(line[i]){
        case ':':
          cur = fiveman_instruction_allocate();
          cur->name = token;
          if(prev != NULL){
            prev->next = cur;
          }
          if(head == NULL){
            head = cur;
          }
          expecting = '\n';
          break;
        case '\n':
          cur->exec = token;
          prev = cur;
          found ++ ;
          expecting = ':';
          break;
        }
        parse_head = i + 1;
      }
    }
    assert(parse_head > 0);
    total_read += parse_head;
    if(feof(file)){
      break;
    } else {
      assert(fseek(file, total_read, SEEK_SET) == 0);
    }
  }
  if(found > 0){
    assert(head != NULL);
  } else {
    assert(head == NULL);
  }
  *instructions = head;
  return found;
}
