# RTEMS Micropython Port on i386/pc686

## Building the Port

Run make (replace the prefix path with your own):

```
$ RTEMS_PREFIX=~/rtems-workspace/rtems/7 RTEMS_VERSION=7 RTEMS_BSP=i386/pc686 make
```

This generates a `build/` directory with objects that can be directly linked with an RTEMS Application.

TODO: add example application code or link to one
