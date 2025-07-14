#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <stdbool.h>

#include <rtems.h>

// taken from py/misc.h
typedef struct _vstr_t {
    size_t alloc;
    size_t len;
    char *buf;
    bool fixed_buf;
} vstr_t;

/* Receive single character, blocking until one is available */
int mp_hal_stdin_rx_chr(void) {
  return getc(stdin);
}

/* Send the string of given length */
void mp_hal_stdout_tx_strn(const char *str, int len) {
  printf("%s", str);
  fflush(stdout);
}

/* custom readline implementation */
int readline(vstr_t *line, const char *ps1) {
    printf("%s", ps1);
    fflush(stdout);

    int i = line->len;
    char ch;
    while ((ch = getc(stdin)) != '\n') {
      line->buf[i++] = ch;
    }

    line->len = i;
    return 0;
}

/* Get uptime in nanoseconds */
uint64_t mp_hal_time_ns(void) {
  return rtems_clock_get_uptime_nanoseconds();
}

/* Wait for specified amount of milliseconds */
void mp_hal_delay_ms(uint64_t delay) {
  rtems_task_wake_after(rtems_clock_get_ticks_per_second() * delay / 1e3);
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
