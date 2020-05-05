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
#include "platform.h"
#include "report.h"
#include "io.h"
#include "args.h"
#include "symbol_table.h"
#include "chew.h"
#include "if_control.h"
#include "line_despatch.h"
#include "evaluator.h"
#include "exception.h"
#include "dataset.h"
#include <stdarg.h>
#include <stdio.h>
#include <string.h>
#include <assert.h>

/*!\ingroup report_module, report_interface, report_internals
 *\file report.c
 * This file implements the Report module
 */

/*! \addtogroup report_internals */
/*@{*/

/*! Prefixes for debugging names of the comment states */
static char const * const comment_name[] = {
	"NO","C","CXX",	"STARTING",	"FINISHING","PSEUDO"
};

/*! Prefixes for debugging names of the line states */
static char const * const linestate_name[] = {
	"VACANT","DIRECTIVE","CODE"
};

/*! Debugging names of the \c #if states */
static char const * const ifstate_name[] = {
	"OUTSIDE", "FALSE_PREFIX", "TRUE_PREFIX",
	"PASS_MIDDLE", "FALSE_MIDDLE", "TRUE_MIDDLE",
	"PASS_ELSE", "FALSE_ELSE", "TRUE_ELSE",
	"FALSE_TRAILER"
};

/*! Debugging names of the input line types */
static char const * const linetype_name[] = {
	"TRUEI", "FALSEI", "IF", "TRUE", "FALSE",
	"ELIF", "ELTRUE", "ELFALSE", "ELSE", "ENDIF",
	"DODGY TRUEI", "DODGY FALSEI",
	"DODGY IF", "DODGY TRUE", "DODGY FALSE",
	"DODGY ELIF", "DODGY ELTRUE", "DODGY ELFALSE",
	"DODGY ELSE", "DODGY ENDIF",
	"PLAIN", "EOF",
	"CONSISTENT DEFINE KEEP","CONSISTENT DEFINE DROP",
	"CONTRADICTORY DEFINE","DIFFERING DEFINE",
	"CONSISTENT UNDEF KEEP", "CONSISTENT UNDEF DROP",
	"CONTRADICTORY UNDEF"
};


/*! Get the debugging name of the current comment state
 *	\return A pointer to the name in static storage.
 */
static char const *
get_comment_type_name(void)
{
	return comment_name[GET_PUBLIC(chew,comment_state)];
}

/*! Get the debugging name of the current line state
 *	\return A pointer to the name in static storage.
 */
static char const *
get_linestate_name(void)
{
	return linestate_name[GET_PUBLIC(chew,line_state)];
}

/*! Get the debugging name of the current \c #if state
 *	\return A pointer to the name in static storage.
 */
static char const *
if_state_name(void)
{
	return ifstate_name[if_state()];
}

/*! Get the debugging name of a line type.
 *
 *	\param	type	The line type whose name is wanted.
 *	\return	The debugging name of \em type.
 */
static char const *
get_linetype_name(line_type_t type)
{
	return linetype_name[type];
}


/*! Compose an output message.

	The function interface is the same as that of
	\c format_output(), with	the exception that the parameter
	<tt>va_list argp</tt> is a pointer that the caller has
	initialised with \c va_start() to address a list of
	unspecified arguments.
*/
static size_t
vformat_output(	void *dest,
			int *buflen,
			int *startoff,
			char const *format,
			va_list argp)
{
	size_t written;
	if (*buflen >= 0) {
		char *buf = *(heap_str *)dest;
		size_t spare;
		if (*buflen == 0) {
			*buflen = 1024;
			*startoff = 0;
			release(dest);
			buf = allocate(*buflen);
		}
		spare = *buflen - *startoff;
		written = vsnprintf(buf + *startoff,spare,format,argp);
		while(written >= spare) {
			spare += *buflen;
			buf = reallocate(buf,*buflen >>= 1);
			written = vsnprintf(buf + *startoff,spare,format,argp);
		}
		*(heap_str *)dest = buf;
		*startoff += (int)written;
	}
	else {
		FILE * out = dest;
		written = vfprintf(out,format,argp);
	}
	return written;
}

/*!	Produce a diagnostic message.
 *  The function interface is the same as that of \c report(), with
 *	the exception that the parameter <tt>va_list argp</tt> is a
 *	pointer that the caller has initialised with \c va_start() to
 *	address a list of unspecified arguments.
 *
 *	All diagnostic messages are ultimately composed and output
 * by this function.
 */
