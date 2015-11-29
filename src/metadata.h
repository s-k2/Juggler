/*
  metadata.h - functions to edit the metadata
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

#ifndef _JUGGLER_METADATA_H_
#define _JUGGLER_METADATA_H_

#include "document.h"

typedef struct {
	char *title;
	char *author;
	char *subject;
	char *keywords;
	char *creationDate;
	char *modifyDate;
	char *creator;
	char *producer;
} meta_data;

/* read the metadata of the pdf */
extern ErrorCode juggler_get_meta_data(juggler_t *juggler, meta_data *data);

/* overwrite with new data. NULL values will be preserved */
extern ErrorCode juggler_set_meta_data(juggler_t *juggler, meta_data *data);

#endif /* _JUGGLER_METADATA_H_ */
