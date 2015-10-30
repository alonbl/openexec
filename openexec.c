#include <sys/types.h>
#include <sys/resource.h>
#include <sys/wait.h>
#include <fcntl.h>
#include <dlfcn.h>
#include <unistd.h>
#include <errno.h>
#include <limits.h>
#include <string.h>
#include <stdlib.h>
#include <stdio.h>

static int (*_orig_open)(const char *pathname, int flags, mode_t mode);
static int (*_orig_open64)(const char *pathname, int flags, mode_t mode);
static int (*_orig_close)(int fd);
static const char *_program;
static char _files[100][PATH_MAX];
static struct {
	int fd;
	pid_t pid;
} _fds[1024];
static int _enabled;

static int _hook(void *t, const char * const name) {
	void *p = dlsym(RTLD_NEXT, name);
	if (p == NULL) {
		return 0;
	}
	memcpy(t, &p, sizeof(p));
	return 1;
}

static int _setup_env() {
	const char * envfiles = getenv("OPENEXEC_FILES");
	int i=0;

	while(envfiles != NULL && i < (int)(sizeof(_files) / sizeof(_files[0]))) {
		const char *p = strchr(envfiles, ' ');
		const char *next = p;
		char name[PATH_MAX];
		if (p == NULL) {
			p = envfiles + strlen(envfiles);
		} else {
			next++;
		}
		if ((size_t)(p - envfiles) < sizeof(name) - 1) {
			strncpy(name, envfiles, p - envfiles);
			name[p - envfiles] = '\0';
			if (realpath(name, _files[i]) != NULL) {
				i++;
			}
		}
		envfiles = next;
	}
	_program = getenv("OPENEXEC_PROGRAM");
	return _program != NULL && i > 0;
}

__attribute__ ((__constructor__))
static void _init(void) {
	_hook(&_orig_open, "open");
	_hook(&_orig_open64, "open64");
	_hook(&_orig_close, "close");
	_enabled = _setup_env();
}

static int _our_file(const char * const f) {
	int i=0;
	while (_files[i][0] != '\0') {
		if (strcmp(_files[i], f) == 0) {
			return 1;
		}
		i++;
	}
	return 0;
}

static int _open_process(const char *pathname) {
	int fds[2] = {-1, -1};
	int pid;
	struct rlimit rlim;
	int fd_index = -1;
	int ret = -1;
	int i;

	for (i = 0;i < (int)(sizeof(_fds)/sizeof(_fds[0])); i++) {
		if (_fds[i].fd == 0) {
			fd_index = i;
			break;
		}
	}
	if (fd_index == -1) {
		errno = EPERM;
		goto cleanup;
	}

	if (getrlimit(RLIMIT_NOFILE, &rlim) == -1) {
		goto cleanup;
	}
	if (pipe(fds) == -1) {
		goto cleanup;
	}
	if ((pid = fork()) == -1) {
		goto cleanup;
	}
	if (pid == 0) {
		int null = open("/dev/null", O_RDWR, 0);
		rlim_t i;
		if (null == -1) {
			_exit(1);
		}
		if (
			dup2(null, 0) == -1 ||
			dup2(fds[1], 1) == -1 ||
			dup2(null, 2) == -1
		) {
			_exit(1);
		}
		for (i=3;i<rlim.rlim_cur;i++) {
			close(i);
		}
		execl(_program, _program, pathname, NULL);
		_exit(1);
	} else {
		_fds[fd_index].fd = fds[0];
		_fds[fd_index].pid = pid;
		fds[0] = -1;
		errno = 0;
		ret = _fds[fd_index].fd;
	}

cleanup:
	if (fds[0] != -1) {
		close(fds[0]);
	}
	if (fds[1] != -1) {
		close(fds[1]);
	}
	return ret;
}


static int _openexec_open(int (*orig)(const char *pathname, int flags, mode_t mode), const char *pathname, int flags, mode_t mode) {
	char real[PATH_MAX];
	if (_enabled && realpath(pathname, real) != NULL && _our_file(real)) {
		return _open_process(real);
	} else {
		if (orig == NULL) {
			errno = EACCES;
			return -1;
		} else {
			return orig(pathname, flags, mode);
		}
	}
}

int openexec_open(const char *pathname, int flags, mode_t mode) {
	return _openexec_open(_orig_open, pathname, flags, mode);
}

int openexec_open64(const char *pathname, int flags, mode_t mode) {
	return _openexec_open(_orig_open64, pathname, flags, mode);
}

int openexec_close(int fd) {
	int ret = -1;
	if (_orig_close == NULL) {
		errno = EIO;
	} else {
		ret = _orig_close(fd);
	}

	if (_enabled) {
		int i;
		for (i=0;i<(int)(sizeof(_fds) / sizeof(_fds[0])); i++) {
			if (_fds[i].fd == fd) {
				waitpid(_fds[i].pid, NULL, 0);
				memset(&_fds[i], 0, sizeof(_fds[0]));
				break;
			}
		}
	}

	return ret;
}
