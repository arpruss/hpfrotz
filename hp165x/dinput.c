/*
 * dinput.c - Dumb interface, input functions
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
#include <string.h>
#include <assert.h>
#include <stdio.h>

#include "dfrotz.h"

#define MAX_PICK_FILES 36
#define PICK_FILES_PER_LINE 4

extern f_setup_t f_setup;

static char runtime_usage[] =
	"DUMB-FROTZ runtime help:\n"
	"  General Commands:\n"
	"    \\help    Show this message.\n"
	"    \\set     Show the current values of runtime settings.\n"
	"    \\s       Show the current contents of the whole screen.\n"
/* 	"    \\d       Discard the part of the input before the cursor.\n" */
	"    \\wN      Advance clock N/10 seconds, possibly causing the current\n"
	"                and subsequent inputs to timeout.\n"
	"    \\w       Advance clock by the amount of real time since this input\n"
	"                started (times the current speed factor).\n"
	"    \\t       Advance clock just enough to timeout the current input\n"
	"  Output Compression Settings:\n"
	"    \\cn      none: show whole screen before every input.\n"
	"    \\cm      max: show only lines that have new nonblank characters.\n"
	"    \\cs      spans: like max, but emit a blank line between each span of\n"
	"                screen lines shown.\n"
	"    \\chN     Hide top N lines (orthogonal to above modes).\n"
	"  Misc Settings:\n"
	"    \\sfX     Set speed factor to X.  (0 = never timeout automatically).\n"
	"    \\mp      Toggle use of MORE prompts\n"
	"    \\pr      Toggle display of line numbers.\n"
	"    \\lt      Toggle display of the line type identification chars.\n"
	"    \\vb      Toggle visual bell.\n"
	"    \\pb      Toggle display of picture outline boxes.\n"
	"    (Toggle commands can be followed by a 1 or 0 to set value ON or OFF.)\n"
	"  Character Escapes:\n"
	"    \\\\  backslash    \\#  backspace    \\[  escape    \\_  return\n"
	"    \\< \\> \\^ \\.  cursor motion        \\1 ..\\0  f1..f10\n"
	"    \\D ..\\X   Standard Frotz hotkeys.  Use \\H (help) to see the list.\n"
	"  Line Type Identification Characters:\n"
	"    Input lines:\n"
	"      untimed  timed\n"
	"      >        T      A regular line-oriented input\n"
	"      )        t      A single-character input\n"
	"      }        D      A line input with some input before the cursor.\n"
	"                         (Use \\d to discard it.)\n"
	"    Output lines:\n"
	"      ]     Output line that contains the cursor.\n"
	"      .     A blank line emitted as part of span compression.\n"
	"            (blank) Any other output line.\n"
;

static int speed_100 = 100;

enum input_type {
	INPUT_CHAR,
	INPUT_LINE,
	INPUT_LINE_CONTINUED,
};

int getTextContinuable(char* _buffer, uint16_t maxSize, int timeoutTicks, char continued);
char pick_file(char* name, char** extData, int numExts);
long timeTenths(void);

static uint16_t startX;
static uint16_t startY;
static uint16_t cols;
static uint16_t rows;
static char* buffer;
static uint16_t cursor;
static uint16_t length;
static char singleLine;

static void setXYFromOffset(uint16_t offset) {
	uint16_t p = startX + offset;
	uint16_t row = startY + p / cols;
	uint16_t col = p % cols;
	if (row >= rows) {
		if (singleLine) {
			row = rows-1;
			col = cols-1;
		}
		else {
			uint16_t delta = row - rows + 1;
			scrollTextUp(delta);
			row = rows-1;
			startY -= delta;
		}
	}
	setTextXY(col,row);
}

static void drawFrom(uint16_t offset) {
	setXYFromOffset(offset);
	startY -= putText(buffer+offset);
}

static void clearCursor(void) {
	setXYFromOffset(cursor);
	setScrollMode(0);
	if (cursor<length)
		putChar(buffer[cursor]);
	else
		putChar(' ');
	setScrollMode(1);
}

