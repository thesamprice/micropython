#ifndef INCLUDED_MPTHREADPORT_H
#define INCLUDED_MPTHREADPORT_H

#include <rtems/thread.h>

#define RTEMS_MPY_THREAD_TASK_STACK_SIZE (4 * 1024)

typedef rtems_mutex mp_thread_mutex_t;
typedef rtems_recursive_mutex mp_thread_recursive_mutex_t;

void mp_thread_init(void);
void mp_thread_deinit(void);

#endif /* INCLUDED_MPTHREADPORT_H */
