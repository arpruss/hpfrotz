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
// Uses Huffman decompression code based on Spatterlight's

#include <hp165x.h>
#include <string.h>
#include <assert.h>
#include <stdio.h>

#include "hpfrotz.h"
#include <hpposix.h>

//pixel size: 0.2754mm x 0.3258mm
//pixel height to width: 1.18:1

#define ASPECT_FIX_NUMERATOR   118
#define ASPECT_FIX_DENOMINATOR 100

#define IMAGE_FULLSCREEN_WIDTH  480
#define IMAGE_FULLSCREEN_HEIGHT 300

#define FILE_BUFFER_SIZE   128 // must fit in int16_t

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

// the scaling code does not work for a factor of 2 or greater
static void getStretches(uint16_t imageWidth, uint16_t imageHeight, uint16_t* xDeltaP,
	uint16_t* xDenominatorP, uint16_t* yDeltaP, uint16_t* yDenominatorP) {
	(void)imageHeight;
	if (imageWidth < 479 && story_id == JOURNEY) {
		// less scaling looks better for Journey
		*xDeltaP = ASPECT_FIX_NUMERATOR-ASPECT_FIX_DENOMINATOR;
		*xDenominatorP = ASPECT_FIX_DENOMINATOR;
		*yDeltaP = 0;
		*yDenominatorP = 1;
	}	
	else {
		*xDeltaP = getScreenWidth()-IMAGE_FULLSCREEN_WIDTH;
		*xDenominatorP = IMAGE_FULLSCREEN_WIDTH;
		*yDeltaP = getScreenHeight()-IMAGE_FULLSCREEN_HEIGHT;
		*yDenominatorP = IMAGE_FULLSCREEN_HEIGHT;
	}
}

