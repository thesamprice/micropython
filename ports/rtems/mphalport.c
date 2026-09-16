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

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <stdbool.h>
#include <time.h>

#include <rtems.h>
#include <unistd.h>

#include "mpconfigport.h"
#include "py/misc.h"
#include "shared/readline/readline.h"

#define BACKSPACE 0x7F
#define ENTER     0x0D

/* Receive single character, blocking until one is available */
int mp_hal_stdin_rx_chr(void) {
  return getc(stdin);
}

/* Send the string of given length */
void mp_hal_stdout_tx_strn(const char *str, int len) {
  write(STDOUT_FILENO, str, len);
}

/* custom readline implementation */
int readline(vstr_t *line, const char *ps1) {
    printf("%s", ps1);
    fflush(stdout);

    int i = line->len;
    char ch;
    while (ch != ENTER) {
      ch = getc(stdin);

      if (ch == BACKSPACE) {
        if (i > 0) {
          line->buf[i--] = 0;
          printf("\b \b");
        }
        continue;
      } else if (ch == CHAR_CTRL_A || ch == CHAR_CTRL_B || ch == CHAR_CTRL_C || 
                 ch == CHAR_CTRL_D) { 
        fflush(stdout);
        line->len = 0;
        return ch;
      } 
      else {
        printf("%c", ch);
      }
      line->buf[i++] = ch;
    }
    printf("\n");

    line->len = i;
    return 0;
}

/* Get uptime in nanoseconds */
uint64_t mp_hal_time_ns(void) {
  return rtems_clock_get_uptime_nanoseconds();
}

/* Wait for specified amount of milliseconds */
void mp_hal_delay_ms(uint64_t delay) {
  struct timespec duration;

  /*
   * The remainder is milliseconds and tv_nsec is nanoseconds, so it has to be
   * scaled by a million.  Without that, time.sleep_ms(100) sleeps for 100
   * nanoseconds: every delay shorter than a second returns immediately, and
   * any loop that polls while waiting for something spins through its whole
   * budget before the thing it is waiting for can happen.
   */
  duration.tv_sec = delay / 1000;
  duration.tv_nsec = (long)(delay % 1000) * 1000000L;

  (void)duration;

  /*
   * rtems_task_wake_after(), not nanosleep().
   *
   * nanosleep() does not return here: a script that polls in a loop prints its
   * first iteration and then stops, with the radio still working and the event
   * task still running underneath it.  Whatever the cause, the directive is
   * the one the rest of this port already uses -- mp_hal_delay_us() below --
   * and it is what the ticker actually drives.
   */
  {
    rtems_interval ticks =
      (rtems_interval)((delay * rtems_clock_get_ticks_per_second() + 999u) / 1000u);
    rtems_task_wake_after(ticks != 0 ? ticks : 1);
  }
}

/* Wait for specified amount of microseconds */
void mp_hal_delay_us(uint64_t delay) {
  /*
   * Integer arithmetic, and at least one tick for a non-zero delay.  The
   * floating-point form rounded any delay under half a tick to zero, so
   * sleep_us() of anything shorter than the tick did not wait at all.
   */
  rtems_interval ticks =
    (rtems_interval)((delay * rtems_clock_get_ticks_per_second() + 999999u) / 1000000u);

  rtems_task_wake_after(ticks != 0 ? ticks : 1);
}

uint64_t mp_hal_ticks_ms(void) {
  return rtems_clock_get_ticks_since_boot();
}

uint64_t mp_hal_ticks_us(void) {
  return rtems_clock_get_ticks_since_boot() * 1e3;
}

uint64_t mp_hal_ticks_cpu(void) {
  return rtems_clock_get_ticks_since_boot();
}
