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

static io_counter fs;
static io_counter net;
static io_counter dev;

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

static dtrace_hdl_t *dtp = NULL;
struct ps_prochandle * proc = NULL;

static int dorec(const dtrace_probedata_t *data, const dtrace_recdesc_t *rec, void *arg)
{
	dtrace_actkind_t act;
  io_record * addr;
	if (rec == NULL) return (DTRACE_CONSUME_NEXT);
	act = rec->dtrd_action;
	addr = (io_record *)data->dtpda_data;
  if(act == DTRACEACT_DIFEXPR && rec->dtrd_size > 0){ // a trace w/ actual values
    io_counter * counter = NULL;
    switch(addr->type){
    case IO_RECORD_DEV_READ:
      counter = &dev;
      counter->read += addr->block_size;
      break;
    case IO_RECORD_DEV_WRITE:
      counter = &dev;
      counter->write += addr->block_size;
      break;
    case IO_RECORD_FS_READ:
      counter = &fs;
      counter->read += addr->block_size;
      break;
    case IO_RECORD_FS_WRITE:
      counter = &fs;
      counter->write += addr->block_size;
      break;
    case IO_RECORD_NET_READ:
      counter = &net;
      counter->read += addr->block_size;
      break;
    case IO_RECORD_NET_WRITE:
      counter = &net;
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
          | set_opt(dtp, "bufsize", "4m")
          | set_opt(dtp, "aggsize", "512k")
          | set_opt(dtp, "specsize", "512k")
          | set_opt(dtp, "aggrate", "3000msec")
          | set_opt(dtp, "switchrate", "10hz")
          | set_opt(dtp, "arch", "x86_64"));
}

void fiveman_init_sampling(pid_t pid){

  int err;
  dtrace_proginfo_t info;

  dtrace_prog_t *prog;

  dtp = dtrace_open(DTRACE_VERSION, DTRACE_O_LP64, &err);
  assert(dtp != NULL);

  err = set_opts(dtp);
  assert(err != -1);

  char pid_str[1024];
  bzero(pid_str, sizeof(char) * 1024);
  snprintf(pid_str, 1024, "%i", pid);

  proc = dtrace_proc_grab(dtp, pid, 0);
  assert(proc != NULL);

  char * args[2] = {
    "/bin/sh",
    pid_str
  };

  prog = dtrace_program_strcompile(dtp, progstr, DTRACE_PROBESPEC_NAME, 0, 2, args);
  if(prog == NULL){
    printf("ERROR2: bad program: %s\n", dtrace_errmsg(NULL,err));
  }
  assert(prog != NULL);

  err = dtrace_program_exec(dtp, prog, &info);
  assert(err != -1);

  err = dtrace_go(dtp);
  assert(err != -1);

  dtrace_proc_continue(dtp, proc);

}

void fiveman_teardown_sampling(pid_t pid) {
  dtrace_proc_continue(dtp, proc);
  dtrace_proc_release(dtp, proc);
  dtrace_stop(dtp);
  dtrace_close(dtp);
}


void fiveman_sampling_sleep(int last_slept){
  time_t now;
  time(&now);
  int delta = now - last_slept;
  dtrace_sleep(dtp);
  if(delta < 1){
    sleep(1);
  }
}

// Most of this is shamefully taken from Chrome

#define TIME_VALUE_TO_TIMEVAL(a, r) do {  \
		(r)->tv_sec = (a)->seconds;							\
		(r)->tv_usec = (a)->microseconds;				\
	} while (0)

const int kMicrosecondsPerSecond = 1000000;

static task_t fiveman_sampled_task = MACH_PORT_NULL;

uint64_t TimeValToMicroseconds(const struct timeval * tv) {
  return tv->tv_sec * kMicrosecondsPerSecond + tv->tv_usec;
}