static void
vreport(reason_code_t reason, heap_str *buf, const char *format, va_list argp);

/*@}*/


/*! \ingroup report_internals_state_utils */
/*@{*/

/*! The global state of the Diagnostic module */
STATE_DEF(diagnostic) {
	bool		dbg; /*!< Is --debug enabled? */
	unsigned int	exitstat; /*!< Program exit status */
} STATE_T(diagnostic);
/*@}*/

/*! \addtogroup report_internals_state_utils */
/*@{*/
NO_PUBLIC_STATE(diagnostic);

IMPLEMENT(diagnostic,ZERO_INITABLE);
/*@}*/


static void
vreport(reason_code_t reason, heap_str *bufp, const char *format, va_list argp)
{
	if (GET_PUBLIC(args,got_opts) && reason != MSGCLASS_NONE) {
		int mask = GET_PUBLIC(args,diagnostic_filter);
		if (mask && ((reason & mask) != 0)) {
			/* This message is filtered out. Don't output */
			SET_STATE(diagnostic,exitstat) |= reason & MSGEVENT_MASK;
			if (bufp) {
				*bufp = NULL;
			}
			return;
		}
	}
	if (!format) {
		assert(bufp);
		if (*bufp) {
			char *mess = *bufp;
			fputs(mess,stderr);
			release((void **)bufp);
		}
	}
	else {
		void *dest;
		int buflen;
		int startoff = 0;
		int msgclass = MSGCLASS_NONE;
		size_t depth = if_depth();
		if (!bufp) {
			dest = stderr;
			buflen = -1;
		}
		else {
			dest = bufp;
			buflen = 0;
		}
		format_output(dest,&buflen,&startoff,"%s: ",GET_PUBLIC(args,prog_name));
		if (GET_PUBLIC(io,filename)) {
			format_output(dest,&buflen,&startoff,"%s: ",GET_PUBLIC(io,filename));
		}
		if (GET_PUBLIC(io,line_num)) {
			format_output(dest,&buflen,&startoff,"line %d: ",
				GET_PUBLIC(io,line_num));
		}
		if (reason & ISSUE_MASK) {
			char * category;
			msgclass = reason & MSGSEVERITY_MASK;
			switch(msgclass) {
			case MSGCLASS_PROGRESS:
				category = "progress";
				break;
			case MSGCLASS_INFO:
				category = "info";
				break;
			case MSGCLASS_WARNING:
				category = "warning";
				break;
			case MSGCLASS_ERROR:
				category = "error";
				break;
			case MSGCLASS_ABEND:
				category = "abend";
				break;
			default:
				assert(false);
			}
			format_output(dest,&buflen,&startoff,"%s 0x%05x: ",category,reason);
			SET_STATE(diagnostic,exitstat) |= reason & MSGEVENT_MASK;
		}
		vformat_output(dest,&buflen,&startoff,format,argp);
		if (msgclass != MSGCLASS_PROGRESS && depth > 0) {
			format_output(dest,&buflen,&startoff," (#if line %d depth %d)",
				(int)if_start_line(),(int)depth);
		}
		format_output(dest,&buflen,&startoff,"\n");
	}
}


/* API ***************************************************************/

/*!	Convert embedded linefeeds and tabs in a line to spaces for tidy
	printing in diagnostics.
	\param	line	Line putatively containing embedded linefeeds or tabs.
*/
void
flatten_line(char * line)
{
	size_t len = line_len(line);
	for (	;len && *line; ++line,--len) {
		if (*line == '\n' || *line == '\r' || *line == '\t') {
			*line = ' ';
		}
	}
}


size_t
line_len(char const *str)
{
	/* Some rigmarole here to maintain line-end agnosticism */
	size_t len = strlen(str);
	char const *last_ch = str + len - 1;
	char const *last_but_1_ch = last_ch - 1;
	if (EOL(last_but_1_ch)) {
		/* This will be a Windows line-end when we're a Unixoid build.
			Need to test this first */
		len -= 2;
	}
	else if (EOL(last_ch)) {
		/* This will be the usual case Unix or Windows */
		--len;
	}
	return len;
}

