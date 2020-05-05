#ifndef REPORT_H
#define REPORT_H

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
#include "bool.h"
#include <stddef.h>
#include <stdarg.h>

 /*!\ingroup report_module report_interface
 *  \file report.h
 *	This file provides the Report module interface
 */

/*!\addtogroup report_interface */
/*@{*/

/*! Enumeration of all diagnostic reason codes and auxiliary
	bit masks and offsets.
	A complete reason code consists or an or-combination
	of a message class code and an issue code.
	A range of issue codes are \em event \em summary codes,
	representing significant diagnostics accrued in the course
	of processing all inputs. Another range of issue codes are
	\em gripes, representing complaints specific to input
	fragments.
 */
typedef enum {
	/*! The message class bitfield is offset this amount */
	MSGCLASS_SHIFT = 11,
	/*! &-ed with code strips off message class */
	ISSUE_MASK = (1 << MSGCLASS_SHIFT) - 1,
	/*! No message class */
	MSGCLASS_NONE = 0 << MSGCLASS_SHIFT,
	/*! Progress message */
	MSGCLASS_PROGRESS = 1 << MSGCLASS_SHIFT,
	/*! Informational diagnositic */
	MSGCLASS_INFO = 2 << MSGCLASS_SHIFT,
	/*! Warning diagnostic */
	MSGCLASS_WARNING = 4 << MSGCLASS_SHIFT,
	/*! User error diagnostic */
	MSGCLASS_ERROR = 8 << MSGCLASS_SHIFT,
	/*! Fatal internal or environment error diagnostic */
	MSGCLASS_ABEND = 16 << MSGCLASS_SHIFT,
	/*! Summary message. */
	MSGCLASS_SUMMARY = 32 << MSGCLASS_SHIFT,

	/*! &-ed with code extracts message class */
	MSGCLASS_MASK = 255 << MSGCLASS_SHIFT,
	/*! &-ed with code extracts message severity, i.e.
		message class other than summary */
	MSGSEVERITY_MASK = MSGCLASS_MASK & ~MSGCLASS_SUMMARY,
	/*! &-ed with code extracts any event message class, i.e.
		message class other than progress and summary.
	*/
	MSGEVENT_MASK = MSGSEVERITY_MASK & ~MSGCLASS_PROGRESS,

	/*! Min value of event summaries */
	EVENT_SUMMARY_MIN = 1,
	/*! Summary diagnostic - input lines were dropped */
	EVENT_SUMMARY_DROPPED_LINES = 1 | MSGCLASS_INFO | MSGCLASS_SUMMARY,
	/*! Summary diagnostic - input lines were changed */
	EVENT_SUMMARY_CHANGED_LINES = 2 | MSGCLASS_INFO  | MSGCLASS_SUMMARY,
	/*! Summary diagnostic - input lines were converted to #errors */
	EVENT_SUMMARY_ERRORED_LINES = 4 | MSGCLASS_WARNING | MSGCLASS_SUMMARY,
	/*! Summary diagnostic - unconditional #error lines were output */
	EVENT_SUMMARY_ERROR_OUTPUT = 8 | MSGCLASS_WARNING  | MSGCLASS_SUMMARY,
	/*! All valid summary values are less than this */
	EVENT_SUMMARY_MAX = 16,
	/*! &-ed with code extracts summary diagnostics */
	EVENT_SUMMARY_MASK = EVENT_SUMMARY_MAX - 1,
	/*! The progress messages bitfield is offset this amount */
	PROGRESS_SHIFT = 4,
	/*! Report file being processed */
	PROGRESS_PROCESSING_FILE = (45 << PROGRESS_SHIFT) | MSGCLASS_PROGRESS,
	/*! Report entering directory  */
	PROGRESS_ENTERING_DIR = (46 << PROGRESS_SHIFT) | MSGCLASS_PROGRESS,
	/*! Report leaving directory  */
	PROGRESS_LEAVING_DIR = (47 << PROGRESS_SHIFT) | MSGCLASS_PROGRESS,
	/*! Report building the input tree */
	PROGRESS_BUILDING_TREE = (49 << PROGRESS_SHIFT) | MSGCLASS_PROGRESS,
	/*! Report searching a directory for input files */
	PROGRESS_SEARCHING_DIR = (50 << PROGRESS_SHIFT) | MSGCLASS_PROGRESS,
	/*! Report added a file to input */
	PROGRESS_ADDED_FILE = (51 << PROGRESS_SHIFT) | MSGCLASS_PROGRESS,
		/*! Report added files from a directory */
	PROGRESS_ADDED_DIR = (52 << PROGRESS_SHIFT) | MSGCLASS_PROGRESS,
		/*! Report total input files found */
	PROGRESS_FILE_TALLY = (53 << PROGRESS_SHIFT) | MSGCLASS_PROGRESS,
		/*! Report a commandline argument parsed */
	PROGRESS_GOT_OPTIONS = (54 << PROGRESS_SHIFT) | MSGCLASS_PROGRESS,


	/*! The progress summaries bitfield is offset this amount. */
	PROGRESS_SUMMARY_SHIFT = 4,
		/*! Report total files reached & not reached */
	PROGRESS_SUMMARY_FILES_REACHED =
		(55 << PROGRESS_SUMMARY_SHIFT) | MSGCLASS_INFO | MSGCLASS_SUMMARY,
		/*! Report total files abandoned due to errors */
	PROGRESS_SUMMARY_FILES_DROPPED =
		(56 << PROGRESS_SUMMARY_SHIFT) | MSGCLASS_INFO | MSGCLASS_SUMMARY,
	PROGRESS_SUMMARY_ALL_DONE =
		(57 << PROGRESS_SUMMARY_SHIFT) | MSGCLASS_PROGRESS | MSGCLASS_SUMMARY,

	/*! The gripes bitfield is offset this amount */
	GRIPE_SHIFT = 4,
	/*! Duplicate diagnoistic filters are specified with the \c --gag
	 * option
	 */
	GRIPE_DUPLICATE_MASK = (27 << GRIPE_SHIFT) | MSGCLASS_INFO,
	/*! Input file or directory is symbolic link.
	*/
	GRIPE_SYMLINK = (48 << GRIPE_SHIFT) | MSGCLASS_INFO,
	/*! --replace option is redundant with --symbols
	*/
	GRIPE_REDUNDANT_OPTION = (49 << GRIPE_SHIFT) | MSGCLASS_WARNING,

	/*! The same argument occurs for multiple \c --define or \c
	 *	--undefine options
	 */
	GRIPE_DUPLICATE_ARGS = (1 << GRIPE_SHIFT) | MSGCLASS_WARNING,
	/*! An input \c #def or \c #undef directive was deleted on
	 * 	output because it contradicts a \c --define or \c --undefine
	 *	option together with \c --contradict \c delete
	 */
	GRIPE_DELETED_CONTRADICTION = (2 << GRIPE_SHIFT) | MSGCLASS_WARNING,
	/*! An input \c #def or \c #undef directive was commented out on
	 *	output because it contradicts a \c --define or \c --undefine
	 *	option togther with \c --contradict \c comment
	 */
	GRIPE_COMMENTED_CONTRADICTION = (3 << GRIPE_SHIFT) | MSGCLASS_WARNING,
	/*! An input \c #def or \c #undef directive was converted to an
	 *	#error on output because it contradicts a \c --define or \c --undefine
	 *	option \c --contradict \c error
	 */
	GRIPE_ERRORED_CONTRADICTION = (4 << GRIPE_SHIFT) | MSGCLASS_WARNING,
	/*! An \c #error directive was input unconditionally, i.e.
	 *	not in the scope of any \c #if
	 */
	GRIPE_UNCONDITIONAL_ERROR_INPUT = (5 << GRIPE_SHIFT) | MSGCLASS_WARNING,
	/*! An \c #error directive was output unconditionally, i.e.
	 *	not in the scope of any \c #if
	 */
	GRIPE_UNCONDITIONAL_ERROR_OUTPUT = (6 << GRIPE_SHIFT) | MSGCLASS_WARNING,
	/*! Garbage text was input following a \c # directive
	 */
	GRIPE_GARBAGE_AFTER_DIRECTIVE = (7 << GRIPE_SHIFT) | MSGCLASS_WARNING,
	/*! More than \c MAXMASKS diagnostic filters are specified with the
	 *	\c --gag option
	 */
	GRIPE_TOO_MANY_MASKS = (28 << GRIPE_SHIFT) | MSGCLASS_WARNING,
	/*! The \c --verbose option is mixed with the \c --gag option */
	GRIPE_VERBOSE_ONLY = (29 << GRIPE_SHIFT) | MSGCLASS_WARNING,
	/*! A divide by zero was found in an expression */
	GRIPE_ZERO_DIVIDE = (38 << GRIPE_SHIFT) | MSGCLASS_WARNING,
	/*! Directory name ignored on input when \c --recurse not
		not specified */
	GRIPE_DIR_IGNORED = (41 << GRIPE_SHIFT) | MSGCLASS_WARNING,
	/*! An integer constant evaluates > INT_MAX */
	GRIPE_INT_OVERFLOW = (60 << GRIPE_SHIFT | MSGCLASS_WARNING),
	/* MAX REASON = 60. When you add a new gripe, you must give it
		it the MAX GRIPE gripe number, increment MAX REASON in this
		comment and move this comment adjacent to your new gripe
	   The maximum reason */

	/*! A symbol that evaluates to an empty string is an
		operand in an expression */
	GRIPE_EMPTY_SYMBOL = (58 << GRIPE_SHIFT) | MSGCLASS_WARNING,
	/*! An orphan \c #elif was found in input */
	GRIPE_ORPHAN_ELIF = (8 << GRIPE_SHIFT) | MSGCLASS_ERROR,
	/*! An orphan \c #else was found in input */
	GRIPE_ORPHAN_ELSE = (9 << GRIPE_SHIFT) | MSGCLASS_ERROR,
	/*! An orphan \c #endif was found in input */
	GRIPE_ORPHAN_ENDIF = (10 << GRIPE_SHIFT) | MSGCLASS_ERROR,
	/*! Unexpected end of input */
	GRIPE_EOF_TOO_SOON = (11 << GRIPE_SHIFT) | MSGCLASS_ERROR,
	/*! An obfuscated \c # directive was found on input */
	GRIPE_OBFUSC_DIRECTIVE = (12 << GRIPE_SHIFT) | MSGCLASS_ERROR,
	/*! Input ends within a comment */
	GRIPE_EOF_IN_COMMENT = (13 << GRIPE_SHIFT) | MSGCLASS_ERROR,
	/*! Input ends within quotation */
	GRIPE_EOF_IN_QUOTE = (14 << GRIPE_SHIFT) | MSGCLASS_ERROR,
	/*! The commandline does not specify anything to do */
	GRIPE_NOTHING_TO_DO = (15 << GRIPE_SHIFT)  | MSGCLASS_ERROR,
	/*! The commandline invalidly specifies multiple input files without
	 *	the \c --replace option
	 */
	GRIPE_ONE_FILE_ONLY = (16 << GRIPE_SHIFT)  | MSGCLASS_ERROR,
	/*! A newline occurs within quotation in input */
	GRIPE_NEWLINE_IN_QUOTE = (17 << GRIPE_SHIFT)  | MSGCLASS_ERROR,
	/*! A newline is missing at end of input */
	GRIPE_MISSING_EOF_NEWLINE = (18 << GRIPE_SHIFT)  | MSGCLASS_ERROR,
	/*! The commandline options are not a valid combination */
	GRIPE_INVALID_ARGS = (19 << GRIPE_SHIFT)  | MSGCLASS_ERROR,
	/*! The commandline is syntactically invalid */
	GRIPE_USAGE_ERROR = (20 << GRIPE_SHIFT)  | MSGCLASS_ERROR,
	/*! \c #if nesting exceeds \c MAXDEPTH in input */
	GRIPE_TOO_DEEP = (21 << GRIPE_SHIFT)  | MSGCLASS_ERROR,
	/*! The \c --file option occurs more than once */
	GRIPE_MULTIPLE_ARGFILES = (31 << GRIPE_SHIFT) | MSGCLASS_ERROR,
	/*! The argument to a \c --define or \c --undefine option
	 *	contains mysterious characters.
	 */
	GRIPE_GARBAGE_ARG = (32 << GRIPE_SHIFT) | MSGCLASS_ERROR,
	/*! A valid preprocessor identifier was not found where required */
	GRIPE_NOT_IDENTIFIER = (33 << GRIPE_SHIFT) | MSGCLASS_ERROR,
	/*! Unbalanced parenthesis */
	GRIPE_UNBALANCED_PAREN = (59 << GRIPE_SHIFT) | MSGCLASS_ERROR,

	/*! Cannot open an input file */
	GRIPE_CANT_OPEN_INPUT = (22 << GRIPE_SHIFT) | MSGCLASS_ABEND,
	/*! Out of memory */
	GRIPE_OUT_OF_MEMORY = (23 << GRIPE_SHIFT) | MSGCLASS_ABEND,
	/*! A filename exceeds the maximum length for the environment  */
	GRIPE_FILENAME_TOO_LONG = (24 << GRIPE_SHIFT) | MSGCLASS_ABEND,
	/*! End of input encountered while reading a quoted filename.
		A closing quotation was not found */
	GRIPE_EOF_IN_FILENAME = (25 << GRIPE_SHIFT) | MSGCLASS_ABEND,
	/*! Internal logic error */
	GRIPE_CONFUSED = (26 << GRIPE_SHIFT) | MSGCLASS_ABEND,
	/*! Read error on input */
	GRIPE_CANT_READ_INPUT = (30 << GRIPE_SHIFT) | MSGCLASS_ABEND,
	/*! Write error on output */
	GRIPE_CANT_WRITE_FILE = (39 << GRIPE_SHIFT) | MSGCLASS_ABEND,
	/*! Can't identify file or directory */
	GRIPE_NO_FILE = (40 << GRIPE_SHIFT) | MSGCLASS_ABEND,
	/*! Invalid filename input */
	GRIPE_ILLEGAL_FILENAME = (34 << GRIPE_SHIFT) | MSGCLASS_ABEND,
	/*! Failure to delete a file */
	GRIPE_CANT_DELETE_FILE = (35 << GRIPE_SHIFT) | MSGCLASS_ABEND,
	/*! Failure to rename a file */
	GRIPE_CANT_RENAME_FILE = (36 << GRIPE_SHIFT) | MSGCLASS_ABEND,
	/*! Cannot create a temporary file */
	GRIPE_NO_TEMPFILE = (37 << GRIPE_SHIFT) | MSGCLASS_ABEND,
	/*! Cannot open directory */
	GRIPE_CANT_OPEN_DIR = (42 << GRIPE_SHIFT) | MSGCLASS_ABEND,
	/*! Read error on directory */
	GRIPE_CANT_READ_DIR = (43 << GRIPE_SHIFT) | MSGCLASS_ABEND

} reason_code_t;

