#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <assert.h>

#include "log.h"
#include "net_is.h"

static inline ssize_t data_available(struct net_is* isp)
{
	return isp->read - isp->copied;
}

static ssize_t data_copy(struct net_is* isp, void* dstp, ssize_t to_copy)
{
#define min(a, b) ((a) < (b) ? (a) : (b))
	to_copy = min(to_copy, data_available(isp));
	if (to_copy == 0) {
		return 0;
	}
	memcpy(dstp, isp->data + isp->copied, to_copy);
	isp->copied += to_copy;
	return to_copy;
}

static inline ssize_t safe_read(int fd, char* datap, ssize_t to_read)
{
	ssize_t copied = 0;
	while (to_read > 0) {
		ssize_t rc = read(fd, datap, to_read);
		if (rc <= 0) {
			if (rc == 0) {
				errno = ECONNRESET;
				return 0;
			}
			return copied;
		}
		datap += rc;
		copied += rc;
		to_read -= rc;
	}
	return copied;
}

static inline ssize_t socket_read(struct net_is* isp)
{
	if (net_is_closed(isp)) {
		errno = ECONNRESET;
		return 0;
	}
	assert(data_available(isp) == 0);

	isp->copied = 0;
	isp->read = safe_read(isp->fd, isp->data, isp->size);
	if (isp->read == 0) {
		isp->error = errno;
		if (isp->error != EAGAIN &&
				isp->error != EWOULDBLOCK &&
				isp->error != EINTR) {
			isp->closed = true;
		} else {
			errno = EAGAIN;
		}
	}
	return isp->read;
}

struct net_is* net_is_alloc(int fd, size_t size)
{
	struct net_is* isp;
	isp = calloc(1, size + sizeof(*isp));
	if (isp == NULL) {
		return isp;
	}
	isp->fd = fd;
	isp->size = size;
	return isp;
}

void net_is_free(struct net_is* isp)
{
	free(isp);
}

bool net_is_closed(struct net_is* isp)
{
	return isp->closed;
}

int net_is_last_error(struct net_is* isp)
{
	return isp->error;
}

bool net_is_has_data(struct net_is* isp)
{
	if (data_available(isp)) {
		return true;
	}
	(void) socket_read(isp);
	return data_available(isp);
}

ssize_t net_is_read(struct net_is* isp, char* dstp, size_t nbytes)
{
	ssize_t copied = data_copy(isp, dstp, nbytes);
	if (nbytes == copied) {
		errno = 0;
		return copied;
	}
	assert(copied < nbytes && data_available(isp) == 0);

	if (socket_read(isp) == 0) {
		return copied;
	}
	return copied + data_copy(isp, dstp + copied, nbytes - copied);
}
