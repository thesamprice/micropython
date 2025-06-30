RTEMS_ARCH = $(word 1, $(subst /, ,$(RTEMS_BSP)))
RTEMS_TARGET = $(word 2, $(subst /, ,$(RTEMS_BSP)))
RTEMS_PKGCONFIG = $(RTEMS_PREFIX)/lib/pkgconfig/
RTEMS_PKGCONFIG_PKG = $(RTEMS_ARCH)-rtems$(RTEMS_VERSION)-$(RTEMS_TARGET)

CROSS_COMPILE ?= $(RTEMS_ARCH)-rtems$(RTEMS_VERSION)-

# Include the core environment definitions; this will set $(TOP).
include ../../py/mkenv.mk

# Include py core make definitions.
include $(TOP)/py/py.mk
include $(TOP)/extmod/extmod.mk

# Set CFLAGS and libraries.
CFLAGS += -I. -I$(BUILD) -I$(TOP)
LIBS += -lm

RTEMS_TARGET_CFLAGS += $(shell pkg-config --with-path=$(RTEMS_PKGCONFIG) --cflags $(RTEMS_PKGCONFIG_PKG))
RTEMS_TARGET_LDFLAGS += $(shell pkg-config --with-path=$(RTEMS_PKGCONFIG) --libs $(RTEMS_PKGCONFIG_PKG))

CFLAGS += $(RTEMS_TARGET_CFLAGS)
LDFLAGS += $(RTEMS_TARGET_LDFLAGS)

# Define the required source files.
SRC_C = \
    shared/readline/readline.c \
    shared/runtime/gchelper_generic.c \
    shared/runtime/pyexec.c \
    shared/runtime/stdout_helpers.c \
    mphalport.c

# Define source files containung qstrs.
SRC_QSTR += shared/readline/readline.c shared/runtime/pyexec.c

# Define the required object files.
OBJ = $(PY_CORE_O) $(addprefix $(BUILD)/, $(SRC_C:.c=.o))

# Build MicroPython Library
all: lib

# Include remaining core make rules.
include $(TOP)/py/mkrules.mk
