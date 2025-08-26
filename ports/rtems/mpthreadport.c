#include <errno.h>
#include <rtems.h>
#include <rtems/thread.h>
#include <stdio.h>
#include <stdlib.h>

#include "py/mpthread.h"
#include "py/runtime.h"
#include "mpthreadport.h"


static rtems_mutex thread_list_mutex;

typedef struct _mp_thread_t {
  rtems_id id; /* RTEMS task ID */
  bool ready;
  void *arg; /* thread-entry args */
  struct _mp_thread_t *next;
  mp_state_thread_t *state; /* pointer to this thread's mp state */
} mp_thread_t;

static mp_thread_t *thread_list = NULL;


void mp_thread_init(void) {
  /* Lower main thread priority so worker threads can preempt it */
  rtems_task_set_priority(RTEMS_SELF, 50, NULL);

  /* Initialize global mutex for thread list */
  rtems_mutex_init(&thread_list_mutex, NULL);

  /* Create first list entry for the main thread */
  mp_thread_t *m = malloc(sizeof(mp_thread_t));
  if (m == NULL) {
    mp_raise_msg(&mp_type_OSError, "malloc failed");
  }

  m->id = rtems_task_self(); /* main thread ID */
  m->ready = true;
  m->arg = NULL;                   /* no thread-entry args for main */
  m->state = &mp_state_ctx.thread; /* main thread uses mp_state_ctx.thread */
  m->next = NULL;
  thread_list = m;
}


void lock_threads_list(void) { rtems_mutex_lock(&thread_list_mutex); }


void unlock_threads_list(void) { rtems_mutex_unlock(&thread_list_mutex); }


mp_uint_t mp_thread_create(void *(*entry)(void *), void *arg,
                           size_t *stack_size) {
  if (*stack_size == 0) {
    *stack_size = RTEMS_MPY_THREAD_TASK_STACK_SIZE;
  }

  /* create RTEMS task */
  rtems_id task_id;

  rtems_status_code sc =
      rtems_task_create(rtems_build_name('M', 'T', 'H', 'D'), /* task name  */
                        1,                                    /* priority   */
                        RTEMS_MPY_THREAD_TASK_STACK_SIZE,     /* stack size */
                        RTEMS_DEFAULT_MODES | RTEMS_TIMESLICE,
                        RTEMS_DEFAULT_ATTRIBUTES, &task_id);
  if (sc != RTEMS_SUCCESSFUL) {
    mp_raise_msg(&mp_type_OSError, "task_create failed");
  }

  /* start task */
  sc = rtems_task_start(task_id, (rtems_task_entry)entry,
                        (rtems_task_argument)arg);
  if (sc != RTEMS_SUCCESSFUL) {
    mp_raise_msg(&mp_type_OSError, "task_start failed");
  }

  /* Add to thread list (lock list while modifying) */
  mp_thread_t *th = malloc(sizeof(mp_thread_t));
  th->id = task_id;
  th->ready = false;
  th->arg = arg;
  th->state = NULL;

  lock_threads_list();

  th->next = thread_list;
  thread_list = th;

  unlock_threads_list();

  /* Add yield here - this function gets called frequently */
  rtems_task_wake_after(0);

  return (mp_uint_t)task_id;
}


void mp_thread_start(void) {
  rtems_id self = rtems_task_self();

  lock_threads_list();

  for (mp_thread_t *th = thread_list; th != NULL; th = th->next) {
    if (th->id == self) {
      th->ready = true;
      break;
    }
  }

  unlock_threads_list();
}


void mp_thread_finish(void) {
  rtems_id self = rtems_task_self();

  /* Remove thread entry from list */
  lock_threads_list();

  mp_thread_t *prev = NULL;

  mp_thread_t *th;

  for (th = thread_list; th != NULL; th = th->next) {
    if (th->id == self) {
      if (prev == NULL) {
        thread_list = th->next;
      } else {
        prev->next = th->next;
      }
      free(th);
      break;
    }
    prev = th;
  }

  unlock_threads_list();

  /* terminate RTEMS task */
  rtems_task_exit();
}


mp_state_thread_t *mp_thread_get_state(void) {
  mp_state_thread_t *result = NULL;
  rtems_id self = rtems_task_self();

  lock_threads_list();

  for (mp_thread_t *th = thread_list; th != NULL; th = th->next) {
    if (th->id == self) {
      result = th->state;
      break;
    }
  }

  unlock_threads_list();

  return result;
}


void mp_thread_set_state(mp_state_thread_t *state) {
  rtems_id self = rtems_task_self();

  lock_threads_list();

  for (mp_thread_t *th = thread_list; th != NULL; th = th->next) {
    if (th->id == self) {
      th->state = state;
      break;
    }
  }

  unlock_threads_list();
}

mp_uint_t mp_thread_get_id(void) { return (mp_uint_t)rtems_task_self(); }

void mp_thread_deinit(void) {
  lock_threads_list();

  /* free all nodes */
  mp_thread_t *th = thread_list;

  while (th != NULL) {
    mp_thread_t *next = th->next;
    free(th);
    th = next;
  }

  thread_list = NULL;

  unlock_threads_list();
}


void mp_thread_mutex_init(mp_thread_mutex_t *mutex) {
  rtems_mutex_init(mutex, NULL);
}


void mp_thread_mutex_unlock(mp_thread_mutex_t *mutex) {
  rtems_mutex_unlock(mutex);
}


int mp_thread_mutex_lock(mp_thread_mutex_t *mutex, int wait) {
  int rc = rtems_mutex_try_lock(mutex);

  if (rc == EBUSY) { /* mutex already locked */
    return 0;
  } else if (rc == 0) {
    return 1;
  } else {
    return -1;
  }
}


void mp_thread_recursive_mutex_init(mp_thread_recursive_mutex_t *mutex) {
  rtems_recursive_mutex_init(mutex, NULL);
}


void mp_thread_recursive_mutex_unlock(mp_thread_recursive_mutex_t *mutex) {
  rtems_recursive_mutex_unlock(mutex);
}


int mp_thread_recursive_mutex_lock(mp_thread_recursive_mutex_t *mutex,
                                   int wait) {
  int rc = rtems_recursive_mutex_try_lock(mutex);

  if (rc == EBUSY) { /* mutex already locked */
    return 0;
  } else if (rc == 0) {
    return 1;
  } else {
    return -1;
  }
}
