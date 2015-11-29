/*
  put-content.c - export all images of the pdf
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

#define _GNU_SOURCE
#include <stdio.h>
#include <tiffio.h>

#include "export-images.h"

#include "internal.h"

#define COPY_BUFFERSIZE (64 * 1024)


struct image_info
{
	char *filter;
	enum { COLOR_GRAY, COLOR_RGB, COLOR_CMYK, COLOR_BW_BITMAP, COLOR_NOT_SPECIFIED } colorspace;
	char using_icc;
	char using_indexed;
	union
	{
		struct 
		{
			pdf_obj *profile;
			// possible icc-colorspaces according to reference
			enum icc_colorspace_t { 
				ICC_XYZData		= 0x58595A20,
				ICC_labData		= 0x4C616220,
				ICC_luvData		= 0x4C757620,
				ICC_YCbCrData	= 0x59436272,
				ICC_YxyData		= 0x59787920,
				ICC_rgbData		= 0x52474220,
				ICC_grayData	= 0x47524159,
				ICC_hsvData		= 0x48535620,
				ICC_hlsData		= 0x484C5320,
				ICC_cmykData	= 0x434D594B,
				ICC_cmyData		= 0x434D5920,
				ICC_2colorData	= 0x32434C52,
				ICC_3colorData	= 0x33434C52,
				ICC_4colorData	= 0x34434C52,
				ICC_5colorData	= 0x35434C52,
				ICC_6colorData	= 0x36434C52,
				ICC_7colorData	= 0x37434C52,
				ICC_8colorData	= 0x38434C52,
				ICC_9colorData	= 0x39434C52,
				ICC_10colorData	= 0x41434C52,
				ICC_11colorData	= 0x42434C52,
				ICC_12colorData	= 0x43434C52,
				ICC_13colorData	= 0x44434C52,
				ICC_14colorData	= 0x45434C52,
				ICC_15colorData	= 0x46434C52
			} colorspace;
		} icc_based;
		
		struct {
			pdf_obj *lookup;
			int high_value;
			
		} indexed;
		
	} more;
};

/*struct jpeg_icc_tag
{
	char marker[2]; // 0xFF, 0xE2
	char len_high;
	char len_low;
	char identifier[12]; // "ICC_PROFILE\0x00"
	char marker_num; // number of markers of that profile
	char marker_total; // number that this profile needs in total
	// now the icc-profile ;)
};*/
static int export_rgb_jpeg(fz_context *ctx, pdf_document *doc, pdf_obj *image, char *filename)
{
    memcpy(filename + (strlen(filename) - 3), "jpg", 3);
	printf("Exporting rgb jpeg image %s\n", filename);
	
	fz_stream *stream = pdf_open_raw_stream(ctx, doc, 
		pdf_to_num(ctx, image), pdf_to_gen(ctx, image));
		
	FILE *file = fopen(filename, "wb");
	if(file == NULL)
		return(-1);

	unsigned char buffer[COPY_BUFFERSIZE];
	
	/* TODO: Make the inclusion of ICC-profiles standard-compliant, so that we
	         don't need to use TIFF for RGB JPEGs with an ICC-profile
	fz_read(stream, buffer, 2); // jump over soi marker
	
	char soi[] = { 0xFF, 0xD8 };
	fwrite(soi, sizeof(soi), 1, file);
	
	struct jpeg_icc_tag tag;
	tag.marker[0] = 0xFF;
	tag.marker[1] = 0xE2;
	size_t tag_len = data_len + sizeof(struct jpeg_icc_tag) - sizeof(tag.marker);
	tag.len_high = (tag_len & 0xFF00) >> 8;
	tag.len_low = tag_len & 0xFF;
	memcpy(tag.identifier, "ICC_PROFILE\x00", sizeof("ICC_PROFILE\x00") - 1);
	tag.marker_num = 1;
	tag.marker_total = 1;
	fwrite(&tag, sizeof(struct jpeg_icc_tag), 1, file);
	fwrite(data, data_len, 1, file);*/

	size_t read_bytes;
	while((read_bytes = fz_read(ctx, stream, buffer, COPY_BUFFERSIZE)) > 0)
		fwrite(buffer, read_bytes, 1, file);
		
	fclose(file);
	fz_drop_stream(ctx, stream);
	
	return(0);
}

