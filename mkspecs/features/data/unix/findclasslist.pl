#!/usr/bin/env perl
# Copyright (C) 2016 Intel Corporation.
# SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

use strict;
my $syntax = "findclasslist.pl\n" .
             "Replaces each \@FILE:filename\@ in stdin with the classes found in that file\n";

# Match a struct or class declaration at the top-level, but not a forward
# declaration
my $classmatch = qr/^(?:struct|class)(?:\s+Q_\w*_EXPORT)?\s+([\w:]+)(\s*;$)?/;

# Match an exported namespace
my $nsmatch = qr/^namespace\s+Q_\w+_EXPORT\s+([\w:]+)/;

$\ = $/;
while (<STDIN>) {
    chomp;
    unless (/\@FILE:(.*)\@/) {
        print;
        next;
    }

    # Replace this line with the class list
    my $fname = $1;
    open HDR, "<$1" or die("Could not open header $1: $!");
    while (my $line = <HDR>) {
        if ($line =~ /\bELFVERSION:(\S+)\b/) {
            last if $1 eq "stop";
            <HDR> if $1 eq "ignore-next"; # load next line
            next if $1 eq "ignore" or $1 eq "ignore-next";
        }

        $line =~ s,\s*(//.*)?$,,;  # remove // comments and trailing space
        next unless $line =~ $nsmatch or $line =~ $classmatch;
        next if $2 ne "";       # forward declaration

        # split the namespace-qualified or nested class identifiers
        my $sym = ($1 =~ s/:$//r); # remove trailing :
        my @sym = split /::/, $sym;
        @sym = map { sprintf "%d%s", length $_, $_; } @sym;
        $sym = sprintf "    *%s*;", join("", @sym);
        printf "%-55s # %s:%d\n", $sym, $fname, $.;
    }
    close HDR;
}
