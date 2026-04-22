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

#define MAX_PICK_FILES 144
static uint16_t pickFileList[MAX_PICK_FILES];
static uint16_t numPickFiles;
static int pickFlag;
static char** pickExtData;
static short pickNumExts;

extern f_setup_t f_setup;

#define HISTORY_BUFFER_SIZE 2048
static uint16_t historyCursor;
static uint16_t historyCursorOffset;
static uint16_t historyLength;
static uint16_t historyLastOffset;
static uint16_t numInHistory;
static zchar history[HISTORY_BUFFER_SIZE];

static int speed_100 = 100;

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
static char afterHotkey = 0;

static struct {
	char c;
	uint8_t zc;
} key_table[] = {
	{ '\b', ZC_BACKSPACE },
	{ '\n', ZC_RETURN },
	{ '\r', ZC_RETURN },
	{ 27, ZC_ESCAPE },
	{ KEYBOARD_UP, ZC_ARROW_MIN },
	{ KEYBOARD_DOWN, ZC_ARROW_MIN+1 },
	{ KEYBOARD_LEFT, ZC_ARROW_MIN+2 },
	{ KEYBOARD_RIGHT, ZC_ARROW_MIN+3 },
	{ KEYBOARD_F1, ZC_FKEY_MIN },
	{ KEYBOARD_F1+1, ZC_FKEY_MIN+1 },
	{ KEYBOARD_F1+2, ZC_FKEY_MIN+2 },
	{ KEYBOARD_F1+3, ZC_FKEY_MIN+3 },
	{ KEYBOARD_F1+4, ZC_FKEY_MIN+4 },
	{ KEYBOARD_F1+5, ZC_FKEY_MIN+5 },
	{ KEYBOARD_F1+6, ZC_FKEY_MIN+6 },
	{ KEYBOARD_F1+7, ZC_FKEY_MIN+7 },
	{ KEYBOARD_F1+8, ZC_FKEY_MIN+8 },
	{ KEYBOARD_F1+9, ZC_FKEY_MIN+9 }, // todo: numpad, mouse
};


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
	uint16_t x = getTextX();
	if (cursor<length)
		putChar(buffer[cursor]);
	else
		putChar(' ');
	setTextX(x);
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

static void complete(void)
{
	int status;

	if (length == cursor) {
		zchar extension[10];

		status = completion((zchar*)buffer, extension);
	
		if (status == 0) {
			unsigned short extLength = strlen((char*)extension);
			if (length + extLength < maxSize) {
				clearCursor();
				strcpy(buffer+length, (char*)extension);
				length += extLength;
				buffer[length] = 0;
				drawFrom(cursor);
				cursor += extLength;
				drawCursor();
			}
			else {
				status = 2;
			}
		}
	} 
	else {
		status = 2;
	}

	/* Beep if the completion was impossible or ambiguous */
	if (status != 0)
		os_beep(status);
} 

