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

#include "platform.h"
#include "args.h"
#include "report.h"
#include "categorical.h"
#include "io.h"
#include "line_despatch.h"
#include "symbol_table.h"
#include "filesys.h"
#include "dataset.h"
#include <stdio.h>
#include <string.h>
#include <stdarg.h>


/*!\ingroup args_module args_interface args_internals
 *\file args.c
 * This file implements the Args module
 */

/*! \ingroup args_internals_state_utils */
/*@{*/

/*! The global state of the Args module */
STATE_DEF(args) {
	/*! The public state of the Args module */
	INCLUDE_PUBLIC(args);
	ptr_vector_h argfile_argv;
		/*!< Array of args read from \c --file \c ARGFILE */
	char 	*memfile;
		/*!< Read whole \c ARGFILE into heap here */
	int arg_dirs_ignored;
		/*!< Count of commandline arguments that
			specify input directories ignored because
			--recurse is not specified */
} STATE_T(args);
/*@}*/

/*! \addtogroup args_internals_state_utils */
/*@{*/

IMPLEMENT(args,USER_INITABLE)

/*! \fn void args_init(args_state_t * args_st)
 *	Define initialisation function for the Args module.
 *	\param	 args_st		Pointer to module state
 */
DEFINE_USER_INIT(args)(STATE_T(args) * args_st)
{
	args_st->argfile_argv = ptr_vector_new();
}

/*! \fn void args_finis(args_state_t * args_st)
 *	Define finalisation function for the Args module.
 *	\param args_st		Pointer to module state
 */
DEFINE_USER_FINIS(args)(STATE_T(args) * args_st)
{
	ptr_vector_dispose(&(SET_STATE(args,argfile_argv)));
}
/*@}*/

/*! \addtogroup args_internals */
/*@{*/

/*! Say whether we are suppressing progress messages */
#define PROGRESS_GAGGED() \
((GET_PUBLIC(args,diagnostic_filter) & MSGCLASS_PROGRESS) != 0)

static int optind;
	/*!< Index of option parsed by getopt_long() */
static int optopt;
	/*!< Unrecogised short option parsed by getopt_long() */
static char *optarg;
	/*!< Argument to an option parsed by getopt_long() */

/*! Info structure for a commandline option for getopt_long() */
struct option {
	/*! The name of the option */
	const char *name;
	/*! Can the option take an argument? One of:
		- \c no_argument
		- \c required_argument
		- \c optional argument
	*/
	int has_arg;
	/*! \c NULL here makes getopt_long() return the field \c val if
	 *	this option is matched. Non-null makes getopt_long() return
	 *	0 if any option is matched, and set the \c int addressed by
	 *	<tt>flag == val</tt>/ if this option is matched, otherwise
	 *	leaving it unchanged.
	 */
	int *flag;
	/*!	Value to be returned or stored by getopt_long() if this
	 *	option is matched (depending on \c flag).
	 */
	int val;
};

/*! Possible values of the \c has_arg field in an \c option structure */
enum {
	no_argument, /*!< The option cannot have an argument */
	required_argument, /*!< The option must have an argument */
	optional_argument /*!< The option mat or may not have an argument */
};


/*! New Enumeraton of codes for the commandline options */
enum {
	OPT_FILE = 'f', 		/*!< The \c --file option */
	OPT_BACKUP = 'B', 		/*!< The \c --backup option */
	OPT_REPLACE = 'r', 		/*!< The \c --replace option */
	OPT_DEF = 'D', 			/*!< The \c --define option */
	OPT_UNDEF = 'U', 		/*!< The \c --undefine option */
	OPT_CONFLICT = 'x', 	/*!< The \c --conflict option */
	OPT_GAG = 'g', 			/*!< The \c --gag option */
	OPT_VERBOSE = 'V', 		/*!< The \c --verbose option */
	OPT_COMPLEMENT = 'c', 	/*!< The \c --complement option */
	OPT_DEBUG = 'd', 		/*!< The \c --debug option */
	OPT_CONSTANT = 'n', 	/*!< The \c --constant option */
	OPT_DISCARD = 'k', 		/*!< The \c --discard option */
	OPT_LINE = 'l',			/*!< The \c --line option */
	OPT_SYMBOLS = 's', 		/*!< The \c --symbols option */
	OPT_POD = 'P', 			/*!< The \c --pod option */
	OPT_HELP = 'h', 		/*!< The \c --help option */
	OPT_VERSION = 'v',		/*!< The \c --version option */
	OPT_RECURSE = 'R', 		/*!< The \c --recurse option */
	OPT_FILTER = 'F', 		/*!< The \c --filter option */
	OPT_KEEPGOING = 'K'		/*!< The \c --keepgoing option */
};


