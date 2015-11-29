/*
  page-remove.c - remove pages from a pdf
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

#include "page-remove.h"

#include "internal.h"

// Be aware that page_num needs to be the index of the page to delete + 1
// Return values:
//  -2: an error occured
//  -1: do count-- on current
//   0: delete the object that is current
//  >0: continue to next node
static int remove_from_page_tree(juggler_t *juggler, int page_num, pdf_obj *current)
{
	if(!strcmp(pdf_to_name(juggler->ctx, pdf_dict_gets(juggler->ctx, current, "Type")), "Page")) {
		return(--page_num); // returns -1 if this is the right page, else a higher number
	} if(!strcmp(pdf_to_name(juggler->ctx, pdf_dict_gets(juggler->ctx, current, "Type")), "Pages")) {
		pdf_obj *kids = pdf_dict_gets(juggler->ctx, current, "Kids");
		pdf_obj *count_obj = pdf_dict_gets(juggler->ctx, current, "Count");
		if(!pdf_is_array(juggler->ctx, kids) || !pdf_is_int(juggler->ctx, count_obj))
			return(-2);

		int count = pdf_to_int(juggler->ctx, count_obj);
		int i;
		for(i = 0; i < count; i++) {
			pdf_obj *current_kid = pdf_array_get(juggler->ctx, kids, i);

			page_num = remove_from_page_tree(juggler, page_num, current_kid);
			if(page_num == 0) {
				printf("Deleting object: %d %d obj\n", 
					pdf_to_num(juggler->ctx, current_kid), 
					pdf_to_gen(juggler->ctx, current_kid));
				pdf_array_delete(juggler->ctx, kids, i);

                // update count value
				count--;
				pdf_obj *new_count = pdf_new_int(juggler->ctx, juggler->pdf, count);
				pdf_dict_puts_drop(juggler->ctx, current, "Count", new_count);
				
				if(count < 1) // if this was the only kid...
					return(0); // ...delete this pages-object too
				else
					return(-1); // everything is done
			} else if(page_num == -1) {
				count--;
				pdf_obj *new_count = pdf_new_int(juggler->ctx, juggler->pdf, count);
				pdf_dict_puts_drop(juggler->ctx, current, "Count", new_count);

				return(-1);
			} else if(page_num < -1) {
				return(page_num);
			}
		}

		return(page_num);
	}

	return(-2);
}

ErrorCode juggler_remove_page(juggler_t *juggler, int page_index)
{
	if(page_index >= juggler->pagecount)
		return(ERROR_INVALID_RANGE);

	pdf_obj *pages = pdf_dict_getp(juggler->ctx, pdf_trailer(juggler->ctx, juggler->pdf), "Root/Pages");
	if(!pdf_is_indirect(juggler->ctx, pages) || !pdf_is_dict(juggler->ctx, pages))
		return(ERROR_NO_PAGES);

	remove_from_page_tree(juggler, page_index + 1, pages);

	juggler_page_tree_changed_due_to_remove(juggler, page_index, 1);

	// TODO: Additional things: names, outlines, ...

	return(NoError);
}


/* Be aware that this function could only delete some of the pages of the
   supplied range if there is a problem with just one page! */
ErrorCode juggler_remove_pages(juggler_t *juggler, 
	int firstIndex, int lastIndex)
{
	if(firstIndex < 0 || firstIndex >= juggler->pagecount || // first is invalid
		lastIndex < 0 || lastIndex >= juggler->pagecount || // last is invalid
		firstIndex > lastIndex ||
		(firstIndex == 0 && lastIndex == juggler->pagecount - 1)) // all pages
	{
		return(ERROR_INVALID_RANGE);
	}

	ErrorCode lastError;
	int i;
	for(i = lastIndex; i >= firstIndex; i--)
		if((lastError = juggler_remove_page(juggler, i)) != NoError)
			return(lastError);

	return(NoError);
}
