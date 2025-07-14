#include <rtems.h>
#include <time.h>
#include "py/obj.h"

mp_obj_t mp_time_time_get(void) {
  struct timespec uptime;
  rtems_clock_get_uptime(&uptime);

  return mp_obj_new_int(uptime.tv_sec);
}

