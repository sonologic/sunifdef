#ifndef DATASET_H
#define DATASET_H
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

/*!\ingroup dataset_module dataset_interface
 *\file dataset.h
 * This file provides the Dataset module interface.
 */

/*!	\addtogroup dataset_interface */
/*@{*/


/*!	 Parse the argument to the \c --filter option - a comma
	separated list of file extensions - and store these
	extensions internally for filtering files added to
	the input dataset.
 */
void
dataset_filter_filetypes(const char *list);

/*!
	Add files to the input dataset.

	\param		path		Name of file or directory to
							be included in the input dataset.

	If \e path is a file that satisfies any \c --filter option
	it is added to the input dataset.

	If \e path is a directory then files recursively beneath it
	that satisfy any \c --filter option are added to the input
	dataset.
*/
void
dataset_add(char const *path);

/*@}*/

/*!\addtogroup dataset_interface_state_utils */
/*@{*/

/*! * The public state of the Dataset module.*/
PUBLIC_STATE_DEF(dataset) {
	file_tree_h file_tree;
		/*!< File tree of input files */
	unsigned int donefiles;
		/*!< Number of files processed, including those
			abandoned due to parse errors */
	unsigned int errorfiles;
		/*!< Number of files abandoned due to parse errors */
} PUBLIC_STATE_T(dataset);

IMPORT(dataset);
/*@}*/

#endif /* EOF */
