/*
 *
 * This file is part of Frotz.
 *
 * Frotz is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License (version 3) as published by
 * the Free Software Foundation; 
 
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

// currently only implemented for Journey

#include <hp165x.h>
#include <string.h>
#include <assert.h>
#include <stdio.h>

#include "hpfrotz.h"

//pixel size: 0.2754mm x 0.3258mm
//pixel height to width: 1.18:1

#define STRETCH_NUM   118
#define STRETCH_DEN   100

static FILE* picFile = NULL;

static struct picture_directory {
    uint16_t number;
    uint16_t width;
    uint16_t height;
    uint16_t flags;
    uint32_t data_address;
} *directory = NULL;

static struct {
    uint8_t part;
    uint8_t flags;
    uint16_t skip1;
    uint16_t count;
    uint16_t global_pointer;
    uint8_t directory_entry_size;;
    uint8_t skip2;
    uint16_t checksum;
    uint16_t skip3;
    uint16_t version;	
} header;

static uint8_t* tree = NULL;
static uint16_t fontWidth;
static uint16_t fontHeight;

void hp_close_pictures(void) {
	if (picFile != NULL) {
		fclose(picFile);
	}
	if (directory != NULL) {
		free(directory);
		directory = NULL;
	}
	if (tree != NULL) {
		free(tree);
		tree = NULL;
	}
}

void hp_init_pictures(void) {
	if (story_id != JOURNEY)
		return;
	
	fontHeight = getFontHeight();
	fontWidth = getFontWidth();
	
	picFile = fopen("PIC.DATA", "rb");
	if (picFile == NULL) 
		goto ERROR;
	
	f_setup.blorb_file = "picFile"; // NOT ACTUALLY A BLORB FILE, BUT STOPS WARNINGS
	
	if (1 != fread(&header, sizeof(header), 1, picFile)) 
		goto ERROR;

	directory = malloc(sizeof(*directory) * header.count);
	if (directory == NULL) 
		goto ERROR;

	for (unsigned short i=0; i<header.count; i++) {
		fread(directory+i, sizeof(*directory)-4, 1, picFile);
		directory[i].data_address = 0; // stored in 3 bytes
		fread((char*)&(directory[i].data_address) + 1, 3, 1, picFile);
		fseek(picFile, sizeof(header) + header.directory_entry_size*(i+1), SEEK_SET);
	}
	if (header.flags == 0xE) {
		tree = malloc(256);
		if (tree == NULL) {
			goto ERROR;
		}
		fread(tree, 256, 1, picFile);
		return;
	}

ERROR:
	hp_close_pictures();
	z_header.flags &= ~GRAPHICS_FLAG;
}

bool os_picture_data(int num, int *height, int *width){
	if (picFile == NULL)
		return 0;
	
	if (num>0) {
		struct picture_directory* pd = directory;
		for (unsigned short i=0; i<header.count; i++, pd++) 
			if (directory[i].number == num) {
				*height = (directory[i].height + fontHeight - 1) / fontHeight;
				*width = (directory[i].width * STRETCH_NUM / STRETCH_DEN + fontWidth - 1) / fontWidth;
				return 1;
			}
		return 0;
	}
	else {
		*height = header.count;
		*width = header.version;
		return 0;
	}
}	


/* Huffman decompression code from Spatterlight */


void drawImage(uint16_t x, uint16_t y, struct picture_directory* pd) {
	fseek(picFile, pd->data_address, SEEK_SET);
	uint32_t compressedSize = 0;
	fread(1+(char*)&compressedSize, 3, 1, picFile);
	fseek(picFile, 3, SEEK_CUR);
	uint8_t* data = malloc(compressedSize);
	if (data == NULL)
		return;
	if (compressedSize != fread(data, 1, compressedSize, picFile)) {
		free(data);
		return;
	}
	
	uint16_t width = pd->width;
	uint16_t height = pd->height;
	uint8_t* line = malloc(width);
	uint16_t index = 0;
	if (line == NULL) {
		free(data);
		return;
	}
    uint8_t bit = 7;
	int j = 0;
    unsigned char repeats = 0;
    unsigned char color_index = 0;

	uint16_t lineSize = getScreenWidth()/4;
	volatile uint16_t* posTop = SCREEN + y * lineSize + (x/4)*2;
	uint8_t startMask = 1 << (3-x%4);
	uint16_t imageY = 0;
	uint16_t imageX = 0;	
	uint8_t mask = startMask;
	volatile uint16_t* pos = posTop;
	uint16_t stretchCount = 0;
	
	// colors 00, 02, 03
	while (1) {
        if (repeats == 0) {
            // Repeat while bit 7 of count is unset
            while (repeats < 0x80) {
                // This conditional is inverted in
                // the Magnetic code in ms_extract1().
                // That code will add 1 and read from the subsequent
                // byte in the array if the bit is *unset*.
                // Here we do that if the bit is *set*.
                if (data[j] & (1 << bit)) {
                    repeats = tree[2 * repeats + 1];
                } else {
                    repeats = tree[2 * repeats];
                }

                if (!bit)
                    j++;

                if (j > compressedSize) {
					goto ERROR;
                }

                bit = bit ? bit - 1 : 7;
            }

            repeats &= 0x7f;
            if (repeats >= 0x10) {
                // We subtract 1 less here than the
                // Magnetic code in ms_extract1()
                repeats -= 0xf;
            }  else {
                color_index = repeats;
                repeats = 1;
            }
        }

		repeats--;
		uint8_t pixel = (imageY == 0) ? color_index : (color_index ^ line[imageX]);
		line[imageX] = pixel;

		if (pixel != 0) {
			if (pixel == 2) {
				*SCREEN_MEMORY_CONTROL = WRITE_WHITE;
			}
			else if (pixel == 3) {
				*SCREEN_MEMORY_CONTROL = WRITE_BLACK;
			}
			*pos = mask;
		}
		mask >>= 1;
		if (mask == 0) {
			pos++;
			mask = 8;
		}
#if STRETCH_NUM > STRETCH_DEN
		stretchCount += (STRETCH_NUM-STRETCH_DEN);
		if (stretchCount >= STRETCH_DEN) {
			if (pixel != 0)
				*pos = mask;
			mask >>= 1;
			if (mask == 0) {
				pos++;
				mask = 8;
			}
			stretchCount -= STRETCH_DEN;
		}
#endif
		imageX++;
		if (imageX >= width) {
			imageX = 0;
			imageY++;
			mask = startMask;
			pos = posTop + imageY * lineSize;
			stretchCount = 0;
			if (imageY >= height)
				break;
		}
	}

ERROR:	
	free(line);
	free(data);
}

void os_draw_picture (int num, int row, int col){
	if (picFile == NULL)
		return;
	
	struct picture_directory* pd = directory;
	for (unsigned short i=0; i<header.count; i++,pd++) 
		if (pd->number == num) {
			drawImage((col-1)*fontWidth, (row-1)*fontHeight, pd);
			return;
		}
}

int os_peek_colour (void) {return BLACK_COLOUR; }

void hp_pic_init(void) {
}