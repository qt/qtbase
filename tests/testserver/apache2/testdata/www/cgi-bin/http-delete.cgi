#!/usr/bin/perl

use CGI;

if ($ENV{'REQUEST_METHOD'} eq "DELETE") {
    $queryString = $ENV{'QUERY_STRING'};
    if ($queryString eq "200-ok") {
        $returnCode = 200;
    } elsif ($queryString eq "202-accepted") {
        $returnCode = 202;
    } elsif ($queryString eq "204-no-content") {
        $returnCode = 204;
    } else {
        $returnCode = 404;
    }
} else {
    # 405 = Method Not Allowed
    $returnCode = 405;
}

$q = new CGI;
print $q->header(-status=>$returnCode);