static int export_jpeg_2000(fz_context *ctx, pdf_document *doc, pdf_obj *image, char *filename)
{
	// JPXDecode is a bit more complicated than just simply exporting it!
	// e. g. if a ColorSpace is present we must ignore this, ...
	// TODO: Take care of those things as this code doesn't yet
	memcpy(filename + (strlen(filename) - 3), "jpx", 3);
	
	printf("Exporting JPEG-2000 image %s\n", filename);
	
	fz_stream *stream = pdf_open_raw_stream(ctx, doc, 
		pdf_to_num(ctx, image), pdf_to_gen(ctx, image));
	FILE *file = fopen(filename, "wb");
	if(file == NULL)
		return(-1);

	unsigned char buffer[COPY_BUFFERSIZE];

	size_t read_bytes;
	while((read_bytes = fz_read(ctx, stream, buffer, COPY_BUFFERSIZE)) > 0)
		fwrite(buffer, read_bytes, 1, file);
		
	fclose(file);
	fz_drop_stream(ctx, stream);
	
	return(0);
}

static int export_icc_to_tiff(fz_context *ctx, pdf_document *doc, 
	pdf_obj *profile, TIFF *out)
{
	fz_stream *stream = pdf_open_stream(ctx, doc, 
		pdf_to_num(ctx, profile), pdf_to_gen(ctx, profile));
	
	// most profiles won't exceed 4 kilobytes
	fz_buffer *buffer = fz_read_all(ctx, stream, 4096);
	
	int ret = TIFFSetField(out, TIFFTAG_ICCPROFILE, buffer->len, buffer->data);
	
	fz_drop_buffer(ctx, buffer);
	fz_drop_stream(ctx, stream);
	
	return(ret ? 0 : -1);
}

