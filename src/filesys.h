#ifndef FILESYS_H
#define FILESYS_H
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

/*! \file filesys.h
 * \ingroup filesystem_module filesystem_interface
 *
 * This file provides the Filesystem module interface
 */

/*!\addtogroup filesystem_interface */
/*@{*/

/*! Abstract type of directory handle */
typedef void * fs_dir_t;


/*! Enumerated type of filesystem objects */
typedef enum {
	FS_OBJ_NONE,	/*!< No such object */
	FS_OBJ_SLINK = 1,	/*!< Object is a symbolic link */
	FS_OBJ_FILE = 2,	/*!< Object is a file */
	FS_OBJ_DIR = 4		/*!< Object is a directory */
} fs_obj_type_t;

/*! Enumerated attributes of pathnames as returned by \em fs_path_type() */
typedef enum {
	FS_ABS_PATH = 1 ,		/*!< Absolute path */
	FS_UNIX_PATH = 2,		/*!< Unix path */
	FS_WIN_PATH = 6,	    /*!< Windows path */
	FS_DANGLING_PATH = 8,	/*!< Path has trailing PATH_DELIM */
	FS_ROOT_PATH = 16		/*!< Path represents files system root */ 
} fs_path_type_t;

/*!
	The path type \e pathtype denotes an absolute path.
*/
#define fs_absolute_path(pathtype) \
	(((pathtype) & FS_ABS_PATH) == FS_ABS_PATH)

/*!
	The path type \e pathtype denotes a unix path.
*/
#define fs_unix_path(pathtype) \
	(((pathtype) & FS_UNIX_PATH) == FS_UNIX_PATH)

/*!
	The path type \e pathtype denotes a windows path.
*/
#define fs_windows_path(pathtype) \
	(((pathtype) & FS_WIN_PATH) == FS_WIN_PATH)

/*!
	The path type \e pathtype denotes a dangling path
	(with trailing path delimiter)
*/
#define fs_dangling_path(pathtype) \
	(((pathtype) & FS_DANGLING_PATH) == FS_DANGLING_PATH)

/*!
	The path type \e pathtype denotes a root path
	of the filesystem (\c "/" for unix, drive paths for
	windows.
*/
#define fs_root_path(pathtype) \
	(((pathtype) & FS_ROOT_PATH) == FS_ROOT_PATH)

/*!
	The object type \e objtype indicates a file
*/
#define FS_IS_FILE(objtype) (((objtype) & FS_OBJ_FILE) != 0)

/*!
	The object type \e objtype indicates a directory
*/
#define FS_IS_DIR(objtype) (((objtype) & FS_OBJ_DIR) != 0)

/*!
	The object type \e objtype indicates a symbolic link
*/
#define FS_IS_SLINK(objtype) (((objtype) & FS_OBJ_SLINK) != 0)

/*! Return the type of the object putatively designated
		by a filename
*/
extern fs_obj_type_t
fs_obj_type(char const *name);

/*! Return the type of the putative file or directory
	designated by a filename. Bail if neither file nor
	directory.
*/
extern
fs_obj_type_t
fs_file_or_dir(char const *name);


/*! Open an abstract directory handle through which
	the contents of a directory can be traversed.

	\param		dirname The name of the directory to be
	opened.
	\param		parent. The handle of the open directory of
				\em dirname is a subdirectory, or NULL if none.
				

	\return	A handle to \em dirname, or NULL if an error
	occurs, which will be indicated	by \c errno.
*/
extern fs_dir_t
fs_open_dir(char const *dirname, fs_dir_t const parent);

/*! Close an abstract directory handle, releasing
	its resources.
*/
extern void
fs_close_dir(fs_dir_t dir);

/*! Get the name of the next entry in
	a directory.

	\param		dir		The open directory handle to read.
	\param		fullname	NULL, or a pointer at which
				the function will store the address of
				the absolute name of the entry returned.
				This pointer is only valid until the next
				call or until \e fs_close_dir(dir)
	\return	A pointer to the next entry in the directory, if
				any; otherwise NULL.
		The pointer is only valid until the next call or
		until \e fs_close_dir(dir)

	The function ignores the entries '.' and '..'.

*/
extern char const *
fs_read_dir(fs_dir_t dir, char const ** fullname);


