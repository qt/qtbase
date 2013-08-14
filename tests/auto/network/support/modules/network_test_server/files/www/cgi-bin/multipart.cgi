#!/usr/bin/perl

use CGI;
use Digest::MD5 qw(md5_hex);

$q = new CGI;
print $q->header();

%params = $q->Vars;

$contentType = $ENV{"CONTENT_TYPE"};
print "content type: $contentType\n";

if ($contentType =~ /^multipart\/form-data/) {
    while(($key, $value) = each(%params)) {
        if ($key =~ /text/) {
            $retValue = $value;
        } else {
            $retValue = md5_hex($value);
        }
        print "key: $key, value: $retValue\n";
    }
} else {
    #$contentLength = $ENV{"CONTENT_LENGTH"};
    #print "content length: $contentLength\r\n";

    $data = $q->param('POSTDATA');
    $data =~ s/--\S*--$//; # remove ending boundary
    @parts = split(/--\S*\r\n/, $data);
    shift(@parts);
    foreach (@parts) {
            #print "raw: $_";
        ($header, $content) = split("\r\n\r\n");
        @headerFields = split("\r\n", $header);
        foreach (@headerFields) {
            ($fieldName, $value) = split(": ");
            print "header: $fieldName, value: '$value'\n";
        }
        $content =~ s/\r\n//;
        print "content: $content\n\n";
    }
}
