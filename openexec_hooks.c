#include <sys/types.h>
extern int openexec_open(const char *pathname, int flags, mode_t mode);
int open(const char *pathname, int flags, mode_t mode) {
	return openexec_open(pathname, flags, mode);
}
extern int openexec_open64(const char *pathname, int flags, mode_t mode);
int open64(const char *pathname, int flags, mode_t mode) {
	return openexec_open64(pathname, flags, mode);
}
extern int openexec_close(int fd);
int close(int fd) {
	return openexec_close(fd);
}
