#!/usr/bin/perl

use strict;
package SunifdefLib;
use Exporter;

our @ISA=qw(Exporter);
our @EXPORT=qw(set_prog set_usage get_usage set_verbosity help usage_error progress info warn error fatal bail cyg2win cygwin windows);

use constant PROGRESS => 0;
use constant INFO => 1,;
use constant WARNING => 2;
use constant ERROR => 3;
use constant FATAL => 4;

my $OS = "$^O"; 
my $prog = "<Unknown Prog>";
my $usage = "No usage help available";
my $verbose_level = PROGRESS;
my %severities_by_word = (
	'progress' => PROGRESS,
	'info' => INFO,
	'warning' => WARNING,
	'error' => ERROR,
	'fatal' => FATAL
);

my %severities_by_num = (
	'0' => 'progress',
	'1' => 'info',
	'2' => 'warning',
	'3' => 'error',
	'4' => 'fatal'
);

sub set_prog($);
sub set_usage($);
sub set_verbosity($);
sub help();
sub usage_error(@);
sub progress(@);
sub info(@);
sub warn(@);
sub error(@);
sub fatal(@);
sub bail(@);
sub report(@);
sub cyg2win($);
sub cygwin();
sub windows();

sub set_prog($)
{
	$prog = $_[0];
}

sub set_usage($)
{
	$usage = $_[0];
}

sub set_verbosity($)
{
	my $verbosity = $_[0];
	$verbose_level = $severities_by_word{$verbosity} if defined($severities_by_word{$verbosity});
}

sub report(@) {
	my $severity = shift;
	my $msg = shift;
	if ($severity >= $verbose_level) {
		my $severity_word = $severities_by_num{"$severity"};
		if ($severity < WARNING) {
			printf STDOUT "$prog: $severity_word: $msg\n",@_
		}
		else {
			printf STDERR "$prog: $severity_word: $msg\n",@_;
		}
	}
}

sub progress(@)
{
	my $msg = shift;
	report(PROGRESS,$msg,@_);
}

sub info(@)
{
	my $msg = shift;
	report(INFO,$msg,@_);
}

sub warn(@)
{
	my $msg = shift;
	report(WARNING,$msg,@_);
}

sub error(@)
{
	my $msg = shift;
	report(ERROR,$msg,@_);
}

sub fatal(@)
{
	my $msg = shift;
	report(FATAL,$msg,@_);
}

sub bail(@)
{
	my $exitcode = shift;
	fatal(@_);
	exit($exitcode);
}

sub usage_error(@)
{
	my ($msg) = @_;
	if (defined($msg)) {	
		print STDERR "$prog: usage error: $msg\n";
	}
	else {
		print STDERR "$prog: usage error: \n$usage\n";
	}
	exit(1);
}

sub help()
{
	print STDOUT $usage;
}

sub cyg2win($)
{
    my $filename = $_[0];
    $filename =~ s#/cygdrive/(\w)/#$1:/#g;
    return $filename;
}

sub cygwin()
{
    return "$OS" eq "cygwin";
}

sub windows()
{
    return "$OS" =~ m/"MSWin"/;
}


1;