static void drawCursor(void) {
	setXYFromOffset(cursor);
	setScrollMode(0);
	setTextReverse(1);
	if (cursor<length) 
		putChar(buffer[cursor]);
	else
		putChar(' ');
	setTextReverse(0);
	setScrollMode(1);
}

int getTextContinuable(char* _buffer, uint16_t maxSize, int timeoutTicks, char continued) {	
	uint32_t endTime = 0;
	
	if (0<timeoutTicks) {
		if (getVBLCounter()==(uint32_t)(-1))
			patchVBL();
		endTime = getVBLCounter() + timeoutTicks;
	}
	
	if (maxSize == 0)
		return -1;
	else if (maxSize == 1) {
		*_buffer = 0;
		return 0;
	}
	
	if (! continued) {
		setTextReverse(0);
		setScrollMode(1);
		startX = getTextX();
		startY = getTextY();
		rows = getTextRows();
		cols = getTextColumns();
		buffer = _buffer;
		cursor = length = strlen(_buffer);
		char* p = buffer;
		singleLine = startX + maxSize - 1 <= cols;		
		
		while(*p) {
			if (*p == '\n' || *p == '\r')
				*p = ' ';
			p++;
		}
		drawFrom(0);
	}
	drawCursor();
	
	while(1) {
		while(!kbhit()) {
			if ((length == 0 || *buffer!='\\') && 0 < timeoutTicks && endTime <= getVBLCounter()) {
				clearCursor();
				return ERROR_TIMEOUT;
			}
		}
		char c = getch();
		switch(c) {
			case '\n':
			case '\r':
				clearCursor();
				return length;
			case KEYBOARD_BREAK:
				clearCursor();
				return -1;
			case KEYBOARD_LEFT:
				if (cursor > 0) {
					clearCursor();
					cursor--;
					drawCursor();
				}
				break;
			case KEYBOARD_RIGHT:
				if (cursor < length) {
					clearCursor();
					cursor++;
					drawCursor();
				}
				break;
			case '\b':
			case '\x7F':
				if (cursor > 0) {
					clearCursor();
					if (cursor < length) 
						memmove(buffer+cursor-1, buffer+cursor, length+1-cursor);
					else
						buffer[cursor-1] = 0;
					cursor--;
					length--;
					drawFrom(cursor);
					putChar(' ');
					drawCursor();
				}
				break;
			default:
				if (length + 1 < maxSize - 1) {
					clearCursor();
					if (cursor < length)
						memmove(buffer+cursor+1, buffer+cursor, length+1-cursor);
					else
						buffer[cursor+1] = 0;
					buffer[cursor] = c;
					cursor++;
					length++;
					drawFrom(cursor-1);
					drawCursor();
				}
				break;
		}
	}
}

/* Translate in place all the escape characters in s.  */
static void translate_special_chars(char *s)
{
	char *src = s, *dest = s;
	while (*src)
		switch(*src++) {
		default: *dest++ = src[-1]; break;
		case '\n': *dest++ = ZC_RETURN; break;
		case '\\':
			switch (*src++) {
			case '\n': *dest++ = ZC_RETURN; break;
			case '\\': *dest++ = '\\'; break;
			case '?': *dest++ = ZC_BACKSPACE; break;
			case '[': *dest++ = ZC_ESCAPE; break;
			case '_': *dest++ = ZC_RETURN; break;
			case '^': *dest++ = ZC_ARROW_UP; break;
			case '.': *dest++ = ZC_ARROW_DOWN; break;
			case '<': *dest++ = ZC_ARROW_LEFT; break;
			case '>': *dest++ = ZC_ARROW_RIGHT; break;
			case 'R': *dest++ = ZC_HKEY_RECORD; break;
			case 'P': *dest++ = ZC_HKEY_PLAYBACK; break;
			case 'S': *dest++ = ZC_HKEY_SEED; break;
			case 'U': *dest++ = ZC_HKEY_UNDO; break;
			case 'N': *dest++ = ZC_HKEY_RESTART; break;
			case 'X': *dest++ = ZC_HKEY_QUIT; break;
			case 'D': *dest++ = ZC_HKEY_DEBUG; break;
			case 'H': *dest++ = ZC_HKEY_HELP; break;
			case '1': *dest++ = ZC_FKEY_F1; break;
			case '2': *dest++ = ZC_FKEY_F2; break;
			case '3': *dest++ = ZC_FKEY_F3; break;
			case '4': *dest++ = ZC_FKEY_F4; break;
			case '5': *dest++ = ZC_FKEY_F5; break;
			case '6': *dest++ = ZC_FKEY_F6; break;
			case '7': *dest++ = ZC_FKEY_F7; break;
			case '8': *dest++ = ZC_FKEY_F8; break;
			case '9': *dest++ = ZC_FKEY_F9; break;
#ifdef TOPS20
			case '0': *dest++ = (char) (ZC_FKEY_F10 & 0xff); break;
#else
			case '0': *dest++ = ZC_FKEY_F10; break;
#endif
			default:
				fprintf(stderr, "DUMB-FROTZ: unknown escape char: %c\n", src[-1]);
				fprintf(stderr, "Enter \\help to see the list\n");
			}
		}
	*dest = '\0';
} /* translate_special_chars */


