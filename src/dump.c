/*
  dump.c - dump contents of pdf-objects
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

#include "dump.h"

#define JUGGLER_DUMP_BUFFERSIZE 0x80
#define JUGGLER_STREAM_DUMP_SIZE 0x4000

// used for indentation               0123456789013456789012345678901
static const char *INDENT_STRING = "                               ";
static const size_t INDENT_STRING_MAX = 32;

static void dump_obj(juggler_dump_t **dump, fz_context *ctx, pdf_obj *obj, const char *indent);

static juggler_dump_t *dump_new(juggler_dump_type_e type)
{
	juggler_dump_t *dump = malloc(sizeof(juggler_dump_t) + JUGGLER_DUMP_BUFFERSIZE);
	dump->type = type;
	dump->next = NULL;
	dump->used = 0;
	dump->content[0] = 0x00;

	return(dump);
}

static void dump_printf(juggler_dump_t **dump_out, const char *format, ...)
{
	juggler_dump_t *dump = *dump_out;
	va_list args;

    // all bytes until the BUFFERSIZE limit are available, minus 1 for the null-byte!
	size_t available_size =
		JUGGLER_DUMP_BUFFERSIZE - (dump->used % JUGGLER_DUMP_BUFFERSIZE) - 1;

	va_start(args, format);
	int count = vsnprintf(dump->content + dump->used, available_size, format, args);
	
	if(count < 0) {
		// we need the real count
		count = vsnprintf(NULL, 0, format, args);
		
        // count does not contain the null-byte so we have to add one
		size_t new_size = dump->used + count + 1; 
		if(new_size % JUGGLER_DUMP_BUFFERSIZE > 0)
			new_size += JUGGLER_DUMP_BUFFERSIZE - (new_size % JUGGLER_DUMP_BUFFERSIZE);
		
		dump = realloc(dump, sizeof(juggler_dump_t) + new_size); // TODO: Out of memory
		
		// now put the contents
		count = vsnprintf(dump->content + dump->used, new_size - dump->used, format, args);
	}
	
	va_end(args);

	dump->used += count;
	*dump_out = dump;
}

// Don't mix it with dump_printf (!), else we will get trouble with the null-char at the end
static void dump_bytes(juggler_dump_t **dump_out, void *memory, size_t size)
{
	juggler_dump_t *dump = *dump_out;

	size_t available_size =
		JUGGLER_DUMP_BUFFERSIZE - (dump->used % JUGGLER_DUMP_BUFFERSIZE);

	if(size + 1 > available_size) {
		size_t new_size = dump->used + size + 1; 
		if(new_size % JUGGLER_DUMP_BUFFERSIZE > 0)
			new_size += JUGGLER_DUMP_BUFFERSIZE - (new_size % JUGGLER_DUMP_BUFFERSIZE);

		dump = realloc(dump, sizeof(juggler_dump_t) + new_size); // TODO: Out of memory
	}

	memcpy(dump->content + dump->used, memory, size);
	dump->content[dump->used + size] = 0x00;
	dump->used += size;

	*dump_out = dump;
}

static void dump_dict(juggler_dump_t **dump, fz_context *ctx, pdf_obj *dict, const char *indent)
{
  int i, len = pdf_dict_len(ctx, dict);

  indent = (indent == INDENT_STRING) ? INDENT_STRING : indent - 2;

  dump_printf(dump, "<<\n");
  for(i = 0; i < len; i++) {
	  dump_printf(dump, "%s/", indent);
	  dump_obj(dump, ctx, pdf_dict_get_key(ctx, dict, i), indent);
	  dump_printf(dump, " ");
	  dump_obj(dump, ctx, pdf_dict_get_val(ctx, dict, i), indent);
	  dump_printf(dump, "\n");
  }
  dump_printf(dump, "%s>>", (indent == INDENT_STRING + INDENT_STRING_MAX) ? indent : indent + 1);
}

static void dump_array(juggler_dump_t **dump, fz_context *ctx, pdf_obj *array, const char *indent)
{
	int i, len = pdf_array_len(ctx, array);

	dump_printf(dump, "[ ");
	for(i = 0; i < len; i++) {
		dump_obj(dump, ctx, pdf_array_get(ctx, array, i), indent);
		dump_printf(dump, " ");
	}
	dump_printf(dump, "]");
}

static void dump_obj(juggler_dump_t **dump, fz_context *ctx, pdf_obj *obj, const char *indent)
{
	if(pdf_is_null(ctx, obj))
		dump_printf(dump, "[??null??]");
	// put it to the top, else MuPDF resolves indirect references and tells us 
	// what type the referenc's destination has
	else if(pdf_is_indirect(ctx, obj)) {
		juggler_dump_t *ref_dump = dump_new(JUGGLER_DUMP_TYPE_REF);
		ref_dump->next = *dump; // use it temporary to store a pointer to the last dump
		dump_printf(&ref_dump, "%d %d R", pdf_to_num(ctx, obj), pdf_to_gen(ctx, obj));

		juggler_dump_t *new_dump = dump_new(JUGGLER_DUMP_TYPE_NO_FORMAT);
		new_dump->next = ref_dump; // use it temporary to store a pointer to the last dump
		*dump = new_dump;
	} else if(pdf_is_bool(ctx, obj))
		dump_printf(dump, pdf_to_bool(ctx, obj) ? "true" : "false");
	else if(pdf_is_int(ctx, obj))
		dump_printf(dump, "%d", pdf_to_int(ctx, obj));
	else if(pdf_is_real(ctx, obj))
		dump_printf(dump, "%f", pdf_to_real(ctx, obj));
	else if(pdf_is_name(ctx, obj))
		dump_printf(dump, "%s", pdf_to_name(ctx, obj));
	else if(pdf_is_string(ctx, obj))
		dump_printf(dump, "%s", pdf_to_str_buf(ctx, obj));
	else if(pdf_is_array(ctx, obj))
		dump_array(dump, ctx, obj, indent);
	else if(pdf_is_dict(ctx, obj))
		dump_dict(dump, ctx, obj, indent);
}

static void dump_stream(juggler_dump_t **dump_out, fz_context *ctx, pdf_document *doc, int num, int gen)
{
	juggler_dump_t *dump = dump_new(JUGGLER_DUMP_TYPE_STREAM);
	dump->next = *dump_out;

	fz_stream *stream = pdf_open_stream(ctx, doc, num, gen);

	unsigned char buffer[JUGGLER_STREAM_DUMP_SIZE];

	size_t read_bytes;
	/*TODO: while*/if((read_bytes = fz_read(ctx, stream, buffer, JUGGLER_STREAM_DUMP_SIZE)) > 0)
		dump_bytes(&dump, (void *) buffer, read_bytes);

	fz_drop_stream(ctx, stream);

	*dump_out = dump;
}

