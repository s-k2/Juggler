/*
  internal.h - functions that aren't exported to the public API of Juggler
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

#ifndef _JUGGLER_INTERNAL_H_
#define _JUGGLER_INTERNAL_H_

#include "document.h"

extern pdf_obj *juggler_lookup_inherited_page_item(fz_context *ctx, pdf_document *doc, pdf_obj *node, const char *key);

/* this updates all juggler-structures after changes hat been made in the 
   page-tree using only the MuPDF-API */
extern void juggler_page_tree_changed(juggler_t *juggler);

extern void juggler_page_tree_changed_due_to_remove(juggler_t *juggler, 
	int delete_index, int delete_count);

extern void juggler_page_tree_changed_due_to_insert(juggler_t *juggler, 
	int index, int count);

extern void juggler_page_changed(juggler_t *juggler, int page_index);

#endif /* _JUGGLER_INTERNAL_H_ */
