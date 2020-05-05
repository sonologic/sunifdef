#ifndef CHEW_H
#define CHEW_H

/***************************************************************************
 *   Copyright (C) 2004, 2006 Symbian Software Ltd.                        *
 *   All rights reserved.                                                  *
 *   Copyright (C) 2002, 2003 Tony Finch <dot@dotat.at>.                   *
 *   All rights reserved.                                                  *
 *   Copyright (C) 1985, 1993 The Regents of the University of California. *
 *   All rights reserved.                                                  *
 *   Copyright (C) 2007, 2008 Mike Kinghan, imk@strudl.org                 *
 *   All rights reserved.                                                  *
 *                                                                         *
 *   Contributed by Mike Kinghan, imk@strudl.org, derived from the code    *
 *   of Tony Finch                                                         *
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

/*!\ingroup chew_module, chew_interface
 *\file chew.h
 * This file provides the Chew module interface
 */

/*! \addtogroup chew_interface */
/*@{*/

/*! Enumeration of comment states */
typedef enum comment_state {
	/*! Outside a comment */
	NO_COMMENT = false,
	/*! In a C-style comment*/
	C_COMMENT,
	/*! In a C++-style comment, i.e. // and end of line */
	CXX_COMMENT,
	/*! Just after forward-slash + back-slash + newline */
	STARTING_COMMENT,
	/*! Just after star + back-slash + newline in a C comment */
	FINISHING_COMMENT,
	/*! In the text of an #error directive or the definiens of #define */
	PSEUDO_COMMENT
} comment_state_t;

/*! Enumeration of states of a source line */
typedef enum {
	/*! Only whitespace and comments so far on the line */
	LS_NEUTER,
	/*! Only whitespace, comments, and a \c # so far on the line */
	LS_DIRECTIVE,
	/*! Source code found on the line (text other than whitespace,
		comments, or a <tt>#</tt>-directive) */
	LS_CODE
} line_state_t;

/*! Say whether a <tt>char *</tt> address a line end,
	either Unix type or Windows type.
*/
#define EOL(cp) ((cp)[0] == '\n' || ((cp)[0] == '\r' && (cp)[1] == '\n'))

/*! Consume whitespace, comments, quotation or the text of an #error directive
 *  in the source text and stop at the first character beyond all these.
 *
 *	\param	cp	The current text pointer
 *	\return A pointer to the next input character that is not to be chewed.
 *
 *	The function replenishs the line buffer and copes with line-continuations.
 * It maintains state with respect to all syntactically significant categories
 * of context.
 */
extern char *
chew_on(char *cp);

/*! Consume an identifier in the source text and stop at the next character.
 *
 *	\param	cp	The current text pointer
 *	\return A pointer to The next input character that cannot be part of
 *	an identifier.
 */
extern char *
chew_sym(char *cp);

/*! Consume a string until whitespace is found.
 *
 *	\param	cp	The current text pointer
 *	\return A pointer to The next whitespace input character.
 */
extern char *
chew_str(char *cp);

/*! Is the text pointer within quotation? */
extern bool
in_quotation(void);

/*! Reinitialise the line state and comment state */
extern void
chew_toplevel(void);


/*@}*/

/*! \ingroup chew_interface_state_utils */
/*@{*/

/*! The public state of the Chew module */
PUBLIC_STATE_DEF(chew) {
	enum comment_state comment_state;
		/*!< The current comment state */
	line_state_t line_state;
		/*!< The current line state */
	size_t	last_quote_start_line;
		/*!< Line number of the most recent open-quote */
	size_t	 last_comment_start_line;
		/*!< Line number of most recent open-comment */
} PUBLIC_STATE_T(chew);
/*@}*/

/*! \addtogroup chew_interface_state_utils */
/*@{*/
IMPORT(chew);
/*@}*/

#endif /* EOF */
