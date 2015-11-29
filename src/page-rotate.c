/*
  page-rotate.c - set and get page-rotation
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

#include "page-rotate.h"

#include "internal.h"

ErrorCode juggler_get_page_rotation(juggler_t *juggler, int index, int *rotation)
{
	pdf_obj *pageobj = pdf_lookup_page_obj(juggler->ctx, juggler->pdf, index);
	pdf_obj *rotate = juggler_lookup_inherited_page_item(juggler->ctx, juggler->pdf, pageobj, "Rotate");
	
	if(pdf_is_int(juggler->ctx, rotate))
		*rotation = pdf_to_int(juggler->ctx, rotate);
	else
		*rotation = 0;

	return(NoError);
}

ErrorCode juggler_set_page_rotation(juggler_t *juggler, int index, int rotation)
{
	if(rotation != 0 && rotation != 90 && rotation != 180 && rotation != 270)
		return(ERROR_INVALID_RANGE);

	pdf_obj *pageobj = pdf_lookup_page_obj(juggler->ctx, juggler->pdf, index);
	pdf_obj *new_rotate = pdf_new_int(juggler->ctx, juggler->pdf, rotation);

	pdf_dict_puts_drop(juggler->ctx, pageobj, "Rotate", new_rotate);

	juggler_page_tree_changed(juggler);

	return(NoError);
}
