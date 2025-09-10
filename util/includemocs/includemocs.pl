#!/usr/bin/perl
# Copyright (C) 2017 Intel Corporation.
# SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0
use strict;

MAKEFILE: while ($_ = shift @ARGV) {
    chdir($ENV{PWD});
    open MAKEFILE, "<", $_
        or die("Could not open Makefile");
    print "includemocs.pl: Processing $_\n";

    my $srcdir;
    my $sourcesline;

    # Find "SOURCES =" line
    while (<MAKEFILE>) {
        $srcdir = $1 if m,^# Project:\s+(.*)/[^/]+.pro,;
        if (/^# Template:\s+(\w+)/) {
            next MAKEFILE if $1 eq "subdirs";
        }
        if (/^SOURCES\s*=\s*(.*)/) {
            $sourcesline = $1;
            last;
        }
    }
    if ($sourcesline =~ s/\s+\\//) {
        # continuation
        while (<MAKEFILE>) {
            chomp;
            /^\s*([^ ]+)/;
            $sourcesline .= " $1";
            last unless m/\\$/;
        }
    }
    close MAKEFILE;

    # Now parse the sources
    my @mocs;
    my @sources;
    for (split(/ /, $sourcesline)) {
        if (/\.moc\/(moc_.*\.cpp)/) {
            push @mocs, $1;
        } elsif (/^\.(rcc|uic)/) {
            # ignore
        } else {
            push @sources, $_;
        }
    }

    chdir($srcdir) or die("Where's $srcdir? $!");
    for my $moc (@mocs) {
        my $include = "#include \"$moc\"\n";

        # Find a corresponding .cpp file to host the new #include
        my $basename = ($moc =~ s/^moc_//r);
        $basename =~ s/\.[^.]+//;
        my @candidates = grep { m,\Q/$basename.\E, } @sources;

        if (scalar @candidates == 0) {
            # Try without a _p suffix
            $basename =~ s/_p$//;
            @candidates = grep { m,\Q/$basename.\E, } @sources;
        }
        if (scalar @candidates == 0) {
            print STDERR "includemocs.pl: Cannot find .cpp file for $moc\n";
            next;
        }

        my $cpp = $candidates[0];
        undef @candidates;

        #print "$moc -> $cpp\n";
        open CPP, "<", $cpp
            or die("Cannot open source $cpp: $!");

        my @lines;
        while (<CPP>) {
            push @lines, $_;
            next unless defined($include);

            # Print the new include next to a pre-existing moc include
            if (/#include \"moc_/ || /#include ".*\.moc"/) {
                push @lines, $include;
                undef $include;
            }
        }
        close CPP;

        if (defined($include)) {
            # Try to insert the new #include between QT_END_NAMESPACE and any #endif lines
            my $n = 0;
            my $extrablank = "";
            while (defined($include)) {
                --$n;
                $_ = $lines[$n];
                if (/^#endif/) {
                    $extrablank = "\n";
                    next;
                }

                $_ .= "\n" unless /^$/;
                splice @lines, $n, 1, ($_, $include, $extrablank);
                undef $include;
            }
        }

        # Write the file again
        open CPP, ">", $cpp
            or die("Cannot open source $cpp for writing: $!");
        map { print CPP $_; } @lines;
        close CPP;
    }
}
