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

/*! \file fs_nix.c
 * \ingroup filesystem_module filesystem_unix filesystem_unix_internals
 *
 * This file implements the filesystem module for Unix.
 */
#include "platform.h"

#ifdef UNIX

#include <sys/stat.h>
#include <sys/types.h>
#include <dirent.h>
#include "filesys.h"
#include "report.h"


/*! \addtogroup filesystem_unix_internals */
/*@{*/

/*! Structure implementing \e fs_dir_t for Unix */
typedef struct fs_dir_nix {
	/*! Handle of parent directory if any, else NULL. */
	struct fs_dir_nix * parent;
	/*! Name of directory */
	heap_str dirname;
	/*! The end of the directory name */
	char *dirname_end;
	/*! The directory handle */
	DIR * dir;
	/*! Latest retrieved directory entry */
	struct dirent entry;
} fs_dir_nix_t;

/*@}*/


heap_str
fs_real_path(char const *relname, size_t *namelen)
{
	char buf[PATH_MAX];
	if (realpath(relname,buf)) {
		char *full_path;
		size_t len = strlen(buf);
		if (namelen) {
			*namelen = len;
		}
		full_path = allocate(len + 1);
		strcpy(full_path,buf);
		return full_path;
	}
	return NULL;
}

fs_obj_type_t
fs_obj_type(char const *name)
{
	fs_obj_type_t type = FS_OBJ_NONE;
	struct stat obj_info;
	int res = lstat(name,&obj_info);
	if (!res) {
		if (S_ISLNK(obj_info.st_mode)) {
			type |= FS_OBJ_SLINK;
		}
		res = stat(name,&obj_info);
		if (!res) {
			if (S_ISREG(obj_info.st_mode)) {
				type |= FS_OBJ_FILE;
			}
			else if (S_ISDIR(obj_info.st_mode)) {
				type |= FS_OBJ_DIR;
			}
		}
	}
	return type;
}

fs_dir_t
fs_open_dir(char const *dirname, fs_dir_t const parent)
{
	fs_dir_nix_t * dir = NULL;
	DIR * dir_handle = opendir(dirname);
	if (dir_handle ) {
		size_t namelen;
		heap_str fullname = fs_real_path(dirname,&namelen);
		if (fullname) {
			dir = allocate(sizeof(fs_dir_nix_t));
			dir->parent = (fs_dir_nix_t *)parent;
			dir->dir = dir_handle;
			dir->dirname = fullname;
			dir->dirname_end = dir->dirname + namelen;
		}
	}
	return dir;
}


void
fs_close_dir(fs_dir_t dir)
{
	fs_dir_nix_t * nix_dir = dir;
	assert(dir);
	if (nix_dir->dir) {
		closedir(nix_dir->dir);
		free(nix_dir->dirname);
		free(nix_dir);
	}
}

char const *
fs_read_dir(fs_dir_t dir, char const **fullname)
{
	char *filename = NULL;
	fs_dir_nix_t * nix_dir = dir;
	struct dirent * entry;
	int res = readdir_r(nix_dir->dir,&nix_dir->entry,&entry);
	if (res) {
		bail(GRIPE_CANT_READ_DIR,"Read error on directory \"%s\"",
			nix_dir->dirname);
	}
	if (entry) {
		filename = nix_dir->entry.d_name;
		if (filename[0] == '.' &&
			((filename[1] == '.' && filename[2] == '\0') || filename[1] == '\0')) {
			return fs_read_dir(dir,fullname);
		}
		if (fullname) {
			size_t len = nix_dir->dirname_end - nix_dir->dirname;
			size_t available = strlen(nix_dir->dirname_end) ;
			size_t needed = strlen(filename) + 1;
			if (available < needed) {
				nix_dir->dirname = reallocate(nix_dir->dirname,len + needed + 1);
				nix_dir->dirname_end = nix_dir->dirname + len;
			}
			*nix_dir->dirname_end = PATH_DELIM;
			strcpy(nix_dir->dirname_end + 1,filename);
			filename = nix_dir->dirname_end + 1;
			*fullname = nix_dir->dirname;
		}
	}
	else {
		*nix_dir->dirname_end = '\0';
	}
	return filename;
}

char const *
fs_cur_dir_entry(fs_dir_t dir, char const **entry)
{
	fs_dir_nix_t * nix_dir = dir;
	assert(dir);
	if (entry) {
		*entry = nix_dir->dirname_end;
	}
	return nix_dir->dirname;
}

fs_dir_t
fs_get_parent(fs_dir_t dir)
{
	fs_dir_nix_t * nix_dir;
	assert(dir);
 	nix_dir = dir;
	return nix_dir->parent;
}

#endif

/* EOF */

