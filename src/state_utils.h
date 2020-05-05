#ifndef STATE_UTILS_H
#define STATE_UTILS_H
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
#include "bool.h"
#include "memory.h"
#include <stdlib.h>
#include <assert.h>
#include <string.h>
 
/*! \ingroup state_utils_interface, state_utils_infrastructure
 *	\file state_utils.h
 *	The file provides the State Utils interface
 */
 
/*! \ingroup state_utils_infrastructure
 *
 *	These items that are imported by state_utils.h to implement
 *	the user macros but are inessential to client code.
 */
/*@{*/

#ifndef STATE_ALLOCATOR
/*! Define malloc-like function for allocating dynamic state storage */
#define STATE_ALLOCATOR (malloc)
#endif

/*! Generated name for module global state instance */
#define STATE(module)	module##_state
/*! Generated name for module public state instance */
#define PUBLIC_STATE(module) module##_public_state
/*! Generated name for pointer to module global state instance  */
#define HANDLE(module) module##_h
/*! Generated name for pointer to module public state instance */
#define PUBLIC_HANDLE(module)	module##_public_h
/*! Mandatory name user defined module initialiser */
#define USER_INIT(module)	module##_init
/*! Mandatory name user defined module finaliser */  
#define USER_FINIS(module)	module##_finis
/*! Mandatory name for pointer to user defined module initialiser */ 
#define USER_INIT_HANDLE(module)	module##_init_h
/*! Mandatory name for pointer to user defined module finaliser */ 
#define USER_FINIS_HANDLE(module)	module##_finis_h
/*! Generated name for module initialising function */
#define INITOR(module)	module##_initor
/*!	Generated name for module finalising function */
#define FINITOR(module)	module##_finitor
/*! Generated name for module static initialiser */
#define STATIC_INITIALISER(module)	module##_static_initialiser
/*! Generated name for pointer to module static initialiser */
#define STATIC_INITIALISER_HANDLE(module)	module##_static_initialiser_h
/*! enum tag name for module storage type, static or dynamic */
#define STORAGE_TYPE_TAG(module)	module##_storage_type
/*! enum tag name for module's initialisation type */
#define INITIALISATION_TYPE_TAG(module)		module##_initialisation_type
/*! Declare user-defined module initialising function */
#define DECL_USER_INIT(module)\
	void USER_INIT(module)(STATE_T(module) *)
/*! Declare user-defined module finalising function */
#define DECL_USER_FINIS(module)\
	void USER_FINIS(module)(STATE_T(module) *)
/*! Declare pointer to user-defined module initialising function */
#define DECL_USER_INIT_HANDLE(module)\
	void (*USER_INIT_HANDLE(module))(STATE_T(module) *)
/*! Declare pointer to user-defined finalising function */
#define DECL_USER_FINIS_HANDLE(module)\
	 void (*USER_FINIS_HANDLE(module))(STATE_T(module) *)
/*! Declare generated module initialising function */
#define DECL_INITOR(module)	void INITOR(module)(void)
/*! Declare generated module finalising function */
#define DECL_FINITOR(module)	void FINITOR(module)(void)
/*! Declare a static initialiser for the module */
#define DECL_STATIC_INITIALISER(module)\
	static const STATE_T(module) STATIC_INITIALISER(module)
/*! Declare pointer to module const initialiser. Can be null if none */  
#define DECL_STATIC_INITIALISER_HANDLE(module)\
	static const STATE_T(module) * const STATIC_INITIALISER_HANDLE(module)
/*! Select static storage for module state instance */
#define SELECT_STATIC(module)\
	enum module##_storage_type { STORAGE_TYPE_TAG(module) = STATE_STATIC }
/*! Select dynamic storage for module state instance */
#define SELECT_DYNAMIC(module)\
	enum module##_storage_type { STORAGE_TYPE_TAG(module) = STATE_DYNAMIC }
/*! Select the module's initialisation type */
#define SELECT_INITIALISATION_TYPE(module,init_type)\
	enum module##_init_type { INITIALISATION_TYPE_TAG(module) = init_type }
