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
#include "filesys.h"
#include "io.h"
#include "args.h"
#include "platform.h"
#include "dataset.h"
#include <ctype.h>

/*!\ingroup io_module, io_interface, io_internals
 *\file io.c
 * This file implements the I/O module
 */

/*! \addtogroup io_internals */
/*@{*/

/*!	Read a source line from the current input stream into the line buffer.
 *  \return \em true iff a source line is read.
 */
static bool
readon(void);

/*!	Generate a temporary file for output, when the \c --replace
 *	option is in force.
 *
 *	The temporary file is created in the same location
 *  as the input source file for ready identification in
 *  the event of a crash.
 */
static void
make_tempfile(void);

/*!	Replace the current input source file with the temporary output
 * 	file, when the \c --replace option is in force.
 *
 *  The input source file has either been deleted or renamed to a backup
 *	when this function is called, depending on whether the \c --backup
 *	option is in force.
 */
static void
replace_infile(void);

/*! Generate a backup name for the current input source file,
 *	when the \c --replace and \c --backup options
 *	are in force.
 *
 *	The backup filename is committed to static storage.
 *	\param filename The name of the source file to be backed up.
 */
static void
make_backup_name(const char *filename);

/*! Backup the current input source file when the \c backup option is
 *	in force.
 *	The current source file is renamed with a generated backup filename.
 */
static void
backup_infile(void);

/*! Delete the current input source file, preparatory to replacing it
 *	with the corresponding output file.
 */
static void
delete_infile(void)
{
	if (remove(GET_PUBLIC(io,filename))) {
		bail(GRIPE_CANT_DELETE_FILE,
			"Cannot remove file \"%s\"",GET_PUBLIC(io,filename));
	}
}


/*! Open an output stream for the current input file.
 *	The output stream is \c stdout unless input source
 *  files are to be replaced with output. Otherwise the
 *	output stream is opened on a temporary output file.
 */
static void
open_output(void);

/*@}*/

/*! \ingroup io_internals_state_utils */
/*@{*/

/*! * The global state of the I/O module. */
STATE_DEF(io) {
	INCLUDE_PUBLIC(io); /*!< The public state of the I/O module */
	FILE * input;	/*!< The input stream */
	size_t bufsz;	/*!< Current size of line buffer */
	size_t linelen;
		/*!< Length of current the line, excluding terminal nul */
	size_t infile;	/*!< Index of current input file */
	char * in_name_buf;
		/*!< Current input filename on heap, if needed */
	char * out_name_buf;
		/*!< Current output filename on heap, if needed */
	char * bak_name_buf; /*!< Backup filename on heap, if needed */
	size_t saved_read_pos; /*!< Saved offset into line buffer */
} STATE_T(io);
/*@}*/

/*! \addtogroup io_internals_state_utils */
/*@{*/
IMPLEMENT(io,ZERO_INITABLE);
/*@}*/

/*! Read the name of a source file from \c stdin.
 *  Filenames may contain spaces if quoted.
 *	\return A pointer to the source filename in static storage,
 *  if a valid filename is read; NULL if no filename is read.
 */
char * read_filename(void)
{
	int ch;
	size_t pos = 0;
	bool quoted;
	char * in_name_buf = GET_STATE(io,in_name_buf);
	if (in_name_buf == NULL) {
		in_name_buf = SET_STATE(io,in_name_buf) = allocate(PATH_MAX + 1);
	}
	ch = getchar();
	in_name_buf[0] = '\0';
	/* Skip whitespace on stdin */
	for (	;ch != EOF && isspace(ch); ch = getchar()){};
	if (ch == EOF) {
		return NULL;
	}
	quoted = ch == '\"';
	if (quoted) {
		for (ch = getchar() ;ch != EOF && ch != '\"'; ch = getchar()) {
			if (isspace(ch) && ch != ' ') {
				in_name_buf[pos] = '\0';
				bail(GRIPE_ILLEGAL_FILENAME,
					"Illegal whitespace in input filename: \"%s...",
					in_name_buf);
			}
			in_name_buf[pos++] = ch;
			if (pos == PATH_MAX) {
				in_name_buf[pos] = '\0';
				bail(GRIPE_FILENAME_TOO_LONG,
					"A filename exceeds max %d bytes: \"%s...",
						PATH_MAX,in_name_buf);
			}
		}
		in_name_buf[pos] = '\0';
		if (ch == EOF) {
			bail(GRIPE_EOF_IN_FILENAME,
					"A quoted input filename is unterminated: \"%s...",
					in_name_buf);
		}
	}
	else {
		for (	;ch != EOF && !isspace(ch); ch = getchar()) {
			in_name_buf[pos++] = ch;
			if (pos == PATH_MAX) {
				in_name_buf[pos] = '\0';
				bail(GRIPE_FILENAME_TOO_LONG,
					"An input filename exceeds max %d bytes: \"%s...",
					PATH_MAX,in_name_buf);
			}
		}
		in_name_buf[pos] = '\0';
	}
	return in_name_buf;
}


