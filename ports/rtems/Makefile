# SPDX-License-Identifier: BSD-2-Clause

# Copyright (C) 2025 Sameer Srivastava <sam33r012@gmail.com>
# Redistribution and use in source and binary forms, with or without
# modification, are permitted provided that the following conditions
# are met:
# 
# Redistributions of source code must retain the above copyright
# notice, this list of conditions and the following disclaimer.
# Redistributions in binary form must reproduce the above copyright
# notice, this list of conditions and the following disclaimer in the
# documentation and/or other materials provided with the distribution.
# 
# THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
# AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
# IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
# ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT OWNER OR CONTRIBUTORS BE
# LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
# CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
# SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
# INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
# CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
# ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
# POSSIBILITY OF SUCH DAMAGE.

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

