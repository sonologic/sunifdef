#!/usr/bin/perl

use strict;
use Getopt::Long;
use File::Path;
use File::Spec;
use File::Find;
use Cwd 'abs_path';
use SunifdefLib;

my $pkgdir;
my $execdir = "../src";
my $depth = 4;
my $breadth = 5;
my $keep = 0,
my $seed = 987654321;
my $help;
my $verbosity = 'progress';
my $windows_exe;
my $bail;

my $scrapdir;
my $arg_scrapdir;
my $infiles = 0;
my @scrap_files = ();
my $sabotaged_files = 0;
my @infiles_list = ();
my $fails = 0;
my $undefs = "";
my $stderr_file = "stderr.temp.txt";
my $stdout_file = "stdout.temp.txt";
my $infiles_file = "infiles.temp.txt";
my $undefs_file = "undefs.temp.txt";

sub gather_scrap_file();
sub tally_source_file();
sub restore_backed_up_file();
sub append_to_infile_list();
sub run(@);
sub run_noerr(@);
sub slurp($);
sub check_test_result(@);

my %optmap = (	'pkgdir' => \$pkgdir,
				'execdir' => \$execdir,
				'depth' => \$depth,
				'breadth' => \$breadth,
				'keep' => \$keep,
				'seed' => \$seed,
				'help' => \$help,
				'verbosity' => \$verbosity,
				'bail' => \$bail,
                'winexe' => \$windows_exe);				

my $prog = "sunifdef_bulk_tester";

END {
    if ( -d "$scrapdir") {
	   rmtree("$scrapdir") unless $keep;
	}   
	unless($fails) {
		unlink("$stderr_file") if ( -f "$stderr_file");
		unlink("$stdout_file") if ( -f "$stdout_file");
		unlink("$infiles_file") if ( -f "$infiles_file");
		unlink("$undefs_file") if ( -f "$undefs_file");
	}
}

set_prog($prog);

set_usage(
	"$prog: Exercise sunifdef in traversing a directory tree. " .
	"A roughly random tree of directories is created in PKGDIR/test_sunifdef/scrap " .
	"and each directory is populated with the sunifdef source files from PKGDIR/src. " .
	"The directory tree is passed into a number of sunifdef tests.\n" .
	"Usage:\n" .
	"$prog [--verbosity=LEVEL] [--pkgdir PKGDIR] [--execdir EXECDIR] " .
	"[--breadth BREADTH] [--depth DEPTH] [--seed SEED] [--keep]\n" .
	"$prog --help\n" .
	"Arguments:\n" .
	"-v | --verbosity LEVEL   Display diagnostics with severity >= LEVEL, where " .
    "LEVEL = 'progress', 'info', 'warning', 'error' or 'fatal. Default = 'progress'\n" . 
	"-h | --help              Display this information on stdout.\n" .
	"-p | --pkgdir PKGDIR     The sunifdef package directory. Default '..'\n" .
	"-e | --execdir EXECDIR   Directory beneath PKGDIR from which to run sunidef: Default 'src'.\n " .
	"-b | --breadth NUMBER    There will be at most NUMBER subdirectories in a directory. Default 5.\n" .
	"-d | --depth NUMBER      A directory will have at most NUMBER levels of subdirectories. Default 4.\n" .
	"-s | --seed NUMBER       Seed the pseudo random number generator with " .
	"NUMBER. Default 987654321.\n" .
	"-k | --keep              Do not delete the directory tree at exit.\n" .
	"-b | --bail			  Quit at the first test failure.\n" .
	"-w | --winexe          The sunifdef executable is a native windows executable" .	
	"NUMBER must in all cases be > 0.\n" .
	"BEWARE!: The size of the hierarchy grows exponentially with BREADTH and DEPTH. " .
	"BREADTH = 20, DEPTH = 10 is likely to create in the order of a million files.\n");


