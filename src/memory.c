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
#include "memory.h"
#include "ptr_vector.h"

/*!	\ingroup memory_interface, memory_internals
 *	\file memory.c
 *	This file implements the Memory module.
 */

/* Helpers ***********************************************************/

/*! \addtogroup memory_internals */
/*@{*/

/*! Terminate the program with an out of memory diagnositic
 *	if a putative heap pointer is null.
 *
 *	\param	ptr		Pointer to test.
 *	\return	\c ptr, provided it is not null.
 */
static void *
no_alloc_fail(void *ptr)
{
	if (!ptr) {
		bail(GRIPE_OUT_OF_MEMORY,"Out of memory");
	}
	return ptr;
}
/*@}*/

/* API ***************************************************************/

void *
allocate(size_t bytes)
{
	return no_alloc_fail(calloc(bytes,1));
}

void *
reallocate(void *ptr, size_t bytes)
{
	return no_alloc_fail(realloc(ptr,bytes));
}
	
void *
callocate(size_t items, size_t size)
{
	return no_alloc_fail(calloc(items,size));
}

void release(void **pp)
{
	if (*pp) {
		free(*pp);
		*pp = NULL;
	}
}

/* EOF */
