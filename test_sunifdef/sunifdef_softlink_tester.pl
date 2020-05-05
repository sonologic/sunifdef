#!/usr/bin/perl

use strict;
use Getopt::Long;
use File::Find;
use File::Copy;
use File::Path;
use File::Spec;
use Cwd qw(realpath);
use SunifdefLib;

my $verbosity = 0;
my $help;
my $pkgdir;
my $execdir = "../src";
my $rootname = 'scrap';
my $stderr_file = "stderr.temp.txt";
my $stdout_file = "stdout.temp.txt";
my $keep = 0;
my $fails = 0;
my $windows_exe = 0;
my %optmap = (	'pkgdir' => \$pkgdir,
				'execdir' => \$execdir,
				'rootname' => \$rootname,
				'keep' => \$keep,
				'help' => \$help);

my $prog = "sunifdef_softlink_tester";
my $infiles = 0;

sub copy_test_file_to_scrap();
sub make_softlink($$);
sub softlink_testfile();
sub check_test_result();
sub run(@);
sub slurp($);

END {
	unless($keep) {
		unlink("link2args.h");
		rmtree("link2scrap");
		rmtree($rootname);
	}
	unless($fails) {
		unlink("$stderr_file") if ( -f "$stderr_file");
		unlink("$stdout_file") if ( -f "$stdout_file");
	}
}

set_prog($prog);

set_usage(
	"$prog: Exercise sunifdef in traversing a directory tree containing softlinks " .
	"to verify that sunifdef follows them correctly\n".
	"Usage:\n" .
	"$prog [--verbosity=LEVEL] [--pkgdir PKGDIR] [--execdir EXECDIR] [--rootname DIRNAME] [--keep]\n" .
	"$prog --help\n" .
	"Arguments:\n" .
	"-v | --verbosity LEVEL   Display diagnostics with severity >= LEVEL, where " .
    "LEVEL = 'progress', 'info', 'warning', 'error' or 'fatal. Default = 'progress'\n" . 
	"-h | --help              Display this information on stdout.\n" .
	"-p | --pkgdir PKGDIR     The sunifdef package directory. Default '..'\n" .
	"-e | --execdir EXECDIR   Directory beneath PKGDIR from which to run sunidef: Default 'src'.\n" .
	"-r | rootname            The name of the root directory of the test tree.\n" .
	"-k | --keep              Do not delete the directory tree at exit.\n");

GetOptions(	\%optmap,
			'pkdir=s',
			'execdir=s',
			'help!',
			'verbosity=s',
			'rootname=s',
			'keep!',
            'winexe!')  or usage_error();

set_verbosity($verbosity);

if ($help) {
	help();
	exit(0);
}

if ("$^O" =~ m/MSWin/ || cygwin()) {
    info("This test is not applicable on Windows or Cygwin");
    exit(0);
}

unless (defined($pkgdir)) {
	if (defined($ENV{'SUNIFDEF_PKGDIR'})) {
		$pkgdir = "$ENV{'SUNIFDEF_PKGDIR'}";
	}
	else {
		$pkgdir = "..";
	}
}



$pkgdir = realpath($pkgdir);
$execdir = realpath($execdir);
$rootname = File::Spec->rel2abs($rootname);

system("chmod -R +w $pkgdir") unless windows();

if ( -l "link2args.h") {
	unlink("link2args.h");
}
if ( -l "link2scrap") {
	unlink("link2scrap");
}
if ( -d "$rootname") {
	rmtree("$rootname");
}

mkdir($rootname);
mkdir("$rootname/subdir1");
make_softlink($rootname,"link2scrap");

find(\&copy_test_file_to_scrap,("$pkgdir/src"));

bail(1,"*** No test data found! Bailing out confused ***") if $infiles == 0;

find(\&softlink_testfile,("$rootname"));
make_softlink("$rootname/subdir1","$rootname/link2subdir1");
make_softlink("$rootname/subdir1","$rootname/subdir1/self_including_link");
make_softlink("$rootname/args.h","link2args.h");
progress("*** Symlink Test: to process $infiles files ***");

# Run sunifdef recursively on a dataset in which numerous
# members are symbolic links to other members, to ensure
# that symbolic links are resolved.
run("$execdir/sunifdef --symbols first --verbose --recurse --filter c,h  $rootname link2scrap link2args.h 2> $stderr_file");
progress("*** Done ***");
check_test_result();

exit($fails);

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

sub run(@)
{
	my @cmd = @_;
	progress("@cmd");
	my $ret = system(@cmd);
	return $ret >> 8;
}

sub check_test_result()
{
	my $fail = 0;
	my $stderr_text = slurp("$stderr_file");
	if ($stderr_text !~ m/info 0x11370: $infiles out of $infiles input files were reached/) {
		++$fails;
		error("*** Test Failed! See $stderr_file ****");
	}
}

sub make_softlink($$)
{
	my ($target,$link) = @_;
	system("ln -s $target $link") &&
		die("Failed to make softlink \"$target\" -> \"$link\"");
}

sub copy_test_file_to_scrap()
{
	if ($_ =~ m/\.c$/ or $_ =~ m/\.h$/) { 
		copy($_,"$rootname/$_");
		++$infiles;
	}
}

sub softlink_testfile()
{
	if (($_ =~ m/\.c$/ or $_ =~ m/\.h$/) && $File::Find::dir ne "$rootname/subdir1") { 
		make_softlink($File::Find::name,"$rootname/subdir1/link2$_");
	}
}
