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

#include "evaluator.h"
#include "io.h"
#include "report.h"
#include "line_edit.h"
#include "chew.h"
#include "args.h"
#include "categorical.h"
#include "if_control.h"
#include "symbol_table.h"
#include "report.h"
#include "line_despatch.h"
#include <stddef.h>
#include <ctype.h>
#include <math.h>
#include <limits.h>
#include <assert.h>

/*!\ingroup evaluator_module evaluator_internals evaluator_interface
 *
 * \file evaluator.c
 *	This file implements the Evaluator module
 */

/*! \addtogroup evaluator_internals */
/*@{*/

/*! Set flags in an \em eval_result_t
 */
#define SET_FLAGS(eval_result,bits)	((eval_result).flags |= (bits))

/*! Set flags in an \em eval_result_t if a condition is true
 */
#define SET_FLAGS_IF(cond,eval_result,bits)\
	(void)((cond) ? SET_FLAGS(eval_result,bits) : 0)

/*! Clear flags in an \em eval_result_t
 */
#define CLEAR_FLAGS(eval_result,bits) ((eval_result).flags &= ~(bits))

/*! Clear flags in an \em eval_result_t if a condition is true
 */
#define CLEAR_FLAGS_IF(cond,eval_result,bits)\
	(void)((cond) ? CLEAR_FLAGS(eval_result,bits) : 0)

/*! Invert the values of specified flags in an
	\em eval_result_t
 */
#define FLIP_FLAGS(eval_result,bits) ((eval_result).flags ^= (bits))

/*!	 Get the flags in \em eval_result_t that match a mask */
#define FLAGS_IN(eval_result,bits)	((eval_result).flags & (bits))

/*! Test whether specified flags are set in an \em eval_result_t
*/
#define AFFIRM_FLAGS(eval_result,bits)\
	(((eval_result).flags & (bits)) == (bits))

/*! Test whether specified flags are clear in an
	\em eval_result_t
*/
#define DENY_FLAGS(eval_result,bits)\
	(((eval_result).flags & (bits)) == 0)

/*! Test whether an \em eval_result_t
	represents a true expression
*/
#define IS_TRUE(eval_result)	AFFIRM_FLAGS(eval_result,EVAL_TRUE)

/*! Test whether an \em eval_result_t represents a false
	expression.
*/
#define IS_FALSE(eval_result)	AFFIRM_FLAGS(eval_result,EVAL_FALSE)

/*! Test whether an \em eval_result_t represents a constant.
*/
#define IS_CONST(eval_result)	AFFIRM_FLAGS(eval_result,EVAL_CONST)

/*! Test whether an \em eval_result_t represents
	unresolved text.
*/
#define UNRESOLVED(eval_result)	DENY_FLAGS(eval_result,EVAL_FALSE | EVAL_TRUE)

/*! Test whether an \em eval_result_t represents a
	resolved expression.
*/
#define RESOLVED(eval_result)	!UNRESOLVED(eval_result)

/*! Test whether an \em eval_result_t represents
	insoluble text.
*/
#define INSOLUBLE(eval_result)	AFFIRM_FLAGS(eval_result,EVAL_INSOLUBLE)

/*! Test whether an \em eval_result_t represents
	a symbol already parsed.

	If symbol is found to be neither resolved nor insoluble
	and has already been visited then it has a circular
	definition.
*/
#define VISITED(eval_result)	AFFIRM_FLAGS(eval_result,EVAL_VISITED)

/*! Test whether an \em eval_result_t represents
	a symbol defined as an empty string.
*/
#define EMPTY_SYMBOL(eval_result) \
	((eval_result).sym_name && \
	(eval_result).sym_def && \
	!(eval_result).sym_def[0])

/*! Test whether an \em eval_result_t stipulates
	deleting superfluous parentheses
*/
#define	DEL_PAREN(eval_result)	AFFIRM_FLAGS(eval_result,EVAL_DEL_PAREN)

/*! Flag an \em eval_result_t for deletion of superflous
	parentheses.
*/
#define SET_DEL_PAREN(eval_result)	SET_FLAGS(eval_result,EVAL_DEL_PAREN)

/*! Test whether an \em eval_result_t evaluates as
	an expression we can't eliminate.
*/
#define KEEP(eval_result)	AFFIRM_FLAGS(eval_result,EVAL_KEEP)

/*! Flag an \em eval_result_t as an expression we can't
	eliminate.
*/
#define SET_KEEP(eval_result)	SET_FLAGS(eval_result,EVAL_KEEP)

/*! Test whether an \em eval_result_t evaluates as
	a non-eliminable constant
*/
#define KEEP_CONST(eval_result)	AFFIRM_FLAGS(eval_result,EVAL_KEEP_CONST)

/*! Flag an \em eval_resultT as a non-eliminable constant */
#define SET_KEEP_CONST(eval_result)	SET_FLAGS(eval_result,EVAL_KEEP_CONST)

/*! Flag an \em eval_resultT as a constant */
#define SET_CONST(eval_result)	SET_FLAGS(eval_result,EVAL_CONST)


/* Helpers *****************************************************************/

/*! Assign a value in an \em eval_result_t */
static void
set_value(eval_result_t *result, int val)
{
	result->value = val;
	SET_FLAGS(*result,val ? EVAL_TRUE : EVAL_FALSE);
}


/*! Integer less-than */
static int lt(int l, int r) { return l < r; }
/*! Integer greater-than */
static int gt(int l, int r) { return l > r; }
/*! Integer less-than-or_equal */
static int le(int l, int r) { return l <= r; }
/*! Integer greater-than-or_equal */
static int ge(int l, int r) { return l >= r; }
/*! Integer equality */
static int eq(int l, int r) { return l == r; }
/*! Integer inequality */
static int ne(int l, int r) { return l != r; }
/*! Bitwise and */
static int bit_and(int l, int r) { return l & r; }
/*! Bitwise inclusive or */
static int bit_or(int l, int r) { return l | r; }
/*! Bitwise exclusive or */
static int bit_xor(int l, int r) { return l ^ r; }
/*! Left shift */
static int lshift(int l, int r) { return l << r; }
/*! Right shift */
static int rshift(int l, int r) { return l >> r; }
/*! Plus */
static int plus(int l, int r) { return l + r; }
/*! Minus */
static int minus(int l, int r) { return l - r; }
/*! Multiply */
static int mult(int l, int r) { return l * r; }
/*! Division */
static int divide(int l, int r) {	return l / r; }
/*! Modulus */
static int mod(int l, int r) {	return l % r; }