GetOptions(	\%optmap,
			'pkgdir=s',
			'execdir=s',
			'depth=i',
			'breadth=i',
			'keep!',
			'seed=i',
			'help!',
			'verbosity=s',
			'bail!',
            'winexe!')  or usage_error();

set_verbosity($verbosity);

if ($help) {
	help();
	exit(0);
}

unless (defined($pkgdir)) {
	if (defined($ENV{'SUNIFDEF_PKGDIR'})) {
		$pkgdir = "$ENV{'SUNIFDEF_PKGDIR'}";
	}
	else {
		$pkgdir = File::Spec->updir();
	}
}

$execdir = "$pkgdir/src" unless (defined($execdir));

$pkgdir = abs_path($pkgdir);
$execdir = abs_path($execdir);

system("chmod -R +w $pkgdir") unless windows();

$scrapdir = "$pkgdir/test_sunifdef/scrap";
if ($windows_exe && cygwin()) {
    $arg_scrapdir = cyg2win($scrapdir);
}
else {
    $arg_scrapdir = $scrapdir;    
}
 
# Create and populate directory structure for bulk tests 
progress("*** Generating data for bulk tests. Patience! ***");
find(\&gather_scrap_file,("$pkgdir/src"));
run_noerr("perl -Mstrict -I $pkgdir/perl -w $pkgdir/test_sunifdef/scrap_dir_tree.pl --verbosity=fatal --wipe --seed=$seed --depth=$depth --rootname=$scrapdir " . 
	"--breadth=$breadth @scrap_files");

# Count the *.c and *.h files created.
find(\&tally_source_file,($scrapdir));
bail(1,"*** No test data found! Bailing out confused ***") if $infiles == 0;

progress("*** Bulk Test 1: to process $infiles files ***");
# Run sunifdef with --recurse to list all preprocessor
# symbols in .c and .h files, passing the scrap tree _twice_
# Test that the number of files processed is the number
# of .c and .h files
run("$execdir/sunifdef --symbols first --verbose --recurse --filter c,h $arg_scrapdir $arg_scrapdir " . 
	"2> $stderr_file 1> $stdout_file");
progress("*** Done ***");
check_test_result(1);

progress("*** Bulk Test 2: to process $infiles files ***");
# Run sunifdef with --recurse on the scrap tree again, to
# apply --undef SYM to all .c and .h files, for symbol SYM
# that was found by the first test.

# Build list of -U-macros for all symbols found
open IN,"<$stdout_file" or die("Cannot open file \"$stdout_file\" for reading\n");
my $line;
while($line = <IN>) {
	chomp($line);
	$undefs .= "-U$line ";
}
close(IN);
if ($windows_exe && cygwin()) {
    $undefs =~ s/\r//g;
}

run("$execdir/sunifdef $undefs --verbose --recurse --filter c,h --backup \"~\" $arg_scrapdir 2> $stderr_file");
progress("*** Done ***");
check_test_result(2);

progress("*** Bulk Test 3: to process $infiles files ***");
# Run sunifdef as per the 2nd test, but this time read all the
# file and directory names in the scrap tree from stdin.

# Restore all the files backed up by the last test.
find(\&restore_backed_up_file,($scrapdir));
# Push all input files onto @infile_list.
find(\&append_to_infile_list,($scrapdir));
# List all input files to $infiles_file
open OUT,">$infiles_file" or die("Cannot open \"$infiles_file\" for writing\n");
foreach (@infiles_list) {
	print OUT "$_\n";
}
close(OUT);
run("$execdir/sunifdef $undefs --verbose --recurse --filter c,h --backup \"~\" 2> $stderr_file < $infiles_file");
progress("*** Done ***");
check_test_result(3);

progress("*** Bulk Test 4: to process $infiles files ***");
# Run sunifdef with --recurse on the scrap tree again, this time
# reading all the --undef options of test 2 from a file.