/*! Arbitary codes to select the behaviour of the \c debug() function */
typedef enum {
	DBG_1 = 1,
	DBG_2,
	DBG_3,
	DBG_4,
	DBG_5,
	DBG_6,
	DBG_8,
	DBG_7,
	DBG_9,
	DBG_10,
	DBG_11,
	DBG_12,
	DBG_13,
	DBG_14,
	DBG_15,
	DBG_16,
	DBG_17,
	DBG_18,
	DBG_19,
	DBG_20,
	DBG_21,
	DBG_22
} dbg_code_t;

/*! Set or clear event summary or severity flags in the exit status.
	\param bits		The bits to be set or cleared
	\param set		If \em true, then \em bits are set in the
					 	exit status; otherwise they are cleared.
 */
extern void
set_exit_flags(int bits, bool set);

/*! Query the current event summary and severity flags.
	\param bits		The bits to be queried.
	\return A bit mask consisting of those bits that are set in \em bits
	and also in the exit status.
 */
extern int
get_exit_flags(int bits);

/*! Convert the internal exit status to an exit code for \c exit(),
	manipulating the bits so that the exit diagnostic bits are
	returned in the low order byte.
	\return RC such that:

	- RC = 0:			MSGCLASS_NONE: No diagnostics accrued.

	- RC & 0x2:		MSGCLASS_INFO: 	At worst info diagnostics accrued.

	- RC & 0x4:		MSGCLASS_WARNING: At worst warnings accrued.

	- RC & 0x8:		MSGCLASS_ERROR:	At worst errors accrued.

	- RC & 0x16:		MSGCLASS_ABEND:	Execution was aborted.

	- RC & 0x12:		EVENT_SUMMARY_DROPPED_LINES (INFO): Input lines were dropped on
	 				output.

	- RC & 0x22:	EVENT_SUMMARY_CHANGED_LINES (INFO): Input lines were changed on
					output.

	- RC & 0x44:	EVENT_SUMMARY_ERRORED_LINES (WARNING): Lines were changed to
					\c #errors on output.

	- RC & 0x84:	EVENT_SUMMARY_ERROR_OUPUT (WARNING): Unconditional \c #errors were
					output.
 */