/*! Return the handle of the open parent directory of
	a directory, if any, else NULL.
*/
extern fs_dir_t
fs_get_parent(fs_dir_t dir);

/*! Compose a filename from a path and a leafname.
	If either \e path or \e leafname is quoted or
	contains spaces then the resulting filename is
	quoted.

	\return The composed filename in a string allocated
	on the heap. It is the caller's responsiblity to
	free this string.
*/
extern heap_str
fs_compose_filename(char const *path, char const *leafname);

/*! Split at filename into its parent directory component
	and its terminal component.

	\param		path	Path to be split
	\param		leafname	NULL, or an address at which the
				function will store the address of the
				terminal component of \em path.

	\return The parent directory component of \em path
	allocated in a string on the heap. It is the caller's
	responsibility to free this string.

	If \em path has only one component then, if that
	component is a directory, it is returned and NULL
	will be stored at \em leafname; if the one component
	is a file then NULL is returned 
*/
extern heap_str
fs_split_filename(char const *path, char **leafname);

/*! Return the absolute real pathname of a
	file or directory name (which may be the name of
	a symbolic link).

	\param		relname	Possibly relative file or
							directory name.
	\param		namelen	If non-NULL then the length of
				the absolute real name of \c relname
				is stored here at return.

	\return The absolute real pathname of \c relname
	in a string allocated	on the heap. It is the caller's
	responsiblity to free this string. NULL is returned
	if an error occurs.
*/
extern heap_str
fs_real_path(char const *relname, size_t *namelen);

/*! Return the absolute pathname of the current entry
	from a directory handle.

	\param		dir			Directory handle
	\param		entry	NULL, or an address at which the
				function shall store a pointer to the terminal
				portion of the current full pathname that
				follows the full pathname of \em dir. 
	\return Pointer to the absolute pathname of the
			current entry in \em dir.

	The current entry in \em dir is the entry read by the last
	call to \em fs_read_dir(dir), if any. If \em fs_read_dir(dir)
	has not yet been called, or if the last call returned NULL,
	then the returned pointer addresses the absolute
	name of \em dir and, if \em leafname is not NULL, then
	\em *leafname[0] will be 0.

	Otherwise, if \em leafname is not NULL then \em *leafname
	will point to the last component of the current absolute
	pathname commencing with '/' (unix) or '\' (windows) 

*/
extern char const *
fs_cur_dir_entry(fs_dir_t dir, char const **entry);


/*! Get the type of a pathname */
extern fs_path_type_t
fs_path_type(char const *filename);

/*! Create a tempory filename from a template
	\param		template A pathname pattern from which to compose
				the temporary filename.
	\return	A pointer to the temporary filename created,
				or NULL if a temporary file satisfying template
				cannot be created.

	The writable string \c template must terminate with \c XXXXXX
	This suffix will be replaced, if possible, with a string of
	characters to compose a filename different from that of
	any existing file. 
*/
extern char *
fs_tempname(char * template); 

/*!		Compare two pathnames.
	\param		first		The first pathname.
	\param		second		The second pathname.
	\param		sharedlen		Address of an integer at which the
				function shall store the length of any common
				initial component of \em first and \em second
				unless they are identical pathnames, in which
				case 0 shall be stored at \em sharedlen.

	\return	Pointer to the common initial component of
				\em first and \em second, or NULL if there is none.

	If the returned value \em cp is non-NULL then it is equal to
	either \em first or to \em second. Then:

	- If \em *sharedlen != 0 and \em cp[*sharedlen] == 0, then
	returned pathname is entirely an intitial component of the
	other pathname.
	
	- If \em *sharedlen != 0 and \em cp[*sharedlen] != 0 then
	the \em *sharedlen bytes at \em cp comprise the common
	initial component of \em first and \em second.

	- If \em *sharedlen == 0 then \em first and \em second
	are identical pathnames.

	If the returned value is NULL then \em first and \em second
	are wholly disjoint pathnames.

*/
extern char const *
fs_path_comp(char const *first, char const *second, size_t *sharedlen);

/* @) */
#endif /* EOF */