/*! Select 0-fill initialisation for module state instance */
#define SELECT_ZERO_INITABLE(module)\
	SELECT_INITIALISATION_TYPE(module,ZERO_INITABLE)
/*! Select static initialisation for the module's state instance */
#define SELECT_STATIC_INITABLE(module)\
	SELECT_INITIALISATION_TYPE(module,STATIC_INITABLE)
/*! Select user-defined initialisation for the module's state instance */
#define SELECT_USER_INITABLE(module)\
	SELECT_INITIALISATION_TYPE(module,USER_INITABLE)
/*! Is module state in static storage ? */
#define IS_STATIC(module)	(STORAGE_TYPE_TAG(module) == STATE_STATIC)
/*! Is module zero-initialisable? */
#define IS_ZERO_INITABLE(module)\
	(INITIALISATION_TYPE_TAG(module) == ZERO_INITABLE)
/*! Has module a static initialiser? */
#define HAS_STATIC_INITIALISER(module)\
	(INITIALISATION_TYPE_TAG(module) == STATIC_INITABLE)
/*! Has module user-defined initialisation and finalisation? */
#define HAS_USER_INIT(module) (INITIALISATION_TYPE_TAG(module) == USER_INITABLE)
/*! Declare handles to module's global and public state
 *	when state is in dynamic storage
 */
#define DECL_DYNAMIC_STATE(module)\
	 STATE_T(module) * HANDLE(module) = NULL;\
	 PUBLIC_STATE_T(module) * PUBLIC_HANDLE(module) = NULL

/*! Declare module's state in static storage, with handles to global and
 *	public state.
 */
#define DECL_STATIC_STATE(module)\
	static STATE_T(module) STATE(module);\
	STATE_T(module) * HANDLE(module) = &STATE(module);\
	PUBLIC_STATE_T(module) * PUBLIC_HANDLE(module) =\
	 (PUBLIC_STATE_T(module) *)&STATE(module) 

/*! Extern declaration of generated module initialising function */
#define IMPORT_INITOR(module)	extern DECL_INITOR(module)
/*! Extern declaration of generated module finalsing function */
#define IMPORT_FINITOR(module)	extern DECL_FINITOR(module)

/*! Extern declaration of handle to module's public state */
#define IMPORT_STATE(module)\
extern PUBLIC_STATE_T(module) * PUBLIC_HANDLE(module)

/*! Get the value of a field from the module's global state, accessed through
 *	the module's handle.
 */
#define GET_STATE_BY_HANDLE(module,field)\
	(assert(true),HANDLE(module)->field)

/*! Reference a field in the module's global state, accessed through the the
 *	the module's handle.
 */ 	
#define SET_STATE_BY_HANDLE(module,field) (HANDLE(module)->field)

/*! Get the value of a field from the module's global static state.
 *	Quicker than \c GET_STATE_BY_HANDLE for a statically implemented module.
 */ 
#define GET_STATIC_STATE(module,field)		(assert(true),STATE(module).field)

/*! Reference a field in the module's global static state.
 *	Quicker than \c SET_STATE_BY_HANDLE for a statically implemented mdoule
 */			
#define SET_STATIC_STATE(module,field)		(STATE(module).field)

/*! Define the generated module initialisation function \c INITOR(module) */ 
#define DEFINE_INITOR(module)\
DECL_INITOR(module) {\
	module_initor(\
		IS_STATIC(module),\
		IS_ZERO_INITABLE(module),\
		(void **)(char *)&HANDLE(module),\
		(void **)(char *)&PUBLIC_HANDLE(module),\
		STATIC_INITIALISER_HANDLE(module),\
		sizeof(STATE_T(module)),\
		USER_INIT_HANDLE(module));\
}

/*! Define the generated module finalisation function \c FINITOR(module) */ 
#define DEFINE_FINITOR(module)\
DECL_FINITOR(module) {\
	module_finitor(\
		IS_STATIC(module),\
		(void **)(char *)&HANDLE(module),\
		(void **)(char *)&PUBLIC_HANDLE(module),\
		USER_INIT_HANDLE(module));\
}

