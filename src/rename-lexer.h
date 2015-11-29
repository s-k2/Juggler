/*
  rename-lexer.h - a flex-lexer to rename all names in a content stream
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

/* this header is manually generated and just exposes this function */
int rename_res_in_content_stream(fz_context *in_ctx, fz_stream *input, 
	fz_context *out_ctx, fz_output *output, pdf_obj *rename_dict);
