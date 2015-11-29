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
 
%option nodefault
%option 8bit
%option noyywrap
%option reentrant
%option prefix="juggler_rename_res_" 
%option extra-type="struct rename_res_extra *"
%option batch
%x HEX_STR
%x LIT_STR
%x DICT
%x INDICT_HEX_STR
%x INDICT_LIT_STR
%x INLINE_IMAGE_HEADER
%x INLINE_IMAGE_DATA
%x MARKED_CONTENT
%x TEST_STATE
%{
#include <mupdf/fitz.h>
#include <mupdf/pdf.h>

struct dict_level
{
    int array_level; // >0 means inside array
    char is_key;
};

struct rename_res_extra
{
	fz_context *in_ctx;
    fz_stream *input;
    fz_context *out_ctx; /* also the context of rename_dict */
    fz_output *output;

    pdf_obj *rename_dict;
    int lit_str_brackets_count;

    struct dict_level *dict_levels;
    size_t dict_levels_used;
    size_t dict_levels_free;
};

static size_t juggler_rename_res_read(struct rename_res_extra *extra, char *buffer, size_t max_size);
#undef YY_INPUT
#define YY_INPUT(buffer, read_bytes, max_size) \
{ \
    read_bytes = juggler_rename_res_read(yyextra, (buffer), (max_size)); \
}

static void enter_dict(struct rename_res_extra *extra);
static size_t leave_dict(struct rename_res_extra *extra);
static void enter_array(struct rename_res_extra *extra);
static void leave_array(struct rename_res_extra *extra);
static int is_key(struct rename_res_extra *extra);
static void after_name(struct rename_res_extra *extra);
static void after_value(struct rename_res_extra *extra);

static void rename_res(struct rename_res_extra *extra, char *old_name);

%}

%%


                                     /* all inline image things (put here, because of its precedence over the "B"-operator */
                                     /* TODO: Verify that image-inline-dicts may not contain any resource-references... */
"BI"                                 { fz_printf(yyextra->out_ctx, yyextra->output, "%s", yytext); BEGIN INLINE_IMAGE_HEADER; }
<INLINE_IMAGE_HEADER>"ID"            { fz_printf(yyextra->out_ctx, yyextra->output, "%s", yytext); BEGIN INLINE_IMAGE_DATA; }
<INLINE_IMAGE_HEADER>.|\n            { fz_printf(yyextra->out_ctx, yyextra->output, "%c", *yytext); }
<INLINE_IMAGE_DATA>(\n|\r|\r\n)EI    { fz_printf(yyextra->out_ctx, yyextra->output, "%s", yytext); BEGIN INITIAL; yylineno++; }
<INLINE_IMAGE_DATA>\n                { fz_printf(yyextra->out_ctx, yyextra->output, "\n"); yylineno++; }
<INLINE_IMAGE_DATA>.|\n              { fz_printf(yyextra->out_ctx, yyextra->output, "%c", *yytext); }



"B*"    |
"b*"    |
"B"     |
"b"     |
"BDC"   |
"BMC"   |
"BT"    |
"BX"    |
"c"     |
"cm"    |
"CS"    |
"cs"    |
"d"     |
"d0"    |
"d1"    |
"Do"    |
"DP"    |
"EMC"   |
"ET"    |
"EX"    |
"f*"    |
"F"     |
"f"     |
"G"     |
"g"     |
"gs"    |
"h"     |
"i"     |
"j"     |
"J"     |
"K"     |
"k"     |
"l"     |
"M"     |
"m"     |
"MP"    |
"n"     |
"q"     |
"Q"     |
"re"    |
"RG"    |
"rg"    |
"ri"    |
"S"     |
"s"     |
"SC"    |
"sc"    |
"SCN"   |
"scn"   |
"sh"    |
"T*"    |
"Tc"    |
"TD"    |
"Td"    |
"Tf"    |
"TJ"    |
"Tj"    |
"TL"    |
"Tm"    |
"Tr"    |
"Ts"    |
"Tw"    |
"Tz"    |
"v"     |
"W*"    |
"w"     |
"W"     |
"y"     |
"'"     |
"\""    { fz_printf(yyextra->out_ctx, yyextra->output, "%s", yytext); }

"<<"                   { BEGIN DICT; enter_dict(yyextra); fz_printf(yyextra->out_ctx, yyextra->output, "<<"); } // must always be in front of HEX_STR
<DICT>"<<"             { enter_dict(yyextra); fz_printf(yyextra->out_ctx, yyextra->output, "<<"); }
<DICT>\/[^\0\t\n\r\f ()<>\[\]{}/%]+ { if(!is_key(yyextra)) rename_res(yyextra, yytext); else fz_printf(yyextra->out_ctx, yyextra->output, "%s", yytext); after_name(yyextra); }

<DICT>"null"                 |

<DICT>"true"                 |
<DICT>"false"                |

<DICT>-*[0-9]+               |
<DICT>-*[0-9]*(\.[0-9]+)*    { after_value(yyextra); fz_printf(yyextra->out_ctx, yyextra->output, "%s", yytext); }

