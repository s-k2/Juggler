/*
  put-content.c - put the contents of one page onto another  Copyright (C) 2015 Stefan Klein

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

#include "put-content.h"

#include "copy-helper.h"
#include "internal.h"
#include "rename-lexer.h"

struct put_info
{
	pdf_document *dest_doc;
	pdf_document *src_doc;
	pdf_obj *rename_dict;
	int *new_ids;
	int next_inline_id;
};

// just copy one of the resource sub-entries (e.g. /Font)
static int copy_and_rename_resource(fz_context *dest_ctx, pdf_obj *dest, 
	fz_context *src_ctx, pdf_obj *src, char *prefix, struct put_info *info)
{
	char new_name[64]; /* this buffer is big enough up to hold all digits for two 16-bit numbers */

    int i;
    for(i = 0; i < pdf_dict_len(src_ctx, src); i++) {
        pdf_obj *src_key = pdf_dict_get_key(src_ctx, src, i);
		pdf_obj *src_val = pdf_dict_get_val(src_ctx, src, i);

		if(!pdf_is_name(src_ctx, src_key)) {
			return(2);
		}
		
		/* if this is an inline resource, just copy this object into the new
		    resource dict */	
		if(!pdf_is_indirect(src_ctx, src_val)) {
			if(snprintf(new_name, sizeof(new_name) / sizeof(new_name[0]), 
					"%sinline_%d", prefix, info->next_inline_id++) >= sizeof(new_name) / sizeof(new_name[0]))
				return(1); // not enough space
		
			pdf_obj *new_res = copy_unassigned_object_continue(dest_ctx, 
				info->dest_doc, src_ctx, info->src_doc, src_val, &info->new_ids);
				
			//pdf_obj *new_res = pdf_new_dict(dest_ctx, info->dest_doc, 10);
			printf("dump it...\n");
			pdf_fprint_obj(dest_ctx, stdout, new_res, 0);
				
			/* now reference this new object in the resource object of this sheet */
			pdf_obj *dest_key = pdf_new_name(dest_ctx, info->dest_doc, new_name);

			pdf_dict_put(dest_ctx, dest, dest_key, new_res);
			pdf_drop_obj(dest_ctx, dest_key);
			pdf_drop_obj(dest_ctx, new_res);
		} else {
			/* The new name of resource objects is always the num/gen of the 
			   referenced object in the src-file. Thus we can check by that name
			   if the object was already referenced by another page of this sheet. */
			if(snprintf(new_name, sizeof(new_name) / sizeof(new_name[0]), 
					"%s%d_%d", prefix, pdf_to_num(dest_ctx, src_val), pdf_to_gen(dest_ctx, src_val)) >= 
					sizeof(new_name) / sizeof(new_name[0]))
				return(1); // not enough space
						
			if(pdf_dict_gets(dest_ctx, dest, new_name) == NULL) {
			   /* if this resource is not inlined and not already in the resource-dict 
				  of the current sheet... */
			
				/* ...copy the referenced resource to the new document!
				   If this object has copied already (for another sheet in dest_doc),
				   copy_object_continue() will do nothing */
				pdf_obj *new_res = copy_object_continue(dest_ctx, info->dest_doc, 
					src_ctx, info->src_doc, src_val, &info->new_ids);

				/* now reference this new object in the resource object of this sheet */
				pdf_obj *dest_key = pdf_new_name(dest_ctx, info->dest_doc, new_name);

				pdf_dict_put(dest_ctx, dest, dest_key, new_res);
				pdf_drop_obj(dest_ctx, dest_key);
				pdf_drop_obj(dest_ctx, new_res);	
			}
		}

		/* even if it was used on another sheet or on this sheet, add it to the 
		   rename-dict for this sheet! Because it could have different names
		   on different source-pages */
		pdf_obj *rename_key = pdf_new_name(dest_ctx, info->dest_doc, pdf_to_name(dest_ctx, src_key));
		pdf_obj *rename_val = pdf_new_name(dest_ctx, info->dest_doc, new_name);
		pdf_dict_put(dest_ctx, info->rename_dict, rename_key, rename_val);
		pdf_drop_obj(dest_ctx, rename_key);
		pdf_drop_obj(dest_ctx, rename_val);
	}

	return(0);
}

