#!/usr/bin/env perl

# call getpwnam() or getgrnam() to check if user or group (given as arg) exists
#   on the system (getpwnam() and getgrnam() use nsswitch.conf, so it works
#   even if NYS (or similar) is used)

sub usage {
    return "usage:\n  has_usrgrp.pl [-user user|-group group] [-printuid|-printgid]\n";
}

if ( @ARGV < 2 || @ARGV > 3) {
    die usage();
}

if ( $ARGV[0] eq "-user" ) {
    ($name, $passwd, $uid, $gid) = getpwnam($ARGV[1]);
}
elsif ( $ARGV[0] eq "-group" ) {
    ($name, $passwd, $gid) = getgrnam($ARGV[1]);
}
else {
    die usage();
}

if ( ! $name) {
    exit 1;
}

if ( @ARGV == 3 ) {
    if ( $ARGV[2] eq "-printgid" ) {
	print $gid, "\n";
    }
    elsif ( $ARGV[2] eq "-printuid" ) {
	if ( defined $uid ) {
	    print $uid, "\n";
	} else {
	    die usage();
	}
    }
}
exit 0;