typedef int (integer_bin_op_t)(int,int);

static eval_result_t
integer_binary_op(	eval_result_t *lhs,
						eval_result_t *rhs,
						integer_bin_op_t op)
{
	eval_result_t result = {0,0,0};
	if ((IS_CONST(*lhs) || RESOLVED(*lhs)) &&
			(IS_CONST(*rhs) || RESOLVED(*rhs))) {
		set_value(&result,op(lhs->value,rhs->value));
	}
	return result;
}

/*! Less-than operator */
static eval_result_t
op_lt(eval_result_t * lhs, eval_result_t * rhs)
{
	return integer_binary_op(lhs,rhs,lt);
}

/*! Greater-than operator */
static eval_result_t
op_gt(eval_result_t *lhs, eval_result_t *rhs)
{
	return integer_binary_op(lhs,rhs,gt);
}

/*! Less-than-or-equal operator */
static eval_result_t
op_le(eval_result_t *lhs, eval_result_t *rhs)
{
	return integer_binary_op(lhs,rhs,le);
}

/*! Greater-than-or-equal operator */
static eval_result_t
op_ge(eval_result_t *lhs, eval_result_t *rhs)
{
	return integer_binary_op(lhs,rhs,ge);
}

/*! Equality operator */
static eval_result_t
op_eq(eval_result_t *lhs, eval_result_t *rhs)
{
	return integer_binary_op(lhs,rhs,eq);
}

/*! Inequality operator */
static eval_result_t
op_ne(eval_result_t *lhs, eval_result_t *rhs)
{
	return integer_binary_op(lhs,rhs,ne);
}

/*! Inclusive-or operator */
static eval_result_t
op_or(eval_result_t *lhs, eval_result_t *rhs)
{
	eval_result_t result = {0,0,0};
	if (IS_TRUE(*lhs) || IS_TRUE(*rhs)) {
		set_value(&result,true);
	}
	else if (IS_FALSE(*lhs) && IS_FALSE(*rhs)) {
		set_value(&result,false);
	}
	if (FLAGS_IN(*lhs,EVAL_RESOLVED) == FLAGS_IN(*rhs,EVAL_RESOLVED)) {
		SET_FLAGS_IF(KEEP_CONST(*lhs) | KEEP_CONST(*rhs),
			result,EVAL_KEEP_CONST);
	}
 	else if (IS_TRUE(*lhs)) {
		SET_FLAGS_IF(KEEP_CONST(*lhs),result,EVAL_KEEP_CONST);
		CLEAR_FLAGS_IF(!KEEP_CONST(*rhs),*rhs,EVAL_KEEP);
	}
 	else if (IS_TRUE(*rhs)) {
		SET_FLAGS_IF(KEEP_CONST(*rhs),result,EVAL_KEEP_CONST);
		CLEAR_FLAGS_IF(!KEEP_CONST(*lhs),*lhs,EVAL_KEEP);
	}
 	else if (IS_FALSE(*lhs)) {
		SET_FLAGS_IF(KEEP_CONST(*rhs),result,EVAL_KEEP_CONST);
	}
	else if (IS_FALSE(*rhs)) {
		SET_FLAGS_IF(KEEP_CONST(*lhs),result,EVAL_KEEP_CONST);
	}
	return result;
}

/*! And operator */
static eval_result_t
op_and(eval_result_t *lhs, eval_result_t *rhs)
{
	eval_result_t result = {0,0,0};
	if (IS_TRUE(*rhs) && IS_FALSE(*lhs)) {
		set_value(&result,false);
		SET_FLAGS_IF(KEEP_CONST(*lhs),result,EVAL_KEEP_CONST);
	}
	else if (IS_TRUE(*lhs) && IS_FALSE(*rhs)) {
		set_value(&result,false);
		SET_FLAGS_IF(KEEP_CONST(*rhs),result,EVAL_KEEP_CONST);
	}
	else if (UNRESOLVED(*lhs)) {
		if (IS_FALSE(*rhs)) {
			set_value(&result,false);
			SET_FLAGS_IF(KEEP_CONST(*rhs),result,EVAL_KEEP_CONST);
		}
		else {
			SET_FLAGS_IF(KEEP_CONST(*lhs),result,EVAL_KEEP_CONST);
		}
	}
	else if (UNRESOLVED(*rhs)) {
		if (IS_FALSE(*lhs)) {
			set_value(&result,false);
			SET_FLAGS_IF(KEEP_CONST(*lhs),result,EVAL_KEEP_CONST);
		}
		else {
			SET_FLAGS_IF(KEEP_CONST(*rhs),result,EVAL_KEEP_CONST);
		}
	}
	else {
		set_value(&result,lhs->value && rhs->value);
		SET_FLAGS_IF(KEEP_CONST(*rhs) | KEEP_CONST(*lhs),
				result,EVAL_KEEP_CONST);
	}
	return result;
}

/*! Bitwise AND operator */
static eval_result_t
op_bit_and(eval_result_t *lhs, eval_result_t *rhs)
{
	return integer_binary_op(lhs,rhs,bit_and);
}

/*! Bitwise OR operator */
static eval_result_t
op_bit_or(eval_result_t *lhs, eval_result_t *rhs)
{
	return integer_binary_op(lhs,rhs,bit_or);
}

/*! Bitwise XOR operator */
static eval_result_t
op_bit_xor(eval_result_t *lhs, eval_result_t *rhs)
{
	return integer_binary_op(lhs,rhs,bit_xor);
}

/*! Left-shift operator */
static eval_result_t
op_lshift(eval_result_t *lhs, eval_result_t *rhs)
{
	return integer_binary_op(lhs,rhs,lshift);
}

/*! Right-shift operator */
static eval_result_t
op_rshift(eval_result_t *lhs, eval_result_t *rhs)
{
	return integer_binary_op(lhs,rhs,rshift);
}