static int copy_and_rename_resources(fz_context *dest_ctx, pdf_obj *dest, 
	fz_context *src_ctx, pdf_obj *src, struct put_info *info)
{
    static char *RESOURCE_TYPES[] = 
		{ "ExtGState", "ColorSpace", "Pattern", "Shading", "XObject", 
		  "Font", "Properties" };
	static char *RESOURCE_PREFIXES[] =
		{ "E_", "C_", "P_", "S_", "X_", "F_", "T_" };
    static const size_t RESOURCE_TYPES_COUNT = 
		sizeof(RESOURCE_TYPES) / sizeof(RESOURCE_TYPES[0]);

    size_t i;
    for(i = 0; i < RESOURCE_TYPES_COUNT; i++) {
        pdf_obj *src_type = pdf_dict_gets(src_ctx, src, RESOURCE_TYPES[i]);
        pdf_obj *dest_type = pdf_dict_gets(dest_ctx, dest, RESOURCE_TYPES[i]);

		/* we only copy resource-dicts that exist in the source-page ;) */
		if(pdf_is_dict(src_ctx, src_type)) {printf("TODO: REMOVEME: Copying and renaming resources of type %s!!!\n", RESOURCE_TYPES[i]);
			/* if this kind of dict does not exists in the dest resources, 
			   we must create it */
			if(!pdf_is_dict(dest_ctx, dest_type)) {
				dest_type = pdf_new_dict(dest_ctx, info->dest_doc, 8);
				pdf_dict_puts_drop(dest_ctx, dest, RESOURCE_TYPES[i], dest_type);
			}

			if(copy_and_rename_resource(dest_ctx, dest_type, 
				src_ctx, src_type, RESOURCE_PREFIXES[i], info))
			{
				return(2);
			}
		}
	}

    // TODO: Merge Procedure-Sets (although they are obsolete)

	return(0);
}

struct pos_info
{
	int rotate;
	double x, y;
	double width, height; /* in page units of the sheet */
	double scale;

	double content_translate_x, content_translate_y;

	/* the outer clipping area prevents content to be shown outside the 
	   boudaries for this page on this sheet */
	double outer_clip_x, outer_clip_y;
	double outer_clip_width, outer_clip_height;

	/* the transformation-matrix for the content */
	double a, b, c, d, e, f;

	/* the inner bleed clip prevents content to be shown that is outside
	   the page's bleed-box - be aware that those coordinates are page-oriented
	   and this clipping needs to be applied after the transformation */
	double bleed_clip_x, bleed_clip_y;
	double bleed_clip_width, bleed_clip_height;


	/* page-number of the page in the source-file */
	int src_pagenum;
	/* page-number of the sheet in the destiantion page */
	int sheet_pagenum;
};


/* dest points to the new pages content-streams-dict, src is a reference to 
   one source content-stream */
int copy_content_stream_of_page(fz_context *dest_ctx, pdf_obj *dest, 
	fz_context *src_ctx, pdf_obj *src, 
	struct put_info *info, struct pos_info *pos)
{
	if(!pdf_is_array(dest_ctx, dest) && !pdf_is_indirect(src_ctx, src))
		return(-1);

	/*
	  translation:  1     0    0    1     diff_x diff_y
	  scale:        scale 0    0    scale 0      0
	  rotation:     cos   sin  -sin cos   0      0
	  -------------------------------------------------
	  rotation 0:   1     0    0    1     0      0
	  rotation 90:  0     1    -1   0     0      0
	  rotation 180: -1    0    0    -1    0      0
	  rotation 270: 0     -1   1    0     0      0 
	*/

	fz_buffer *buffer = fz_new_buffer(dest_ctx, 1024);
	fz_output *output = fz_new_output_with_buffer(dest_ctx, buffer);

	fz_printf(dest_ctx, output, "q\n");

	/* set the outer clip region */
	fz_printf(dest_ctx, output, "%f %f %f %f re W n\n", 
		pos->outer_clip_x, pos->outer_clip_y, 
		pos->outer_clip_width, pos->outer_clip_height);

