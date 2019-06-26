#include <string.h>
#include <unistd.h>
#include <errno.h>
#include <stdlib.h>
#include <assert.h>

#include "net_os.h"

static inline ssize_t data_free_space(struct net_os* osp)
{
	return osp->size - osp->copied;
}

static inline ssize_t data_available(struct net_os* osp) {
	return osp->copied - osp->wrote;
}

static ssize_t data_copy(struct net_os* osp, const void* srcp, ssize_t to_copy)
{
#define min(a, b) ((a) < (b) ? (a) : (b))
	to_copy = min(to_copy, data_free_space(osp));
	memcpy(osp->data + osp->copied, srcp, to_copy);
	osp->copied += to_copy;
	return to_copy;
}

static inline size_t socket_write(int fd, const char* srcp, ssize_t to_write)
{
	size_t wrote = 0;
	while (to_write) {
		ssize_t rc = write(fd, srcp, to_write);
		if (rc < 0) {
			if (errno == EINTR || errno == EAGAIN || errno == EWOULDBLOCK) {
				continue;
			}
			return wrote;
		}
		wrote += rc;
		srcp += rc;
		to_write -= rc;
	}
	return wrote;
}

static inline ssize_t data_flush(struct net_os* osp)
{
	if (net_os_closed(osp)) {
		errno = ECONNRESET;
		return 0;
	}

	ssize_t to_write = data_available(osp);
	if (to_write == 0) {
		return 0;
	}

	ssize_t wrote = socket_write(osp->fd, osp->data + osp->wrote, to_write);
	if (wrote != to_write) {
		osp->error = errno;
		if (osp->error != EAGAIN && osp->error != EINTR && osp->error != EWOULDBLOCK) {
			osp->closed = true;
		}
		osp->wrote += wrote;
	} else {
		osp->wrote = 0;
		osp->copied = 0;
	}
	return wrote;
}

struct net_os* net_os_alloc(int fd, ssize_t size)
{
	struct net_os* osp;
	osp = calloc(1, sizeof(*osp) + size);
	if (osp == NULL) {
		return NULL;
	}
	osp->fd = fd;
	osp->size = size;
	return osp;
}

void net_os_free(struct net_os* osp)
{
	free(osp);
}

bool net_os_closed(struct net_os* osp)
{
	return osp->closed;
}

int net_os_last_error(struct net_os* osp)
{
	return osp->error;
}

ssize_t net_os_write(struct net_os* osp, const char* srcp, size_t nbytes)
{
	ssize_t avail = data_free_space(osp);
	if (avail < nbytes) {
		ssize_t to_write = data_available(osp);
		ssize_t wrote = data_flush(osp);
		if (wrote != to_write || data_free_space(osp) != osp->size) {
			return 0;
		}
		assert(data_available(osp) == 0 && data_free_space(osp) == osp->size);
	}
	assert(data_free_space(osp) >= avail);
	return data_copy(osp, srcp, nbytes);
}

bool net_os_flush(struct net_os* osp)
{
	ssize_t to_write = data_available(osp);
	return to_write == data_flush(osp);
}

bool net_os_has_data(struct net_os* osp)
{
	return data_available(osp) != 0;
}