static int export_indexed_as_tif(fz_context *ctx, pdf_document *doc, 
	pdf_obj *image, char *filename, struct image_info *info)
{
	if(!pdf_is_int(ctx, pdf_dict_gets(ctx, image, "Width")) || 
		!pdf_is_int(ctx, pdf_dict_gets(ctx, image, "Height")))
	{
		return(-1);
	}
	
	memcpy(filename + (strlen(filename) - 3), "tif", 3);
	printf("Exporting indexed tif-image %s using MuPDF encoder\n", filename);
	
	int width = pdf_to_int(ctx, pdf_dict_gets(ctx, image, "Width"));
	int height = pdf_to_int(ctx, pdf_dict_gets(ctx, image, "Height"));
	int bits_per_sample;
	if(pdf_is_int(ctx, pdf_dict_gets(ctx, image, "BitsPerComponent")))
		bits_per_sample = pdf_to_int(ctx, pdf_dict_gets(ctx, image, "BitsPerComponent"));
	else
		bits_per_sample = 1; // BitsPerComponent is always required except for bitmaps
	
	// determine TIFF-colorspace and the number of samples per pixel using image_info
	int sample_per_pixel;
	int photometric;
	if(info->colorspace == COLOR_GRAY) {
		photometric = PHOTOMETRIC_MINISWHITE;
		sample_per_pixel = 1;
	} else if(info->colorspace == COLOR_RGB) {
		photometric = PHOTOMETRIC_RGB;
		sample_per_pixel = 3;
	} else if(info->colorspace == COLOR_CMYK) {
		photometric = PHOTOMETRIC_SEPARATED;
		sample_per_pixel = 4;
	} else if(info->colorspace == COLOR_BW_BITMAP) {
		photometric = PHOTOMETRIC_MINISBLACK;
		sample_per_pixel = 1;
	} else {
		printf("Unknown image file format\n");
		return(-1);
	}
	
	TIFF *out= TIFFOpen(filename, "w");
	
	if(!TIFFSetField(out, TIFFTAG_IMAGEWIDTH, width) ||
		!TIFFSetField(out, TIFFTAG_IMAGELENGTH, height) ||
		!TIFFSetField(out, TIFFTAG_SAMPLESPERPIXEL, sample_per_pixel) ||
		!TIFFSetField(out, TIFFTAG_BITSPERSAMPLE, bits_per_sample) ||
		!TIFFSetField(out, TIFFTAG_ORIENTATION, ORIENTATION_TOPLEFT) ||
		!TIFFSetField(out, TIFFTAG_PLANARCONFIG, PLANARCONFIG_CONTIG) ||
		!TIFFSetField(out, TIFFTAG_PHOTOMETRIC, photometric))
	{
		printf("Error: could not initialize TIFF-image\n");
		return(-1);
	}
	
		
	if(info->using_icc && export_icc_to_tiff(ctx, doc, info->more.icc_based.profile, out) < 0)
		return(-1);
	/* indexed special */
	fz_buffer *lookup_buffer = NULL;
	unsigned char *lookup;
	//size_t lookup_size;
	if(pdf_is_string(ctx, info->more.indexed.lookup)) {
		lookup = 
			(unsigned char *) pdf_to_str_buf(ctx, info->more.indexed.lookup);
		//lookup_size = pdf_to_str_len(ctx, info->more.indexed.lookup);
	} else {
		fz_stream *stream = pdf_open_stream(ctx, doc, 
			pdf_to_num(ctx, info->more.indexed.lookup), 
			pdf_to_gen(ctx, info->more.indexed.lookup));

		lookup_buffer = fz_read_all(ctx, stream, 0x100);
		lookup = lookup_buffer->data;
		//lookup_size = lookup_buffer->len;
		fz_drop_stream(ctx, stream);
	}
	
	size_t input_linesize = width; // always width bytes...
	unsigned char *input_line = malloc(input_linesize);
	
	size_t bytes_per_input = bits_per_sample * sample_per_pixel / 8;
	/* input-special */
	
	size_t line_bytes = bits_per_sample * sample_per_pixel * width / 8;
	size_t scanline_size = TIFFScanlineSize(out) < line_bytes ? line_bytes : TIFFScanlineSize(out);
	unsigned char *scanline = TIFFScanlineSize(out) < line_bytes ? 
		_TIFFmalloc(line_bytes) : _TIFFmalloc(TIFFScanlineSize(out));
	
	if(scanline == NULL || input_line == NULL) {
		goto err;
	}
	
	if(!TIFFSetField(out, TIFFTAG_ROWSPERSTRIP, TIFFDefaultStripSize(out, scanline_size))) {
		printf("Error: could not initialize TIFF-image\n");
		_TIFFfree(scanline);
		return(-1);
	}
	
	/* indexed-special */
	printf("Number of indexed image: %d %d\n", pdf_to_num(ctx, image), pdf_to_gen(ctx, image));
	fz_stream *stream = pdf_open_stream(ctx, doc, pdf_to_num(ctx, image), pdf_to_gen(ctx, image));

	size_t read_bytes;
	size_t row = 0;

	while((read_bytes = fz_read(ctx, stream, input_line, input_linesize)) > 0) {
		size_t scanline_pos = 0;
		for(size_t i = 0; i < input_linesize; i++) {
			memcpy(scanline + scanline_pos, lookup + (input_line[i] * bytes_per_input), bytes_per_input);
			scanline_pos += bytes_per_input;
		}
		
		if(TIFFWriteScanline(out, scanline, row, 0) < 0) {
			break;
		}
		row++;
	}
	/* indexed-special */

err:
	fz_drop_buffer(ctx, lookup_buffer);
	fz_drop_stream(ctx, stream);
	TIFFClose(out);
	if(input_line)
		free(input_line);
	if(scanline)
		 _TIFFfree(scanline);
		 
	return(row < width ? -1 : 0);
}