int
exitcode(void)
{
	unsigned severities;
	unsigned summaries;
	unsigned ret = GET_STATE(diagnostic,exitstat);
	if (GET_PUBLIC(line_despatch,lines_dropped)) {
		ret |= EVENT_SUMMARY_DROPPED_LINES & ~MSGCLASS_SUMMARY;
	}
	if (GET_PUBLIC(line_despatch,lines_changed)) {
		ret |= EVENT_SUMMARY_CHANGED_LINES & ~MSGCLASS_SUMMARY;
	}
	SET_STATE(diagnostic,exitstat) = ret;
	/* Drop the progress flag off the severities */
	severities = ret >> (MSGCLASS_SHIFT + 1);
	summaries = (ret & EVENT_SUMMARY_MASK) << GRIPE_SHIFT;
	return severities | summaries;
}

int
get_exit_flags(int bits)
{
	return bits &
		(EVENT_SUMMARY_MASK | MSGSEVERITY_MASK) & GET_STATE(diagnostic,exitstat);
}

void
set_exit_flags(int bits, bool set)
{
	bits &= (EVENT_SUMMARY_MASK | MSGSEVERITY_MASK);
	if (set) {
		SET_STATE(diagnostic,exitstat) |= bits;
	}
	else {
		SET_STATE(diagnostic,exitstat) &= ~bits;
	}
}

size_t
format_output(void *dest, int *buflen,	int *startoff, char const *format,...)
{
	size_t written;
	va_list argp;
	va_start(argp,format);
	written = vformat_output(dest,buflen,startoff,format,argp);
	va_end(argp);
	return written;
}


void
report(reason_code_t reason, heap_str *buf, const char *format,...)
{
	va_list argp;
	va_start(argp,format);
	vreport(reason,buf,format,argp);
	va_end(argp);
}

void
parse_error(int reason, const char *format,...)
{
	va_list argp;
	va_start(argp,format);
	vbail(reason,format,argp);
	va_end(argp);
}

void
bad_args_error(char const *sym, char const *symend, int symind)
{
	static char * formats[] = {
		"Duplicated argument '-%c%s'",
		"Argument '-%c%s' contradicts previous '-%c%s=%s'",
		"Argument '-%c%s' contradicts previous '-%c%s'"
	};

	enum {
		DUPLICATE,
		CONTRADICTS_NAME_EQ_VAL,
		CONTRADICTS_NAME
	};

	char const *this_val = 0;
	eval_result_t * symbol = SYMBOL(symind);
	char const *prev_val = symbol->sym_def;
	char const *name = symbol->sym_name;
	char this_opt = sym[-1];
	char prev_opt = prev_val ? 'D' : 'U';
	if (this_opt == 'D') {
		if (*symend == '=') {
			this_val = ++symend;
		}
		else {
			this_val = "";
		}
	}
	if (this_opt == prev_opt) {
		if (this_opt == 'D') {
			if (!strcmp(this_val,prev_val)) {
				report(	GRIPE_DUPLICATE_ARGS,
							NULL,
							formats[DUPLICATE],
							this_opt,
							sym);
			}
			else if (prev_val[0]) {
				bail(GRIPE_INVALID_ARGS,formats[CONTRADICTS_NAME_EQ_VAL],
					this_opt,sym,prev_opt,name,prev_val);
			}
			else {
				bail(GRIPE_INVALID_ARGS,formats[CONTRADICTS_NAME],
					this_opt,sym,prev_opt,name);
			}
		}
		else { /* Both -U, so can only be duplicate */
			report(GRIPE_DUPLICATE_ARGS,NULL,formats[DUPLICATE],this_opt,sym);
		}
	}
	else if (this_opt == 'D') { /* Previous must be -U */
		bail(GRIPE_INVALID_ARGS,formats[CONTRADICTS_NAME],
				this_opt,sym,prev_opt,name);
	}
	else if (prev_val[0]) { /* This = -UNAME, previous = -DNAME=STRING */
			bail(GRIPE_INVALID_ARGS,formats[CONTRADICTS_NAME_EQ_VAL],
				this_opt,sym,prev_opt,name,prev_val);
	}
	else { /* This = -UNAME, previous = -DNAME */
		bail(GRIPE_INVALID_ARGS,formats[CONTRADICTS_NAME],
				this_opt,sym,prev_opt,name);
	}
}

void
give_up_confused(void)
{
	bail(GRIPE_CONFUSED,"Bailing out in confusion");
}

