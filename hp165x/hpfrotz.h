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

#include <sys/param.h>

#define fprintf(stream,...) printf(__VA_ARGS__)
#define vfprintf(stream,s,m) printf(s,m)

#ifndef MAX
#define MAX(x,y) ((x)>(y)) ? (x) : (y)
#endif
#ifndef MIN
#define MIN(x,y) ((x)<(y)) ? (x) : (y)
#endif

/* from ../common/setup.h */
extern f_setup_t f_setup;

extern bool quiet_mode;

/* From input.c.  */
bool is_terminator (zchar);

/* dumb-input.c */
void hp_init_input(void);

/* dumb-output.c */
void hp_init_output(void);
bool dumb_output_handle_setting(const char *setting, bool show_cursor,
				bool startup);
void dumb_show_screen(bool show_cursor);
void dumb_show_prompt(bool show_cursor, char line_type);
void dumb_dump_screen(void);
void dumb_display_user_input(char *);
void dumb_discard_old_input(int num_chars);
void dumb_elide_more_prompt(void);
void dumb_set_picture_cell(int row, int col, zchar c);

void hp_set_window(void);
void hp_clear_window(void);

/* dumb-pic.c */
bool dumb_init_pictures(void);

#define FOREGROUND WRITE_WHITE
#define BACKGROUND WRITE_BLACK

#endif
