#ifndef PTR_VECTOR_H
#define PTR_VECTOR_H

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
#include <stddef.h>

/*!	\addtogroup ptr_vector_interface */
/*@{*/

/*! \file ptr_vector.h
 *	This file provides the Pointer-Vector module interface.
 */

/*! Abstract type of pointer-vector object */
typedef struct ptr_vector * ptr_vector_h;

/*! Create new pointer-vector object
 *	\return A new \c ptr_vector object
 */
extern ptr_vector_h 
ptr_vector_new(void);

/*! Dispose of a pointer-vector object
 *	\param ppv Pointer to a pointer-vector to be destroyed. The
 *	object addressed by this pointer is set to \c NULL on return.
 *	\c free() is called on each pointer stored in the pointer,
 *	and then remaining memory controlled by the pointer-vector is
 *	released.
 */
extern void
ptr_vector_dispose(ptr_vector_h *ppv);

/*! Insert a pointer into a pointer-vector.
 *	\param pv	The pointer-vector in which a pointer is to be inserted.
 *	\param	pos	The position in the vector at which to insert.
 *	\param ptr	The pointer to be inserted.
 *	If \c pos is beyond the end of \c pv then \c ptr is appended to \c pv
 *	The vector is extended if necessary to insert or append \c ptr.
 *	\note Pointers that the caller is holding to pointers within the vector
 *	must be assumed invalidated after an insertion.
 *	\note Insertion is inefficient even when it does not provoke an
 *	extension of the vector, because it requires all elements from \c pos
 *	onward to be moved up 1 position.
 *	
 */
extern void
ptr_vector_insert(ptr_vector_h pv, size_t pos, void *ptr);

/*! Append a pointer to a pointer-vector.
 *	\param pv	The pointer-vector to which the pointer is to be appended.
 *	\param ptr	The pointer to be appended.
 *	The vector is extended if necessary to append \c ptr.
 *	\note Pointers that the caller is holding to pointers within the vector
 *	must be assumed invalidated after an insertion.
 */
extern void
ptr_vector_append(ptr_vector_h pv, void *ptr);
/* Append `ptr' to `pv'. `pv' is extended if need be */

/*! Get the number of elements in a pointer-vector.
 *	\param pv	The pointer-vector to count.
 *	\return 	The number of elements in \c pv.
 */
extern size_t
ptr_vector_count(ptr_vector_h pv);

/*! Get the start of a pointer-vector.
 *	\param pv	The pointer-vector whose start is wanted.
 *	\return 	Pointer to the first element of \c pv, if
 *				\c pv is non-empty; otherwise \c NULL
 */
extern void **
ptr_vector_start(ptr_vector_h pv);

/*! Get the end of a pointer-vector.
 *	\param pv	The pointer-vector whose end is wanted.
 *	\return 	Pointer just past the last element of \c pv, if
 *				\c pv is non-empty; otherwise \c NULL
 */
extern void **
ptr_vector_end(ptr_vector_h pv);

/*! Get the element at a given position in a pointer-vector.
 *	\param pv	The pointer-vector from which an element is wanted.
 *	\param pos	The position in the vector from which the element is wanted.
 *	\return 	The pointer at position \c pos in  \c pv, unless \c pos
 *				is out of range; \c NULL if \c pos is out of range.
 */
extern void *
ptr_vector_at(ptr_vector_h pv, size_t pos);

/*@}*/

#endif /* EOF */
