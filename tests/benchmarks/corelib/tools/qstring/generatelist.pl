#!/usr/bin/perl
## Copyright (C) 2013 Digia Plc and/or its subsidiary(-ies).
## Contact: http://www.qt-project.org/legal
##
## This file is part of the QtCore module of the Qt Toolkit.
##
## $QT_BEGIN_LICENSE:LGPL$
## Commercial License Usage
## Licensees holding valid commercial Qt licenses may use this file in
## accordance with the commercial license agreement provided with the
## Software or, alternatively, in accordance with the terms contained in
## a written agreement between you and Digia.  For licensing terms and
## conditions see http://qt.digia.com/licensing.  For further information
## use the contact form at http://qt.digia.com/contact-us.
##
## GNU Lesser General Public License Usage
## Alternatively, this file may be used under the terms of the GNU Lesser
## General Public License version 2.1 as published by the Free Software
## Foundation and appearing in the file LICENSE.LGPL included in the
## packaging of this file.  Please review the following information to
## ensure the GNU Lesser General Public License version 2.1 requirements
## will be met: http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
##
## In addition, as a special exception, Digia gives you certain additional
## rights.  These rights are described in the Digia Qt LGPL Exception
## version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
##
## GNU General Public License Usage
## Alternatively, this file may be used under the terms of the GNU
## General Public License version 3.0 as published by the Free Software
## Foundation and appearing in the file LICENSE.GPL included in the
## packaging of this file.  Please review the following information to
## ensure the GNU General Public License version 3.0 requirements will be
## met: http://www.gnu.org/copyleft/gpl.html.
##
##
## $QT_END_LICENSE$
#
# Parses a file (passed as argument) that contains a dump of pairs of
# strings and generates C source code including said data.
#
# The format of the file is:
#   LEN = <len> <keyword> <align1> <align2>\n<data1><data2>\n
# where:
#   LEN		the literal string "LEN"
#   <len>	the length of the data, in 16-bit words
#   <keyword>	the literal string "SAME" or "DIFF"
#   <align1>    the alignment or pointer value of the first data
#   <align2>    the alignment or pointer value of the second data
#   <data1>     the first data
#   <data2>     the second data
#   \n          newline
#
# The code to write this data would be:
#   fprintf(out, "LEN = %d %s %d %d\n", len,
#          (p1 == p2) ? "SAME" : "DIFF",
#          uint(quintptr(p1)) & 0xfff, uint(quintptr(p2)) & 0xfff);
#   fwrite(p1, 2, len, out);
#   fwrite(p2, 2, len, out);
#   fwrite("\n", 1, 1, out);

sub printUshortArray($$$) {
    $str = $_[0];
    $align = $_[1] & 0x1f;
    $offset = $_[2];

    die if ($align & 1) != 0;
    $align /= 2;

    $len = (length $str) / 2;
    $headpadding = $align & 0x7;
    $tailpadding = 8 - (($len + $headpadding) & 0x7);
    $multiplecachelines = ($align + $len) > 0x20;

    if ($multiplecachelines) {
	# if this string crosses into a new cacheline, then
	# replicate the result
	$headpadding |= ($offset & ~0x1f);
	$headpadding += 0x20
	    if ($headpadding < $offset);
	$headpadding -= $offset;
	++$cachelinecrosses;
    }
    for $i (1..$headpadding) {
	print 65536-$i,",";
    }
    print "\n        " if ($headpadding > 0);
    print "    " if ($headpadding == 0);

    for ($i = 0; $i < $len * 2; $i += 2) {
	print " ", ord(substr($str, $i, 1)) +
	    ord(substr($str, $i + 1, 1)) * 256,
	    ",";
    }
    print "\n    " if ($tailpadding > 0);

    for $i (1..$tailpadding) {
	print 65536-$i, ",";
    }
    print " // ", $offset + $headpadding + $len + $tailpadding;
    print "+" if $multiplecachelines;

    return ($offset + $headpadding, $offset + $headpadding + $len + $tailpadding);
}

print "// This is a generated file - DO NOT EDIT\n\n";

print "#include \"data.h\"\n\n";

print "const ushort stringCollectionData[] __attribute__((aligned(64))) = {\n";
$count = 0;
$offset = 0;
$totalsize = 0;
$maxlen = 0;
$cachelinecrosses = 0;

open IN, "<" . $ARGV[0];
while (1) {
    $line = readline(*IN);
    last unless defined($line);
    $line =~ /LEN = (\d+) (\w+) (\d+) (\d+)/;
    $len = $1;
    $data[$count]->{len} = $len;
    $sameptr = $2;
    $data[$count]->{align1} = $3 - 0;
    $data[$count]->{align2} = $4 - 0;

    # statistics
    $alignhistogram{$3 & 0xf}++;
    $alignhistogram{$4 & 0xf}++;
    $samealignments{$3 & 0xf}++ if ($3 & 0xf) == ($4 & 0xf);

    read IN, $a, $len * 2;
    read IN, $b, $len * 2;

    <IN>;			# Eat the newline

    if ($len == 0) {
	$data[$count]->{offset1} = $offset;
	$data[$count]->{offset2} = $data[$count]->{offset1};
	++$data[$count]->{offset2} if ($sameptr eq "DIFF");
    } else {
	print "    // #$count\n";
	print "    ";
	($data[$count]->{offset1}, $offset) =
	    printUshortArray($a, $data[$count]->{align1}, $offset);
	print "\n    ";
	die if ($offset & 0x7) != 0;

	if ($sameptr eq "DIFF") {
	    ($data[$count]->{offset2}, $offset) =
		printUshortArray($b, $data[$count]->{align2}, $offset);
	    print "\n\n";
	} else {
	    $data[$count]->{offset2} = $data[$count]->{offset1};
	    print "\n\n";
	}
    }
    ++$count;

    $totalsize += $len;
    $maxlen = $len if $len > $maxlen;
}
print "};\n";
close IN;

print "const struct StringCollection stringCollection[] = {\n";
for $i (0..$count-1) {
    print "    {",
        $data[$i]->{len}, ", ",
        $data[$i]->{offset1}, ", ",
        $data[$i]->{offset2}, ", ",
        $data[$i]->{align1}, ", ",
        $data[$i]->{align2},
        "},     // #$i\n";
    next if $data[$i]->{len} == 0;
    die if (($data[$i]->{offset1} & 0x7) != ($data[$i]->{align1} & 0xf)/2);
    die if (($data[$i]->{offset2} & 0x7) != ($data[$i]->{align2} & 0xf)/2);
}
print "};\n";

print "const int stringCollectionCount = $count;\n";
print "const int stringCollectionMaxLen = $maxlen;\n";
printf "// average comparison length: %.4f\n", ($totalsize * 1.0 / $count);
printf "// cache-line crosses: %d (%.1f%%)\n",
    $cachelinecrosses, ($cachelinecrosses * 100.0 / $count / 2);

print "// alignment histogram:\n";
for $key (sort { $a <=> $b } keys(%alignhistogram)) {
    $value = $alignhistogram{$key};
    $samealigned = $samealignments{$key};
    printf "//   0xXXX%x = %d (%.1f%%) strings, %d (%.1f%%) of which same-aligned\n",
	$key, $value, $value * 100.0 / ($count*2),
        $samealigned, $samealigned * 100.0 / $value;
    $samealignedtotal += $samealigned;
}
printf "//   total  = %d (100%) strings, %d (%.1f%%) of which same-aligned\n",
    $count * 2, $samealignedtotal, $samealignedtotal * 100 / $count / 2;
