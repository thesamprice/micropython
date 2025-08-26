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
SRC_C =                               \
    shared/readline/readline.c        \
    shared/runtime/gchelper_generic.c \
    shared/runtime/pyexec.c           \
    shared/runtime/stdout_helpers.c   \
    extmod/modtime.c                  \
    extmod/modrandom.c                \
    extmod/modplatform.c              \
    extmod/modheapq.c                 \
    extmod/modhashlib.c               \
    extmod/modre.c                    \
    extmod/modos.c                    \
    extmod/modjson.c                  \
    extmod/moductypes.c               \
    extmod/modbinascii.c              \
                                      \
    extmod/modmachine.c               \
    extmod/machine_mem.c              \
    mphalport.c                       \
    mpthreadport.c

# Define source files containung qstrs.
SRC_QSTR += shared/readline/readline.c shared/runtime/pyexec.c

# Define the required object files.
OBJ = $(PY_CORE_O) $(addprefix $(BUILD)/, $(SRC_C:.c=.o))

# Build MicroPython Library
all: lib

# Build test application
TEST_APP_INCLUDE = -L$(BUILD)/ -lmicropython -I. -I$(BUILD)/ -I../../

test_app: lib
	@echo "Building test_app.exe"
	$(CC) $(CFLAGS) -o $(BUILD)/test_app.exe test/init.c test/main.c $(BUILD)/pytest.c $(TEST_APP_INCLUDE) $(LDFLAGS) -lm

$(BUILD)/pytest.c: $(BUILD)/pytest.tar
	rtems-bin2c $(BUILD)/pytest.tar $(BUILD)/pytest.c

$(BUILD)/pytest.tar: test/test.py
	tar -C test -cf $(BUILD)/pytest.tar test.py

# Include remaining core make rules.
include $(TOP)/py/mkrules.mk

