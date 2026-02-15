/*
 * dinput.c - HP152B/1653B interface, input functions
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

#include "hpfrotz.h"

#define MAX_PICK_FILES 36
#define PICK_FILES_PER_LINE 4

extern f_setup_t f_setup;

#define HISTORY_BUFFER_SIZE 2048
uint16_t historyCursor;
uint16_t historyCursorOffset;
uint16_t historyLength;
uint16_t historyLastOffset;
uint16_t numInHistory;
zchar history[HISTORY_BUFFER_SIZE];

static int speed_100 = 100;

enum input_type {
	INPUT_CHAR,
	INPUT_LINE,
	INPUT_LINE_CONTINUED,
};

int getTextContinuable(char* _buffer, uint16_t _maxSize, int timeoutTicks, char continued);
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
static uint16_t maxSize;

#define INPUT_STOP (-1)
#define INPUT_ESC  (-2)

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

static void prepareHistory(void) {
	if (historyLength + INPUT_BUFFER_SIZE > HISTORY_BUFFER_SIZE) {
		/* remove items from history */
		zchar* q = history + ((historyLength + INPUT_BUFFER_SIZE) - HISTORY_BUFFER_SIZE);
		zchar* p = history;
		do {
			p += strlen((char*)p)+1;
			numInHistory--;
		} while(p<q);
		uint16_t delta = p-history;
		historyLength -= delta; 
		memmove(history, p, historyLength);
	}
	numInHistory++;
	historyCursor = numInHistory-1;
	historyLastOffset = historyCursorOffset = historyLength;
	history[historyLength] = 0;
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
	else if (getTextX() == cols-1 && cursor == length)
		putChar(buffer[cursor-1]);
	else
		putChar(' ');
	setTextReverse(0);
	setScrollMode(1);
}

static void leftWord(void) {
	if (cursor <= 0)
		return;
	clearCursor();
	while(cursor > 0 && buffer[cursor-1] == ' ')
		cursor--;
	while(cursor > 0 && buffer[cursor-1] != ' ')
		cursor --;
	drawCursor();
}

static void rightWord(void) {
	if (cursor >= length)
		return;
	clearCursor();
	while(cursor + 1 < length && buffer[cursor] == ' ')
		cursor++;
	while(cursor + 1 < length && buffer[cursor] != ' ')
		cursor++;
	drawCursor();
}

static void loadHistoryEntry(zchar* p) {
	uint16_t oldLength = length;
	clearCursor();
	setScrollMode(0);
	strncpy(buffer, (char*)p, INPUT_BUFFER_SIZE);
	buffer[INPUT_BUFFER_SIZE-1] = 0;
	length = strlen(buffer);
	cursor = length;
	drawFrom(0);
	while (length < oldLength--)
		putChar(' ');
	drawCursor();		
}

static void clearBuffer(void) {
	clearCursor();
	setXYFromOffset(0);
	while(length-->0) {
		putChar(' ');
	}
	setXYFromOffset(0);
	length = 0;
	cursor = 0;
	*buffer = 0;
	drawCursor();
}