# Restore all the files backed up by the last test.
find(\&restore_backed_up_file,($scrapdir));
open OUT,">$undefs_file" or die("Cannot open \"$undefs_file\" for writing\n");
print OUT "$undefs\n";
close(OUT);
run("$execdir/sunifdef --file $undefs_file --verbose --recurse --filter c,h --backup \"~\" $arg_scrapdir 2> $stderr_file");
progress("*** Done ***");
check_test_result(4);

progress("*** Bulk Test 5: to process $infiles files ***");
# Run sunifdef with --recurse on the scrap tree again, this time
# reading all the --undef options of test 2 from a file.

# Restore all the files backed up by the last test.
find(\&restore_backed_up_file,($scrapdir));

# Sabotage many source files in the scrap tree by deleting
# all occurrences of the #endif directive. Then run sunifdef
# to traverse the scrap tree and determine that it drops all
# the sabotaged files.
#
find(\&sabotage_source_file,($scrapdir));
progress("*** Generated $sabotaged_files invalid input files ***");
run("$execdir/sunifdef --symbols first --verbose --keepgoing --recurse --filter c,h $arg_scrapdir 2> $stderr_file");
progress("*** Done ***");
check_test_result(5,"$sabotaged_files were abandoned due to parse errors");

exit($fails);

sub check_test_result(@)
{
	my $fail = 0;
	my ($test,$pattern) = @_;
	my $stderr_text = slurp("$stderr_file");
	if ($stderr_text !~ m/info 0x11370: $infiles out of $infiles input files were reached/) {
		$fail = 1;
	}
	if (!$fail && defined($pattern)) {
		$fail = 1 if ($stderr_text !~ m/$pattern/);
	}
	if ($fail) {
		++$fails;
		error("*** Bulk test $test: Failed! See $stderr_file ****");
		if ($bail) {
		  exit($fails);
    }
	}
}

sub tally_source_file()
{
	my $file = $File::Find::name;
	++$infiles if ($file =~ m/\.c$/ or $file =~ m/\.h$/); 
}

sub gather_scrap_file()
{
    my $file = $File::Find::name;
    push(@scrap_files,"\"$file\"") if ($file =~ m/\.h$/ or $file =~ m/\.c$/); 
}

sub restore_backed_up_file()
{
	my $file = $_;
	if ($file =~ m/(.+)~$/) {
		unlink ($1) or die("Cannot delete file \"$1\"\n");
		rename($file,$1) or die("Can't rename \"$file\" as \"$1\"\n");
	}
}

sub append_to_infile_list()
{
	my $file = $File::Find::name;
	$file = cyg2win($file) if ($windows_exe && cygwin());
	push(@infiles_list,"\"$file\"");
}

sub sabotage_source_file()
{
	my $file = $_;
	my $sabotaged = 0;
	if ($file =~ m/\.c$/ or $file =~ m/\.h$/) {
		open IN,"<$file" or die("Cannot open file \"$file\" for reading\n");
		my @lines = <IN>;
		foreach (@lines) {
			if ($_ =~ m/^#endif(.*$)/) {
				$_ = $1;
				$sabotaged = 1;
			}
		}
		close(IN);
		open OUT,">$file" or die("Cannot open file \"$file\" for writing\n");
		print OUT @lines;
		close(OUT);
		$sabotaged_files += $sabotaged;
	}
}
	
sub run(@)
{
	my @cmd = @_;
	progress("@cmd");
	my $ret = system(@cmd);
	return $ret >> 8;
}

sub run_noerr(@)
{
	my $ret = run(@_);
	die("Command failed: \"@_\": $!\n") if ($ret);
}

sub slurp($)
{
	my $terminator = $/;
	open IN,"<$_[0]" or die("Cannot open file \"$_[0]\" for reading\n");
	undef $/;
	my $text = <IN>;
	close(IN);
	$/ = $terminator;
	return $text;
}

