#!/usr/bin/env perl
# Copyright (C) 2016 Intel Corporation.
# SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

use strict;
my $syntax = "findclasslist.pl\n" .
             "Replaces each \@FILE:filename\@ in stdin with the classes found in that file\n";

$\ = $/;
while (<STDIN>) {
    chomp;
    unless (/\@FILE:(.*)\@/) {
        print;
        next;
    }

    # Replace this line with the class list
    open HDR, "<$1" or die("Could not open header $1: $!");
    my $comment = "    /* $1 */";
    while (my $line = <HDR>) {
        # Match a struct or class declaration, but not a forward declaration
        $line =~ /^(?:struct|class|namespace) (?:Q_.*_EXPORT)? (\w+)(?!;)/ or next;
        print $comment if $comment;
        printf "    *%d%s*;\n", length $1, $1;
        $comment = 0;
    }
    close HDR;
}