int getTextContinuable(char* _buffer, uint16_t _maxSize, int timeoutTicks, char continued) {	
	uint32_t endTime = 0;
	
	if (0<timeoutTicks) {
		if (getVBLCounter()==(uint32_t)(-1))
			patchVBL();
		endTime = getVBLCounter() + timeoutTicks;
	}
	
	if (_maxSize == 0)
		return -1;
	else if (_maxSize == 1) {
		*_buffer = 0;
		return 0;
	}
	
	if (! continued) {
		prepareHistory();
		setTextReverse(0);
		setScrollMode(1);
		startX = getTextX();
		startY = getTextY();
		rows = getTextRows();
		cols = getTextColumns();
		maxSize = cols - startX + 1;
		if (maxSize > _maxSize)
			maxSize = _maxSize;
		buffer = _buffer;
		cursor = length = strlen(_buffer);
		char* p = buffer;
		singleLine = true; // startX + maxSize - 1 <= cols;		
		
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
		uint8_t c = getch();
		switch(c) {
			case '\n':
			case '\r':
				clearCursor();
				memcpy(history + historyLastOffset, buffer, length + 1);
				historyLength += length + 1;
				return length;
			case KEYBOARD_BREAK:
			case 27:
				clearBuffer();
				if (c == KEYBOARD_BREAK) {
					numInHistory--;
					*buffer++ = ZC_HKEY_QUIT;
					*buffer = 0;
					clearCursor();
					return 1;
				}
				break;
				
			case KEYBOARD_UP:
				if (*buffer && strcmp((char*)history+historyCursorOffset, buffer)) { // commmit changed history entry to history
					strncpy((char*)history+historyLastOffset, buffer, INPUT_BUFFER_SIZE);
					historyCursorOffset = historyLastOffset;
					historyCursor = numInHistory-1;
				}
				if (0 < historyCursor) {
					zchar* p = history + historyCursorOffset - 2;
					while (p >= history) {
						if (*p == 0)
							break;
						p--;
					}
					p++;
					historyCursor--;
					historyCursorOffset = p-history;
					loadHistoryEntry(p);
				}
				break;
			case KEYBOARD_DOWN:
				if (*buffer && strcmp((char*)history+historyCursorOffset, buffer)) { // commmit changed history entry to history
					strncpy((char*)history+historyLastOffset, buffer, INPUT_BUFFER_SIZE);
					historyCursorOffset = historyLastOffset;
					historyCursor = numInHistory-1;
				}
				if (historyCursor + 1 < numInHistory) {
					historyCursor++;
					historyCursorOffset += strlen((char*)history+historyCursorOffset) + 1;
					loadHistoryEntry(history + historyCursorOffset);
				}
				break;
			case KEYBOARD_CTRL_LEFT:
				leftWord();
				break;
			case KEYBOARD_CTRL_RIGHT:
				rightWord();
				break;
			case KEYBOARD_HOME:
				clearCursor();
				cursor = 0;
				drawCursor();
				break;
			case KEYBOARD_END:
				clearCursor();
				cursor = length;
				drawCursor();
				break;
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
			case '\x7F':
			case _CTRL('g'):
				if (cursor < length) {
					clearCursor();
					if (cursor < length) 
						memmove(buffer+cursor, buffer+cursor+1, length-cursor);
					else
						buffer[cursor] = 0;
					length--;
					drawFrom(cursor);
					putChar(' ');
					drawCursor();
				}
				break;
			default: 
				if (length + 1 < maxSize) {
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
static void translate_special_chars(zchar *s)
{
	zchar *src = s, *dest = s;
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
			case '0': *dest++ = ZC_FKEY_F10; break;
			default:
				break;
			}
		}
	*dest = '\0';
} /* translate_special_chars */



/* Read a line to s.
 * Return true if timed-out.  */
static int16_t hp_read_line(zchar *s, char *prompt, bool show_cursor,
			   int timeout, enum input_type type,
			   bool continued)
{
	for (;;) {
		if (!continued) {
			if (prompt)
				putText(prompt);
		}
		else {
			short len = strlen((char*)s);
			if (len > 0 && s[len-1] == '\n')
				s[len-1] = 0;
		}

		int16_t n = getTextContinuable((char*)s, INPUT_BUFFER_SIZE-2, timeout*6, continued);
		if (0 <= n) {
			s[n] = '\n';
			s[n+1] = 0;
		}

		translate_special_chars(s);

		return n;
	}
} /* hp_read_line */


/* Read a line that is not part of z-machine input (more prompts and
 * filename requests).  */
static int16_t hp_read_misc_line(char *s, char *prompt)
{
	int16_t result = hp_read_line((zchar*)s, prompt, 0, 0, 0, 0);
	/* Remove terminating newline */
	s[strlen(s) - 1] = '\0';
	return result;
} /* hp_read_misc_line */


// TODO: support show_cursor
zchar os_read_key (int timeout, bool show_cursor)
{
	long start_time;
	
	if (timeout) {
		start_time = timeTenths();
	}
	
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
	zchar *p;
	int terminator;

	int16_t result = hp_read_line(buf, NULL, TRUE,
		timeout * 100 / speed_100, buf[0] ? INPUT_LINE_CONTINUED : INPUT_LINE,
		continued);
		
	if (result == ERROR_TIMEOUT) {
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
	
	return terminator;
} /* os_read_line */

static char right_type(DirEntry_t* d, char** extList, int numExts) {
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


char *hp_read_file_name (const char *default_name, int flag) {
	static char file_name[FILENAME_MAX + 1];
	char prompt[INPUT_BUFFER_SIZE];

	char fullpath[INPUT_BUFFER_SIZE];
	char *buf;

	FILE *fp;
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
			if (pick_file(file_name, &ext,1)) {
				return file_name;
			}
		}
		
		sprintf(prompt, "Please enter a filename [%s]: ", default_name);

		if (hp_read_misc_line(fullpath, prompt) < 0)
			return NULL;

		/* If using default filename... */
		freebuf = TRUE;
		if (strlen(fullpath) == 0)
			buf = strndup(default_name, MAX_FILE_NAME);
		else /* Using supplied filename... */
			buf = strndup(fullpath, MAX_FILE_NAME);

		if (strlen(buf) > MAX_FILE_NAME) {
			printf("Filename too long\n");
			delayTicks(30);
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
		if (hp_read_misc_line(fullpath, "Overwrite existing file? ") < 0)
			return NULL;
		if (tolower(fullpath[0]) != 'y') {
			return NULL;
		}
	}
	return file_name;
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
	hp_set_window();
	char* p = hp_read_file_name(default_name, flag);
	hp_clear_window();

	return p;
} /* os_read_file_name */



void hp_init_input(void)
{
	initKeyboard(1);
	if ((z_header.version >= V4) && (speed_100 != 0))
		z_header.config |= CONFIG_TIMEDINPUT;

	if (z_header.version >= V5)
		z_header.flags &= ~(MOUSE_FLAG | MENU_FLAG);
	
	numInHistory = 0;
	historyCursor = 0;
	historyCursorOffset = 0;
	historyLength = 0;
} /* hp_init_input */


zword os_read_mouse(void)
{
	/* NOT IMPLEMENTED */
	return 0;
} /* os_read_mouse */

void os_tick(void)
{
	/* Nothing here yet */
} /* os_tick */
