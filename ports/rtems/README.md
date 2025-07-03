# RTEMS Micropython Port on i386/pc686

## Building the Port

Run make (replace the prefix path with your own):

```
$ RTEMS_PREFIX=~/rtems-workspace/rtems/7 RTEMS_VERSION=7 RTEMS_BSP=i386/pc686 make
```

This generates a `build/` directory with generated headers and a `libmicropython.a` that can be directly linked with an RTEMS Application.

## Building the example app

```
$ RTEMS_PREFIX=~/rtems-workspace/rtems/7 RTEMS_VERSION=7 RTEMS_BSP=i386/pc686 make test_app
```