/*! Initialise a module's global state
 *	\param is_static	Is the module's global state in static storage?
 *	\param is_zero_initable		Is the module's global state zero-initialisable?
 *	\param	state	Pointer to a pointer to the module's global state.
 *	\param	public_state	Pointer to a pointer to the module's public
 *							state.
 *	\param	initialiser		\c NULL, otherwise a pointer to a constant
 *							initialiser for the module's global state.
 *	\param	size			The size of the module's global state.
 *	\param	user_init		\c NULL, otherwise a pointer to a user-supplied
 *							function to initialise the module's global
 *							state.
 *	If \c is_static is false, the function asserts that <tt>*state == NULL</tt>
 *	and then assigns to \c *state a heap block of \c size bytes. 
 *
 *	Otherwise, \c state is assumed to address the module's global state in
 *	static storage.  
 *
 *	Then, if \c is_zero_initable is true, the module's state is 0-filled.
 *
 *	Otherwise, if <tt>initialiser != NULL</tt>, the \c size bytes at
 *	\c initialiser are copied to \c *state.
 *
 *	Otherwise, the function asserts that <tt>user_init != NULL</tt>, and
 *	then calls \c user_init(*state)
 */
extern void
module_initor(
	bool is_static,
	bool is_zero_initable,
	void **state,
	void **public_state,
	void const *initialiser,
	size_t size,
	void (*user_init)());

/*! Finalise a module's global state
 *	\param is_static	Is the module's global state in static storage?
 *	\param	state	Pointer to a pointer to the module's global state.
 *	\param	public_state	Pointer to a pointer to the module's public
 *							state.
 *	\param	user_finis		\c NULL, otherwise a pointer to a user-supplied
 *							function to finalise the module's global
 *							state.
 *	If <tt>user_init != NULL</tt>, the function asserts that
 *	<tt>*state != NULL</tt>	and then calls \c user_init(*state)
 *
 *	Otherwise, \c state is assumed to address the module's global state in
 *	static storage.  
 *
 *	Then, if \c is_static is false, the function again asserts that
 *	<tt>*state != NULL</tt>, then calls \c free(*state), and sets
 *	<tt>*public_state = *state = NULL</tt>
 *
 */
extern void
module_finitor(
	bool is_static,
	void **state,
	void **public_state,
	void (*user_finis)());
			
/*@}*/

