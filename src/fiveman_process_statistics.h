#ifndef FIVEMAN_PROCESS_STATISTICS_H
#define FIVEMAN_PROCESS_STATISTICS_H

#include <stdint.h>
#include <stdlib.h>
#include <time.h>


typedef struct {
  uint64_t sample_time;
  uint64_t total_time;
  uint32_t cpu_usage;
  uint64_t memory_usage;
} fiveman_process_statistics_sample;

void fiveman_sample_info(fiveman_process_statistics_sample * previous_sample, fiveman_process_statistics_sample * sample);
pid_t fiveman_sampling_fork();

#endif