/* The time in tenths of seconds that the user is ahead of z time.  */
static int time_ahead = 0;



/* If val is '0' or '1', set *var accordingly, otherwise toggle it.  */
static void toggle(bool *var, char val)
{
	*var = val == '1' || (val != '0' && !*var);
} /* toggle */


/* Handle input-related user settings and call dumb_output_handle_setting.  */
bool dumb_handle_setting(const char *setting, bool show_cursor, bool startup)
{
	if (!strncmp(setting, "sf", 2)) {
		speed_100 = atoi(&setting[2]);
		printf("Speed Factor %d\n", speed_100);
	} else if (!strncmp(setting, "mp", 2)) {
		toggle(&do_more_prompts, setting[2]);
		printf("More prompts %s\n", do_more_prompts ? "ON" : "OFF");
	} else {
		if (!strcmp(setting, "set")) {
			printf("Speed Factor %d\n", speed_100);
			printf("More Prompts %s\n",
 				do_more_prompts ? "ON" : "OFF");
		}
		return dumb_output_handle_setting(setting, show_cursor, startup);
	}
	return TRUE;
} /* dumb_handle_setting */


/* Read a line, processing commands (lines that start with a backslash
 * (that isn't the start of a special character)), and write the
 * first non-command to s.
 * Return true if timed-out.  */