/*! \addtogroup state_utils_user_macros

	<h3>The State Utils macros</h3>

	Macros are provided to define a module's global state structure, and
	optionally create a public state structure as part of the global
	state, for exporting to the module's clients.
	
	Macros are provided to create the module's state structure
	statically or dynamically (on the heap). Options are provided to specify
	that the structure is zero-initialisable, or has a constant static
	initialiser, or is to be initialised and finalised by
	user-defined functions.

	<b>Using the macros:</b>
	
	<ul>
		<li>
			<p><b>In my_module.h:</b></p>
			<ul>
				<li>
					<p>Define the module's public state:</p>
					<table border="0" bgcolor="WhiteSmoke">
						<tr><td>
<tt><pre>
PUBLIC_STATE_DEF(my_module) {
	int field1;
	int field2;
	...
} PUBLIC_STATE_T(my_module);</pre></tt>
						</td></tr>
					</table>
				</li>
				<br>
				<li>
					<p>Import the module's extern declarations:</p>
					<table border="0" bgcolor="WhiteSmoke">
						<tr><td>
							<tt>IMPORT(my_module);</tt>
						</td></tr>
					</table>
				</li>
				<li>
					<p>Or, where the module has no state:</p>
					<table border="0" bgcolor="WhiteSmoke">
						<tr><td>
							<tt><pre>
IMPORT_INITOR(my_module);
IMPORT_FINITOR(my_module);</pre></tt>
						</td></tr>
					</table>
				</li>
			</ul>
		</li>
		<br>
		<li>
			<p><b>In my_module.c:</b></p>
			<ul>
				<li>
					<p>Define the module's gobal state:</p>
					<table border="0" bgcolor="WhiteSmoke">
						<tr><td>
<tt><pre>
STATE_DEF(my_module) {
	INCLUDE_PUBLIC(my_module); // Must be first
	int field3;
	int field4;
	...
STATE_T(my_module);</pre></tt>
						</td></tr>
					</table>
				</li>
				<br>
				<li>
					<p>
						If the module has no public state then the
						\c STATE_DEF will instead be:
					</p>	
					<table border="0" bgcolor="WhiteSmoke">
						<tr><td>
<tt><pre>
STATE_DEF(my_module) {
	int field1;
	int field2;
	...
STATE_T(my_module);
NO_PUBLIC_STATE(my_module);</pre></tt>
						</td></tr>
					</table>
				</li>
				<br>
				<li>
					<p>
						If the module has no private state then instead of
						the \c STATE_DEF you need:
					</p>
					<table border="0" bgcolor="WhiteSmoke">
						<tr><td>
							<tt>NO_PRIVATE_STATE(my_module);</tt>
						</td></tr>
					</table>	
				</li>
				<br>
				<li>
					<p>
						Define my_module's state as static and
						zero-initialisable:
					</p>
					<table border="0" bgcolor="WhiteSmoke">
						<tr><td>
							<tt>
								IMPLEMENT_STATIC(my_module,ZERO_INITABLE);
							</tt>
						</td></tr>
					</table>
				</li>
				<br>
				<li>
					<p>Or static with constant initialiser:</p>
					<table border="0" bgcolor="WhiteSmoke">
						<tr><td>
<tt><pre>
IMPLEMENT_STATIC(my_module,STATIC_INITABLE);
USE_STATIC_INITIALISER(my_module) = { { 1, 2,...}, 3, 4... };</pre></tt>
						</td></tr>
					</table>
				</li>
				<br>
				<li>
					<p>Or static with initialising and finalising functions:</p>
					<table border="0" bgcolor="WhiteSmoke">
						<tr><td>
<tt><pre>
IMPLEMENT_STATIC(my_module,USER_INITABLE);

DEFINE_USER_INIT(my_module)(STATE_T(my_mdoule) * statep)
{
	...
}

DEFINE_USER_FINIS(my_module)(STATE_T(my_module) * statep)
{
	...
}</pre></tt>
						</td></tr>
					</table>
				</li>
				<br>
				<li>
					<p>	
						If you implement \c my_module with the
						\c USER_INITABLE option, you must define functions to
						intialise and finalise the state structure with
						\c DEFINE_USER_INIT and \c DEFINE_USER_FINIS.
					</p>
				</li>
				<br>
				<li>
					<p>
						Instead of \c IMPLEMENT_STATIC you may use
						\c IMPLEMENT_DYNAMIC to have the module's state structure
						allocated on the heap.
					</p>
				</li>
				<br>
				<li>
					<p>
						Alternatively you can just use \c IMPLEMENT.
						If you define \c DEFAULT_STATIC_STATE to the compiler,
						\c IMPLEMENT is short for \c IMPLEMENT_STATIC; otherwise
						it is short for \c IMPLEMENT_DYNAMIC. 
					</p>
				</li>
			</ul>
		<li>
			<p><b>Initialising and cleaning up module state:</b></p>
			<ul>
				<li>
					<p>
						Before you can safely access any fields in the module's
						state structure you must call:
					</p>
					<table border="0" bgcolor="WhiteSmoke">
						<tr><td>
							<tt>INITIALISE(my_module);</tt>
						</td></tr>
					</table>
				</li>
				<li>
					<p>
						If you every need to dispose of the state structure you
						must call:
					</p>
					<table border="0" bgcolor="WhiteSmoke">
						<tr><td>
							<tt>FINALISE(my_module);</tt>
						</td></tr>
					</table>
				</li>
				<li>
					<p>You can reinitialise the state structure by calling:</p>
					<table border="0" bgcolor="WhiteSmoke">
						<tr><td>
							<tt>REINITIALISE(my_module);</tt>
						</td></tr>
					</table>
				</li>
			</ul>
		</li>
		<br>
		<li>
			<p><b>Accessing module state:</b></p>
			<ul>	
				<li>
					<p>
						Wherever the module's public state is visible, a field 
						can be referenced like this:
					</p>
					<table border="0" bgcolor="WhiteSmoke">
						<tr><td>
							<tt>val = GET_PUBLIC(my_module,field);</tt>
						</td></tr>
					</table>
					<br>
					<p>and can be set like this:</p>
					<table border="0" bgcolor="WhiteSmoke">
						<tr><td>
							<tt>SET_PUBLIC(my_module,field) = val;</tt>
						</td></tr>
					</table>
				</li>
				<br>
				<li>
					<p>
						Wherever the module's global state is visible, a field 
						can be referenced like this:
					</p>
					<table border="0" bgcolor="WhiteSmoke">
						<tr><td>
							<tt>val = GET_STATE(my_module,field);</tt>
						</td></tr>
					</table>
					<br>
					<p>and set like this:</p>
					<table border="0" bgcolor="WhiteSmoke">
						<tr><td>
							<tt>SET_STATE(my_module,field) = val;</tt>
						</td></tr>
					</table>
				</li>
			</ul>
		</li>
	</ul>
	
	<b>Generated symbols (for debugging):</b>
	
	Symbols that can be generated by the
	macros for a module \c my_module are:

	- <tt><b>struct my_module_state_s</b></tt>
		-  <tt><b>my_module_state</b></tt>
		- The module's global state structure.
		- \b Note: This symbol will exist if the module is implemented
			statically.
		
	- <tt><b>typedef struct my_module_state_s</b></tt>
		- <tt><b>my_module_state_t</b></tt>

	- <tt><b>struct my_module_public_state_s</b></tt>
		- <tt><b>my_module_public_state</b></tt>
		- The module's public state structure.
		- \b Note: This symbol will exist if the module is implemented statically.
		
	- <tt><b>typedef struct my_module_public_state_s</b></tt>
		- <tt><b>my_module_public_state_t</b></tt>

	- <tt><b>struct my_module_state_s *</b></tt>
		- <tt><b>my_module_h</b></tt>
		- Pointer to the module's global state.
		- \b Note: This symbol will \em always address the module's global
			state, if it has any.

	- <tt><b>struct my_module_public_state_s *</b></tt>
		- <tt><b>my_module_public_h</b></tt>
		- Pointer to the module's public state.
		- \b Note: This symbol will \em always address the module's public
			state, if it has any.

	- <tt><b>void my_module_init(my_module_state_s *statep)</b></tt>
		- Mandatory name of user-supplied module initialising function, if any.
		You must define this function if you specify the module with the
		\c \b USER_INITABLE attribute. You can use the macro \c \b 
		DEFINE_USER_INIT(my_module)	to compose the mandatory signature
		for this function.

	- <tt><b>void my_module_finis(my_module_state_s *statep)</b></tt>
		- Mandatory name of user-supplied module finalising function, if any.
		You must define this function if you specify the module with the
		\c \b USER_INITABLE attribute. You can use the macro \c \b
		DEFINE_USER_FINIS(my_module) to compose the mandatory signature
		for this function.
		- \b Note: \c my_module_init() and \c my_module_finis() do not need to
			control storage for the module's state structure. They can
			the assume the argument \c statep addresses that structure and need only
			initialise and finalise the elements of the structure.

	- <tt><b>void (*my_module_init_h)(my_module_state_s *)</b></tt>
		- If you specify the module with the \c \b USER_INITABLE attribute,
		this function pointer will be set to the address of \c my_module_init()
		and	will otherwise be \c NULL.

	- <tt><b>void (*my_module_finis_h)(my_module_state_s *)</b></tt>
		- If you specify the module with \c \b USER_INITABLE, this function
		pointer will be set to the address of \c my_module_finis()
		and	will otherwise be \c NULL.

	- <tt><b>const struct my_module_state_s</b></tt>
		- <tt><b>my_module_static_initialiser</b></tt>
		- Mandatory name of the module's constant static initialiser,
		if any. You can use the macro \c USE_STATIC_INITIALISER(my_module)
		to compose the mandatory declaration.
		
	- <tt><b>const struct my_module_state_s *</b></tt>
		- <tt><b>const my_module_static_initialiser_h</b></tt>
		- If you specify the module  with \c \b HAS_STATIC_INITIALISER, this
		pointer will be set to the address of \c my_module_static_initialiser
		and	will otherwise be \c NULL.

*/

 
/*! \addtogroup state_utils_impl_macros */
/*@{*/

