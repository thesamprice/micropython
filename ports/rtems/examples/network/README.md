# Networking from MicroPython on RTEMS

A Python script that listens on a TCP port, accepts a connection and answers
it, on `arm/xilinx_zynq_a9_qemu` with rtems-lwip.

```
--- micropython networking on rtems ---
inet_pton('10.0.2.15') -> b'\n\x00\x02\x0f'
inet_ntop(that)        -> 10.0.2.15
listening on 5555
accepted a connection
received b'hello'
CI-MARKER mpnet ok
```

and on the host side, `guest replied: b'MICROPYTHON'`.

## The two lwIP calls, and where the boundary is

There are only two, and both are in `main.c` rather than in Python:

```c
#include <netstart.h>

start_networking(&net_interface, &addr, &mask, &gw, mac_address);
```

Everything after that is ordinary BSD sockets, which is exactly why `net.py`
imports nothing but `socket` and why the port needs no lwIP-specific module.

Bringing the interface up is C's job on purpose. Which controller the board
has and how it is wired is fixed when the BSP is built, so there is nothing
for a script to choose; the address is all that is left, and even that is
only in Python's reach here because the example uses a static one.

## Building

Needs the BSP and rtems-lwip installed against it.

```sh
RTEMS_PREFIX=$HOME/rtems/7 RTEMS_VERSION=7 \
    RTEMS_BSP=arm/xilinx_zynq_a9_qemu make net_app
```

Run it with the harness from the rtems-esphome tree, which forwards a host
port into the guest and connects to it:

```sh
tools/zynq-lwip-run.sh -M "CI-MARKER mpnet ok" build/net_app.exe
```

The host connects *in*. A guest that could send but not receive would pass a
test that only sent.

## What this does not do

**It does not configure an SSID, and it cannot.** That was the original
framing of the request and it does not survive contact with the code: SSID is
802.11, which is layer 2, and lwIP is a TCP/IP stack. `espressif/esp-lwip` is
"a fork of lwIP with ESP-IDF specific patches" and contains no `ssid`, no
`esp_wifi`, no `wifi_config` — searching the repository for any of them
returns nothing.

Joining a network by name lives in ESP-IDF's `esp_wifi_*`, which is a
different component, depends on binary blobs built against FreeRTOS, and is
not part of any lwIP. A `network.WLAN(...).connect(ssid, password)` on RTEMS
needs a WiFi driver underneath before it needs anything in Python, and that
driver is the open question — not the Python API, which MicroPython already
has in `extmod/modnetwork.c`.

So this example goes as far as the stack goes: sockets over lwIP, on an
interface something else brought up.
