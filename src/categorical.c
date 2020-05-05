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

#include "categorical.h"
#include "report.h"
#include "evaluator.h"
#include "line_despatch.h"
#include "memory.h"
#include "io.h"
#include "if_control.h"
#include <stdlib.h>
#include <assert.h>

/*!\ingroup categorical_internals categorical_interface
 * \file categorical.c
 *	This file implements the Categorical module
 */

/*! \ingroup categorical_internals_state_utils */
/*@{*/

/*! The global state of the Categorical module */
STATE_DEF(categorical) {
	/*! The public state of the Categorical module */
	INCLUDE_PUBLIC(categorical);
	/*! This is the prefix for diagnostics inserted into the output:
	 *	-	"//" when <tt>--conflict comment</tt> is in force.
	 *	-	"#" when <tt>--conflict error</tt> is in force.
	 */
	char *		contradiction_insert_prefix;
	/*! This is the reason code issued when a contradiction is diagnosed:
	 *	-	GRIPE_COMMENTED_CONTRADICTION when <tt>--conflict comment</tt> is
	 *		in force.
	 *	-	GRIPE_ERRORED_CONTRADICTION when <tt>--conflict error</tt> is
	 *		in force.
	 *	-	GRIPE_DELETED_CONTRADICTION when <tt>--conflict delete</tt> is
	 *		in force.
	 */
	int			contradiction_insert_reason;
	/*! In the preamble of \c save_contradiction() we record here if any
	 *	warnings had already accrued in the exit status, because we will
	 *	provisionally register warnings in the exit status. If we later
	 *	discover there is no contradiction then we need to clear warnings
	 *	from the exit status if this was the first.
	 */
	bool		had_warnings;
	/*! Pointers for a pair of heap buffers in which to compose
	 *	diagnositic messages for \c stdout and \c stderr
	 */
	heap_str err_msg_buf[2];
} STATE_T(categorical);
/*@}*/

/*! \addtogroup categorical_internals_state_utils */
/*@{*/

IMPLEMENT(categorical,STATIC_INITABLE)

USE_STATIC_INITIALISER(categorical) = {
	{ -1 },
	 "//",
	GRIPE_COMMENTED_CONTRADICTION
};

/*@}*/

/*!\addtogroup categorical_internals */
/*@{*/

/*!	Is there an undischarged conflict between an input \c #undef
 *	and a \c --define option? The macro tests whether any memory
 *	is allocated for our \c stderr buffer.
 */
#define CONTRADICTION_PENDING\
	(GET_STATE(categorical,err_msg_buf)[ERR_MSG_BUF] != 0)

/*! Labels for output buffers associated with standard streams. */
enum {
	ERR_MSG_BUF,	/*!< Buffer for \c stderr */
	OUT_MSG_BUF		/*!< Buffer for \c stdout */
};


/* Helpers *****************************************************************/

/*! Compose a standard \c printf format for inserting error
	diagnostics on output.

	\param		buf		Address of a pointer at which storage shall
						be allocated to hold the composed format.
	\param		sub_format	A \c printf format that is to be
						embedded in within the one we are constructing.

	The function partially completes a format for diagnostics
	that may be inserted in output in place of an \c #undef or
	\c #define directive that contradicts a \c --define option.
	It resolves the prefix of the diagnostic, depending on the
	specified or default \c --conflict	policy.

	\em sub_format varies depending on whether we want to
	complain	of an \c #undef that contradicts a \c --define
	option, or of a \c #define that conflicts with a \c --define
	option by differently redfining a symbol.

	The constructed format is stored in storage allocated
	at a pointer that is stored at \em buf on return. It is
	the caller's responsibility to free this storage.
 */
static void
compose_contradiction_insert_format(heap_str *buf, const char *sub_format)
{
	int buflen = 0;
	int startoff = 0;
	format_output(buf,&buflen,&startoff,
		"%serror : inserted by sunifdef: %s at %s(%d)\n",
		GET_STATE(categorical,contradiction_insert_prefix),
		sub_format,
		GET_PUBLIC(io,filename),
		GET_PUBLIC(io,line_num));
}

/*! Erase the stored information that represents a pending diagnostic for
 *	a contradiction.
 *
 *	We will call this function either when it turns out that there is
 *	no real contradiction to complain of, or when there is a contradiction
 *	and the diagnostic is discharged.
 *
 *	There will be a diagnostic message at least for \c stderr, and also for
 *	\c stdout unless <tt>--conflict delete </tt> is in force. The diagnostics
 *	are allocated in heap blocks, and we will have stored the line number
 *	of the conflicting \c #undef directive to which the diagnostics refer.
 *	These data are deleted.
 *
 */
void
forget_pending_contradiction(void)
{
	release((void **)(&GET_STATE(categorical,err_msg_buf)[OUT_MSG_BUF]));
	release((void **)(&GET_STATE(categorical,err_msg_buf)[ERR_MSG_BUF]));
	SET_PUBLIC(categorical,last_contradictory_undef) = -1;
}

/*! Insert a stored error diagnostic into the output as an \c #error
 *	directive or a comment (depending on the \c --conflict policy)
 *	to replace an \c #undef or \c #define directive that conflicts with a
 *	\c --define  option. Reiterate the diagnostic on stderr.
 *
 *	If the <tt>--conflict delete</tt> is in force, there will be
 *	no diagnostic to insert in output, but the diagnostic is still
 *	written to \c stderr.
 *
 */
