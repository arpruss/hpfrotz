#ifdef FAKEFILE_TEST
#include <sys/stat.h>
#include <stdio.h>
#define MAX_FILENAME_LENGTH 256
#else
#include <hp165x.h>
#endif
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

typedef struct {
	uint32_t position;
	uint32_t dataSize;
	char write;
	void* rw;
} MYFILE;

typedef struct {
	char filename[MAX_FILENAME_LENGTH+1];
	struct chunk* currentChunk;
	struct chunk firstChunk;
} WriteData_t;

static MYFILE* files[MAX_FILES] = {NULL};

MYFILE* myfopen(const char* name, const char* mode) {
#ifndef FAKEFILE_TEST	
	DirEntry_t d;
#endif
	
	int fd = 0;
	for (fd=0;fd<MAX_FILES;fd++) {
		if (files[fd] == NULL)
			break;
	}
	if (fd >= MAX_FILES)
		return NULL;
	if (mode[0] == 'r') {
		int16_t i = 0;
#ifndef FAKEFILE_TEST		
		while(1) {
			if ( -1 == getDirEntry(i, &d) ) {
				return NULL;
			}
			if (!strcmp(d.name, name)) {
				break;
			}
			i++;
		}		
		uint32_t dataSize = 254*d.numBlocks;
#else
		struct stat st;
		if (stat(name, &st) < 0)
			return NULL;
		uint32_t dataSize = (st.st_size + 253) / 254 * 254;
#endif			
		char* data = malloc(dataSize);
		if (data == NULL)
			return NULL;
		files[fd] = malloc(sizeof(MYFILE));
		if (files[fd] == NULL) {
			free(data);
			return NULL;
		}
#ifndef FAKEFILE_TEST
		int f = openFile(name, DEFAULT_FILE_TYPE, READ_FILE);
		if (f < 0) {
			free(data);
			free(files[fd]);
			files[fd] = NULL;
			return NULL;
		}
		int s = readFile(f, data, -1);
		closeFile(f);
#else
		void* f = fopen(name, "rb");
		if (f == NULL) {
			free(data);
			free(files[fd]);
			files[fd] = NULL;
			return NULL;
		}
		int s = fread(data, 1, dataSize, f); 
		fclose(f);
#endif	
		if (s < 0) {
			free(data);
			free(files[fd]);
			files[fd] = NULL;
			return NULL;
		}
		files[fd]->dataSize = s;
		files[fd]->rw = data;
		files[fd]->position = 0;
		files[fd]->write = 0;
		return files[fd];
	}
	files[fd] = malloc(sizeof(MYFILE));
	if (files[fd] == NULL) {
		return NULL;
	}
	WriteData_t *w = malloc(sizeof(WriteData_t));
	if (w == NULL) {
		free(files[fd]);
		return NULL;
	}
	files[fd]->rw = w;
	strncpy(w->filename,name,MAX_FILENAME_LENGTH+1);
	w->filename[MAX_FILENAME_LENGTH] = 0;
	w->firstChunk.next = NULL;
	w->firstChunk.offset = 0;
	w->currentChunk = &(w->firstChunk);
	files[fd]->position = 0;
	files[fd]->write = 1;
	files[fd]->dataSize = 0;
	return files[fd];
}

int myferror(MYFILE* f) {
	return 0; // TODO
}

int myfclose(MYFILE* f) {
	int e = 0;
	if (f->write) {
		uint32_t size = f->dataSize;
		WriteData_t* w = (WriteData_t*)f->rw;
		struct chunk *chunkP = &(w->firstChunk);
		char first = 1;
#ifndef FAKEFILE_TEST		
		int out = openFile(w->filename, DEFAULT_FILE_TYPE, WRITE_FILE);
#else
		void* out = fopen(w->filename, "wb");
#endif	
		do {
			struct chunk* next = chunkP->next;
#ifndef FAKEFILE_TEST		
			if (0 <= out) 
#else
			if (out != NULL) 
#endif	
			{
				int32_t s = CHUNK_SIZE;
				if ((uint32_t)s > size)
					s = size;
				if (s > 0) {
#ifndef FAKEFILE_TEST					
					if (s != writeFile(out, chunkP->data, s)) {
						e = -1;
						closeFile(out);
						out = -1;
					}
#else
					if (s != fwrite(chunkP->data, 1, s, out)) {
						e = -1;
						fclose(out);
						out = NULL;
					}
#endif	
				}
				size -= s;
			}
			if (! first) {
				free(chunkP);
			}
			else {
				first = 0;
			}
			chunkP = next;
		} while(chunkP != NULL);
#ifndef FAKEFILE_TEST					
		if (0 <= out)
			closeFile(out);
#else
		if (out != NULL)
			fclose(out);
#endif	
	}
	for (int i=0;i<MAX_FILES;i++) {
		if (files[i] == f) {
			files[i] = NULL;
			break;
		}
	}
	free(f->rw);
	free(f);
	
	return e;
}