extern int exitcode(void);

/*! Get the length of a string excluding any final newline.
 *	\param str	The string to be measured.
 *	\return The length of \c str excluding any final newline.
 */
extern size_t
line_len(char const *str);

/*!	Convert embedded linefeeds and tabs in string line to spaces for tidy
	printing in diagnostics.
	\param	text	String putatively containing embedded linefeeds or tabs.
*/
extern void
flatten_line(char * text);


/*!	Produce a diagnostic message.

	\param reason	The reason code of the diagnostic
	\param bufp 	Either \c NULL or the address of a pointer at
					which storage shall be allocated to store the
 					diagnostic.
	\param format	Either \c NULL or a \c printf format from
	which the diagnostic may be composed together with the
	ensuing unspecified arguments.

	The diagnostic is prefixed with the program name, and then
	the current input filename and line number, if any, that provoke the
	diagnostic.

	Then follows one of:

	- <tt>progress \em reason</tt>
	- <tt>info \em reason</tt>
	- <tt>warning \em reason</tt>
	- <tt>error \em reason </tt>
	- <tt>abend \em reason </tt>

depending on the message class of \em reason.

Then follows the body of diagnostic composed from \em format and
subsequent args.

Finally, if the diagnostic refers to an output line that is
within the scope of a true \c #if condition from input line \c N
at nesting depth \c D, a parenthesis is written of the form:

<tt>(if-line N, depth D)</tt>

The way in which production of the diagnostic is determined as
follows:

If \em format is NULL then \em buf is assumed to be the address
of a pointer to storage in which a previous call to the function
has composed a diagnostic that is now to be output. In this case
the diagnostic at that pointer is written to stderr, the pointer
at \em buf is freed and set == NULL.

Otherwise, if <tt>\em buf == NULL then a diagnostic composed from
\em format and subsequent arguments is written to stderr.

Otherwise, if the pointer addressed by \em buf is not NULL, it
is freed. Then a diagnostic composed from \em format
and subsequent arguments is composed in heap storage and the
address of this storage is stored at \em buf. If is the
responsibility of the caller to ensure that this storage is
freed.
*/

