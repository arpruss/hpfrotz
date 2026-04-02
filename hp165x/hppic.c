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

// currently only implemented for Journey

static short data[][3] = {
{ 1, 166, 188 }, //  at 1 (1864)  size   0 x 166   188 colours   colour map ------
{ 2, 166, 188 }, //  at 2 (6737)  size   0 x 166   188 colours   colour map ------
{ 3, 164, 182 }, //  at 3 (11179)  size   0 x 164   182 colours   colour map ------
{ 5, 165, 183 }, //  at 5 (14339)  size   0 x 165   183 colours   colour map ------
{ 6, 165, 143 }, //  at 6 (19074)  size   0 x 165   143 colours   colour map ------   transparent is 0
{ 7, 165, 183 }, //  at 7 (21368)  size   0 x 165   183 colours   colour map ------
{ 8, 151, 183 }, //  at 8 (25532)  size   0 x 151   183 colours   colour map ------   transparent is 0
{ 9, 165, 183 }, //  at 9 (28119)  size   0 x 165   183 colours   colour map ------
{ 10, 165, 182 }, //  at 10 (32359)  size   0 x 165   182 colours   colour map ------
{ 11, 168, 190 }, //  at 11 (36395)  size   0 x 168   190 colours   colour map ------
{ 12, 68, 82 }, //  at 12 (40718)  size   0 x  68   82 colours   colour map ------   transparent is 0
{ 13, 168, 190 }, //  at 13 (41605)  size   0 x 168   190 colours   colour map ------
{ 14, 164, 182 }, //  at 14 (46215)  size   0 x 164   182 colours   colour map ------
{ 15, 164, 182 }, //  at 15 (50541)  size   0 x 164   182 colours   colour map ------
{ 16, 166, 189 }, //  at 16 (54295)  size   0 x 166   189 colours   colour map ------
{ 17, 165, 183 }, //  at 17 (58397)  size   0 x 165   183 colours   colour map ------
{ 18, 166, 189 }, //  at 18 (62813)  size   0 x 166   189 colours   colour map ------
{ 19, 164, 183 }, //  at 19 (67575)  size   0 x 164   183 colours   colour map ------
{ 20, 165, 183 }, //  at 20 (71301)  size   0 x 165   183 colours   colour map ------
{ 21, 165, 156 }, //  at 21 (75439)  size   0 x 165   156 colours   colour map ------   transparent is 0
{ 22, 165, 183 }, //  at 22 (78822)  size   0 x 165   183 colours   colour map ------
{ 23, 165, 183 }, //  at 23 (82485)  size   0 x 165   183 colours   colour map ------
{ 24, 170, 183 }, //  at 24 (85958)  size   0 x 170   183 colours   colour map ------
{ 25, 164, 182 }, //  at 25 (90082)  size   0 x 164   182 colours   colour map ------
{ 26, 164, 182 }, //  at 26 (94330)  size   0 x 164   182 colours   colour map ------
{ 28, 166, 183 }, //  at 28 (95717)  size   0 x 166   183 colours   colour map ------
{ 30, 165, 183 }, //  at 30 (98901)  size   0 x 165   183 colours   colour map ------
{ 32, 165, 184 }, //  at 32 (102852)  size   0 x 165   184 colours   colour map ------
{ 33, 168, 189 }, //  at 33 (107115)  size   0 x 168   189 colours   colour map ------
{ 34, 165, 183 }, //  at 34 (110670)  size   0 x 165   183 colours   colour map ------
{ 36, 165, 183 }, //  at 36 (114368)  size   0 x 165   183 colours   colour map ------
{ 38, 164, 183 }, //  at 38 (117380)  size   0 x 164   183 colours   colour map ------
{ 39, 163, 183 }, //  at 39 (121781)  size   0 x 163   183 colours   colour map ------
{ 40, 166, 184 }, //  at 40 (124803)  size   0 x 166   184 colours   colour map ------
{ 42, 164, 182 }, //  at 42 (128813)  size   0 x 164   182 colours   colour map ------
{ 43, 165, 182 }, //  at 43 (132434)  size   0 x 165   182 colours   colour map ------
{ 45, 164, 184 }, //  at 45 (136366)  size   0 x 164   184 colours   colour map ------
{ 46, 137, 183 }, //  at 46 (140632)  size   0 x 137   183 colours   colour map ------   transparent is 0
{ 47, 164, 182 }, //  at 47 (143477)  size   0 x 164   182 colours   colour map ------
{ 48, 164, 183 }, //  at 48 (147462)  size   0 x 164   183 colours   colour map ------
{ 49, 164, 183 }, //  at 49 (150920)  size   0 x 164   183 colours   colour map ------
{ 50, 165, 183 }, //  at 50 (154525)  size   0 x 165   183 colours   colour map ------
{ 51, 165, 182 }, //  at 51 (158172)  size   0 x 165   182 colours   colour map ------
{ 52, 165, 183 }, //  at 52 (161824)  size   0 x 165   183 colours   colour map ------
{ 53, 165, 184 }, //  at 53 (164634)  size   0 x 165   184 colours   colour map ------
{ 56, 165, 183 }, //  at 56 (169022)  size   0 x 165   183 colours   colour map ------
{ 58, 165, 183 }, //  at 58 (172873)  size   0 x 165   183 colours   colour map ------
{ 61, 165, 181 }, //  at 61 (176862)  size   0 x 165   181 colours   colour map ------
{ 62, 166, 183 }, //  at 62 (179259)  size   0 x 166   183 colours   colour map ------
{ 63, 166, 184 }, //  at 63 (183325)  size   0 x 166   184 colours   colour map ------
{ 64, 164, 183 }, //  at 64 (187102)  size   0 x 164   183 colours   colour map ------
{ 67, 164, 183 }, //  at 67 (191726)  size   0 x 164   183 colours   colour map ------
{ 68, 165, 183 }, //  at 68 (195364)  size   0 x 165   183 colours   colour map ------
{ 69, 166, 183 }, //  at 69 (199519)  size   0 x 166   183 colours   colour map ------
{ 70, 161, 180 }, //  at 70 (203997)  size   0 x 161   180 colours   colour map ------
{ 72, 165, 183 }, //  at 72 (207455)  size   0 x 165   183 colours   colour map ------
{ 73, 164, 183 }, //  at 73 (210269)  size   0 x 164   183 colours   colour map ------
{ 74, 165, 182 }, //  at 74 (214674)  size   0 x 165   182 colours   colour map ------
{ 77, 165, 183 }, //  at 77 (219395)  size   0 x 165   183 colours   colour map ------
{ 78, 165, 183 }, //  at 78 (223746)  size   0 x 165   183 colours   colour map ------
{ 80, 165, 183 }, //  at 80 (227900)  size   0 x 165   183 colours   colour map ------
{ 81, 97, 161 }, //  at 81 (231215)  size   0 x  97   161 colours   colour map ------   transparent is 0
{ 82, 76, 183 }, //  at 82 (233191)  size   0 x  76   183 colours   colour map ------   transparent is 0
{ 83, 165, 183 }, //  at 83 (234984)  size   0 x 165   183 colours   colour map ------
{ 84, 164, 183 }, //  at 84 (238887)  size   0 x 164   183 colours   colour map ------
{ 85, 138, 183 }, //  at 85 (243575)  size   0 x 138   183 colours   colour map ------   transparent is 0
{ 86, 165, 183 }, //  at 86 (246160)  size   0 x 165   183 colours   colour map ------
{ 87, 165, 183 }, //  at 87 (250378)  size   0 x 165   183 colours   colour map ------
{ 88, 165, 182 }, //  at 88 (252779)  size   0 x 165   182 colours   colour map ------   transparent is 0
{ 89, 168, 183 }, //  at 89 (255947)  size   0 x 168   183 colours   colour map ------
{ 90, 165, 183 }, //  at 90 (260713)  size   0 x 165   183 colours   colour map ------
{ 91, 165, 134 }, //  at 91 (264766)  size   0 x 165   134 colours   colour map ------   transparent is 0
{ 92, 165, 183 }, //  at 92 (267802)  size   0 x 165   183 colours   colour map ------
{ 93, 165, 183 }, //  at 93 (270710)  size   0 x 165   183 colours   colour map ------
{ 94, 165, 183 }, //  at 94 (274418)  size   0 x 165   183 colours   colour map ------
{ 95, 166, 183 }, //  at 95 (278455)  size   0 x 166   183 colours   colour map ------
{ 96, 165, 183 }, //  at 96 (282796)  size   0 x 165   183 colours   colour map ------
{ 97, 163, 183 }, //  at 97 (285896)  size   0 x 163   183 colours   colour map ------
{ 98, 165, 183 }, //  at 98 (289078)  size   0 x 165   183 colours   colour map ------
{ 99, 165, 183 }, //  at 99 (292461)  size   0 x 165   183 colours   colour map ------
{ 100, 165, 183 }, //  at 100 (296181)  size   0 x 165   183 colours   colour map ------
{ 101, 155, 143 }, //  at 101 (300089)  size   0 x 155   143 colours   colour map ------   transparent is 0
{ 102, 165, 183 }, //  at 102 (302883)  size   0 x 165   183 colours   colour map ------
{ 103, 165, 183 }, //  at 103 (306890)  size   0 x 165   183 colours   colour map ------
{ 104, 166, 183 }, //  at 104 (311053)  size   0 x 166   183 colours   colour map ------
{ 106, 32, 17 }, //  at 106 (0)  size   0 x  32   17 colours   colour map ------
{ 109, 166, 183 }, //  at 109 (314852)  size   0 x 166   183 colours   colour map ------
{ 110, 165, 183 }, //  at 110 (318302)  size   0 x 165   183 colours   colour map ------
{ 111, 165, 181 }, //  at 111 (321096)  size   0 x 165   181 colours   colour map ------
{ 113, 165, 182 }, //  at 113 (323083)  size   0 x 165   182 colours   colour map ------
{ 114, 165, 183 }, //  at 114 (326394)  size   0 x 165   183 colours   colour map ------
{ 115, 92, 183 }, //  at 115 (329892)  size   0 x  92   183 colours   colour map ------   transparent is 0
{ 116, 165, 183 }, //  at 116 (331678)  size   0 x 165   183 colours   colour map ------
{ 117, 165, 183 }, //  at 117 (336034)  size   0 x 165   183 colours   colour map ------
{ 119, 165, 183 }, //  at 119 (339669)  size   0 x 165   183 colours   colour map ------
{ 121, 165, 182 }, //  at 121 (343085)  size   0 x 165   182 colours   colour map ------
{ 122, 165, 183 }, //  at 122 (346447)  size   0 x 165   183 colours   colour map ------
{ 123, 165, 183 }, //  at 123 (349521)  size   0 x 165   183 colours   colour map ------
{ 124, 165, 183 }, //  at 124 (352623)  size   0 x 165   183 colours   colour map ------
{ 126, 165, 183 }, //  at 126 (356492)  size   0 x 165   183 colours   colour map ------
{ 128, 165, 183 }, //  at 128 (359789)  size   0 x 165   183 colours   colour map ------
{ 129, 165, 183 }, //  at 129 (364527)  size   0 x 165   183 colours   colour map ------   transparent is 0
{ 131, 165, 183 }, //  at 131 (367422)  size   0 x 165   183 colours   colour map ------
{ 132, 165, 183 }, //  at 132 (372019)  size   0 x 165   183 colours   colour map ------
{ 133, 166, 184 }, //  at 133 (376396)  size   0 x 166   184 colours   colour map ------
{ 135, 165, 183 }, //  at 135 (380342)  size   0 x 165   183 colours   colour map ------
{ 136, 165, 183 }, //  at 136 (385051)  size   0 x 165   183 colours   colour map ------
{ 138, 165, 183 }, //  at 138 (388325)  size   0 x 165   183 colours   colour map ------
{ 139, 139, 125 }, //  at 139 (391229)  size   0 x 139   125 colours   colour map ------   transparent is 0
{ 140, 165, 183 }, //  at 140 (392827)  size   0 x 165   183 colours   colour map ------
{ 141, 146, 46 }, //  at 141 (397678)  size   0 x 146   46 colours   colour map ------   transparent is 0
{ 143, 165, 183 }, //  at 143 (398564)  size   0 x 165   183 colours   colour map ------
{ 144, 165, 183 }, //  at 144 (402466)  size   0 x 165   183 colours   colour map ------
{ 145, 105, 65 }, //  at 145 (405963)  size   0 x 105   65 colours   colour map ------   transparent is 0
{ 146, 164, 183 }, //  at 146 (406791)  size   0 x 164   183 colours   colour map ------
{ 147, 104, 108 }, //  at 147 (410491)  size   0 x 104   108 colours   colour map ------   transparent is 0
{ 148, 165, 183 }, //  at 148 (412011)  size   0 x 165   183 colours   colour map ------
{ 149, 165, 183 }, //  at 149 (416863)  size   0 x 165   183 colours   colour map ------
{ 150, 165, 183 }, //  at 150 (421657)  size   0 x 165   183 colours   colour map ------
{ 151, 86, 93 }, //  at 151 (426836)  size   0 x  86   93 colours   colour map ------   transparent is 0
{ 152, 86, 93 }, //  at 152 (427020)  size   0 x  86   93 colours   colour map ------   transparent is 0
{ 153, 86, 93 }, //  at 153 (427174)  size   0 x  86   93 colours   colour map ------   transparent is 0
{ 154, 86, 93 }, //  at 154 (427327)  size   0 x  86   93 colours   colour map ------   transparent is 0
{ 155, 86, 93 }, //  at 155 (427485)  size   0 x  86   93 colours   colour map ------   transparent is 0
{ 156, 86, 93 }, //  at 156 (427652)  size   0 x  86   93 colours   colour map ------   transparent is 0
{ 157, 166, 183 }, //  at 157 (427823)  size   0 x 166   183 colours   colour map ------
{ 158, 104, 39 }, //  at 158 (431888)  size   0 x 104   39 colours   colour map ------   transparent is 0
{ 159, 165, 183 }, //  at 159 (432336)  size   0 x 165   183 colours   colour map ------
{ 160, 480, 299 }, //  at 160 (436446)  size   0 x 480   299 colours   colour map ------
{ 161, 165, 184 }, //  at 161 (449732)  size   0 x 165   184 colours   colour map ------
{ 162, 165, 183 }, //  at 162 (453661)  size   0 x 165   183 colours   colour map ------
{ 163, 165, 183 }, //  at 163 (457495)  size   0 x 165   183 colours   colour map ------
{ 164, 165, 183 }, //  at 164 (461964)  size   0 x 165   183 colours   colour map ------
{ 165, 165, 182 }, //  at 165 (464991)  size   0 x 165   182 colours   colour map ------
{ 166, 166, 183 }, //  at 166 (469066)  size   0 x 166   183 colours   colour map ------
};

bool os_picture_data(int num, int *height, int *width){
	if (num>0) {
		for (int i=0; i<sizeof(data)/sizeof(*data); i++) 
			if (data[i][0] == num) {
				*height = (data[i][2]+13)/14;
				*width = (data[i][1]+7)/8;
				return 1;
			}
		return 1;
	}
	else {
		*height = 166;
		*width = 2; // version
		return 0;
	}
}	

void os_draw_picture (int num, int row, int col){
	setTextXY(col-1,row-1);
	printf("Image %d", num);
}

int os_peek_colour (void) {return BLACK_COLOUR; }
