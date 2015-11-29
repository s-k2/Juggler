/*
  document.c - do basic juggler-operations
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

#include "document.h"

#include "internal.h"

ErrorCode juggler_init(fz_context **init_data)
{
	fz_context *ctx;

	if((ctx = fz_new_context(NULL, NULL, FZ_STORE_UNLIMITED)) == NULL) {
	  fprintf(stderr, "fz_new_context() failed\n");
	  return(ErrorNewContext);
	}

	*init_data = ctx;
	return(NoError);
}

ErrorCode juggler_open(fz_context *ctx, char *filename, juggler_t **juggler)
{
	pdf_document *doc = pdf_open_document(ctx, filename);

	if(pdf_needs_password(ctx, doc)) {
		fprintf(stderr, "pdf_needs_password(): Cannot handle password-protected files\n");
		return(ErrorPasswordProtected);
	}

	*juggler = (juggler_t *) malloc(sizeof(juggler_t));
	(*juggler)->pdf = doc;
	(*juggler)->ctx = ctx;
	juggler_page_tree_changed(*juggler);

	return(NoError);
}

ErrorCode juggler_close(juggler_t *juggler)
{
	// TODO: Move to render.c
	free(juggler->sizes);
	size_t i;
	for(i = 0; i < RENDER_BUFFER_COUNT; i++) {
		if(juggler->rendered[i].samples != NULL) {
			printf("Attempting to drop pixmap #%d: %p\n", i, juggler->rendered[i].pixmap);
			fz_drop_pixmap(juggler->ctx, juggler->rendered[i].pixmap);
		}
	}

	pdf_close_document(juggler->ctx, juggler->pdf);
	free(juggler);

	return(NoError);
}


ErrorCode juggler_save(juggler_t *juggler, char *filename)
{
	fz_write_options write_options;
	write_options.do_incremental = 0;
	write_options.do_ascii = 0;
	write_options.do_expand = 0;
	write_options.do_garbage = 1;
	write_options.do_linear = 0;
	write_options.do_clean = 0;
	write_options.continue_on_error = 0;
	
	pdf_write_document(juggler->ctx, juggler->pdf, filename, &write_options);

	return(NoError);
}
