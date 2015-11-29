/*
  impose.h - impose pdf-files
  Copyright (C) 2015 Stefan Klein

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

#ifndef _JUGGLER_IMPOSE_H_
#define _JUGGLER_IMPOSE_H_

#include "document.h"
#include "error.h"

typedef struct
{
	size_t pages_count;
	double **pages_rects;
	/* extra elements, ... */
}  juggler_sheet_t;


typedef struct
{
	size_t pagenum;
//	fz_ctm *ctm; // todo.. how do we get the positions again if they have been converted to a ctm before?
} juggler_assignment;

typedef struct
{
	juggler_sheet_t *sheet;
	size_t pages_count;
	juggler_assignment *assignments; /* there is something missing! we need this array for every sheet */
} juggler_imposed_sheet_t;


typedef struct 
{
	size_t sheets_count;
	size_t sheets_avail;
	juggler_imposed_sheet_t *sheets;

} juggler_impose_t;

ErrorCode load_sheets_info(); // TODO: args... per thread?

ErrorCode juggler_get_sheet_capacity(juggler_sheet_t *sheet, size_t capacity);

ErrorCode juggler_impose_start(juggler_impose_t **impose);


/* sheet managment */
ErrorCode juggler_impose_sheet_add(juggler_impose_t *impose, juggler_sheet_t *sheet, size_t *pos);

ErrorCode juggler_impose_sheet_del(juggler_impose_t *impose, size_t pos); // what to do with all assigned pages?

ErrorCode juggler_impose_sheet_get(juggler_impose_t *impose, size_t pos, juggler_imposed_sheet_t **sheet);

ErrorCode juggler_impose_sheet_replace(juggler_impose_t *impose, size_t pos, juggler_sheet_t *new_sheet);

ErrorCode juggler_impose_sheet_get_page(juggler_impose_t *impose, size_t sheet_pos, size_t page_on_sheet_pos, size_t *pagenum);

ErrorCode juggler_impose_sheet_get_pages_count(juggler_impose_t *impose, size_t sheet_pos, size_t page_on_sheet_pos, size_t *pagecount);

/* page distribution on all available pages */
typedef enum { juggler_distribute_stapled, juggler_distribute_normal } juggler_distribute_method_t;

ErrorCode juggler_impose_assign(juggler_impose_t *impose, juggler_distribute_method_t method, size_t *pagenums, size_t num);

/* page position */
typedef enum { juggler_impose_origin_center, juggler_impose_origin_inner, juggler_impose_origin_outer } juggler_impose_origin_t;

ErrorCode juggler_impose_page_position(juggler_impose_t *impose, size_t sheet_pos, size_t page_on_sheet_pos,
	juggler_impose_origin_t origin, double diff_x, double diff_y);

ErrorCode juggler_impose_page_locate(juggler_impose_t *impose, size_t sheet_pos, size_t page_on_sheet_pos,
	juggler_impose_origin_t *origin, double *diff_x, double *diff_y);

/* finish that all */
ErrorCode juggler_impose_do(juggler_t *doc, juggler_impose_t *impose);

ErrorCode juggler_impose_free(juggler_impose_t *impose);

#endif /* _JUGGLER_IMPOSE_H_ */
