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
  uint64_t io_read_rate;
  uint64_t io_write_rate;
  uint64_t net_read_rate;
  uint64_t net_write_rate;
  uint64_t fs_read_rate;
  uint64_t fs_write_rate;
} fiveman_process_statistics_sample;

void fiveman_init_sampling(pid_t pid);
void fiveman_teardown_sampling(pid_t pid);
void fiveman_sampling_sleep(int last_slept);

void fiveman_sample_info(fiveman_process_statistics_sample * previous_sample, fiveman_process_statistics_sample * sample);

pid_t fiveman_sampling_fork();

#endif
