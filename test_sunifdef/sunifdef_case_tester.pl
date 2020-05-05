#!/usr/bin/perl

use strict;
use Getopt::Long;
use SunifdefLib;
use File::Path;
use File::Spec;
use Cwd 'abs_path';

my $verbosity;
my $pkgdir;
my $execdir = "../src";
my $help = 0;
my $bail = 0;
my $windows_exe;
my $testdir = "test_cases";
my %optmap = (	'verbosity' => \$verbosity,
				'pkgdir' => \$pkgdir,
				'execdir' => \$execdir,
				'help' => \$help,
				'bail' => \$bail,
                'winexe' => \$windows_exe);

my $prog = "sunifdef_case_tester";
my $tests = 0;
my $fails = 0;
my $passes = 0;
my @testfiles;

sub parse_testfile($);
sub compose_test_command($$$);
sub run_test($$$$$);
sub parse_syscode_from_stderr($);
sub verify_output($$);
sub match_words($$);

set_prog($prog);
set_usage(
		"$prog: - Run a sunifdef test case\n" .
		"Usage:\n" .
		"$prog [--pkgdir PKGDIR] [--execdir DIR] [--verbosity=LEVEL] [--bail ] [TESTFILE...]\n" .
		"$prog --help\n" .
		"Arguments:\n" .
		"-h | --help            Display this information on stdout.\n" .
		"-p | --pkgdir PKGDIR     The sunifdef package directory. Default '..'\n" .
		"-e | --execdir EXECDIR   Directory from which to run sunidef: Default '../src'.\n " .
		"-v | --verbosity LEVEL   Display diagnostics with severity >= LEVEL, where " .
    	"LEVEL = 'progress', 'info', 'warning', 'error' or 'fatal. Default = 'progress'\n" . 
		"-b | --bail            Bail at the first error. " .
		"-w | --winexe          The sunifdef executable is a native windows executable" .
		"Default: Keep going through errors.\n" .
		"TESTFILE\t\tThe test case specification file.\n" .
		"If TESTFILE... is absent then PKGDIR/test_sunifdef/test_cases/*.c is assumed\n" . 
		"\tTESTFILE contains sample code on which sunifdef is to be tested. " .
		"If there exists a file called TESTFILE.expect then that file contains " .
		"the code that sunifdef is expected to emit from the test case. " .
		"Surplus whitespace is ignored for the purpose of " .
		"comparing TESTFILE with TESTFILE.expect.\n" .
		"\tTESTFILE has a mandatory header comment of the form:\n" .
		"\t\t/**ARGS:\n" .
		"\t\t\tARGS_BODY\n" .
		"\t\t*/\n" .
		"where ARGS_BODY comprises the commandline options to be passed to " .
		"sunifdef.\n" .
		"\tOptionally, TESTFILE also has a header comment of the form:\n" .
		"\t\t/**ALTFILES:\n" .
		"\t\t\tFILENAME1 [FILENAME2...]\n" .
		"\t\t*/\n" .
		"The listed files will be input to sunifdef with the arguments " .
		"ARGS_BODY instead of TESTFILE itself.\n" .
		"If FILEENAMEn is relative then it is assumed to be relative to PKGDIR/test_sunifdef.\n" .
		"\tOptionally, TESTFILE also has a header comment of the form:\n" .
		"\t\t/**SYSCODE:\n" .
		"\t\t\t[=]FLAGS\n" .
		"\t\t*/\n" .
		"where FLAGS is a bitset to be satisfied by the sunifdef return code " .
		"from running the test case. If the prefix `=' is present then the " .
		"return code must exactly match FLAGS. Otherwise the bitset " .
		"is satisfied provided that all bits set in FLAGS are set in the " .
		"return code. FLAGS may be a decimal numeral or an |-combination " .
		"of decimal numerals.\n");

GetOptions(\%optmap, 'verbosity=s', 'pkgdir=s', 'execdir=s', 'help!', 'bail!', 'winexe!')  or usage_error();

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
	