static TIFF *initialize_suitable_tif(fz_context *ctx, pdf_document *doc, 
	pdf_obj *image, char *filename, struct image_info *info, 
	size_t *width_ptr, size_t *height_ptr, 
	size_t *line_bytes_ptr, size_t *scanline_size_ptr)
{
	if(!pdf_is_int(ctx, pdf_dict_gets(ctx, image, "Width")) || 
		!pdf_is_int(ctx, pdf_dict_gets(ctx, image, "Height")))
	{
		return(NULL);
	}
	
	int width = pdf_to_int(ctx, pdf_dict_gets(ctx, image, "Width"));
	int height = pdf_to_int(ctx, pdf_dict_gets(ctx, image, "Height"));
	int bits_per_sample;
	if(pdf_is_int(ctx, pdf_dict_gets(ctx, image, "BitsPerComponent")))
		bits_per_sample = pdf_to_int(ctx, pdf_dict_gets(ctx, image, "BitsPerComponent"));
	else
		bits_per_sample = 1; // BitsPerComponent is always required except for bitmaps
	
	// determine TIFF-colorspace and the number of samples per pixel using image_info
	int sample_per_pixel;
	int photometric;
	if(info->colorspace == COLOR_GRAY) {
		photometric = PHOTOMETRIC_MINISWHITE;
		sample_per_pixel = 1;
	} else if(info->colorspace == COLOR_RGB) {
		photometric = PHOTOMETRIC_RGB;
		sample_per_pixel = 3;
	} else if(info->colorspace == COLOR_CMYK) {
		photometric = PHOTOMETRIC_SEPARATED;
		sample_per_pixel = 4;
	} else if(info->colorspace == COLOR_BW_BITMAP) {
		photometric = PHOTOMETRIC_MINISBLACK;
		sample_per_pixel = 1;
	} else {
		printf("Unknown image colorspace\n");
		return(NULL);
	}
	
	// prevent integer overflows when converting to size_t
	if(width < 0 || height < 0 || bits_per_sample < 0 || sample_per_pixel < 0)
		return(NULL);
	
	*height_ptr = (size_t) height;
	*width_ptr = (size_t) width;
	*line_bytes_ptr = (size_t) bits_per_sample * 
		(size_t) sample_per_pixel * (size_t) width / 8;
	// in case of an black/white-bitmap add padding, if needed
	if(bits_per_sample == 1 && width % 8)
		(*line_bytes_ptr)++;

	
	TIFF *out= TIFFOpen(filename, "w");
	
	if(!TIFFSetField(out, TIFFTAG_IMAGEWIDTH, width) ||
		!TIFFSetField(out, TIFFTAG_IMAGELENGTH, height) ||
		!TIFFSetField(out, TIFFTAG_SAMPLESPERPIXEL, sample_per_pixel) ||
		!TIFFSetField(out, TIFFTAG_BITSPERSAMPLE, bits_per_sample) ||
		!TIFFSetField(out, TIFFTAG_ORIENTATION, ORIENTATION_TOPLEFT) ||
		!TIFFSetField(out, TIFFTAG_PLANARCONFIG, PLANARCONFIG_CONTIG) ||
		!TIFFSetField(out, TIFFTAG_PHOTOMETRIC, photometric))
	{
		return(NULL);
	}
		
	if(info->using_icc && 
		export_icc_to_tiff(ctx, doc, info->more.icc_based.profile, out) < 0)
	{
		return(NULL);
	}
	
	*scanline_size_ptr = TIFFScanlineSize(out) < *line_bytes_ptr ? *line_bytes_ptr : TIFFScanlineSize(out);
		
	if(!TIFFSetField(out, TIFFTAG_ROWSPERSTRIP, TIFFDefaultStripSize(out, *scanline_size_ptr)))
		return(NULL);
	
	return(out);
}

static int export_tif_pixels(fz_context *ctx, pdf_document *doc, 
	pdf_obj *image, size_t height, size_t line_bytes, 
	size_t scanline_size, TIFF *out)
{
	unsigned char *scanline = _TIFFmalloc(scanline_size < line_bytes ? line_bytes : scanline_size);
	if(scanline == NULL)
		return(-1);
		
	fz_stream *stream = pdf_open_stream(ctx, doc, 
		pdf_to_num(ctx, image), pdf_to_gen(ctx, image));
		
	size_t read_bytes;
	size_t row = 0;
	
	while((read_bytes = fz_read(ctx, stream, scanline, line_bytes)) > 0) {
		if(TIFFWriteScanline(out, scanline, row, 0) < 0)
			break;
		row++;
	}
	
	fz_drop_stream(ctx, stream);
	TIFFClose(out);
	_TIFFfree(scanline);
	
	return(row < height ? -1 : 0);
}

