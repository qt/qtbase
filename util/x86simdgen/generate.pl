#!/usr/bin/env perl
#############################################################################
##
## Copyright (C) 2018 Intel Corporation.
## Contact: https://www.qt.io/licensing/
##
## This file is part of the build configuration tools of the Qt Toolkit.
##
## $QT_BEGIN_LICENSE:MIT$
## Permission is hereby granted, free of charge, to any person obtaining a copy
## of this software and associated documentation files (the "Software"), to deal
## in the Software without restriction, including without limitation the rights
## to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
## copies of the Software, and to permit persons to whom the Software is
## furnished to do so, subject to the following conditions:
##
## The above copyright notice and this permission notice shall be included in
## all copies or substantial portions of the Software.
##
## THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
## IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
## FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
## AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
## LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
## OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
## THE SOFTWARE.
## $QT_END_LICENSE$
##
#############################################################################

use strict;
$\ = "\n";
$/ = "\n";
my %leaves = (
    Leaf1EDX        => "CPUID Leaf 1, EDX",
    Leaf1ECX        => "CPUID Leaf 1, ECX",
    Leaf7_0EBX      => "CPUID Leaf 7, Sub-leaf 0, EBX",
    Leaf7_0ECX      => "CPUID Leaf 7, Sub-leaf 0, ECX",
    Leaf7_0EDX      => "CPUID Leaf 7, Sub-leaf 0, EDX",
);
my @leafNames = sort keys %leaves;

# Read data from stdin
my $i = 1;
my @features;
while (<STDIN>) {
    s/#.*$//;
    chomp;
    next if $_ eq "";

    my ($name, $function, $bit, $depends) = split /\s+/;
    die("Unknown CPUID function \"$function\"")
        unless grep $function, @leafNames;

    my $id = uc($name);
    $id =~ s/[^A-Z0-9_]/_/g;
    push @features,
        { name => $name, depends => $depends, id => $id, bit => $bit, leaf => $function };
    ++$i;
}

if (my $h = shift @ARGV) {
    open HEADER, ">", $h;
    select HEADER;
}

# Print the qsimd_x86_p.h output
print q{// This is a generated file. DO NOT EDIT.
// Please see util/x86simdgen/generate.pl";
#ifndef QSIMD_P_H
#  error "Please include <private/qsimd_p.h> instead"
#endif
#ifndef QSIMD_X86_P_H
#define QSIMD_X86_P_H

#include "qsimd_p.h"

//
//  W A R N I N G
//  -------------
//
// This file is not part of the Qt API.  It exists purely as an
// implementation detail.  This header file may change from version to
// version without notice, or even be removed.
//
// We mean it.
//

QT_BEGIN_NAMESPACE

// used only to indicate that the CPU detection was initialized
#define QSimdInitialized                            (Q_UINT64_C(1) << 0)};

# Print the enum
my $lastleaf;
for (my $i = 0; $i < scalar @features; ++$i) {
    my $feature = $features[$i];
    # Leaf header:
    printf "\n// in %s:\n", $leaves{$feature->{leaf}}
        if $feature->{leaf} ne $lastleaf;
    $lastleaf = $feature->{leaf};

    # Feature
    printf "#define CpuFeature%-33s (Q_UINT64_C(1) << %d)\n", $feature->{id}, $i + 1;

    # Feature string names for Clang and GCC
    my $str = $feature->{name};
    $str .= ",$feature->{depends}" if defined($feature->{depends});
    printf "#define QT_FUNCTION_TARGET_STRING_%-17s \"%s\"\n",
        $feature->{id}, $str;
}

print q{
static const quint64 qCompilerCpuFeatures = 0};

# And print the compiler-enabled features part:
for (my $i = 0; $i < scalar @features; ++$i) {
    my $feature = $features[$i];
    printf
        "#ifdef __%s__\n" .
        "         | CpuFeature%s\n" .
        "#endif\n",
        $feature->{id}, $feature->{id};
}

print q{        ;

QT_END_NAMESPACE

#endif // QSIMD_X86_P_H
};

if (my $cpp = shift @ARGV) {
    open CPP, ">", $cpp;
    select CPP;
} else {
    print q{

---- cut here, paste the rest into qsimd_x86.cpp ---


};
};

print "// This is a generated file. DO NOT EDIT.";
print "// Please see util/x86simdgen/generate.pl";
print '#include "qsimd_p.h"';
print "";

# Now generate the string table and bit-location array
my $offset = 0;
my @offsets;
print "static const char features_string[] =";
for my $feature (@features) {
    print "    \" $feature->{name}\\0\"";
    push @offsets, $offset;
    $offset += 2 + length($feature->{name});
}
print "    \"\\0\";";

# Print the string offset table
printf "\nstatic const %s features_indices[] = {\n    %3d",
    $offset > 255 ? "quint16" : "quint8", $offset;
for (my $j = 0; $j < scalar @offsets; ++$j) {
    printf ",%s%3d",
        ($j + 1) % 8 ? " " : "\n    ", $offsets[$j];
}
print "\n};";

# Print the locator enum and table
print "\nenum X86CpuidLeaves {";
map { print "    $_," } @leafNames;
print "    X86CpuidMaxLeaf\n};";

my $type = scalar %leaves > 8 ? "quint16" : "quint8";
printf "\nstatic const %s x86_locators[] = {",
    $type, $type;
my $lastname;
for (my $j = 0; $j < scalar @features; ++$j) {
    my $feature = $features[$j];
    printf ", // %s", $lastname
        if defined($lastname);
    printf "\n    %s*32 + %2d",
        $feature->{leaf}, $feature->{bit};
    $lastname = $feature->{name};
}
printf qq{  // $lastname
\};

// List of AVX512 features (see detectProcessorFeatures())
static const quint64 AllAVX512 = 0};

# Print AVX512 features
for (my $j = 0; $j < scalar @features; ++$j) {
    my $feature = $features[$j];
    $_ = $feature->{id};
    printf "\n        | CpuFeature%s", $_ if /AVX512/;
}
print ";";