/*! Array of structures specifying the commandline options */
static struct option long_options [] = {
	{ "file", required_argument, NULL, OPT_FILE },
	{ "backup", required_argument, NULL, OPT_BACKUP },
	{ "replace",no_argument, NULL, OPT_REPLACE },
	{ "define", required_argument, NULL, OPT_DEF },
	{ "undef", required_argument, NULL, OPT_UNDEF },
	{ "conflict", required_argument, NULL, OPT_CONFLICT },
	{ "gag", required_argument, NULL, OPT_GAG },
	{ "verbose", no_argument, NULL, OPT_VERBOSE },
	{ "complement", no_argument, NULL, OPT_COMPLEMENT },
	{ "debug", no_argument, NULL, OPT_DEBUG },
	{ "constant", required_argument, NULL, OPT_CONSTANT },
	{ "discard", required_argument, NULL, OPT_DISCARD },
	{ "line", no_argument, NULL, OPT_LINE },
	{ "symbols", required_argument, NULL, OPT_SYMBOLS },
	{ "pod", no_argument, NULL, OPT_POD },
	{ "help", no_argument, NULL, OPT_HELP },
	{ "version", no_argument, NULL, OPT_VERSION },
	{ "recurse", no_argument, NULL, OPT_RECURSE },
	{ "filter", required_argument, NULL, OPT_FILTER },
	{ "keepgoing", no_argument, NULL, OPT_KEEPGOING },
	{ 0, 0, 0, 0 }
};

/*! We need the standard getopt_long() function, but the host implementation
 *	cannot be relied on to be re-entrant. Therefore we roll out own
 *	re-entrant version. The interface is like that of its standard namesake.
 */
static int
getopt_long(int argc, char * argv[], const char *optstr,
			const struct option *longopts, int *longind)
{
	char *opt;
	optarg = NULL;
	if (longind) {
		*longind = -1;
	}
	if (optind < 0) {
		return -1;
	}
	if (optind == 0) {	/* Initial or re-initialising call */
		++optind; /* Skip argv[0] */
	}
	if (optind >= argc) {
		return -1;
	}
	opt = argv[optind];
	if (*opt == '-') {	/* argv[optind] is an option */
		++optind;
		if (*++opt == ':') {
			optopt = ':';
			return '?';
		}
		if (*opt != '-') { /* Short option */
			char *where = strchr(optstr,*opt);
			if (!where) {
				optopt = *opt;
				return '?';
			}
			if (where[1] != ':') { 	/* No argument */
				if (opt[1]) {
					optopt = *opt;
					return '?';
				}
				optarg = NULL;
				return *opt;
			}
			else if (where[2] != ':') { /* Optional argument */
				if (opt[1] != 0) {	/* arg abuts opt */
					optarg = opt + 1;
					return *opt;
				}
				else if (optind >= argc || *argv[optind] == '-') {
					/* No arg */
					optarg = NULL;
					return *opt;
				}
				else {	/* Arg is next argv */
					optarg = argv[optind++];
					return *opt;
				}
			}
			else {	/* Required argument */
				if (opt[1] != 0) {	/* arg abuts opt */
					optarg = opt + 1;
					return *opt;
				}
				else if (optind >= argc || *argv[optind] == '-') {
					/* Required arg missing */
					optopt = *opt;
					return '?';
				}
				else {	/* arg is next argv */
					optarg = argv[optind++];
					return *opt;
				}
			}
		}
		else { /* Long option */
			int i, match = -1;
			size_t optlen;
			/* Test for `opt=arg' and get length of opt */
			char *eq = strchr(++opt,'=');
			if (eq) {
				optarg = eq + 1;
				optlen = eq - opt;
			}
			else {
				optlen = strlen(opt);
			}
			/* Test for exactly one match of the option */
			for (i = 0; longopts[i].name != NULL; ++i) {
				if (!strncmp(opt,longopts[i].name,optlen)) {
					if (match != -1) {
						match = -1;
						break;
					}
					match = i;
				}
			}
			if (longind != NULL) {
				*longind = match;
			}
			if (match == -1) { /* Not found */
				return '?';
			}
			switch(longopts[match].has_arg) {
				case no_argument:
					if (optarg) {	/* Illegal arg */
						return '?';
					}
					break;
				case required_argument:
					if (optarg) {	/* Arg after `=' */
						break;
					}
					if (optind >= argc || *argv[optind] == '-') {
						/* Required arg missing */
						return '?';
					}
					/* Arg is next argv */
					optarg = argv[optind++];
					break;
				case optional_argument:
					if (optarg) {	/* Arg after `=' */
						break;
					}
					if (optarg || optind >= argc || *argv[optind] == '-') {
						/* No argument */
						break;
					}
					optarg = argv[optind++];
					break;
				default:
					assert(false);
			}
			/* Store or return value as required */
			if (longopts[match].flag != NULL) {
				*(longopts[match].flag) = longopts[i].val;
				return 0;
			}
			else {
				return longopts[match].val;
			}
		}
	}
	else {	/* argv[optind] is a non-option.
			We want to shuffle it to the end */
		bool done = true;	/* Posit no more options to the right */
		int i;
		for (i = optind; done && i < argc; ++i) {
			/* Now see if this is true */
			 done = *argv[i] != '-';
		}
		if (done) { /* There are no more options to the right */
			optarg = argv[optind]; /* optind at start of non-options */
			return -1; /* All done */
		}
		else { /* There are more options to the right */
			/* Swap the current non-opt to the end of argv */
			char *temp = argv[optind];
			memmove(argv + optind,argv + optind + 1,
				(argc - optind - 1) * sizeof(char *));
			argv[argc - 1] = temp;
			/* Try for another option */
			return getopt_long(argc,argv,optstr,longopts,longind);
		}
	}
}

