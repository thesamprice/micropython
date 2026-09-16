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

# lwIP's headers and library, when they are not in the BSP's own prefix.
#
# rtems-lwip installs into whichever prefix it was built against, and a BSP can
# reasonably have been built twice -- once per console, say -- with lwIP built
# against only one of them.  The headers are the same either way, so pointing
# at them is better than rebuilding lwIP to move a directory.
#
#   make lib RTEMS_BSP=riscv/esp32c3db RTEMS_PREFIX=... \
#       LWIP_CFLAGS="-isystem /path/to/other-prefix/riscv-rtems7/esp32c3db/lib/include"
CFLAGS += $(LWIP_CFLAGS)

# Define the required source files.
SRC_C =                               \
    modsocket.c                       \
    $(WLAN_SRC)                       \
    extmod/modselect.c                \
    shared/readline/readline.c        \
    shared/runtime/gchelper_generic.c \
    shared/runtime/pyexec.c           \
    shared/runtime/stdout_helpers.c   \
    extmod/modtime.c                  \
    extmod/modrandom.c                \
    extmod/modplatform.c              \
    extmod/modheapq.c                 \
    $(HASHLIB_SRC)                    \
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
# modsocket.c is here for the same reason as the two below: its method names
# -- bind, listen, accept, recv -- are qstrs, and a file not scanned here
# compiles against qstr identifiers that were never generated.
SRC_QSTR += shared/readline/readline.c shared/runtime/pyexec.c modsocket.c $(WLAN_SRC)

# network.WLAN, for BSPs that have a radio.
#
# Off unless asked for: it needs the ESP32-C3 WiFi driver and its headers, and
# a BSP without one would fail to compile a module it can never use.  Turn it
# on with WLAN=1 and point WLAN_CFLAGS at the driver's includes.
ifeq ($(WLAN),1)
WLAN_SRC = modnetwork.c
CFLAGS += $(WLAN_CFLAGS)

CFLAGS += -DMICROPY_PY_HASHLIB=0

# hashlib is left out when linking beside the WiFi supplicant.
#
# extmod/modhashlib.c carries its own SHA-256 and so does the supplicant's
# src/crypto/sha256-internal.c, and both define sha256_init and its siblings
# with external linkage -- the link fails with "multiple definition of
# `sha256_init'".  Dropping the module is the smaller loss here; making the two
# coexist means renaming one set or hiding it, which is worth doing but is not
# this example's business.
else
HASHLIB_SRC = extmod/modhashlib.c
endif

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

# The networking example.  Separate from test_app because it needs a BSP with a
# network stack -- arm/xilinx_zynq_a9_qemu through rtems-lwip -- and linking
# -llwip into the plain test app would make every board need one.
NET_APP_INCLUDE = -L$(BUILD)/ -lmicropython -I. -I$(BUILD)/ -I../../

net_app: lib $(BUILD)/pynet.c
	@echo "Building net_app.exe"
	$(CC) $(CFLAGS) -o $(BUILD)/net_app.exe examples/network/init.c \
	    examples/network/main.c $(BUILD)/pynet.c $(NET_APP_INCLUDE) \
	    -llwip $(LDFLAGS) -lm

$(BUILD)/pynet.c: $(BUILD)/pynet.tar
	rtems-bin2c $(BUILD)/pynet.tar $(BUILD)/pynet.c

$(BUILD)/pynet.tar: examples/network/net.py
	tar -C examples/network -cf $(BUILD)/pynet.tar net.py

# Include remaining core make rules.
include $(TOP)/py/mkrules.mk

