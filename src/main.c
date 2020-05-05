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

#include "report.h"
#include "evaluator.h"
#include "categorical.h"
#include "line_despatch.h"
#include "if_control.h"
#include "args.h"
#include "symbol_table.h"
#include "io.h"
#include "chew.h"
#include "line_edit.h"
#include "platform.h"
#include "exception.h"
#include "dataset.h"

/*! \ingroup main_module
 * \file main.c
 *	This file implments the Sunifdef program
 */

/*! \addtogroup main_module */
/*@{*/

/*! Perform all module initialisations */
static void		ready(void);

/*! Execute a sunifdef process */
static void		process(void);

/*! The main program. */
int
main(int argc, char *argv[])
{
	ready();
	atexit(exit_diagnostics);
	parse_executable(argv);
	parse_args(argc,argv);
	finish_args();
	process();
	give_up_confused(); /* bug */
	return 0;
}

static void
line_debug(int line)
{
	if (line < 0) {
		/* Call with negative value to write line numbers */
		printf("%d\n",GET_PUBLIC(io,line_num));
	}
	else if (line > 0 && line == GET_PUBLIC(io,line_num)) {
		/* Call with positive line number and put breakpoint here ... */
		fputs("Here",stderr);	
	}
}

/*! Initialise all module states */
static void
ready(void)
{
	INITIALISE(chew);
	INITIALISE(diagnostic);
	INITIALISE(evaluator);
	INITIALISE(if_control);
	INITIALISE(io);
	INITIALISE(dataset);
	INITIALISE(line_edit);
	INITIALISE(args);
	INITIALISE(symbol_table);
	INITIALISE(line_despatch);
	INITIALISE(categorical);
}

/*! The \c file_tree_callback_t that is
	iterated over the input file tree.
*/
static void
node_proc(	file_tree_h file_tree,
			char const *name,
			file_tree_traverse_state_t context)
{
	static int error;
	switch(context) {
	case FT_AT_FILE:
		processing_file(name);
		open_io(name);
		if_control_toplevel();
		chew_toplevel();
		exceptions_enabled = GET_PUBLIC(args,keepgoing);
		error = catch();
		for (;!error && !input_eof();) {
			line_type_t lineval;
			line_debug(0);
			lineval = eval_line();
			if (!weed_categorical_directive(lineval)) {
				transition(lineval);
			}
			debug(DBG_19,lineval);
		}
		if (if_depth() && !error && input_eof()) {
			early_eof();
		}
		exceptions_enabled = false;
		close_io(false);
		break;
	case FT_ENTERING_DIR:
		{
			file_tree_count_t count;
			file_tree_count(file_tree,FT_COUNT_CHILDREN | FT_COUNT_ALL,&count);
			/* Don't report entry to a directory if it is merely
				the parent of another */
			if (count.dirs != 1 || count.files != 0) {
				entering_dir(name);
			}
		}
		break;
	case FT_LEAVING_DIR:
		{
			file_tree_count_t count;
			file_tree_count(file_tree,FT_COUNT_CHILDREN | FT_COUNT_ALL,&count);
			/* Don't report leaving a directory if it is merely
				the parent of another */
			if (count.dirs != 1 || count.files != 0) {
				leaving_dir(name);
			}
		}
		break;
	case FT_ENTERING_TREE:
		if (file_tree_is_empty(file_tree)) {
			node_proc(file_tree,STDIN_NAME,FT_AT_FILE);
		}
		break;
	case FT_LEAVING_TREE:
		io_toplevel();
		break;
	default:
		assert(false);
	}
}

static void
process(void)
{
	file_tree_h tree = GET_PUBLIC(dataset,file_tree);
	file_tree_traverse(tree,node_proc);
	exit(exitcode());
}

/*@}*/

/* EOF */
