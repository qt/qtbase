#!/usr/bin/perl
# Copyright (C) 2016 Intel Corporation.
# SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

# Changes the Mach-O file type header to PowerPC.
#
# The header is (from mach-o/loader.h):
# struct mach_header {
#    uint32_t   magic;          /* mach magic number identifier */
#    cpu_type_t cputype;        /* cpu specifier */
#    cpu_subtype_t      cpusubtype;     /* machine specifier */
#    uint32_t   filetype;       /* type of file */
#    uint32_t   ncmds;          /* number of load commands */
#    uint32_t   sizeofcmds;     /* the size of all the load commands */
#    uint32_t   flags;          /* flags */
# };
#
# The 64-bit header is identical in the first three fields, except for a different
# magic number. We will not touch the magic number, we'll just reset the cputype
# field to the PowerPC type and the subtype field to zero.
#
# We will not change the file's endianness. That means we might create a little-endian
# PowerPC binary, which could not be run in real life.
#
# We will also not change the 64-bit ABI flag, which is found in the cputype's high
# byte. That means we'll create a PPC64 binary if fed a 64-bit input.
#
use strict;
use constant MH_MAGIC => 0xfeedface;
use constant MH_CIGAM => 0xcefaedfe;
use constant MH_MAGIC_64 => 0xfeedfacf;
use constant MH_CIGAM_64 => 0xcffaedfe;
use constant CPU_TYPE_POWERPC => 18;
use constant CPU_SUBTYPE_POWERPC_ALL => 0;

my $infile = shift @ARGV or die("Missing input filename");
my $outfile = shift @ARGV or die("Missing output filename");

open IN, "<$infile" or die("Can't open $infile for reading: $!\n");
open OUT, ">$outfile" or die("Can't open $outfile for writing: $!\n");

binmode IN;
binmode OUT;

# Read the first 12 bytes, which includes the interesting fields of the header
my $buffer;
read(IN, $buffer, 12);

my $magic = vec($buffer, 0, 32);
if ($magic == MH_MAGIC || $magic == MH_MAGIC_64) {
    # Big endian
    # The low byte of cputype is at offset 7
    vec($buffer, 7, 8) = CPU_TYPE_POWERPC;
} elsif ($magic == MH_CIGAM || $magic == MH_CIGAM_64) {
    # Little endian
    # The low byte of cpytype is at offset 4
    vec($buffer, 4, 8) = CPU_TYPE_POWERPC;
} else {
    $magic = '';
    $magic .= sprintf("%02X ", $_) for unpack("CCCC", $buffer);
    die("Invalid input. Unknown magic $magic\n");
}
vec($buffer, 2, 32) = CPU_SUBTYPE_POWERPC_ALL;

print OUT $buffer;

# Copy the rest
while (!eof(IN)) {
    read(IN, $buffer, 4096) and
    print OUT $buffer or
    die("Problem copying: $!\n");
}
close(IN);
close(OUT);