void hp_close_pictures(void) {
	if (picFile != NULL) {
		fclose(picFile);
        picFile = NULL;
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
	if (story_id == JOURNEY) {
		picFile = fopen("journey.bw", "rb");
		if (picFile == NULL) {
			picFile = fopen("PIC.DATA", "rb");
			if (picFile == NULL)
				goto ERROR;
		}
	}
	else if (story_id == SHOGUN) {
		picFile = fopen("a5.bwmac.1", "rb");
		if (picFile == NULL) {
			picFile = fopen("PIC.DATA", "rb");
			if (picFile == NULL)
				goto ERROR;
		}
	}
	else {
		goto ERROR;
	}
	
	fontHeight = getFontHeight();
	fontWidth = getFontWidth();
	
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
				uint16_t xDelta;
				uint16_t xDenominator;
				uint16_t yDelta;
				uint16_t yDenominator;
				
				getStretches(directory[i].width,directory[i].height,&xDelta,&xDenominator,&yDelta,&yDenominator);
				
				*height = (directory[i].height * (yDelta+yDenominator)/yDenominator + fontHeight - 1) / fontHeight;
				*width = (directory[i].width * (xDelta+xDenominator)/xDenominator + fontWidth - 1) / fontWidth;
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

/* Huffman decompression code originally from Spatterlight, but optimized and made
   to work with disk streaming. */

void drawImage(uint16_t x, uint16_t y, struct picture_directory* pd) {
	if (picFile == NULL)
		return;
	
	uint16_t xDelta;
	uint16_t xDenominator;
	uint16_t yDelta;
	uint16_t yDenominator;
	
	getStretches(pd->width,pd->height,&xDelta,&xDenominator,&yDelta,&yDenominator);
	
	fseek(picFile, pd->data_address, SEEK_SET);
	uint32_t remainingSize = 0;
	fread(1+(char*)&remainingSize, 3, 1, picFile);
	fseek(picFile, 3, SEEK_CUR);

	uint16_t width = pd->width;
	uint16_t height = pd->height;
	uint8_t* buffer = malloc(FILE_BUFFER_SIZE + width);
	if (buffer == NULL)
		return;
	// store both input and output in one chunk to save memory and be
	// more efficient
	uint8_t* line = buffer+FILE_BUFFER_SIZE;
    memset(line, 0, width);

	uint8_t decodeMask = 0;
    unsigned char outValue = 0;
    unsigned char colorIndex = 0;

	uint16_t lineSize = getScreenWidth()/4;
	uint8_t startXMask = 1 << (3-x%4);
	uint16_t imageY = 0;
	uint16_t outY = y;

	uint16_t imageX = 0;	
	uint16_t yStretchCount = 0;

	int16_t inBuffer = 0;
	int16_t bufferIndex = -1;
	uint8_t currentByte = 0;
	volatile uint16_t* posTop = SCREEN + x/4;

    // Compression method:
    //  Each row except the top one is replaced by a XOR of it with the previous row.
    //  The full stream is RLE compressed.
    //        0x00-0x0F = literal value
    //        0x10-0x7F = repeat last literal value: value = repeat count plus 0x10
    //  The RLE compressed stream is then Huffman encoded, with a common tree for all
    //  the images, and with leaf nodes indicated by bit 7 set.

	while (1) {
        if (outValue == 0) {
            while (outValue < 0x80) {
                if (decodeMask == 0) {
					decodeMask = 0x80;
                    bufferIndex++;

                    if (bufferIndex >= inBuffer) {
                        
                        if (remainingSize == 0)
                            goto DONE; // ERROR!
                        
                        // refill buffer
                        
                        uint16_t toRead;
                        
                        if (remainingSize <= FILE_BUFFER_SIZE)
                            toRead = remainingSize;
                        else
                            toRead = FILE_BUFFER_SIZE;
                        
                        inBuffer = fread(buffer, 1, toRead, picFile);
                        
                        if (inBuffer == 0)
                            goto DONE; // ERROR
                        
                        remainingSize -= inBuffer;
                        bufferIndex = 0;
                    }

                    currentByte = buffer[bufferIndex];
				}
                
                if (currentByte & decodeMask) {
                    outValue = tree[2 * outValue + 1];
                } else {
                    outValue = tree[2 * outValue];
                }

				decodeMask >>= 1;
            }

            if (outValue >= 0x90) {
                outValue -= 0x90;
            }  else {
                colorIndex = outValue&0x7F;
                outValue = 0;
            }
        }
        else {
            outValue--;
        }

		line[imageX++] ^= colorIndex; 

		if (imageX >= width) {
            // line filled up, draw it, y-scaling as needed
			yStretchCount += yDelta;
			do {
				volatile uint16_t* pos = posTop + outY * lineSize;
				uint16_t xStretchCount = 0;
				uint8_t mask = startXMask;

				for (imageX=0; imageX<width; imageX++) {
					uint8_t pixel = line[imageX];
					
                    if (pixel == 2) {
                        *SCREEN_MEMORY_CONTROL = WRITE_WHITE;
                        *pos = mask;
                    }
                    else if (pixel == 3) {
                        *SCREEN_MEMORY_CONTROL = WRITE_BLACK;
                        *pos = mask;
                    }
					
                    mask >>= 1;
					
                    if (mask == 0) {
						pos++;
						mask = 8;
					}
                    
					xStretchCount += xDelta;
					
                    if (xStretchCount >= xDenominator) {
						if (pixel != 0)
							*pos = mask;
					
                        mask >>= 1;
						
                        if (mask == 0) {
							pos++;
							mask = 8;
						}
						
                        xStretchCount -= xDenominator;
					}
				}
                
				outY++;
                
				if (yStretchCount < yDenominator)
					break;
                
				yStretchCount -= yDenominator;
			} while(1);
			
			imageY++;
			if (imageY >= height)
				break; // all done
			imageX = 0;
		}
	}

DONE:
	free(buffer);
}

void os_draw_picture (int num, int row, int col){
	if (picFile == NULL)
		return;
	
	struct picture_directory* pd = directory;
	for (unsigned short i=0; i<header.count; i++,pd++) 
		if (pd->number == num) {
			drawImage((col-1)*FONT_WIDTH, (row-1)*fontHeight, pd);
			return;
		}
}

int os_peek_colour (void) {
	if (0 == ( z_header.flags & GRAPHICS_FLAG ) )
		return BLACK_COLOUR;
	
	if (readPixel(getTextX()*FONT_WIDTH, getTextY()*fontHeight,0))
		return WHITE_COLOUR;
	else
		return BLACK_COLOUR;
}
