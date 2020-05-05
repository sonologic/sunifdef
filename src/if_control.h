#ifndef IF_CONTROL_H
#define IF_CONTROL_H
 
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
#include "evaluator.h" 
#include "bool.h"
#include <stddef.h>

/*!\ingroup if_control_module, if_control_interface
 *\file if_control.h
 * This file provides the If-Control module interface
 */

/*!	\addtogroup if_control_interface */
/*@{*/

/*! Enumeration of the if-control states */
typedef enum {
	IS_OUTSIDE,	
		/*!< We are outside any \c #if */
	IS_FALSE_PREFIX,
		/*!< We have a false \c #if followed by false <tt>#elif</tt>s */
	IS_TRUE_PREFIX,	
		/*!< The first non-false \c #(el)if is true */
	IS_PASS_MIDDLE,	
		/*!< The first non-false \c #(el)if is unknown */
	IS_FALSE_MIDDLE,
		/*!< We have a false \c #elif after a pass state */
	IS_TRUE_MIDDLE,	
		/*!< We have a true \c #elif after a pass state */
	IS_PASS_ELSE,	
		/*!< We have an \c #else after a pass state */
	IS_FALSE_ELSE,
		/*!< We have an \c #else after a true state */
	IS_TRUE_ELSE,
		/*!< We have an \c #else after only false states */
	IS_FALSE_TRAILER,
		/*!< All <tt>#elif</tt>s after a true are false */
	IS_COUNT
		/*!< Count of if-control states */
} if_state_t;

/*!	Transition the if-control state given an evaluated line type.
 *	\param	linetype	The linetype of the last evaluated input line.
 *	
 *	The function advances a state-machine whose parameters are the
 *	current input linetype and the current if-control state. The
 *	transition performs the appropriate action to despatch the
 *	line just evaluated and sets a new if-state. 
 */
extern void
transition(line_type_t linetype);

/*! Are we dropping the current line? */
extern bool
dropping_line(void);

/*! Were we always going to keep the current line unconditionally?
 * I.e. it is not in any #if-scope?
 */	 
extern bool
was_unconditional_line(void);

/*!	Are we keeping the current line unconditionally?
 *	i.e. it is not in any #if-scope or is in the scope of a
 *	satisfied #if-condition.	
 */	
extern bool
is_unconditional_line(void);

/*! For debug(), return the starting line number of the
 *	current #if-sequence.
 */ 
extern size_t
if_start_line(void);

/*! Return the current depth of if-nesting */
extern size_t
if_depth(void);

/*! Return the current if-state */
extern if_state_t
if_state(void);

/*! Reset the depth of if-nesting to 0 */
void
if_control_toplevel(void);

 
/*@}*/

/*! \addtogroup if_control_interface_state_utils */
/*@{*/
IMPORT_INITOR(if_control);
IMPORT_FINITOR(if_control);
/*@}*/


#endif /* EOF */
