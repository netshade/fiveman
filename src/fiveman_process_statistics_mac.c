#include "fiveman_process_statistics.h"

#include <assert.h>
#include <dtrace.h>
#include <mach/mach.h>
#include <math.h>
#include <stdint.h>
#include <sys/time.h>
#include <unistd.h>

typedef enum {
  IO_RECORD_DEV_READ  = 1,
  IO_RECORD_DEV_WRITE = 2,
  IO_RECORD_FS_READ   = 3,
  IO_RECORD_FS_WRITE  = 4,
  IO_RECORD_NET_READ  = 5,
  IO_RECORD_NET_WRITE = 6,
  IO_RECORD_UNKNOWN
} IO_RECORD_TYPE;


#define TIME_VALUE_TO_TIMEVAL(a, r) do {  \
		(r)->tv_sec = (a)->seconds;							\
		(r)->tv_usec = (a)->microseconds;				\
	} while (0)

const int kMicrosecondsPerSecond = 1000000;

uint64_t TimeValToMicroseconds(const struct timeval * tv) {
  return tv->tv_sec * kMicrosecondsPerSecond + tv->tv_usec;
}

// Dtrace setup
typedef struct {
  uint32_t block_size;
  uint64_t start;
  uint32_t type;
} io_record;

typedef struct {
  uint64_t start;
  uint64_t read;
  uint64_t write;
} io_counter;

typedef struct {
  io_counter fs;
  io_counter net;
  io_counter dev;
  task_t fiveman_sampled_task;
  dtrace_hdl_t * dtp;
  dtrace_prog_t * prog;
  dtrace_proginfo_t info;
  struct ps_prochandle * proc;
} mac_sampling_data;

// This is the program.
static const char *progstr = "\
typedef struct {\
  uint32_t block_size;\
  uint64_t start;\
  uint32_t type;\
} io_record;\
io_record record_io;\
io_record record_fs;\
io_record record_net;\
io:::done,io:::wait-done /pid == $1/{\
  record_io.block_size = args[0]->b_bcount;\
  record_io.type = (args[0]->b_flags & B_WRITE) ? 2 : 1;\
  record_io.start = timestamp;\
  trace(record_io);\
}\
\
fsinfo:::read / pid == $1 / { \
  record_fs.block_size = arg1;\
  record_fs.type = 3;\
  record_fs.start = timestamp;\
  trace(record_fs);\
}\
fsinfo:::write / pid == $1 / { \
  record_fs.block_size = arg1;\
  record_fs.type = 4;\
  record_fs.start = timestamp;\
  trace(record_fs);\
}\
syscall::read*:entry, syscall::recv*:entry \
/(fds[arg0].fi_fs == \"sockfs\" || fds[arg0].fi_name == \"<socket>\") && pid == $1/\
{\
	self->socket = 1;\
}\
\
syscall::read*:return, syscall::recv*:return /self->socket && pid == $1 && arg0 > 0/\
{\
  record_net.block_size = arg0;\
  record_net.type = 5;\
  record_net.start = timestamp;\
  trace(record_net);\
	self->socket = 0;\
}\
\
syscall::write*:entry, syscall::send*:entry\
/(fds[arg0].fi_fs == \"sockfs\" || fds[arg0].fi_name == \"<socket>\") && pid == $1/\
{\
	self->socket = 1;\
}\
\
syscall::write*:return, syscall::send*:return\
/self->socket && pid == $1 && arg0 > 0/\
{\
  record_net.block_size = arg0;\
  record_net.type = 6;\
  record_net.start = timestamp;\
  trace(record_net);\
	self->socket = 0;\
}\
";



