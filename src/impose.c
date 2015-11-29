/*
  impose.c - prepare the impose information
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

#include "impose.h"

#include <stdlib.h>

#define INITIAL_SHEETS_SIZE 8

#define ARRAY_ADJUST_FOR_NEW(buffer, count, avail, sizeof_item, new_items_count) \
	if((avail) == 0) {													\
		(buffer) =														\
			realloc((buffer), (sizeof_item) * ((count) + (new_items_count))); \
																		\
		(avail) = (new_items_count);									\
	}

#define ARRAY_GET_NEW(buffer, count, avail, pos) \
	&(buffer)[(count)];							  \
	if((pos))									  \
		*(pos) = (count);						  \
	(count)++;									  \
	(avail)--;


ErrorCode juggler_impose_start(juggler_impose_t **impose_ptr)
{
	juggler_impose_t *impose = malloc(sizeof(juggler_impose_t));

	impose->sheets_count = 0;
	impose->sheets_avail = INITIAL_SHEETS_SIZE;
	impose->sheets = malloc(sizeof(juggler_imposed_sheet_t) * INITIAL_SHEETS_SIZE);

	*impose_ptr = impose;
	return(NoError);
}

ErrorCode juggler_impose_sheet_add(juggler_impose_t *impose, juggler_sheet_t *sheet, size_t *pos)
{
	ARRAY_ADJUST_FOR_NEW(impose->sheets, impose->sheets_count, 
		impose->sheets_avail, sizeof(juggler_imposed_sheet_t), INITIAL_SHEETS_SIZE);

	juggler_imposed_sheet_t *current = ARRAY_GET_NEW(
		impose->sheets, impose->sheets_count, impose->sheets_avail, pos);

	current->sheet = sheet;
	current->pages_count = 0;
	current->assignments = 0;

	return(NoError);
}

ErrorCode juggler_impose_sheet_get(juggler_impose_t *impose, size_t pos, juggler_imposed_sheet_t **sheet)
{
	*sheet = &impose->sheets[pos];

	return(NoError);
}

int impose_teste_main()
{
	juggler_impose_t *impose;
	juggler_impose_start(&impose);

	size_t pos;
	while(1) {
		juggler_impose_sheet_add(impose, NULL + pos, &pos);

		printf("pos %lu\n", pos);
		if(pos > 12)
			break;
	}

	while(pos > 0) {
		juggler_imposed_sheet_t *sheet;
		juggler_impose_sheet_get(impose, pos--, &sheet);

			printf("Item: %p\n", sheet->sheet);
	}

	return(0);
}
