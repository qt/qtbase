#!/usr/bin/perl

if ($ENV{'REQUEST_METHOD'} eq "GET") {
	$request = $ENV{'QUERY_STRING'};
} elsif ($ENV{'REQUEST_METHOD'} eq "POST") {
	read(STDIN, $request,$ENV{'CONTENT_LENGTH'}) || die "Could not get query\n";
} 

print "Content-type: text/plain\n\n";
#print "your input was (via the '", $ENV{'REQUEST_METHOD'}, "') request method:\n";
print $request;


# Perl code fragment to split the HTML FORM input in name value
# pairs.  The input is in the form name1=value1&name2=value2&...

# Split the name-value pairs

#@pairs = split(/&/, $input);
#foreach $pair (@pairs)
#{
#($name, $value) = split(/=/, $pair);
#$FORM{$name} = $value;
#}