/*! Addition operator */
static eval_result_t
op_plus(eval_result_t *lhs, eval_result_t *rhs)
{
	return integer_binary_op(lhs,rhs,plus);
}

/*! Substraction operator */
static eval_result_t
op_minus(eval_result_t *lhs, eval_result_t *rhs)
{
	return integer_binary_op(lhs,rhs,minus);
}

/*! Multiplication operator */
static eval_result_t
op_mult(eval_result_t *lhs, eval_result_t *rhs)
{
	return integer_binary_op(lhs,rhs,mult);
}

/*! Division operator */
static eval_result_t
op_divide(eval_result_t *lhs, eval_result_t *rhs)
{
	eval_result_t result = {0,0,0};
	if ((IS_CONST(*lhs) || RESOLVED(*lhs)) &&
			(IS_CONST(*rhs) || RESOLVED(*rhs))) {
 		if (!rhs->value) {
			report(GRIPE_ZERO_DIVIDE,NULL,
				"Divide by zero");
		}
		else {
			set_value(&result,divide(lhs->value,rhs->value));
		}
	}
	return result;
}

/*! Modulus operator */
static eval_result_t
op_mod(eval_result_t *lhs, eval_result_t *rhs)
{
	eval_result_t result = {0,0,0};
	if ((IS_CONST(*lhs) || RESOLVED(*lhs)) &&
			(IS_CONST(*rhs) || RESOLVED(*rhs))) {
 		if (!rhs->value) {
			report(GRIPE_ZERO_DIVIDE,NULL,
				"Divide by zero");
		}
		else {
			set_value(&result,mod(lhs->value,rhs->value));
		}
	}
	return result;
}


struct ops;

/*! Type of evaluation functions.

	\param		ops 	A pointer to an element of the precedence
						table which lists the	operators at the
						current level of precedence.

	\param		On entry, a pointer to a \c char* that points to the
				expression to be evaluated. On return, receives
				the address terminating the evaluated expression.

	\return An \em eval_result_t representing the result of
				evaluation.
 */
typedef eval_result_t (evaluator_t)(const struct ops *,char **);

/*! The evaluator for binary expressions */
static evaluator_t eval_table;
/*! The evaluator for unary expressions */
static evaluator_t eval_unary;

/*! Structure representing an operator
	(Would rather call it "operator" but GCC thinks
	that is a reserved word even in C)*/

struct operation {
	/*! Points to the preprocessor token that denotes the operator */
	const char *str;
	/*! Pointer to function to implement the operator */
	eval_result_t (*fn)(eval_result_t *, eval_result_t *);
};

/*! Structure of the operator precedence table.
 *
 *	Expressions involving binary operators are evaluated
 *	in a table-driven way by \c eval_table(). When it evaluates a subexpression
 *	it calls the inner function with its first argument pointing to the next
 *	element of the table.
 *
 *	Innermost expressions have special non-table-driven handling.
 */
struct ops {
	/*! Pointer to function for evaluating operands of operator */
	evaluator_t *inner;
	/*! Array of operators of same precedence associated with
		the evaluator \c inner */
	struct operation op[5];
};


/*! The operator precedence table */
static const struct ops eval_ops[] = {

	{
		eval_table,
		{
			{ "||", op_or }
		}
	},
	{
		eval_table,
		{
			{ "&&", op_and }
		}
	},
	{
		eval_table,
		{
			{ "|", op_bit_or },
			{ "^", op_bit_xor },
			{ "&", op_bit_and }
		}
	},
	{
		eval_table,
		{
			{ "==", op_eq },
			{ "!=", op_ne }
		}
	},
	{
		eval_table,
		{
			{ "<=", op_le },
			{ ">=", op_ge },
			{ "<", op_lt },
			{ ">", op_gt }
		}
	},
	{
		eval_table,
		{
			{ "<<", op_lshift },
			{ ">>", op_rshift },
		}
	},
	{
		eval_table,
		{
			{ "+", op_plus },
			{ "-", op_minus },
		}
	},
	{
		eval_unary,
		{
			{ "*", op_mult },
			{ "/", op_divide },
			{ "%", op_mod },
		}
	},
};

/*! Test whether a given operator occurs at a given line-buffer position.
 *	\param txt	A line buffer position
 *	\param op		Pointer to an operator token
 *	\return		\em true iff the operator token \c op occurs at position \c txt
 */
static bool
op_cmp(char const * txt, char const *op)
{
	assert(op[1] == '\0' || op[2] =='\0');
	if (txt[0] != op[0]) {
		return false;
	}
	if (op[1]) {
		if (txt[1] != op[1]) {
			return false;
		}
	}
	else if (txt[1] == op[0]) {
		return false;
	}
	return true;
}

/*! Evaluator for innermost subexpressions, i.e.
 - \em !expr
 - \em (expr)
 - \em (symbol)
 - \em symbol
 - \em number

 \param	ops		Pointer to the \em struct ops to be used for
					the evaluation.
 \param cpp		On entry, a pointer to the start of the text
					to be evaluated. Receives the address reached
					by evaluation.
 \return	An \em eval_result_t representing
			the result of evaluation.
 */
static eval_result_t
eval_unary(const struct ops *ops, char **cpp);

/*!	Skip right-hand operand of \c && in an \c #if
 *  directive when we can short-circuit the evaluation.
 *
 *	\param cp	Line-buffer position at which the operand
 *				begins.
 *
 *	When an \c && can be short-circuited, the evaluator
 *	can skip from the start of the righthand operand until:
 *	- the end of the directive, or
 *	- an unbalanced ')', which must close the scope of the \c &&, or
 *	- a \c || operator, which has higher precedence.
 *
 *	\return The address just past the skipped operand.
 */
static char *
short_circuit_and(char *cp)
{
	int balance = 0;
	for (	;*(cp = chew_on(cp)); ++cp) {
		if (*cp == '(') {
			++balance;
		}
		else if (*cp == ')') {
			--balance;
		}
		if (balance < 0) {
			break;
		}
		else if (balance == 0) {
			/* Check for another binary truth functor on same
			 * level to respect precedence.
			 */
			if (cp[0] == cp[1] && cp[0] == '|') {
				break;
			}
		}
	}
	return cp;
}

