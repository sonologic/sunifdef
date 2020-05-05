#ifndef SYMBOL_TABLE_H
#define SYMBOL_TABLE_H

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
 
#include "opts.h"
#include "ptr_vector.h"
#include "evaluator.h"
#include <ctype.h>

/*!\ingroup symbol_table_interface
 *\file symbol_table.h
 * This file provides the Symbol Table module interface
 */

/*!\addtogroup symbol_table_interface */
/*@{*/

/*! Get a pointer the symbol with a given index in the symbol table.
 *	\param	ind	Index into the symbol table.
 */
#define SYMBOL(ind)\
	((eval_result_t *)ptr_vector_at(GET_PUBLIC(symbol_table,sym_tab),ind))

/*! Test whether a character can occur in a symbol
 *	\param ch	 The character to test.
 */
#define symchar(ch) \
	(isalnum((unsigned)ch) || (ch) == '_')

/*! Lookup an identifier in the symbol table.
 * \param str	Start of the identifier to match with symbols in the table.
 * \param	end	Either null or a pointer at which the address just past
 *				the end of the identifier will be stored.
 *	\return The index of the symbol in the symbol table, if
			found. Otherwise the value ~N, where N is the
			index at which the unmatched symbol should be
			inserted.
 */
extern int
find_sym(char *str, char ** end);

/*! Add a symbol with specified attributes to the symbol table.
	\param	 definethis	Is this symbol deemed to be defined, or undefined?
	\param		sym	 Null-terminated symbol to be added.

	This function is called to add symbols that are specified
	as defined or undefined.
*/
extern void
add_specified_symbol(bool definethis, char *sym);

/*! Add a symbol without specified attributes to the symbol table.
	\param	  	at The position at which the symbol is to be
				inserted in the symbol table.
	\param	 name	The start of the symbol/
	\param		namelen The length of the symbol.

	This function is called to add symbols that are parsed
	from \c #if expressions and found to be unknown.
*/
extern void
add_unknown_symbol(int at, char const *name, size_t namelen);

/*@}*/

/*!\ingroup symbol_table_interface_state_utils */
/*@{*/
/*! The public state of the Symbol Table module */
PUBLIC_STATE_DEF(symbol_table) {
	ptr_vector_h sym_tab;
		/*!< Handle to the symbol table */
} PUBLIC_STATE_T(symbol_table);
/*@}*/

/*!\addtogroup symbol_table_interface_state_utils */
/*@{*/
IMPORT(symbol_table);
/*@}*/


#endif /* EOF */