int16_t getTextContinuable(char* _buffer, uint16_t _maxSize, int timeoutTicks, bool continued, bool cancelable) {	
	uint32_t endTime = 0;
	
	if (0<timeoutTicks) {
		endTime = getVBLCounter() + timeoutTicks;
	}
	
	if (_maxSize == 0)
		return -1;
	else if (_maxSize == 1) {
		*_buffer = 0;
		return 0;
	}
	
	if (! continued) {
		afterHotkey = 0;
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
	}
	else {
		if (afterHotkey)
			startX = 0;
	}
	afterHotkey = 0;
	drawFrom(0);
	drawCursor();
	
	while(1) {
		InputEvent_t e;
		while(!getInputEvent(&e)) {
			if (0 < timeoutTicks && endTime <= getVBLCounter()) {
				clearCursor();
				return ERROR_TIMEOUT;
			}
		}
		if (e.type == INPUT_MOUSE) {
			mouse_x = 1 + e.data.mouse.x / getFontWidth();
			mouse_y = 1 + e.data.mouse.y / getFontHeight();
			if ((e.data.mouse.buttons & e.data.mouse.buttonDifference & MOUSE_DOUBLE_CLICK)) {
				clearCursor();
				afterHotkey = 1;
				return INPUT_MOUSE_DOUBLE_CLICK;
			}
			else if ((e.data.mouse.buttons & e.data.mouse.buttonDifference & MOUSE_BUTTON_LEFT)) {
				clearCursor();
				afterHotkey = 1;
				return INPUT_MOUSE_CLICK;
			}
			continue;
		}
		else if (e.type != INPUT_KEY) {
			continue;
		}
		uint8_t c = e.data.key.character;
		switch(c) {
			case '\n':
			case '\r':
				clearCursor();
				memcpy(history + historyLastOffset, buffer, length + 1);
				historyLength += length + 1;
				return length;
			case '\t':
				complete();
				break;
			case KEYBOARD_BREAK:
			case KEYBOARD_ALT_ALPHA('x'):
				clearCursor();
				afterHotkey = 1;
				return INPUT_STOP;
			case KEYBOARD_ALT_ALPHA('u'):
				clearCursor();
				afterHotkey = 1;
				return INPUT_UNDO;
			case KEYBOARD_ALT_ALPHA('n'):
				clearCursor();
				afterHotkey = 1;
				return INPUT_RESTART;
			case KEYBOARD_ALT_ALPHA('d'):
				clearCursor();
				afterHotkey = 1;
				return INPUT_DEBUG;
			case KEYBOARD_ALT_ALPHA('s'):
				clearCursor();
				afterHotkey = 1;
				return INPUT_SEED;
			case KEYBOARD_F1: 
			case KEYBOARD_ALT_ALPHA('h'):
				clearCursor();
				afterHotkey = 1;
				return INPUT_HELP;
			case 27:
				if (cancelable) {
					clearCursor();
					afterHotkey = 1;
					return INPUT_STOP;
				}
				clearBuffer();
				break;
				
			case KEYBOARD_UP:
				if (*buffer && strcmp((char*)history+historyCursorOffset, buffer)) { // commmit changed history entry to history
					strncpy((char*)history+historyLastOffset, buffer, INPUT_BUFFER_SIZE);
					history[historyLastOffset+INPUT_BUFFER_SIZE-1] = 0;
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
					history[historyLastOffset+INPUT_BUFFER_SIZE-1] = 0;
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



/* Read a line to s.
 * Return true if timed-out.  */
static int16_t hp_read_line(zchar *s, uint16_t bufferSize, char *prompt, bool show_cursor,
			   int timeout, bool continued, bool cancelable)
{
	if (!continued) {
		if (prompt)
			putText(prompt);
	}

	return getTextContinuable((char*)s, bufferSize, timeout*6, continued, cancelable);
} /* hp_read_line */


/* Read a line that is not part of z-machine input (more prompts and
 * filename requests).  */
static int16_t hp_read_misc_line(char *s, char *prompt, uint16_t length)
{
	int16_t result = hp_read_line((zchar*)s, length, prompt, 0, 0, 0, 1);
	putChar('\n');
	return result;
} /* hp_read_misc_line */

static zchar translate_key(uint8_t c) {
	if (32 <= c && c <= 126)
		return c;
	for (uint16_t i=0; i<sizeof(key_table)/sizeof(*key_table); i++)
		if (key_table[i].c == c)
			return key_table[i].zc;
	return c;
}

// TODO: support show_cursor
zchar os_read_key (int timeout, bool show_cursor)
{
	long start_time;
	
	if (timeout) {
		start_time = timeTenths();
	}
	
	InputEvent_t e;

	while(1) {
		while (! getInputEvent(&e)) {
			if (timeout) {
				int elapsed = (timeTenths() - start_time) * speed_100 / 100;
				if (elapsed > timeout) {
					return ZC_TIME_OUT;
				}
			}
		}
		if (e.type == INPUT_KEY)
			return translate_key(e.data.key.character);
		else if (e.type == INPUT_MOUSE) {
			mouse_x = 1 + e.data.mouse.x / getFontWidth();
			mouse_y = 1 + e.data.mouse.y / getFontHeight();
			if ((e.data.mouse.buttons & MOUSE_DOUBLE_CLICK) && (e.data.mouse.buttonDifference & MOUSE_DOUBLE_CLICK) ) {
				return ZC_DOUBLE_CLICK;				
			}
			else if ((e.data.mouse.buttons & MOUSE_BUTTON_LEFT) && (e.data.mouse.buttonDifference & MOUSE_BUTTON_LEFT) ) {
				return ZC_SINGLE_CLICK;				
			}
		}
	}

	/* TODO: error messages for invalid special chars.  */
} /* os_read_key */


zchar os_read_line (int UNUSED (max), zchar *buf, int timeout, int UNUSED(width), int continued)
{
	int16_t result = hp_read_line(buf, INPUT_BUFFER_SIZE-1, NULL, TRUE,
		timeout * 100 / speed_100, 
		continued, 0);
		
	switch (result) {
		case ERROR_TIMEOUT:
			return ZC_TIME_OUT;
		case INPUT_STOP:
			return ZC_HKEY_QUIT;
		case INPUT_HELP:
			return ZC_HKEY_HELP;
		case INPUT_UNDO:
			return ZC_HKEY_UNDO;
		case INPUT_RESTART:
			return ZC_HKEY_RESTART;
		case INPUT_DEBUG:
			return ZC_HKEY_DEBUG;
		case INPUT_SEED:
			return ZC_HKEY_SEED;
		case INPUT_MOUSE_CLICK:
			return ZC_SINGLE_CLICK;
		case INPUT_MOUSE_DOUBLE_CLICK:
			return ZC_DOUBLE_CLICK;
		default:
			return 13;
	}
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

static unsigned short pickFileLoader(void) {
	DirEntry_t d;
	int i = 0;
	numPickFiles = 0;
	while (-1 != getDirEntry(i, &d) && numPickFiles < MAX_PICK_FILES) {
		if (right_type(&d, pickExtData, pickNumExts)) {
			if (pickFlag == FILE_RESTORE) {
				struct IFhd* h = getSaveHeader(d.name);
				if (h != NULL && h->release == z_header.release &&
					h->checksum == z_header.checksum &&
					!memcmp(h->serial, zmp+H_SERIAL, 6))
					pickFileList[numPickFiles++] = i;
			}
			else {
				pickFileList[numPickFiles++] = i;
			}
		}
		i++;
	}
	return numPickFiles;
}

static char* pickFileNamer(unsigned short i) {
	DirEntry_t d;
	static char name[MAX_FILE_NAME+1];
	
	if (-1 != getDirEntry(pickFileList[i], &d)) {
		strcpy(name, d.name);
		return name;
	}
	else {
		return "";
	}
}

uint8_t pick_file(char* name, char** extData, short numExts, int flag) {
	pickFlag = flag;
	pickExtData = extData;
	pickNumExts = numExts;
	
	short i = hpChooser(1, 1, WIN_X2-WIN_X1-2, WIN_Y2-WIN_Y1-2, 2, 10, pickFileLoader, pickFileNamer, CHOOSER_DISK_BASED);
	
	if (i < 0)
		return 0;
	
	char* n = pickFileNamer(i);
	if (*n == 0)
		return 0;
	strcpy(name, n);
	return 1;
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
		strcpy(file_name, default_name);
		return file_name;
	} 
	
	hp_set_window();
	
	if (flag == FILE_NO_PROMPT) {
		ext = strrchr(default_name, '.');
		if (ext == NULL || strncmp(ext, EXT_AUX, 4)) {
			hp_clear_window();
			os_warn("Blocked unprompted access of %s. Should only be %s files.", default_name, EXT_AUX);
			return NULL;
		}
		buf = strndup(default_name, MAX_FILE_NAME - EXT_LENGTH);
	} else {
		if (flag == FILE_RESTORE || flag == FILE_LOAD_AUX) {
			char *ext = (flag == FILE_RESTORE) ? EXT_SAVE : EXT_AUX;
			uint8_t r = pick_file(file_name, &ext, 1, flag);
			hp_clear_window();
			return r ? file_name : NULL;
		}
		
		sprintf(prompt, "Please enter a filename [%s]: ", default_name);

		*fullpath = 0;
		if (hp_read_misc_line(fullpath, prompt, MAX_FILE_NAME - 4 + 1) < 0) {
			hp_clear_window();
			return NULL;
		}

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
			hp_clear_window();
			return NULL;
		}
	}

	assert(buf);
	strncpy(file_name, buf, FILENAME_MAX+1);
	file_name[FILENAME_MAX]=0;
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
		putText("Overwrite existing file? ");
		char c = getch();
		putChar(c);
		if (tolower(c) != 'y') {
			hp_clear_window();
			return NULL;
		}
	}
	hp_clear_window();
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
char *os_read_file_name(const char *default_name, int flag)
{
	static char fixed_name[MAX_FILE_NAME+1];
	char* ext = strrchr(default_name, '.');
	if (ext == NULL) {
		strncpy(fixed_name, default_name, MAX_FILE_NAME);
		fixed_name[MAX_FILE_NAME] = 0;
	}
	else {
		short extLen = strlen(ext);
		short baseLen = ext - default_name;
		if (baseLen + extLen > MAX_FILE_NAME)
			baseLen = MAX_FILE_NAME - extLen;
		strncpy(fixed_name, default_name, baseLen);
		strcpy(fixed_name + baseLen, ext);
	}
	char* p = hp_read_file_name(fixed_name, flag);

	return p;
} /* os_read_file_name */



void hp_init_input(void)
{
	initKeyboard(1);
	if ((z_header.version >= V4) && (speed_100 != 0))
		z_header.config |= CONFIG_TIMEDINPUT;

	if (z_header.version >= V5)
		z_header.flags &= ~MENU_FLAG;
	
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
