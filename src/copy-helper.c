/*
  copy-helper.c - copy pdf-objects from file to file
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

#include "copy-helper.h"

typedef struct {
	int *vals;
	size_t len;
	size_t cap;
} objref_stack;

static inline objref_stack *objref_stack_new(int init_cap)
{
	objref_stack *stack = malloc(sizeof(objref_stack));
	stack->len = 0;
	stack->cap = init_cap * 2;
	stack->vals = malloc(stack->cap * sizeof(int));

	return(stack);
}

static inline objref_stack *objref_stack_clone(objref_stack *src)
{
	objref_stack *stack = malloc(sizeof(objref_stack));
	stack->len = src->len;
	stack->cap = src->cap;
	stack->vals = malloc(src->cap * sizeof(int));
	memcpy(stack->vals, src->vals, src->len * sizeof(int));

	return(stack);
}
	

static inline void objref_stack_get(objref_stack *stack, size_t index, 
	int *num, int *gen)
{
	index *= 2;
	*num = stack->vals[index];
	*gen = stack->vals[index + 1];
}

static inline int objref_stack_get_num(objref_stack *stack, size_t index)
{
	return(stack->vals[index * 2]);
}

static inline void objref_stack_push(objref_stack *stack, int num, int gen)
{
	if(stack->cap == 0) {
		stack->vals = realloc(stack->vals, stack->len * sizeof(int) * 2);
		stack->cap = stack->len;//*= 2;
	}
	stack->vals[stack->len++] = num;
	stack->vals[stack->len++] = gen;
	stack->cap -= 2;
}

static inline void objref_stack_pop(objref_stack *stack, int *num, int *gen)
{
	stack->cap += 2;
	stack->len -= 2;
	*num = stack->vals[stack->len];
	*gen = stack->vals[stack->len + 1];
}

static inline void objref_stack_delete(objref_stack *stack)
{
	free(stack->vals);
	free(stack);
}

static inline size_t objref_stack_len(objref_stack *stack)
{
	return(stack->len / 2);
}

typedef struct {
	fz_context *src_ctx;
	pdf_document *src;
	fz_context *dest_ctx;
	pdf_document *dest;
	objref_stack *to_do;
	int *new_ids;
} copy_info;

static pdf_obj *copy_obj(pdf_obj *src, copy_info *copy);

static pdf_obj *copy_dict(pdf_obj *src, copy_info *copy)
{
	int len = pdf_dict_len(copy->src_ctx, src);

	pdf_obj *dest = pdf_new_dict(copy->dest_ctx, copy->dest, len);
	
	int i;
	for(i = 0; i < len; i++) {
		pdf_obj *new_key = 
			copy_obj(pdf_dict_get_key(copy->src_ctx, src, i), copy);
		pdf_obj *new_value = 
			copy_obj(pdf_dict_get_val(copy->src_ctx, src, i), copy);
		pdf_dict_put(copy->dest_ctx, dest, new_key, new_value);
		pdf_drop_obj(copy->dest_ctx, new_key);
		pdf_drop_obj(copy->dest_ctx, new_value);
	}
	return(dest);
}

static pdf_obj *copy_array(pdf_obj *src, copy_info *copy)
{
	int len = pdf_array_len(copy->src_ctx, src);

	pdf_obj *dest = pdf_new_array(copy->dest_ctx, copy->dest, len);
	
	int i;
	for(i = 0; i < len; i++) {
		pdf_obj *new_obj = copy_obj(pdf_array_get(copy->src_ctx, src, i), copy);
		pdf_array_push_drop(copy->dest_ctx, dest, new_obj);
	}
	return(dest);
}

static pdf_obj *copy_obj(pdf_obj *src, copy_info *copy)
{
	if(pdf_is_null(copy->src_ctx, src)) {
		return(pdf_new_null(copy->dest_ctx, copy->dest));

	/* check for indirects first, else MuPDF resolves them and tells us the
	   type of the reference's destination */
	} else if(pdf_is_indirect(copy->src_ctx, src)) {
		int src_num = pdf_to_num(copy->src_ctx, src);
		int src_gen = pdf_to_gen(copy->src_ctx, src); // TODO: validate gen?
		if(src_num < 1 || src_num > pdf_xref_len(copy->src_ctx, copy->src))
			return(pdf_new_null(copy->dest_ctx, copy->dest));

		int new_id = copy->new_ids[src_num];
		if(new_id == 0) {
			new_id = pdf_create_object(copy->dest_ctx, copy->dest);
			copy->new_ids[src_num] = new_id;
			objref_stack_push(copy->to_do, src_num, src_gen);
		}
		return(pdf_new_indirect(copy->dest_ctx, copy->dest, new_id, 0));
	} else if(pdf_is_bool(copy->src_ctx, src)) {
		return(pdf_new_bool(copy->dest_ctx, copy->dest, 
			pdf_to_bool(copy->src_ctx, src)));
	} else if(pdf_is_int(copy->src_ctx, src)) {
		return(pdf_new_int(copy->dest_ctx, copy->dest, 
			pdf_to_int(copy->src_ctx, src)));
	} else if(pdf_is_real(copy->src_ctx, src)) {
		return(pdf_new_real(copy->dest_ctx, copy->dest, 
			pdf_to_real(copy->src_ctx, src)));
	} else if(pdf_is_name(copy->src_ctx, src)) {
		return(pdf_new_name(copy->dest_ctx, copy->dest, 
			pdf_to_name(copy->src_ctx, src)));
	} else if(pdf_is_string(copy->src_ctx, src)) {
		return(pdf_new_string(copy->dest_ctx, copy->dest, 
			pdf_to_str_buf(copy->src_ctx, src), 
			pdf_to_str_len(copy->src_ctx, src)));
	} else if(pdf_is_array(copy->src_ctx, src)) {
		return(copy_array(src, copy));
	} else if(pdf_is_dict(copy->src_ctx, src)) {
		return(copy_dict(src, copy));
	}

	return(NULL);
	
}

