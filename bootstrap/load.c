#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>

#include <fcntl.h>
#include <sys/stat.h>
#include <sys/mman.h>
#include <unistd.h>

void load(const char *filename, void **msg, size_t *size)
{
	int fd = open(filename, O_RDONLY);
	if (fd == -1) {
		perror("open");
		exit(1);
	}
	struct stat statbuf;
	if (fstat(fd, &statbuf) == -1) {
		perror("fstat");
		exit(1);
	}
	*size = statbuf.st_size;
	// printf("len(msg) = %zu\n", statbuf.st_size);
	*msg = mmap(NULL, *size, PROT_READ, MAP_SHARED, fd, 0);
	if (*msg == MAP_FAILED) {
		perror("mmap");
		exit(1);
	}
	close(fd);
}

void _(const char *fmt, ...) {}