<DICT>"("                    { yyextra->lit_str_brackets_count = 1; fz_printf(yyextra->out_ctx, yyextra->output, "("); BEGIN INDICT_LIT_STR; }
<INDICT_LIT_STR>"("          { yyextra->lit_str_brackets_count++; fz_printf(yyextra->out_ctx, yyextra->output, "("); }
<INDICT_LIT_STR>")"          { if(--yyextra->lit_str_brackets_count == 0) { BEGIN DICT; after_value(yyextra); } fz_printf(yyextra->out_ctx, yyextra->output, ")");}
<INDICT_LIT_STR>"\\n"        |
<INDICT_LIT_STR>"\\r"        |
<INDICT_LIT_STR>"\\t"        |
<INDICT_LIT_STR>"\\b"        |
<INDICT_LIT_STR>"\\f"        |
<INDICT_LIT_STR>"\\("        |
<INDICT_LIT_STR>"\\)"        |
<INDICT_LIT_STR>"\\\\"       |
<INDICT_LIT_STR>\\[0-7]{3}   |
<INDICT_LIT_STR>\\[0-7]{2}   |
<INDICT_LIT_STR>\\[0-7]{1}   { fz_printf(yyextra->out_ctx, yyextra->output, "%s", yytext); }
<INDICT_LIT_STR>\r\n         |
<INDICT_LIT_STR>\n           |
<INDICT_LIT_STR>\r           { fz_printf(yyextra->out_ctx, yyextra->output, "%s", yytext); yylineno++; }
<INDICT_LIT_STR>.            { fz_printf(yyextra->out_ctx, yyextra->output, "%c", *yytext); }

<DICT>"<"                    { BEGIN INDICT_HEX_STR; fz_printf(yyextra->out_ctx, yyextra->output, "<"); }
<INDICT_HEX_STR>[ \v\t\n\r]  { fz_printf(yyextra->out_ctx, yyextra->output, "%c", *yytext);} // ignore whitespaces in hex-strings
<INDICT_HEX_STR>[0-9A-Fa-f]  { fz_printf(yyextra->out_ctx, yyextra->output, "%c", *yytext); } // todo: concenate all nibbles
<INDICT_HEX_STR>">"          { BEGIN DICT; after_value(yyextra); fz_printf(yyextra->out_ctx, yyextra->output, ">"); }
<INDICT_HEX_STR>.            { fprintf(stderr, "UNKNOWN-INDICT-INHEXSTR '%c'(0x%X) in line %d - ", *yytext, *yytext, yylineno); }

<DICT>"["                    { fz_printf(yyextra->out_ctx, yyextra->output, "["); enter_array(yyextra); }
<DICT>"]"                    { fz_printf(yyextra->out_ctx, yyextra->output, "]"); leave_array(yyextra); }

% // inline dicts are not allowed to contain any references (does PDF-Reference tell anything about this???)

<DICT>[ \v\t\r\f]     { fz_printf(yyextra->out_ctx, yyextra->output, "%c", *yytext); } // ignore whitespaces in dict
<DICT>\n              { fz_printf(yyextra->out_ctx, yyextra->output, "%c", *yytext); yylineno++; }

<DICT>">>"            { if(leave_dict(yyextra) == 0) BEGIN INITIAL; fz_printf(yyextra->out_ctx, yyextra->output, ">>"); }
<DICT>.               { fprintf(stderr, "UNKNOWN INDICT '%c'(0x%X) in line %d - ", *yytext, *yytext, yylineno); }

"("                   { yyextra->lit_str_brackets_count = 1; BEGIN LIT_STR; fz_printf(yyextra->out_ctx, yyextra->output, "("); }
<LIT_STR>"("          { yyextra->lit_str_brackets_count++; fz_printf(yyextra->out_ctx, yyextra->output, "("); }
<LIT_STR>")"          { if(--yyextra->lit_str_brackets_count == 0) BEGIN INITIAL; fz_printf(yyextra->out_ctx, yyextra->output, ")");}
<LIT_STR>"\\n"        |
<LIT_STR>"\\r"        |
<LIT_STR>"\\t"        |
<LIT_STR>"\\b"        |
<LIT_STR>"\\f"        |
<LIT_STR>"\\("        |
<LIT_STR>"\\)"        |
<LIT_STR>"\\\\"       |
<LIT_STR>\\[0-7]{3}   |
<LIT_STR>\\[0-7]{2}   |
<LIT_STR>\\[0-7]{1}   { fz_printf(yyextra->out_ctx, yyextra->output, "%s", yytext); }
<LIT_STR>\r\n         |
<LIT_STR>\n           |
<LIT_STR>\r           { fz_printf(yyextra->out_ctx, yyextra->output, "%s", yytext); yylineno++; }
<LIT_STR>.            { fz_printf(yyextra->out_ctx, yyextra->output, "%c", *yytext); }