/*! The storage type of the module's state */
enum module_storage_type {
	STATE_STATIC,
		/*!< Module's state is in static storage */
	STATE_DYNAMIC
		/*!< Module's state is in dynamic storage */
};

/*! The initialisation type of the module's state */
enum module_initialisation_type {
	ZERO_INITABLE,
		/*!< Module is zero-initialisable */
	STATIC_INITABLE,
		/*!< Module has a constant static initialiser */
	USER_INITABLE
		/*!< Module has user-defined initialisation	and finalisation
			functions */
};


/*! Implement the module's state in static storage
 *	\param	module	Name of the module
 *	\param init_type	Type of module initialisation. One of:
 *		- \c ZERO_INITABLE
 *		- \c STATIC_INITABLE
 *		- \c USER_INITABLE	
 */	
#define IMPLEMENT_STATIC(module,init_type)\
	DECL_STATIC_STATE(module);\
	SELECT_STATIC(module);\
	SELECT_INITIALISATION_TYPE(module,init_type);\
	DECL_STATIC_INITIALISER_HANDLE(module);\
	DECL_USER_INIT_HANDLE(module);\
	DECL_USER_FINIS_HANDLE(module);\
	DEFINE_INITOR(module)\
	DEFINE_FINITOR(module)

/*! Implement the module's state in dynamic storage
 *	\param	module	Name of the module
 *	\param init_type	Type of module initialisation. One of:
 *		- \c ZERO_INITABLE
 *		- \c STATIC_INITABLE
 *		- \c USER_INITABLE	
 */	
