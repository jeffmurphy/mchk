#!/utilnsg_solaris/bin/perl -w
#
# insert.pl [.S file]
#
# Copyright (c) 1998, 1999 Nickel City Software


use strict;
use POSIX;

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

# AE 12/10/98 this appears to work somewhat well.

# push %l0 - %l3 and %o0 - %o3 on the stack.

#       add %sp, -32, %sp    \
#       std %l0, [%sp + 0]  |
#       std %l2, [%sp + 8]  |--- "push"
#       std %o0, [%sp + 16] |
#       std %o2, [%sp + 24] /
#       add %sp, 32, %o0
#	mov $len,%o2
#	mov $rhs,%o1 <--- note, this doesn't work on sparc. need to use add
#                         with %g0 to load the address.
#	call chk
#	nop
#       ldd [%sp + 0], %l0  \
#       ldd [%sp + 8], %l2  |
#       ldd [%sp + 16], %o0 |--- "pop"
#       ldd [%sp + 24], %o2 |
#       add %sp, 32, %sp    /




my ($mainflag) = 0;
my ($gccs_stack, $gimmemorestack,
    $stackaddr1, $stackaddr2,
    $stackaddr3, $stackaddr4,
    $midstack, $gcc_alloted_data) = 0;

while(<I>) {
  chomp;
#  print "$_\n";
  # if this is the main routine, flag it so we can stick
  # some exit code in at the end.

  if(/^\s*\.global (.*)\s*$/) {
    print STDERR "gotcha:\n";
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
    print O "\tnop\n";
  }

  # if we are calling exit, lets cleanup first
  if(/^\s*call exit.*$/) {
    print O "\tcall chkexit\n";
    print O "\tnop\n";
  } 

  # TODO: figure out the equivalent sparc instructions
  #
  # ex: write an abs value
  #        stb %o0,[%fp-17]
  # ex: read a mem
  #        ldb [%fp-17],%o0
  
#
# create a hole in the middle of the stack for our registers.
# we only need 32 bytes, but adding 128 bytes gives us a 
# 'safety zone' of 48 bytes on each side of the hole.
# it'd be nice if we could have rchk/wchk detect something
# clobbering stuff inside the hole. 
#


  if (/\s*save %sp,(-\d+),%sp\s*$/) {
    $gccs_stack = $1;
    $gimmemorestack = $gccs_stack - 128;
    $gcc_alloted_data = abs($gimmemorestack) - 64;
    $midstack = ($gcc_alloted_data/2)-16;
    print STDERR "mid:$midstack\n";
    if ($midstack%8 != 0) {
      $midstack = ceil($midstack/8);
      print STDERR "align:$midstack\n";
      $midstack = $midstack*8;
    }
    $stackaddr1 = abs($midstack);
    $stackaddr2 = abs($midstack+8);
    $stackaddr3 = abs($midstack+16);
    $stackaddr4 = abs($midstack+24);
    print O "\tsave %sp,$gimmemorestack,%sp\n";
    print STDERR "modified stack: old:$gccs_stack new:$gimmemorestack mid:$midstack\n";
  }
    
  elsif((/st(.*)\ (.+),(.+)/) || (/ld(.*)\ (.+),(.+)/)) {
    my($t, $lhs, $rhs) = ($1, $2, $3);

    # if the rhs is a memory reference, then this is
    # a write.

    my ($register, $sign, $offset,$fudge) = "";
    $offset = 0;
    if($rhs =~ /\[.*\]/) {
      if ($rhs =~ /\[(.+)([\-\+])(.+)\]/) {
	($register,$sign,$offset) = ($rhs =~ /\[(.+)([\-\+])(.+)\]/);
      } else {
	($register) = ($rhs =~ /\[(.+)\]/);
	$sign = "";
	$offset = "";
      }
      print "write($t) $lhs to $rhs ($_)\n" if $debug;
      print O "\tstd %l0,[%fp-$stackaddr1]\n";
      print O "\tstd %l2,[%fp-$stackaddr2]\n";
      print O "\tstd %o0,[%fp-$stackaddr3]\n";
      print O "\tstd %o2,[%fp-$stackaddr4]\n";
      print O "\tmov %sp,%o0\n";
      print O "\tmov ".so($t).",%o2\n";
      print O "\tmov %g0,%o1\n";
      if ($offset ne "") {
	if ($sign =~ /\-/) {
	  print O "\tadd $register,-$offset,%o1\n";
	} else {
	  print O "\tadd $register,$offset,%o1\n";
	}
      } else {
	if ($register =~ /%o0/) {
	  $fudge = $stackaddr3;
	  print O "\tld [%fp-$fudge],%o1\n";
	} elsif ($register =~ /%o1/) {
	  $fudge = $stackaddr3 + 4;
	  print O "\tld [%fp-$fudge],%o1\n";
	} elsif ($register =~ /%o2/) {
	  $fudge = $stackaddr4;
	  print O "\tld [%fp-$fudge],%o1\n";
	} elsif ($register =~ /%o3/) {
	  $fudge = $stackaddr4 + 4;
	  print O "\tld [%fp-$fudge],%o1\n";
	} elsif ($register =~ /%l0/) {
	  $fudge = $stackaddr1;
	  print O "\tld [%fp-$fudge],%o1\n";
	} elsif ($register =~ /%l1/) {
	  $fudge = $stackaddr1 + 4;
	  print O "\tld [%fp-$fudge],%o1\n";
	} elsif ($register =~ /%l2/) {
	  $fudge = $stackaddr2;
	  print O "\tld [%fp-$fudge],%o1\n";
	} elsif ($register =~ /%l3/) {
	  $fudge = $stackaddr2 + 4;
	  print O "\tld [%fp-$fudge],%o1\n";
	} else {
	  print O "\tmov $register,%o1\n";
	}
      }
      print O "\tcall wchk\n";
      print O "\tnop\n";
      print O "\tldd [%fp-$stackaddr1],%l0\n";
      print O "\tldd [%fp-$stackaddr2],%l2\n";
      print O "\tldd [%fp-$stackaddr3],%o0\n";
      print O "\tldd [%fp-$stackaddr4],%o2\n";
    }

    # if the lhs is a memory reference, then this is 
    # a read

    if($lhs =~ /\[.*\]/) {
      if ($lhs =~ /\[(.+)([\-\+])(.+)\]/) {
	($register,$sign,$offset) = ($lhs =~ /\[(.+)([\-\+])(.+)\]/);
      } else {
	($register) = ($lhs =~ /\[(.+)\]/);
	$sign = "";
	$offset = "";
      }
      print "read($t) $lhs to $rhs ($_)\n" if $debug;
      print O "\tstd %l0,[%fp-$stackaddr1]\n";
      print O "\tstd %l2,[%fp-$stackaddr2]\n";
      print O "\tstd %o0,[%fp-$stackaddr3]\n";
      print O "\tstd %o2,[%fp-$stackaddr4]\n";
      print O "\tmov %sp,%o0\n";
      print O "\tmov ".so($t).",%o2\n";
      print O "\tmov %g0,%o1\n";
      if ($offset ne "") {
	if ($sign =~ /\-/) {
	  print O "\tadd $register,-$offset,%o1\n";
	} else {
	  print O "\tadd $register,$offset,%o1\n";
	}
      } else {
	if ($register =~ /%o0/) {
	  $fudge = $stackaddr3;
	  print O "\tld [%fp-$fudge],%o1\n";
	} elsif ($register =~ /%o1/) {
	  $fudge = $stackaddr3 + 4;
	  print O "\tld [%fp-$fudge],%o1\n";
	} elsif ($register =~ /%o2/) {
	  $fudge = $stackaddr4;
	  print O "\tld [%fp-$fudge],%o1\n";
	} elsif ($register =~ /%o3/) {
	  $fudge = $stackaddr4 + 4;
	  print O "\tld [%fp-$fudge],%o1\n";
	} elsif ($register =~ /%l0/) {
	  $fudge = $stackaddr1;
	  print O "\tld [%fp-$fudge],%o1\n";
	} elsif ($register =~ /%l1/) {
	  $fudge = $stackaddr1 + 4;
	  print O "\tld [%fp-$fudge],%o1\n";
	} elsif ($register =~ /%l2/) {
	  $fudge = $stackaddr2;
	  print O "\tld [%fp-$fudge],%o1\n";
	} elsif ($register =~ /%l3/) {
	  $fudge = $stackaddr2 + 4;
	  print O "\tld [%fp-$fudge],%o1\n";
	} else {
	  print O "\tmov $register,%o1\n";
	}
      }
      print O "\tcall rchk\n";
      print O "\tnop\n";
      print O "\tldd [%fp-$stackaddr1],%l0\n";
      print O "\tldd [%fp-$stackaddr2],%l2\n";
      print O "\tldd [%fp-$stackaddr3],%o0\n";
      print O "\tldd [%fp-$stackaddr4],%o2\n";

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
  return 4 if($t eq "");
  return 8 if($t eq "dd");
  return 1 if($t eq "sb");
  return 1 if($t eq "ub");
  return 2 if($t eq "sh");
  return 2 if($t eq "uh");
  return 8 if($t eq "d");
  return 4 if($t eq "h");
  return 1 if($t eq "b");

  die "unknown type ($t) can't figure out 'sizeof'";
}
