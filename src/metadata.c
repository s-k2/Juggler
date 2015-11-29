/*
  metadata.c - functions to edit the metadata of an pdf
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

#include "metadata.h"


ErrorCode juggler_get_meta_data(juggler_t *juggler, meta_data *data)
{
	pdf_obj *info = pdf_dict_gets(juggler->ctx, 
		pdf_trailer(juggler->ctx, juggler->pdf), "Info");

	// this may be null if there is no info-dictionary in the pdf
	if(info != NULL) {
		pdf_obj *title = pdf_dict_gets(juggler->ctx, info, "Title");
		data->title = title != NULL ? 
			strdup(pdf_to_utf8(juggler->ctx, juggler->pdf, title)) : NULL;

		pdf_obj *author = pdf_dict_gets(juggler->ctx, info, "Author");
		data->author = author != NULL ? 
			strdup(pdf_to_utf8(juggler->ctx, juggler->pdf, author)) : NULL;

		pdf_obj *subject = pdf_dict_gets(juggler->ctx, info, "Subject");
		data->subject = subject != NULL ? 
			strdup(pdf_to_utf8(juggler->ctx, juggler->pdf, subject)) : NULL;

		pdf_obj *keywords = pdf_dict_gets(juggler->ctx, info, "Keywords");
		data->keywords = keywords != NULL ? 
			strdup(pdf_to_utf8(juggler->ctx, juggler->pdf, keywords)) : NULL;

		pdf_obj *creationDate = pdf_dict_gets(juggler->ctx, info, "CreationDate");
		data->creationDate = creationDate != NULL ? 
			strdup(pdf_to_str_buf(juggler->ctx, creationDate)) : NULL;

		pdf_obj *modifyDate = pdf_dict_gets(juggler->ctx, info, "ModifyDate");
		data->modifyDate = modifyDate != NULL ? 
			strdup(pdf_to_str_buf(juggler->ctx, modifyDate)) : NULL;

		pdf_obj *creator = pdf_dict_gets(juggler->ctx, info, "Creator");
		data->creator = creator != NULL ? 
			strdup(pdf_to_utf8(juggler->ctx, juggler->pdf, creator)) : NULL;

		pdf_obj *producer = pdf_dict_gets(juggler->ctx, info, "Producer");
		data->producer = producer != NULL ? 
			strdup(pdf_to_utf8(juggler->ctx, juggler->pdf, producer)) : NULL;
	} else {
		memset(data, 0, sizeof(meta_data));
	}

	return(NoError);
}


static ErrorCode juggler_create_document_info(juggler_t *juggler, pdf_obj **newInfo)
{
	pdf_obj *trailer = pdf_trailer(juggler->ctx, juggler->pdf);

	pdf_obj *info = pdf_new_dict(juggler->ctx, juggler->pdf, 16);
	pdf_dict_puts(juggler->ctx, trailer, "Info", info);

    // According to the PDF Reference (page 844), 
	// ModDate must be set if PieceInfo exists!
	if(pdf_dict_gets(juggler->ctx, trailer, "PieceInfo") != NULL) {

		// write it this way: D:YYYYMMDDHHmmSSOHH'mm'
        //                    01234567890123456789012
		char date_str[32];

		// TODO: %s may be up to 60 due to leap seconds, but Adobe forces a 
		// maximum of 59
		time_t unix_time;
		time(&unix_time);
		struct tm *calendar_time = gmtime(&unix_time);
		strftime(date_str, sizeof(date_str), "(D:%Y%m%d%H%M%S%z", calendar_time);
		// strftime does not put the ' in the middle of the UTC-difference,
        // so we do this here manually
		date_str[22] = 0x00;
		date_str[21] = date_str[20];
		date_str[20] = date_str[21];
		date_str[19] = '\'';
		strcat(date_str, "')");		

		pdf_obj *mod_date_obj = pdf_new_string(juggler->ctx, 
			juggler->pdf, date_str, strlen(date_str) + 1);
		pdf_dict_puts_drop(juggler->ctx, info, "ModDate", mod_date_obj);
	}
	pdf_drop_obj(juggler->ctx, info);

	return(NoError);
}

ErrorCode juggler_set_meta_data(juggler_t *juggler, meta_data *data)
{
	ErrorCode errorCode;
	pdf_obj *info = pdf_dict_gets(juggler->ctx, 
		pdf_trailer(juggler->ctx, juggler->pdf), "Info");


    // if there is no info-dict, create one!
	if(info == NULL && 
	     (errorCode = juggler_create_document_info(juggler, &info)) != NoError)
	{
			return(errorCode);
	}

	char *keys[] = { "Title", "Author", "Subject", "Keywords", "CreationDate",
					 "ModifyDate", "Producer", "Creator" };
	char *values[] = { data->title, data->author, data->subject, data->keywords,
					 data->creationDate, data->modifyDate, data->producer, 
					 data->creator };
	
	int i;
	for(i = 0; i < sizeof(keys) / sizeof(char *); i++) {
		pdf_dict_dels(juggler->ctx, info, keys[i]);
		if(values[i] != NULL) {
			pdf_obj *content = pdf_new_string(juggler->ctx, juggler->pdf, 
											  values[i], strlen(values[i]) + 1);
			pdf_dict_puts_drop(juggler->ctx, info, keys[i], content);
		}
	}	

	return(NoError);
}