static int export_tif_indexed_pixels(fz_context *ctx, pdf_document *doc, 
	pdf_obj *image, struct image_info *info, size_t width, size_t height, 
	size_t line_bytes, size_t scanline_size, TIFF *out)
{
	fz_buffer *lookup_buffer = NULL;
	unsigned char *lookup;
	//size_t lookup_size;
	if(pdf_is_string(ctx, info->more.indexed.lookup)) {
		lookup = 
			(unsigned char *) pdf_to_str_buf(ctx, info->more.indexed.lookup);
		//lookup_size = pdf_to_str_len(ctx, info->more.indexed.lookup);
	} else {
		fz_stream *stream = pdf_open_stream(ctx, doc, 
			pdf_to_num(ctx, info->more.indexed.lookup), 
			pdf_to_gen(ctx, info->more.indexed.lookup));

		lookup_buffer = fz_read_all(ctx, stream, 0x100);
		lookup = lookup_buffer->data;
		//lookup_size = lookup_buffer->len;
		fz_drop_stream(ctx, stream);
	}
	
	size_t input_linesize = width; // always width bytes...
	unsigned char *input_line = malloc(input_linesize);
	
	size_t bytes_per_input = line_bytes / width;
	
	unsigned char *scanline = _TIFFmalloc(scanline_size < line_bytes ? line_bytes : scanline_size);
	if(scanline == NULL)
		return(-1);
	
	printf("Number of indexed image: %d %d\n", pdf_to_num(ctx, image), pdf_to_gen(ctx, image));
	fz_stream *stream = pdf_open_stream(ctx, doc, pdf_to_num(ctx, image), pdf_to_gen(ctx, image));

	size_t read_bytes;
	size_t row = 0;

	while((read_bytes = fz_read(ctx, stream, input_line, input_linesize)) > 0) {
		size_t scanline_pos = 0;
		for(size_t i = 0; i < input_linesize; i++) {
			memcpy(scanline + scanline_pos, lookup + (input_line[i] * bytes_per_input), bytes_per_input);
			scanline_pos += bytes_per_input;
		}
		
		if(TIFFWriteScanline(out, scanline, row, 0) < 0) {
			break;
		}
		row++;
	}
	
	fz_drop_buffer(ctx, lookup_buffer);
	fz_drop_stream(ctx, stream);
	TIFFClose(out);
	if(input_line)
		free(input_line);
	if(scanline)
		 _TIFFfree(scanline);
		 
	return(row < width ? -1 : 0);
}

static int export_tif_from_decoded(fz_context *ctx, pdf_document *doc, 
	pdf_obj *image, char *filename, struct image_info *info)
{
	memcpy(filename + (strlen(filename) - 3), "tif", 3);

//	if(info->using_indexed)
	//	return(export_indexed_as_tif(ctx, doc, image, filename, info));
	printf("Exporting tif-image %s using MuPDF encoder\n", filename);
	
	size_t width, height, line_bytes, scanline_size;
	TIFF *out = initialize_suitable_tif(ctx, doc, image, filename, info, 
			&width, &height, &line_bytes, &scanline_size);
	
	if(out == NULL) {
		printf("Error: could not initialize TIFF-image\n");
		return(-1);
	}
	
	if(info->using_indexed)
		return(export_tif_indexed_pixels(ctx, doc, image, info, width, height, line_bytes, scanline_size, out));
	else
		return(export_tif_pixels(ctx, doc, image, height, line_bytes, scanline_size, out));
}