void
vbail(int reason, const char *msg, va_list argp)
{
	close_io(true);
	if (msg) {
		vreport(reason,NULL,msg,argp);
	}
	else {
		SET_STATE(diagnostic,exitstat) |= reason & MSGEVENT_MASK;
	}
	if (exceptions_enabled && ((reason & MSGEVENT_MASK) != MSGCLASS_ABEND)) {
		throw(reason);
	}
	exit(exitcode());
}


void
bail(int reason, const char *format,...)
{
	va_list argp;
	va_start(argp,format);
	vbail(reason,format,argp);
	va_end(argp);
}

void
debugging(bool on)
{
	SET_STATE(diagnostic,dbg) = on;
}

void
debug(dbg_code_t how,...)
{
	if (GET_STATE(diagnostic,dbg)) {
		va_list ap;
		va_start(ap, how);
		switch(how)
		{
		case DBG_1:
			vreport(MSGCLASS_NONE,NULL,"eval%d !",ap);
			break;
		case DBG_2:
			vreport(MSGCLASS_NONE,NULL,"eval%d (",ap);
			break;
		case DBG_3:
			vreport(MSGCLASS_NONE,NULL,"eval%d number",ap);
			break;
		case DBG_4:
			vreport(MSGCLASS_NONE,NULL,"eval%d defined",ap);
			break;
		case DBG_5:
			vreport(MSGCLASS_NONE,NULL,"eval%d symbol",ap);
			break;
		case DBG_6:
			vreport(MSGCLASS_NONE,NULL,"eval%d bad expr",ap);
			break;
		case DBG_7:
			vreport(MSGCLASS_NONE,NULL,"eval%d = %d",ap);
			break;
		case DBG_8:
			vreport(MSGCLASS_NONE,NULL,"eval%d",ap);
			break;
		case DBG_9:
			vreport(MSGCLASS_NONE,NULL,"eval%d %s",ap);
			break;
		case DBG_10:
			vreport(MSGCLASS_NONE,NULL,"eval%d = %d",ap);
			break;
		case DBG_11:
			vreport(MSGCLASS_NONE,NULL,"eval %s",ap);
			break;
		case DBG_12:
			vreport(MSGCLASS_NONE,NULL,"eval = %d",ap);
			break;
		case DBG_13:
			vreport(MSGCLASS_NONE,NULL,"eval #define %.*s",ap);
			break;
		case DBG_14:
		{
			size_t safelen = va_arg(ap,size_t);
			char const * define = va_arg(ap,char const *);
			int linetype = va_arg(ap,int);
			char const * const linetype_name = get_linetype_name(linetype);
			report(MSGCLASS_NONE,NULL,"eval #define %.*s = %s",
				safelen,define,linetype_name);
			break;
		}
		case DBG_15:
			vreport(MSGCLASS_NONE,NULL,"eval #undef %.*s",ap);
			break;
		case DBG_16:
		{
			size_t safelen = va_arg(ap,size_t);
			char const * define = va_arg(ap,char const *);
			int linetype = va_arg(ap,int);
			char const * const linetype_name = get_linetype_name(linetype);
			report(MSGCLASS_NONE,NULL,"eval #undef %.*s = %s",
				safelen,define,linetype_name);
			break;
		}
		case DBG_17:
			report(MSGCLASS_NONE,NULL,"parser %s comment %s line",
				get_comment_type_name(), get_linestate_name());
			break;
		case DBG_18:
			vreport(MSGCLASS_NONE,NULL,"find_sym %s %s",ap);
			break;
		case DBG_19:
		{
			int linetype = va_arg(ap,int);
			report(MSGCLASS_NONE,NULL,"process %s -> %s depth %d",
		    	get_linetype_name(linetype),
		    	if_state_name(),
				if_depth());
			break;
		}
		case DBG_20:
			vreport(MSGCLASS_NONE,NULL,"eval%d +",ap);
			break;
		case DBG_21:
			vreport(MSGCLASS_NONE,NULL,"eval%d -",ap);
			break;
		case DBG_22:
			vreport(MSGCLASS_NONE,NULL,"eval%d ~",ap);
			break;
		default:
			assert(false);
		}
		va_end(ap);
	}
}