"<"                   { BEGIN HEX_STR; fz_printf(yyextra->out_ctx, yyextra->output, "<"); }
<HEX_STR>[ \v\t\n\r]  { fz_printf(yyextra->out_ctx, yyextra->output, "%c", *yytext); } // ignore whitespaces in hex-strings
<HEX_STR>[0-9A-Fa-f]  { fz_printf(yyextra->out_ctx, yyextra->output, "%c", *yytext); } // todo: concenate all nibbles
<HEX_STR>">"          { BEGIN INITIAL; fz_printf(yyextra->out_ctx, yyextra->output, ">"); }
<HEX_STR>.            { fprintf(stderr, "UNKNOWN '%c'(0x%X) in line %d - ", *yytext, *yytext, yylineno); }

"["                   { fz_printf(yyextra->out_ctx, yyextra->output, "["); }
"]"                   { fz_printf(yyextra->out_ctx, yyextra->output, "]"); }


\/[^\0\t\n\r\f ()<>\[\]{}/%]+ { rename_res(yyextra, yytext); }
-*[0-9]*(\.[0-9]+)*   { fz_printf(yyextra->out_ctx, yyextra->output, "%s", yytext); }

"\n"                  { fz_printf(yyextra->out_ctx, yyextra->output, "\n"); yylineno++; }
[ \v\t\r\f]           { fz_printf(yyextra->out_ctx, yyextra->output, "%c", *yytext); }
.                     { fprintf(stderr, "UNKNOWN '%c'(0x%X) in line %d - ", *yytext, *yytext, yylineno); }

%%

int rename_res_in_content_stream(fz_context *in_ctx, fz_stream *input, fz_context *out_ctx, fz_output *output, pdf_obj *rename_dict)
{
    yyscan_t scanner;
    
    struct rename_res_extra *extra = 
        malloc(sizeof(struct rename_res_extra));
    memset(extra, 0, sizeof(struct rename_res_extra));

	extra->in_ctx = in_ctx;
    extra->input = input;
    extra->out_ctx = out_ctx;
    extra->output = output;
    extra->rename_dict = rename_dict;

    if(juggler_rename_res_lex_init_extra(extra, &scanner))
        return(1);

    juggler_rename_res_lex(scanner);

    yylex_destroy(scanner);
    
    free(extra->dict_levels);
    free(extra);

    return(0);
}

static void rename_res(struct rename_res_extra *extra, char *old_name)
{
	pdf_obj *new_name = pdf_dict_gets(extra->out_ctx, extra->rename_dict, old_name + 1);
	if(!pdf_is_name(extra->out_ctx, new_name)) {
		printf("Rename of %s failed! It will be preserved\n", old_name);
		fz_printf(extra->out_ctx, extra->output, "%s", old_name);
	} else {
		//printf("Renaming %s to %s\n", old_name, pdf_to_name(new_name));
		fz_printf(extra->out_ctx, extra->output, "/%s", pdf_to_name(extra->out_ctx, new_name));
	}
}

static size_t juggler_rename_res_read(struct rename_res_extra *extra, char *buffer, size_t max_size)
{
    return(fz_read(extra->in_ctx, extra->input, (unsigned char *) buffer, max_size));
}

#define LEVEL_BLOCK_SIZE 16
static void enter_dict(struct rename_res_extra *extra)
{
    if(extra->dict_levels_free < 1) {
        extra->dict_levels = realloc(extra->dict_levels, 
			sizeof(struct dict_level) * (extra->dict_levels_used + LEVEL_BLOCK_SIZE));
		extra->dict_levels_free += LEVEL_BLOCK_SIZE;
	}
	extra->dict_levels_used++;
	extra->dict_levels[extra->dict_levels_used].is_key = 1;
	extra->dict_levels[extra->dict_levels_used].array_level = 0;
}

static size_t leave_dict(struct rename_res_extra *extra)
{
    extra->dict_levels_used--;
	extra->dict_levels_free++;
    after_value(extra);

    return(extra->dict_levels_used);
}

static void enter_array(struct rename_res_extra *extra)
{
	extra->dict_levels[extra->dict_levels_used].array_level++;
}

static void leave_array(struct rename_res_extra *extra)
{
	extra->dict_levels[extra->dict_levels_used].array_level--;
    if(extra->dict_levels[extra->dict_levels_used].array_level == 0)
        after_value(extra);
}

static int is_key(struct rename_res_extra *extra)
{
    return(extra->dict_levels[extra->dict_levels_used].is_key);
}

static void after_name(struct rename_res_extra *extra)
{
    if(extra->dict_levels[extra->dict_levels_used].array_level < 1)
        extra->dict_levels[extra->dict_levels_used].is_key = 
            !extra->dict_levels[extra->dict_levels_used].is_key;
}

static void after_value(struct rename_res_extra *extra)
{
    if(extra->dict_levels[extra->dict_levels_used].array_level < 1)
        extra->dict_levels[extra->dict_levels_used].is_key = 
            !extra->dict_levels[extra->dict_levels_used].is_key;
}