#define IMPLEMENT_DYNAMIC(module,init_type)\
	DECL_DYNAMIC_STATE(module);\
	SELECT_DYNAMIC(module);\
	SELECT_INITIALISATION_TYPE(module,init_type);\
	DECL_STATIC_INITIALISER_HANDLE(module);\
	DECL_USER_INIT_HANDLE(module);\
	DECL_USER_FINIS_HANDLE(module);\
	DEFINE_INITOR(module)\
	DEFINE_FINITOR(module)
	/* Implement the module's state dynamically */

#ifdef DEFAULT_STATIC_STATE
/*! Abbreviate \c IMPLEMENT_STATIC as \c IMPLEMENT when \c DEFAULT_STATIC_STATE
 *	is defined.
 */
#define IMPLEMENT(module,init_type)	IMPLEMENT_STATIC(module,init_type)
#else
/*! Abbreviate \c IMPLEMENT_DYNAMIC as \c IMPLEMENT when \c DEFAULT_STATIC_STATE
 *	is not defined.
 */
#define	IMPLEMENT(module,init_type)	IMPLEMENT_DYNAMIC(module,init_type)
#endif

/*! Declare a static initialiser for the module.
 *	\param	 module	The name of the module.
 *	The macro invocation should be immediately followed by the
 *	specification of the static initialiser:
 *
 *	-	<tt>USE_STATIC_INITIALISER(module) = {...};</tt>
 *
 *	The static initialiser will be copied into the module's global
 *	state instance whenever \c INITIALISE(module) is invoked.
 */
#define USE_STATIC_INITIALISER(module)\
	DECL_STATIC_INITIALISER(module);\
	DECL_STATIC_INITIALISER_HANDLE(module) = &STATIC_INITIALISER(module);\
	DECL_STATIC_INITIALISER(module)

/*! Define a user initialisation function for module.
 *	\param	module	Name of the module.
 *	The function body should be written immediately after the macro invocation:
 *
 *	- <tt>DEFINE_USER_INIT(module){...};</tt>
 *
 *	The macro generates the function signature:
 *
 *	- <tt>void USER_INIT(module)(STATE_T(module) *)</tt>
 *
 *	A user initialisation function must be defined if the module is implemented
 * 	with initialisation type \c USER_INITABLE.	The user initialisation
 *	function \c USER_INIT(module) should not be confused with the
 *	generated module initialisation function \c INITOR(module).
 *	\c USER_INIT(module) does not need to provide storage for the module's
 *	state. It can assume that storage is addressed by the
 *	<tt>STATE_T(module) *</tt> argument and need only initialise the
 *	the elements of the state structure.
 */  