	/* position the page correctly */
	if(pos->rotate == 0) {
		fz_printf(dest_ctx, output, "1 0 0 1 %f %f cm\n", 
			pos->x + pos->content_translate_x, pos->y + pos->content_translate_y);
	} else if(pos->rotate == 90) {
		fz_printf(dest_ctx, output, "0 1 -1 0 %f %f cm\n", pos->x + pos->width, pos->y);
	} else if(pos->rotate == 180) {
		fz_printf(dest_ctx, output, "-1 0 0 -1 %f %f cm\n", 
			pos->width + pos->x - pos->content_translate_x, 
			pos->height + pos->y - pos->content_translate_y);
	} else if(pos->rotate == 270) {
		fz_printf(dest_ctx, output, "0 -1 1 0 %f %f cm\n", pos->x, pos->y + pos->height);
	}
	
	if(pos->bleed_clip_x != 0.0 || pos->bleed_clip_y != 0.0 || 
		pos->bleed_clip_width != 0.0 || pos->bleed_clip_height != 0.0)
	{
		fz_printf(dest_ctx, output, "%f %f %f %f re W n\n", 
			pos->bleed_clip_x, pos->bleed_clip_y, pos->bleed_clip_width, pos->bleed_clip_height);
	}

 	int src_num = pdf_to_num(src_ctx, src);
	int src_gen = pdf_to_gen(src_ctx, src);
	fz_stream *input = pdf_open_stream(src_ctx, info->src_doc, src_num, src_gen);

	rename_res_in_content_stream(src_ctx, input, dest_ctx, output, info->rename_dict);

	fz_printf(dest_ctx, output, "Q");

	fz_drop_output(dest_ctx, output);
	fz_drop_stream(dest_ctx, input);
	
	int new_num = pdf_create_object(dest_ctx, info->dest_doc);
	pdf_obj *new_ref = pdf_new_indirect(dest_ctx, info->dest_doc, new_num, 0);

	/* each stream has a dict containing at least its length... */
	pdf_obj *stream_info_dict = pdf_new_dict(dest_ctx, info->dest_doc, 1);
	pdf_dict_puts_drop(dest_ctx, stream_info_dict, "Length", pdf_new_int(dest_ctx, info->dest_doc, buffer->len));
	pdf_update_object(dest_ctx, info->dest_doc, new_num, stream_info_dict);
	pdf_drop_obj(dest_ctx, stream_info_dict);

	pdf_update_stream(dest_ctx, info->dest_doc, new_ref, buffer, 0);
	fz_drop_buffer(dest_ctx, buffer);

	pdf_array_push(dest_ctx, dest, new_ref);
	pdf_drop_obj(dest_ctx, new_ref);

	return(0);
}

int adjust_bleed_clipping(fz_context *ctx, pdf_document *doc, pdf_page *page, struct pos_info *pos)
{
	/* We clip each page two times... The first clipping rectangle just 
	   prevents the page from drawing outside of its area on the sheet.
	   The second clipping is done here and clips out all those things that
	   are located outside the bleed-box (e. g. printer's marks). */
	pdf_obj *bleed_box = 
		juggler_lookup_inherited_page_item(ctx, doc, page->me, "BleedBox");

	/* if no bleeding-box exists, there is no need for the second clipping */
	if(bleed_box == NULL) {
		pos->bleed_clip_x = pos->bleed_clip_y = 
			pos->bleed_clip_width = pos->bleed_clip_height = 0.0;
		return(0);
	}

	if(!pdf_is_array(ctx, bleed_box) || pdf_array_len(ctx, bleed_box) != 4)
		return(-1);

	fz_rect bleed_rect;
	pdf_to_rect(ctx, bleed_box, &bleed_rect);

	pos->bleed_clip_x = bleed_rect.x0;
	pos->bleed_clip_y = bleed_rect.y0;
	pos->bleed_clip_width = bleed_rect.x1 - bleed_rect.x0;
	pos->bleed_clip_height = bleed_rect.y1 - bleed_rect.y0;

	return(0);
}

