#ifndef PLATFORM_H
#define PLATFORM_H
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
#include <stdio.h>

/*!	\file platform.h
 *	This file defines macros for values that vary with the host environment.
 */
#ifdef _WIN32
/*! Character that delimits elements of a filesystem path for Windows
 */
#define PATH_DELIM '\\'
/*! String that represents the root of the filesystem for Windows
*/
#define FS_ROOT_PREFIX ""

#define _POSIX_
/*! Name of the host operating system type  */
#define OS_TYPE "Windows"

/*! This is a Windows build  */
#define WINDOWS


#ifdef _MSC_VER
/*! This is a an MS C build */
#define vsnprintf _vsnprintf
#endif 

#include <stdlib.h>
/*! Maximum length of pathname. */
#define PATH_MAX _MAX_PATH

#else /* ! _WIN32 */
/*! Character that delimits elements of a filesystem path for Unix
 */
#define PATH_DELIM '/'
/*! String that represents the root of the filesystem for Unix.
*/
#define FS_ROOT_PREFIX "/"

/*! Name of the host operating system type  */
#define OS_TYPE "Unix"

/*! This is a Unix build */
#define UNIX

#include <limits.h>
#endif

#endif
/* EOF */