if (@ARGV != 0) {
	@testfiles = <@ARGV>;
}
else {
	@testfiles = <$pkgdir/test_sunifdef/test_cases/*.c>;
}

$tests = @testfiles;
bail(1,"*** No tests found! ***") unless($tests);

my $executable = "$execdir/sunifdef";

foreach my $testfile (@testfiles) {
	my ($test_args,$test_syscode,$test_exact,$test_altfiles) =
		parse_testfile($testfile);
	if (!run_test(	$testfile,
					$test_args,
					$test_syscode,
					$test_exact,
					$test_altfiles)) {
		last if (++$fails && $bail);
	}
}

print "$fails out of $tests failed\n";

exit($fails);

sub import_test_cases()
{
	my $curfile = $_;
	
}

sub parse_syscode_from_stderr($)
{
	my $stderr = shift;
	my $syscode = "";
	open IN,"<$stderr" or bail(1,"*** Cannot open \"$stderr\" for reading ***");
	my $line = "";
	while ($line = <IN>) {
		chomp $line;
		if ($line =~ m/exit code (0x[a-fA-F0-9]+)/) {
			$syscode = $1;
			last;
		}
	}
	close(IN);
	return $syscode;
}

sub match_words($$)
{
	my ($actual_word,$expected_word) = @_;
	return 1 if ($actual_word eq $expected_word);
	return 0 unless ($expected_word =~ m#^\./(test_cases/(altfiles/)?test\d{4}(-\d)?\.c)(\(\d+\))$#);
	my $filename = $1;
	my $lineno = $4;
	my $expected_len = length($expected_word) - 2;
	my $actual_len = length($actual_word);
	$filename =~ s#/#\\#g if ($windows_exe);
	my $substring = substr($actual_word,$actual_len - $expected_len);
	if (substr($actual_word,$actual_len - $expected_len) eq "$filename$lineno") {
		$filename = "$pkgdir/test_sunifdef/$filename";
		if ($windows_exe) {
    	   $filename =~ s#/#\\#g;
    	   $filename =~ s#\\cygdrive\\(\w)\\#$1:\\#g if cygwin();
		}		
		return 1 if ($actual_word eq "$filename$lineno");		
	}
	return 0;
}

sub verify_output($$)
{
	my ($expected_output,$output) = @_;
	local $/;
	open IN, "<$expected_output" or
		bail(1,"*** Cannot open \"$expected_output\"for reading ***");
	my $expected_text = <IN>;
	close(IN);
	open IN, "<$output" or
		bail(1,"*** Cannot open \"$output\" for reading***");
	my $actual_text = <IN>;
	close(IN);
	my @expected_words = split(/\s+/,"$expected_text");
	my @actual_words = split(/\s+/,"$actual_text");
	my $awords = @actual_words;
	my $ewords = @expected_words;
	return 0 if (@actual_words != @expected_words);
	for (my $word = 0; $word < @actual_words; ++$word) {
		return 0 unless
			match_words($actual_words[$word],$expected_words[$word]);
	}
	return 1;
}

sub run_test($$$$$)
{
	my (	$testfile,
			$test_args,
			$test_syscode,
			$test_exact,
			$test_altfiles) = @_;

	my $test_command = compose_test_command($test_args,
											$testfile,
											$test_altfiles);
	my $syscode_correct = 0;
	my $output_correct = 0;

	progress("*** Running test file \"$testfile\"");
	progress("*** ARGS: $test_args");
	if ($test_altfiles) {
		progress("*** ALTFILES: $test_altfiles");
	}
	if ($test_exact) {
		progress("*** SYSCODE: = 0x%04x",$test_syscode);
	}
	else {
		progress("*** SYSCODE: match 0x%04x",$test_syscode);
	}

	system($test_command);
	my $actual_syscode = parse_syscode_from_stderr("$testfile.stderr");
	bail(1,"*** Could not parse system code from $testfile.stderr ***") unless ($actual_syscode);
	$actual_syscode = hex($actual_syscode); 
	if ($test_exact) {
		$syscode_correct = ($actual_syscode == $test_syscode);
	}
	else {
		$syscode_correct = (($actual_syscode & $test_syscode) == $actual_syscode);
	}
	$output_correct = verify_output("$testfile.expect","$testfile.output");
	error("*** FAILED test file \"%s\". Unexpected system code 0x%04x",
			$testfile,$actual_syscode)
		unless $syscode_correct;
	error( "*** FAILED test file \"$testfile\". Unexpected output")
		unless $output_correct;
	if ($syscode_correct && $output_correct) {
		progress("*** PASSED");
		return 1;
	}
	return 0;
}

sub compose_test_command($$$)
{
	my ($test_args,$testfile,$test_altfiles) = @_;
	my $testfiles = $testfile;
	if ($test_altfiles) {
		my @altfiles = split(/\s/,$test_altfiles);
		print "@altfiles\n";
		foreach (@altfiles) {
			$_ = File::Spec->rel2abs($_,"$pkgdir/test_sunifdef") unless (File::Spec->file_name_is_absolute($_));
		}
		$testfiles = join(' ',@altfiles);
	}
	if ($windows_exe) {
	   $testfiles =~ s#/#\\\\#g;
	   $testfiles =~ s#\\\\cygdrive\\\\(\w)\\\\#$1:\\\\#g;
    }
	my $test_command = "\"$executable\" $test_args --verbose $testfiles " .
		"1>$testfile.output 2>$testfile.stderr";	
	return $test_command;
}

sub parse_testfile($)
{
	my $testfile = shift;

	my $test_args = "";
	my $test_syscode = 0;
	my $test_exact = 0;
	my $test_altfiles = "";
	my $line_end = $/;

	open (TESTFILE, "<$testfile") or
		bail(1, "*** Cannot open file \"$testfile\" for reading ***");
	undef $/;
	my $text = <TESTFILE>;
	close TESTFILE;
	$/ = $line_end;

	if ($text =~ m#\/\*\*ARGS:\s*(.+?)\s*\*\/#) {
		$test_args = $1;
	}
	if ($text =~ m#\/\*\*SYSCODE:\s*(.+?)\s*\*\/#) {
		$test_syscode = $1;
		if ($test_syscode =~ m/^\s*(=)\s*(.+)$/) {
			$test_exact = 1;
			$test_syscode = $2;
		}
		$test_syscode = eval($test_syscode);
	}
	if ($text =~ m#\/\*\*ALTFILES:\s*(.+?)\s*\*\/#) {
		$test_altfiles = $1;
	}
	return ($test_args,$test_syscode,$test_exact,$test_altfiles);
}

