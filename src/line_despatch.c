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

#include "line_despatch.h"
#include "report.h"
#include "args.h"
#include "line_edit.h"
#include "io.h"
#include "opts.h"
#include "bool.h"
#include <stdio.h>
#include <ctype.h>

/*!\ingroup line_despatch_internals line_despatch_interface
 * \file line_despatch.c
 *	This file implements the Line Despatch module
 */

/*!\addtogroup line_despatch_internals */
/*@{*/

/* Helpers ***********************************************************/

/*! Count number of newline characters in string
 *
 * \param text 	Zero-terminated string.
 * \returns     Number of newline characters in text
 */
static size_t count_lines(char const *text)
{
	size_t count = 0;
	while(text != NULL)
	{
		text = strchr(text, '\n');
		if (text!=NULL)
		{
			count++;
			text += 1;
		}
	}
	return count;
}

/*! Print an unmodified line to output with no complications
 */
static void
printline_fast(void)
{
	fputs(GET_PUBLIC(io,line_start),GET_PUBLIC(io,output));
}

/*! Print a line to output deleting chunks marked for
 *	unconditional deletion.
 */
static void
printline_cut(void)
{
	char *leader = GET_PUBLIC(io,line_start);
	char *follower = leader;
	int concat_risk = 0;
	for (	;*leader; leader += (*leader != 0)) {
		for (	;!DELETEABLE(*leader); ++leader) {};
		if (fwrite(follower,1,leader - follower,GET_PUBLIC(io,output)) !=
			leader - follower) {
			bail(GRIPE_CANT_WRITE_FILE,"Write error on output\n");
		}

		follower = leader;
		concat_risk = !isspace(follower[-1]);
		for (	;*leader && DELETEABLE(*leader); ++leader) {};
		concat_risk += (*leader && !isspace(*leader));
		if (concat_risk == 2) {
			fputc(' ',GET_PUBLIC(io,output));
		}
		follower = leader;
	}
	if (*follower) {
		fputc(*follower,GET_PUBLIC(io,output));
	}
}

/*! Print an output line that may contain superfluous
 *	parentheses marked for deletion, but no other parts
 *	marked for deletion.
 *
 *	In this case we have not materially simplified the line and just restore
 *	the parentheses before printing the line.
 */
static void
printline_restore_paren(void)
{
	restore_paren();
	printline_fast();
}

/*!	Print an output line containing chunks marked for
 *	deletion.
 *
 *	We will delete all these chunks only if and only if some of them
 *	represent truth-functional simplifications of the line.
 *  Otherwise the only logical deletions will represent superfluous parentheses.
 *	We will then restore those parentheses before printing the line.
 */
static void
printline_slow(void)
{
	if (GET_PUBLIC(line_edit,simplification_state) & OPS_CUT) {
		++SET_PUBLIC(line_despatch,lines_changed);
		printline_cut();
	}
	else {
		printline_restore_paren();
	}
}

/*!	Print a line to output from the line-buffer.
 *
 *	The function selects a more specialised helper function to which
 *	it delegates.
 */
static void
printline(void)
{
	if (GET_PUBLIC(line_edit,simplification_state) == UNSIMPLIFIED) {
		printline_fast();
	}
	else {
		printline_slow();
	}
}

/*! No-op implementation of flushline() selected when the \c --symbols option
 *	in force
 */
static void
flushline_dummy(bool keep,const char *insert_text){}


/*! Write a line to the output or drop it, according to command line options.
 *
 *	\param	keep	Is the line to be kept of dropped?
 *	\param	insert_text		Either \c NULL or a pointer to a null-terminated
 *							diagnostic that is to be written instead of
 *							the line-buffer.
 *
 *	If \c insert_text is non-null then this string is output.
 *
 *	Otherwise is \c keep is true then the line-buffer is output unless
 *	the \c --complement option is in force.
 *
 *	If \c keep is false, or is true together with \c --complement,
 *	the line buffer is dropped. If the \c --blank is not in force then
 *	nothing is written to putput. Otherwise an empty line is written to
 *	output.
 */

