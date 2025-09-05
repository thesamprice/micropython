# RTEMS Micropython Port on i386/pc686

## Building the Port

Run make (replace the prefix path with your own):

```
$ mkdir build
$ RTEMS_PREFIX=~/rtems-workspace/rtems/7 RTEMS_VERSION=7 RTEMS_BSP=i386/pc686 make
```

This generates a `build/` directory with generated headers and a `libmicropython.a` that can be directly linked with an RTEMS Application.

## Building the example app

```
$ mkdir build
$ make build/pytest.c
$ RTEMS_PREFIX=~/rtems-workspace/rtems/7 RTEMS_VERSION=7 RTEMS_BSP=i386/pc686 make test_app
```

Run the test app with QEMU:

```
qemu-system-i386 --cpu core2duo -append "--console=/dev/com1" -serial stdio -kernel build/test_app.exe
```

## Running MicroPython Tests

Run the test app with a virtual serial port:

```
qemu-system-i386 --cpu core2duo -no-reboot -append "--console=/dev/com1" -serial chardev:char0 -chardev pty,id=char0,logfile=chardev.log,signal=off -kernel build/test_app.exe
```

The assigned device `/dev/pts/X` is printed by qemu. Now use `run-tests.py` under `tests/`:

```
./run-tests.py -t /dev/pts/X -i "basics/*" -i "float/*" -i "extmod/heapq*" -i "misc/*" -e "misc/print_exception.py" -e "misc/sys_settrace*" -e "misc/cexample*"
```

## MPRemote Examples

```
mpremote connect port:/dev/pts/6 eval "print ('hi')"
mpremote connect port:/dev/pts/6 run <path to script>
```