static bool dumb_read_line(char *s, char *prompt, bool show_cursor,
			   int timeout, enum input_type type,
			   zchar *continued_line_chars, bool continued)
{
	time_t start_time = timeTenths();

	dumb_show_screen(show_cursor);
	for (;;) {
		char *command;
		if (!continued) {
			if (prompt)
				putText(prompt);
			else
				dumb_show_prompt(show_cursor,
					(timeout ? "tTD" : ")>}")[type]);
		}
		else {
			short len = strlen(s);
			if (len > 0 && s[len-1] == '\n')
				s[len-1] = 0;
		}

		int n = getTextContinuable(s, INPUT_BUFFER_SIZE-2, timeout*6, continued);
		if (n != ERROR_TIMEOUT) {
			putText("\n");
			if (0 <= n) {
				s[n] = '\n';
				s[n+1] = 0;
			}
			else if (-1 == n) {
				strcpy(s, "quit\n");
				putText(s);
			}
		}

		if ((s[0] != '\\') || ((s[1] != '\0') && !islower(s[1]))) {
			/* Is not a command line.  */
			translate_special_chars(s);
			if (n == ERROR_TIMEOUT) {
				return TRUE;
			}
			return FALSE;
		}
		/* Commands.  */

		/* Remove the \ and the terminating newline.  */
		command = s + 1;
		command[strlen(command) - 1] = '\0';
		*s = 0;

		if (!strcmp(command, "t")) {
			if (timeout) {
				time_ahead = 0;
				s[0] = '\0';
				return TRUE;
			}
		} else if (*command == 'w') {
			if (timeout) {
				int elapsed = atoi(&command[1]);
				long now = timeTenths();
				if (elapsed == 0)
					elapsed = (now - start_time) * speed_100 / 100;
				if (elapsed >= timeout) {
					s[0] = '\0';
					return TRUE;
				}
				timeout -= elapsed;
				start_time = now;
			}
/*		} else if (!strcmp(command, "d")) {
			if (type != INPUT_LINE_CONTINUED)
				fprintf(stderr, "DUMB-FROTZ: No input to discard\n");
			else {
				dumb_discard_old_input(strlen((char*) continued_line_chars));
				continued_line_chars[0] = '\0';
				type = INPUT_LINE;
 		} */
		} else if (!strcmp(command, "help")) {
			if (!do_more_prompts)
				putText(runtime_usage);
			else {
				char *current_page, *next_page;
				current_page = next_page = runtime_usage;
				for (;;) {
					int i;
					for (i = 0; (i < z_header.screen_rows - 2) && *next_page; i++)
						next_page = strchr(next_page, '\n') + 1;
					/* next_page - current_page is width */
					printf("%.*s", (int) (next_page - current_page), current_page);
					current_page = next_page;
					if (!*current_page)
						break;
					printf("HELP: Type <return> for more, or q to stop: ");
					char c = getch();
					if (c=='q') {
						putChar(c);
						putChar('\n');
						break;
					}
					else {
						putChar('\n');
					}
				}
			}
		} else if (!strcmp(command, "s")) {
			dumb_dump_screen();
    		} else if (!dumb_handle_setting(command, show_cursor, FALSE)) {
			fprintf(stderr, "DUMB-FROTZ: unknown command: %s\n", s);
			fprintf(stderr, "Enter \\help to see the list of commands\n");
		} 
	}
} /* dumb_read_line */


/* Read a line that is not part of z-machine input (more prompts and
 * filename requests).  */
static void dumb_read_misc_line(char *s, char *prompt)
{
	dumb_read_line(s, prompt, 0, 0, 0, 0, 0);
	/* Remove terminating newline */
	s[strlen(s) - 1] = '\0';
} /* dumb_read_misc_line */


/* Similar.  Useful for using function key abbreviations.  */
static char read_line_buffer[INPUT_BUFFER_SIZE];



// TODO: support show_cursor
zchar os_read_key (int timeout, bool show_cursor)
{
	long start_time;
	
	if (timeout) {
		start_time = timeTenths();
	}
	
	dumb_show_screen(false);
	
	while (! kbhit()) {
		if (timeout) {
			int elapsed = (timeTenths() - start_time) * speed_100 / 100;
			if (elapsed > timeout) {
				return ZC_TIME_OUT;
			}
		}
	}
	
	return getch();

	/* TODO: error messages for invalid special chars.  */
} /* os_read_key */


zchar os_read_line (int UNUSED (max), zchar *buf, int timeout, int UNUSED(width), int continued)
{
	char *p;
	int terminator;
	static bool timed_out_last_time;
	int timed_out = 0;

	timed_out = dumb_read_line(buf, NULL, TRUE,
		timeout * 100 / speed_100, buf[0] ? INPUT_LINE_CONTINUED : INPUT_LINE,
		buf, continued);

	if (timed_out) {
		timed_out_last_time = TRUE;
		return ZC_TIME_OUT;
	}

	/* find the terminating character.  */
	for (p = buf;; p++) {
		if (is_terminator(*p)) {
			terminator = *p;
			*p++ = '\0';
			break;
		}
	}

	/* TODO: Truncate to width and max.  */

	/* copy to screen */
//	dumb_display_user_input(read_line_buffer);

	/* copy to the buffer and save the rest for next time.  */
	/*strncat((char*) buf, (char *) read_line_buffer,
		INPUT_BUFFER_SIZE - strlen(read_line_buffer) - 1);
	p = read_line_buffer + strlen(read_line_buffer) + 1;
	memmove(read_line_buffer, p, strlen(p) + 1); */

	/* If there was just a newline after the terminating character,
	 * don't save it.  */
//	if ((read_line_buffer[0] == '\r') && (read_line_buffer[1] == '\0'))
//		read_line_buffer[0] = '\0';

	timed_out_last_time = FALSE;
	return terminator;
} /* os_read_line */

