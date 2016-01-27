#!/usr/bin/env perl
#############################################################################
##
## Copyright (C) 2015 Intel Corporation
## Contact: http://www.qt.io/licensing/
##
## This file is part of the build configuration tools of the Qt Toolkit.
##
## $QT_BEGIN_LICENSE:LGPL21$
## Commercial License Usage
## Licensees holding valid commercial Qt licenses may use this file in
## accordance with the commercial license agreement provided with the
## Software or, alternatively, in accordance with the terms contained in
## a written agreement between you and The Qt Company. For licensing terms
## and conditions see http://www.qt.io/terms-conditions. For further
## information use the contact form at http://www.qt.io/contact-us.
##
## GNU Lesser General Public License Usage
## Alternatively, this file may be used under the terms of the GNU Lesser
## General Public License version 2.1 or version 3 as published by the Free
## Software Foundation and appearing in the file LICENSE.LGPLv21 and
## LICENSE.LGPLv3 included in the packaging of this file. Please review the
## following information to ensure the GNU Lesser General Public License
## requirements will be met: https://www.gnu.org/licenses/lgpl.html and
## http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
##
## As a special exception, The Qt Company gives you certain additional
## rights. These rights are described in The Qt Company LGPL Exception
## version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
##
## $QT_END_LICENSE$
##
#############################################################################

use strict;
my $syntax = "findclasslist.pl [private header list]\n" .
             "Replaces \@CLASSLIST\@ with the classes found in the header files\n";
$\ = $/;
while (<STDIN>) {
    chomp;
    unless (/\@CLASSLIST\@/) {
        print;
        next;
    }

    # Replace @CLASSLIST@ with the class list
    for my $header (@ARGV) {
        open HDR, "<$header" or die("Could not open header $header: $!");
        my $comment = "    /* $header */";
        while (my $line = <HDR>) {
            # Match a struct or class declaration, but not a forward declaration
            $line =~ /^(?:struct|class) (?:Q_.*_EXPORT)? (\w+)(?!;)/ or next;
            print $comment if $comment;
            printf "    *%d%s*;\n", length $1, $1;
            $comment = 0;
        }
        close HDR;
    }
}