extern void
report(reason_code_t reason, heap_str *buf, const char *format,...);

/*! Exit the program on a fatal parsing error with a diagnostic message.
 *	\param reason	The reason code for the error.
 *	\param format	A \c printf format from which to compose the diagnostic
 *	together with subsequent arguments.
 */
extern void
parse_error(int reason, const char *format,...);

/*! Write a diagnostic on \c stderr when the same symbol occurs twice
 *	as an argument among the \c --define and \c --undefine options.
 *
 *	\param sym	The start of symbol that occurs twice.
 *	\param symend	The end of the symbol that occurs twice.
 *	\param symind The index of the symbol in the symbol table.
 *
 *	If the argument occurs only for more than one \c --define or for more than
 *  one \c --undefine, then these options are merely duplicated and the
 *  diagnostic is a warning.
 *
 *	If the argument occurs for a \c --define and also an \c --undefine,
 *	then these options are contradictory and the diagnostic is fatal.
 *
 *	If contradictory options are detected the function calls \c exit().
 */
extern void
bad_args_error(char const *sym, char const *symend, int symind);

/*! Exit the program on an internal logic error with an aplogetic message.
 *	The function is called if the program recognises that it
 *	it has lost its bearings.
 */
extern void
give_up_confused(void);

/*! Exit the program with a specified return code and diagnostic
 *	message.
 *  The function interface is the same as that of \c bail(), with
 *	the exception that the parameter <tt>va_list argp</tt> is a
 *	pointer that the caller has initialised with \c va_start() to
 *	address a list of unspecified arguments.
 *
 *	All functions that terminate the program do so by calling this function
 */