/*!	Skip the right-hand operand of \c || in an \c #if
 *  directive when we can short-circuit the evaluation.
 *
 *	\param cp	Line-buffer position at which the operand
 *				begins.
 *
 *	When an \c || can be short-circuited, the evaluator can
 *	skip from the start of the righthand operand until the end
 *	of the directive, or an unbalanced ')', which must close
 *	the scope of the \c ||. None of our operators has higher
 *	precedence.
 *
 *	\return The address just past the skipped operand.
 */
static char *
short_circuit_or(char *cp)
{
	int balance = 0;
	for (	;*(cp = chew_on(cp)); ++cp) {
		if (*cp == '(') {
			++balance;
		}
		else if (*cp == ')') {
			--balance;
		}
		if (balance < 0) {
			break;
		}
	}
	return cp;
}

/*! Evaluate the expression of a \c #if or \c #elif directive.
 *	If we can resolve the expression we return \c LT_TRUE or
 *	\c LT_FALSE accordingly, otherwise \c LT_IF.
 */
static int
eval_if(char **cpp);

/*!	Evaluate the text of a \c #define directive to determine if it is
	consistent with the \c --define and \c --undefine assumptions.

	\param cpp  Pointer to the address of the directive text. This
				pointer is updated at exit with the address just past the
				text consumed.

	\return		A line-type, as follows:

	- Case 1: Directive is \c #define \c sym

		-	If neither \c --define \c sym nor \c --undefine \c sym,
			return \c LT_CONSISTENT_DEFINE_KEEP
		-	If \c --define \c sym, return \c LT_CONSISTENT_DEFINE_DROP
		-	If <tt>--define sym=val</tt>, return \c LT_DIFFERING_DEFINE
		-	If \c --undefine \c sym, return \c LT_CONTRADICTORY_DEFINE

	- Case 2: Directive is <tt>#define sym str1 [str2...]</tt>

		-	If neither \c --define \c sym nor \c --undefine \c sym,
			return \c LT_CONSISTENT_DEFINE_KEEP
		-	If \c --define \c sym, return \c LT_DIFFERING_DEFINE
		-	If <tt>--define sym=str1</tt>, return \c LT_CONSISTENT_DEFINE_KEEP
		-	If <tt>--define sym=val</tt>, where <tt>val != str1</tt>,
				return \c LT_DIFFERING_DEFINE
		-	If \c --undefine \c sym return \c LT_CONTRADICTORY_DEFINE

	-	Case 3: Directive is <tt>#define sym(arg1[,arg2...]) str1 [str2...]</tt>

		-	If neither \c --define \c sym nor \c --undefine \c sym,
			return \c LT_CONSISTENT_DEFINE_KEEP
		-	If \c --undefine \c sym, return \c LT_CONTRADICTORY_DEFINE
		-	If <tt>--define sym=val</tt> return \c LT_DIFFERING_DEFINE
		-	If \c --define \c sym, return \c LT_DIFFERING_DEFINE
*/
static int
eval_define(char **cpp)
{
	char const *def;
	char * cp = *cpp;
	int retval;
	int cursym;
	bool functionoid = false;
	debug(DBG_13,line_len(*cpp),*cpp);
	cp = chew_on(cp);
	cursym = find_sym(cp,&cp);
	do {
		if (cursym >= 0) {
			def = SYMBOL(cursym)->sym_def;
		}
		else {
			retval = LT_CONSISTENT_DEFINE_KEEP;	/* symbol not -Ded or -Ued */
			break;
		}
		if (def == NULL) {
			retval = LT_CONTRADICTORY_DEFINE;	/* symbol is -Ued */
			break;
		}
		/* Symbol is -Ded */
		if (*cp == '(')	{ /* #define sym(x...) is inconsistent */
			retval = LT_DIFFERING_DEFINE;
			functionoid = true; /* #defining a function-type macro */
			break;
		}
		/* Not functionoid macro, so enter PSEUDO_COMMENT state now */
		SET_PUBLIC(chew,comment_state) = PSEUDO_COMMENT;
		SET_PUBLIC(chew,last_comment_start_line) = GET_PUBLIC(io,line_num);
		cp = chew_on(cp);
		if (*cp != '\0') {
			/* #define sym str1 [str2..] */
			char *str = chew_str(cp);
			size_t def_len = strlen(def);
			size_t val_len = str - cp;
			if (strncmp(def,cp,def_len)) {
				/* str1 differs from -D value within length of value */
				retval = LT_DIFFERING_DEFINE;
				break;
			}
			if (val_len > def_len) {
				/* str1 is longer than -D value */
				retval = LT_DIFFERING_DEFINE;
				break;
			}
			/* str1 is identical with -D value */
			cp = chew_on(str);
			str = chew_sym(cp);	/* But is there a str2? */
			retval = (cp == str) ?
				LT_CONSISTENT_DEFINE_DROP :	/* No str2. */
				LT_DIFFERING_DEFINE;  /* There is a str2 */
			break;
		}
		/* #define sym */
		if (def[0] == '\0') {
			if (cursym == GET_PUBLIC(categorical,last_contradictory_undef)) {
				forget_contradiction();
			}
			retval = LT_CONSISTENT_DEFINE_DROP;
		}
		else {
			retval = LT_DIFFERING_DEFINE;
		}
	} while(false);
	if (functionoid) {
		/* If this is a functionoid macro then normal comment parsing
			continues till we reach the closing ')' of the parameters */
		for (	;*(cp = chew_on(cp)) != '\0' && *cp != ')'; ++cp){};
	}
	if (*cp != '\0') { /* There's more */
		/* Might be unbalanced '(' but we don't care */
		SET_PUBLIC(chew,comment_state) = PSEUDO_COMMENT;
		/* Pseudo-comment parsing */
		for (	;*(cp = chew_on(cp)) != '\0'; ++cp){} /* Skip to newline */
	}
	debug(DBG_14,line_len(*cpp),*cpp,retval);
	*cpp = cp;
	return retval;
}

