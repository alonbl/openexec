openexec LD_PRELOAD
===================

Handy utility to return an output of a program instead of the content of
the file.

Usage:

```
LD_PRELOAD=/full/path/to/openexec.so \
    OPENEXEC_PROGRAM=/full/path/to/program \
    OPENEXEC_FILES="space delimited list of full path to files to monitor" \
    program
```

Each time a monitored file will be opened ${PROGRAM} will be executed and
its output will be used as read.

${PROGRAM} is executed with single argument, the real file name that is
being processed.

Monitored file must exists on filesystem, a file with zero size is sufficient.

AUTHOR
------

Alon Bar-Lev &lt;alon.barlev@gmail.com&gt;
