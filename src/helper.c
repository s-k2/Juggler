/*
  helper.c - all functions that don't have another place ;-)
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

#include "helper.h"

ErrorCode juggler_get_info_obj_num(juggler_t *juggler, int *num, int *gen)
{
	pdf_obj *info = pdf_dict_gets(juggler->ctx, 
		pdf_trailer(juggler->ctx, juggler->pdf), "Info");

    // according to the pdf reference (p. 97) the value of Info 
	// always has to be an indirect reference
	if(info == NULL || !pdf_is_indirect(juggler->ctx, info))
		return(NoDocumentInfoExists);

	*num = pdf_to_num(juggler->ctx, info);
	*gen = pdf_to_gen(juggler->ctx, info);

	return(NoError);
}

ErrorCode juggler_get_root_obj_num(juggler_t *juggler, int *num, int *gen)
{
	pdf_obj *root = pdf_dict_gets(juggler->ctx, 
		pdf_trailer(juggler->ctx, juggler->pdf), "Root");

    // according to the pdf reference (p. 97) the value of Root 
	// always has to be an indirect reference
	if(root == NULL || !pdf_is_indirect(juggler->ctx, root))
		return(NoDocumentInfoExists);

	*num = pdf_to_num(juggler->ctx, root);
	*gen = pdf_to_gen(juggler->ctx, root);

	return(NoError);

}