/*!	Write brief command usage information to a file
 *	\param fp	Stream to which the info will be written.
 */
static void
usage(FILE * fp)
{
	const char *prog_name = GET_PUBLIC(args,prog_name);

	fprintf(fp,"usage:\n"
		"1)\t%s [OPTIONS] [FILE...]\n"
		"\t\tProcess FILEs using OPTIONS. By default read stdin.\n"
		"OPTIONS:\n"
		"-fARGFILE | --file ARGFILE\n"
		"\t\tRead other arguments from ARGFILE.\n"
		"-r, --replace\n"
		"\t\tOverwrite input file with output file.\n"
		"\t\tWith -r, stdin supplies the input filenames.\n"
		"\t\tOtherwise stdin supplies an input file; "
			"the output file is stdout.\n"
		"-R, --recurse\n"
		"\t\tRecurse into directories to find input files.\n"
		"\t\tImplies --replace.\n"
		"-FEXT1[,EXT2...]\n"
		"--filter EXT1[,EXT2...]\n"
		"\t\tProcess only input files that have one of the file extensions "
		"EXT1,EXT2...\n"
		"-BSUFFIX, --backup SUFFIX\n"
		"\t\tBackup each input file by appending SUFFIX to the name.\n"
		"\t\tApplies only with -r.\n"
		"-DSYM, --define SYM\n"
		"\t\tAssume symbol SYM to be defined.\n"
		"-USYM, --undef SYM\n"
		"\t\tAssume symbol SYM to be undefined\n"
		"-xd, --conflict delete\n"
		"\t\tDelete #defines and #undefs that contradict -D or -U args.\n"
		"-xc, --conflict comment\n"
		"\t\tInsert diagnostic comments on contradictions as per -xd. "
			"(Default)\n"
		"-xe, --conflict error\n"
		"\t\tInsert diagnostic #errors on contradictions as per -xd.\n"
		"-g[p|i|w|e|a],\n"
		"--gag [progress| info | warning | error | abend]\n"
		"\t\tSuppress diagnostics no worse than "
			"{progress | info | warning | error | abend}\n"
		"-gs, --gag summary\n"
		"\t\tSuppress summary diagnostics at exit\n"
		"\t\tDefault is -gp -gi -gs\n"
		"-V, --verbose\n"
		"\t\tOutput all diagnostics\n"
		"-c, --complement\n"
		"\t\tComplement. Retain lines that should be dropped and vice versa.\n"
		"\t\t(Retained #-directives will still be simplified where possible.)\n"
		"-D, --debug\n"
		"\t\tDisplay debugging info.\n"
		"-ne[d], --constant eval[,del]\n"
		"\t\tEvaluate constants as truth-functional operands in #ifs, "
		"\and optionally eliminate them.\n"
		"\t\t(Constant arithmetic operands are always evaluated.)\n"
		"-nu, --constant unknown\n"
		"\t\tTreat constants as unknowns for truth functional-simplification of #ifs\n"
		"-kd, --discard drop\n"
		"\t\tDrop discarded lines from output\n"
		"-kb, --discard blank\n"
		"\t\tBlank discarded lines on output\n"
		"-kc, --discard comment\n"
		"\t\tComment out discarded lines on output\n"
		"-K, --keepgoing\n"
		"\t\tIf a parse error is encountered in an input file, continue "
		"processing subsequent input files\n"
		"-P, --pod\n"
		"\t\tApart from #-directives, input is Plain Old Data.\n"
		"-l, --line\n"
		"\t\tOutput #line directives to preserve the line numbers of retained lines\n"
		"\t\tComments and quotations will not be parsed.\n\n"
		"2)\t%s -h, --help\n"
		"\t\tDisplay this help and exit.\n\n"
		"3)\t%s -v, --version\n"
		"\t\tDisplay version info and exit.\n\n"
		"4)\t%s -s{a|f}[l] [OTHER_OPTS] [FILE...]\n"
		"\t%s --symbols { all | first }[,locate] [OTHER_OPTS] [FILE...]\n"
		"\t\tOnly output a list of symbols that control #ifs in FILEs.\n"
		"\t\t\ta, all: list all occurrences of symbols.\n"
		"\t\t\tf, first: list only first occurrence of each symbol.\n"
		"\t\t\tl, locate: list file and line location.\n"
		"\t\tOTHER_OPTS may be -R, --recurse or -F, --filter as per form 1)\n\n",
		prog_name,prog_name,prog_name,prog_name,prog_name);
}