void fiveman_sample_info(fiveman_process_statistics_sample * previous_sample, fiveman_process_statistics_sample * sample) {
  int status = dtrace_status(dtp);
  if (status == DTRACE_STATUS_OKAY) {
    status = dtrace_work(dtp, NULL, NULL, dorec, NULL);
  } else if (status != DTRACE_STATUS_NONE) {
    // no -op
  }

  assert(fiveman_sampled_task != MACH_PORT_NULL);
  struct task_basic_info_64 task_info_data;
  struct task_thread_times_info thread_info_data;
  mach_msg_type_number_t thread_info_count = TASK_THREAD_TIMES_INFO_COUNT;
  mach_msg_type_number_t size = sizeof(task_info_data);
  kern_return_t kr = task_info(fiveman_sampled_task, TASK_BASIC_INFO_64, (task_info_t) &task_info_data, &size);
  assert(kr == KERN_SUCCESS);
   kr = task_info(fiveman_sampled_task, TASK_THREAD_TIMES_INFO, (task_info_t) &thread_info_data, &thread_info_count);
  assert(kr == KERN_SUCCESS);

  struct timeval user_timeval, system_timeval, task_timeval;
	TIME_VALUE_TO_TIMEVAL(&thread_info_data.user_time, &user_timeval);
	TIME_VALUE_TO_TIMEVAL(&thread_info_data.system_time, &system_timeval);
	timeradd(&user_timeval, &system_timeval, &task_timeval);

	// ... task info contains terminated time.
	TIME_VALUE_TO_TIMEVAL(&task_info_data.user_time, &user_timeval);
	TIME_VALUE_TO_TIMEVAL(&task_info_data.system_time, &system_timeval);
	timeradd(&user_timeval, &task_timeval, &task_timeval);
	timeradd(&system_timeval, &task_timeval, &task_timeval);

  struct timeval now;
  gettimeofday(&now, NULL);

  uint64_t now_time = TimeValToMicroseconds(&now);
  uint64_t total_time = TimeValToMicroseconds(&task_timeval);
  uint64_t time_delta = now_time - previous_sample->sample_time;
  uint32_t time_delta_in_seconds = time_delta / kMicrosecondsPerSecond;
  if(time_delta_in_seconds == 0){
    time_delta_in_seconds = 1;
  }

  sample->sample_time = now_time;
  sample->total_time = total_time;
  if(previous_sample->sample_time > 0){
    uint64_t total_delta = sample->total_time - previous_sample->total_time;
    double cpu_usage = (((double)total_delta / (double)time_delta) * 100.0);
    sample->cpu_usage = lround(cpu_usage);
  }
  sample->memory_usage = task_info_data.resident_size;


  sample->io_read_rate = dev.read / time_delta_in_seconds;
  sample->io_write_rate = dev.write / time_delta_in_seconds;
  sample->net_read_rate = net.read / time_delta_in_seconds;
  sample->net_write_rate = net.write / time_delta_in_seconds;
  sample->fs_read_rate = fs.read / time_delta_in_seconds;
  sample->fs_write_rate = fs.write / time_delta_in_seconds;

  net.read = 0;
  net.write = 0;
  dev.read = 0;
  dev.write = 0;
  fs.read = 0;
  fs.write = 0;
}


// except this, taken from:
// http://www.foldr.org/~michaelw/log/computers/macosx/task-info-fun-with-mach
#define CHECK_MACH_ERROR(err, msg)																			\
		if (err != KERN_SUCCESS) {																					\
				mach_error (msg, err);																					\
				return -1;																											\
		}



static int setup_recv_port (mach_port_t *recv_port) {
    kern_return_t       err;
    mach_port_t         port = MACH_PORT_NULL;
    err = mach_port_allocate (mach_task_self (),
                              MACH_PORT_RIGHT_RECEIVE, &port);
    CHECK_MACH_ERROR (err, "mach_port_allocate failed:");

    err = mach_port_insert_right (mach_task_self (),
                                  port,
                                  port,
                                  MACH_MSG_TYPE_MAKE_SEND);
    CHECK_MACH_ERROR (err, "mach_port_insert_right failed:");

    *recv_port = port;
    return 0;
}

