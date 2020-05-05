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
#include "line_edit.h"
#include "io.h"
#include "report.h"
#include "line_despatch.h"
#include "symbol_table.h"
#include "string_util.h"
#include <string.h>

/*!\ingroup line_edit_module, line_edit_interface, line_edit_internals
 *\file line_edit.c
 * This file implements the Line Edit module
 */

/*! \addtogroup line_edit_internals */
/*@{*/


/*!	Macro function that marks a character block as logically deleted in the
 *	course of simplifying a directive, by setting all chars with a deletion
 *	placeholder
 *	\param	start	The start of the text to be deleted.
 *	\param	end		Just past the text to be deleted.
 */
#define DELETE_CHARS(start,end) memset(start,DELETEABLE_CHR,end - start)


/*! Replace text at the end of the line-buffer
 *	\param	where	The start of the terminal segment of the line-buffer
 *					to replace
 *	\param	what	The start of the null-terminated string to replace
 *					the terminal segement at \c where.
 *
 *	The function does the editing work for \c keywordedit(). It ensures
 *	that there is sufficient capacity in the line-buffer to contain the
 *	replacement, extending the buffer if necessary.
 *
 *	If where contains more lines than what, what will be extended with
 *	newlines to match the number of lines in where.
 */
static void
tail_edit(char *where, const char *what)
{
	size_t where_lines = count_lines(where);
	size_t what_lines = count_lines(what);
	char *what_extended = allocate(strlen(what) + what_lines - 1);
	strcpy(what_extended, what);
	for(size_t i=0; i<(where_lines-what_lines); i++) {
		strcat(what_extended, "\n");
	}
	assert(GET_PUBLIC(io,line_start) <= where &&
			where < GET_PUBLIC(io,line_end));
	ensure_buf(strlen(what_extended));
	strcpy(where,what_extended);
	release((void**)(&what_extended));
}

/*@}*/

/* API *********************************************************************/

void
keywordedit(const char *replacement)
{
	tail_edit(GET_PUBLIC(line_edit,keyword),replacement);
	++SET_PUBLIC(line_despatch,lines_changed);
	print();
}


void
delete_paren(char *lparen, char *rparen)
{
#if 0
	if (*lparen == '(' && *rparen == ')' ) {
		if ((lparen[-1] == ' ' ||
			lparen[-1] == '(' ||
			lparen[1] == ' ' ||
			lparen[1] == '(' )
			 &&
			(rparen[-1] == ' ' ||
			rparen[-1] == ')' ||
			rparen[1] == ' ' ||
			rparen[1] == ')' ||
			rparen[1] == '\n'))

		{
			*lparen = DELETEABLE_LPAREN;
			*rparen = DELETEABLE_RPAREN;
			SET_PUBLIC(line_edit,simplification_state) |= BRACKETS_PRUNED;
		}
	}
#endif
	if (*lparen == '(' && *rparen == ')' ) {
		if ((!symchar(lparen[-1]) || !symchar(lparen[1]))
			 &&
			(!symchar(rparen[-1]) || !symchar(rparen[1])))
		{
			*lparen = DELETEABLE_LPAREN;
			*rparen = DELETEABLE_RPAREN;
			SET_PUBLIC(line_edit,simplification_state) |= BRACKETS_PRUNED;
		}
	}

}

void
restore_paren(void)
{
	char *buf = GET_PUBLIC(io,line_start);
	for (	;*buf; ++buf) {
		if (*buf == DELETEABLE_LPAREN) {
			*buf = '(';
		}
		else if (*buf == DELETEABLE_RPAREN) {
			*buf = ')';
		}
	};
}

void
cut_text(char *start, char *end)
{
	 /* Set up for printing a line with deleted chunks */
	SET_PUBLIC(line_edit,simplification_state) |= OPS_CUT;
	DELETE_CHARS(start,end);
	/* Never delete the terminal newline in directive */
	if (!*end) {
		end[-1] = '\n';
	}
}

/*!\ingroup line_edit_internals_state_utils */
/*@{*/
/*! The Line Edit module has no private state */
NO_PRIVATE_STATE(line_edit);
/*@}*/

/*!\addtogroup line_edit_internals_state_utils */
/*@{*/
IMPLEMENT(line_edit,ZERO_INITABLE);
/*@}*/


/* EOF */
