#!/usr/local/bin/perl -w
#
# "as" wrapper
#
# Copyright (c) 1998, 1999 Nickel City Software

use strict;

my($infile) = $ARGV[$#ARGV];
my($pd);

if($0 =~ /(.*)\/[^\/]+$/) {
  $pd = $1;
} else {
  $pd = ".";
}

my ($p);
chomp($p = `uname -p`);

my ($cmd) = "$pd/insert_${p}.pl";

print '"as" wrapper v1.0'." using $cmd\n";

die "no file given\n" if(!defined($infile));

#print "\n\n./insert.pl $infile\n";
#print "/usr/bin/as @ARGV\n\n";

if(defined($ENV{"MCHK"}) && ($ENV{"MCHK"} eq "yes")) {
  system ("$cmd $infile");
}

my($s);
chomp($s = `uname -s`);
my($as);
$as = "/usr/ccs/bin/as" if($s eq "SunOS");
$as = "/usr/bin/as" if ($s eq "Linux");

#print "**** $s AS = $as\n";

system ("$as @ARGV");

exit 0;
