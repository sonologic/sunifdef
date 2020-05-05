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
#include "ptr_vector.h"
#include "report.h"
#include "memory.h"
#include <string.h>

/*!	\addtogroup ptr_vector_internals */
/*@{*/
 
/*! \file ptr_vector.c
 *	This file implements the Pointer-Vector module
 */

/*! Structure of a pointer-vector object */ 
struct ptr_vector {
	/*! Size in bytes by which the vector will be extended on demand.
	 *	Each time an extension is necessary this quantity is doubled. */
	size_t tranch;
	/*! The number of pointers in the vector */ 
	size_t count;
	/*! The number of pointers the vector can currently contain without
		extension */
	size_t cap;
	/*! The start of the stored pointers */
	void **ptrs;
};

/*@}*/

/* API ***************************************************************/

/*!\addtogroup ptr_vector_interface */
/*@{*/

ptr_vector_h 
ptr_vector_new(void)
{
	ptr_vector_h  pv = allocate(sizeof(struct ptr_vector));
	pv->tranch = 16 * sizeof(void *);
	pv->ptrs = NULL;
	return pv;
}

void
ptr_vector_dispose(ptr_vector_h *ppv)
{
	if (*ppv) {
		ptr_vector_h pv = *ppv;
		if (pv->ptrs) {
			free(pv->ptrs);
		}
		free(pv);
		*ppv = 0;
	}
}

void
ptr_vector_insert(ptr_vector_h pv, size_t pos, void *ptr)
{
	if (pv->count == pv->cap) {
	    pv->cap += pv->tranch;
		pv->ptrs = reallocate(pv->ptrs,pv->cap * sizeof(void *));
		pv->tranch <<= 1;
	}
	if (pos >= pv->count) {
		pv->ptrs[pv->count++] = ptr;
	}
	else {
		memmove(pv->ptrs + pos + 1,
				pv->ptrs + pos,
				((pv->count - pos) * sizeof(void *)));
		pv->ptrs[pos] = ptr;
		++pv->count;
	}
}

void
ptr_vector_append(ptr_vector_h pv, void *ptr)
{
	ptr_vector_insert(pv,(size_t)-1,ptr);
}

size_t
ptr_vector_count(ptr_vector_h pv)
{
	return pv->count;
}

void **
ptr_vector_start(ptr_vector_h pv)
{
	return pv->ptrs;
}

void **
ptr_vector_end(ptr_vector_h pv)
{
	return pv->ptrs + pv->count;
}

void *
ptr_vector_at(ptr_vector_h pv, size_t pos)
{
	return (pos < pv->count) ? pv->ptrs[pos] : 0;
}

/*@}*/

/* EOF */
