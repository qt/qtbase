#!/usr/bin/perl
#############################################################################
##
## Copyright (C) 2013 Intel Corporation.
## Contact: http://www.qt.io/licensing/
##
## This file is the build configuration utility of the Qt Toolkit.
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
use constant FAT_MAGIC => 0xcafebabe;
use constant MH_MAGIC => 0xfeedface;
use constant MH_MAGIC_64 => 0xfeedfacf;
use constant CPU_TYPE_X86 => 7;
use constant CPU_TYPE_X86_64 => CPU_TYPE_X86 | 0x01000000;
use constant CPU_SUBTYPE_I386_ALL => 3;
use constant MH_DYLIB => 6;
use constant LC_SEGMENT => 1;
use constant LC_SEGMENT_64 => 0x19;

my $good = pack("(L7 L2 Z16 L8 Z16 Z16 L9 . L)>",
    MH_MAGIC, CPU_TYPE_X86, CPU_SUBTYPE_I386_ALL, MH_DYLIB, # 0-3
    1, # 4: ncmds
    4 * (37 - 6), # 5: sizeofcmds
    0, # 6: flags

    LC_SEGMENT, # 7: cmd
    4 * (37 - 6), # 8: cmdsize
    '__TEXT', # 9-12: segname
    0, # 13: vmaddr
    0x1000, # 14: vmsize
    0, # 15: fileoff
    0x204, # 16: filesize
    7, # 17: maxprot (rwx)
    5, # 18: initprot (r-x)
    1, # 19: nsects
    0, # 20: flags

    'qtmetadata', # 21-24: sectname
    '__TEXT', # 25-28: segname
    0x200, # 29: addr
    4, # 30: size
    0x200, # 31: offset
    2, # 32: align (2^2)
    0, # 33: reloff
    0, # 34: nreloc
    0, # 35: flags
    0, # 36: reserved1
    0, # 37: reserved2

    0x200,
    0xc0ffee # data
);

my $good64 = pack("(L8 L2 Z16 Q4 L4 Z16 Z16 Q2 L8 . Q)>",
    MH_MAGIC_64, CPU_TYPE_X86_64, CPU_SUBTYPE_I386_ALL, MH_DYLIB, # 0-3
    1, # 4: ncmds
    4 * (45 - 7), # 5: sizeofcmds
    0, # 6: flags
    0, # 7: reserved

    LC_SEGMENT_64, # 8: cmd
    4 * (45 - 7), # 9: cmdsize
    '__TEXT', # 10-13: segname
    0, # 14-15: vmaddr
    0x1000, # 16-17: vmsize
    0, # 18-19: fileoff
    0x208, # 20-21: filesize
    7, # 22: maxprot (rwx)
    5, # 23: initprot (r-x)
    1, # 24: nsects
    0, # 25: flags

    'qtmetadata', # 26-29: sectname
    '__TEXT', # 30-33: segname
    0x200, # 34-35: addr
    4, # 36-37: size
    0x200, # 38: offset
    3, # 39: align (2^3)
    0, # 40: reloff
    0, # 41: nreloc
    0, # 42: flags
    0, # 43: reserved1
    0, # 44: reserved2
    0, # 45: reserved3

    0x200,
    0xc0ffeec0ffee # data
);

my $fat = pack("L>*",
    FAT_MAGIC, # 1: magic
    2, # 2: nfat_arch

    CPU_TYPE_X86, # 3: cputype
    CPU_SUBTYPE_I386_ALL, # 4: cpusubtype
    0x1000, # 5: offset
    0x1000, # 6: size
    12, # 7: align (2^12)

    CPU_TYPE_X86_64, # 8: cputype
    CPU_SUBTYPE_I386_ALL, # 9: cpusubtype
    0x2000, # 10: offset
    0x1000, # 11: size
    12, # 12: align (2^12)
);

my $buffer;

our $badcount = 1;
sub generate($) {
    open OUT, ">", "bad$badcount.dylib" or die("Could not open file bad$badcount.dylib: $!\n");
    binmode OUT;
    print OUT $_[0];
    close OUT;
    ++$badcount;
}

# Bad file 1-2
# Except that the cmdsize fields are null
$buffer = $good;
vec($buffer, 5, 32) = 0;
generate $buffer;

$buffer = $good;
vec($buffer, 8, 32) = 0;
generate $buffer;

# Bad file 3-4: same as above but 64-bit
$buffer = $good64;
vec($buffer, 5, 32) = 0;
generate $buffer;

$buffer = $good64;
vec($buffer, 9, 32) = 0;
generate $buffer;

# Bad file 5-8: same as 1-4, but set cmdsize to bigger than file
$buffer = $good;
vec($buffer, 5, 32) = 0x1000;
generate $buffer;

$buffer = $good;
vec($buffer, 8, 32) = 0x1000;
generate $buffer;

$buffer = $good64;
vec($buffer, 5, 32) = 0x1000;
generate $buffer;

$buffer = $good64;
vec($buffer, 9, 32) = 0x1000;
generate $buffer;

# Bad file 9-10: overflow size+offset
$buffer = $good;
vec($buffer, 30, 32) = 0xffffffe0;
generate $buffer;

$buffer = $good64;
vec($buffer, 36, 32) = 0xffffffff;
vec($buffer, 37, 32) = 0xffffffe0;
generate $buffer;

# Bad file 11: FAT binary with just the header
generate $fat;

# Bad file 12: FAT binary where the Mach contents don't match the FAT directory
$buffer = pack("a4096 a4096 a4096", $fat, $good64, $good);
generate $buffer;

# Bad file 13: FAT binary with overflowing size
$buffer = pack("a4096 a4096 a4096", $fat, $good, $good64);
vec($buffer, 5, 32) = 0xfffffffe0;
vec($buffer, 10, 32) = 0xfffffffe0;
generate $buffer;
