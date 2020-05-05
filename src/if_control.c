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

#include "if_control.h"
#include "report.h"
#include "evaluator.h"
#include "line_edit.h"
#include "line_despatch.h"
#include "opts.h"
#include "chew.h"
#include "args.h"
#include "io.h"
#include <stddef.h>


/*!	\ingroup if_control_module, if_control_interface, if_control_internals
 *	\file if_control.c
 *
 *	This file implements the #if-Control module
 */

/*! \addtogroup if_control_internals */
/*@{*/

/*! Maximum depth of #if nesting, c.f.
 *	Minimum translation limits from ISO/IEC 9899:1999 5.2.4.1
 */
#define	MAXDEPTH        64

/* Helpers *****************************************************************/

/*! Type of if-state transition functions.*/
typedef void transition_t(void);

/*! End-of-file routine.
 *
 *	Trap errors for end of file within comment or within quotation.
 *	Reset nesting depth = 0
 */
static void
done_file(void);

/*! Increment the nesting depth. */
static void
nest(void);

/*! Assign the if-control state at the current nesting depth.
 *	\param	is	The state to be assigned. */
static void
set_state(if_state_t is);

/*! State transition */
static void Strue (void) { drop(); set_state(IS_TRUE_PREFIX); }
/*! State transition */
static void Sfalse(void) { drop(); set_state(IS_FALSE_PREFIX); }
/*! State transition */
static void Selse (void) { drop(); set_state(IS_TRUE_ELSE); }
/*! State transition */
static void Pelif (void) { print(); set_state(IS_PASS_MIDDLE); }
/*! State transition */
static void Pelse (void) { print(); set_state(IS_PASS_ELSE); }
/*! State transition */
static void Pendif(void);
/*! State transition */
static void Dfalse(void) { drop(); set_state(IS_FALSE_TRAILER); }
/*! State transition */
static void Delif (void) { drop(); set_state(IS_FALSE_MIDDLE); }
/*! State transition */
static void Delse (void) { drop(); set_state(IS_FALSE_ELSE); }
/*! State transition */
static void Dendif(void);
/*! State transition */
static void Fdrop (void) { nest();  Dfalse(); }
/*! State transition */
static void Fpass (void) { nest();  Pelif(); }
/*! State transition */
static void Ftrue (void) { nest();  Strue(); }
/*! State transition */
static void Ffalse(void) { nest();  Sfalse(); }
/*! State transition */
static void Mpass (void){
	strncpy(GET_PUBLIC(line_edit,keyword), "if  ", 4);
	Pelif();
}
/*! State transition */
static void Mtrue (void) { keywordedit("else\n");  set_state(IS_TRUE_MIDDLE); }
/*! State transition */
static void Melif (void)
{
	keywordedit("endif\n");
	set_state(IS_FALSE_TRAILER);
}
/*! State transition */
static void Melse (void) { keywordedit("endif\n"); set_state(IS_FALSE_ELSE); }

/*! The state transition table
 *
 *	An array of pointers to state transition functions.
 */
static transition_t * const transition_table[IS_COUNT][LT_TABLE_HANDLERS] = {
/* IS_OUTSIDE */
{
	Fpass,
	Ftrue,
	Ffalse,
	orphan_elif,
	orphan_elif,
	orphan_elif,
	orphan_else,
	orphan_endif,
  	print,
	done_file
},
/* IS_FALSE_PREFIX */
{
	Fdrop,
	Fdrop,
	Fdrop,
	Mpass,
	Strue,
	Sfalse,
	Selse,
	Dendif,
  	drop,
	early_eof
},
/* IS_TRUE_PREFIX */
{
	Fpass,
	Ftrue,
	Ffalse,
	Dfalse,
	Dfalse,
	Dfalse,
	Delse,
	Dendif,
  	print,
	early_eof
},
/* IS_PASS_MIDDLE */
{
	Fpass,
	Ftrue,
	Ffalse,
	Pelif,
	Mtrue,
	Delif,
	Pelse,
	Pendif,
  	print,
	early_eof
},
/* IS_FALSE_MIDDLE */
{
	Fdrop,
	Fdrop,
	Fdrop,
	Pelif,
	Mtrue,
	Delif,
	Pelse,
	Pendif,
	drop,
	early_eof
},
/* IS_TRUE_MIDDLE */
{
	Fpass,
	Ftrue,
	Ffalse,
	Melif,
	Melif,
	Melif,
	Melse,
	Pendif,
	print,
	early_eof
},
/* IS_PASS_ELSE */
{
	Fpass,
	Ftrue,
	Ffalse,
	orphan_elif,
	orphan_elif,
	orphan_elif,
	orphan_else,
	Pendif,
	print,
	early_eof
},
/* IS_FALSE_ELSE */
{
	Fdrop,
	Fdrop,
	Fdrop,
	orphan_elif,
	orphan_elif,
	orphan_elif,
	orphan_else,
	Dendif,
	drop,
	early_eof
},
/* IS_TRUE_ELSE */
{
	Fpass,
	Ftrue,
	Ffalse,
	orphan_elif,
	orphan_elif,
	orphan_elif,
	orphan_else,
	Dendif,
	print,
	early_eof
},
/* IS_FALSE_TRAILER */
{
	Fdrop,
	Fdrop,
	Fdrop,
	Dfalse,
	Dfalse,
	Dfalse,
	Delse,
	Dendif,
	drop,
	early_eof
}
/*IF     TRUE   FALSE  ELIF   ELTRUE ELFALSE ELSE  ENDIF  PLAIN  EOF */
};