/*!	Wrapper for \c usage(stderr) call, used for \c atexit()
 *	registration
 */
static void
usage_error_summary(void)
{
	usage(stderr);
}

/*!	Write a usage error diagnostic on \c stderr followed by a usage
 *	summary, then terminate the program.
 *
 *	\param reason	Reason code the for usage error
 *	\param format	Format for composing error disgnostic with subsequent args
 *
 *	If \c format is \c NULL then the diagnostic is assumed to be
 * "Invalid usage" and the reason is defaulted to GRIPE_USAGE_ERROR
 */
static void
usage_error(int reason, const char *format,...)
{
	if (format) {
		va_list argp;
		atexit(usage_error_summary);
		va_start(argp,format);
		vbail(reason,format,argp);
		va_end(argp);
	}
	else {
		usage_error(GRIPE_USAGE_ERROR,"Invalid usage");
	}
}

/*!	Write brief command usage information to \c stdout
 *	for the \c --help option, then terminate the program
 *	wihtout error.
 */
static void
usage_help(void)
{
	usage(stdout);
	exit(MSGCLASS_NONE);
}

#ifndef VERSION
#ifdef UNIX
#error VERSION not defined.
#endif
#define VERSION "3.1.3"
#endif
/*!	Write version information about the program on \c stdout
 */
static void
version(void)
{
	printf("%s, version %s for %s (built %s, %s)\n",
		GET_PUBLIC(args,prog_name),VERSION,OS_TYPE,__DATE__,__TIME__);
	exit(MSGCLASS_NONE);
}

/*!	Configure diagnostic output in response to a \c --gag or \c --verbose
 *	option.
 *	\param	arg		Either "verbose" or an argument to the \c --gag option
 */
static void
config_diagnostics(const char * arg)
{
	int mask = MSGCLASS_NONE;
	if (GET_PUBLIC(args,diagnostic_filter) < 0 && strcmp(arg,"verbose")) {
		report(GRIPE_VERBOSE_ONLY,NULL,
			"Can't mix --verbose with --gag.'--gag %s' ignored",arg);
		return;
	}
	if (!strcmp(arg,"progress")) {
		mask = MSGCLASS_PROGRESS;
	}
	else if (!strcmp(arg,"info")) {
		mask = MSGCLASS_INFO;
	}
	else if (!strcmp(arg,"warning")) {
		mask = MSGCLASS_WARNING;
	}
	else if (!strcmp(arg,"error")) {
		mask = MSGCLASS_ERROR;
	}
	else if (!strcmp(arg,"abort")) {
		mask = MSGCLASS_ABEND;
	}
	else if (!strcmp(arg,"summary")) {
		mask = MSGCLASS_SUMMARY;
	}
	else if (!strcmp(arg,"verbose")) {
		int diagnostic_filter = GET_PUBLIC(args,diagnostic_filter);
		if (diagnostic_filter > 0) {
			report(GRIPE_VERBOSE_ONLY,NULL,
				"Can't mix --verbose with --gag. '--verbose' ignored");
			return;
		}
		else if (diagnostic_filter < 0) {
			report(GRIPE_DUPLICATE_MASK,NULL,"'--verbose' already seen");
			return;
		}
		SET_PUBLIC(args,diagnostic_filter) = -1;
		return;
	}
	else {
		usage_error(GRIPE_USAGE_ERROR,"Invalid argument for --gag: \"%s\"",arg);
	}
	if (mask & GET_PUBLIC(args,diagnostic_filter)) {
		report(GRIPE_DUPLICATE_MASK,NULL,
				"'--gag %s' already seen",arg);
	}
	SET_PUBLIC(args,diagnostic_filter) |=  mask;
	if (mask == MSGCLASS_INFO) {
		config_diagnostics("progress");
	}
	else if (mask == MSGCLASS_WARNING) {
		config_diagnostics("info");
	}
	else if (mask == MSGCLASS_ERROR) {
		config_diagnostics("warning");
	}
	else if (mask == MSGCLASS_ABEND) {
		config_diagnostics("error");
	}
}

