#ifndef LINE_EDIT_H
#define LINE_EDIT_H

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
 
/*!\ingroup line_edit_module, line_edit_interface
 *\file line_edit.h
 * This file provides the Line Edit module interface
 */
 
/*!	\addtogroup line_edit_interface */
/*@{*/

/*! This character is provisionally inserted in the line
	buffer to mark it for eventual deletion
*/
#define DELETEABLE_CHR  ((char)001)	
/*! This character is provisionally inserted in the line
	buffer in place of a '(' that will eventually be
	deleted if the line is significantly simplified.
*/
#define DELETEABLE_LPAREN ((char)002)
/*! This character is provisionally inserted in the line
	buffer in place of a ')' that will eventually be
	deleted if the line is significantly simplified.
*/
#define DELETEABLE_RPAREN ((char)003)
/*! Macro function to decide if a character in the line-buffer is deleteable
 *	\param ch	The character to test.
 *	\return		Non-zero if the character may be deleted.
 */ 
#define DELETEABLE(ch) ((ch) < 4)

/*! Enumeration of the possible simplification states of an
 *	<tt>#i</tt>-directive in the line-buffer 
 */	  
typedef enum {
	UNSIMPLIFIED,	/*!< The directive has not been simplified */
	BRACKETS_PRUNED,	/*!< Superfluous brackets have been deleted */
	OPS_CUT	/*!< Operators and operands have been deleted through
				truth-functional simplification */
} simplification_state_t;


/*! Replace the keyword of the current directive with another
 *	to rectify the CPP syntax for dropped lines, then output
 *	the edited directive.
 *
 *	\param	replacement		The keyword with which to replace the current
 *							keyword.
 */ 
extern void
keywordedit(const char *replacement);

/*! Logically delete a pair of redundant parentheses from a 
 *	<tt>#</tt>-directive in the line-buffer.
 *	\param	lparen	Pointer to the '('
 *	\param	rparen	Poiunter to the ')'
 *
 *	Logical deletion is done by inserting \c DELETEABLE_LPAREN
 *	in the line-buffer in place of the '(' and \c DELETEABLE_RPAREN
 *	in place of the ')'.
 *
 *	If evaluation of the directive results in material simplification
 * (i.e. results in deletions other	than deletions of redundant parentheses),
 *	then the logically	deleted parentheses will be physically deleted when
 *	evaluation is complete.
 *
 *	If evaluation of the directive results in no materal simplification
 *	then the logically deleted parentheses will finally be restored
 *	by restore_paren();
 *
 *	The function updates the module state with \c BRACKETS_PRUNED
 */
void
delete_paren(char *lparen, char *rparen);  

/*! Restore logically deleted parentheses to the <tt>#</tt>-directive in the
 *	line buffer	when we cannot materially simplify it. 
 *
 *	This is done on the principle that if we cannot materially simplify
 *	the directive then wev will leave it alone.
 *
 *	The function does not bother to retract \c BRACKETS_PRUNED from the module
 *	state, because this is superflous whenever the the function is called. 
 */
void
restore_paren(void);

/*!	Logically delete part of a <tt>#</tt>-directive in the line buffer.
 *	\param	start	The start of the text to delete
 *	\param	end		The address just past the text to delete.
 *
 *	The logical deletion is done by filling the line-buffer from \c start
 *	to \c end with \c DELETEABLE_CHR. Logical deletion is efficient because
 *	it avoids shuffling down terminal segments of the buffer in the course
 *	of evaluation, when the effect of such editing may be obliterated by
 *	subsequent deletions. Physical deletion is performed only once at the
 *	end of evaluation, by removing all logically deleted portions of the
 *	line-buffer.
 *
 *	The function is only called to delete a truth-functionally redundant
 *	operator and one of its operands, so it updates the module state with
 *	\c OPS_CUT
 */
void
cut_text(char *start, char *end);

/*@}*/

/*!	\ingroup line_edit_interface_state_utils */
/*@{*/

/*! The public state of the Line Edit module */
PUBLIC_STATE_DEF(line_edit) { 
	char*	keyword; /*!< Pointer to keyword of the current directive */
	int simplification_state;
		/*!< The simplification state of the current directive */
} PUBLIC_STATE_T(line_edit);
/*@}*/

/*!	\addtogroup line_edit_interface_state_utils */
/*@{*/
IMPORT(line_edit);
/*@}*/


#endif /* EOF */