static int send_port (mach_port_t remote_port, mach_port_t port) {
    kern_return_t       err;

    struct {
        mach_msg_header_t          header;
        mach_msg_body_t            body;
        mach_msg_port_descriptor_t task_port;
    } msg;

    msg.header.msgh_remote_port = remote_port;
    msg.header.msgh_local_port = MACH_PORT_NULL;
    msg.header.msgh_bits = MACH_MSGH_BITS (MACH_MSG_TYPE_COPY_SEND, 0) |
        MACH_MSGH_BITS_COMPLEX;
    msg.header.msgh_size = sizeof msg;

    msg.body.msgh_descriptor_count = 1;
    msg.task_port.name = port;
    msg.task_port.disposition = MACH_MSG_TYPE_COPY_SEND;
    msg.task_port.type = MACH_MSG_PORT_DESCRIPTOR;

    err = mach_msg_send (&msg.header);
    CHECK_MACH_ERROR (err, "mach_msg_send failed:");

    return 0;
}

static int recv_port (mach_port_t recv_port, mach_port_t *port) {
    kern_return_t       err;
    struct {
        mach_msg_header_t          header;
        mach_msg_body_t            body;
        mach_msg_port_descriptor_t task_port;
        mach_msg_trailer_t         trailer;
    } msg;

    err = mach_msg (&msg.header, MACH_RCV_MSG,
                    0, sizeof msg, recv_port,
                    MACH_MSG_TIMEOUT_NONE, MACH_PORT_NULL);
    CHECK_MACH_ERROR (err, "mach_msg failed:");

    *port = msg.task_port.name;
    return 0;
}

pid_t fiveman_sampling_fork(){
    kern_return_t       err;
    mach_port_t         parent_recv_port = MACH_PORT_NULL;
    mach_port_t         child_recv_port = MACH_PORT_NULL;

    if (setup_recv_port (&parent_recv_port) != 0)
        return -1;
    err = task_set_bootstrap_port (mach_task_self (), parent_recv_port);
    CHECK_MACH_ERROR (err, "task_set_bootstrap_port failed:");

    pid_t               pid;
    switch (pid = fork ()) {
    case -1:
        err = mach_port_deallocate (mach_task_self(), parent_recv_port);
        CHECK_MACH_ERROR (err, "mach_port_deallocate failed:");
        return pid;
    case 0: /* child */
        err = task_get_bootstrap_port (mach_task_self (), &parent_recv_port);
        CHECK_MACH_ERROR (err, "task_get_bootstrap_port failed:");
        if (setup_recv_port (&child_recv_port) != 0)
            return -1;
        if (send_port (parent_recv_port, mach_task_self ()) != 0)
            return -1;
        if (send_port (parent_recv_port, child_recv_port) != 0)
            return -1;
        if (recv_port (child_recv_port, &bootstrap_port) != 0)
            return -1;
        err = task_set_bootstrap_port (mach_task_self (), bootstrap_port);
        CHECK_MACH_ERROR (err, "task_set_bootstrap_port failed:");
        break;
    default: /* parent */
        err = task_set_bootstrap_port (mach_task_self (), bootstrap_port);
        CHECK_MACH_ERROR (err, "task_set_bootstrap_port failed:");
        if (recv_port (parent_recv_port, &fiveman_sampled_task) != 0)
            return -1;
        if (recv_port (parent_recv_port, &child_recv_port) != 0)
            return -1;
        if (send_port (child_recv_port, bootstrap_port) != 0)
            return -1;
        err = mach_port_deallocate (mach_task_self(), parent_recv_port);
        CHECK_MACH_ERROR (err, "mach_port_deallocate failed:");
        break;
    }

    return pid;
}