#define DEFINE_USER_INIT(module)\
	DECL_USER_INIT(module);\
	DECL_USER_INIT_HANDLE(module) = &USER_INIT(module);\
	void USER_INIT(module) /* ...user_init function body here */

/*! Define a user finalisation function for module.
 *	\param	module	Name of the module.
 *	The function body should be written immediately after the macro invocation:
 *
 *	- <tt>DEFINE_USER_FINIS(module){...};</tt>
 *
 *	The macro generates the function signature:
 *
 *	- <tt>void USER_FINIS(module)(STATE_T(module) *)</tt>
 *
 *	A user finalisation function must be defined if the module is implemented
 * 	with initialisation type \c USER_INITABLE.	The user finalisation
 *	function \c USER_FINIS(module) should not be confused with the
 *	generated module finalisation function \c FINITOR(module).
 *	\c USER_FINIS(module) does not need to destroy storage for the module's
 *	state, even if the module is implemented dynamically. That dynamic
 *	storage will be disposed of by \c FINITOR(module). \c USER_FINIS(module)
 *	need only finalise the elements of the state structure.
 */  
#define DEFINE_USER_FINIS(module)\
	DECL_USER_FINIS(module);\
	DECL_USER_FINIS_HANDLE(module) = &USER_FINIS(module);\
	void USER_FINIS(module) /* ... user_finis function body here */

/*! Invoke this macro immediately after opening \c STATE_DEF(module) to place
 *	the public state instance at the start of the state instance. It must be
 *	invoked at that position if the module has public as well as private
 *	state.
 *	\param	module	Name of the module
 */
#define INCLUDE_PUBLIC(module)	PUBLIC_STATE_T(module)	PUBLIC_STATE(module)

/*! Invoke this macro in a module's implementation file to open the definition
 *	of the module's global state structure:
 *
 *	- <tt>STATE_DEF(module) {...}</tt>
 *
 *	\param	module	Name of the module
 */
#define STATE_DEF(module)	typedef struct module##_state_s	

/*! Invoke this macro in a module's implementation file to close the definition
 *	of the module's global state:
 *
 *	- <tt>STATE_DEF(module){...} STATE_T(module);</tt>
 *
 *	\param	module	Name of the module
 */
#define STATE_T(module)		module##_state_t

/*! Invoke this macro in a module's implementation file in place of
 *	\c STATE_DEF(module) to say that the module's global state is simply its
 *	public state (if any ).
 *	\param	module	Name of the module
 */ 
#define NO_PRIVATE_STATE(module)\
	typedef PUBLIC_STATE_T(module) STATE_T(module)

#ifdef DEFAULT_STATIC_STATE
	/*! Abbreviate \c GET_STATIC_STATE as \c GET_STATE when
	 *	\c DEFAULT_STATIC_STATE is defined.
	 *
	 *	Invoke this macro in the module's implementation file to
	 *	evaluate a field in the module's global state. If the
	 *	field also belongs to the module's public state, then
	 *	prefer \c GET_PUBLIC.    
	 */
	#define GET_STATE(module,field)		GET_STATIC_STATE(module,field)
	/*! Abbreviate \c SET_STATIC_STATE as \c SET_STATE when
	 *	\c DEFAULT_STATE_STATIC is defined.
	 *
	 *	Invoke this macro in the module's implementation file to
	 *	code an assignable reference to a field in the module's global state.
	 *
	 *	- <tt>SET_STATE(module,field) = value;</tt>
	 *
 	 *	\param	module	Name of the module
	 *	\param	field	Name of the field to be evaluated in the module's state.
	 *
	 * 	If the field also belongs to the module's public state, then
	 *	prefer \c SET_PUBLIC.    
	 */
	#define SET_STATE(module,field)		SET_STATIC_STATE(module,field)