static int read_colorspace_obj(fz_context *ctx, pdf_document *doc, 
	pdf_obj *colorspace_obj, struct image_info *info)
{
	if(pdf_is_name(ctx, colorspace_obj)) {
		char *colorspace = pdf_to_name(ctx, colorspace_obj);
		if(!strcmp(colorspace, "DeviceGray")) {
			info->colorspace = COLOR_GRAY;
		} else if(!strcmp(colorspace, "DeviceRGB")) {
			info->colorspace = COLOR_RGB;
		} else if(!strcmp(colorspace, "DeviceCMYK")) {
			info->colorspace = COLOR_CMYK;
		// We use the default colors in its desired color-space, thus no tranformation is needed
		} else if(!strcmp(colorspace, "DefaultGray")) {
			info->colorspace = COLOR_GRAY;
		} else if(!strcmp(colorspace, "DefaultRGB")) {
			info->colorspace = COLOR_RGB;
		} else if(!strcmp(colorspace, "DefaultCMYK")) {
			info->colorspace = COLOR_CMYK;
		} else {
			printf("Invalid name of colorspace %s\n", colorspace);
			return(-1);
		}
	} else if(pdf_is_array(ctx, colorspace_obj) && 
		pdf_is_name(ctx, pdf_array_get(ctx, colorspace_obj, 0))) 
	{
		char *type = pdf_to_name(ctx, pdf_array_get(ctx, colorspace_obj, 0));
		
		if(!strcmp(type, "ICCBased")) {
			pdf_obj *profile = pdf_array_get(ctx, colorspace_obj, 1);
			if(!pdf_is_indirect(ctx, profile) || !pdf_is_dict(ctx, profile) || 
				!pdf_is_stream(ctx, doc, pdf_to_num(ctx, profile), pdf_to_gen(ctx, profile)))
			{
				return(-1);
			}
			
			info->using_icc = 1;
			info->more.icc_based.profile = profile;
			
			// TODO: Get real colorspace, not just that guess
			pdf_obj *n = pdf_dict_gets(ctx, profile, "N");
			if(pdf_is_int(ctx, n) && pdf_to_int(ctx, n) == 3) {
				info->more.icc_based.colorspace = ICC_rgbData;
				info->colorspace = COLOR_RGB;
			} else if(pdf_is_int(ctx, n) && pdf_to_int(ctx, n) == 4) {
				info->more.icc_based.colorspace = ICC_cmykData;
				info->colorspace = COLOR_CMYK;
			} else if(pdf_is_int(ctx, n) && pdf_to_int(ctx, n) == 1) {
				info->more.icc_based.colorspace = ICC_grayData;
				info->colorspace = COLOR_GRAY;
			} else {
				printf("Ignoring unknown ICC-colorspace");
				return(-1);
			}
		} else {
			printf("Ignoring colorspace %s\n", 
				pdf_to_name(ctx, pdf_array_get(ctx, colorspace_obj, 0)));
			return(-1);
		}
	} else {
		printf("Found invalid specified colorspace\n");
		return(-1);
	}
	
	return(0);
}

static int read_colorspace(fz_context *ctx, pdf_document *doc, 
	pdf_obj *image, struct image_info *info)
{
	int ret = 0;
	pdf_obj *colorspace_obj = pdf_dict_gets(ctx, image, "ColorSpace");
	pdf_obj *image_mask = pdf_dict_gets(ctx, image, "ImageMask");
	
	if(pdf_is_array(ctx, colorspace_obj) && 
		pdf_array_len(ctx, colorspace_obj) == 4 &&
		pdf_is_name(ctx, pdf_array_get(ctx, colorspace_obj, 0)) &&
		!strcmp(pdf_to_name(ctx, pdf_array_get(ctx, colorspace_obj, 0)), "Indexed"))
	{
		if(!pdf_is_int(ctx, pdf_array_get(ctx, colorspace_obj, 2))) {
			ret = -1;
			goto err;
		}
		printf("Found indexed\n");
		
		info->using_indexed = 1;
		info->more.indexed.high_value = 
			pdf_to_int(ctx, pdf_array_get(ctx, colorspace_obj, 2));
		
		register pdf_obj *lookup = pdf_array_get(ctx, colorspace_obj, 3);
		if((!pdf_is_indirect(ctx, lookup) || 
			!pdf_is_stream(ctx, doc, pdf_to_num(ctx, lookup), pdf_to_gen(ctx, lookup))) &&
			!pdf_is_string(ctx, lookup)) 
		{
			ret = -1;
			goto err;
		}
		info->more.indexed.lookup = lookup;
		
		if((ret = read_colorspace_obj(ctx, doc, 
			pdf_array_get(ctx, colorspace_obj, 1), info)) < 0)
		{
			goto err;
		}
	} else if(pdf_is_bool(ctx, image_mask) && pdf_to_bool(ctx, image_mask)) {
		info->colorspace = COLOR_BW_BITMAP;
	} else {
		ret = read_colorspace_obj(ctx, doc, colorspace_obj, info);
	}	

err:
	return(ret);
}

