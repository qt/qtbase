#!/usr/bin/env perl
#############################################################################
##
## Copyright (C) 2011 Nokia Corporation and/or its subsidiary(-ies).
## All rights reserved.
## Contact: Nokia Corporation (qt-info@nokia.com)
##
## This file is part of the porting tools of the Qt Toolkit.
##
## $QT_BEGIN_LICENSE:LGPL$
## GNU Lesser General Public License Usage
## This file may be used under the terms of the GNU Lesser General Public
## License version 2.1 as published by the Free Software Foundation and
## appearing in the file LICENSE.LGPL included in the packaging of this
## file. Please review the following information to ensure the GNU Lesser
## General Public License version 2.1 requirements will be met:
## http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
##
## In addition, as a special exception, Nokia gives you certain additional
## rights. These rights are described in the Nokia Qt LGPL Exception
## version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
##
## GNU General Public License Usage
## Alternatively, this file may be used under the terms of the GNU General
## Public License version 3.0 as published by the Free Software Foundation
## and appearing in the file LICENSE.GPL included in the packaging of this
## file. Please review the following information to ensure the GNU General
## Public License version 3.0 requirements will be met:
## http://www.gnu.org/copyleft/gpl.html.
##
## Other Usage
## Alternatively, this file may be used in accordance with the terms and
## conditions contained in a signed written agreement between you and Nokia.
##
##
##
##
##
## $QT_END_LICENSE$
##
#############################################################################


use Cwd;
use File::Find;
use File::Spec;
use strict;
use warnings;

my $dry_run = 0;
my $fixedFileCount = 0;

sub show_help
{
    print "This script replaces all Qt 4 style includes with Qt 5 includes\n";
    print "\n";
    print "Usage: $0 [--dry-run]\n";
    print "\n";
    print "   --dry-run : Do not replace anything, just print what would be replaced\n";
    print "\n";
}

if ($#ARGV >= 0) {
    if ($#ARGV >= 1) {
        die "$0: Takes only one or zero arguments\n";
    } elsif ($ARGV[0] eq "--dry-run") {
        $dry_run = 1;
    } elsif ($ARGV[0] eq "--help") {
        show_help();
        exit 0
    } else {
        show_help();
        print "Unknown argument: $ARGV[0]\n";
        exit 1;
    }
}

my %headerSubst = ();
my $qtdir;
my $cwd = getcwd();

sub fixHeaders
{
    my $fileName = $File::Find::name;
    my $relFileName = File::Spec->abs2rel($fileName, $cwd);

    # only check sources, also ignore symbolic links and directories
    if ($fileName !~ /(\.h|\.cpp|\/C|\.cc|\.CC)$/ || ! -f $fileName) {
        return;
    }

    open IN, "<", $fileName || die "Unable to open \"$fileName\": $!\n";

    my $found = 0;

    # First, we check whether we have a match
    while (<IN>) {
        my $line = $_;

        if ($line =~ /^#\s*include\s*<(.*?\/(.*?))>/) {
            my $newHeader = $headerSubst{$2};
            if ($newHeader && $1 ne $newHeader) {
                if ($dry_run) {
                    print "$relFileName: <$1> => <$newHeader>\n";
                } else {
                    $found = 1;
                    last;
                }
            }
        } elsif ($line =~ /^#\s*include\s*<QtGui>/) {
            if ($dry_run) {
                print "$relFileName: <QtGui> => <QtWidgets>\n";
            } else {
                $found = 1;
                last;
            }
        }
    }

    if ($dry_run || !$found) {
        return;
    }

    # rewind to top
    seek(IN, 0, 0) || die "Unable to seek in $fileName: $!\n";

    open OUT, ">", "$fileName.new" || die "Unable to open \"$fileName.new\": $!\n";

    while (<IN>) {
        my $line = $_;
        if ($line =~ /^#(\s*)include(\s*)<.*?\/(.*?)>(.*)/) {
            my $newHeader = $headerSubst{$3};
            if ($newHeader) {
                $line = "#$1include$2<$newHeader>$4\n";
            }
        } elsif ($line =~ /^#(\s*)include(\s*)<QtGui>(.*)/) {
            $line = "#$1include$2<QtWidgets>$3\n";
        }

        print OUT $line;
    }

    close OUT;

    close IN;

    rename "$fileName.new", $fileName || die "Unable to move $fileName.new to $fileName: $!\n";

    $fixedFileCount += 1;
}

sub findQtHeaders
{
    my ($dirName,$baseDir) = @_;

    local (*DIR);

    opendir(DIR, "$baseDir/include/$dirName") || die "Unable to open \"$baseDir/include/$dirName\": $!\n";
    my @headers = readdir(DIR);
    closedir(DIR);

    foreach my $header (@headers) {
        if (-d "$baseDir/include/$dirName/$header" || $header =~ /\.pri$/) {
            next;
        }
        $headerSubst{$header} = "$dirName/$header";
    }
}

$qtdir = $ENV{'QTDIR'};

if (!$qtdir) {
    die "This script requires the QTDIR environment variable pointing to Qt 5\n";
}

findQtHeaders("QtWidgets", $qtdir);
findQtHeaders("QtPrintSupport", $qtdir);

if (-d "$qtdir/include/QtQuick1") {
    findQtHeaders("QtQuick1", $qtdir);
} elsif (-d "$qtdir/../qtdeclarative" ) {
    # This is the case if QTDIR points to a source tree instead of an installed Qt
    findQtHeaders("QtQuick1", "$qtdir/../qtdeclarative");
} else {
    print "Warning - cannot find QtQuick1 headers\n";
}

# special case
$headerSubst{"QtGui"} = "QtWidgets/QtWidgets";

find({ wanted => \&fixHeaders, no_chdir => 1}, $cwd);

if ($dry_run == 0) {
    print "Done. Modified $fixedFileCount file(s).\n";
}
