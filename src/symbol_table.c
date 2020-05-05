/***************************************************************************
 *   Copyright (C) 2004, 2006 Symbian Software Ltd.                        *
 *   All rights reserved.                                                  *
 *   Copyright (C) 2007, 2008 Mike Kinghan, imk@strudl.org                 *
 *   All rights reserved.                                                  *
 *                                                                         *
 *   Contributed originally by Mike Kinghan, imk@strudl.org                *
 *                                                                         *
 *   Redistribution and use in source and binary forms, with or without    *
 *   modification, are permitted provided that the following conditions    *
 *   are met:                                                              *
 *                                                                         *
 *   Redistributions of source code must retain the above copyright        *
 *   notice, this list of conditions and the following disclaimer.         *
 *                                                                         *
 *   Redistributions in binary form must reproduce the above copyright     *
 *   notice, this list of conditions and the following disclaimer in the   *
 *   documentation and/or other materials provided with the distribution.  *
 *                                                                         *
 *   Neither the name of Symbian Software Ltd. nor the names of its        *
 *   contributors may be used to endorse or promote products derived from  *
 *   this software without specific prior written permission.              *
 *                                                                         *
 *   THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS   *
 *   "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT     *
 *   LIMITED TO, THE IMPLIED WARRANTIES OF  MERCHANTABILITY AND FITNESS    *
 *   FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE        *
 *   COPYRIGHT OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT,   *
 *   INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING,  *
 *   BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS *
 *   OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED    *
 *   AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY,*
 *   OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF *
 *   THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH  *
 *   DAMAGE.                                                               *
 *                                                                         *
 ***************************************************************************/
#include "symbol_table.h"
#include "chew.h"
#include "args.h"
#include "report.h"
#include <stddef.h>
#include <stdio.h>

/*!\ingroup symbol_table_module, symbol_table_interface, symbol_table_internals
 *\file symbol_table.c
 * This file implements the Symbol Table module
 */

/*!\ingroup symbol_table_internals_state_utils */
/*@{*/
NO_PRIVATE_STATE(symbol_table);
/*@}*/

/*!\addtogroup symbol_table_internals_state_utils */
/*@{*/
IMPLEMENT(symbol_table,USER_INITABLE);

DEFINE_USER_INIT(symbol_table)(STATE_T(symbol_table) * sym_state)
{
	sym_state->sym_tab = ptr_vector_new();
}

DEFINE_USER_FINIS(symbol_table)(STATE_T(symbol_table) * sym_state)
{
	ptr_vector_dispose(&(sym_state->sym_tab));
}
/*@}*/

/* API ***************************************************************/

int
find_sym(char *str, char ** end)
{
	char *cp;
	const char *name = NULL;
	const char *def = NULL;
	size_t symind;
	int comp;
	size_t symbols = ptr_vector_count(GET_STATE(symbol_table,sym_tab));

	cp = chew_sym(str);
	if (end) {
		*end = cp;
	}
	if (cp == str) {
		bail(	GRIPE_NOT_IDENTIFIER,
				"Identifier needed instead of \"%s\"",
				str);
	}
	for (comp = 1, symind = 0; symind < symbols; ++symind) {
		ptrdiff_t len;
		eval_result_t * pos = SYMBOL(symind);
		name = pos->sym_name;
		def = pos->sym_def;
		len = cp - str;
		comp = strncmp(str,name,len);
		if (comp <= 0) {
			if (comp == 0) { /* Initial match */
				if (name[len] != '\0') {
					/* Symbol in table is longer than `str' */
					comp = -1;
				}
				/* Otherwise full match */
			}
			break;
		}
	}
	if (comp == 0) {
		debug(DBG_18, name, (def ? def : ""));
		return (int)symind;
	}
	return (int)~symind;
}

void
add_specified_symbol(bool definethis, char *sym)
{
	char *val;
	eval_result_t *symbol;
	int ind = find_sym(sym,&val);
	if (ind < 0) {
		symbol = allocate(sizeof(eval_result_t));
		symbol->sym_name = allocate((val - sym) + 1);
		memcpy(symbol->sym_name,sym,val-sym);
		if (definethis) { /* -D */
			if (*val == '=') {
				size_t val_len = strlen(val + 1);
				symbol->sym_def = allocate(val_len + 1);
				memcpy(symbol->sym_def,val + 1,val_len);
			}
			else if (*val == '\0') {
				symbol->sym_def = "";
			}
			else {	/* Invalid */
				bail(GRIPE_GARBAGE_ARG,"Garbage in argument \"%s\"",sym - 2);
			}
		}
		else {	/* -U */
			if (*val != '\0') { /* Invalid */
				bail(GRIPE_GARBAGE_ARG,"Garbage in argument \"%s\"",sym - 2);
			}
			symbol->sym_def = NULL;
		}
		ptr_vector_insert(GET_STATE(symbol_table,sym_tab),~ind,symbol);
	}
	else {
 		/* Duplicate arg */
		bad_args_error(sym,val,ind);
	}
}

extern void
add_unknown_symbol(int at, char const *name, size_t namelen)
{
	eval_result_t *elem;
	elem = allocate(sizeof(eval_result_t));
	elem->sym_name = allocate(namelen + 1);
	memcpy(elem->sym_name,name,namelen);
	ptr_vector_insert(GET_STATE(symbol_table,sym_tab),at,elem);
}


/* EOF */