static void copy_stream(fz_context *src_ctx, pdf_document *src_doc, 
	pdf_obj *src_obj, int src_num, int src_gen, fz_context *dest_ctx, 
	pdf_document *dest_doc, pdf_obj *dest_obj, int dest_num)
{
	int length = 
		pdf_to_int(dest_ctx, pdf_dict_gets(src_ctx, src_obj, "Length"));
	if(length == 0) // TODO: Error case
		return;

    /* we have to allocate a lot of memory here, but there is no other way in
	   mupdf's api */
	unsigned char *dest_data = fz_malloc(dest_ctx, length);

	fz_try(dest_ctx) {
		fz_stream *src_stream = 
			pdf_open_raw_stream(src_ctx, src_doc, src_num, src_gen);

        /* read all to the data part of the new buffer */
		length = fz_read(src_ctx, src_stream, dest_data, length);

		fz_drop_stream(src_ctx, src_stream);
	} fz_catch(dest_ctx) {
		fz_free(dest_ctx, dest_data);
		return;
	}

	fz_buffer *dest_buffer = 
		fz_new_buffer_from_data(dest_ctx, dest_data, length);
	// TODO: Currently its okay, to tell everything is compressed as MuPDF only
	// removes Filter- and DecodeParams-entries if this is 0, but nothing more
	// if compression is used... But will it change?
	pdf_update_stream(dest_ctx, dest_doc, dest_obj, dest_buffer, 1); 
	fz_drop_buffer(dest_ctx, dest_buffer);
}

