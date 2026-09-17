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

/*
 * RTEMS Micropython Test Application
 */
/*
 * The networking example's entry point.
 *
 * Identical to test/main.c apart from what happens before MicroPython starts:
 * the interface is brought up first, because a script asking for a socket on a
 * stack that is not running gets an error that says nothing about why.
 *
 * These are the lwIP calls the example is here to show, and there are only
 * two of them.  start_networking() is rtems-lwip's, declared in <netstart.h>;
 * it takes a netif to fill in, an address, a netmask, a gateway and a MAC, and
 * everything after that is ordinary BSD sockets -- which is exactly why the
 * Python side needs no lwIP-specific module.
 */

#include <netstart.h>
#include <lwip/netif.h>

#include <rtems.h>
#include <rtems/untar.h>

#include <stdio.h>
#include <string.h>
#include <termios.h>
#include <unistd.h>

#include "py/builtin.h"
#include "py/compile.h"
#include "py/runtime.h"
#include "py/gc.h"
#include "py/stackctrl.h"
#include "py/mperrno.h"
#include "py/misc.h"
#include "py/objstr.h"
#include "shared/runtime/gchelper.h"
#include "genhdr/mpversion.h"
#include <rtems/imfs.h>
#include <rtems/untar.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <stdlib.h>
#include "shared/runtime/pyexec.h"

#include "pynet.h"
#define TARFILE_START pynet_tar
#define TARFILE_SIZE pynet_tar_size

static struct netif net_interface;

/* Locally administered, so it cannot collide with a real card. */
static unsigned char mac_address[6] = { 0x02, 0x52, 0x54, 0x00, 0x12, 0x34 };

/*
 * QEMU's user-mode networking puts the guest at 10.0.2.15 behind a NAT with
 * the gateway at 10.0.2.2.  Static rather than DHCP so that a failure here is
 * this example's and not QEMU's DHCP server's.
 */
static bool bring_up_the_interface(void)
{
    ip_addr_t addr, mask, gw;
    int rv;

    /* IP_ADDR4, not IP4_ADDR: with LWIP_IPV6 on, ip_addr_t is the dual-stack
     * union and needs its type tag set as well as its bytes. */
    IP_ADDR4(&addr, 10, 0, 2, 15);
    IP_ADDR4(&mask, 255, 255, 255, 0);
    IP_ADDR4(&gw,   10, 0, 2, 2);

    rv = start_networking(&net_interface, &addr, &mask, &gw, mac_address);
    if (rv != 0) {
        printf("start_networking failed: %d\n", rv);
        return false;
    }

    printf("interface up at 10.0.2.15\n");
    return true;
}

void *POSIX_Init(void *argument)
{
    rtems_status_code sc;
    int stack_dummy;

    (void) argument;

    if (!bring_up_the_interface()) {
        printf("no network; the script would fail on socket()\n");
    }

    sc = Untar_FromMemory((void *) TARFILE_START, TARFILE_SIZE);
    if (sc != RTEMS_SUCCESSFUL) {
        printf("error: untar failed: %s\n", rtems_status_text(sc));
    }

    mp_stack_ctrl_init();
    mp_stack_set_limit(24 * 1024);

    /*
     * One allocation, and both ends of it.  Calling malloc() twice here --
     * once for the start and once to compute the end -- brackets the heap
     * with pointers into two different blocks, and MicroPython then collects
     * over memory it does not own.  It does not fault immediately; it faults
     * later, somewhere unrelated.
     */
    {
        const size_t heap_size = 192 * 1024;
        char *heap = malloc(heap_size);

        if (heap == NULL) {
            printf("cannot allocate the MicroPython heap\n");
            exit(1);
        }
        gc_init(heap, heap + heap_size);
    }

    mp_init();

    pyexec_file_if_exists("/net.py");

    mp_deinit();
    (void) stack_dummy;

    printf("*** END OF MICROPYTHON NETWORK EXAMPLE ***\n");
    exit(0);
}

void nlr_jump_fail(void *val)
{
    printf("FATAL: uncaught exception %p\n", val);
    exit(1);
}

void gc_collect(void)
{
    gc_collect_start();
    gc_collect_end();
}

/*
 * pyexec_file_if_exists() asks this before it opens anything, so a stub that
 * always says "no such file" makes the script silently not run -- the program
 * starts, prints nothing of its own and exits successfully, which is a far
 * worse failure than an error would have been.
 */
mp_import_stat_t mp_import_stat(const char *path)
{
    struct stat st;

    if (stat(path, &st) != 0) {
        return MP_IMPORT_STAT_NO_EXIST;
    }
    return S_ISDIR(st.st_mode) ? MP_IMPORT_STAT_DIR : MP_IMPORT_STAT_FILE;
}

mp_obj_t mp_builtin_open(size_t n_args, const mp_obj_t *args, mp_map_t *kwargs)
{
    (void) n_args; (void) args; (void) kwargs;
    return mp_const_none;
}
MP_DEFINE_CONST_FUN_OBJ_KW(mp_builtin_open_obj, 1, mp_builtin_open);
