/*
  error.h - error codes of juggler
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

#ifndef _JUGGLER_ERROR_H_
#define _JUGGLER_ERROR_H_

typedef enum { 
	NoError, 
	ErrorUsage, // function called with wrong parameters
	ErrorNewContext, // ???
	ErrorPasswordProtected, // password-protected files are not supported yet
	ErrorTrailerNoDict, // no trailer in the file -> invalid pdf
	ErrorCatalogNoDict, // no catalog in the file -> invalid pdf
	ErrorCacheObject, // could not cache an object
	ErrorEntryNoObject,
	ErrorNoInfo, 
	NoMemoryError, // out of memory
	NoDocumentInfoExists, // ???
	ErrorNoRoot, 
	ERROR_NO_PAGES, 
	ERROR_INVALID_RANGE,
	ErrorInvalidReference // a reference object seen by the function is invalid
} ErrorCode;

#endif /* _JUGGLER_ERROR_H_ */