/*!		Configure the final state of diagnostic output filtering.

		If no \c --gag or \c --verbose option has been seen,
			default to <tt>--gag info --gag summary</tt>.
*/

static void
finalise_diagnostics(void)
{
	int mask = GET_PUBLIC(args,diagnostic_filter);
	if (!mask) {
		/* Default diagnostic masking to no progress
			messages, no infos, no summaries */
		config_diagnostics("info");
		config_diagnostics("summary");
	}
	else if (mask == -1) {
		/* --verbose was temporarily set as -1 to block later
			--gag options. Can now reset as 0 */
		SET_PUBLIC(args,diagnostic_filter) = 0;
	}
}

/*!	Parse commandline options from a file specified with the \c --file
 *	option.
 *	\param	argsfile	Name of the file from which to read options
 *	Options are separated by whitespace in the file. The parsed options
 *	are stored in an \c argv array held in the module's private state.
 *	This array can then be parsed by parse_args() as if passed to main().
 */
static void
parse_args_file(const char *argsfile)
{
	char *arg, * delims=" \t\n\r";
	char **argv;
	int argc;
	size_t filesz,read;
	FILE *in;
	if (GET_STATE(args,memfile)) {
		bail(GRIPE_MULTIPLE_ARGFILES,"--file can only be used once");
	}
	in = open_file(argsfile,"r");
    fseek(in,0,SEEK_END);
    filesz = ftell(in);
	fseek(in,0,SEEK_SET); /* Back to start of file */
	SET_STATE(args,memfile) = allocate(filesz + 1);
	ptr_vector_append(GET_STATE(args,argfile_argv),GET_PUBLIC(args,prog_name));
		/* Remember program name comes first */
	read = fread(GET_STATE(args,memfile),1,filesz,in);
	if (!read) { /* Read args file into heap */
		bail(GRIPE_CANT_READ_INPUT,"Read error on file %s",argsfile);
		/* Not bullet-proof to assume that only 0 bytes read is
		a read error, but cannot portably test `read' == `filesz'
		because if there are newlines in the file then on Windows, fewer
		bytes will be read than there are in the file due to newline
		conversion */

	}
	fclose(in);
	GET_STATE(args,memfile)[read] = '\0'; /* 0-terminate commandline */
	for(arg = strtok(GET_STATE(args,memfile),delims);
		arg != NULL; arg = strtok(NULL,delims)) {
		ptr_vector_append(GET_STATE(args,argfile_argv),arg);
	}
	argc = (int)ptr_vector_count(GET_STATE(args,argfile_argv));
	argv = (char **)ptr_vector_start(GET_STATE(args,argfile_argv));
	parse_args(argc,argv);
}

/*!	Parse the policy for handling constants in <tt>#if</tt>s from the
 *	argument to the \c --constant option.
 */
static void
parse_constants_policy(const char *optarg)
{
	bool eval_consts = false;
	bool delete_consts = false;

	int optlen = (int)strlen(optarg);
	if (optlen < 3) {
		/* Short form */
		eval_consts = optarg[0] == 'e';
		if (!eval_consts && optarg[0] != 'u') {
			usage_error(GRIPE_USAGE_ERROR,
				"Invalid argument for -n: \"%s\"",optarg);
		}
		delete_consts = optlen == 2 && optarg[1] == 'd';
		if (!delete_consts && optlen == 2) {
			usage_error(GRIPE_USAGE_ERROR,
				"Invalid argument for -n: \"%s\"",optarg);
		}
	}
	else {
		/* Long form */
		char *comma = strchr(optarg,',');
		if (comma) {
			size_t len = comma - optarg;
			if (!strncmp("eval",optarg,len)) {
				eval_consts = true;
			}
			else {
				usage_error(GRIPE_USAGE_ERROR,
					"Invalid argument for --constant: \"%s\"",optarg);
			}
			if (!strcmp(comma + 1,"del")) {
				delete_consts = true;
			}
			else {
				usage_error(GRIPE_USAGE_ERROR,
					"Invalid argument for --constant: \"%s\"",optarg);
			}
		}
		else if (!strcmp("eval",optarg)) {
			eval_consts = true;
		}
		else if (strcmp("unk",optarg)) {
			usage_error(GRIPE_USAGE_ERROR,
				"Invalid argument for --constant: \"%s\"",optarg);
		}
	}
	SET_PUBLIC(args,eval_consts) = eval_consts;
	SET_PUBLIC(args,del_consts) = delete_consts;
}