int myfread(void* ptr, size_t itemSize, size_t count, MYFILE* f) {
	if (f->write)
		return 0;
	if (f->position >= f->dataSize)
		return 0;
	uint32_t size = itemSize * count;
	if (f->position + size >= f->dataSize)
		size = (f->dataSize - f->position) / itemSize * itemSize;
	memcpy(ptr, (char*)f->rw + f->position, size);
	f->position += size;
	return size / itemSize;
}

int myfgetc(MYFILE* f) {
	if (f->write || f->position >= f->dataSize)
		return -1;
	return ((uint8_t*)f->rw)[f->position++];
}

int myfseek(MYFILE* f, long offset, int origin) {
	int32_t pos;
	
	switch(origin) {
		case SEEK_SET:
			pos = offset;
			break;
		case SEEK_CUR:
			pos = f->position + offset;
			break;
		case SEEK_END:
			pos = f->dataSize + offset;
			break;
		default:
			return -1;
	}
	if (pos < 0) {
		f->position = 0;
		return -1;
	}
	else if ((uint32_t)pos > f->dataSize) {
		f->position = f->dataSize;
		return -1;
	}
	f->position = pos;
	return 0;
}

long myftell(MYFILE* f) {
	return f->position;
}

int myfwrite(const void* p, size_t itemSize, size_t count, MYFILE* f) {
	if (! f->write)
		return -1;
	uint32_t size = itemSize * count;
	if (size == 0)
		return 0;
	
	WriteData_t* w = (WriteData_t*)f->rw;

	struct chunk* curChunk = w->currentChunk;
	
	if (f->position < curChunk->offset) {
		curChunk = &w->firstChunk;
	}

	while (curChunk->offset + CHUNK_SIZE <= f->position) {
		if (f->dataSize < curChunk->offset + CHUNK_SIZE) {
			f->dataSize = curChunk->offset + CHUNK_SIZE;
		}
		if (curChunk->next == NULL) {
			curChunk->next = malloc(sizeof(struct chunk));
			if (curChunk->next == NULL) {
				w->currentChunk = curChunk;
				return 0;
			}
			memset(curChunk->next, 0, sizeof(struct chunk));
			curChunk->next->offset = curChunk->offset + CHUNK_SIZE;
		}			
		curChunk = curChunk->next;
	}
	
	if (f->dataSize < f->position)
		f->dataSize = f->position;
	
	uint32_t wrote = 0;
	
	while (0 < size) {
		uint32_t posInChunk = f->position - curChunk->offset;
		uint32_t toCopy = CHUNK_SIZE - posInChunk;
		if (toCopy > size)
			toCopy = size;
		memcpy(curChunk->data + posInChunk, p, toCopy);
		f->position = curChunk->offset + posInChunk + size;
		if (f->position > f->dataSize)
			f->dataSize = f->position;
		size -= toCopy;
		wrote += toCopy;
		p = (char*)p + toCopy;
		if (0 < size) {
			if (curChunk->next == NULL) {
				curChunk->next = malloc(sizeof(struct chunk));
				if (curChunk->next == NULL) {
					w->currentChunk = curChunk;
					return wrote;
				}
				memset(curChunk->next, 0, sizeof(struct chunk));
				curChunk->next->offset = curChunk->offset + CHUNK_SIZE;
			}
			curChunk = curChunk->next;
		}
	}
	
	w->currentChunk = curChunk;
	return wrote;
}

int myfputc(int c, MYFILE* f) {
	char ch = c;
	if (1 == myfwrite(&ch, 1, 1, f))
		return 0;
	else
		return -1;
}
	