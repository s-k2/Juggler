/*
  put-content.h - put the contents of one page onto another
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

#ifndef _JUGGLER_PUT_CONTENT_H_
#define _JUGGLER_PUT_CONTENT_H_

#include "document.h"
/*
struct sheet_info
{
	char *content_stream;
    size_t pages_count;
	struct pages
	{
		int page_num;
		int x, y;
		int width, height;
	} pages[];
};
*/
ErrorCode juggler_put_page_contents(juggler_t *juggler);

#endif /* _JUGGLER_PUT_CONTENT_H_ */
