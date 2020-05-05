#ifndef ARGS_H
#define ARGS_H
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
#include "file_tree.h"
#include "ptr_vector.h"


/*!\ingroup args_module args_module_interface
 *\file args.h
 * This file provides the Args module interface
 */

/*!\addtogroup args_interface */
/*@{*/
/*! The maximum supported number of diagnostic filters.
 */
#define MAXMASKS 		64

/*! Enumeration of policies for discarding lines */
typedef enum {
	DISCARD_DROP,	/*!< Drop discarded lines */
	DISCARD_BLANK,	/*!< Blank discarded lines */
	DISCARD_COMMENT /*!< Comment discarded lines */
} discard_policy_t;

/*! Enumeration of policies for discarding lines */
typedef enum {
	SYMBOLS_NO,	/*!< Not listing symbols */
	SYMBOLS_LIST = 1,	/*!< Listing symbols */
	SYMBOLS_LIST_FIRST = 3,	/*!< List symbol only once */
	SYMBOLS_LOCATE = 4	/*!< List symbol file and line location */
} symbols_policy_t;


/*! \fn void parse_executable(char **argv);
 * Parse the full and short names of the executable.
 *	\param argv		The array of commandline arguments.
 *
 *	Element 0 of \em argv is parsed to find the complete
 *	pathname and filename of our executable and the addresses
 *	of these strings are stored respectively at \c exec_path and
 *	\c prog_name in the module's state.
 */
extern void
parse_executable(char **argv);

/*! Parse the commandline.
 *	\param argc		The number of commandline arguments.
 *	\param	argv	Array of commandline arguments.
 *
 *	Populate the module's state by parsing the commandline arguments.
 */
extern void
parse_args(int argc, char *argv[]);

/*! Analyse the module's state after parsing the commandline,
 *	diasgnose any errors and draw final conclusions.
 */
extern void
finish_args(void);

/*@}*/

/*!\ingroup args_interface_state_utils */
/*@{*/

/*! The public state of the Args module
 */
PUBLIC_STATE_DEF(args) {
	char	*exec_path;
		/*!< argv[0] */
	char	*prog_name;
		/*!< Filename element of \c exec_path */
	char	*backup_suffix;
		/*!< Suffix for backup files */
	bool	got_opts;
		/*!< Are we finished parsing the commandline? */
	bool	replace;
		/*!< Do we replace input files with output files? */
	symbols_policy_t symbols_policy;
		/*!< Are we just to list <tt>#if</tt>-controlling symbols?
			If so, how?
		*/
	bool	complement;
		/*!< Are to output lines instead of dropping tem and vice versa? */
	bool	eval_consts;
		/*!< Do we evaluate constants in <tt>#if</tt>s or treat them as
			unknowns? */
	bool	del_consts;
		/*!< Do we delete evaluated contants? */
	discard_policy_t	discard_policy;
		/*!< Policy for discarding lines */
	bool	line_directives;
		/*!< Do we output #line directives? */
	bool	plaintext;
		/*!< Are we to omit parsing for comments? */
	bool	recurse;
		/*!< Recurse into directories? */
	bool	keepgoing;
		/*!< Continue to process input files after errors */
	int		diagnostic_filter;
		/*!< Bitmask of diagnostic filters */
} PUBLIC_STATE_T(args);

IMPORT(args);
/*@}*/


#endif /* EOF */
