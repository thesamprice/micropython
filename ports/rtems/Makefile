# Step 1: Compile MicroPython library
CROSS_COMPILE ?= i386-rtems7-

# Include the core environment definitions; this will set $(TOP).
include ../../py/mkenv.mk

# Include py core make definitions.
include $(TOP)/py/py.mk
include $(TOP)/extmod/extmod.mk

# Set CFLAGS and libraries.
CFLAGS += -I. -I$(BUILD) -I$(TOP)
LIBS += -lm

# Define the required source files.
SRC_C = \
    main.c \
    shared/readline/readline.c \
    shared/runtime/gchelper_generic.c \
    shared/runtime/pyexec.c \
    shared/runtime/stdout_helpers.c \

# Define source files containung qstrs.
SRC_QSTR += shared/readline/readline.c shared/runtime/pyexec.c

# Define the required object files.
OBJ = $(PY_CORE_O) $(addprefix $(BUILD)/, $(SRC_C:.c=.o))

# Define the top-level target, the main firmware.
micropython_lib: $(OBJ)

# Include remaining core make rules.
include $(TOP)/py/mkrules.mk




# Step 2: Compile code that uses REMS libraries
# re-invoke Makefile with RTEMS=1 to prevent conflicts between RTEMS and Micropython make environments
rtems_port_impl:
	@echo ""
	@echo "Building RTEMS port files..."
	$(MAKE) --no-print-directory RTEMS=1 build_rtems

# Generate object file for mphalport.c
ifeq ($(RTEMS),1)

include $(RTEMS_MAKEFILE_PATH)/Makefile.inc

BUILD=build

CSRCS = mphalport.c
COBJS = $(CSRCS:%.c=$(BUILD)/%.o)

OBJS= $(COBJS)

build_rtems: $(OBJS)

endif




all: micropython_lib rtems_port_impl
