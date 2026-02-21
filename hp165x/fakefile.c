#undef FILE
#undef fopen
#undef fwrite
#undef fgetc
#undef fputc
#undef ftell
#undef fseek
#undef fread
#undef ferror
#undef fclose

#include <hp165x.h>
#include <hpposix.h>
#include "stdio.h"
#include <stdint.h>
#include <stddef.h>
#include <string.h>
#include <wchar.h>
#include <malloc.h>

#define CHUNK_SIZE 4096

#define MAX_FILES 6
#define DEFAULT_FILE_TYPE 1

#ifndef SEEK_SET
#define SEEK_SET 0
#define SEEK_CUR 1
#define SEEK_END 2
#endif

/* The files are entirely stored in memory. 
   This is a kludge to get around the fact that I don't know how to seek
   within the HP's files, and the HP has a limit of one open file at a time. 
   But, hey, there is 1mb of RAM. */

struct chunk {
	uint32_t offset;
	struct chunk* next;
	char data[CHUNK_SIZE];
};

static char* basePtr = (char*)0x10; 
typedef char MYFILE; // MYFILE* is a pretend pointer
#define DECODE_FD(p) ((char*)p - basePtr)
#define ENCODE_FD(fd) (basePtr+fd)

MYFILE* myfopen(const char* name, const char* mode) {
	int fd = open(name, mode[0] == 'w' ? O_WRONLY : O_RDONLY);
	if (fd < 0)
		return NULL;
	return basePtr + fd;
}

int myferror(MYFILE* f) {
	(void)f;
	return 0; // TODO
}

int myfclose(MYFILE* f) {
	return close(DECODE_FD(f));
}

int myfread(void* ptr, size_t itemSize, size_t count, MYFILE* f) {
	size_t size = itemSize * count;
	int didRead = read(DECODE_FD(f), ptr, size);
	if (didRead < 0)
		return -1;
	return didRead / itemSize;
}

int myfgetc(MYFILE* f) {
	uint8_t c;
	if (1 == read(DECODE_FD(f), &c, 1))
		return c;
	else
		return -1;
}

int myfseek(MYFILE* f, long offset, int origin) {
	return lseek(DECODE_FD(f), offset, origin);
}

long myftell(MYFILE* f) {
	return lseek(DECODE_FD(f), 0, SEEK_CUR);
}

int myfwrite(const void* p, size_t itemSize, size_t count, MYFILE* f) {
	size_t size = itemSize * count;
	int didWrite = write(DECODE_FD(f), p, size);
	if (didWrite < 0)
		return -1;
	return didWrite / itemSize;
}

int myfputc(int c, MYFILE* f) {
	char ch = c;
	if (1 == write(DECODE_FD(f),&ch,1))
		return 0;
	else
		return -1;
}
	