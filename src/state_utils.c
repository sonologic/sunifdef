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

/*!	Use allocate() to allocate dynamic state storage */
#ifdef STATE_ALLOCATOR
#undef STATE_ALLOCATOR
/*!\ingroup state_utils_infrastructure
 *	Use allocate() instead of malloc()
 */
#define STATE_ALLOCATOR		(allocate)
#endif
#include "opts.h"
#include "memory.h"

/*! \ingroup state_utils_infrastructure
 *	\file state_utils.c
 *	The file implements the State Utilities module.
 */

/*!	\addtogroup state_utils_infrastructure */
/*@{*/

void
module_initor(
	bool is_static,
	bool is_zero_initable,
	void **state,
	void **public_state,
	void const *initialiser,
	size_t size,
	void (*user_init)())
{
	if (!is_static) {
		assert(*state == NULL);
		*state = allocate(size);
		*public_state = *state;
	}
	if (is_zero_initable) {
		memset(*state,0,size);
	}
	else if (initialiser) {
		memcpy(*state,initialiser,size);
	}
	else {
		assert(user_init);
		user_init(*state);
	}
}

void
module_finitor(
	bool is_static,
	void **state,
	void **public_state,
	void (*user_finis)())
{
	if (user_finis) {
		assert(*state);
		user_finis(*state);
	}
	if (!is_static) {
		assert(*state);
		release(state);
		*public_state = NULL;
	}
}
/*@}*/

/* EOF */
