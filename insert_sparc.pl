#!/utiloss/perl/bin/perl -w
#
# insert.pl [.S file]
#
# Copyright (c) 1998, 1999 Nickel City Software


use strict;

my($debug) = 1;

my($file) = shift;
open(I, $file) || die "open($file): $!";
my ($tfile) = ${file}."_tfile";
open(O, "> $tfile") || die "open($tfile): $!";

#       a = 123 -> stb %o0,[%fp-17]
#
# 	a write operation.
#

# here's what a sparc function call looks like. 
#
# !   40    foo(4, 8, 12);
#         mov     4,%l0
#         mov     8,%l1
#         mov     12,%l2
#         mov     %l0,%o0
#         mov     %l1,%o1
#         mov     %l2,%o2
#         call    foo
#         nop

# we'll be using %l0 - %l2 and %o0 - %o2, save them
# on the stack

# this is the basic framework. HOWEVER, we really need to save
# the reg's on the stack. but i'm not certain how to do this 
# cleanly.

#       mov len,%l0
#       mov [%fp-17], %l1
#       mov %sp, %l2
#       mov %l0, %o0
#       mov %l1, %o1
#       mov %l2, %o2
#       call chk
#       nop

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

  # TODO: figure out the equivalent sparc instructions
  #
  # ex: write an abs value
  #        movb $123,-1(%ebp)
  #        movb $1,(%eax)
  # ex: read a mem -> write a mem
  #        movb -1(%ebp),(%eax)
  # ex: read a mem
  #        movb -1(%ebp),%eax
    
  elsif(/mov(.)\ (.+),(.+)/) {
    my($t, $lhs, $rhs) = ($1, $2, $3);

    # if the rhs is a memory reference, then this is
    # a write.

    if($rhs =~ /\(.*\)/) {
      print "write($t) $lhs to $rhs ($_)\n" if $debug;
      print O "\n\tpushl %eax\n";
      print O "\tpushl \$1\n";
      print O "\tpushl \$".so($t)."\n";
      print O "\tleal $rhs,%eax\n";
      print O "\tpushl %eax\n";
      print O "\tcall chk\n";
      print O "\tadd \$12, %esp\n";
      print O "\tpopl %eax\n";
    }

    # if the lhs is a memory reference, then this is 
    # a read

    if($lhs =~ /\(.*\)/) {
      print "read($t) $lhs to $rhs ($_)\n" if $debug;
      print O "\n\tpushl %eax\n";
      print O "\tpushl \$2\n";
      print O "\tpushl \$".so($t)."\n";
      print O "\tleal $lhs,%eax\n";
      print O "\tpushl %eax\n";
      print O "\tcall chk\n";
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
