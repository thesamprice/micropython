/*
 * RTEMS Micropython Test Application
 */
#include <rtems.h>
#include <stdlib.h>
#include <stdio.h>

#include <string.h>

#include "py/builtin.h"
#include "py/compile.h"
#include "py/gc.h"
#include "py/mperrno.h"
#include "py/stackctrl.h"
#include "shared/runtime/gchelper.h"
#include "shared/runtime/pyexec.h"
#include "py/runtime.h"

#include "py/misc.h"
#include "genhdr/mpversion.h"

#include <rtems/shell.h>

/* Allocate memory for the MicroPython GC heap */
static char heap[4096];

rtems_task Init(rtems_task_argument ignored) {
    /* Initialise the MicroPython runtime */
    mp_stack_ctrl_init();
    gc_init(heap, heap + sizeof(heap));

    mp_init();

    /* hack to get serial input to work on i386/pc686 BSP, TODO: replace with termios calls */
    rtems_shell_wait_for_input(STDIN_FILENO, 0, NULL, NULL);

    /* Start a REPL */
    pyexec_friendly_repl();

    gc_sweep_all();

    /* Deinitialise the runtime */
    mp_deinit();
  
    exit(0);
}

/* Handle uncaught exceptions (should never be reached in a correct C implementation) */
void nlr_jump_fail(void *val) {
    for (;;) {
    }
}

/* Do a garbage collection cycle */
void gc_collect(void) {
    gc_collect_start();
    gc_helper_collect_regs_and_stack();
    gc_collect_end();
}

/* There is no filesystem so stat'ing returns nothing */
mp_import_stat_t mp_import_stat(const char *path) {
    return MP_IMPORT_STAT_NO_EXIST;
}

/* There is no filesystem so opening a file raises an exception */
mp_lexer_t *mp_lexer_new_from_file(qstr filename) {
    mp_raise_OSError(MP_ENOENT);
}
