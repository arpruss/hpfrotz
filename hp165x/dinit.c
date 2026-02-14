/*
 * dinit.c - Dumb interface, initialization
 *
 * This file is part of Frotz.
 *
 * Frotz is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * Frotz is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA
 * Or visit http://www.fsf.org/
 */

#include <hp165x.h>
#include <stdarg.h>

#include "dfrotz.h"
#include "dblorb.h"

extern f_setup_t f_setup;
extern z_header_t z_header;

static void print_version(void);
char pick_file(char* name, char** extList, int numExts);
static char* storyExts[] = {
	".dat",
	".z1",
	".z2",
	".z3",
	".z4",
	".z5",
	".z6",
	".z7",
	".z8"	
};


static int user_random_seed = -1;
static bool plain_ascii = FALSE;

bool quiet_mode;
bool do_more_prompts;

long int timeTenths() {
	return getVBLCounter()/6;
}


/*
 * os_process_arguments
 *
 * Handle command line switches.
 * Some variables may be set to activate special features of Frotz.
 *
 */
void os_process_arguments(int _argc, char *_argv[])
{
	int c, num;
	char *p = NULL;
	char *format_orig = NULL;
	
	static char story[MAX_FILE_NAME+1];
	hp_set_window();
	if (!pick_file(story,storyExts,sizeof(storyExts)/sizeof(*storyExts))) {
		putText("Please enter a filename: ");
		short n = getText(story, MAX_FILE_NAME+1);
		putText("\n");
		if (n <= 0) {
			putText("Goodbye!");
			os_quit(0);
		}
	}
	hp_clear_window();
		
	int argc = 2;
	char* argv[2] = {"hpfrotz", story};

	zoptarg = NULL;

	do_more_prompts = TRUE;
	quiet_mode = FALSE;

	f_setup.format = FORMAT_NORMAL;

	/* Save the story file name */
	f_setup.story_file = strdup(argv[zoptind]);

#ifdef NO_BASENAME
	f_setup.story_name = strdup(f_setup.story_file);
#else
	f_setup.story_name = strdup(basename(argv[zoptind]));
#endif
#ifndef NO_BLORB
	if (argv[zoptind+1] != NULL)
		f_setup.blorb_file = strdup(argv[zoptind+1]);
#endif

	if (!quiet_mode) {
		printf("Loading %s.\n", f_setup.story_file);

#ifndef NO_BLORB
	if (f_setup.blorb_file != NULL)
		printf("Also loading %s.\n", f_setup.blorb_file);
#endif
	}

	/* Now strip off the extension */
	p = strrchr(f_setup.story_name, '.');
	if ( p != NULL )
		*p = '\0';	/* extension removed */

#ifndef NO_SCRIPT
	/* Create nice default file names */
	f_setup.script_name = malloc((strlen(f_setup.story_name) + strlen(EXT_SCRIPT) + 1) * sizeof(char));
	memcpy(f_setup.script_name, f_setup.story_name, (strlen(f_setup.story_name) + strlen(EXT_SCRIPT)) * sizeof(char));
	strncat(f_setup.script_name, EXT_SCRIPT, strlen(EXT_SCRIPT)+1);

	f_setup.command_name = malloc((strlen(f_setup.story_name) + strlen(EXT_COMMAND) + 1) * sizeof(char));
	memcpy(f_setup.command_name, f_setup.story_name, (strlen(f_setup.story_name) + strlen(EXT_COMMAND)) * sizeof(char));
	strncat(f_setup.command_name, EXT_COMMAND, strlen(EXT_COMMAND)+1);
#endif	

	if (!f_setup.restore_mode) {
		f_setup.save_name = malloc((strlen(f_setup.story_name) + strlen(EXT_SAVE) + 1) * sizeof(char));
		memcpy(f_setup.save_name, f_setup.story_name, (strlen(f_setup.story_name) + strlen(EXT_SAVE)) * sizeof(char));
		strncat(f_setup.save_name, EXT_SAVE, strlen(EXT_SAVE) + 1);
	} else { /* Set our auto load save as the name save */
		f_setup.save_name = malloc((strlen(f_setup.tmp_save_name) + strlen(EXT_SAVE) + 1) * sizeof(char));
                memcpy(f_setup.save_name, f_setup.tmp_save_name, (strlen(f_setup.tmp_save_name) + strlen(EXT_SAVE)) * sizeof(char));
                free(f_setup.tmp_save_name);
	}

	f_setup.aux_name = malloc((strlen(f_setup.story_name) + strlen(EXT_AUX) + 1) * sizeof(char));
	memcpy(f_setup.aux_name, f_setup.story_name, (strlen(f_setup.story_name) + strlen(EXT_AUX)) * sizeof(char));
	strncat(f_setup.aux_name, EXT_AUX, strlen(EXT_AUX) + 1);
} /* os_process_arguments */


void os_init_screen(void)
{
	if (z_header.version == V3 && f_setup.tandy)
		z_header.config |= CONFIG_TANDY;

	if (z_header.version >= V5 && f_setup.undo_slots == 0)
		z_header.flags &= ~UNDO_FLAG;

	z_header.screen_rows = getTextRows();
	z_header.screen_cols = getTextColumns();

	/* Use the ms-dos interpreter number for v6, because that's the
	 * kind of graphics files we understand.  Otherwise, use DEC.  */
	if (f_setup.interpreter_number == INTERP_DEFAULT)
		z_header.interpreter_number = z_header.version ==
			6 ? INTERP_MSDOS : INTERP_DEC_20;
	else
		z_header.interpreter_number = f_setup.interpreter_number;

	z_header.interpreter_version = 'F';

	dumb_init_input();
	dumb_init_output();
	dumb_init_pictures();
} /* os_init_screen */