/*!	Parse the policy for listing symbols that control <tt>#if</tt>s
 	from the argument to the \c --symbols option.
 */
static void
parse_symbols_policy(const char *optarg)
{
	bool bad_arg = false;
	SET_PUBLIC(args,symbols_policy) = SYMBOLS_LIST;
	if (optarg) {
		if (strlen(optarg) <= 2) {
			switch(optarg[0]) {
			case 'a':
				break;
			case 'f':
				SET_PUBLIC(args,symbols_policy) |= SYMBOLS_LIST_FIRST;
				break;
			case 'l':
				SET_PUBLIC(args,symbols_policy) |= SYMBOLS_LOCATE;
				break;
			default:
				bad_arg = true;
			}
			switch(optarg[1]) {
			case 0:
				break;
			case 'a':
				if (optarg[0] == 'a') {
					bad_arg = true;
				}
				break;
			case 'f':
				if (optarg[0] == 'f') {
					bad_arg = true;
				}
				SET_PUBLIC(args,symbols_policy) |= SYMBOLS_LIST_FIRST;
				break;
			case 'l':
				if (optarg[0] == 'l') {
					bad_arg = true;
				}
				SET_PUBLIC(args,symbols_policy) |= SYMBOLS_LOCATE;
				break;
			default:
				bad_arg = true;
			}
		}
		else if (!strcmp(optarg,"all")) {}
		else if (!strcmp(optarg,"first")) {
			SET_PUBLIC(args,symbols_policy) |= SYMBOLS_LIST_FIRST;
		}
		else if (!strcmp(optarg,"locate")) {
			SET_PUBLIC(args,symbols_policy) |= SYMBOLS_LOCATE;
		}
		else {
			char const *comma = strchr(optarg,',');
			if (comma) {
				if (!strncmp(optarg,"all",comma - optarg)) {}
				else if (!strncmp(optarg,"first",comma - optarg)) {
					SET_PUBLIC(args,symbols_policy) |= SYMBOLS_LIST_FIRST;
				}
				else if (!strncmp(optarg,"locate",comma - optarg)) {
					SET_PUBLIC(args,symbols_policy) |= SYMBOLS_LOCATE;
				}
				else {
					bad_arg = true;
				}
				if (!strcmp(comma + 1,"all")) {
					if (!strncmp(optarg,"all",comma - optarg)) {
						bad_arg = true;
					}
				}
				else if (!strcmp(comma + 1,"first")) {
					if (GET_PUBLIC(args,symbols_policy) & SYMBOLS_LIST_FIRST) {
						bad_arg = true;
					}
					else {
						SET_PUBLIC(args,symbols_policy) |= SYMBOLS_LIST_FIRST;
					}
				}
				else if (!strcmp(comma + 1,"locate")) {
					if (GET_PUBLIC(args,symbols_policy) & SYMBOLS_LOCATE) {
						bad_arg = true;
					}
					else {
						SET_PUBLIC(args,symbols_policy) |= SYMBOLS_LOCATE;
					}
				}
				else {
					bad_arg = true;
				}
			}
			else {
				bad_arg = true;
			}
		}
		if (bad_arg) {
			usage_error(GRIPE_USAGE_ERROR,
				"Invalid argument for --symbols: \"%s\"",optarg);
		}
	}
}

/*!
	Check the consistency of the commandline options
	and rectify innocuous mistakes.
*/
static void
sanity_checks(void)
{
	bool list_symbols_only = GET_PUBLIC(args,symbols_policy) != SYMBOLS_NO;
	bool replace = GET_PUBLIC(args,replace);
	bool recurse = GET_PUBLIC(args,recurse);
	char *backup_suffix = GET_PUBLIC(args,backup_suffix);
	size_t symbols = ptr_vector_count(GET_PUBLIC(symbol_table,sym_tab));

	if (list_symbols_only && symbols > 0) {
		usage_error(GRIPE_INVALID_ARGS,
			"--symbols does not mix with --define,--undefine");
	}
	if (symbols == 0 && !list_symbols_only) {
		usage_error(GRIPE_NOTHING_TO_DO,
			"Nothing to do. Must --define or --undefine at least one symbol");
	}
	if (list_symbols_only) {
		if (recurse) {
			replace = SET_PUBLIC(args,replace) = false;
		}
		if (replace) {
			report(GRIPE_REDUNDANT_OPTION,NULL,
			"--replace is redundant with --symbols");
			SET_PUBLIC(args,replace) = false;
		}
		if (backup_suffix != NULL) {
			report(GRIPE_REDUNDANT_OPTION,NULL,
			"--backup is redundant with --symbols");
			backup_suffix = SET_PUBLIC(args,backup_suffix) = NULL;
		}
		line_despatch_no_op();
	}
	if (backup_suffix != NULL && !replace) {
		usage_error(GRIPE_INVALID_ARGS,
			"--backup needs --replace");
	}
}

