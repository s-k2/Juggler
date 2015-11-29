/*
  copy-helper.h - copies pdf objects from one file to another
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

#ifndef _COPY_HELPER_H_
#define _COPY_HELPER_H_

#include "document.h"

/* TODO: All in here needs to be named better! 
   And maybe we should create an extra helper-directory? */

/* Returns an indirect pdf_obj * pointing to the copy of src_obj. You are 
   responsible to drop this new item! 
   Call this function if you just need to do one copy-operation from src to dest. */
extern pdf_obj *copy_object_single(fz_context *dest_ctx, pdf_document *dest, 
	fz_context *src_ctx, pdf_document *src, pdf_obj *src_obj);

/* ...And this if you need to copy more than once from src to dest. It will 
   ensure that referenced objects that had already been copied in a previous 
   call to this function won't be copied again!
   But you are responsible to free the int-array new_ids_ptr! And you have to 
   assign *new_ids_ptr with NULL for the first call! 

   Returns an indirect pdf_obj * pointing to the copy of src_obj. You are 
   responsible to drop this new item! */
   
extern pdf_obj *copy_object_continue(fz_context *dest_ctx, pdf_document *dest, 
	fz_context *src_ctx, pdf_document *src, pdf_obj *src_obj, int **new_ids_ptr);
	
/* returns the copied object that does not need to be assigned to an entry in
   the xref-info...
   Thus only parts of one assigned object could be copied into another document,
   but if there are any referenced objects those would be copied (and assigned
   a new xref-number in the destination file */
extern pdf_obj *copy_unassigned_object_continue(fz_context *dest_ctx, pdf_document *dest, 
	fz_context *src_ctx, pdf_document *src, pdf_obj *src_obj, int **new_ids_ptr);

#endif /* _COPY_HELPER_H_ */