int os_random_seed (void)
{
	if (user_random_seed == -1)	/* Use the epoch as seed value */
		return (timeTenths() & 0x7fff);
	return user_random_seed;
} /* os_random_seed */


/*
 * os_quit
 *
 * Immediately and cleanly exit, passing along exit status.
 *
 */
void os_quit(int status)
{
	waitSeconds(1);
	reload();
} /* os_quit */


void os_restart_game (int UNUSED (stage)) {}


/*
 * os_warn
 *
 * Display a warning message and continue with the game.
 *
 */
void os_warn (const char *s, ...)
{
	va_list m;
	int style;

	os_beep(BEEP_HIGH);
	style = os_get_text_style();
	os_set_text_style(BOLDFACE_STYLE);
	fprintf(stderr, "Warning: ");
	os_set_text_style(NORMAL_STYLE);
	va_start(m, s);
	vfprintf(stderr, s, m);
	va_end(m);
	fprintf(stderr, "\n");
	os_set_text_style(style);
	return;
} /* os_warn */


/*
 * os_fatal
 *
 * Display error message and exit program.
 *
 */
void os_fatal (const char *s, ...)
{
	dumb_show_screen(FALSE);
	fprintf(stderr, "\nFatal error: %s\n", s);
	if (f_setup.ignore_errors)
		fprintf(stderr, "Continuing anyway...\n");
	else
		os_quit(EXIT_FAILURE);
} /* os_fatal */


FILE *os_load_story(void)
{
#ifndef NO_BLORB
	FILE *fp;

	switch (dumb_blorb_init(f_setup.story_file)) {
	case bb_err_NoBlorb:
		/* printf("No blorb file found.\n\n"); */
		break;
	case bb_err_Format:
		printf("Blorb file loaded, but unable to build map.\n\n");
		break;
	case bb_err_NotFound:
		printf("Blorb file loaded, but lacks executable chunk.\n\n");
		break;
	case bb_err_None:
		/* printf("No blorb errors.\n\n"); */
		break;
	}

	fp = fopen(f_setup.story_file, "rb");

	/* Is this a Blorb file containing Zcode? */
	if (f_setup.exec_in_blorb)
		fseek(fp, blorb_res.data.startpos, SEEK_SET);

	return fp;
#else
	return fopen(f_setup.story_file, "rb");
#endif
} /* os_load_story */


/*
 * Seek into a storyfile, either a standalone file or the
 * ZCODE chunk of a blorb file.
 */
int os_storyfile_seek(FILE * fp, long offset, int whence)
{
#ifndef NO_BLORB
	/* Is this a Blorb file containing Zcode? */
	if (f_setup.exec_in_blorb) {
		switch (whence) {
		case SEEK_END:
			return fseek(fp, blorb_res.data.startpos + blorb_res.length + offset, SEEK_SET);
			break;
		case SEEK_CUR:
			return fseek(fp, offset, SEEK_CUR);
			break;
		case SEEK_SET:
			/* SEEK_SET falls through to default */
		default:
			return fseek(fp, blorb_res.data.startpos + offset, SEEK_SET);
			break;
		}
	} else
		return fseek(fp, offset, whence);
#else
	return fseek(fp, offset, whence);
#endif
} /* os_storyfile_seek */


/*
 * Tell the position in a storyfile, either a standalone file
 * or the ZCODE chunk of a blorb file.
 */
int os_storyfile_tell(FILE * fp)
{
#ifndef NO_BLORB
	/* Is this a Blorb file containing Zcode? */
	if (f_setup.exec_in_blorb)
		return ftell(fp) - blorb_res.data.startpos;
	else
		return ftell(fp);
#else
	return ftell(fp);
#endif
} /* os_storyfile_tell */

static void intro(void)
{
	printf("FROTZ V%s - HP165x interface.\n", VERSION);
	return;
} /* usage */


void os_init_setup(void)
{
	patchVBL();
	*SCREEN_MEMORY_CONTROL = BACKGROUND;
	fillScreen();
	*SCREEN_MEMORY_CONTROL = FOREGROUND;
	setTextColors(WRITE_WHITE,WRITE_BLACK);
	setTextXY(0,0);
	initKeyboard(1);
	intro();
} /* os_init_setup */


static void print_version(void)
{
	printf("FROTZ V%s     Dumb interface.\n", VERSION);
	printf("Commit date:    %s\n", GIT_DATE);
	printf("Git commit:     %s\n", GIT_HASH);
	printf("Notes:          %s\n", RELEASE_NOTES);
	printf("  Frotz was originally written by Stefan Jokisch.\n");
	printf("  It complies with standard 1.1 of the Z-Machine Standard.\n");
	printf("  It was ported to Unix by Galen Hazelwood.\n");
	printf("  It is distributed under the GNU General Public License version 2 or\n");
	printf("    (at your option) any later version.\n");
	printf("  This software is offered as-is with no warranty or liability.\n");
	printf("  The core and dumb port are maintained by David Griffith.\n");
	printf("  Frotz's homepage is https://661.org/proj/if/frotz.\n\n");
	return;
} /* print_version */
