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

my ($mainflag) = 0;

while(<I>) {
  chomp;

  # if this is the main routine, flag it so we can stick
  # some exit code in at the end.

  if(/^\.globl (.*)$/) {
    if($1 eq "main") {
      $mainflag = 1;
    } else {
      $mainflag = 0;
    }
  }

  # if this is the end of the main routine, call our 
  # cleanup routine

  if(($mainflag == 1) && (/^\s*ret\s*$/)) {
    print O "\tcall chkexit\n";
  }

  # if we are calling exit, lets cleanup first
  if(/^\s*call\s+exit\s*$/) {
    print O "\tcall chkexit\n";
    print O "$_\n";
  } 

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
      print "write($t) $lhs to $rhs ($_)\n" if $debug;
      print O "\n";
      print O "\tpushl %eax\n";           # save eax
      print O "\tpushl \$".so($t)."\n";   # len
      print O "\tleal $rhs,%eax\n";
      print O "\tpushl %eax\n";           # ptr
      print O "\tmovl %esp, %eax\n";      # record sp
      print O "\tpushl %eax\n";           # push sp
      print O "\tcall wchk\n";            # wchk()
      print O "\tadd \$12, %esp\n";
      print O "\tpopl %eax\n";            # restore eax
    }

    # if the lhs is a memory reference, then this is 
    # a read

    if($lhs =~ /\(.*\)/) {
      print "read($t) $lhs to $rhs ($_)\n" if $debug;
      print O "\n";
      print O "\tpushl %eax\n";           # save eax
      print O "\tpushl \$".so($t)."\n";   # len
      print O "\tleal $lhs,%eax\n";
      print O "\tpushl %eax\n";           # ptr
      print O "\tmovl %esp, %eax\n";      # record sp
      print O "\tpushl %eax\n";           # push sp
      print O "\tcall rchk\n";            # rchk()
      print O "\tadd \$12, %esp\n";
      print O "\tpopl %eax\n";
    }

    print O "$_\n\n";
  } 
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