char right_type(DirEntry_t* d, char** extList, int numExts) {
	if (d->type == 0)
		return 0;
	short nameLen = strlen(d->name);
	if (numExts == 0)
		return 1;
	for (short i = 0; i < numExts; i++) {
		short extLen = strlen(extList[i]);
		if (nameLen <= extLen)
			return 0;
		if (0==strncasecmp(d->name+nameLen-extLen,extList[i],extLen))
			return 1;
	}
	return 0;
}

char pick_file(char* name, char** extData, int numExts) {
	DirEntry_t d;
	
	while(1) {
		int i=0;
		int pos = 0;
		if (HARDWARE_STATUS_NO_DISK & *HARDWARE_STATUS) {
			putText("Please insert a disk.\n");
			while (HARDWARE_STATUS_NO_DISK & *HARDWARE_STATUS) {
				if (kbhit()) {
					char c = getch();
					if (c == KEYBOARD_BREAK || c == 27)
						return 0;
				}
			}
		}
			
		while (-1 != getDirEntry(i, &d) && pos < 36) {
			if (right_type(&d, extData, numExts)) {
				char c;
				if (pos < 10)
					c = '0'+pos;
				else
					c = 'A'+(pos-10);
				if (i>0 && pos%4==0)
					putText("\n");
				printf("[%c] %-10s  ", c, d.name);
				pos++;
			}
			i++;
		}
		putText("\nPlease select a file or press STOP/ESC to type a name.\n");
		while ((HARDWARE_STATUS_OLD_DISK & *HARDWARE_STATUS) && ! kbhit()) ;
		if (0 == (HARDWARE_STATUS_OLD_DISK & *HARDWARE_STATUS)) {
			if (kbhit()) getch();
			continue;
		}
		else {
			char c = getch();
			if (c == KEYBOARD_BREAK || c == 27)
				return 0;
			short seekPos;
			if ('0' <= c && c <= '9')
				seekPos = c - '0';
			else if ('a' <= c && c <= 'z')
				seekPos = c - 'a' + 10;
			else if ('A' <= c && c <= 'Z')
				seekPos = c - 'A' + 10;
			else
				continue;
 			pos = 0;
			i = 0;
			while (-1 != getDirEntry(i, &d) && pos < 36) {
				if (right_type(&d, extData, numExts)) {
					if (pos == seekPos) {
						strncpy(name, d.name, MAX_FILE_NAME + 1);
						putText("Selected: ");
						putText(d.name);
						putText(".\n");
						return 1;
					}
					pos++;
				}
				i++;
			}
		}
	}
	return 0;
}

/*
 * os_read_file_name
 *
 * Return the name of a file. Flag can be one of:
 *
 *    FILE_SAVE      - Save game file
 *    FILE_RESTORE   - Restore game file
 *    FILE_SCRIPT    - Transcript file
 *    FILE_RECORD    - Command file for recording
 *    FILE_PLAYBACK  - Command file for playback
 *    FILE_SAVE_AUX  - Save auxiliary ("preferred settings") file
 *    FILE_LOAD_AUX  - Load auxiliary ("preferred settings") file
 *    FILE_NO_PROMPT - Return file without prompting the user
 *
 * The length of the file name is limited by MAX_FILE_NAME. Ideally
 * an interpreter should open a file requester to ask for the file
 * name. If it is unable to do that then this function should call
 * print_string and read_string to ask for a file name.
 *
 * Return value is NULL if there was a problem.
 */
