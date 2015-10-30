CC = gcc
WARN_CFLAGS = -Wall -Wextra -pedantic

TEST_FILE = non-existence-file-121312421.tmp

all:	openexec.so

clean:
	rm -f openexec.so
	rm -f "$(TEST_FILE)"

openexec.so:	openexec.c openexec_hooks.c openexec.version
	$(CC) -D_GNU_SOURCE -D_FILE_OFFSET_BITS=64 -D_LARGE_FILE $(WARN_CFLAGS) $(CFLAGS) -shared -fPIC -fpic -o openexec.so openexec.c openexec_hooks.c -Wl,-soname,openexec.so -Wl,--error-unresolved-symbols -Wl,--version-script=openexec.version $(LDFLAGS) -ldl

check:	all
	echo test > $(TEST_FILE)
	[ "$$(cat "$$(pwd)/$(TEST_FILE)")" = "test" ]
	[ "$$(LD_PRELOAD="$$(pwd)/openexec.so" cat "$$(pwd)/$(TEST_FILE)")" = "test" ]
	[ "$$(LD_PRELOAD="$$(pwd)/openexec.so" OPENEXEC_PROGRAM=/bin/echo cat "$$(pwd)/$(TEST_FILE)")" = "test" ]
	[ "$$(LD_PRELOAD="$$(pwd)/openexec.so" OPENEXEC_FILES="$(TEST_FILE)" cat "$$(pwd)/$(TEST_FILE)")" = "test" ]
	[ "$$(LD_PRELOAD="$$(pwd)/openexec.so" OPENEXEC_FILES="$$(pwd)/$(TEST_FILE)" OPENEXEC_PROGRAM=/bin/echo cat "$$(pwd)/$(TEST_FILE)")" = "$$(pwd)/$(TEST_FILE)" ]
	[ "$$(LD_PRELOAD="$$(pwd)/openexec.so" OPENEXEC_FILES="$$(pwd)/$(TEST_FILE)" OPENEXEC_PROGRAM=/bin/echo cat "$(TEST_FILE)")" = "$$(pwd)/$(TEST_FILE)" ]
	[ "$$(LD_PRELOAD="$$(pwd)/openexec.so" OPENEXEC_FILES="$(TEST_FILE)" OPENEXEC_PROGRAM=/bin/echo cat "$$(pwd)/$(TEST_FILE)")" = "$$(pwd)/$(TEST_FILE)" ]
