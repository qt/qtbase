#!/usr/bin/perl

# your httpd.conf should have something like this:

# Alias /perl/  /real/path/to/perl-scripts/

# <Location /perl>
# SetHandler  perl-script
# PerlHandler Apache::Registry
# PerlSendHeader On
# Options +ExecCGI
# </Location>

print "Content-type: text/html\n\n";

print "<b>Date: ", scalar localtime, "</b><br>\n";

print "<hr><h1>It worked!</h1>\n";
print "This script runs under: ".$ENV{"GATEWAY_INTERFACE"}."<hr></n";

$ENV{"SERVER_NAME"}="(Hidden for security purposes)";
$ENV{"SERVER_ADMIN"}="(Hidden for security purposes)";
$ENV{"SCRIPT_FILENAME"}="(Hidden for security purposes)";
$ENV{"SERVER_SOFTWARE"}="(Hidden for security purposes)";
$ENV{"SERVER_PORT"}="(Hidden for security purposes)";
$ENV{"SERVER_SIGNATURE"}="Apache-AdvancedExtranetServer (Complete info hidden)";
$ENV{"PATH"}="(Hidden for security purposes)";
$ENV{"SERVER_ADDR"}="(Hidden for security purposes)";
$ENV{"DOCUMENT_ROOT"}="(Hidden for security purposes)";
$ENV{"MOD_PERL"}="(Hidden for security purposes)";


print "%ENV: <br>\n", map { "$_ = $ENV{$_} <br>\n" } keys %ENV;

