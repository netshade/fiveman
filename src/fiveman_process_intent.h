#ifndef FIVEMAN_PROCESS_INTENT_H
#define FIVEMAN_PROCESS_INTENT_H

typedef enum {
  INTENT_START,
  INTENT_SIG,
  INTENT_STOP,
  INTENT_NONE
} FIVEMAN_INTENT;

typedef struct fiveman_process_intent {
  FIVEMAN_INTENT intent;
  int data;
} fiveman_process_intent;

#endif