static void
insert_pending_contradiction(void)
{
	assert(GET_STATE(categorical,err_msg_buf)[ERR_MSG_BUF]);
	report(GET_STATE(categorical,contradiction_insert_reason),
		&GET_STATE(categorical,err_msg_buf)[ERR_MSG_BUF],NULL);
	if (GET_STATE(categorical,contradiction_insert_prefix)) {
		substitute(GET_STATE(categorical,err_msg_buf)[OUT_MSG_BUF]);
		if (GET_STATE(categorical,contradiction_insert_reason) ==
			GRIPE_ERRORED_CONTRADICTION) {
			set_exit_flags(EVENT_SUMMARY_ERRORED_LINES,true);
			if (is_unconditional_line()) {
				set_exit_flags(EVENT_SUMMARY_ERROR_OUTPUT,true);
				report(GRIPE_UNCONDITIONAL_ERROR_INPUT,NULL,
					"An unconditional #error directive was output");
			}
		}
	}
	else {
		++SET_PUBLIC(line_despatch,lines_dropped);
	}
	forget_pending_contradiction();
}

/*! Insert an error diagnostic into the output as an \c #error
 *	directive or a comment (depending on the \c --conflict policy)
 *	to replace an \c #undef directive that conflicts with a \c --define
 *	option. Reiterate the diagnostic on stderr.
 *
 *	\param	sub_format	A \c printf format with which to compose the
 *						appropriate diagnostic. Varies depending on whether
 *						we want to complain of an \c #undef that contradicts
 *						a \c --define option, or of a \c #define that conflicts
 *						with a \c --define option by differently redfining a
 *						symbol.
 */

static void
insert_contradiction(char const *sub_format)
{
	save_contradiction(sub_format);
	insert_pending_contradiction();
}
/*@}*/


/* API ***************************************************************/

bool
weed_categorical_directive(int lineval)
{
	bool handled = false;
	if (lineval > LT_EOF) {
		handled = true;
		switch(lineval) {
			case LT_CONSISTENT_DEFINE_DROP:
				drop();
				break;
			case LT_CONSISTENT_UNDEF_KEEP:
			case LT_CONSISTENT_DEFINE_KEEP:
				print();
				break;
			case LT_CONSISTENT_UNDEF_DROP:
				drop();
				break;
			case LT_CONTRADICTORY_DEFINE:
				insert_contradiction("\"%.*s\" contradicts -U");
				break;
			case LT_DIFFERING_DEFINE:
				insert_contradiction("\"%.*s\" differently redefines -D "
					"symbol");
				break;
			case LT_CONTRADICTORY_UNDEF:
				drop();
				/* N.B.! We return here so that our saved error is not
					inserted immediately */
				return handled;
			default:;
		}
	}
	return handled;
}

void
contradiction_policy(contradiction_policy_t p)
{
	switch(p){
	case CONTRADICTION_DELETE:
		SET_STATE(categorical,contradiction_insert_prefix) = NULL;
		SET_STATE(categorical,contradiction_insert_reason) =
			GRIPE_DELETED_CONTRADICTION;
		break;
	case CONTRADICTION_COMMENT:
		SET_STATE(categorical,contradiction_insert_prefix) = "//";
		SET_STATE(categorical,contradiction_insert_reason) =
			GRIPE_COMMENTED_CONTRADICTION;
		break;
	case CONTRADICTION_ERROR:
		SET_STATE(categorical,contradiction_insert_prefix) = "#";
		SET_STATE(categorical,contradiction_insert_reason) =
			GRIPE_ERRORED_CONTRADICTION;
		break;
	default:
		assert(false);
	}
}

void
flush_contradiction(void)
{
	if (GET_STATE(categorical,err_msg_buf)[ERR_MSG_BUF])
	{
		--SET_PUBLIC(line_despatch,lines_dropped);
		insert_pending_contradiction();
	}
}

void
forget_contradiction(void)
{
	if (GET_STATE(categorical,err_msg_buf)[OUT_MSG_BUF]) {
		if (!GET_STATE(categorical,had_warnings)) {
			set_exit_flags(MSGCLASS_WARNING,false);
		}
		forget_pending_contradiction();
	}
}

void
save_contradiction(char const *sub_format)
{
	size_t linelen = line_len(GET_PUBLIC(io,line_start));
	size_t extension_lines = GET_PUBLIC(io,extension_lines);
	assert(GET_STATE(categorical,err_msg_buf)[ERR_MSG_BUF] == 0);
	if (extension_lines) {
		/* There may be embedded newlines or tabs in the error message buffer */
		flatten_line(GET_PUBLIC(io,line_start));
	}
	SET_STATE(categorical,had_warnings) = get_exit_flags(MSGCLASS_WARNING) != 0;
	report(GET_STATE(categorical,contradiction_insert_reason),
		&GET_STATE(categorical,err_msg_buf)[ERR_MSG_BUF],
		sub_format,
		linelen,
		GET_PUBLIC(io,line_start));
	if (GET_STATE(categorical,contradiction_insert_prefix)) {
		heap_str contradiction_insert_format = NULL;
		int buflen = 0;
		int startoff;
		compose_contradiction_insert_format(&contradiction_insert_format,
			sub_format);
		assert(GET_STATE(categorical,err_msg_buf)[OUT_MSG_BUF] == 0);
		format_output(&GET_STATE(categorical,err_msg_buf)[OUT_MSG_BUF],
			&buflen,&startoff,
			contradiction_insert_format,
			linelen,
			GET_PUBLIC(io,line_start),
			GET_PUBLIC(io,filename),
			GET_PUBLIC(io,line_num));
		free(contradiction_insert_format);
	}
}

/* EOF */
