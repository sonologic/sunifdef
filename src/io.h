#ifndef IO_H
#define IO_H

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

/*! \ingroup io_module io_interface
 * \file io.h
 * This file provides the I/O module interface
 */

/*! \addtogroup io_interface */
/*@{*/
/*! Nominal filename for the standard input stream */
#define STDIN_NAME "[stdin]"

/*! Read the name of a source file from \c stdin.
 *  Filenames may contain spaces if quoted.
 *	\return A pointer to the source filename in static storage,
 *  if a valid filename is read; NULL if no filename is read.
 */
extern
char * read_filename(void);

/*! Open a named file for reading or writing
 *	\param file The name of the file to be opened
 *	\param mode	The read or write mode in which to open the file,
 *	\c "r" or \c "w".
 *	\return  Stream to opened file on success, else NULL.
 */
extern
FILE * open_file(const char *file, const char *mode);

/*! Open an input file and the appropriate
 * output file.

	\param		filename	The name of the input file to open.
				An empty string, "", denotes the standard input
 */
extern void
open_io(char const *filename);

/*! Finalise the current pairing of source input and processed output,
 *	if any.

	\param		error	0 if no errors were raised in processing the
				input file that is to be closed, else the issue code
				of the error that aborted processing.

 *	If a file is associated with either of these streams, it is closed.
 *	The output file replaces the input file, if \c error == 0 and the \c --replace
 *	option is in force, and is also backed up beforehand, if the \c --backup
 *	option is in force. If the function is called before any input has been
 *	opened it is a NOOP.
 */
extern void
close_io(int error);

/*! Close the current source file. */
extern void
close_input(void);

/*! Try to read a line of input from the current source file.
 *	\return \em true iff a line is read.
 */
extern bool
get_line(void);

/*! Read another line of input from the current source file
	to extend the current line when a line-continuation is found or
	when a newline is read within a C-comment.

	\param	readpos	The current text pointer in the line buffer.
	\return The restored text pointer, which
		will differ from \em readpos if the line buffer has been
		reallocated.

	The function assumes that more input is available. If not an
	unexpected eof error is raised.

 */
extern char *
read_more(char const *readpos);

/*! Is the current source file open?
 */
extern bool
input_opened(void);

/*! Is the current source file at end-of-file?
 */
extern bool
input_eof(void);

/*! Ensure additional capacity in the input line-buffer.
 *	\param extra	The number of additional bytes to ensure.
 *	The function will extend the line buffer as necessary to
 *	ensure that \em extra bytes are available.
 */
extern void
ensure_buf(size_t extra);

/*! Save the current line buffer position.
 *
 *	\param cp		The current line buffer position.
 *
 *	The position is saved as an offset into the line buffer
 *	so that the correct position can be recovered is the
 * line buffer is moved.
 */
extern void
save_read_pos(char const *cp);

/*! Get the last saved line buffer position.
 *
 *	\returns cp	A ptr to the line buffer position
 *					last saved by save_read_pos()
 *
 *	The returned position equates to the last position saved
 *	even if the line buffer has moved in the interim.
 */
extern char *
saved_read_pos(void);

/*! Reset the input filename to NULL and line number to 0 */
extern void
io_toplevel(void);

/*! Get the offset of an address in the input line buffer */
extern size_t
read_offset(char const *readpos);

/*! Get the address of an ofset into the input line buffer */
extern char *
read_pos(size_t readoff);


/*@}*/

/*!\ingroup io_interface_state_utils */
/*@(*/

/*! * The public state of the I/O module.*/
PUBLIC_STATE_DEF(io) {
	int line_num;  /*!< The current source line number */
	const char *filename; /*!< The name of the current source file */
	char *line_start; /*!< The start of the current source line in memory */
	char *line_end; /*!< The end of the current source line in memory */
	FILE * output;	/*!< The output stream */
	int extension_lines; /*!< Number of linefeeds embedded in the current
							extended line */
} PUBLIC_STATE_T(io);

/*@}*/

/*!\addtogroup io_interface_state_utils */
/*@(*/
IMPORT(io);
/*@}*/


#endif /* EOF */
