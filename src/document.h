/*
  document.h - basic operations of juggler
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

#ifndef _JUGGLER_DOCUMENT_H_
#define _JUGGLER_DOCUMENT_H_

#include "document.h"
#include "render.h"
#include "error.h"

#include <mupdf/fitz.h>
#include <mupdf/pdf.h>

struct juggler_redo;

#define RENDER_BUFFER_COUNT 4
typedef struct
{
	pdf_document *pdf; // MuPDF's data
	int pagecount;
	page_size *sizes;
	render_data_t rendered[RENDER_BUFFER_COUNT]; // already rendered pages/parts of pages
	size_t recent_rendered;

	struct juggler_redo *redo;
	fz_context *ctx;
} juggler_t;

/* initialize all components, must be called before anything else */
extern ErrorCode juggler_init(fz_context **init_data);

/* open a pdf-document */
extern ErrorCode juggler_open(fz_context *ctx, char *filename, juggler_t **juggler_pdf);

/* save any changes you made to the document */
extern ErrorCode juggler_save(juggler_t *juggler, char *filename);

/* close the document and free all additional structures */
extern ErrorCode juggler_close(juggler_t *juggler);

#endif /* _JUGGLER_DOCUMENT_H_ */