static int dorec(const dtrace_probedata_t *data, const dtrace_recdesc_t *rec, void *arg)
{
  fiveman_process_state * state = (fiveman_process_state *)arg;
  mac_sampling_data * ctxt = (mac_sampling_data *) state->sample_ctxt;

	dtrace_actkind_t act;
  io_record * addr;
	if (rec == NULL) return (DTRACE_CONSUME_NEXT);
	act = rec->dtrd_action;
	addr = (io_record *)data->dtpda_data;
  if(act == DTRACEACT_DIFEXPR && rec->dtrd_size > 0){ // a trace w/ actual values
    io_counter * counter = NULL;
    switch(addr->type){
    case IO_RECORD_DEV_READ:
      counter = &ctxt->dev;
      counter->read += addr->block_size;
      break;
    case IO_RECORD_DEV_WRITE:
      counter = &ctxt->dev;
      counter->write += addr->block_size;
      break;
    case IO_RECORD_FS_READ:
      counter = &ctxt->fs;
      counter->read += addr->block_size;
      break;
    case IO_RECORD_FS_WRITE:
      counter = &ctxt->fs;
      counter->write += addr->block_size;
      break;
    case IO_RECORD_NET_READ:
      counter = &ctxt->net;
      counter->read += addr->block_size;
      break;
    case IO_RECORD_NET_WRITE:
      counter = &ctxt->net;
      counter->write += addr->block_size;
      break;
    }
  }
	if (act == DTRACEACT_EXIT) {
		return (DTRACE_CONSUME_NEXT);
	}
	return (DTRACE_CONSUME_THIS);
}

// set the option, otherwise print an error & return -1
int
set_opt(dtrace_hdl_t *dtp, const char *opt, const char *value)
{
  if (-1 == dtrace_setopt(dtp, opt, value)) {
    fprintf(stderr, "Failed to set '%1$s' to '%2$s'.\n", opt, value);
    return (-1);
  }
  return (0);
}

// set all the options, otherwise return an error
int
set_opts(dtrace_hdl_t *dtp)
{
  return (set_opt(dtp, "strsize", "4096")
          | set_opt(dtp, "bufsize", "64k")
          | set_opt(dtp, "aggsize", "0k")
          | set_opt(dtp, "specsize", "0k")
          | set_opt(dtp, "arch", "x86_64"));
}


void fiveman_init_sampling(fiveman_process_state * state){

  mac_sampling_data * ctxt = (mac_sampling_data *) calloc(1, sizeof(mac_sampling_data));
  assert(ctxt != NULL);

  assert(task_for_pid(mach_task_self(), state->pid, &ctxt->fiveman_sampled_task) == KERN_SUCCESS);

  int err;

  ctxt->dtp = dtrace_open(DTRACE_VERSION, DTRACE_O_LP64, &err);
  assert(ctxt->dtp != NULL);

  err = set_opts(ctxt->dtp);
  assert(err != -1);

  char pid_str[1024];
  bzero(pid_str, sizeof(char) * 1024);
  snprintf(pid_str, 1024, "%i", state->pid);

  ctxt->proc = dtrace_proc_grab(ctxt->dtp, state->pid, 0);
  assert(ctxt->proc != NULL);

  char * args[2] = {
    "/bin/sh",
    pid_str
  };

  ctxt->prog = dtrace_program_strcompile(ctxt->dtp, progstr, DTRACE_PROBESPEC_NAME, 0, 2, args);
  if(ctxt->prog == NULL){
    printf("ERROR2: bad program: %s\n", dtrace_errmsg(NULL,err));
  }
  assert(ctxt->prog != NULL);

  err = dtrace_program_exec(ctxt->dtp, ctxt->prog, &ctxt->info);
  assert(err != -1);

  err = dtrace_go(ctxt->dtp);
  assert(err != -1);

  dtrace_proc_continue(ctxt->dtp, ctxt->proc);

  state->sample_ctxt = ctxt;

}

