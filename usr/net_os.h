#ifndef __NET_OS_H__
#define __NET_OS_H__

#include <stdbool.h>

struct net_os {
	int fd; /* socket fd */

	int error; /* error while writing to socket */
	bool closed; /* true when stream is closed */

	ssize_t wrote; /**/
	ssize_t copied; /* total data copied to stream */

	ssize_t size; /* size of data[0] */
	char data[0];
};

struct net_os* net_os_alloc(int fd, ssize_t size);
void net_os_free(struct net_os* osp);
bool net_os_closed(struct net_os* osp);
int net_os_last_error(struct net_os* osp);
ssize_t net_os_write(struct net_os* osp, const char* srcp, size_t nbytes);
bool net_os_flush(struct net_os* osp);
bool net_os_has_data(struct net_os* osp);

#endif
