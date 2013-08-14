#!/usr/bin/perl

if ($ENV{'REQUEST_METHOD'} eq "GET") {
	$request = $ENV{'QUERY_STRING'};
} elsif ($ENV{'REQUEST_METHOD'} eq "POST") {
	read(STDIN, $request,$ENV{'CONTENT_LENGTH'}) || die "Could not get query\n";
} 

print "Content-type: text/plain\n\n";
print $request;

