#ifndef FILE_TREE_H
#define FILE_TREE_H
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
#include "memory.h"

/*!\ingroup file_tree_module file_tree_interface
 *\file file_tree.h
 * This file provides the File Tree module interface.
 */

/*!	\addtogroup file_tree_interface */
/*@{*/

/*!
	Symbolic constant denoting the index of the last child
	of a file tree.
*/
#define FT_LAST		((size_t)-1)

/*! Enmeration of the states of file tree traversal. */
typedef enum {
	FT_ENTERING_TREE = -2,	/*! Entering file tree */
	FT_ENTERING_DIR	= -1,	/*!< Entering directory */
	FT_AT_FILE = 0,		/*!< At a file */
	FT_LEAVING_DIR	= 1,	/*!< Leaving directory */
	FT_LEAVING_TREE = 2		/*! Leaving file tree */
} file_tree_traverse_state_t;

/*! Bit flags that may be combined in flags passed to
	\c file_tree_count() to select the nodes counted.
*/
enum file_tree_count_flags {
	FT_COUNT_FILES = 1,	/*!< Count only files */
	FT_COUNT_DIRS = 2,	/*!< Count only directories */
	FT_COUNT_ALL = FT_COUNT_FILES | FT_COUNT_DIRS, /*!< Count everything */
	FT_COUNT_CHILDREN = 4 /*!< Only count immediate children.*/
};

/*! Abstract type of file tree */
typedef struct file_tree * file_tree_h;


/*! Type of structure optionally passed to \em file_tree_count()
	in which to store a break-down of the counted objects */
typedef struct {
	size_t files;		/*!< Files counted, with \c FT_COUNT_FILES */
	size_t dirs;		/*!< Directories counted, with \c FT_COUNT_DIRS */
	size_t children;	/*!< Number of immediate children counted,
							with FT_COUNT_CHILDREN */
} file_tree_count_t;

/*! Type of callabck function to be iterated over a file tree.

	\param tree	The current file tree.
	\param name 	The pathame of the file or directory 
	\param context  Indicator of calling context:-
	FT_ENTERING_TREE => Starting traversal. \e name is NULL
	FT_ENTERING_DIR	=> Entering directory \e name
	FT_AT_FILE => \e name is a file
	FT_LEAVING_DIR => Entering directory \e name
	FT_LEAVING_TREE => Finished traversal. \e name is NULL

 */

typedef void (*file_tree_callback_t)(	file_tree_h tree,
										char const *name,
										file_tree_traverse_state_t context);

/*! Type of functions for filtering files by leafname.
	A file tree contains a function pointer of this type that
	is used to decide the eligibility of files (not directories)
	for insertion in the file tree. If no such function is
	assigned to a file tree then the default filter accepts
	all files.

	\param		leafname	The leafname of the file.
	\return	True iff the leafname satisfies the filter.	
*/
typedef bool (*file_filter_t)(char const *leafname);


/*! Compose the full name of a file tree, i.e.
	the pathname composed from the root name of the
	file tree and the root names of its recursive
	parents.
*/
extern heap_str
file_tree_name(const file_tree_h file_tree);

/*! Construct and return a new file tree.
	The new file tree is empty and may be populated with
	\c file_tree_add(), but this object always represents
	the root of the file system.
*/
extern file_tree_h
file_tree_new(void);

/*! Try to attach a filter function to a file tree, to be used
	to be used to decide the eligibility of files (not directories)
	for insertion in the file tree.

	\param		file_tree		The file tree to which the filter
				will be attached.
	\param		filter			Pointer to the filter function to
				attach.

	\return	True if the filter function can be attached to
				the file tree, else false. A filter can only be
				attached to a file tree when it is	empty.
*/
extern bool
file_tree_set_filter(file_tree_h file_tree, file_filter_t filter);


/*! Add the filtered contents of a path to a file tree.

	\param		parent	 The file tree to which a path is to be
							added.
	\param		path		The path to be added.

	\param		callback	NULL, or \c file_tree_callback_t to be
						called on entry and exit for each directory
						added and for each file added.

	Files in \c path are added to the tree that satisfy the
	assigned or default filter function of the tree. The
	default filter accepts all files.

	If \c path is a file, the function adds the file to \c parent
	if it satisfies the filter and is not already
	present, together with all component directories of \c path
	that are not already present.

	If \c path is a directory and recursively contains any files
	that satisfy the filter and are not already present
	then all these files are added to parent, together with any
	of the directories recursively containing them that are not
	already present.

	The function populates the file tree grudgingly. A new
	node will only be added to any parent when the subtree rooted
	at the new node has been wholly constructed in the same
	grudging manner and found to contain any wanted files.
*/
extern void
file_tree_add(	file_tree_h parent,
				char const *path,
				file_tree_callback_t callback);


/*! Dispose of a file tree, releasing its resources.
	
	\param		file_tree		Pointer to the file tree to be
								destroyed.
*/
extern void
file_tree_dispose(file_tree_h * file_tree);

/*! Iterate a function over a file tree.

	\param		file_tree		The file_tree to be traversed.
	\param		callback		NULL, or \c file_tree_callback_t to be
						called on entry and exit for each directory
						added and for each file traversed.
*/
extern void
file_tree_traverse(	file_tree_h file_tree,
					file_tree_callback_t callback);


/*! Say whether a file tree contains any files or directories.
*/
extern bool
file_tree_is_empty(file_tree_h file_tree);

/*! Count nodes in a file tree
	\param		file_tree		The file tree to be counted.
	\param		flags		A combination of the bitflags in
				\c enum \c file_tree_count_flags controlling
				which nodes are counted.
	\param		counter. NULL, or a pointer to a
				\em file_tree_count_t that is filled in with:-
				- Total files counted, for flag \em FT_COUNT_FILES
				- Total dirs counted, for flag \em FT_COUNT_FILES
				- Immediate children counted, for flag
					\em FT_COUNT_CHILDREN
*/
extern size_t
file_tree_count(	const file_tree_h file_tree, 
					unsigned flags,
					file_tree_count_t * counter);

/*!
	Return a child of a file tree.

	\param		file_tree		The file tree from which a child
				is to be returned.
	\param		which			The index of the required child.
				If \em which == \c FT_LAST then the last child,
				if any, is returned.
	\return	The child of \em file_tree that is indexed by
				\em which, if any, else NULL.
*/
extern file_tree_h
file_tree_child(file_tree_h file_tree, size_t which);

#ifdef DEBUG_FILE_TREE
extern void
file_tree_dump(file_tree_h file_tree);
#endif

/*@}*/

#endif /* EOF */
