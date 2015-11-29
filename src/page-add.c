/*
  page-add.c - insert pages from other documents
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

#include "page-add.h"

#include "copy-helper.h"
#include "internal.h"

static int find_destination_pages(fz_context *ctx, pdf_obj *current, int page_num, pdf_obj **dest_pages, int *index)
{
	if(!strcmp(pdf_to_name(ctx, pdf_dict_gets(ctx, current, "Type")), "Page")) {
		return(--page_num);
	} if(!strcmp(pdf_to_name(ctx, pdf_dict_gets(ctx, current, "Type")), "Pages")) {
		pdf_obj *kids = pdf_dict_gets(ctx, current, "Kids");
		pdf_obj *count_obj = pdf_dict_gets(ctx, current, "Count");
		if(!pdf_is_array(ctx, kids) || !pdf_is_int(ctx, count_obj))
			return(-2);

		int count = pdf_to_int(ctx, count_obj);
		int i;
		for(i = 0; i < count; i++) {
			pdf_obj *current_kid = pdf_array_get(ctx, kids, i);

			page_num = find_destination_pages(ctx, current_kid, page_num, dest_pages, index);
			if(page_num == -1) {
				*index = i;
				*dest_pages = current;
				return(-2);
			} else if(page_num == -2) {
				return(-2); // just return, preserve index and dest_pages
			}
		}

		return(page_num);
	}

	return(page_num);
}

ErrorCode juggler_add_pages_from_file(juggler_t *dest, juggler_t *src, int dest_index)
{
	pdf_obj *dest_pages = pdf_dict_getp(dest->ctx, 
		pdf_trailer(dest->ctx, dest->pdf), "Root/Pages");
	int dest_pages_index = pdf_array_len(dest->ctx, 
		pdf_dict_gets(dest->ctx, dest_pages, "Kids"));
		
	/* be aware that this function does not change the two variables if the page
	   index is greater than the number of pages */
	find_destination_pages(dest->ctx, 
		dest_pages, dest_index, &dest_pages, &dest_pages_index);

	pdf_obj *dest_kids = pdf_dict_gets(dest->ctx, dest_pages, "Kids");
	if(!pdf_is_indirect(dest->ctx, dest_pages) || 
		!pdf_is_dict(dest->ctx, dest_pages) || !pdf_is_array(dest->ctx, dest_kids))
	{
		return(ERROR_INVALID_RANGE);
	}
	
	pdf_obj *pages_root = pdf_dict_getp(src->ctx, pdf_trailer(src->ctx, src->pdf), "Root/Pages");
	if(!pdf_is_indirect(src->ctx, pages_root) || !pdf_is_dict(src->ctx, pages_root))
		return(ERROR_NO_PAGES);

	/* if we copy the root pages-node and it's referenced objects, we will copy 
	   all pages and all objects those pages need */
	pdf_obj *new_pages_ref = copy_object_single(dest->ctx, dest->pdf, src->ctx, src->pdf, pages_root);

	/* insert new pages-node */
	pdf_array_insert_drop(dest->ctx, dest_kids, new_pages_ref, dest_pages_index);

	/* update the parent */
	pdf_obj *new_pages_parent = pdf_new_indirect(dest->ctx, dest->pdf, 
		pdf_to_num(dest->ctx, dest_pages), pdf_to_gen(dest->ctx, dest_pages));
	pdf_dict_puts_drop(dest->ctx, new_pages_ref, "Parent", new_pages_parent);

	/* TODO: If dest_pages contains anything inheritable but not the new node
	         we need to insert empty items to prevent this inerhitance */

	/* update count */
	int new_count = pdf_to_int(dest->ctx, 
		pdf_dict_gets(dest->ctx, dest_pages, "Count")) + src->pagecount;
	pdf_dict_puts_drop(dest->ctx, dest_pages, "Count", 
		pdf_new_int(dest->ctx, dest->pdf, new_count));

	/* let MuPDF rebuild the page tree */
	pdf_finish_edit(dest->ctx, dest->pdf);
	dest->pdf->page_count = new_count;

	/* update juggler's state */
	juggler_page_tree_changed_due_to_insert(dest, dest_index, src->pagecount);

	return(NoError);
}
