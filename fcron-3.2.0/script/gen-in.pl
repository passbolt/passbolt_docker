#!/usr/bin/env perl

# - read define assignations in config.h
# - read variable assignations in Makefile
# - construct a hash with that reads
# - read file given in arg 1 and perform substitutions
#   of the name of a variable by its value
#
# ex :  "@@VERSION@"   will be substitute by    "0.8.4"
#
#  + substitute "@@Date@" by the current date
#
# USAGE: gen-in.pl 

%map = ();



open(CONFIG, "$ARGV[2]/config.h") or print "error while opening config.h: $!\n" and exit;
while ( <CONFIG> ) {
    if ( /^\#define\s+(\w+?)\s+([\w\/-]+?)\s/ ) {
	$map{$1} = $2;
    }
    if ( /^\#define\s+(\w+?)\s+["](.+?)["]\s/ ) {	
	$map{$1} = $2;
    }
    
}
close(CONFIG);

open(MAKEFILE, "$ARGV[2]/Makefile") or print "error while opening Makefile: $!\n" and exit;
while ( <MAKEFILE> ) {
    if ( /^\s*?(\w+?)\s*?=\s*?([^\s]+)\s/ ) {
	$name = $1;
	$value = $2;

	# replace all occurrences of Makefile variables of the form ${VAR} by their values
	$value =~ s/\$\{([^\}]+)\}/$map{$1}/g;
	
	$map{$name} = $value;
    }
    if ( /^\#define\s+(\w+?)\s+["](.+)["]\s/ ) {
	$map{$1} = $2;
    }
}
close(MAKEFILE);

chop ($map{Date} = `date +%m/%d/%Y`);

open(SRC, $ARGV[0]) or print "error while opening $ARGV[0]: $!\n" and exit;
open(DEST, ">$ARGV[1]") or print "error while opening $ARGV[1]: $!\n" and exit;

while ( <SRC> ) {
    s/@@([^@]*)@/$map{$1}/g;
    print DEST $_;

}

close(SRC);
close(DEST);
