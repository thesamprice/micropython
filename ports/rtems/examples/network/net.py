# Networking from MicroPython on RTEMS.
#
# Everything here is the ordinary `socket` module. There is no lwIP-specific
# API and no RTEMS-specific API, because there does not need to be one: RTEMS
# has BSD sockets, rtems-lwip provides them over lwIP, and MicroPython's POSIX
# socket module sits straight on top.
#
# The interface is already up when this runs -- see main.c. Bringing it up is
# C's job: which controller the board has and how it is wired is fixed when the
# BSP is built, so there is nothing here for a script to choose.

import socket

PORT = 5555


def show_address_helpers():
    """The parts of `socket` that need no connection, so they run first."""
    packed = socket.inet_pton(socket.AF_INET, "10.0.2.15")
    print("inet_pton('10.0.2.15') ->", packed)
    print("inet_ntop(that)        ->", socket.inet_ntop(socket.AF_INET, packed))


def serve_one_connection():
    """Listen, accept one client, echo what it sends, and hang up.

    The host connects in: a guest that could send but not receive would pass a
    test that only sent, so the exchange goes both ways.
    """
    s = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
    # setsockopt() in this module takes a buffer, not an int -- passing 1
    # raises "object with buffer protocol required". Four zero-padded bytes
    # are what the C call wants for a boolean option.
    s.setsockopt(socket.SOL_SOCKET, socket.SO_REUSEADDR, b"\x01\x00\x00\x00")

    # This module takes a packed sockaddr, not a (host, port) tuple: it is the
    # POSIX socket module from ports/unix, which is closer to the C API than
    # CPython's. getaddrinfo() is what packs it, and its last element is the
    # address in the form bind() and connect() want.
    # This module takes a packed sockaddr, not a (host, port) tuple: it is
    # the POSIX socket module from ports/unix, which is closer to the C API
    # than CPython's. getaddrinfo() packs it, and its last element is what
    # bind() and connect() want.
    addr = socket.getaddrinfo("0.0.0.0", PORT)[0][-1]

    # Trimmed to the 16 bytes an AF_INET sockaddr occupies. lwIP reports
    # ai_addrlen as sizeof(struct sockaddr_storage) -- 128 -- and bind()
    # passes that length straight through to lwip_bind(), which fails with
    # EIO. The failure surfaces at listen(), not at bind(), which is why it
    # is worth a comment rather than a shrug.
    addr = addr[:16]

    s.bind(addr)
    s.listen(1)
    print("listening on", PORT)

    conn, peer = s.accept()
    print("accepted a connection")

    data = conn.recv(64)
    print("received", data)

    conn.send(b"MICROPYTHON")
    conn.close()
    s.close()
    return data


print("--- micropython networking on rtems ---")
show_address_helpers()

data = serve_one_connection()

if data == b"hello":
    print("CI-MARKER mpnet ok")
else:
    print("FAIL: expected b'hello', got", data)