/*!	Evaluate the text of an \c #undef to determine if it is consistent
	with the \c --define and \c --undefine options.

	\param cpp  Pointer to the address of the directive text. This
				pointer is updated at exit with the address just past the
				text consumed.

	\return		A line-type, as follows:

	- 	If neither <tt>--define sym</tt> nor <tt>--undefine sym</tt>,
		return \c LT_CONSISTENT_UNDEF_KEEP
	-	If \c --undefine \c sym, return \c LT_CONSISTENT_UNDEF_DROP
	-	Else return \c LT_CONTRADICTORY_UNDEF
*/
static int
eval_undef(char **cpp)
{
	char const *def;
	char *cp = *cpp;
	int retval;
	int cursym;
	debug(DBG_15,line_len(*cpp),*cpp);
	cp = chew_on(cp);
	cursym = find_sym(cp,&cp);
	do {
		if (cursym >= 0) {	/* Symbol is -Ded or -Ued */
			def = SYMBOL(cursym)->sym_def;
		}
		else {
			retval = LT_CONSISTENT_UNDEF_KEEP;	/* symbol not -Ded or -Ued */
			break;
		}
		if (def == NULL) {
			retval = LT_CONSISTENT_UNDEF_DROP;	/* symbol is -Ued */
		}
		else {	/* Symbol is -Ded */
			/* This #undef FOO contradicts -DFOO. We will complain
			about this unless #define FOO is the next thing we see after
			comments and whitespace, so we stash the complaint for the
			the time being - unless we've already got a pending
			contradiction for the same symbol */
			if (GET_PUBLIC(categorical,last_contradictory_undef) != cursym) {
				flush_contradiction();
				SET_PUBLIC(categorical,last_contradictory_undef) = cursym;
				save_contradiction("\"%.*s\" contradicts -D symbol");
			}
			retval = LT_CONTRADICTORY_UNDEF;
		}
	} while(false);
	debug(DBG_16,line_len(*cpp),*cpp,retval);
	*cpp = cp = chew_on(cp);
	return retval;
}

/*! Table-driven evaluation of binary operators.
  The evaluator does shortcircuit evaluation for && and ||.
  It also simplifies && and || subexpressions that
  can be partially solved.
  If we are keeping constants then constants are treated like
  unresolved symbols, and the rules are:
	<constant1> && <constant2> := <constant1>
	<constant> && TRUE : = <constant>
	<constant> && FALSE := FALSE
	<constant1> || <constant2> := <constant1>
	<constant> || TRUE := TRUE
	<constant> || FALSE := <constant>

 \param	ops		Pointer to the \em struct ops to be used for
					the evaluation.
 \param result	Pointer to an \em eval_result_t that receives
					the result of evaluation.
 \param cpp		On entry, a pointer to the start of the text
					to be evaluated. Receives the address reached
					by evaluation.

 */
static eval_result_t
eval_table(const struct ops *ops,char **cpp);

/*@}*/


/*! \ingroup evaluator_internals_state_utils */
/*@{*/
/*! The global state of the Evaluator module */
STATE_DEF(evaluator) {
	/*! Line-buffer position of the start of the current \c #if condition */
	char			*ifpos;
	/*! The line type of the current line */
	line_type_t		line_type;
	/*! Are we parsing the definiens VAL of an option -DSYM=VAL,
		rather than input source? */
	bool parsing_sym_def;
} STATE_T(evaluator);
/*@}*/

/*! \addtogroup evaluator_internals_state_utils */
/*@{*/
NO_PUBLIC_STATE(evaluator);

IMPLEMENT(evaluator,ZERO_INITABLE);
/*@}*/

/*!
	Report a symbol the occurs in an \c #if in accordance
	with the \c --symbols option.

	\param		policy	The operative policy for listing symbols.
	\param		name	The start of the symbol.
	\param		namelen	The length of the symbol.
	\param		symind	 The value returned by find_sym() for
				\em name.

*/
static void
list_symbol(	symbols_policy_t policy,
				char const *name,
				size_t namelen,
				int symind)
{
	eval_result_t * symbol;
	bool visited;
	if (symind < 0) {
		add_unknown_symbol(~symind,name,namelen);
		visited = false;
		symbol = SYMBOL(~symind);
		SET_FLAGS(*symbol,EVAL_VISITED);
	}
	else {
		symbol = SYMBOL(symind);
		visited = VISITED(*symbol);
	}
	if (visited && ((policy & SYMBOLS_LIST_FIRST) == SYMBOLS_LIST_FIRST)) {
		return;
	}
	printf("%.*s",(int)namelen,name);
	if ((policy & SYMBOLS_LOCATE) == SYMBOLS_LOCATE) {
		printf(": %s(%d)",GET_PUBLIC(io,filename),GET_PUBLIC(io,line_num));
	}
	putchar('\n');
}

