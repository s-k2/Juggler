/*
  dump.h - dump an object
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

#ifndef _JUGGLER_DUMP_H_
#define _JUGGLER_DUMP_H_

#include "document.h"

typedef enum { 
	JUGGLER_DUMP_TYPE_NO_FORMAT, 
	JUGGLER_DUMP_TYPE_REF, 
	JUGGLER_DUMP_TYPE_STREAM 
} juggler_dump_type_e;

typedef struct _juggler_dump_t {
	struct _juggler_dump_t *next;
	juggler_dump_type_e type;
	size_t used; // just the characters, not the null-byte!
	char content[];
} juggler_dump_t;

extern ErrorCode juggler_dump_object(juggler_t *juggler,
	int num, int gen, juggler_dump_t **dump);

extern ErrorCode juggler_dump_delete(juggler_dump_t *dump);

#endif /* _JUGGLER_DUMP_H_ */
