#ifndef EVALUATOR_H
#define EVALUATOR_H

/***************************************************************************
 *   Copyright (C) 2004, 2006 Symbian Software Ltd.                        *
 *   All rights reserved.
 *   Copyright (C) 2002, 2003 Tony Finch <dot@dotat.at>.                   *
 *   All rights reserved.                                                  *
 *   Copyright (C) 1985, 1993 The Regents of the University of California. *
 *   All rights reserved.                                                  *
 *   Copyright (C) 2007, 2008 Mike Kinghan, imk@strudl.org                 *
 *   All rights reserved.                                                  *
 *                                                                         *
 *   Contributed by Mike Kinghan, imk@strudl.org, derived from the code    *
 *   of Tony Finch                                                         *
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

 /*!\ingroup evaluator_module evaluator_interface
 *  \file evaluator.h
 *	This file provides the Evaluator module interface
 */

/*! \addtogroup evaluator_interface */
/*@{*/

/*! Enumeration of types of input lines */
typedef enum {
	/*! An \c #if that we can't resolve */
	LT_IF,
	/*! A true \c #if */
	LT_TRUE,
	/*! A false \c #if */
	LT_FALSE,
	/*! An \c #elif that we can't resolve */
	LT_ELIF,
	/*! A true \c #elif */
	LT_ELTRUE,
	/*! A false \c #elif */
	LT_ELFALSE,
	/*! An \c #else */
	LT_ELSE,
	/*! An \c #endif */
	LT_ENDIF,
	/*! An non-directive line */
	LT_PLAIN,
	/*! End of file */
	LT_EOF,
	/*! Not a line type. This is the number of line types for which we
		handlers exist in the transition table. Subsequent line types
		are handled outside the transition table.
	*/
	LT_TABLE_HANDLERS,
	/*! A \c #define that is independant of the \c --define and \c --undefine
	 *	options and is kept
	 */
	LT_CONSISTENT_DEFINE_KEEP,
	/*! A \c #define that echoes a \c --define option and is dropped */
	LT_CONSISTENT_DEFINE_DROP,
	/*! A \c #define that contradicts an \c --undefine option */
	LT_CONTRADICTORY_DEFINE,
	/*! A \c #define that differently redefines a \c --define option */
	LT_DIFFERING_DEFINE,
	/*! An \c #undef that is independant of the \c --define and \c --undefine
	 *	options and is kept
	 */
	LT_CONSISTENT_UNDEF_KEEP,
	/*! An \c #undefine that echoes a \c --undefine option and is dropped */
	LT_CONSISTENT_UNDEF_DROP,
	/*! An \c #undef that contradicts a \c --define option */
	LT_CONTRADICTORY_UNDEF,
} line_type_t;

/*! Bit flags denoting findings of evaluation */
enum eval_flags {
	EVAL_INSOLUBLE = 1,
		/*!< Evaluated text cannot be resolved */
	EVAL_CONST = 2,
		/*!< Expression is a constant */
	EVAL_KEEP =  4,
		/*!< Expression is not to be eliminated */
	EVAL_KEEP_CONST = EVAL_CONST | EVAL_KEEP,
		/*!< Expression is constant not be eliminated */
	EVAL_DEL_PAREN = 8,
		/*!< Redundant parentheses to deleted from expression */
	EVAL_TRUE = 16,
		/*!< Expression evaluates as true */
	EVAL_FALSE = 32,
		/*!< Expression evaluates as false */
	EVAL_RESOLVED = EVAL_TRUE | EVAL_FALSE,
		/*!< Expression is be resolved */
	EVAL_VISITED = 64
		/*!< The text has been visited by the parser.
			This flag lets us head off infinite looping on
			circular macro definitions */
};

/*! Structure representing the evaluation of
	a text item */
typedef struct eval_result {
	char * sym_name;
		/*!< Name of symbol, if text is a symbol or a
			unary function of one */
	char * sym_def;
		/*!< Definition of symbol, if text assigns a
			definition to a symbol */
	int value;
		/*!< The value of text if it is a soluble
			expression  */
	int flags;
		/*!< Bit set of \em eval_flags */
} eval_result_t;


/*! Parse the current input line and return its line type. */
extern line_type_t
eval_line(void);


/*@}*/

/*! \addtogroup evaluator_interface_state_utils */
/*@{*/
IMPORT_INITOR(evaluator);
IMPORT_FINITOR(evaluator);
/*@}*/


#endif /* EOF */
