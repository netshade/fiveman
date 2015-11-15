#include "fiveman_process_statistics.h"

#include <assert.h>
#include <mach/mach.h>
#include <math.h>
#include <sys/time.h>
#include <unistd.h>

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

  sample->sample_time = now_time;
  sample->total_time = total_time;
  if(previous_sample->sample_time > 0){
    uint64_t total_delta = sample->total_time - previous_sample->total_time;
    uint64_t time_delta = sample->sample_time - previous_sample->sample_time;
    double cpu_usage = (((double)total_delta / (double)time_delta) * 100.0);
    sample->cpu_usage = lround(cpu_usage);
  }
  sample->memory_usage = task_info_data.resident_size;
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