extern void
vbail(int reason, const char *msg, va_list argp);


/*! Exit the program out with a specified return code and
 *	diagnostic message.
 *	\param retcode	The code to be returned by \c exit().
 *	\param format	A \c printf format from which to compose the diagnostic
 *	together with subsequent arguments.
 */
extern void
bail(int retcode, const char *format,...);

/*! Turn debugging output on or off
 *	\param on	If \em true, debugging output is enabled; otherwise disabled.
 */
extern void
debugging(bool on);

/*! Write debugging info on \c stderr.
 * \param how	A code that selects how the function will compose
 *	debugging output from the subsequent arguments.
 */
extern void
debug(dbg_code_t how,...);

/*! Write summary diagnostics on \c stderr at exit.
 *	The summary diagnostics will be output unless \c --gag \c summary
 *	is in force, which it is by default.
 */
extern void
exit_diagnostics(void);

/*! Diagnose an input orphan\c #elif on \c stderr */
extern void
orphan_elif(void);

/*! Diagnose an input orphan \c #else on \c stderr */
extern void
orphan_else(void);

/*! Diagnose an input orphan \c #endif on \c stderr */
extern void
orphan_endif(void);

/*! Diagnose unexpected end of input on \c stderr */
extern void
early_eof(void);

/*! Diagnose an input obfuscated \c # directive on \c stderr */
extern void
obfuscation(void);