void fiveman_teardown_sampling(fiveman_process_state * state) {
  mac_sampling_data * ctxt = (mac_sampling_data *)state->sample_ctxt;
  if(ctxt != NULL){
    dtrace_proc_continue(ctxt->dtp, ctxt->proc);
    dtrace_proc_release(ctxt->dtp, ctxt->proc);
    dtrace_stop(ctxt->dtp);
    dtrace_close(ctxt->dtp);

    free(state->sample_ctxt);
  }
  state->sample_ctxt = NULL;
}


void fiveman_sample_info(fiveman_process_state * state, fiveman_process_statistics_sample * previous_sample, fiveman_process_statistics_sample * sample) {
  mac_sampling_data * ctxt = (mac_sampling_data *)state->sample_ctxt;

  int status = dtrace_status(ctxt->dtp);
  if (status == DTRACE_STATUS_OKAY) {
    status = dtrace_work(ctxt->dtp, NULL, NULL, dorec, state);
  } else if (status != DTRACE_STATUS_NONE) {
    // no -op
  }

  struct timeval now;
  gettimeofday(&now, NULL);

  uint64_t now_time = TimeValToMicroseconds(&now);
  uint64_t time_delta = now_time - previous_sample->sample_time;
  uint32_t time_delta_in_seconds = time_delta / kMicrosecondsPerSecond;
  if(time_delta_in_seconds == 0){
    time_delta_in_seconds = 1;
  }

  sample->sample_time = now_time;

  if(ctxt->fiveman_sampled_task != MACH_PORT_NULL){
    struct task_basic_info_64 task_info_data;
    struct task_thread_times_info thread_info_data;
    mach_msg_type_number_t thread_info_count = TASK_THREAD_TIMES_INFO_COUNT;
    mach_msg_type_number_t size = sizeof(task_info_data);
    kern_return_t kr = task_info(ctxt->fiveman_sampled_task, TASK_BASIC_INFO_64, (task_info_t) &task_info_data, &size);
    if(kr == KERN_SUCCESS){
      kr = task_info(ctxt->fiveman_sampled_task, TASK_THREAD_TIMES_INFO, (task_info_t) &thread_info_data, &thread_info_count);
      if(kr == KERN_SUCCESS){


        struct timeval user_timeval, system_timeval, task_timeval;
        TIME_VALUE_TO_TIMEVAL(&thread_info_data.user_time, &user_timeval);
        TIME_VALUE_TO_TIMEVAL(&thread_info_data.system_time, &system_timeval);
        timeradd(&user_timeval, &system_timeval, &task_timeval);

        // ... task info contains terminated time.
        TIME_VALUE_TO_TIMEVAL(&task_info_data.user_time, &user_timeval);
        TIME_VALUE_TO_TIMEVAL(&task_info_data.system_time, &system_timeval);
        timeradd(&user_timeval, &task_timeval, &task_timeval);
        timeradd(&system_timeval, &task_timeval, &task_timeval);

        uint64_t total_time = TimeValToMicroseconds(&task_timeval);
        sample->total_time = total_time;

        if(previous_sample->sample_time > 0){
          uint64_t total_delta = sample->total_time - previous_sample->total_time;
          double cpu_usage = (((double)total_delta / (double)time_delta) * 100.0);
          sample->cpu_usage = lround(cpu_usage);
        }
        sample->memory_usage = task_info_data.resident_size;
      }
    }
  }


  sample->io_read_rate = ctxt->dev.read / time_delta_in_seconds;
  sample->io_write_rate = ctxt->dev.write / time_delta_in_seconds;
  sample->net_read_rate = ctxt->net.read / time_delta_in_seconds;
  sample->net_write_rate = ctxt->net.write / time_delta_in_seconds;
  sample->fs_read_rate = ctxt->fs.read / time_delta_in_seconds;
  sample->fs_write_rate = ctxt->fs.write / time_delta_in_seconds;

  ctxt->net.read = 0;
  ctxt->net.write = 0;
  ctxt->dev.read = 0;
  ctxt->dev.write = 0;
  ctxt->fs.read = 0;
  ctxt->fs.write = 0;
}