/*! Evaluate a string as a numeral.
	\param	num	The putative numeral, 0-terminated.
	\param	numend	The address terminating any valid numeral is stored
		here on return.
	\return The double float value of any decimal, octal, or hex numeral
		commencing at \e num, or 0 is there is no such numeral, or HUGE_VAL
		if the integer value of the numeral exceeds INT_MAX.

	If the number \e N of characters \e *numend - \e num is > 0 at return then
	then those \e N characters comprise the evaluated numeral. If \e *numend
	!= 0 then the 0-terminated string at \e num is not a valid numeral,
	and if \e N == 0 then no numeral was scanned.

	The string "0x" or "0X" is parsed as a valid numeral "0" followed
	by 'x' or 'X'.

	The return value != HUGE_VAL for validity. If valid, may be cast to
	\c int without loss.

	This function is more serviceable that \e strtol() for evaluating
	preprocessor integer constants since a seperate invocation of the
	latter would be required for each possible base 8, 10, 16.

*/
static double
eval_numeral(char const *num, char const **numend)
{
	int sign = 1;
	int base = 10;
	int dval = 0;
	int val = 0;
	bool overflow = false;
	size_t num_len = 0;
	char const *start = num;

	switch(*num) {
	case '-':
		sign = -1;
		start = ++num;
		break;
	case '+':
		start = ++num;
		break;
	default:;
	}
	if (*num == '0') {
		++num;
		if (*num == 'x' || *num == 'X') {
			++num;
			base = 16;
		}
		else {
			base = 8;
		}
	}
	for (	;; ++num) {
		switch(*num) {
		case '0':
			dval = 0;
			break;
		case '1':
			dval = 1;
			break;
		case '2':
			dval = 2;
			break;
		case '3':
			dval = 3;
			break;
		case '4':
			dval = 4;
			break;
		case '5':
			dval = 5;
			break;
		case '6':
			dval = 6;
			break;
		case '7':
			dval = 7;
			break;
		case '8':
			dval = 8;
			break;
		case '9':
			dval = 9;
			break;
		case 'a':
		case 'A':
			dval = 10;
			break;
		case 'b':
		case 'B':
			dval = 11;
			break;
		case 'c':
		case 'C':
			dval = 12;
			break;
		case 'd':
		case 'D':
			dval = 13;
			break;
		case 'e':
		case 'E':
			dval = 14;
			break;
		case 'f':
		case 'F':
			dval = 15;
			break;
		default:
			dval = 16;
		}
		if (dval < base) {
			int tmp = val;
			val *= base;
			if ((val / base != tmp) || (val<tmp)) {
				overflow = true;
			}
			else {
				tmp = val;
				val += dval;
				if (val - dval != tmp) {
					overflow = true;
				}
			}
		}
		else {
			break;
		}
	}
	num_len = num - start;
	if (num_len == 1 && (*num == 'x' || *num == 'X')) {
		--num;
	}
	else {
		if (*num == 'u' || *num == 'U') {
			++num;
		}
		if (*num == 'l' || *num == 'L') {
			++num;
		}
	}
	*numend = num;
	if (overflow) {
		report(GRIPE_INT_OVERFLOW,NULL,
			"Integer constant \"%.*s\" is too big for sunifdef "
			"(max %d): expression will not be resolved",
			num - start,start,INT_MAX);
		return HUGE_VAL;
	}
	return sign * val;
}

/*!		Evaluate a symbol.

	\param symbol	Pointer to an eval_result_t that
						holds the symbol to evaluate.

	If the symbol has no definition, e.g. \c -DFOO,
	then it is evaluated as insoluble.

	Otherwise an attempt is made to evaluate
	the definition of the symbol as a decimal numeral.
	If this attempt is successful then the symbol
	assumes the integer value of numeral.

	Otherwise the definition is recursively evaluated
	as an expression and the symbol assumes the
	result of this evaluation.

	The function initially flags the symbol as having
	been visited. If parser visits the a symbol it has
	already visited and finds that the symbol is not
	flagged as either resolved or insoluble it can
	conclude that the evaluation of circular definition
	has recursed back to its starting point. An infinite
	loop can this be pre-empted and the symbol can
	be evaluated as insoluble.

*/
static void
eval_symbol(eval_result_t *symbol)
{
	char const *symdef = symbol->sym_def;
	SET_FLAGS(*symbol,EVAL_VISITED);
	if (!symdef) {
		SET_FLAGS(*symbol,EVAL_FALSE);
	}
	else {
		char const *numend;
		double val = eval_numeral(symdef,&numend);
		if (val != HUGE_VAL) {
			if (*numend != '\0' || numend == symdef) {
				/* Value of symbol is not an integer.
					Is it an expression? */
				eval_result_t result;
				size_t len = strlen(symdef);
				if (len) {
					/* Possible expression */
					char * symdup = allocate(len + 1);
					char * heapblk = symdup;
					strcpy(symdup,symdef);
					SET_STATE(evaluator,parsing_sym_def) = true;
					result = eval_table(eval_ops,&symdup);
					SET_STATE(evaluator,parsing_sym_def) = false;
					free(heapblk);
					if (UNRESOLVED(result)) {
						SET_FLAGS(*symbol,EVAL_INSOLUBLE);
					}
					else {
						set_value(symbol,result.value);
					}
				}
			}
			else {
				set_value(symbol,(int)val);
			}
		}
	}
}