/*! Report processing an input file \c stderr */
extern void
processing_file(char const *filename);

/*! Report entering a directory file \c stderr */
extern void
entering_dir(char const *dirname);

/*! Report leaving a directory file \c stderr */
extern void
leaving_dir(char const *dirname);


/*! Compose an output message.
 *
 *	\param		dest	Depending on \c buflen, either a pointer to
				a  \c FILE to which the formatted output shall be
				written, or else a pointer to the address of a
				heap buffer in which to format the output.
	\param		buflen		If \c *buflen < 0 then \c dest is a
				\c FILE*. Otherwise \c *buflen is current length
				of the buffer at \c *dest.
	\param		startoff	If \c *buflen >= 0, \c *startoff is the
				offset into the buffer at which to format the
				message; otherwise	ignored.
	\param		format		The \c printf() format with which to
				compose the message using subsequent arguments.
 *	\return	The length of the formatted message, excluding
				any terminal nul.

	If \c *buflen < 0 then the function writes output to the
	\c FILE at \c dest composed from \c format and subsequent
	arguments.

	Otherwise function attempts to compose output within the
	spare \c *buflen - \c *startoff bytes of the buffer.
	As long as the output will not fit the function reallocates
	the buffer to twice its current size and tries again. The
	final address of the buffer will be stored at \c dest. The
	final length of the buffer will be stored at \c buflen, and
	the address of the terminal nul of the formatted message
	with be stored at \c startoff.

	It is the caller's responsibility to free storage allocated
	at \c *dest.
 */
extern size_t
format_output(void *dest, int *buflen,	int *startoff, char const *format,...);

/*!
	Concatenate strings.

	\param		count The number of strings to concatenate,
	\param		str		The array of strings to concatenate.
	\param		punctch	Optional character with which to
				punctuate the concatenated strings, -1 == none.
	\return	Pointer to heap storage in which the concatenation
				is stored. It is the caller's responsibility to
				free this storage.
*/
extern heap_str
concatenate(size_t count, char *strs[], int punctch);
/*@}*/

/*! \addtogroup report_interface_state_utils */
/*@{*/
IMPORT_INITOR(diagnostic);
IMPORT_FINITOR(diagnostic);
/*@}*/

#endif /* EOF */
