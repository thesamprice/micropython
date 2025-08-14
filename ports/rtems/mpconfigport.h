#include <stdint.h>

// Python internal features.
#define MICROPY_ENABLE_GC                       (1)
#define MICROPY_GC_SPLIT_HEAP                   (0)
#define MICROPY_STACK_CHECK_MARGIN              (1)
#define MICROPY_DEBUG_VM_STACK_OVERFLOW         (1)

#define MICROPY_HELPER_REPL                     (1)
#define MICROPY_ERROR_REPORTING                 (MICROPY_ERROR_REPORTING_DETAILED)
#define MICROPY_FLOAT_IMPL                      (MICROPY_FLOAT_IMPL_FLOAT)

#define MICROPY_READLINE_HISTORY_SIZE           (0)
#define MICROPY_USE_READLINE                    (0)
#define MICROPY_REPL_AUTO_INDENT                (1)

// #define MICROPY_ENABLE_EXTERNAL_IMPORT          (1)
#define MICROPY_READER_POSIX                    (1)

// time module
#define MICROPY_PY_TIME                         (1)
#define MICROPY_PY_TIME_TIME_TIME_NS            (1)
#define MICROPY_PY_TIME_INCLUDEFILE "ports/rtems/modtime.c"

// random module
#define MICROPY_PY_RANDOM                       (1)
#define MICROPY_PY_RANDOM_EXTRA_FUNCS           (1)

// machine module
#define MICROPY_PY_MACHINE                      (1)
#define MICROPY_PY_MACHINE_RESET                (1)
#define MICROPY_PY_MACHINE_MEMX                 (1)
#define MICROPY_PY_MACHINE_SIGNAL               (0)
#define MICROPY_PY_MACHINE_INCLUDEFILE "ports/rtems/modmachine.c"

#define MICROPY_PY_PLATFORM                     (1)

// Fine control over Python builtins, classes, modules, etc.
#define MICROPY_PY_ASYNC_AWAIT                  (0)
#define MICROPY_PY_BUILTINS_SET                 (1)
#define MICROPY_PY_ATTRTUPLE                    (1)
#define MICROPY_PY_MATH                         (1)
#define MICROPY_PY_IO                           (0)
#define MICROPY_PY_STRUCT                       (1)
#define MICROPY_PY_BUILTINS_NOTIMPLEMENTED      (1)
#define MICROPY_CAN_OVERRIDE_BUILTINS           (1)
#define MICROPY_PY_BUILTINS_COMPILE             (1)
#define MICROPY_PY_BUILTINS_POW3                (1)
#define MICROPY_PY_BUILTINS_RANGE_BINOP         (1)

#define MICROPY_PY_FUNCTION_ATTRS               (1)

#define MICROPY_PY_BUILTINS_BYTES_HEX           (1)

#define MICROPY_PY_BUILTINS_MEMORYVIEW          (1)
#define MICROPY_PY_BUILTINS_MEMORYVIEW_ITEMSIZE (1)

#define MICROPY_PY_IO_IOBASE                    (1)
#define MICROPY_PY_IO_BUFFEREDWRITER            (1)

#define MICROPY_PY_BUILTINS_STR_CENTER          (1)
#define MICROPY_PY_BUILTINS_STR_COUNT           (1)
#define MICROPY_PY_BUILTINS_STR_PARTITION       (1)
#define MICROPY_PY_BUILTINS_STR_SPLITLINES      (1)

#define MICROPY_PY_BUILTINS_SLICE               (1)
#define MICROPY_PY_BUILTINS_SLICE_ATTRS         (1)
#define MICROPY_PY_BUILTINS_SLICE_INDICES       (1)

#define MICROPY_PY_BUILTINS_FROZENSET           (1)

#define MICROPY_PY_ERRNO                        (1)
#define MICROPY_PY_ERRNO_ERRORCODE              (1)

#define MICROPY_PY_BUILTINS_ROUND_INT           (1)

#define MICROPY_PY_ARRAY                        (1)
#define MICROPY_PY_ARRAY_SLICE_ASSIGN           (1)

#define MICROPY_PY_ALL_SPECIAL_METHODS          (1)
#define MICROPY_PY_ALL_INPLACE_SPECIAL_METHODS  (1)
#define MICROPY_PY_REVERSE_SPECIAL_METHODS      (1)

#define MICROPY_PY_DELATTR_SETATTR              (1)
#define MICROPY_PY_DESCRIPTORS                  (1)

// re module
#define MICROPY_PY_RE                           (1)
#define MICROPY_PY_RE_DEBUG                     (1)
#define MICROPY_PY_RE_MATCH_GROUPS              (1)
#define MICROPY_PY_RE_MATCH_SPAN_START_END      (1)
#define MICROPY_PY_RE_SUB                       (1)
// sys module
#define MICROPY_PY_SYS                          (1)
#define MICROPY_PY_SYS_MAXSIZE                  (1)
//#define MICROPY_PY_SYS_STDFILES                 (1)
//#define MICROPY_PY_SYS_STDIO_BUFFER             (1)
#define MICROPY_PY_SYS_TRACEBACKLIMIT           (1)
#define MICROPY_PY_SYS_GETSIZEOF                (1)

// collections
#define MICROPY_PY_COLLECTIONS                    (1)
#define MICROPY_PY_COLLECTIONS_DEQUE              (1)
#define MICROPY_PY_COLLECTIONS_DEQUE_ITER         (1)
#define MICROPY_PY_COLLECTIONS_DEQUE_SUBSCR       (1)
#define MICROPY_PY_COLLECTIONS_ORDEREDDICT        (1)
#define MICROPY_PY_COLLECTIONS_NAMEDTUPLE__ASDICT (1)


#define MICROPY_PY_FSTRINGS                     (1)
#define MICROPY_LONGINT_IMPL        (MICROPY_LONGINT_IMPL_MPZ)
#define MPZ_DIG_SIZE                            16

// Type definitions for the specific machine.

typedef intptr_t mp_int_t; // must be pointer size
typedef uintptr_t mp_uint_t; // must be pointer size
typedef long mp_off_t;

// We need to provide a declaration/definition of alloca().
#include <alloca.h>

// Define the port's name and hardware.
#define MICROPY_HW_BOARD_NAME "rtems-i386/pc686"
#define MICROPY_HW_MCU_NAME   "unknown-cpu"

#define MP_STATE_PORT MP_STATE_VM

#define MP_NORETURN __attribute__((noreturn))
