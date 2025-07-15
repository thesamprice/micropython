#include "py/obj.h"

#include <stdio.h>
#include <rtems.h>
#include <bsp/bootcard.h>

void mp_machine_reset(void) {
  bsp_reset(RTEMS_FATAL_SOURCE_EXIT, 0);
}

mp_int_t mp_machine_reset_cause(void) {
  printf("Warning: %s is not implemented\n", __func__);
}

static void mp_machine_idle(void) {
  printf("Warning: %s is not implemented\n", __func__);
}
