/*
  render.h - functions related to rendering
  Copyright (C) 2014 Stefan Klein

  This program is free software: you can redistribute it and/or modify
  it under the terms of the GNU Affero General Public License as
  published by the Free Software Foundation, either version 3 of the
  License, or (at your option) any later version.
   
  This program is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
  GNU Affero General Public License for more details. 

  You should have received a copy of the GNU Affero General Public License
  along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#ifndef _JUGGLER_RENDER_H_
#define _JUGGLER_RENDER_H_

#include <mupdf/fitz.h>

typedef struct
{
	unsigned char *samples;
	fz_pixmap *pixmap;
	double zoom;
	int pagenum;
	int x;
	int y;
	int width;
	int height;
} render_data_t;

typedef struct
{
	int x;
	int width;
	int height;
} page_size;

#include "document.h"


#endif /* _JUGGLER_RENDER_H_ */
