#!/usr/bin/perl

use strict;
use Getopt::Long;
use File::Path;
use File::Spec;
use File::Copy;
use SunifdefLib;

my $depth;
my $breadth;
my $rootname = 'scrap';
my $wipe,
my $seed;
my $help;
my $verbosity;
my %optmap = (	'depth' => \$depth,
				'breadth' => \$breadth,
				'rootname' => \$rootname,
				'wipe' => \$wipe,
				'seed' => \$seed,
				'help' => \$help,
				'verbosity' => \$verbosity);
my $prog = "scrap_dir_tree";
my @alphabet = ( 'a'..'z','A'..'Z','0'..'9','_' );

set_prog($prog);
set_usage(	"$prog: - Create a random structure of randomly named directories\n" .
			"Usage:\n" .
			"$prog --depth NUMBER --breadth] NUMBER " .
			"[--rootname DIRNAME] [--wipe] [--seed NUMBER] [--verbosity=LEVEL] [FILE...]\n" .
			"$prog --help\n" .
			"Arguments:\n" .
			"-h | --help              Display this information on stdout.\n" .
			"created in any directory.\n" .
			"-r | rootname            The name of the root directory of the tree. " .
			"Default \"scrap\". Must not already exist unless --wipe is specified.\n" .
			"-w | --wipe              Remove the tree if it already exists.\n" .
			"-b | --breadth NUMBER    There will be at most NUMBER subdirectories in a directory\n" .
			"-d | --depth NUMBER      The structure will contain at most NUMBER levels.\n" .
			"-s | --seed NUMBER       Seed the pseudo random number generator with " .
			"NUMBER. If not specified use implementation default.\n" .
			"-v | --verbosity LEVEL   Display diagnostics with severity >= LEVEL, where " .
    		"LEVEL = 'progress', 'info', 'warning', 'error' or 'fatal'. Default = 'progress'\n" . 
			"FILE...                  Any specified files will be copied into each created directory.\n" .
			"NUMBER must in all cases be > 0.\n");

sub gen_dirs($$$);
sub gen_scrap_name($);

GetOptions(	\%optmap,
			'depth=i',
			'breadth=i',
			'rootname=s',
			'wipe!',
			'seed=i',
			'help!',
			'verbosity=s')  or usage_error();

set_verbosity($verbosity);

my @scrapfiles = <@ARGV>;

if ($help) {
	help();
	exit(0);
}

if (defined($depth)) {
	bail(1,"--depth must be > 0") unless ($depth > 0);
	progress("Make max $depth levels of directories");
}
else {
	usage_error("No --depth specified");
}
if (defined($breadth)) {
	bail(1,"--breadth must be > 0") unless ($breadth > 0);
	progress("Make max $depth directories per level");
}
else {
	usage_error("No --breadth specified");
}

if (defined($seed)) {
	srand($seed);
}
else {
	progress("No pseudo-random seed given: using implementation default.");	
}

if ( -d "$rootname") {
	if ($wipe) {
		rmtree("$rootname");
	}
	else {
		bail(1,"Root directory \"$rootname\" already exists");
	}
}

progress("Creating scrap directories");
my ($dirs_made,$files_made) = gen_dirs("$rootname",$breadth,$depth);
progress("Made $dirs_made directories, $files_made files");
exit 0;

sub gen_scrap_name($)
{
	my $maxlen = shift;
	my $len = rand($maxlen) + 1;
	$len = 26 if ($len > 26);
		
	my $scrap_name = "";
	foreach (1..$len) {
		$scrap_name .= $alphabet[rand(@alphabet)];
	}
	return $scrap_name;
}

sub gen_dirs($$$)
{
	my ($path, $breadth, $maxdepth) = @_;
	$path = File::Spec->rel2abs($path) unless File::Spec->file_name_is_absolute($path);
	my $dirs_made = 0;
	my $files_made = 0;
	if ($breadth && $maxdepth) {
		my $maxlen = 256 - length($path);
		for (my $dir = 0; $dir < $breadth; ++$dir) {
			my $next_path = $path;
			$next_path .= '/';
			last if ($maxlen < 1);
			my $dirname = gen_scrap_name($maxlen);
			$next_path .= $dirname;
			if (scalar(mkpath($next_path))) {
				++$dirs_made;
				progress("Made directory: \"$next_path\"");
				foreach my $scrapfile (@scrapfiles) {
					my $file = (File::Spec->splitpath($scrapfile))[2];
					my $destfile = "$next_path/$file";
					copy($scrapfile,$destfile) or fatal("$!");
					++$files_made;
					
				}
				if ($maxdepth > 0) {
					my ($new_dirs,$new_files) =
						gen_dirs($next_path,$breadth - 1,$maxdepth - 1);
					$dirs_made += $new_dirs;
					$files_made += $new_files;
				}
			}
			else {
				bail(1,"Could not create directory \"$next_path\"");
			}
		}
	}
	return ($dirs_made,$files_made);
}