static eval_result_t
eval_unary(const struct ops *ops, char **cpp)
{
	char const *ep;
	int symind;
	char *sym_name;
	eval_result_t * symbol;
	eval_result_t result = {0,0,0};
	char *cp = chew_on(*cpp);
	do {

		if (*cp == '!') {
			debug(DBG_1,ops - eval_ops);
			++cp;
			result = eval_unary(ops,&cp);
			if (UNRESOLVED(result)) {
				break;
			}
			result.value = !result.value;
			FLIP_FLAGS(result,EVAL_RESOLVED);
		}
		else if (*cp == '~') {
			eval_result_t neg_result;
			debug(DBG_22,ops - eval_ops);
			++cp;
			neg_result = eval_unary(ops,&cp);
			if (UNRESOLVED(neg_result)) {
				break;
			}
			set_value(&result,~neg_result.value);
		}
		else if (*cp == '(') {
			char *start = cp;
			++cp;
			debug(DBG_2,ops - eval_ops);
			result = eval_table(eval_ops,&cp);
			cp = chew_on(cp);
			if (*cp != ')') {
				/* Missing ')' */
				bail(GRIPE_UNBALANCED_PAREN,
					"Missing \")\" in \"%.*s\"",
					line_len(GET_PUBLIC(io,line_start)),
					GET_PUBLIC(io,line_start));
			}
			else if (DEL_PAREN(result)) {
				delete_paren(start,cp);
			}
			++cp;
		}
		else if (*cp == '+') {
			debug(DBG_20,ops - eval_ops);
			++cp;
			result = eval_unary(ops,&cp);
			break;
		}
		else if (*cp == '-') {
			debug(DBG_21,ops - eval_ops);
			++cp;
			result = eval_unary(ops,&cp);
			if (UNRESOLVED(result)) {
				break;
			}
			result.value = -result.value;
		}
		else if (isdigit((unsigned char)*cp)) {
			double val;
			debug(DBG_3,ops - eval_ops);
			val = eval_numeral(cp,&ep);
			if (val != HUGE_VAL) {
				result.value = (int)val;
				if (!GET_STATE(evaluator,parsing_sym_def)) {
					SET_CONST(result);
					if (!GET_PUBLIC(args,del_consts)) {
						SET_KEEP(result);
					}
					if (GET_PUBLIC(args,eval_consts)) {
						SET_FLAGS(result,result.value ? EVAL_TRUE : EVAL_FALSE);
					}
				}
				else {
					SET_FLAGS(result,result.value ? EVAL_TRUE : EVAL_FALSE);
				}
			}
			cp = (char *)ep;
		}
		else if (strncmp(cp, "defined", 7) == 0 && !symchar(cp[7])) {
			symbols_policy_t symbols_policy;
			bool paren;
			ptrdiff_t sym_len;
			cp = chew_on(cp + 7);
			debug(DBG_4, ops - eval_ops);
			paren = *cp == '(';
			if (paren) {
				++cp;
			}
			sym_name = chew_on(cp);
			symind = find_sym(sym_name,&cp);
			sym_len = cp - sym_name;
			cp = chew_on(cp);
			if (paren) {
				if (*cp == ')') {
					++cp;
				}
				else {
					/* Missing ')' */
					bail(GRIPE_UNBALANCED_PAREN,NULL,
						"Missing \")\" in \"%.*s\"",
						line_len(GET_PUBLIC(io,line_start)),
						GET_PUBLIC(io,line_start));
				}
			}
 			symbols_policy =  GET_PUBLIC(args,symbols_policy);
			if (symbols_policy) {
				/* --symbols in force */
				list_symbol(symbols_policy,sym_name,sym_len,symind);
				break;
			}
			if (symind < 0) {
				/* Unknown symbol */
				break;
			}
			symbol = SYMBOL(symind);
			set_value(&result,symbol->sym_def != NULL);
		}
		else if (symchar(*cp)) {
			symbols_policy_t symbols_policy;
			sym_name = cp;
			debug(DBG_5,ops - eval_ops);
			symind = find_sym(cp,&cp);
 			symbols_policy =  GET_PUBLIC(args,symbols_policy);
			if (symbols_policy) {
				/* --symbols in force */
				list_symbol(symbols_policy,sym_name,cp - sym_name,symind);
				break;
			}
			if (symind < 0) {
				/* Unknown symbol */
				break;
			}
			symbol = SYMBOL(symind);
			if (INSOLUBLE(*symbol)) {
				break;
			}
			if (UNRESOLVED(*symbol)) {
				if (!VISITED(*symbol)) {
					eval_symbol(symbol);
				}
				else {
					SET_FLAGS(*symbol,EVAL_INSOLUBLE);
				}
			}
			if (EMPTY_SYMBOL(result)) {
				report(GRIPE_EMPTY_SYMBOL,NULL,
					"Empty symbol \"%s\" in expression",result.sym_name);
			}
			result = *symbol;

		} else {
			debug(DBG_6,ops - eval_ops);
			break;
		}
	} while(false);
	*cpp = cp;
	if (RESOLVED(result)) {
		debug(DBG_7,ops - eval_ops,result.value);
	}
	else {
		SET_KEEP(result);
	}
	return result;
}

static int
eval_if(char **cpp)
{
	eval_result_t result;
	debug(DBG_11, *cpp);
	result = eval_table(eval_ops,cpp);
	debug(DBG_12,result.value);

	if (KEEP_CONST(result)) {
		return LT_IF;
	}
	if (IS_TRUE(result)) {
		return LT_TRUE;
	}
	if (IS_FALSE(result)) {
		return LT_FALSE;;
	}
	return LT_IF;
}

static eval_result_t
eval_table(const struct ops *ops, char **cpp)
{
	char *start_cut = (debug(DBG_8, ops - eval_ops),*cpp);
	/* Evaluate the lhs... */
	eval_result_t lhs_result = ops->inner(ops+1,cpp);
	/* Assume lhs is all we've got... */
	eval_result_t result = lhs_result;
	/* Assume we will delete parentheses... */
	char * cp = *cpp;
	SET_DEL_PAREN(result);
	for (; *(cp = chew_on(cp)) && *cp != ')'; ) {
		eval_result_t rhs_result = {0,0,0};
		char *start_lhs_cut = start_cut;
		char *end_lhs_cut = cp;
		const struct operation *op;

		/* Now look for binary op at current precedence... */
		for (op = ops->op; op->str != NULL && !op_cmp(cp,op->str); op++){}
		if (op->str == NULL) { /* No binary op, no rhs */
			break;
		}
		cp += strlen(op->str);
			/* Got bin op - we will not delete parentheses... */
		CLEAR_FLAGS(result,EVAL_DEL_PAREN);
		if (RESOLVED(lhs_result) && !KEEP_CONST(lhs_result)) {
			/* ...Possibly can short-circuit */
			if (IS_TRUE(lhs_result) && op->fn == op_or) {
				/*	Can shortcircuit on TRUE || <RHS>... */
				cp = short_circuit_or(cp);
				/* result == lhs_result. OK */
				break;
			}
			else if (IS_FALSE(lhs_result) && op->fn == op_and) {
				/*	Can shortcircuit on FALSE && <RHS>... */
				cp = short_circuit_and(cp);
				/* result == lhs_result. OK */
				break;
			}
			else if ((IS_TRUE(lhs_result) && op->fn == op_and) ||
						(IS_FALSE(lhs_result) && op->fn == op_or)) {
				/* Can delete TRUE &&, or FALSE ||... */
				cut_text(start_lhs_cut,cp);
				/* Cutting lhs can make superflous parentheses */
				SET_DEL_PAREN(result);
			}
		}
		start_cut = end_lhs_cut;
		end_lhs_cut = cp;
		debug(DBG_9, ops - eval_ops, op->str);
		/* Evaluate rhs... */
		rhs_result = ops->inner(ops,&cp);
		result = op->fn(&lhs_result,&rhs_result);
		if (op->fn == op_or || op->fn == op_and) {
			if (!KEEP(lhs_result) && KEEP(rhs_result)) {
				cut_text(start_lhs_cut,end_lhs_cut);
				SET_FLAGS(result,EVAL_DEL_PAREN | EVAL_KEEP);
			}
			else if (!KEEP(rhs_result) && KEEP(lhs_result)) {
				cut_text(start_cut,cp);
				SET_FLAGS(result,EVAL_DEL_PAREN | EVAL_KEEP);
			}
		}
	}
	if (RESOLVED(result)) {
		debug(DBG_10, ops - eval_ops, result.value);
	}
	else {
		SET_KEEP(result);
	}
	*cpp = cp;
	return result;
}

