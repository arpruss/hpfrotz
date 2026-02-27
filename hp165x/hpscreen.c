// TODO: check reverse
// remove extra newline in input

/*
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

#define WRITE_OVERLAY_GRAY       0b110000000001
#define WRITE_OVERLAY_FOREGROUND 0b101000000001
#define WRITE_OVERLAY_BACKGROUND 0b100000000001
#define WRITE_OVERLAY_ERASE 	 0b111000000001

#define WIN_X1 9
#define WIN_Y1 4
#define WIN_X2 71
#define WIN_Y2 22

static uint16_t savedX;
static uint16_t savedY;

void hp_set_window(void) {
	savedX = getTextX();
	savedY = getTextY();
	uint16_t h = getFontHeight();
	*SCREEN_MEMORY_CONTROL = WRITE_OVERLAY_GRAY;
	frameRectangle(WIN_X1*FONT_WIDTH,WIN_Y1*h,WIN_X2*FONT_WIDTH,WIN_Y2*h,8);
	*SCREEN_MEMORY_CONTROL = WRITE_OVERLAY_BACKGROUND;
	fillRectangle(WIN_X1*FONT_WIDTH,WIN_Y1*h,WIN_X2*FONT_WIDTH,WIN_Y2*h);
	setTextWindow(WIN_X1,WIN_Y1,WIN_X2,WIN_Y2);
	setTextColors(WRITE_OVERLAY_FOREGROUND,WRITE_OVERLAY_BACKGROUND);
	setTextXY(0,0);
}

void hp_clear_window() {
	uint16_t h = getFontHeight();
	*SCREEN_MEMORY_CONTROL = WRITE_OVERLAY_ERASE;
	fillRectangle(WIN_X1*FONT_WIDTH-8,WIN_Y1*h-8,WIN_X2*FONT_WIDTH+8,WIN_Y2*h+8);
	setTextWindow(0,0,0,0);
	setTextColors(FOREGROUND,BACKGROUND);
	setTextXY(savedX,savedY);
}

static int current_style = 0;
static int current_font = 0;
static int current_fg = 1;
static int current_bg = 0;

char latin1_to_ibm[] = {
	0x20, 0xad, 0xbd, 0x9c, 0xcf, 0xbe, 0xdd, 0xf5,
	0xf9, 0xb8, 0xa6, 0xae, 0xaa, 0xf0, 0xa9, 0xee,
	0xf8, 0xf1, 0xfd, 0xfc, 0xef, 0xe6, 0xf4, 0xfa,
	0xf7, 0xfb, 0xa7, 0xaf, 0xac, 0xab, 0xf3, 0xa8,
	0xb7, 0xb5, 0xb6, 0xc7, 0x8e, 0x8f, 0x92, 0x80,
	0xd4, 0x90, 0xd2, 0xd3, 0xde, 0xd6, 0xd7, 0xd8,
	0xd1, 0xa5, 0xe3, 0xe0, 0xe2, 0xe5, 0x99, 0x9e,
	0x9d, 0xeb, 0xe9, 0xea, 0x9a, 0xed, 0xe8, 0xe1,
	0x85, 0xa0, 0x83, 0xc6, 0x84, 0x86, 0x91, 0x87,
	0x8a, 0x82, 0x88, 0x89, 0x8d, 0xa1, 0x8c, 0x8b,
	0xd0, 0xa4, 0x95, 0xa2, 0x93, 0xe4, 0x94, 0xf6,
	0x9b, 0x97, 0xa3, 0x96, 0x81, 0xec, 0xe7, 0x98
};

/*
 * os_erase_area
 *
 * Fill a rectangular area of the screen with the current background
 * colour. Top left coordinates are (1,1). The cursor does not move.
 *
 * The final argument gives the window being changed, -1 if only a
 * portion of a window is being erased, or -2 if the whole screen is
 * being erased.  
 *
 */
void os_erase_area(int top, int left, int bottom, int right, int win)
{
	top--;
	left--;
	
	*SCREEN_MEMORY_CONTROL = BACKGROUND;
	
	uint16_t h = getFontHeight();

	if (win == -2)
		fillScreen();
	else
		fillRectangle(left * FONT_WIDTH, top * h, right * FONT_WIDTH, bottom * h);
} /* os_erase_area */



/*
 * os_scroll_area
 *
 * Scroll a rectangular area of the screen up (units > 0) or down
 * (units < 0) and fill the empty space with the current background
 * colour. Top left coordinates are (1,1). The cursor stays put.
 *
 */
void os_scroll_area(int top, int left, int bottom, int right, int units)
{
	if (units == 0)
		return;
	
	top--;
	left--;
	
	uint16_t h = getFontHeight();
	
	if (units < 0)
		scrollDown(-units * h, left * FONT_WIDTH, top * h, right * FONT_WIDTH, bottom * h, BACKGROUND, 1);
	else
		scrollUp(units * h, left * FONT_WIDTH, top * h, right * FONT_WIDTH, bottom * h, BACKGROUND, 1);
} /* os_scroll_area */


bool os_repaint_window(int win, int ypos_old, int ypos_new, int xpos,
		       int ysize, int xsize)
{
	return FALSE;
} /* os_repaint_window */