int adjust_page_position(fz_context *ctx, pdf_document *doc, pdf_page *page, struct pos_info *pos)
{
	/* if page is rotated we must add this to the desired rotation of the page */
	pos->rotate = (pos->rotate + page->rotate) % 360;
	fz_matrix rotation_mtx; /* tranformation of the user-space due to the rotation */
	fz_rotate(&rotation_mtx, page->rotate);

	/* get the media-box (with rotation applied) */
	pdf_obj *media_box = juggler_lookup_inherited_page_item(ctx, doc, page->me, "MediaBox");
	if(!pdf_is_array(ctx, media_box) || pdf_array_len(ctx, media_box) != 4) 
		return(-1); /* the specification forces a valid media-box... */

	fz_rect media_rect;
	pdf_to_rect(ctx, media_box, &media_rect);
	fz_transform_rect(&media_rect, &rotation_mtx);

	/* get trim-box */
	pdf_obj *trim_box = juggler_lookup_inherited_page_item(ctx, doc, page->me, "TrimBox");
	if(trim_box == NULL)
		trim_box = media_box;
	if(!pdf_is_array(ctx, trim_box) || pdf_array_len(ctx, trim_box) != 4)
		return(-2);

	fz_rect trim_rect;
	pdf_to_rect(ctx, trim_box, &trim_rect);
	fz_transform_rect(&trim_rect, &rotation_mtx);
	// TODO: Take scale into account

	double available_width = pos->width;
	double available_height = pos->height;

	double page_width = trim_rect.x1 - trim_rect.x0;
	double page_height = trim_rect.y1 - trim_rect.y0;

	/* position the page in the middle of the destination area */
	pos->content_translate_x = 
		media_rect.x0 - trim_rect.x0 + (available_width - page_width) / 2;
	pos->content_translate_y = 
		media_rect.y0 - trim_rect.y0 + (available_height - page_height) / 2;

	
	/* if needed, clip the contents to the bleed-box */
	if(adjust_bleed_clipping(ctx, doc, page, pos) < 0)
		return(-2);

	return(0);
}

int copy_content_streams_of_page(fz_context *dest_ctx, pdf_page *dest, 
	fz_context *src_ctx, pdf_page *src, 
	struct put_info *info, struct pos_info *pos)
{
	if(pdf_is_array(src_ctx, src->contents)) {
    int i;
		for(i = 0; i < pdf_array_len(src_ctx, src->contents); i++)
			copy_content_stream_of_page(dest_ctx, dest->contents, src_ctx, 
				pdf_array_get(src_ctx, src->contents, i), info, pos);
	} else {
		copy_content_stream_of_page(dest_ctx, dest->contents, 
			src_ctx, src->contents, info, pos);
	}

	return(0);
}

/*
  put_pages_on_new_sheet() - put the contents of pages onto one page in dest_doc

  How does it work?
  1.  Create a page for the new sheet that will contain all source-pages
  2.  Analyze their sizes! If too small, just center. If too big, warn
  3.  Analye their userunit-value... if one of them != 1 we have to insert 
      scale-operations to all others and need to use userunit in the sheet too
  4.  Copy each entry of the resource-dicts of every page of the sheet 
      (if it has not already been copied for a previous sheet)
  5.  Create a dict that contains src-resource-name to dest-resource-name for 
      this sheet (this is an n:m relation where n >= m)
  6.  For each source-page-stream create a new content-stream and change
      scale (if user-unit is needed), translation and rotation... And add a 
      clipping-path around the page
  7.  Copy all source content-streams of the current page to this new content-
      stream renaming all its resources to the new names
  8.  TODO: Annots?
  9.  TODO: Preseparated pages
  10. TODO: Merge procedure sets 
*/
static int put_pages_on_new_sheet(fz_context *dest_ctx, pdf_document *dest_doc, 
	fz_context *src_ctx, pdf_document *src_doc, 
	struct pos_info *positions, size_t put_count)
{
	/* copy each entry of the resource dict */
	static const int RENAME_INITIAL_CAP = 128;
	struct put_info put_info = { dest_doc, src_doc, NULL, NULL, 0 };
	
	/* what destianation page is currently opened? */
	int sheet_pagenum = -1;
	pdf_page *sheet = NULL;
	