/*@}*/


/*! \ingroup if_control_internals_state_utils */
/*@{*/

/*! The private state of if-control module */
STATE_DEF(if_control) {
	/*! Array of states of nested <tt>#if</tt>s  */
	if_state_t	ifstate[MAXDEPTH];
	/*! Current depth of \c #if nesting */
	size_t		depth;
	/*! Array of start lines of nested <tt>#if</tt>s  */
	size_t		if_start_lines[MAXDEPTH];
} STATE_T(if_control);

/*@}*/

/*! \addtogroup if_control_internals_state_utils */
/*@{*/

NO_PUBLIC_STATE(if_control);

IMPLEMENT(if_control,ZERO_INITABLE);

/*@}*/

static void
done_file(void)
{
	if (GET_PUBLIC(chew,comment_state) != NO_COMMENT) {
		parse_error(GRIPE_EOF_IN_COMMENT,
			"EOF in comment, #error ... or #define ... commencing line %d",
			GET_PUBLIC(chew,last_comment_start_line));
	}
	if (in_quotation()) {
		parse_error(GRIPE_EOF_IN_QUOTE,
			"EOF in quotation commencing line %d",
			GET_PUBLIC(chew,last_quote_start_line));
	}
}

static void
nest(void)
{
	size_t deep = ++SET_STATE(if_control,depth);
	if (deep >= MAXDEPTH) {
		bail(GRIPE_TOO_DEEP,"Too many levels of nesting");
	}
	GET_STATE(if_control,if_start_lines)[deep] = GET_PUBLIC(io,line_num);
}

static void
set_state(if_state_t is)
{
	size_t deep = if_depth();
	GET_STATE(if_control,ifstate)[deep] = is;
}

static void Pendif(void) { print(); --SET_STATE(if_control,depth); }

static void Dendif(void) { drop();  --SET_STATE(if_control,depth); }

/* API *********************************************************************/

void
transition(line_type_t linetype)
{
	if_state_t state = GET_STATE(if_control,ifstate)[if_depth()];
	transition_table[state][linetype]();
}

bool
dropping_line(void)
{
	if_state_t state =
		GET_STATE(if_control,ifstate)[if_depth()];
	return state == IS_FALSE_PREFIX ||
			state == IS_FALSE_MIDDLE ||
			state == IS_FALSE_ELSE ||
			state == IS_FALSE_TRAILER;
}

bool
was_unconditional_line(void)
{
	return
		GET_STATE(if_control,ifstate)[if_depth()] ==
			 IS_OUTSIDE;
}

bool
is_unconditional_line(void)
{
	if_state_t state =
		GET_STATE(if_control,ifstate)[if_depth()];
	return	state == IS_OUTSIDE ||
			state == IS_TRUE_PREFIX ||
			state == IS_TRUE_MIDDLE ||
			state == IS_TRUE_ELSE ;
}

if_state_t
if_state(void)
{
	return GET_STATE(if_control,ifstate)[if_depth()];
}

size_t
if_depth(void)
{
	return GET_STATE(if_control,depth);
}

size_t
if_start_line(void)
{
	return GET_STATE(if_control,if_start_lines)[if_depth()];
}

void
if_control_toplevel(void)
{
	SET_STATE(if_control,depth) = 0;
}


/* EOF */