/*!
	Add files to the input dataset.

	\param path	File or directory to be included in the
					input dataset.

	\return True if \c path is added to the input dataset,
	else false.

	If \e path is a file that satisfies any \c --filter option
	it is added to the input dataset.

	If \e path is a directory and \c --recurse is in force then
	files recursively beneath it that satisfy any \c --filter
	option are added to the input dataset.

	If \e path is a directory and \c --recurse is not in force
	then the directory is ignored.

*/
static bool
add_files(char const *path)
{
	fs_obj_type_t obj_type = fs_file_or_dir(path);
	if (FS_IS_FILE(obj_type) || GET_PUBLIC(args,recurse)) {
		dataset_add(path);
		return true;
	}
	report(GRIPE_DIR_IGNORED,NULL,
			"--recurse not specified. Ignoring directory \"%s\"",path);
	return false;
}


/*@}*/

/* API *********************************************************************/

void
parse_executable(char **argv)
{
	char *last_slash;
	SET_PUBLIC(args,exec_path) = *argv;
	last_slash = strrchr(*argv,PATH_DELIM);
	if (!last_slash) {
		SET_PUBLIC(args,prog_name) = *argv;
	}
	else {
		SET_PUBLIC(args,prog_name) = last_slash + 1;
	}
}

void
parse_args(int argc, char *argv[])
{
	static const char * const opts = "x:g:p:f:D:U:B:F:n:k:s:PRrcdlhvVK";
	static bool parsing_file;
	int args = argc;
	int opt, save_ind, long_index;

	for (optind = 0;
		(opt = getopt_long(argc,argv,opts,long_options,&long_index)) != -1; ) {
		switch (opt) {
		case OPT_FILE:	/* Read further args from file */
			save_ind = optind;
				/* Remember where we have parsed up to */
			parsing_file = true;
			parse_args_file(optarg); /* Parse file */
			parsing_file = false;
			optind = save_ind; /* Restore position */
			break;
		case OPT_CONFLICT:	/* Policy for contradictions */
			{
				int conflict_policy = CONTRADICTION_COMMENT;
				if (strlen(optarg) > 1) {
					if (!strcmp(optarg,"delete")) {
						conflict_policy = CONTRADICTION_DELETE;
					}
					else if (!strcmp(optarg,"comment")) {
						conflict_policy = CONTRADICTION_COMMENT;
					}
					else if (!strcmp(optarg,"error")) {
						conflict_policy = CONTRADICTION_ERROR;
					}
					else {
						usage_error(GRIPE_USAGE_ERROR,
							"Invalid argument for --conflict: \"%s\"",optarg);
					}
				}
				else {
					switch(*optarg) {
					case 'd':
						conflict_policy = CONTRADICTION_DELETE;
						break;
					case 'c':
						conflict_policy = CONTRADICTION_COMMENT;
						break;
					case 'e':
						conflict_policy = CONTRADICTION_ERROR;
						break;
					default:
						usage_error(GRIPE_USAGE_ERROR,
							"Invalid argument for -x: \"%c\"",*optarg);
					}
				}
				contradiction_policy(conflict_policy);
			}
			break;
		case OPT_GAG:
			{
				char const *gag_arg = optarg;
				if (strlen(optarg) == 1) {
					switch(*optarg) {
					case 'p':
						gag_arg = "progress";
						break;
					case 'i':
						gag_arg = "info";
						break;
					case 'w':
						gag_arg = "warning";
						break;
					case 'e':
						gag_arg = "error";
						break;
					case 'a':
						gag_arg = "abend";
						break;
					case 's':
						gag_arg = "summary";
						break;
					default:
						assert(false);
					}
				}
				config_diagnostics(gag_arg);
			}
			break;
		case OPT_VERBOSE:
			config_diagnostics("verbose");
			break;
		case OPT_DEF: /* define a symbol */
			add_specified_symbol(true,optarg);
			break;
		case OPT_UNDEF: /* undef a symbol */
			add_specified_symbol(false, optarg);
			break;
		case OPT_COMPLEMENT: /* treat -D as -U and vice versa */
			SET_PUBLIC(args,complement) = true;
			break;
		case OPT_REPLACE:
			SET_PUBLIC(args,replace) = true;
			break;
		case OPT_DEBUG:
			debugging(true);
			break;
		case OPT_BACKUP:
			SET_PUBLIC(args,backup_suffix) = optarg;
			break;
		case OPT_CONSTANT: /* policy for constants in #ifs */
			parse_constants_policy(optarg);
			break;
		case OPT_DISCARD: /* policy for discarding lines on output */
			if (strlen(optarg) > 1) {
				if (!strcmp("drop",optarg)) {
					SET_PUBLIC(args,discard_policy) = DISCARD_DROP;
				}
				else if (!strcmp("blank",optarg)) {
					SET_PUBLIC(args,discard_policy) = DISCARD_BLANK;
				}
				else if (!strcmp("comment",optarg)) {
					SET_PUBLIC(args,discard_policy) = DISCARD_COMMENT;
				}
				else {
					usage_error(GRIPE_USAGE_ERROR,
						"Invalid argument for --discard: \"%s\"",optarg);
				}
			}
			else {
				switch(*optarg) {
				case 'd':
					SET_PUBLIC(args,discard_policy) = DISCARD_DROP;
					break;
				case 'b':
					SET_PUBLIC(args,discard_policy) = DISCARD_BLANK;
					break;
				case 'c':
					SET_PUBLIC(args,discard_policy) = DISCARD_COMMENT;
					break;
				default:
					usage_error(GRIPE_USAGE_ERROR,
						"Invalid argument for -k: \"%c\"",*optarg);
				}
			}
			break;
		case OPT_LINE:
			SET_PUBLIC(args,line_directives) = true;
			break;
		case OPT_SYMBOLS: /* just list symbols that control #ifs */
			parse_symbols_policy(optarg);
			break;
		case OPT_POD: /* don't parse quotes or comments */
			SET_PUBLIC(args,plaintext) = true;
			break;
		case OPT_HELP: /* help */
			usage_help();
			break;
		case OPT_VERSION: /* version */
			version();
			break;
		case OPT_RECURSE: /* recurse into directories */
			SET_PUBLIC(args,recurse) = true;
			SET_PUBLIC(args,replace) = true;
			break;
		case OPT_FILTER: /* Filter input by file extensions */
			dataset_filter_filetypes(optarg);
			break;
		case OPT_KEEPGOING: /* Continue to process subsequent in
								put files after errors */
			SET_PUBLIC(args,keepgoing) = true;
			break;
		default:
			usage_error(GRIPE_USAGE_ERROR,
				"Invalid option: \"%s\"",argv[optind - 1]);
		}
	}
	if (!parsing_file) {
		SET_PUBLIC(args,got_opts) = true;
		finalise_diagnostics();
		if (!PROGRESS_GAGGED()) {
			heap_str argstr = concatenate(args,argv,' ');
			report(PROGRESS_GOT_OPTIONS,NULL,"Args: %s",argstr);
			free(argstr);
		}
		sanity_checks();
		if (argc) {
			report(PROGRESS_BUILDING_TREE,NULL,"Building input tree");
		}
	}
	argc -= optind;
	argv += optind;
	for (	;argc; --argc,++argv) {
		if (!add_files(*argv)) {
			++SET_STATE(args,arg_dirs_ignored);
		}
	}

}