int os_font_data(int font, int *height, int *width)
{
	if (font == TEXT_FONT) {
		*height = 1;
		*width = 1;
		return 1;
	}
	return 0;
} /* os_font_data */

void os_set_colour (int newfg, int newbg)
{
	current_fg = newfg;
	current_bg = newbg;
	
	if (newfg == BLACK_COLOUR && newbg == WHITE_COLOUR) {
		setTextColors(BACKGROUND, FOREGROUND);
	}
	else {
		setTextColors(FOREGROUND, BACKGROUND);
	}
		
} /* os_set_colour */

void os_display_char (zchar c)
{
	if (c >= ZC_LATIN1_MIN) {
		putChar(latin1_to_ibm[c - ZC_LATIN1_MIN]);
	} else if (c >= 32 && c <= 126) {
		putChar(c);
	} else if (c == ZC_GAP) {
		putText("  ");
	} else if (c == ZC_INDENT) {
		putText("   ");
	}
	return;
}

void os_display_string (const zchar *s)
{
	zchar c;

	while ((c = *s++) != 0) {
		if (c == ZC_NEW_FONT)
			s++;
		else if (c == ZC_NEW_STYLE)
			os_set_text_style(*s++);
		else {
			os_display_char (c);
		}
	}
} /* os_display_string */

void os_reset_screen() {
	os_set_font(TEXT_FONT);
	os_set_text_style(0);
	os_display_string((zchar *) "[Hit any key to exit.]");
	os_read_key(0, TRUE);
}

void os_beep (int volume)
{
	*BEEPER=BEEPER_ON;
	delayTicks(7);
	*BEEPER=BEEPER_OFF;
} /* os_beep */

void os_init_sound(void) {}
void os_prepare_sample (int UNUSED (a)) {}
void os_finish_with_sample (int UNUSED (a)) {}
void os_start_sample (int UNUSED (a), int UNUSED (b), int UNUSED (c), zword UNUSED (d)) {}
void os_stop_sample (int UNUSED (a)) {}

int os_check_unicode(int UNUSED (font), zchar UNUSED (c))
{
	/* Only UTF-8 output, no input yet.  */
	return 1;
} /* os_check_unicode */


int os_char_width (zchar z)
{
	return 1;
} /* os_char_width */


int os_string_width (const zchar *s)
{
	int width = 0;
	zchar c;

	while ((c = *s++) != 0) {
		if (c == ZC_NEW_STYLE || c == ZC_NEW_FONT)
			s++;
		else
			width += os_char_width(c);
	}
	return width;
} /* os_string_width */


void os_set_cursor(int row, int col)
{
	if (row >= z_header.screen_rows)
		setTextXY(col-1, z_header.screen_rows - 1);
	else
		setTextXY(col-1,row-1);
} /* os_set_cursor */

int os_get_text_style(void)
{
	return current_style;
} /* os_get_text_style */

void adjust_style(void) {
	setTextReverse((current_style & (REVERSE_STYLE | BOLDFACE_STYLE | EMPHASIS_STYLE)) != 0);
}

void os_more_prompt(void)
{
	/* Save text font and style */
	int saved_font = current_font;
	int saved_style = current_style;

	/* Choose plain text style */
	current_font = TEXT_FONT;
	current_style = 0;

	adjust_style();

	/* Wait until the user presses a key */
	uint16_t saved_x = getTextX();
	uint16_t saved_y = getTextY();
	os_display_string((zchar *) "[MORE]");
	os_read_key(0, TRUE);
	setTextXY(saved_x,saved_y);
	os_display_string((zchar*) "      ");
	setTextXY(saved_x,saved_y);

	/* Restore text font and style */
	current_font = saved_font;
	current_style = saved_style;

	adjust_style();
} /* os_more_prompt */

void os_set_text_style(int x)
{
	current_style = x;
	adjust_style();
} /* os_set_text_style */

int os_from_true_colour(zword UNUSED (colour))
{
	/* Nothing here yet */
	return 0;
} /* os_from_true_colour */


/*
 * os_to_true_colour
 *
 * Given a colour index, return the appropriate true colour.
 *
 */
zword os_to_true_colour(int UNUSED (index))
{
	/* Nothing here yet */
	return 0;
} /* os_to_true_colour */

void os_set_font(int new_font)
{
	current_font = new_font;
} /* os_set_font */


void hp_init_output(void) {
//	z_header.config |= CONFIG_COLOUR | CONFIG_BOLDFACE | CONFIG_EMPHASIS;

	if (z_header.version == V3) {
		z_header.config |= CONFIG_SPLITSCREEN;
		z_header.flags &= ~OLD_SOUND_FLAG;
	}

	if (z_header.version >= V5) {
		z_header.flags &= ~SOUND_FLAG;
	}

	z_header.font_width = 1; 
	z_header.font_height = 1;
	z_header.screen_height = z_header.screen_rows;
	z_header.screen_width = z_header.screen_cols;
	z_header.default_foreground = WHITE_COLOUR;
	z_header.default_background = BLACK_COLOUR;
	os_erase_area(1, 1, z_header.screen_rows, z_header.screen_cols, -2);
}	

bool os_picture_data(int num, int *height, int *width){return FALSE;}
void os_draw_picture (int num, int row, int col){}
int os_peek_colour (void) {return BLACK_COLOUR; }
