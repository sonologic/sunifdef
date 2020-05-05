#ifndef LINE_DESPATCH_H
#define LINE_DESPATCH_H

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

/*!\ingroup line_despatch_module, line_despatch_interface
 *\file line_despatch.h
 * This file provides the Line Despatch module interface
 */

/*!	\addtogroup line_despatch_interface */
/*@{*/

/*!	Print the line-buffer to output */
extern void
print(void);

/*!	Drop the line-buffer from output */
extern void
drop(void);

/*! Substitute a diagnostic insert for the line in the line-buffer
 *	and print it to output.
 *	\param	replacement	The diagnostic insert to print.
 *
 *	Depending on the policy selected by the specified or default value of
 *	the \c --conflict option, a \c #define or \c #undef directive read from
 *	input that contradicts one of the \c --define or \c --undefine options
 *	may be replaced on output with a diagnostic comment or a diagnostic
 *	\c #error directive.
 */
extern void
substitute(const char *replacement);

/*! Config the print() and drop() functions to be no-ops
 *	when the \c --symbols option is specified, as they are then
 *	redundant.
 */
extern void
line_despatch_no_op(void);

/*@}*/

/*!	\ingroup line_despatch_interface_state_utils */
/*@{*/

/*! The public state of the Line Despatch module */
PUBLIC_STATE_DEF(line_despatch) {
	int lines_dropped;	/*!< Number of input lines dropped */
	int lines_changed; /*!< Number of input lines changed */
} PUBLIC_STATE_T(line_despatch);
/*@}*/

/*!	\addtogroup line_despatch_interface_state_utils */
/*@{*/
IMPORT(line_despatch);
/*@}*/


#endif /* EOF */

