/***************************************************************************
 *   Copyright (C) 2004, 2006 Symbian Software Ltd.                        *
 *   All rights reserved.
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

#include "chew.h"
#include "args.h"
#include "io.h"
#include "if_control.h"
#include "report.h"
#include "symbol_table.h"

/*!\ingroup chew_module, chew_interface, chew_internals
 *\file chew.c
 * This file implements the Chew module
 */

/*! \ingroup chew_internals_state_utils */
/*@{*/

/*! The global state of the Chew module */
STATE_DEF(chew) {
	/*! The public state of the module */
	INCLUDE_PUBLIC(chew);
	bool		escape;	/*!< Last char read was escape? */
	bool		in_double_quote; /*!< Are we reading within double quotes? */
	bool		in_single_quote; /*!< Are we reading within single quotes? */
} STATE_T(chew);

/*@}*/

/*! \addtogroup chew_internals_state_utils */
/*@{*/
IMPLEMENT(chew,ZERO_INITABLE)
/*@}*/

/*! \ingroup chew_internals */

/*! Consume line-continuations at the input text pointer.

	\param	cp	The current text pointer
	\return The next character of input following any sequence of 1 or more
	line-continuations, i.e. "\\n". If there is no such sequence \e cp
	is returned unchanged.

	If any line-continuations are found they are discarded from the
	line-buffer and the line is extended with each continuation.

 */
static char *
chew_continuation(char *cp)
{
	if (cp[0] == '\\' && EOL(cp + 1)) {
		size_t gap;
		size_t after_gap;
		char *gap_start = cp;
		do {
			cp = read_more(cp);
			++cp;
			if (*cp == '\r') {
				++cp;
			}
			if (*cp == '\n') {
				++cp;
			}
		}
		while (cp[0] == '\\' && EOL(cp + 1));
		gap = cp - gap_start;
		after_gap = strlen(cp);
		memmove(gap_start,cp,after_gap);
		gap_start[after_gap] = '\0';
		cp -= gap;
	}
	return cp;
}


/* API ***************************************************************/

bool
in_quotation(void)
{
	return GET_STATE(chew,in_single_quote) || GET_STATE(chew,in_double_quote);
}

char *
chew_sym(char *cp)
{
	if (isalpha(*cp) || *cp == '_') {
		for (	;symchar(*cp); ++cp, cp = chew_continuation(cp)){}
	}
	return (cp);
}

char *
chew_str(char *cp)
{
	for (	;isgraph(*cp); ++cp, cp = chew_continuation(cp)){}
	return (cp);
}