static int export_image(fz_context *ctx, pdf_document *doc, pdf_obj *image, char *filename)
{
	pdf_obj *filter_object = pdf_dict_gets(ctx, image, "Filter");
	
	char *filter = "";
	if(pdf_is_name(ctx, filter_object)) {
		filter = pdf_to_name(ctx, filter_object);
	} else if(pdf_is_array(ctx, filter_object) && 
		pdf_array_len(ctx, filter_object) == 1 &&
		pdf_is_name(ctx, pdf_array_get(ctx, filter_object, 0))) 
	{
		filter = pdf_to_name(ctx, pdf_array_get(ctx, filter_object, 0));
	}
	
	struct image_info info;
	info.using_icc = 0;
	info.using_indexed = 0;
	info.filter = NULL;
	if(read_colorspace(ctx, doc, image, &info) < 0) {
		printf("Skipping image due to unknown colorspace\n");
		return(-1);
	}
	
	if(!strcmp(filter, "JPXDecode")) {
		return(export_jpeg_2000(ctx, doc, image, filename));
	} else if(!strcmp(filter, "DCTDecode") && 
		info.colorspace == COLOR_RGB && !info.using_icc && !info.using_indexed)
	{
		return(export_rgb_jpeg(ctx, doc, image, filename));
	} else if(info.colorspace == COLOR_RGB) { // all other cases with rgb
		return(export_tif_from_decoded(ctx, doc, image, filename, &info));
	} else if(info.colorspace == COLOR_CMYK) {
		return(export_tif_from_decoded(ctx, doc, image, filename, &info));
	} else if(info.colorspace == COLOR_GRAY) {
		return(export_tif_from_decoded(ctx, doc, image, filename, &info));
	} else if(info.colorspace == COLOR_BW_BITMAP) {
		return(export_tif_from_decoded(ctx, doc, image, filename, &info));
	} else {
		printf("Found image, but cannot handle encoding %s (%d %d)\n", 
			filter, pdf_to_num(ctx, image), pdf_to_gen(ctx, image));
		return(-1);
	}
}

static int export_page_images(fz_context *ctx, pdf_document *doc, 
	const char *path, int page_num)
{	
	pdf_page *page = pdf_load_page(ctx, doc, page_num);
	pdf_obj *xobjects = pdf_dict_gets(ctx, page->resources, "XObject");
	if(!pdf_is_dict(ctx, xobjects))
		return(NoError);
	
	for(int i = 0; i < pdf_dict_len(ctx, xobjects); i++) {
		pdf_obj *image = pdf_dict_get_val(ctx, xobjects, i);
		
		pdf_obj *subtype;
		if(pdf_is_indirect(ctx, image) && pdf_is_dict(ctx, image) && 
			(subtype = pdf_dict_gets(ctx, image, "Subtype")) != NULL &&
			pdf_is_name(ctx, subtype) && 
			!strcmp(pdf_to_name(ctx, subtype), "Image"))
		{
			char *filename = NULL;
			asprintf(&filename, "%spage%03d-image%03d.???", path, page_num, i);
			export_image(ctx, doc, image, filename);
			free(filename);
		}
	}
	pdf_drop_page(ctx, page);
	
	return(0);
}

ErrorCode juggler_export_images(juggler_t *juggler, const char *path)
{
	printf("Starting with exporting images...\n");
	
	for(int i = 0; i < pdf_count_pages(juggler->ctx, juggler->pdf); i++)
		export_page_images(juggler->ctx, juggler->pdf, path, i);
	
	printf("End of exporting images...\n");
	
	return(NoError);
}