ErrorCode juggler_dump_object(juggler_t *juggler,
	int num, int gen, juggler_dump_t **dump) 
{
	pdf_obj *obj;

	fz_try(juggler->ctx) {
		obj = pdf_load_object(juggler->ctx, juggler->pdf, num, gen);
	} fz_catch(juggler->ctx) {
		*dump = NULL;
		return(ErrorEntryNoObject);
	}

	*dump = dump_new(JUGGLER_DUMP_TYPE_NO_FORMAT);

	dump_obj(dump, juggler->ctx, obj, INDENT_STRING + INDENT_STRING_MAX);

	if(pdf_is_stream(juggler->ctx, juggler->pdf, num, gen)) {
		dump_stream(dump, juggler->ctx, juggler->pdf, num, gen);
	}

	juggler_dump_t *current = *dump; // this points to the last one!
	juggler_dump_t *next = NULL;
    // because next is used as pref during the construction this loop 
    //iterates from the last to the first
	while(current != NULL) {
		juggler_dump_t *prev = current->next; // it's bad that prev and next have to be exchanged
		current->next = next;
		
		next = current;
		current = prev;
	}

	*dump = next;

	return(NoError);
}

ErrorCode juggler_dump_delete(juggler_dump_t *dump)
{
	juggler_dump_t *current = dump;

	while(current != NULL) {
		juggler_dump_t *next = current->next;
		free(current);

		current = next;
	}

	return(NoError);
}
