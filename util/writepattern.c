#include <errno.h>
#include <inttypes.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/eventfd.h>
#include <sys/epoll.h>
#include <assert.h>
#include <stdbool.h>
#include <pthread.h>
#include "cksum.h"
#include <unistd.h>

__u16 get_cksum(unsigned char *bufp, size_t length) { 
	return crc_t10dif(bufp, length);
}


int main(int argc, char *argv[])
{
	FILE * fp;
	char * line = NULL;
	size_t len = 0;
	ssize_t read;
	size_t    length;
	uint64_t  offset;
	char      *bufp = NULL;
	__u16 cksum = 0;

	char *buf_tok = NULL;
	int count = 0, rc = 0, dev_fd = 0, pattern = 2;

	if (argc < 3) {
		printf("Usage error, Usage : writepattern <filename> <devname>\n");
		return 1;
	}

	printf("Input file is : %s\n", argv[1]);
	fp = fopen(argv[1], "r");
	if (fp == NULL) {
		return 1;
	}

	printf("Dev file is : %s\n", argv[2]);
	dev_fd = open(argv[2], O_WRONLY);
	if (dev_fd < 0) {
		printf("Usage error, Usage : cksum <filename> <devname>\n");
		fclose(fp);
		return 1;
	}

	while ((read = getline(&line, &len, fp)) != -1) {
		if(!strlen(line)) {
			if (line)
				free(line);
			line = NULL;
			printf("Invalid line from input file:\n");
			continue;
		}

		printf("Line is :%s", line);
		buf_tok = NULL;
		buf_tok = strtok(line, ":");
		count = 0;
		while (buf_tok != NULL) {
			if (count == 0) {
				offset = atol(buf_tok);
				count++;
			} else if (count == 1) {
				length = atol(buf_tok);
				count++;
			}
			buf_tok = strtok(NULL, ":");
		}

		if (count != 2) {
			printf("Invalid input format, expected is offset:length:cksum\n");
			rc = 1;
			break;
		}

		/* write to device */
		printf("Trying to write at Offset:%"PRIu64" length:%lu\n", offset, length);
		bufp = (char *) malloc(length);
		memset(bufp, pattern, length);
		if (pwrite(dev_fd, bufp, length, offset) != length) {
			printf("Unable to write at Offset:%"PRIu64" length:%lu\n", offset, length);
			rc = 1;
			break;

		}

		cksum = get_cksum((unsigned char *)bufp, length);
		printf("Offset:%"PRIu64" length:%lu cksum:%"PRIu16"\n", offset, length, cksum);

		free(line);
		line = NULL;
		free(bufp);
		bufp = NULL;
		pattern++;
	}

	if(line)
		free(line);

	if (bufp)
		free(bufp);

	fclose(fp);
	close(dev_fd);
	return rc;
}