#else
	/*! Abbreviate \c GET_STATE_BY_HANDLE_ as \c GET_STATE when
	 *	\c DEFAULT_STATIC_STATE is not defined.
	 *
	 *	Invoke this macro in the module's implementation file to
	 *	evaluate a field in the module's global state.
	 *
 	 *	\param	module	Name of the module
	 *	\param	field	Name of the field to be evaluated in the module's state.
	 *
	 *	If the field also belongs to the module's public state, then
	 *	prefer \c GET_PUBLIC.    
	 */
	#define GET_STATE(module,field)		GET_STATE_BY_HANDLE(module,field)
	/*! Abbreviate \c SET_STATE_BY_HANDLE as \c SET_STATE when
	 *	\c DEFAULT_STATIC_STATE is not defined.
	 *
	 *	Invoke this macro in the module's implementation file to
	 *	code an assignable reference to a field in the module's global state.
	 *
	 *	- <tt>SET_STATE(module,field) = value;</tt>
	 *	 
 	 *	\param	module	Name of the module
	 *	\param	field	Name of the field to be evaluated in the module's state.
	 *
	 * 	If the field also belongs to the module's public state, then
	 *	prefer \c SET_PUBLIC.    
	 */
	#define SET_STATE(module,field)		SET_STATE_BY_HANDLE(module,field)
#endif 


/*@}*/

/*! \addtogroup state_utils_hdr_macros */
/*@{*/

/*! Invoke this macro in a module's header file to open the definition
 *	of the module's public state structure:
 *
 *	- <tt>PUBLIC_STATE_DEF(module) {...}</tt>
 *
 *	\param	module	Name of the module
 */
#define PUBLIC_STATE_DEF(module)	typedef struct module##_public_state_s

/*! Invoke this macro in a module's header file to close the definition
 *	of the module's public state:
 *
 *	- <tt>PUBLIC_STATE_DEF(module){...} PUBLIC_STATE_T(module);</tt>
 *
 *	\param	module	Name of the module
 */
#define PUBLIC_STATE_T(module)	module##_public_state_t

/*! Invoke this macro in a module's header file in place of
 *	\c PUBLIC_STATE_DEF(module) to say that the module's global state is simply
 *	its private state (if any ).
 *
 *	\param	module	Name of the module
 */ 
#define NO_PUBLIC_STATE(module)\
typedef STATE_T(module) PUBLIC_STATE_T(module)

/*! Invoke this macro in a module's header file to expose the module's public
 *	state and its generated initialisation and finalisation functions.
 *
 *	\param	module	Name of the module
 */ 
#define IMPORT(module)\
	IMPORT_STATE(module);\
	IMPORT_INITOR(module);\
	IMPORT_FINITOR(module)

/*@}*/

/*! \ingroup state_utils_user_macros
 *	\name Macros for module client code.
 */
/*@{*/

/*! Initialise the module's state - appropriately whether static or dynamic,
 *	zero-initialisable, static-initialisable, user-initialisable.
 *	\param	module	Name of the module
 */	 
#define INITIALISE(module)	INITOR(module)()

/*! Finalise the module's state - appropriately whether static or dynamic,
 * 	user-finalised or not.
 *	\param	module	Name of the module
 */
#define FINALISE(module)	INITOR(module)()

/*! Finalise and then initialise the module's state appropriately
 *	\param module	Name of the module
 */	
#define REINITIALISE(module)\
	(void)(FINALISE(module),INITIALISE(module))

/*!	Invoke this macro in client code file to evaluate a field in the
 *	module's public state.
 *	\param	module	Name of the module
 *	\param	field	Name of the field to be evaluated in the module's state.
 */
#define	GET_PUBLIC(module,field)\
	(assert(true),(PUBLIC_HANDLE(module)->field))

/*!	Invoke this macro in client code file to code an assignable
 *	reference to a field in the module's public state:
 *
 *	- <tt>SET_PUBLIC(module,field) = value;</tt>
 *
 *	\param	module	Name of the module
 *	\param	field	Name of the field to be evaluated in the module's state.
 */
#define	SET_PUBLIC(module,field)	(PUBLIC_HANDLE(module)->field)


#endif
	
/*@}*/

/* EOF */
