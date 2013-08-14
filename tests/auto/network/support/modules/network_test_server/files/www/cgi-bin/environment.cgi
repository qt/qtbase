#!/usr/bin/perl
print "Content-type: text/html\n\n";
foreach $key (keys %ENV) {
	print "$key --> $ENV{$key}<br>";
}
