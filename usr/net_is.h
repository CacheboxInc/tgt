#ifndef __IO_BUFFER_H__
#define __IO_BUFFER_H__

#include <stdbool.h>

/*
 * network input stream
 */
struct net_is {
	int fd; /* socket fd */

	int error; /* error while read from socket  */
	bool closed; /* true when stream is closed */

	ssize_t read; /* total data read from network */
	ssize_t copied; /* total data sent to user */

	ssize_t size; /* size of data[0] */
	char data[0];
};

struct net_is* net_is_alloc(int fd, size_t size);
void net_is_free(struct net_is* isp);
bool net_is_closed(struct net_is* isp);
int net_is_last_error(struct net_is* isp);
bool net_is_has_data(struct net_is* isp);
ssize_t net_is_read(struct net_is* isp, char* dstp, size_t nbytes);
#endif