char *os_read_file_name (const char *default_name, int flag)
{
	static char file_name[FILENAME_MAX + 1];
	char prompt[INPUT_BUFFER_SIZE];

	char fullpath[INPUT_BUFFER_SIZE];
	char *buf;

	FILE *fp;
	char *tempname;
	char *ext;
	bool freebuf = FALSE;

	/* If we're restoring a game before the interpreter starts,
 	 * our filename is already provided.  Just go ahead silently.
	 */
	if (f_setup.restore_mode) {
		/* Caller always strdups */
		return (char*)default_name;
	} else if (flag == FILE_NO_PROMPT) {
		ext = strrchr(default_name, '.');
		if (strncmp(ext, EXT_AUX, 4)) {
			os_warn("Blocked unprompted access of %s. Should only be %s files.", default_name, EXT_AUX);
			free(ext);
			return NULL;
		}
		free(ext);
		buf = strndup(default_name, MAX_FILE_NAME - EXT_LENGTH);
	} else {
		if (flag == FILE_RESTORE || flag == FILE_LOAD_AUX) {
			char *ext = (flag == FILE_RESTORE) ? EXT_SAVE : EXT_AUX;
			if (pick_file(file_name, &ext,1))
				return file_name;
		}
		sprintf(prompt, "Please enter a filename [%s]: ", default_name);

		dumb_read_misc_line(fullpath, prompt);

		/* If using default filename... */
		freebuf = TRUE;
		if (strlen(fullpath) == 0)
			buf = strndup(default_name, MAX_FILE_NAME);
		else /* Using supplied filename... */
			buf = strndup(fullpath, MAX_FILE_NAME);

		if (strlen(buf) > MAX_FILE_NAME) {
			printf("Filename too long\n");
			free(buf);
			return NULL;
		}
	}

	assert(buf);
	strncpy(file_name, buf, FILENAME_MAX);
	if (freebuf) free(buf);

	/* Add appropriate filename extensions if not already there. */
	ext = strrchr(file_name, '.');
	if (flag == FILE_SAVE || flag == FILE_RESTORE) {
		if (ext == NULL || strncmp(ext, EXT_SAVE, EXT_LENGTH) != 0) {
			strncat(file_name, EXT_SAVE, EXT_LENGTH);
		}
#ifndef NO_SCRIPT
	} else if (flag == FILE_SCRIPT) {
		if (ext == NULL || strncmp(ext, EXT_SCRIPT, EXT_LENGTH) != 0) {
			strncat(file_name, EXT_SCRIPT, EXT_LENGTH);
		}
#endif
	} else if (flag == FILE_SAVE_AUX || flag == FILE_LOAD_AUX) {
		if (ext == NULL || strncmp(ext, EXT_AUX, EXT_LENGTH) != 0) {
			strncat(file_name, EXT_AUX, EXT_LENGTH);
		}
#ifndef NO_SCRIPT		
	} else if (flag == FILE_RECORD || flag == FILE_PLAYBACK) {
		if (ext == NULL || strncmp(ext, EXT_COMMAND, EXT_LENGTH) != 0) {
			strncat(file_name, EXT_COMMAND, EXT_LENGTH);
		}
#endif
	}

	/* Warn if overwriting a file.  */
	if ((flag == FILE_SAVE || flag == FILE_SAVE_AUX || flag == FILE_RECORD)
		&& ((fp = fopen(file_name, "rb")) != NULL)) {
		fclose (fp);
		dumb_read_misc_line(fullpath, "Overwrite existing file? ");
		if (tolower(fullpath[0]) != 'y')
			return NULL;
	}
	return file_name;
} /* os_read_file_name */


void os_more_prompt (void)
{
	if (do_more_prompts) {
		dumb_show_screen(false);
		uint16_t x,y;
		x = getTextX();
		y = getTextY();
		putText("***MORE***");
		getch();
		setTextXY(x,y);
		putText("          ");
		setTextXY(x,y);
	} else
		dumb_elide_more_prompt();
} /* os_more_prompt */


void dumb_init_input(void)
{
	initKeyboard(1);
	if ((z_header.version >= V4) && (speed_100 != 0))
		z_header.config |= CONFIG_TIMEDINPUT;

	if (z_header.version >= V5)
		z_header.flags &= ~(MOUSE_FLAG | MENU_FLAG);
} /* dumb_init_input */


zword os_read_mouse(void)
{
	/* NOT IMPLEMENTED */
	return 0;
} /* os_read_mouse */

void os_tick(void)
{
	/* Nothing here yet */
} /* os_tick */
