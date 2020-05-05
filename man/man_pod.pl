=pod

=begin html

<table width="100%"><tr><td align="left">SUNIFDEF(1)</td><td align="center">User Commands</td><td align="right">SUNIFDEF(1)</td></tr></table>

=end html

=head1 NAME

sunifdef - simplify C preprocessor source files

=head1 SYNOPSIS

sunifdef [B<-v> | B<--version>]

sunifdef [B<-h> | B<--help>]

sunifdef [OPTION...] [I<files>...]

=head1 DESCRIPTION

B<sunifdef> is a more powerful successor of the FreeBSD B<unifdef> tool. B<sunifdef> is a preprocessor of C or C++ preprocessor source files (or more briefly a preprocessor of C/C++ source files).

From the commandline arguments it takes a set of assumptions about the symbols to be defined, or undefined, for the CPP. From the commandline it also takes one or more source files. It parses these source files to pick out conditional preprocessor directives (B<#if>,B<#ifdef>,B<#ifndef>,B<#else>,B<#elif>,B<#endif>). It applies the specified assumptions to these directives in attempt to evaluate them. Directives that cannot be fully evaluated on the basis of the assumptions are simplified as much as possible. Directives that can be fully evaluated are eliminated, and the source text that they control is either retained or deleted in accordance with the evaluation, mimicking the behaviour of the CPP.

B<sunifdef> also detects B<#define> and B<#undef> directives and checks them for consistency with the specified assumptions. If a B<#define> or B<#undef> directive repeats one of the assumptions it is deleted on output; if it conflicts with any of the assumptions then it may be deleted or replaced with a diagnostic comment or a diagnostic B<#error>, depending on commandline options.

For each source file, an output file is generated that reflects the simplifications arising from the specified assumptions. The command

B<sunifdef -DFOO bar.c>

will write on the standard output a revision of the file B<bar.c> that has been purged as far as possible of preprocessor constructions controlled by the truth-value of B<defined(FOO)>. This revision is equivalent to B<bar.c> on the assumption that B<FOO> is defined. With appropriate options and inputs, you can use a B<sunifdef> command to perform wholesale removal of redundant preprocessor complexities from a C or C++ source tree. See the B<EXAMPLES> section.

=head1 OPTIONS

=over

=item B<-h>,B<--help>

Display a usage summary and exit.

=item B<-v>,B<--version>

Display version information and exit.

=item B<-s>[B<f>|B<a>][B<l>], B<--symbols> [B<first> | B<all>][B<,locate>]

Output a list of symbols that are determinative for the truth value of B<#if> conditions.

B<f>, B<first>: List only the first occurrence of the symbol on input.

B<a>, B<all>: List all occurrences of the symbol on input.

B<l>, B<locate>: Report the file and line number of each listed occurrence.

=item B<f>I<argfile>, B<--file> I<argfile>

Read (more) arguments from file I<argfile>. Arguments may be written free-form, separated by whitespace, in I<argfile>. These arguments will be parsed exactly as if they were listed on the commandline at the position of -B<f>I<argfile>.

=item B<-D>I<macro>[B<=>I<string>], B<--define> I<macro>[B<=>I<string>]

Assume that B<#define> I<macro>[B<=>I<string>] is in force for processing the input file(s).

=item B<-U>I<macro>, B<--undef> I<macro>

Assume that B<#undef> I<macro> is in force for processing the input file(s).

=item B<-r>, B<--replace>

Replace each input file with the corresponding output file. I<You must specify this option to process multiple input files>.

The option changes the default behaviour of the command when no input files are specified. In this case, input is acquired from the standard input. If B<-r> is I<not> specified, then a single input file is read from the standard input. If B<-r> is specified then the I<names> of the input files are read from the standard input. Note that B<--recurse> implies B<--replace>.

If the names of the input files are read from stdin, the filenames are delimited by whitespace unless enclosed in double-quotes.

=item B<-R>, B<--recurse>

Recurse into directories to find input files. Implies B<--replace>. The input files may include directories with this option: otherwise a directory provokes a non-fatal error.

All files within a directory (and within subdirectories) will be selected for input unless the B<--filter> option is given: otherwise all files (including subdirectories) will be selected that match the B<--filter> option.

When B<--recurse> is in effect, B<sunifdef> builds a graph of all unique input files once and for all as it parses the filenames that are explicitly supplied and before it processes any of them. New files that may later appear in input directories during execution will not be processed, and files that have disappeared from input directories when they are due to be processed will provoke fatal errors.

=item B<-F>I<ext1>[B<,>I<ext2>...], B<--filter> I<ext1>[B<,>I<ext2>...]

Process only input files that have one of the file extensions I<ext1>,I<ext2>... A file extension may be any terminal segment of a filename that follows a '.'.

=item B<-B>I<suffix>, B<--backup> I<suffix>

Backup each input file before replacing it, the backup file having the same name as the input file with I<suffix> appended to it.

=item B<-x>[B<d>|B<c>|B<e>], B<--conflict> [B<delete> | B<comment> | B<error>]

Select the action to be taken when a B<#define> or B<#undef> directive is encountered in an input file that conflicts with one of the B<-D> or B<-U> assumptions:

B<d>, B<delete>: Delete the conflicting directive.

B<c>, B<comment>: Replace the conflicting directive with a diagnostic comment (B<default>).

B<e>, B<error>: Replace the conflicting directive with a diagnostic B<#error> directive.

=item B<-g>[B<p>|B<i>|B<w>|B<e>|B<a>], B<--gag> [B<progress> | B<info> | B<warning> | B<error> | B<abend>]

Suppress diagnostics no worse than [B<progress> | B<info> | B<warning> | B<error> | B<abend>].

=item B<-gs>, B<--gag summary>.

Suppress summary diagnostics at end of input.

=item B<-V>, B<--verbose>

Output all diagnostics,

If neither B<-V> nor B<-g>I<arg> is specified defaults are B<-gp -gi -gs>.

=item B<-n>[B<u>|B<e>[B<d>]], B<--constant> [B<unk> | B<eval>[B<,del>]]

Select the policy for processing constants in B<#if> directives:

B<u>, B<unk>: Treat constants as unknowns, i.e. like macros that are not subject to any assumptions (B<default>).

B<e>[B<d>], B<eval>[B<,del>]: Evaluate constants [and optionally eliminate them].

=item B<-c>, B<--complement>

Ouput the lines that ought to be dropped and vice versa.

=item B<-d>, B<--debug>

Write debugging information to stderr.

=item B<-k>[B<d>|B<b>|B<c>], B<--discard> [B<drop> | B<blank> | B<comment>]

Select the policy for discarding lines from output:

B<d>, B<drop>: Drop discarded lines.

B<b>, B<blank>: Blank discarded lines.

B<c>, B<comment>: Comment out discarded lines.

=item B<-K>, B<--keepgoing>

If a parse error is encountered in an input file, continue processing subsequent input files. An event of severity B<abend> will terminate processing regardless of B<--keepgoing>.

=item B<-P>, B<--pod>

Apart from CPP directives, input is to be treated as Plain Old Data. C/C++ comments and quotations will not be parsed. 

=item B<-l>, B<--line>

Output #line directives in place of discarded lines to preserve the line numbers of retained lines.

=back

=head1 EXAMPLES

=over

=item B<sunifdef -DUNIX -UWIN32 foo.c>

=item B<sunifdef --define UNIX --undef WIN32 foo.c>

Simplify the file B<foo.c> assuming that the symbol B<UNIX> is defined and the symbol B<WIN32> is undefined. Write the simplified file to stdout. By default diagnostic messages whose severity is I<warning> or higher will be output and no summary diagnostics will be output. All diagnostics are written to stderr.

=item B<sunifdef -DUNIX=1 -UWIN32 foo.c>

=item B<sunifdef --define UNIX=1 --undef WIN32 foo.c>

Like the previous example, but the symbol B<UNIX> is defined as 1.

=item B<sunifdef -gw -DUNIX -UWIN32 foo.c>

=item B<sunifdef --gag warn --define UNIX --undef WIN32 foo.c>

Like the first example, but suppress all diagnostics (--gag) whose severity is warning or lower that would otherwise be written to stderr.

=item B<sunifdef -gw -DUNIX -UWIN32 foo.c>

=item B<sunifdef --gag warn --define UNIX --undef WIN32 foo.c>

Like the first example, but suppress all diagnostics (B<--gag>) whose severity is I<warning> or lower that would otherwise be written to stderr. 

=item B<sunifdef -gw -gs -DUNIX -UWIN32 foo.c>

=item B<sunifdef --gag warn -gag summary --define UNIX --undef WIN32 foo.c>

Like the previous example, but also suppress all summary diagnostics that would otherwise be written to stderr after processing is finished (B<--gag summary>).

=item B<sunifdef -V -DUNIX -UWIN32 foo.c>

=item B<sunifdef --verbose --define UNIX --undef WIN32 foo.c>

Like the previous example, but write all diagnostics at all severities to stderr, as well as summary diagnostics (B<--verbose>).

=item B<sunifdef -DUNIX -UWIN32 < bar.c>

=item B<sunifdef --define UNIX --undef WIN32 < bar.c>

Like the previous example, but write only the default diagnostics to stderr and read the input file from stdin (in this case redirected from F<bar.c>)

=item B<sunifdef -r -DUNIX -UWIN32 foo.c bar.c>

=item B<sunifdef --replace --define UNIX --undef WIN32 foo.c bar.c>

Like the previous example, but B<--replace> causes each input file to be replaced with the corresponding simplified output file. With this option multiple input files - F<foo.c>, F<bar.c> - can be supplied.

=item B<sunifdef -r -DUNIX -UWIN32 < filelist.txt>

=item B<sunifdef --replace --define UNIX --undef WIN32 < filelist.txt>

Like the previous example, but read the list of input filenames from stdin (in this case redirected from F<filelist.txt>)

=item B<sunifdef -r -B.bak -DUNIX -UWIN32 < filelist.txt>

=item B<sunifdef --replace --backup ".bak" --define UNIX --undef WIN32 < filelist.txt>

Like the previous example, but create a backup of each input file with the extension B<.bak> (B<--backup ".bak">).

=item B<sunifdef -R -DUNIX -UWIN32 foo.c somedir bar.h otherdir>

=item B<sunifdef --recurse --define UNIX --undef WIN32 foo.c somedir bar.h otherdir>

The B<--recurse> option implies B<--replace> and causes sunifdef to find additional input files by searching recursively within the directories F<somedir> and F<otherdir>

=item B<sunifdef -R -Fc,h -DUNIX -UWIN32 foo.c somedir bar.h otherdir>

=item B<sunifdef --recurse --filter c,h --define UNIX --undef WIN32 foo.c somedir bar.h otherdir>

Like the previous example, but select only input files that have one of the extensions B<.c> or B<.h> (B<--filter c,h>).

=item B<sunifdef -R -Fc,h -K -DUNIX -UWIN32 foo.c somedir bar.h otherdir>

=item B<sunifdef --recurse --filter c,h --keepgoing --define UNIX --undef WIN32 foo.c somedir bar.h otherdir>

Like the previous example, but keep going through parse errors (B<--keepgoing>). Processing of the input file in error will be abandoned but subsequent input files will be processed.

=item B<sunifdef -R -Fc,h -sf foo.c somedir bar.h otherdir>

=item B<sunifdef --recurse --filter c,h --symbols first foo.c somedir bar.h otherdir>

Recursively select all the F<.c> and F<.h> files from F<foo.c>, F<somedir>, F<bar.h>, F<otherdir> and write on stderr a list of all the symbols that influence the truth-values of B<#if>, B<#else>, B<#elif> conditions. Report only the first occurrence of each symbol.

=item B<sunifdef -R -Fc,h -sfl foo.c somedir bar.h otherdir>

=item B<sunifdef --recurse --filter c,h --symbols first,locate foo.c somedir bar.h otherdir>

Like the previous example, but report the file and line number of each reported symbol (B<--symbols first,locate>)

=item B<sunifdef -R -Fc,h -sal foo.c somedir bar.h otherdir>

=item B<sunifdef --recurse --filter c,h --symbols all,locate foo.c somedir bar.h otherdir>

Like the previous example, but report all occurrences of the symbols (B<--symbols first,locate>)

=item B<sunifdef -P -DUNIX -UWIN32 data.txt>

=item B<sunifdef --pod --define UNIX --undef WIN32 data.txt>

Process the file F<data.txt> with the assumptions B<--define UNIX> and B<--undef WIN32> parsing the text (other than B<#>-directives) as Plain Old Data, rather than C/C++ source. C/C++ comments and quotations will not be recognised.

=item B<sunifdef -R -fargs.txt foo.c somedir bar.h otherdir>

=item B<sunifdef -R --file args.txt foo.c somedir bar.h otherdir>

Interprolate the contents of the file F<args.txt> into the commandline, replacing B<--file args.txt> and then execute the resulting command.

=item B<sunifdef -fargs.txt>

=item B<sunifdef --file args.txt>

Substite the contents of the file F<args.txt> for B<--file args.txt> and then execute the resulting command.

=back

=head1 DIAGNOSTICS

=over

=item Diagnostics written to stderr are classified by severity. Each diagnostic includes a distinct hexadecimal code of the form C<0xXXXXX> that encodes its severity. The 5 severities are:

=item Z<>

B<progress>: Progress messages (C<0xXXXXX & 0x00800> is true)

B<info>: Noteworthy information (C<0xXXXXX & 0x01000> is true)

B<warning>: Indicating problematic input (C<0xXXXXX & 0x02000> is true)

B<error>: Indicating invalid input (C<0xXXXXX & 0x04000> is true)

B<abend>: Indicating a fatal environment or internal error (C<0xXXXXX & 0x08000> is true)

=item Unless B<--gag summary> is in force, B<sunifdef> can write summary diagnostics at the end of processing. A summary diagnostic has a hexadecimal code B<S> that encodes one of the severities and in addition B<S> C<& 0x10000> is true. Even if B<--gag summary> is not in force, a summary will not be written if its severity is suppressed by one of the specified or default B<--gag options>. Since all summaries have severity I<info> or I<warning>, this means that by default no summaries will appear and to obtain all summaries you must specify B<--verbose>. The summaries include:

=item Z<>

B<info>: The number of input files that were reached and the number that were not reached (due to abend).

B<info>: The number of input files reached that were abandoned (due to errors).

=item If there was no abend or error, then additional summaries are written (unless suppressed) indicating each of the following outcomes that has occurred:

=item Z<>

B<info>: Input lines were dropped on output.

B<info>: Input lines were changed on output.

B<warning>: Input lines were changed to B<#error> directives.

B<warning>: Unconditional #error directives were output.

=item sunifdef returns a system code B<SC> of which the low order half of the low order byte is always meaningful:

=item Z<>

B<SC> C<& 1>: Informational diagnostics accrued.

B<SC> C<& 2>: Warnings diagnostics accrued.

B<SC> C<& 4>: Error diagnostics accrued. (Input files provoking errors will be unchanged notwithstanding the B<--replace> option.)

B<SC> C<& 8>: An abend occurred. Some input files may not have been reached.

=item If no error or abend is indicated, then the high order half of the low order byte is also meaningful:

B<SC> C<& 16>: Input lines were dropped on output.
B<SC> C<& 32>: Input lines were changed on output.
B<SC> C<& 64>: Input lines were changed to #error directives.
B<SC> C<& 128>: Unconditional #error directives were output.

=item The system code reflects diagnostics that were provoked even if they were not actually output due to B<--gag> options.

=back

=head1 BUGS

The conditional operator B<?...:...> is not parsed.

Trigraphs are not parsed.

B<#define> and B<#undef> directives that are found to be active are not factored into the evalation of subsequent B<#if> directives.

Please report bugs to bugs dot sunifdef at strudl dot org

=head1 AUTHOR

Mike Kinghan imk at strudl dot org

=head1 SEE ALSO

FreeBSD B<unifdef(1)>

=begin html

<p><table width="100%"><tr><td align="left">strudl.org</td><td align="center">{{REL_MONTH}}</td><td align="right">SUNIFDEF(1)</td></tr></table>

=end html

=cut 