/*! Read the remainder of an incomplete line into the
	line buffer.

	Return true if the rest of the line is read, else false
*/
static bool
readon(void)
{
	size_t read = 0;
	for (;;) {
		char *bufp;
		if (GET_STATE(io,linelen) + 1 >= GET_STATE(io,bufsz)) {
			/* Need more buffer */
			SET_PUBLIC(io,line_start)
				= reallocate(GET_PUBLIC(io,line_start),
							SET_STATE(io,bufsz) += BUFSIZ);
		}
		/* Position to end of current line */
		bufp = GET_PUBLIC(io,line_start) + GET_STATE(io,linelen);
		/* Read some more at that position */
		if (NULL == fgets(bufp,(int)(GET_STATE(io,bufsz) -
			 GET_STATE(io,linelen)),GET_STATE(io,input))) {
			/* EOF (or possibly read error). */
			if (ferror(GET_STATE(io,input))) {
				bail(GRIPE_CANT_READ_INPUT,"Read error on file %s",
					GET_PUBLIC(io,filename));
			}
			break;
		}
		/* Update length of line */
		SET_STATE(io,linelen) += read = strlen(bufp);
		bufp += read;
		if (bufp[-1] == '\n') {
			/* End of line. That's all */
			SET_PUBLIC(io,line_end) = bufp;
			break;
		}
	}
	return read != 0;
}

/*! Try to read another line of input from the current source file
	to extend the current line when a line-continuation is found or
	when a newline is read within a C-comment.
	\return \em true iff another line is read.
 */
static bool
extend_line(void)
{
	bool eof = !readon();
	if (!eof) {
		++SET_PUBLIC(io,line_num);
		return true;
	}
	if (GET_STATE(io,linelen) > 0) {
		 parse_error(GRIPE_MISSING_EOF_NEWLINE,
		 	"Missing newline at EOF");
	}
	return false;
}


/*! Generate a temporary filename for an output file
	that will replace its input file
*/
static void
make_tempfile(void)
{
	char *out_name_buf = GET_STATE(io,out_name_buf);
	const char *infile = GET_PUBLIC(io,filename);
	const char *delim = strrchr(infile,PATH_DELIM);
	char const *tempname = NULL;
	size_t dirlen = delim ? delim - infile : 0;
	if (out_name_buf == NULL) {
		out_name_buf = SET_STATE(io,out_name_buf) =
			allocate(PATH_MAX);
	}
	if (dirlen) {
		strncpy(out_name_buf,infile,dirlen);
		out_name_buf[dirlen++] = PATH_DELIM;
	}
	strcpy(out_name_buf + dirlen,"sunifdef_out_XXXXXX");
	tempname = fs_tempname(out_name_buf);
	if (!tempname) {
		bail(GRIPE_NO_TEMPFILE,
				"Cannot create temporary file");
	}
}

static void
replace_infile(void)
{
	if (rename(GET_STATE(io,out_name_buf),
				GET_PUBLIC(io,filename))) {
		bail(GRIPE_CANT_RENAME_FILE,
			"Cannot rename file \"%s\" as \"%s\"",
			GET_STATE(io,out_name_buf),
			GET_PUBLIC(io,filename));
	}
}

static void
make_backup_name(const char *filename)
{
	FILE *exists = NULL;
	size_t suffix_len = strlen(GET_PUBLIC(args,backup_suffix));
	size_t namelen = strlen(filename) + suffix_len;
	if (GET_STATE(io,bak_name_buf) == NULL) {
		SET_STATE(io,bak_name_buf) = allocate(PATH_MAX + 1);
	}
	strncpy(GET_STATE(io,bak_name_buf),
			GET_PUBLIC(io,filename),PATH_MAX)[PATH_MAX] = 0;
	do {
		if (namelen > PATH_MAX) {
			bail(GRIPE_FILENAME_TOO_LONG,
				"A filename exceeds max %d bytes: \"%s...",
					PATH_MAX,GET_STATE(io,bak_name_buf));
		}
		strcat(GET_STATE(io,bak_name_buf),
				GET_PUBLIC(args,backup_suffix));
		exists = fopen(GET_STATE(io,bak_name_buf),"r");
		if (!exists) {
			break;
		}
		fclose(exists);
		namelen += suffix_len;
	} while(true);
}

