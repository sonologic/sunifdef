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

#include "filesys.h"
#include "platform.h"
#include "report.h"
#include <ctype.h>
#include <stdio.h>

/*! \file filesys.c
 * \ingroup filesystem_module
 *
 * This file implements the Filesystem module
 */

heap_str
fs_compose_filename(char const *path, char const *leafname)
{
	size_t path_len = path ? strlen(path) : 0;
	size_t leafname_len = strlen(leafname);
	heap_str filename = allocate(path_len + sizeof(PATH_DELIM) +
							leafname_len + 1);
	if (path) {
		strcpy(filename,path)[path_len++] = PATH_DELIM;
	}
	strcpy(filename + path_len,leafname);
	return filename;
}

fs_obj_type_t
fs_file_or_dir(char const *name)
{
	fs_obj_type_t obj_type = fs_obj_type(name);
	if (obj_type == FS_OBJ_NONE) {
		bail(GRIPE_NO_FILE,"No such file or directory as \"%s\"",name);
	}
	return obj_type;
}

char *
fs_tempname(char * template)
{
	char *tempname = NULL;
	char *suffix = strstr(template,"XXXXXX");
	if (suffix) {
		if (!suffix[6]) {
			unsigned lim = 0xffffff + 1;
			unsigned i = 0;
			for (	;i < lim; ++i) {
				sprintf(suffix,"%06x",i);
				if (fs_obj_type(template) == FS_OBJ_NONE) {
					tempname = template;
					break;
				}
			}
		}
	}
	return tempname;
}

fs_path_type_t
fs_path_type(char const *filename)
{
	/* This lets the style of the first path delimiter
		in filename determine whether we call it a
		unix path or a windows path, unless windows
		drive prefix has already settled it.
	*/
	fs_path_type_t pathtype = 0;
	char lastch;
	if (filename[0] == '/') {
		pathtype |= FS_ABS_PATH | FS_UNIX_PATH;
		if (!filename[1]) {
			return pathtype |= FS_ROOT_PATH | FS_DANGLING_PATH;
		}
	}
	else if (isalpha(filename[0]) && filename[1] == ':') {
		pathtype |= FS_ABS_PATH | FS_WIN_PATH;
		if (!filename[2]) {
			return pathtype |= FS_ROOT_PATH;
		}
		else if (filename[2] == '\\' && !filename[3]) {
			return pathtype |= FS_ROOT_PATH | FS_DANGLING_PATH;
		}
	}
	else if (strchr(filename,'/')) {
		pathtype |= FS_UNIX_PATH;
	}
	else if (strchr(filename,'\\')) {
		pathtype |= FS_WIN_PATH;
	}
	lastch = filename[strlen(filename) - 1];
	if (lastch == '/' || lastch == '\\') {
		pathtype |= FS_DANGLING_PATH;
	}
	return pathtype;
}

char const *
fs_path_comp(char const *first, char const *second, size_t *sharedlen)
{
	char const *delim = NULL;
	char const *lhs = first;
	char const *rhs = second;
	for (	;*lhs && *lhs == *rhs; ++lhs,++rhs) {
		if (*lhs == PATH_DELIM) {
			delim = lhs;
		}
	}
	if (*lhs != *rhs) {
		if (!*lhs) {
			*sharedlen = strlen(first);
			return first;
		}
		if (!*rhs) {
			*sharedlen = strlen(second);
			return second;
		}
		if (delim) {
			*sharedlen = lhs - first;
			return first;
		}
		*sharedlen = 0;
		return NULL;
	}
	*sharedlen = 0;
	return first;
}

heap_str
fs_split_filename(char const *path, char **leafname)
{
	heap_str parent = NULL;
	char const *child = NULL;
	char *delim = strrchr(path,PATH_DELIM);
	if (!delim) {
		fs_obj_type_t obj_type = fs_file_or_dir(path);
		if (FS_IS_FILE(obj_type)) {
			child = path;
		}
		else {
			parent = allocate(strlen(path) + 1);
			strcpy(parent,path);
		}
	}
	else {
		size_t len = delim - path;
		parent = allocate(len + 1);
		memcpy(parent,path,len);
		child = delim + 1;
	}
	if (leafname) {
		*leafname = delim;
	}
	return parent;
}



/* EOF */
