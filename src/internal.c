/*
  internal.c - functions that aren't exported to the public API of Juggler
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

#include "internal.h"

/* copied from MuPDF */
pdf_obj *juggler_lookup_inherited_page_item(fz_context *ctx, pdf_document *doc, pdf_obj *node, const char *key)
{
	pdf_obj *node2 = node;
	pdf_obj *val;

	/* fz_var(node); Not required as node passed in */

	fz_try(ctx)
	{
		do
		{
			val = pdf_dict_gets(ctx, node, key);
			if (val)
				break;
			if (pdf_mark_obj(ctx, node))
				fz_throw(ctx, FZ_ERROR_GENERIC, "cycle in page tree (parents)");
			node = pdf_dict_get(ctx, node, PDF_NAME_Parent);
		}
		while (node);
	}
	fz_always(ctx)
	{
		do
		{
			pdf_unmark_obj(ctx, node2);
			if (node2 == node)
				break;
			node2 = pdf_dict_get(ctx, node2, PDF_NAME_Parent);
		}
		while (node2);
	}
	fz_catch(ctx)
	{
		fz_rethrow(ctx);
	}

	return val;
}
/* end of copied function */


void juggler_page_tree_changed(juggler_t *juggler)
{
	juggler->pagecount = pdf_count_pages(juggler->ctx, juggler->pdf);
	size_t i;
	for(i = 0; i < RENDER_BUFFER_COUNT; i++)
		juggler->rendered[i].samples = NULL;
	juggler->recent_rendered = 0;
	juggler->sizes = NULL;
	get_all_pages_size(juggler);
}

void juggler_page_tree_changed_due_to_remove(juggler_t *juggler, int delete_index, int delete_count)
{
	// update juggler information
	juggler->pagecount--;
	size_t i;
	for(i = 0; i < RENDER_BUFFER_COUNT; i++) {
		if(juggler->rendered[i].samples != NULL) {
			if(juggler->rendered[i].pagenum >= delete_index + delete_count)
				juggler->rendered[i].pagenum -= delete_count;
			// this page does no longer exist
			else if(juggler->rendered[i].pagenum == delete_index) 
				juggler->rendered[i].pagenum = -1; // will be delete later
		}
	}
	juggler->sizes = NULL;
	get_all_pages_size(juggler);
}

void juggler_page_tree_changed_due_to_insert(juggler_t *juggler, int index, int count)
{
	juggler_page_tree_changed(juggler);
}

void juggler_page_changed(juggler_t *juggler, int page_index)
{
  size_t i;
	for(i = 0; i < RENDER_BUFFER_COUNT; i++)
		if(juggler->rendered[i].pagenum == page_index)
			juggler->rendered[i].pagenum = -1;
}