static void
flushline_live(bool keep, char const *insert_text)
{
	int extension_lines = GET_PUBLIC(io,extension_lines);
	if (insert_text) {
		/* Replacing a contradictory input line with a
		 * diagnostic comment or #error */
		fputs(insert_text,GET_PUBLIC(io,output));
		++SET_PUBLIC(line_despatch,lines_changed);
		return;
	}
	if (keep ^ GET_PUBLIC(args,complement)) {
		printline();
	}
	else {
		discard_policy_t discard_policy = GET_PUBLIC(args,discard_policy);
		if (discard_policy == DISCARD_BLANK) {
		for(size_t lines=count_lines(GET_PUBLIC(io,line_start)); lines; lines--)
		{
				putc('\n', GET_PUBLIC(io,output));
				++SET_PUBLIC(line_despatch,lines_changed);
			}
		}
		else if (discard_policy == DISCARD_DROP) {
			SET_PUBLIC(line_despatch,lines_dropped) += extension_lines;
			++SET_PUBLIC(line_despatch,lines_dropped);
		}
		else {
			char *text = GET_PUBLIC(io,line_start);
			while(text && text[0])
			{
				char *newline = strchr(text, '\n');
				if(newline!=NULL)
				{
					newline[0] = 0;
				}
				fputs("//sunifdef < ",GET_PUBLIC(io,output));
				fputs(text,GET_PUBLIC(io,output));
				fputs("\n",GET_PUBLIC(io,output));
				if(newline!=NULL)
				{
					newline[0] = '\n';
				}
				else
				{
					break;
				}
				text = newline + 1;
			}
		}
	}
}

/*@}*/

/*!\ingroup line_despatch_internals_state_utils */
/*@{*/

/*! The global state of the Line Despatch module */
STATE_DEF(line_despatch) {
	/*! The public state of the Line Despatch module */
	INCLUDE_PUBLIC(line_despatch);
	/*! Pointer to the function that will be called to flush the
	 *	the line-buffer to output. Will address flushline_dummy()
	 *	(a no-op) when the \c --symbols option is specified, and
	 *	otherwise \c flushline_line().
	 */
	void	 (*flushline)(bool,const char *);
	/*! Count of contiguous lines that are dropped together */
	size_t drop_run;
} STATE_T(line_despatch);

/*@}*/

/*!\addtogroup line_despatch_internals_state_utils */
/*@{*/

IMPLEMENT(line_despatch,STATIC_INITABLE);

USE_STATIC_INITIALISER(line_despatch) = { { 0, 0 }, flushline_live, false };
/*@}*/

/* API ***************************************************************/

void
line_despatch_no_op(void)
{
	SET_STATE(line_despatch,flushline) = flushline_dummy;
}

void
print(void)
{
	if (GET_PUBLIC(args,line_directives)) {
		if (GET_STATE(line_despatch,drop_run)) {
			int line_num = GET_PUBLIC(io,line_num);
			size_t extension_lines = GET_PUBLIC(io,extension_lines);
			if (extension_lines) {
				--line_num;
			}
			printf("#line %d\n",line_num);
			--SET_PUBLIC(line_despatch,lines_dropped);
			++SET_PUBLIC(line_despatch,lines_changed);
		}
		SET_STATE(line_despatch,drop_run) = 0;
	}
	GET_STATE(line_despatch,flushline)(true,NULL);

}

void
drop(void)
{
	GET_STATE(line_despatch,flushline)(false,NULL);
	if (GET_PUBLIC(args,line_directives)) {
		++SET_STATE(line_despatch,drop_run);
	}
}

void
substitute(const char *replacement)
{
	GET_STATE(line_despatch,flushline)(false,replacement);
}

/* EOF */