pdf_obj *copy_object_continue(fz_context *dest_ctx, pdf_document *dest, 
	fz_context *src_ctx, pdf_document *src, pdf_obj *src_obj, int **new_ids_ptr)
{
    /* we need a map from source object numbers to destination object numbers
	   this is done easily by an array where new_ids[old_ids] is the new number
	   (or NULL when it was not yet copied and needs a new number */
	int *new_ids = *new_ids_ptr;
	if(new_ids == NULL)
		*new_ids_ptr = new_ids = calloc(pdf_xref_len(src_ctx, src), sizeof(int));

	/* this function can be called several times... thus the user could call
	   it with a src_obj that already  has been copied! We don't want to 
	   copy it a second time */
	if(new_ids[pdf_to_num(src_ctx, src_obj)] != 0) {
		//printf("Object src: %d 0 already was copied to %d 0\n", 
		//	pdf_to_num(src_ctx, src_obj), new_ids[pdf_to_num(src_ctx, src_obj)]);
		return(pdf_new_indirect(src_ctx, dest, new_ids[pdf_to_num(src_ctx, src_obj)], 0));
	}

	/* all object's that are referenced but not already copied are be put here */
	objref_stack *to_do = objref_stack_new(256);

	/* create a new obj for src_obj in the destination-file and add it to to-do */
	int dest_obj_num = pdf_create_object(dest_ctx, dest);
	new_ids[pdf_to_num(src_ctx, src_obj)] = dest_obj_num;
	objref_stack_push(to_do, 
		pdf_to_num(src_ctx, src_obj), pdf_to_gen(src_ctx, src_obj));
	
	copy_info info;
	info.src_ctx = src_ctx;
	info.src = src;
	info.dest_ctx = dest_ctx;
	info.dest = dest;
	info.to_do = to_do;
	info.new_ids = new_ids;

	while(objref_stack_len(to_do) > 0) {
		int src_num, src_gen;
		objref_stack_pop(to_do, &src_num, &src_gen);
		
		pdf_obj *src_obj = pdf_load_object(src_ctx, src, src_num, src_gen);

		pdf_obj *dest_obj = copy_obj(src_obj, &info);
		pdf_update_object(dest_ctx, dest, new_ids[src_num], dest_obj);
		pdf_drop_obj(dest_ctx, dest_obj);

		if(pdf_is_stream(src_ctx, src, src_num, src_gen))
			copy_stream(src_ctx, src, src_obj, src_num, src_gen, 
				dest_ctx, dest, dest_obj, new_ids[src_num]);
	}
	objref_stack_delete(to_do);

	return(pdf_new_indirect(dest_ctx, dest, dest_obj_num, 0));
}

pdf_obj *copy_object_single(fz_context *dest_ctx, pdf_document *dest, 
	fz_context *src_ctx, pdf_document *src, pdf_obj *src_obj)
{
	int *new_ids = NULL;
	pdf_obj *obj = 
		copy_object_continue(dest_ctx, dest, src_ctx, src, src_obj, &new_ids);

	free(new_ids);
	return(obj);
}

pdf_obj *copy_unassigned_object_continue(fz_context *dest_ctx, pdf_document *dest, 
	fz_context *src_ctx, pdf_document *src, pdf_obj *src_obj, int **new_ids_ptr)
{
    /* we need a map from source object numbers to destination object numbers
	   this is done easily by an array where new_ids[old_ids] is the new number
	   (or NULL when it was not yet copied and needs a new number */
	int *new_ids = *new_ids_ptr;
	if(new_ids == NULL)
		*new_ids_ptr = new_ids = calloc(pdf_xref_len(src_ctx, src), sizeof(int));

	/* all object's that are referenced but not already copied are be put here */
	objref_stack *to_do = objref_stack_new(256);

	copy_info info;
	info.src_ctx = src_ctx;
	info.src = src;
	info.dest_ctx = dest_ctx;
	info.dest = dest;
	info.to_do = to_do;
	info.new_ids = new_ids;
	
	pdf_obj *copied_obj = copy_obj(src_obj, &info);

	while(objref_stack_len(to_do) > 0) {
		int src_num, src_gen;
		objref_stack_pop(to_do, &src_num, &src_gen);
		
		pdf_obj *src_obj = pdf_load_object(src_ctx, src, src_num, src_gen);

		pdf_obj *dest_obj = copy_obj(src_obj, &info);
		pdf_update_object(dest_ctx, dest, new_ids[src_num], dest_obj);
		pdf_drop_obj(dest_ctx, dest_obj);

		if(pdf_is_stream(src_ctx, src, src_num, src_gen))
			copy_stream(src_ctx, src, src_obj, src_num, src_gen, 
				dest_ctx, dest, dest_obj, new_ids[src_num]);
	}

	objref_stack_delete(to_do);
	return(copied_obj);
}