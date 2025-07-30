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

#include <termios.h>

#include <fcntl.h>
#include <rtems/imfs.h>

#include "pytest.h"
#define TARFILE_START pytest_tar
#define TARFILE_SIZE pytest_tar_size

static char buf[256];
static const char file_path[] = "/test.py";

/* Allocate memory for the MicroPython GC heap */
static char heap[16384];


void make_raw_terminal(struct termios *previous_term) {
    struct termios term;
  if (tcgetattr(fileno(stdin), previous_term) == 0) {
     term = *previous_term;
     term.c_iflag &= ~(IGNBRK|BRKINT|PARMRK|ISTRIP|INLCR|IGNCR|ICRNL|IXON);
     term.c_oflag &= ~OPOST;
     term.c_lflag &= ~(ECHO|ECHONL|ICANON|ISIG|IEXTEN);
     term.c_cflag &= ~(CSIZE | PARENB);
    term.c_cflag |= CS8;

    term.c_cc[VMIN]  = 1;
    term.c_cc[VTIME] = 0;
    if (tcsetattr (fileno(stdin), TCSADRAIN, &term) < 0) {
      printf("shell: cannot set terminal attributes\n");
    }
  }
}


void *POSIX_Init(void *argument) {
soft_reset:
    /* load tarfs image */
    rtems_status_code sc;
    sc = rtems_tarfs_load("/",(void *)TARFILE_START, TARFILE_SIZE);
    if (sc != RTEMS_SUCCESSFUL) {
        printf ("error: untar failed: %s\n", rtems_status_text (sc));
    }

    /* Initialise the MicroPython runtime */
    mp_stack_ctrl_init();
    gc_init(heap, heap + sizeof(heap));

    mp_init();

    struct termios term;
    make_raw_terminal(&term);


    /* Start a REPL */
    for (;;) {
        if (pyexec_mode_kind == PYEXEC_MODE_RAW_REPL) {
            if (pyexec_raw_repl() != 0) {
                break;
            }
        } else {
            if (pyexec_friendly_repl() != 0) {
                break;
            }
        }
    }

    printf("soft reboot\r\n");

    gc_sweep_all();

    /* Deinitialise the runtime */
    mp_deinit();

    goto soft_reset;
  
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
    char file_path[256] = { 0 };

    sprintf(file_path, "/%s", path);

    struct stat file_info;
    int ret = stat(file_path, &file_info);

    if (ret == -1) {
        return MP_IMPORT_STAT_NO_EXIST;
    }

    if (file_info.st_mode & S_IFDIR) {
        return MP_IMPORT_STAT_DIR;
    }

    if (file_info.st_mode & S_IFREG) {
        return MP_IMPORT_STAT_FILE;
    }
}
