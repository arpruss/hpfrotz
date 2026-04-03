/*
 * dfrotz.h
 *
 * Frotz os functions for a standard C library and a dumb terminal.
 * Now you can finally play Zork Zero on your Teletype.
 *
 * Copyright 1997, 1998 Alembic Petrofsky <alembic@petrofsky.berkeley.ca.us>.
 * Any use permitted provided this notice stays intact.
 */

#ifndef HP_FROTZ_H
#define HP_FROTZ_H

#include <hp165x.h>

#include <sys/param.h>
#include "../common/frotz.h"

#ifndef NO_BASENAME
#include <libgen.h>
#endif

#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <string.h>
#include <ctype.h>
#include <time.h>

#define fprintf(stream,...) printf(__VA_ARGS__)
#define vfprintf(stream,s,m) printf(s,m)

#ifndef MAX
#define MAX(x,y) ((x)>(y)) ? (x) : (y)
#endif
#ifndef MIN
#define MIN(x,y) ((x)<(y)) ? (x) : (y)
#endif

#define WIN_X1 8
#define WIN_Y1 4
#define WIN_X2 72
#define WIN_Y2 22

/* from ../common/setup.h */
extern f_setup_t f_setup;

extern bool quiet_mode;

bool is_terminator (zchar);

void hp_init_input(void);

void hp_init_output(void);

void hp_set_window(void);
void hp_clear_window(void);
char right_type(DirEntry_t* d, char** extList, int numExts);
void hp_init_pictures(void);
void hp_close_pictures(void);

#define FOREGROUND WRITE_WHITE
#define BACKGROUND WRITE_BLACK

#define INPUT_STOP (-100)
#define INPUT_HELP (-101)
#define INPUT_UNDO (-102)
#define INPUT_RESTART (-103)
#define INPUT_DEBUG (-104)
#define INPUT_SEED (-105)
#define INPUT_MOUSE_CLICK (-106)
#define INPUT_MOUSE_DOUBLE_CLICK (-107)

uint8_t pick_file(char* name, char** extData, short numExts, int flag);
int16_t getTextContinuable(char* _buffer, uint16_t _maxSize, int timeoutTicks, bool continued, bool cancelable);
struct IFhd {
	uint16_t release;
	char serial[6];
	uint16_t checksum;
};
struct IFhd* getSaveHeader(const char* save);

#endif
