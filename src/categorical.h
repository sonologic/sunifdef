#ifndef CATEGORICAL_H
#define CATEGORICAL_H
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

 /*!\ingroup categorical_module categorical_interface
 *  \file categorical.h
 *	This file provides the Categorical module interface
 */

/*! \addtogroup categorical_interface */
/*@{*/

/*! Enumeration of policies for handling \c #define or \c #undef
 *	directives that conflict with \c --define or \c --undefine
 *	options. 
 */
typedef enum contradiction_policy {
	/*! Delete conflicting directives */ 
	CONTRADICTION_DELETE,
	/*! Replace conflicting directives with diagnostic comments */ 
	CONTRADICTION_COMMENT,
	/*! Replace conflicting directives with \c #error directives */ 
	CONTRADICTION_ERROR
} contradiction_policy_t;


/*! Select the policy for handling \c #define or \c #undef directives.
 *	\param pol	An enumerated constant that represents the selected
 *				policy.
 *
 *	The policy is set in accordance with the \c --conflict option. 
 */
extern void
contradiction_policy(contradiction_policy_t pol);

/*! Deal with \c #define and #undef directives in input in accordance
 *	with the \c --conflict policy.
 *
 *	\param	lineval		The evaluated line type of the current
 *						input line.
 *
 *	If the evaluated line type is checked against the following set: 
 *	- LT_CONSISTENT_DEFINE_DROP:
 *	- LT_CONSISTENT_UNDEF_KEEP:
 *	- LT_CONSISTENT_DEFINE_KEEP:
 *	- LT_CONSISTENT_UNDEF_DROP:	
 *	- LT_CONTRADICTORY_DEFINE:
 *	- LT_DIFFERING_DEFINE:
 *	- LT_CONTRADICTORY_UNDEF:
 *
 *	then the line is a \c #define or \c #undef directive for which
 *	a course of action has been determined that is encoded by the
 *	the line type. The function takes this action and returns true. If
 *	the line type is not one of this set then no action is taken and
 *	the function returns false.
 *
 *	If true is returned then the line is not passed to the if-control
 *	state machine.
 *
 * 	\return  \em true iff the line type is handled by this function.
 */
extern bool
weed_categorical_directive(int lineval);

/*!	Forget about an apparent conflict that would be governed by the
 *	\c --conflict policy when later input shows it to be innocuous.
 *
 *	This function copes with the complication created by the common
 *	precautionary idioms:
 *	
 *	\c #undef FOO<br>
 *	\c #define FOO
 *
 *	and:
 *
 *	\c #undef FOO<br>
 *	\c #define FOO	XYZ
 *
 *	This input may be encountered when \c FOO is the subject of a \c --define
 *	option. In this case the precautionary \c #undef FOO will at first
 *	appear to conflict with that \c --define, but the ensuing \c #define
 *	will show that the conflict is only apparent.
 *
 *	To cope with these cases, we defer diagnostic action on the
 *	\c #undef until we see what follows. If a \c #define follows that
 *	agrees with the \c --define options, with only whitespace and
 *	comments intervening, then we forget about the pending contradiction.
 *	This function clears the pending diagnostic action.
 *
 *	\note	An \c #undef that is prima facie conflicting will \em always be
 *	be dropped, whether or not it is anulled by a following \c #define.
 *	If is not anulled by a following \c #define, then it will dropped
 *	because any of the 3 possible \c --conflict policies will require
 *	it either to be simply dropped, or else dropped and replaced with a
 *	diagnostic insertion. If the \c #undef is anulled by a following
 *	\c #define, then both will be dropped because the \c #define in
 *	isolation would be dropped.
 */	
extern void
forget_contradiction(void);

/*!	When we have dropped an \c #undef directive that conflicts with a
 *	\c --define option, and the conflict is not annulled by a following
 *	\c #define, we have a pending diagnostic action to discharge. It
 *	will consist of writing a warning to stderr, unless warnings are
 *	suppressed, and inserting a diagnostic comment or \c #error in the
 *	output, unless the \c --conflict policy is \c delete.
 *
 *	This function discharges the pending action. 
 */
extern void
flush_contradiction(void);

/*! When an \c #undef directive is read that conflicts with a \c --define
 *	option, we will diagnose a conflict if the \c #undef is not followed
 *	by a \c #define that agrees with the conflicting \c --define.
 *
 *	\param sub_format	A printf format with which to compose the
 *						appropriate diaqnostic diagnostic.
 *
 *	This function saves the diagnostic information pending our decision
 *	whether to use it or forget it.
 */
extern void 
save_contradiction(char const *sub_format);

/*@}*/

/*! \ingroup categorical_interface_state_utils */
/*@{*/

/*! The public state of the Categorical module */
PUBLIC_STATE_DEF(categorical) {
	/*! Line number of the last \c #undef that contradicts a \c --define */
	int		last_contradictory_undef;
} PUBLIC_STATE_T(categorical);

IMPORT(categorical);
/*@}*/

#endif /* EOF */
