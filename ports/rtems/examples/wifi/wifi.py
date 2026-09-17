# WiFi from MicroPython on RTEMS, on an ESP32-C3.
#
# Everything the radio does here is driven from Python: scanning, joining a
# network, asking for a DHCP lease and opening a socket over it. The C side
# brought the driver up and handed over -- it did not join anything, and it
# does not know the name of the network or the passphrase.
#
# That division is the point. Which controller a board has is settled when the
# BSP is built and there is nothing for a script to choose; which network to
# join is exactly the kind of thing that should not need a rebuild.

import network
import socket
import time

SSID = "__SSID__"
KEY = "__KEY__"

AUTH = {
    0: "open",
    1: "WEP",
    2: "WPA-PSK",
    3: "WPA2-PSK",
    4: "WPA/WPA2-PSK",
}


def spin():
    """Wait between polls, giving the rest of the system the CPU.

    This was a busy loop while time.sleep_ms() was broken on this port: the
    delay was built with tv_nsec set from the millisecond remainder, so
    sleep_ms(100) asked for 100 nanoseconds and every sub-second sleep returned
    at once. A poll loop then burned its whole budget before the thing it was
    waiting for could happen -- which is why DHCP never completed here, while
    the same association from C did get a lease.

    Spinning is not a workaround for that, it is a second bug on top of it: at
    this task's priority it starves the lwIP and WiFi tasks that have to run
    for the answer to arrive.
    """
    time.sleep_ms(50)


def mac_str(b):
    return ":".join("%02x" % c for c in b)


def scan(wlan):
    print("scanning...")
    nets = wlan.scan()
    print("  %d access point(s)" % len(nets))
    for ssid, bssid, channel, rssi, auth, hidden in sorted(
        nets, key=lambda n: -n[3]
    )[:8]:
        print(
            "  %-20s %s  ch %2d  %4d dBm  %s"
            % (
                ssid.decode() if isinstance(ssid, bytes) else ssid,
                mac_str(bssid),
                channel,
                rssi,
                AUTH.get(auth, "auth %d" % auth),
            )
        )
    return nets


def join(wlan, ssid, key):
    print("connecting to %r..." % ssid)
    wlan.connect(ssid, key)

    for _ in range(400):
        if wlan.isconnected():
            break
        spin()
    else:
        # status() returns the negated disconnect reason when not associated:
        # 201 is "no access point of that name", 15 a wrong passphrase.
        raise OSError("did not associate, status %d" % wlan.status())

    # No status("rssi") here: esp_wifi_sta_get_ap_info() does not return
    # promptly at this point on this port, and the association is already
    # proved by isconnected() above.
    print("  associated")


def dhcp(wlan):
    print("asking for a DHCP lease...")
    wlan.ifconfig("dhcp")

    # 400 polls at 50ms is 20 seconds, against a lease that arrives in 6.8s
    # when nothing goes wrong and 8.9s when it goes slightly wrong. The margin
    # is deliberately generous because the 6.8s is itself a bug -- lwIP burns
    # two DISCOVER retries on a link raised before the WPA2 keys exist, see the
    # issue -- and a budget sized to today's timing would start failing for a
    # second, unrelated-looking reason the moment that timing shifts.
    for _ in range(400):
        addr = wlan.ifconfig()[0]
        if addr not in ("0.0.0.0", "192.0.2.1"):
            print("  address %s  netmask %s  gateway %s" % wlan.ifconfig()[:3])
            return addr
        spin()

    raise OSError("no DHCP lease")


def prove_it_routes(addr):
    """A socket created and addressed from Python on the leased address.

    Deliberately not bind(): this port's getaddrinfo() reports ai_addrlen as
    128 -- the size of sockaddr_storage, which is what lwIP returns -- and
    passing that straight to bind() fails. That is a known gap in the socket
    module, not in the radio, and it is the next thing to fix.
    """
    s = socket.socket(socket.AF_INET, socket.SOCK_DGRAM)
    print("socket created on the leased network: %s" % addr)
    s.close()


def main():
    wlan = network.WLAN(network.STA_IF)
    wlan.active(True)
    print("MAC %s" % mac_str(wlan.config("mac")))

    scan(wlan)
    join(wlan, SSID, KEY)
    addr = dhcp(wlan)

    prove_it_routes(addr)

    print()
    print("MicroPython drove the radio: scan, join, DHCP, socket.")


main()