/* API ***************************************************************/

/*
 * Parse a line and determine its type. We keep the preprocessor line
 * parser state between calls in the global variable linestate, with
 * help from chew_on().
 */
line_type_t
eval_line(void)
{
	char *cp;
	int cursym;
	size_t kwlen;
	int retval;
	comment_state_t wascomment;

	if (!get_line()) {
		flush_contradiction();
		return LT_EOF;
	}

	SET_PUBLIC(line_edit,simplification_state) = UNSIMPLIFIED;
		/* Assume no simplification possible */
	retval = LT_PLAIN;
	wascomment = GET_PUBLIC(chew,comment_state);
	cp = chew_on(GET_PUBLIC(io,line_start));
	if (GET_PUBLIC(chew,line_state) == LS_NEUTER) {
		if (*cp == '#') {
			SET_PUBLIC(chew,line_state) = LS_DIRECTIVE;
			cp = chew_on(cp + 1);
		}
		else if (*cp != '\0') {
			SET_PUBLIC(chew,line_state) = LS_CODE;
			flush_contradiction();
		}
	}
	if (GET_PUBLIC(chew,comment_state) == NO_COMMENT &&
			 GET_PUBLIC(chew,line_state) == LS_DIRECTIVE) {
		symbols_policy_t symbols_policy = GET_PUBLIC(args,symbols_policy);
		size_t kwoff = read_offset(cp);
		char *kwpos;
		cp = chew_sym(cp);
		kwpos = read_pos(kwoff);
		kwlen = cp - kwpos;
		SET_PUBLIC(line_edit,keyword) = kwpos;
		if (kwpos[0] == 'i' && kwpos[1] == 'f') {
			/* Possible #if, #ifdef or #ifndef */
			bool ifdef = false;
			if (kwpos[2] == 'n') {
				ifdef = false;
				kwpos += 3;
				kwlen -= 3;
			}
			else if (kwpos[2] == 'd') {
				ifdef = true;
				kwpos += 2;
				kwlen -= 2;
			}
			if (strncmp(kwpos,"def",kwlen) == 0 ) {
				/* Got #ifdef or #ifndef */
				char *name;
				cp = chew_on(cp);
				name = cp;
				cursym = find_sym(cp,&cp);
				retval = LT_IF;
				if (symbols_policy) {
					list_symbol(symbols_policy,name,cp - name,cursym);
				}
				if (cursym >= 0) {
					if (SYMBOL(cursym)->sym_def == NULL) {
						/* symbol is -Ued */
						retval = ifdef ? LT_FALSE : LT_TRUE;
					}
					else {
						/* symbol is -Ded */
						retval = ifdef ? LT_TRUE : LT_FALSE;
					}
				}
			}
			else { /* Just #if */
				SET_STATE(evaluator,ifpos) = cp;
				retval = eval_if(&cp);
			}
		}
		else if (GET_PUBLIC(line_edit,keyword)[0] == 'e') {
			/* Possible #elif, #else, #endif, or #error */
			char const *kwpos = GET_PUBLIC(line_edit,keyword) + 1;
			--kwlen;
			if (strncmp(kwpos,"lif",kwlen) == 0) { /* #elif */
				retval = eval_if(&cp) - LT_IF + LT_ELIF;
			}
			else if (strncmp(kwpos,"lse",kwlen) == 0) { /* #else */
				retval = LT_ELSE;
			}
			else if (strncmp(kwpos,"ndif",kwlen) == 0) { /* #endif */
				retval = LT_ENDIF;
			}
			else if (strncmp(kwpos,"rror",kwlen) == 0) { /* #error */
				SET_PUBLIC(chew,comment_state) = PSEUDO_COMMENT;
				if (!symbols_policy && !dropping_line()) {
					SET_PUBLIC(chew,last_comment_start_line) =
						GET_PUBLIC(io,line_num);
					if (is_unconditional_line()) {
						set_exit_flags(EVENT_SUMMARY_ERROR_OUTPUT,true);
						if (was_unconditional_line()) {
							report(GRIPE_UNCONDITIONAL_ERROR_INPUT,NULL,
								"An unconditional #error directive was input");
						}
						else {
							report(GRIPE_UNCONDITIONAL_ERROR_INPUT,NULL,
								"An unconditional #error directive was output");
						}
					}
				}
			}
		}
		else if (strncmp(GET_PUBLIC(line_edit,keyword),"define",kwlen) == 0) {
			if (!symbols_policy && !dropping_line()) {
				retval = eval_define(&cp);
			}
		}
		else if (strncmp(GET_PUBLIC(line_edit,keyword),"undef",kwlen) == 0 ) {
			if (!symbols_policy && !dropping_line()) {
				retval = eval_undef(&cp);
			}
		}
		else {
			SET_PUBLIC(chew,line_state) = LS_CODE;
			retval = LT_PLAIN;
		}
		cp = chew_on(cp);
		if (*cp != '\0') {
			SET_PUBLIC(chew,line_state) = LS_CODE;
			if (retval != LT_PLAIN) {
				if (*cp == ')') {
					bail(GRIPE_UNBALANCED_PAREN,
						"Missing \"(\" in \"%.*s\"",
						line_len(GET_PUBLIC(io,line_start)),
						GET_PUBLIC(io,line_start));
				}
				else {
					report(GRIPE_GARBAGE_AFTER_DIRECTIVE,NULL,
						"Garbage following preprocessor directive in \"%.*s\"",
						line_len(GET_PUBLIC(io,line_start)),
						GET_PUBLIC(io,line_start));
				}
			}
		}
		/* chew should have changed the state */
		if (GET_PUBLIC(chew,line_state) == LS_DIRECTIVE) {
			if (input_eof()) {
				parse_error(GRIPE_MISSING_EOF_NEWLINE,"Missing newline at EOF");
			}
			else {
				give_up_confused(); /* bug */
			}
		}
		if (retval != LT_CONTRADICTORY_UNDEF) {
			flush_contradiction();
		}
	}
	if (GET_PUBLIC(chew,line_state) == LS_CODE) {
		while (*cp != '\0') {
			cp = chew_on(cp + 1);
		}
	}
	debug(DBG_17);
	return (retval);
}


/* EOF */

