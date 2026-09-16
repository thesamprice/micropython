/* SPDX-License-Identifier: MIT */

/*
 * MicroPython driving an ESP32-C3's WiFi.
 *
 * What C does here is everything that cannot be a choice: esp_wifi_init(),
 * which needs the blobs' linker sections and an event loop, and the netif that
 * joins the driver to rtems-lwip.  It does not scan, does not join anything,
 * and does not know the name of a network or a passphrase.
 *
 * Everything after that is the script's -- see wifi.py.  The division is the
 * point of the example: which radio a board has is settled when the BSP is
 * built, and which network to join is exactly the thing that should not need a
 * rebuild.
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>

#include <rtems.h>
#include <rtems/untar.h>

#include <esp_wifi.h>
#include <esp_event.h>
#include <rtems-esp/netif.h>

#include <lwip/netif.h>
#include <netstart.h>

#include "py/compile.h"
#include "py/gc.h"
#include "py/mperrno.h"
#include "py/runtime.h"
#include "py/stackctrl.h"
#include "py/builtin.h"
#include "py/persistentcode.h"
#include "shared/runtime/pyexec.h"
#include "shared/runtime/gchelper.h"
#include "py/mphal.h"

#include "pywifi.h"

#define TARFILE_START pywifi_tar
#define TARFILE_SIZE pywifi_tar_size

/* Registered by modnetwork.c, once the event loop exists. */
extern void rtems_micropython_network_init(void);

static struct netif net_interface;

/*
 * The address here is a placeholder and is meant to be replaced.
 *
 * esp32c3_netif_add() wants one, and the script asks for a DHCP lease as its
 * first act -- so anything routable would be a lie about what this example
 * demonstrates.  It is deliberately in a range nothing here uses, so that an
 * address printed by the script is one DHCP gave it and not this.
 */
static bool bring_up_the_driver(void)
{
    wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();
    ip_addr_t addr, mask, gw;
    esp_err_t rv;

    /*
     * No esp_event_loop_create_default() here.  This port's event layer
     * (src/rtems_esp_event.c) dispatches from its own task and handlers are
     * registered directly, so there is no default loop object to create.
     */
    rtems_micropython_network_init();

    rv = esp_wifi_init(&cfg);
    if (rv != ESP_OK) {
        printf("esp_wifi_init failed: %d\n", (int) rv);
        return false;
    }

    IP_ADDR4(&addr, 192, 0, 2, 1);
    IP_ADDR4(&mask, 255, 255, 255, 0);
    IP_ADDR4(&gw,   192, 0, 2, 1);

    if (start_networking(&net_interface, &addr, &mask, &gw, NULL) != 0) {
        printf("start_networking failed\n");
        return false;
    }

    return true;
}

void *POSIX_Init(void *argument)
{
    rtems_status_code sc;

    (void) argument;

    printf("\n*** MICROPYTHON WIFI ON RTEMS ***\n");

    if (!bring_up_the_driver()) {
        printf("no radio; the script would fail on active(True)\n");
    }

    sc = Untar_FromMemory((void *) TARFILE_START, TARFILE_SIZE);
    if (sc != RTEMS_SUCCESSFUL) {
        printf("error: untar failed: %s\n", rtems_status_text(sc));
    }

    mp_stack_ctrl_init();
    mp_stack_set_limit(14 * 1024);

    /*
     * One allocation and both ends of it -- calling malloc() twice brackets
     * the heap with pointers into two different blocks, and the collector then
     * walks memory it does not own.  It does not fault at the time; it faults
     * later, somewhere unrelated.
     */
    {
        const size_t heap_size = 24 * 1024;
        char *heap = malloc(heap_size);

        if (heap == NULL) {
            printf("cannot allocate the MicroPython heap\n");
            exit(1);
        }
        gc_init(heap, heap + heap_size);
    }

    mp_init();

    pyexec_file_if_exists("/wifi.py");

    mp_deinit();

    printf("\n*** END OF MICROPYTHON WIFI EXAMPLE ***\n");
    exit(0);
}

void nlr_jump_fail(void *val)
{
    printf("FATAL: uncaught exception %p\n", val);
    exit(1);
}

void gc_collect(void)
{
    /*
     * The registers and the C stack have to be scanned, not just the roots.
     *
     * gc_collect_start()/gc_collect_end() on their own mark only what the
     * interpreter already knows about, so an object whose last reference lives
     * in a local variable or a callee-saved register is collected while it is
     * still in use.  It does not fail where it happens: the collection
     * succeeds, and the program wanders off later doing something unrelated --
     * here, a time.sleep_ms() that never returns after a scan() that allocated
     * enough to trigger a collection.
     *
     * gc_helper_collect_regs_and_stack() is what every other port calls, and
     * shared/runtime/gchelper_generic.c is already in this port's SRC_C.
     */
    gc_collect_start();
    gc_helper_collect_regs_and_stack();
    gc_collect_end();
}

/*
 * pyexec_file_if_exists() asks this before it opens anything, so a stub that
 * always says "no such file" makes the script silently not run -- the program
 * starts, prints nothing of its own and exits successfully, which is a worse
 * failure than an error.
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