char *
chew_on(char *cp)
{
	if (GET_PUBLIC(args,plaintext)) {
		for (; isspace((unsigned char)*cp); ++cp) {
			if (EOL(cp)) {
				SET_PUBLIC(chew,line_state) = LS_NEUTER;
			}
		}
		return (cp);
	}
	while (*cp != '\0') {
		if (cp[0] == '\\') {	/* Toggle escape state */
			SET_STATE(chew,escape) = !GET_STATE(chew,escape);
			++cp;
			continue;
		}
		if (cp[0] == '"') {
			if (!GET_STATE(chew,escape) && !GET_STATE(chew,in_single_quote)) {
				/* 	Can enter a double-quote state if we are not escaped and
					not already in a single quote */
				if ((SET_STATE(chew,in_double_quote) =
						!GET_STATE(chew,in_double_quote)) == true) {
					SET_PUBLIC(chew,last_quote_start_line) =
						GET_PUBLIC(io,line_num);
				}
			}
			++cp;
		}
		else if (cp[0] == '\'') {
				/* 	Can enter a single-quote state if we are not escaped and
					not already in a double quote */
			if (!GET_STATE(chew,escape) && !GET_STATE(chew,in_double_quote)) {
				if ((SET_STATE(chew,in_single_quote) =
					!GET_STATE(chew,in_single_quote)) == true) {
					SET_PUBLIC(chew,last_quote_start_line) =
						GET_PUBLIC(io,line_num);
				}
			}
			++cp;
		}
		else if (EOL(cp)) { /* Newline */
			if (GET_STATE(chew,escape)) {
				/* Extend line buffer following line-continuation */
				cp = read_more(cp);
				/* don't reset to LS_NEUTER after a line continuation */
			}
			else {	/* Plain newline */
				SET_PUBLIC(chew,line_state) = LS_NEUTER;
				if (GET_PUBLIC(chew,comment_state) == CXX_COMMENT ||
					 GET_PUBLIC(chew,comment_state) == PSEUDO_COMMENT) {
					/* Newline terminates C++ comment */
					SET_PUBLIC(chew,comment_state) = NO_COMMENT;
					SET_STATE(chew,in_double_quote) =
						SET_STATE(chew,in_single_quote) = false;
				}
				else if (GET_PUBLIC(chew,comment_state) == C_COMMENT) {
					SET_STATE(chew,in_double_quote) =
						SET_STATE(chew,in_single_quote) = false;
					++SET_PUBLIC(io,extension_lines);
					cp = read_more(cp);
				}
				else if (GET_STATE(chew,in_double_quote) ||
					GET_STATE(chew,in_single_quote)) {
					/* Dangling quotation is not in comment. Error */
					parse_error(GRIPE_NEWLINE_IN_QUOTE,
						"Newline within quotation");
				}
			}
			if (*cp == '\r') {
				/* Detected Windows line-end in Unix environment */
				++cp;
			}
			++cp;
		}
		else if (cp[0] == ' ' || cp[0] == '\t') { /* Some whitespace */
			++cp;
		}
		/* Could be at the start or end of a comment */
		else if (GET_PUBLIC(chew,comment_state) == NO_COMMENT ||
			GET_PUBLIC(chew,comment_state) == PSEUDO_COMMENT) {
			/* Not in C or C++ comment */
			/* Check for start of comment */
			if (!GET_STATE(chew,in_double_quote) &&
				!GET_STATE(chew,in_single_quote)) {
				/* Don't let comments start within quotation */
				if (!strncmp(cp, "/\\\n",3)) {
					SET_PUBLIC(chew,comment_state) = STARTING_COMMENT;
					SET_PUBLIC(chew,last_comment_start_line) =
						GET_PUBLIC(io,line_num);
					cp += 3;
				}
				if (!strncmp(cp, "/\\\r\n",4)) {
					SET_PUBLIC(chew,comment_state) = STARTING_COMMENT;
					SET_PUBLIC(chew,last_comment_start_line) =
						GET_PUBLIC(io,line_num);
					cp += 4;
				}
				else if (cp[0] == '/' && cp[1] == '*') {
					SET_PUBLIC(chew,comment_state) = C_COMMENT;
					SET_PUBLIC(chew,last_comment_start_line) =
						GET_PUBLIC(io,line_num);
					cp += 2;
				}
				else if (cp[0] == '/' && cp[1] == cp[0]) {
					SET_PUBLIC(chew,comment_state) = CXX_COMMENT;
					SET_PUBLIC(chew,last_comment_start_line) =
						GET_PUBLIC(io,line_num);
					cp += 2;
				}
				else if (GET_PUBLIC(chew,comment_state) == PSEUDO_COMMENT) {
					/* Inside #error text. Truck on */
					++cp;
				}
				else { /* No comment starting. We're done */
					SET_STATE(chew,escape) = false;
					break;
				}
			}
			else {	/* We're inside quotation. Truck on */
				++cp;
			}
		}
		/* We are in a comment state. Check for end of comment */
		/* No need to test for end of C++ comment 'cos newline case
			covers it */
		else if (GET_PUBLIC(chew,comment_state) == C_COMMENT) {
			if (!strncmp(cp, "*\\\n",3)) {
				SET_PUBLIC(chew,comment_state) = FINISHING_COMMENT;
				cp += 3;
			}
			if (!strncmp(cp, "*\\\r\n",4)) {
				SET_PUBLIC(chew,comment_state) = FINISHING_COMMENT;
				cp += 4;
			}
			else if (cp[0] == '*' && cp[1] == '/') {
				SET_PUBLIC(chew,comment_state) = NO_COMMENT;
				SET_STATE(chew,in_double_quote) =
					SET_STATE(chew,in_single_quote) = false;
				cp += 2;
			}
			else {
				++cp;
			}
		}
		else if (GET_PUBLIC(chew,comment_state) == STARTING_COMMENT) {
			if (*cp == '*') {
				SET_PUBLIC(chew,comment_state) = C_COMMENT;
				++cp;
			}
			else if (*cp == '/') {
				SET_PUBLIC(chew,comment_state) = CXX_COMMENT;
				++cp;
			}
			else {
				SET_PUBLIC(chew,comment_state) = NO_COMMENT;
				SET_PUBLIC(chew,line_state) = LS_CODE;
			}
		}
		else if (GET_PUBLIC(chew,comment_state) == FINISHING_COMMENT) {
			if (*cp == '/') {
				SET_PUBLIC(chew,comment_state) = NO_COMMENT;
				++cp;
			}
			else {
				SET_PUBLIC(chew,comment_state) = C_COMMENT;
			}
		}
		else {
			++cp;
		}
		SET_STATE(chew,escape) = false;
	}
	return cp;
}

void
chew_toplevel(void)
{
	SET_PUBLIC(chew,line_state) = LS_NEUTER;
	SET_PUBLIC(chew,comment_state) = NO_COMMENT;
	SET_STATE(chew,in_double_quote) = false;
	SET_STATE(chew,in_double_quote) = false;
}

/* EOF */

