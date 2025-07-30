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
      } else if (ch == CHAR_CTRL_A || ch == CHAR_CTRL_B || ch == CHAR_CTRL_C || ch == CHAR_CTRL_D) { /* Ctrl+A, Ctrl+B, Ctrl+C, Ctrl+D */
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
  duration.tv_sec = delay / 1000;
  duration.tv_nsec = delay % 1000;

  nanosleep(&duration, NULL);
}

/* Wait for specified amount of microseconds */
void mp_hal_delay_us(uint64_t delay) {
  rtems_task_wake_after(rtems_clock_get_ticks_per_second() * delay / 1e6);
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
