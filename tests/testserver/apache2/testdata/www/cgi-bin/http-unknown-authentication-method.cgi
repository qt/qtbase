#!/usr/bin/perl

use CGI;

$queryString = $ENV{'QUERY_STRING'};
my $message;
if ($queryString eq "407-proxy-authorization-required") {
    $status = 407;
} else {
    $status = 401;
}

$q = new CGI;
print $q->header(-status=>$status,
                 -type=>"text/plain",
                 -WWW_Authenticate=>'WSSE realm="Test", profile="TestProfile"'),
          "authorization required";
