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