void
finish_args(void)
{
	bool list_symbols_only = GET_PUBLIC(args,symbols_policy) != SYMBOLS_NO;
	bool replace = GET_PUBLIC(args,replace);
	bool input_is_stdin = false;

	if (file_tree_is_empty(GET_PUBLIC(dataset,file_tree)) &&
		GET_STATE(args,arg_dirs_ignored) == 0) {
		/* No input files on command line */
		if (!GET_PUBLIC(args,replace)) {
			/* Without --replace, stdin is the input file */
			input_is_stdin = true;
		}
		else {
			/* With --replace, stdin supplies input filenames */
			char * infile;
			while ((infile = read_filename()) != NULL) {
				(void)add_files(infile);
			}
		}
	}
	if (file_tree_is_empty(GET_PUBLIC(dataset,file_tree)) && !input_is_stdin) {
		bail(GRIPE_NOTHING_TO_DO,
			"Nothing to do. No input files.");
	}
	if (!list_symbols_only && !input_is_stdin &&
		file_tree_count(GET_PUBLIC(dataset,file_tree),FT_COUNT_FILES,NULL) > 1 &&
		!replace) {
		bail(GRIPE_ONE_FILE_ONLY,
		"Need --replace to process multiple files");
	}
	report(PROGRESS_FILE_TALLY,NULL,"%d files to process",
		file_tree_count(GET_PUBLIC(dataset,file_tree),FT_COUNT_FILES,NULL));
#ifdef DEBUG_FILE_TREE
	file_tree_dump(GET_PUBLIC(dataset,file_tree));
	exit(0);
#endif

}

/* EOF */
