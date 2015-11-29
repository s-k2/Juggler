/*
  page-rotate.h - get and set page rotation
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

#ifndef _JUGGLER_PAGE_ROTATE_H_
#define _JUGGLER_PAGE_ROTATE_H_

#include "document.h"

extern ErrorCode juggler_get_page_rotation(juggler_t *juggler, int index, int *rotation);

extern ErrorCode juggler_set_page_rotation(juggler_t *juggler, int index, int rotation);

#endif /* _JUGGLER_PAGE_ROTATE_H_ */
