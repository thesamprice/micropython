/* SPDX-License-Identifier: MIT */

/*
 * RTEMS configuration for the MicroPython WiFi example, sized for an
 * ESP32-C3.
 *
 * Deliberately not the Zynq example's configuration.  That one uses
 * CONFIGURE_UNLIMITED_OBJECTS with a 32 KiB minimum task stack, which is
 * reasonable on a part with megabytes of DRAM and is not survivable here: this
 * chip has 400 KiB of SRAM, the image's .bss is already about 260 KiB of it
 * once the WiFi blobs, lwIP and the interpreter are linked, and sixteen
 * threads at 32 KiB would want half a megabyte of stack on its own.  The
 * symptom is INTERNAL_ERROR_TOO_LITTLE_WORKSPACE at boot, before anything
 * prints.
 *
 * So the objects are counted rather than unlimited, and the counts are what
 * the WiFi libraries actually ask for -- they create a handful of tasks,
 * several semaphores and a few queues and timers during esp_wifi_init() and
 * again when the station associates.
 */

#include <rtems.h>

/* confdefs takes the address of this, so it has to be declared first. */
void *POSIX_Init(void *argument);

#define CONFIGURE_APPLICATION_NEEDS_CLOCK_DRIVER
#define CONFIGURE_APPLICATION_NEEDS_SIMPLE_CONSOLE_DRIVER

#define CONFIGURE_FILESYSTEM_IMFS
#define CONFIGURE_MAXIMUM_FILE_DESCRIPTORS 16

/*
 * The BSP's default tick, not 1000us.
 *
 * Asking for a 1ms tick here makes every timed wait block for good: a script
 * that polls in a loop prints its first iteration and never reaches the
 * second, whether it waits through nanosleep() or rtems_task_wake_after().
 * The C examples on this BSP leave the tick alone and their waits work, so
 * this does too rather than carrying a faster tick nothing here needs.
 */

/*
 * Enough for the WiFi libraries' own tasks, the RTEMS timer server the ETS
 * timers run on, lwIP's tcpip thread, the event task and the interpreter.
 * Too few and a WiFi task simply fails to be created, which the libraries do
 * not report -- the adapter now does.  Too many and the workspace runs out
 * before anything prints: this part has 400 KiB of SRAM and the image's .bss
 * already claims about 260 KiB of it, so there is not much to spend.  16 is
 * what the C examples on this BSP use and what fits.
 */
#define CONFIGURE_MAXIMUM_TASKS 16
#define CONFIGURE_MAXIMUM_POSIX_THREADS 4
#define CONFIGURE_MAXIMUM_SEMAPHORES 64
#define CONFIGURE_MAXIMUM_MESSAGE_QUEUES 16
#define CONFIGURE_MAXIMUM_TIMERS 32
#define CONFIGURE_MAXIMUM_POSIX_KEYS 4
#define CONFIGURE_MAXIMUM_DRIVERS 8

/*
 * 4 KiB minimum, not 32.  The WiFi libraries' own tasks ask for what they need
 * explicitly; everything else here is small.
 */
#define CONFIGURE_MINIMUM_TASK_STACK_SIZE (4 * 1024)

/*
 * The interpreter runs on this one and needs the room: mp_stack_set_limit() in
 * main.c is set to 14 KiB and has to sit inside it.
 */
#define CONFIGURE_POSIX_INIT_THREAD_STACK_SIZE (20 * 1024)

#define CONFIGURE_UNIFIED_WORK_AREAS
#define CONFIGURE_POSIX_INIT_THREAD_TABLE

#define CONFIGURE_INIT

#include <rtems/confdefs.h>