	size_t i;
	for(i = 0; i < put_count; i++) {
		/* if the current sheet changes, we need to close the current and open 
		   the new one*/
		if(sheet_pagenum != positions[i].sheet_pagenum) {
			if(sheet != NULL)
				pdf_drop_page(dest_ctx, sheet);
			sheet_pagenum = positions[i].sheet_pagenum;
			sheet = pdf_load_page(dest_ctx, dest_doc, sheet_pagenum);
			// TODO: We assume that the destination-page uses arrays for the
			// content and has a usable resoruce-dict... TODO: add those checks
		}
		
		/* load the source-page */
		pdf_page *src_page = pdf_load_page(src_ctx, src_doc, positions[i].src_pagenum);
		
		/* create a rename_dict */
		put_info.rename_dict = pdf_new_dict(dest_ctx, dest_doc, RENAME_INITIAL_CAP);
	
		/* copy all resources, adjust the page and finally copy the content */
		copy_and_rename_resources(dest_ctx, sheet->resources, 
			src_ctx, src_page->resources, &put_info);
		adjust_page_position(src_ctx, src_doc, src_page, positions + i);
		copy_content_streams_of_page(
			dest_ctx, sheet, src_ctx, src_page, &put_info, positions + i);

		/* free everything we created for that source-page */
		pdf_drop_obj(dest_ctx, put_info.rename_dict);
		pdf_drop_page(src_ctx, src_page);
	}

	if(sheet != NULL)
		pdf_drop_page(dest_ctx, sheet);

	return(0);
}

static ErrorCode juggler_impose_create_sheet(fz_context *ctx, pdf_document *dest_doc, int width, int height)
{
	/* create and insert page to dest_doc */
	fz_rect rect = { 0, 0, width, height };// { 0, 0, 1390, 1684 };
	pdf_page *sheet = pdf_create_page(ctx, dest_doc, rect, 0, 0);

	sheet->resources = pdf_new_dict(ctx, dest_doc, 16);
	pdf_dict_puts(ctx, sheet->me, "Resources", sheet->resources); /* will be droped when freeing the page */
	sheet->contents = pdf_new_array(ctx, dest_doc, 8);
	pdf_dict_puts(ctx, sheet->me, "Contents", sheet->contents); /* will be droped when freeing the page */

	pdf_insert_page(ctx, dest_doc, sheet, INT_MAX);
	
	pdf_drop_page(ctx, sheet);
	
	return(NoError);
}

static pdf_document *test_creation(juggler_t *src)
{
	/* create new document */
	pdf_document *new_doc = pdf_create_document(src->ctx);
	
	size_t i;
	size_t sheets_count = src->pagecount / 2 + (src->pagecount % 2);
	for(i = 0; i < sheets_count; i++)
		juggler_impose_create_sheet(src->ctx, new_doc, 595.27 * 2, 842);
		
	return(new_doc);
}

ErrorCode juggler_put_page_contents(juggler_t *src, const char *filename)
{
	pdf_document *new_doc = test_creation(src);
	
	struct pos_info *pos = calloc(src->pagecount, sizeof(struct pos_info));
	int i = 0;
	for(; i < src->pagecount; i++) {
		//, 695 * i, 0, 695, 942, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 1.0 };
		pos[i].scale = 1.0;
		pos[i].x = 595.27 * (i % 2);
		pos[i].y = 0;
		pos[i].width = 595.27;
		pos[i].height = 842;
		pos[i].outer_clip_x = pos[i].x;
		pos[i].outer_clip_y = pos[i].y;
		pos[i].outer_clip_width = pos[i].width;
		pos[i].outer_clip_height = pos[i].height;
		pos[i].sheet_pagenum = i / 2;
		pos[i].src_pagenum = i;
	}
	
	put_pages_on_new_sheet(src->ctx, new_doc, src->ctx, src->pdf, pos, src->pagecount);

	/* write the new document to disk */
	fz_write_options write_options;
	write_options.do_incremental = 0;
	write_options.do_ascii = 0;
	write_options.do_expand = 0;
	write_options.do_garbage = 1;
	write_options.do_linear = 0;
	write_options.do_clean = 0;
	write_options.continue_on_error = 0;
	pdf_write_document(src->ctx, new_doc, filename, &write_options);

	/* and close it */
	pdf_close_document(src->ctx, new_doc);

	return(NoError);
}

/* WE DON'T NEED THAT */
void juggler_impose(juggler_t *doc)
{
/* 1. How to impose? thread-stitching, saddle-stitch */
/* 2. What master is used? How many pages are need for the sheet */
/* 3. Depending on the stitching method get the pages that are need */
}
