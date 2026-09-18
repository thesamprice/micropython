/* SPDX-License-Identifier: BSD-2-Clause */

/*
 * Copyright (C) 2025 Sameer Srivastava <sam33r012@gmail.com>
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 
 * Redistributions of source code must retain the above copyright
 * notice, this list of conditions and the following disclaimer.
 * Redistributions in binary form must reproduce the above copyright
 * notice, this list of conditions and the following disclaimer in the
 * documentation and/or other materials provided with the distribution.
 * 
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT OWNER OR CONTRIBUTORS BE
 * LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 */

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


/* Static, so this can run before the heap exists -- which it must, since it
 * has to run before anything reaches MP_STATE_THREAD. */
static mp_thread_t thread_entry0;

void mp_thread_init(void) {
  rtems_mutex_init(&thread_list_mutex, NULL);

  /* The main thread's state is mp_state_ctx.thread, which is what
   * mp_thread_is_main_thread() compares mp_thread_get_state() against. */
  thread_entry0.id = rtems_task_self();
  thread_entry0.ready = true;
  thread_entry0.arg = NULL;
  thread_entry0.state = &mp_state_ctx.thread;
  thread_entry0.next = NULL;

  thread_list = &thread_entry0;
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
                        150,                                  /* priority   */
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