void
exit_diagnostics(void)
{
	size_t infiles =
		file_tree_count(GET_PUBLIC(dataset,file_tree),FT_COUNT_FILES,NULL);
	unsigned donefiles = GET_PUBLIC(dataset,donefiles);
	unsigned errorfiles = GET_PUBLIC(dataset,errorfiles);
	char * diagnostic_status = "";

	int ret = exitcode();
	if_control_toplevel();
	io_toplevel();

	if (GET_STATE(diagnostic,exitstat) & MSGCLASS_ABEND) {
		diagnostic_status = " ABNORMALLY";
	}
	else if (GET_STATE(diagnostic,exitstat) & MSGCLASS_ERROR) {
		diagnostic_status = " with errors";
	}
	else if (GET_STATE(diagnostic,exitstat) & MSGCLASS_WARNING) {
		diagnostic_status = " with warnings";
	}
	else if (GET_STATE(diagnostic,exitstat) & MSGCLASS_INFO) {
		diagnostic_status = " with remarks";
	}
	report(PROGRESS_SUMMARY_ALL_DONE,NULL,
		"Completed%s, exit code 0x%02x",diagnostic_status,ret);
	if (infiles) {
		report(PROGRESS_SUMMARY_FILES_REACHED,NULL,
			"%d out of %d input files were reached; %d files were not reached",
			donefiles,infiles,infiles - donefiles);
		report(PROGRESS_SUMMARY_FILES_DROPPED,NULL,
			"%d out %d files reached were valid; "
			"%d were abandoned due to parse errors",
			donefiles - errorfiles,donefiles,errorfiles);
		if (infiles == donefiles && errorfiles == 0) {
			if (GET_STATE(diagnostic,exitstat) &
				EVENT_SUMMARY_DROPPED_LINES & ISSUE_MASK) {
				report(EVENT_SUMMARY_DROPPED_LINES,NULL,"Input lines were dropped");
			}
			if (GET_STATE(diagnostic,exitstat) &
				EVENT_SUMMARY_CHANGED_LINES & ISSUE_MASK) {
				report(EVENT_SUMMARY_CHANGED_LINES,NULL,"Input lines were changed");
			}
			if (GET_STATE(diagnostic,exitstat) &
				EVENT_SUMMARY_ERRORED_LINES & ISSUE_MASK) {
				report(	EVENT_SUMMARY_ERRORED_LINES,
						NULL,
						"Input lines were changed to #error directives");
			}
			if (GET_STATE(diagnostic,exitstat) &
				EVENT_SUMMARY_ERROR_OUTPUT & ISSUE_MASK) {
				report(	EVENT_SUMMARY_ERRORED_LINES,
						NULL,
						"Unconditional #error directives were output");
			}
		}
	}
}

void
orphan_elif(void)
{
	parse_error(GRIPE_ORPHAN_ELIF,"Orphan #elif");
}

void
orphan_else(void)
{
	parse_error(GRIPE_ORPHAN_ELSE,"Orphan #else");
}

void
orphan_endif(void)
{
	parse_error(GRIPE_ORPHAN_ENDIF,"Orphan #endif");
}

void
early_eof(void)
{
	parse_error(GRIPE_EOF_TOO_SOON,"Unexpected EOF");
}

void
obfuscation(void)
{
	parse_error(GRIPE_OBFUSC_DIRECTIVE,
				"Obfuscated preprocessor control line");
}

void
processing_file(char const *filename)
{
	report(PROGRESS_PROCESSING_FILE,NULL,"Processing file \"%s\"",filename);
}

void
entering_dir(char const *dirname)
{
	report(PROGRESS_ENTERING_DIR,NULL,"Entering directory \"%s\"",dirname);
}

void
leaving_dir(char const *dirname)
{
	report(PROGRESS_LEAVING_DIR,NULL,"Leaving directory \"%s\"",dirname);
}

heap_str
concatenate(size_t count, char *strs[], int punctch)
{
	heap_str concat;
	char *posn;
	size_t *lengths = callocate(count,sizeof(size_t));
	size_t buflen = 0;
	size_t punctsz = punctch == -1 ? 0 : count - 1;
	size_t i = 0;
	for (	; i < count; ++i) {
		buflen += lengths[i] = strlen(strs[i]);
	}
	buflen += punctsz + 2;
	concat = allocate(buflen);
	posn = concat;
	for (i = 0; i < count; ++i) {
		strcpy(posn,strs[i]);
		posn += lengths[i];
		if (punctch != -1) {
			*posn++ = (char)punctch;
		}
	}
	if (punctch != -1) {
		posn[-1] = '\0';
	}
	free(lengths);
	return concat;
}


/* EOF */


