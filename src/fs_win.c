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

/*! \file fs_win.c
 * \ingroup filesystem_module filesystem_windows filesystem_windows_internals
 *
 * This file implements the filesystem module for windows.
 */
#include "platform.h"

#ifdef WINDOWS

#include "filesys.h"
#include "memory.h"
#include "report.h"
#include <ctype.h>
#include <stdlib.h>
#include <windows.h>

/*! \addtogroup filesystem_windows_internals */
/*@{*/

/*! Structure implementing \e fs_dir_t for Windows */
typedef struct fs_dir_win {
	/*! Handle of parent directory if any, else NULL. */
	struct fs_dir_win * parent;
	/*! Name of directory */
	heap_str dirname;
	/*! The end of the directory name */
	char *dirname_end;
	/*! Handle returned by FindFirstFile() */
	HANDLE handle;
	/*! Search data updated by FindFirstFile or FindFile() */
	WIN32_FIND_DATA obj_info;
} fs_dir_win_t;


/*@}*/

heap_str
fs_real_path(char const *relname, size_t *namelen)
{
	heap_str fullpath = _fullpath(NULL,relname,MAX_PATH);
	if (namelen) {
		*namelen = strlen(fullpath);
	}
	return fullpath;
}

fs_obj_type_t
fs_obj_type(char const *name)
{
	WIN32_FILE_ATTRIBUTE_DATA obj_info;
	int res = GetFileAttributesEx(name,GetFileExInfoStandard,&obj_info);
	if (res) {
		if (obj_info.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) {
			return FS_OBJ_DIR;
		}
		else {
			return FS_OBJ_FILE;
		}
	}
	return FS_OBJ_NONE;
}

fs_dir_t
fs_open_dir(char const *dirname, fs_dir_t const parent)
{
	fs_dir_win_t * dir = NULL;
	size_t namelen;
	heap_str fullname = fs_real_path(dirname,&namelen);
	if (fullname) {
		HANDLE find_handle;
		WIN32_FIND_DATA obj_info;
		if (fullname[namelen - 1] == PATH_DELIM) {
			--namelen;
		}
		fullname = reallocate(fullname,namelen + 3);
		strcpy(fullname + namelen,"\\*");
		find_handle = FindFirstFile(fullname,&obj_info);
		if (find_handle != INVALID_HANDLE_VALUE) {
			size_t needed;
			dir = allocate(sizeof(fs_dir_win_t));
			dir->parent = (fs_dir_win_t *)parent;
			dir->dirname = fullname;
			dir->handle = find_handle;
			dir->obj_info = obj_info;
			dir->dirname[namelen] = '\0';
			dir->dirname_end = dir->dirname + namelen;
			needed = strlen(dir->obj_info.cFileName) + 1;
			dir->dirname = reallocate(dir->dirname,namelen + needed + 1);
			dir->dirname_end = dir->dirname + namelen;
			*dir->dirname_end = '\0';
		}
	}
	return dir;
}

void
fs_close_dir(fs_dir_t dir)
{
	fs_dir_win_t * win_dir = dir;
	assert(dir);
	if (win_dir->handle != INVALID_HANDLE_VALUE) {
		FindClose(win_dir->handle);
	}
	free(win_dir->dirname);
	free(win_dir);
}

char const *
fs_read_dir(fs_dir_t dir, char const ** fullname)
{
	fs_dir_win_t * win_dir = dir;
	char * filename = NULL;
	BOOL res;
	assert(dir);
	if (win_dir->handle != INVALID_HANDLE_VALUE) {
		size_t len = win_dir->dirname_end - win_dir->dirname;
		size_t available = strlen(win_dir->dirname_end) ;
		size_t needed = strlen(win_dir->obj_info.cFileName) + 1;
		if (available < needed) {
			win_dir->dirname = reallocate(win_dir->dirname,len + needed + 1);
			win_dir->dirname_end = win_dir->dirname + len;
		}
		*win_dir->dirname_end = PATH_DELIM;
		filename = win_dir->dirname_end + 1;
		strcpy(filename,win_dir->obj_info.cFileName);
		if (fullname) {
			*fullname = win_dir->dirname;
		}
	}
	if (filename) {
		res = FindNextFile(win_dir->handle,&win_dir->obj_info);
		if (!res) {
			FindClose(win_dir->handle);
			win_dir->handle = INVALID_HANDLE_VALUE;
			if (GetLastError() != ERROR_NO_MORE_FILES) {
				bail(GRIPE_CANT_READ_DIR,"Read error on directory \"%s\"",
					win_dir->dirname);
			}
		}
		if (filename[0] == '.' &&
			((filename[1] == '.' && filename[2] == '\0') || filename[1] == '\0')) {
			return  fs_read_dir(dir,fullname);
		}
	}
	else {
		*win_dir->dirname_end = '\0';
	}
	return filename;
}

char const *
fs_cur_dir_entry(fs_dir_t dir, char const **entry)
{
	fs_dir_win_t * win_dir = dir;
	assert(dir);
	if (entry) {
		*entry = win_dir->dirname_end;
	}
	return win_dir->dirname;
}

fs_dir_t
fs_get_parent(fs_dir_t dir)
{
	fs_dir_win_t * win_dir;
	assert(dir);
	win_dir = dir;
	return win_dir->parent;
}

#endif

/* EOF */

