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

#include "dataset.h"
#include "platform.h"
#include "report.h"

/*!\ingroup dataset_module dataset_interface dataset_internals
 *\file dataset.c
 * This file implements the Dataset module
 */

/*! \addtogroup dataset_internals_state_utils */
/*@{*/
/*! The global state of the Dataset module */
STATE_DEF(dataset) {
	/*! The public state of the Dataset module */
	INCLUDE_PUBLIC(dataset);
	heap_str filter_types;
		/*!< List of filter file types. Stored as
			continguous nul-punctuated strings with a
			terminatied by a double nul*/
} STATE_T(dataset);


IMPLEMENT(dataset,USER_INITABLE);

DEFINE_USER_INIT(dataset)(STATE_T(dataset) * dataset_st)
{
	dataset_st->dataset_public_state.file_tree = file_tree_new();
}

DEFINE_USER_FINIS(dataset)(STATE_T(dataset) * dataset_st)
{
	file_tree_dispose(&(SET_PUBLIC(dataset,file_tree)));
	free(SET_STATE(dataset,filter_types));
}
/*@}*/

/*! \addtogroup dataset_internals */
/*@{*/

/*! Say whether a file is eligible for processing
	by matching one of the file extensions given by
	the \c --filter option.
*/
static bool
filter_filename(const char *filename)
{
	char const * filter_filetypes = GET_STATE(dataset,filter_types);
	char const *leafname = strrchr(filename,PATH_DELIM);
	if (!leafname) {
		leafname = filename;
	}
	if (filter_filetypes) {
		char const *extension = filter_filetypes;
		size_t ext_len = 0;
		for (	;*extension; extension += ext_len + 1) {
			char *posn = strrchr(leafname,'.');
			ext_len = strlen(extension);
			if (posn && posn[1]) {
				if (!strcmp(++posn,extension)) {
					return true;
				}
			}
		}
	}
	return false;
}

static void
build_proc(	file_tree_h tree,
			char const *name,
			file_tree_traverse_state_t context)
{
	switch(context) {
	case FT_ENTERING_DIR:
		report(PROGRESS_SEARCHING_DIR,NULL,"Searching dir \"%s\"",name);
		break;
	case FT_AT_FILE:
		report(PROGRESS_ADDED_FILE,NULL,"Added file \"%s\"",name);
		break;
	case FT_LEAVING_DIR:
		report(	PROGRESS_ADDED_DIR,
					NULL,
					"Added %d files from dir \"%s\"",
					file_tree_count(tree,FT_COUNT_FILES,NULL),
					name);
		break;
	default:
		assert(false);	
	}
}

/*@}*/

/* API */

void
dataset_filter_filetypes(const char *optarg)
{
	size_t len = strlen(optarg);
	heap_str list = SET_STATE(dataset,filter_types) = allocate(len + 2);
	strcpy(list,optarg);
	list[len + 1] = '\0';
	while(--len) {
		if (list[len] == ',') {
			list[len] = '\0';
		}
	}
	file_tree_set_filter(GET_PUBLIC(dataset,file_tree),filter_filename);
}

void
dataset_add(char const *path)
{
	file_tree_add(GET_PUBLIC(dataset,file_tree),path,build_proc);
}


/* EOF */