static void
backup_infile(void)
{
	make_backup_name(GET_PUBLIC(io,filename));
	if (rename(GET_PUBLIC(io,filename),
				GET_STATE(io,bak_name_buf))) {
		bail(GRIPE_CANT_RENAME_FILE,
			"Cannot rename file \"%s\" as \"%s\"",
			GET_PUBLIC(io,filename),
			GET_STATE(io,bak_name_buf));
	}
}


static void
open_output(void)
{
	if (!GET_PUBLIC(args,replace)) {
		SET_PUBLIC(io,output) = stdout;
	}
	else {
		make_tempfile();
		SET_PUBLIC(io,output) =
				open_file(GET_STATE(io,out_name_buf),"w");
	}
}

/* API ***************************************************************/

FILE * open_file(const char *file, const char *mode)
{
	FILE * stream = fopen(file, mode);
	if (stream == NULL) {
		bail(GRIPE_CANT_OPEN_INPUT,"Can't open %s for %s",
		 	file, (mode[0] == 'r' ? "reading": "writing"));
	}
	return stream;
}

void
close_io(int error)
{
	if (GET_STATE(io,input) != NULL) {
		++SET_PUBLIC(dataset,donefiles);
		if (error) {
			++SET_PUBLIC(dataset,errorfiles);
		}
		if (GET_STATE(io,input) != stdin) {
	
			fclose(GET_STATE(io,input));
			SET_STATE(io,input) = NULL;
	
			if (GET_PUBLIC(io,output) != stdout &&
				GET_PUBLIC(io,output) != NULL) {
				fclose(GET_PUBLIC(io,output));
				SET_PUBLIC(io,output) = NULL;
			}
			if (!error && GET_PUBLIC(args,replace)) {
				if (GET_PUBLIC(args,backup_suffix) != NULL) {
					backup_infile();
				}
				else {
					delete_infile();
				}
				replace_infile();
			}
		}
		io_toplevel();
	}
}


void
open_io(char const *filename)
{
	SET_PUBLIC(io,filename) = filename;
	if (!strcmp(GET_PUBLIC(io,filename),STDIN_NAME)) {
		SET_STATE(io,input) = stdin;
	}
	else {
		SET_STATE(io,input) =
			open_file(GET_PUBLIC(io,filename),"r");
		SET_PUBLIC(io,line_num) = 0;
	}
	open_output();
}

bool get_line(void)
{
	SET_STATE(io,linelen) = 0;
	SET_PUBLIC(io,extension_lines) = 0;
	return extend_line();
}

char *
read_more(char const *readpos)
{
	save_read_pos(readpos);
	if (extend_line()) {
		return saved_read_pos();
	}
	early_eof();
	return NULL;
}

size_t
read_offset(char const *readpos)
{
	char *linestart = GET_PUBLIC(io,line_start);
	assert(readpos > linestart);
	return readpos - linestart;
}

char *
read_pos(size_t readoff)
{
	return GET_PUBLIC(io,line_start) + readoff;
}


void
ensure_buf(size_t extra)
{
	size_t spare = GET_STATE(io,bufsz) - GET_STATE(io,linelen);
	int i;
	for ( i = 0; spare <= extra; ++i) {
		SET_STATE(io,bufsz) += BUFSIZ;
		spare = GET_STATE(io,bufsz) - GET_STATE(io,linelen);
	}
	if (i) {
		SET_PUBLIC(io,line_start)
			= reallocate(GET_PUBLIC(io,line_start),
				GET_STATE(io,bufsz));
	}
}

bool
input_opened(void)
{
	return !!GET_STATE(io,input);
}

bool
input_eof(void)
{
	return feof(GET_STATE(io,input));
}

void
save_read_pos(char const *cp)
{
	SET_STATE(io,saved_read_pos) = cp - GET_PUBLIC(io,line_start);
}

char *
saved_read_pos(void)
{
	return GET_PUBLIC(io,line_start) + GET_STATE(io,saved_read_pos);
}

void
io_toplevel(void)
{
	SET_PUBLIC(io,line_num) = 0;
	SET_PUBLIC(io,filename) = NULL;
}


/* EOF */
