#!/usr/local/bin/perl -w
#
# insert.pl [.S file]
#
# Copyright (c) 1998, 1999 Nickel City Software

use strict;

my($debug) = 0;

print "insert.pl v1.0\n";

my($file) = shift;
open(I, $file) || die "open($file): $!";
my ($tfile) = ${file}."_tfile";
open(O, "> $tfile") || die "open($tfile): $!";

my ($mainflag, $globlmain) = (0, 0);

# Options: R:W:E:S
#   R = perform read checks
#   W = perform write checks
#   E = perform exit checks
#   S = perform startup routine

my (%control);
if(defined($ENV{'MCHKOPTS'})) {
  foreach (split(/:/, $ENV{'MCHKOPTS'})) {
    $control{$_} = 1;
  }
} else {
  %control = ('R' => 1,
	      'W' => 1,
	      'E' => 1,
	      'S' => 1);
}

while(<I>) {
  chomp;

  # if we find ".globl main" it means that this file has
  # a global symbol called "main". good. this will most likely
  # be the executables entry point.

  if(/^\.globl main$/) {
    print "\t\"main\" is global.\n";
    $globlmain = 1;
    print O "$_\n";
  }

  # if we've found a label, analyze it. if it is "main:" then we
  # insert some startup code (provided main is global) else we just
  # print out the label and keep going.

  elsif(/^(\S+):$/) {
    if(($1 eq "main") && ($globlmain == 1)) {
      if($mainflag == 0) {
	$mainflag = 1;
	if(defined($control{'S'})) {
	  print "\tfound global \"main\" .. inserting chksetup() call.\n";
	  print O "$_\n";
	  print O "\n";
	  print O "\tpushl  %ebp\n";
	  print O "\tmovl   %esp, %ebp\n";
	  print O "\tsubl   \$0x14,%esp\n";
	  print O "\tpusha\n";
	  print O "\tmovl   0xc(%ebp),%eax\n";
	  print O "\tpushl  %eax\n"; # ac
	  print O "\tmovl   0x8(%ebp),%eax\n";
	  print O "\tpushl  %eax\n"; # av
	  print O "\tcall   chksetup\n";
	  print O "\taddl   \$8,%esp\n";
	  print O "\tpopa\n";
	  print O "\taddl   \$0x14,%esp\n";
	  print O "\tpopl   %ebp\n";
	} else {
	  print "\tfound global \"main\" but S option not specified.\n";
	}
      } else {
	die "\tWhoops. We've found two 'main:' labels?";
      }
    } else {
      print O "$_\n";
    }
  }

  # if the line is ".size main,..." then this is probably the
  # end of our "main" routine. we'll want to turn off our
  # "mainflag" so we don't incorrectly call "chkexit" when we
  # find "ret" instructions.

  elsif(/^\s*\.size\s+main,.*$/) {
    $mainflag = 0;
    print O "$_\n";
  }

  # if we've found a "ret" instruction and are confident that it
  # is part of our "main" routine then we can expect it to cause
  # the program to exit. therefor, call our cleanup routine first.

  elsif(($mainflag == 1) && (/^\s*ret\s*$/)) {
    print "\tfound 'ret' in main() .. ";
    if(defined($control{'E'})) {
      print "inserting chkexit() call.\n";
      print O "\tpusha\n";
      print O "\tcall chkexit\n";
      print O "\tpopa\n";
    } else {
      print "but E option not specified.\n";
    }
    print O "\t$_\n"; # ret
  }

  # if we are calling exit, lets cleanup first

  elsif(/^\s*call\s+exit\s*$/) {
    print "\tfound 'call exit' .. ";
    if(defined($control{'E'})) {
      print "inserting chkexit() call.\n";
      print O "\tcall chkexit\n";
    } else {
      print "but E option not specified.\n";
    }
    print O "$_\n"; # exit
  } 

  # now check for memory access instructions. 

  # ex: write an abs value
  #        movb $123,-1(%ebp)
  #        movb $1,(%eax)
  # ex: read a mem -> write a mem
  #        movb -1(%ebp),(%eax)
  # ex: read a mem
  #        movb -1(%ebp),%eax

  #        a = 123 -> movb $123,-1(%ebp)
  # 	a write operation.
  #
  #     pushl %eax         ; save %eax
  #     movl %esp, %eax    ; save %esp
  #     pushl %eax         ; save %esp
  # 	pushl $4           ; length of value being read/written
  # 	leal -1(%ebp),%eax ; address being read/written
  # 	pushl %eax         ; push addr
  # 	call Xchk          ; call chk routine X=r|w
  # 	add $12, %esp      ; remove args from stack
  #     popl %eax          ; restore %eax
  
    
  elsif(/mov(.)\ (.+),(.+)/) {
    my($t, $lhs, $rhs) = ($1, $2, $3);

    # if the rhs is a memory reference, then this is
    # a write.

    if($rhs =~ /\(.*\)/) {
      print "\twrite($t) $lhs to $rhs ($_)\n" if $debug;
      if(defined($control{'W'})) {
	print O "\n";
	print O "\tpusha\n";           # save eax
	print O "\tpushl \$".so($t)."\n";   # len
	print O "\tleal $rhs,%eax\n";
	print O "\tpushl %eax\n";           # ptr
	print O "\tmovl %esp, %eax\n";      # record sp
	print O "\tpushl %eax\n";           # push sp
	print O "\tcall wchk\n";            # wchk()
	print O "\tadd \$12, %esp\n";
	print O "\tpopa\n";            # restore eax
      }
    }

    # if the lhs is a memory reference, then this is 
    # a read

    if($lhs =~ /\(.*\)/) {
      print "\tread($t) $lhs to $rhs ($_)\n" if $debug;
      if(defined($control{'R'})) {
	print O "\n";
	print O "\tpusha\n";           # save eax
	print O "\tpushl \$".so($t)."\n";   # len
	print O "\tleal $lhs,%eax\n";
	print O "\tpushl %eax\n";           # ptr
	print O "\tmovl %esp, %eax\n";      # record sp
	print O "\tpushl %eax\n";           # push sp
	print O "\tcall rchk\n";            # rchk()
	print O "\tadd \$12, %esp\n";
	print O "\tpopa\n";
      }
    }

    print O "$_\n\n";
  }

  # finally: we don't know what this is, so just print it
  # out and move on.

  else {
    print O "$_\n";
  }
}

close(O);
close(I);

#rename $file, $file."_orig" || die "rename to _orig: $!";
unlink($file) || die "unlink($file): $!";
rename $tfile, $file || die "rename from temp: $!";

#system("cp $file ${file}-copy");

exit 0;

sub so($) {
  my ($t) = shift;
  return 1 if($t eq "b");
  return 2 if($t eq "w");
  return 4 if($t eq "l");

  die "unknown type ($t) can't figure out 'sizeof'";
}
