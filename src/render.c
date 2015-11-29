/*
  render.c - render pdf-contents to an image
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

#include "render.h"

#include "internal.h"

/* parts of this function have been copied from MuPDF */
ErrorCode get_all_pages_size(juggler_t *juggler)
{
	pdf_document *doc = juggler->pdf;
	fz_context *ctx = juggler->ctx;

	int pagecount = juggler->pagecount;
	juggler->sizes = malloc(sizeof(page_size) * pagecount);

	int i, currentX = 0;
	for(i = 0; i < pagecount; i++) {
		pdf_obj *pageobj = pdf_lookup_page_obj(ctx, doc, i);
		fz_rect mediabox, cropbox;

		float userunit;
		pdf_obj *obj = pdf_dict_gets(ctx, pageobj, "UserUnit");
		if (pdf_is_real(ctx, obj))
			userunit = pdf_to_real(ctx, obj);
		else
			userunit = 1;

		pdf_to_rect(ctx, juggler_lookup_inherited_page_item(ctx, doc, pageobj, 
			"MediaBox"), &mediabox);
		if(fz_is_empty_rect(&mediabox))
		{
			//fz_warn(ctx, "cannot find page size for page %d", number + 1);
			mediabox.x0 = 0;
			mediabox.y0 = 0;
			mediabox.x1 = 612;
			mediabox.y1 = 792;
		}

		pdf_to_rect(ctx, juggler_lookup_inherited_page_item(ctx, doc, pageobj, 
            "CropBox"), &cropbox);
		if(!fz_is_empty_rect(&cropbox))
			fz_intersect_rect(&mediabox, &cropbox);

		juggler->sizes[i].x = currentX;
		juggler->sizes[i].width = fz_max(mediabox.x0, mediabox.x1) * userunit - fz_min(mediabox.x0, mediabox.x1) * userunit;
		juggler->sizes[i].height = fz_max(mediabox.y0, mediabox.y1) * userunit - fz_min(mediabox.y0, mediabox.y1) * userunit;

		int rotate = pdf_to_int(ctx, 
			juggler_lookup_inherited_page_item(ctx, doc, pageobj, "Rotate"));
		if(rotate == 90 || rotate == 270) {
			int real_height = juggler->sizes[i].width;
			juggler->sizes[i].width = juggler->sizes[i].height;
			juggler->sizes[i].height = real_height;
		}

		currentX += juggler->sizes[i].height + 10;
	}
	
	return(NoError);
}

render_data_t *is_already_rendered(juggler_t *juggler, render_data_t *needle)
{
	size_t i;
	for(i = 0; i < RENDER_BUFFER_COUNT; i++) {
		if(juggler->rendered[i].samples != NULL &&
		   //juggler->rendered[i].x == needle->x &&
		   //juggler->rendered[i].y == needle->y &&
		   //juggler->rendered[i].width == needle->width &&
		   //juggler->rendered[i].height == needle->height &&
		   juggler->rendered[i].pagenum == needle->pagenum &&
		   juggler->rendered[i].zoom == needle->zoom)
			return(&juggler->rendered[i]);
	}
	return(NULL);
}

ErrorCode render_page(juggler_t *juggler, render_data_t *data)
{
	pdf_document *doc = juggler->pdf;
	fz_context *ctx = juggler->ctx;

	render_data_t *rendered;
	if((rendered = is_already_rendered(juggler, data)) != NULL) {
		*data = *rendered;
		return(NoError);
	}

	pdf_page *page = pdf_load_page(ctx, doc, data->pagenum);

	int rotation = 0;

	fz_matrix transform;
	fz_rotate(&transform, rotation);
	fz_pre_scale(&transform, data->zoom, data->zoom);

	fz_rect bounds;
	pdf_bound_page(ctx, page, &bounds);
	fz_transform_rect(&bounds, &transform);

	fz_irect bbox;
	fz_round_rect(&bbox, &bounds);
	fz_pixmap *pix = fz_new_pixmap_with_bbox(ctx, fz_device_bgr(ctx), &bbox);
	fz_clear_pixmap_with_value(ctx, pix, 0xff);

	fz_device *dev = fz_new_draw_device(ctx, pix);
	pdf_run_page(ctx, page, dev, &transform, NULL);
	fz_drop_device(ctx, dev);

	data->samples = pix->samples;
	data->width = pix->w;
	data->height = pix->h;
	data->x = 0;
	data->y = 0;

	pdf_drop_page(ctx, page);

	return(NoError);
}

void free_rendered_data(juggler_t *juggler, render_data_t *data)
{
	juggler->recent_rendered = (juggler->recent_rendered + 1) % RENDER_BUFFER_COUNT;

	if(juggler->rendered[juggler->recent_rendered].samples != NULL)
		fz_drop_pixmap(juggler->ctx, juggler->rendered[juggler->recent_rendered].pixmap);

	juggler->rendered[juggler->recent_rendered] = *data;
}
