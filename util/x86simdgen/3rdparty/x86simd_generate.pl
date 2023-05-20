#!/usr/bin/env perl

# Copyright 2022 Intel Corporation.
# SPDX-License-Identifier: Apache-2.0

use strict;
$\ = "\n";
$/ = "\n";
my $debug = 0;
my %leaves = (
    Leaf01ECX           => "CPUID Leaf 1, ECX",
    Leaf07_00EBX        => "CPUID Leaf 7, Sub-leaf 0, EBX",
    Leaf07_00ECX        => "CPUID Leaf 7, Sub-leaf 0, ECX",
    Leaf07_00EDX        => "CPUID Leaf 7, Sub-leaf 0, EDX",
    Leaf07_01EAX        => "CPUID Leaf 7, Sub-leaf 1, EAX",
    Leaf07_01EDX        => "CPUID Leaf 7, Sub-leaf 1, EDX",
    Leaf13_01EAX        => "CPUID Leaf 13, Sub-leaf 1, EAX",
    Leaf80000001hECX    => "CPUID Leaf 80000001h, ECX",
    Leaf80000008hEBX    => "CPUID Leaf 80000008h, EBX",
);
my @leafNames = sort keys %leaves;

# out of order (we want it first)
unshift @leafNames, "Leaf01EDX";
$leaves{Leaf01EDX} = "CPUID Leaf 1, EDX";

# Read input from file specified by first argument
my $input_conf_file = shift @ARGV;
open(FH, '<', $input_conf_file) or die $!;

my $i = 0;
my @features;
my %feature_ids;
my @architecture_names;
my %architectures;
my @xsaveStates;
my $maxarchnamelen = 0;
while (<FH>) {
    chomp $_;
    m/#\s*(.*)\s*/;
    my $comment = $1;

    s/#.*$//;
    s/^\s+//;
    next if $_ eq "";

    if (s/^arch=//) {
        my ($arch, $based, $f) = split /\s+/;
        die("Unknown base architecture \"$based\"")
            unless $based eq "<>" or grep {$_ eq $based} @architecture_names;
        my $id = lc($arch);
        $id =~ s/[^A-Za-z0-9_]/_/g;

        my $prettyname = $arch;
        $prettyname =~ s/\B([A-Z])/ $1/g;
        $prettyname =~ s/-(\w+)/ ($1)/g;
        $maxarchnamelen = length($prettyname) if length($prettyname) > $maxarchnamelen;

        my @basefeatures;
        my @extrafeatures;
        @basefeatures = @{$architectures{$based}->{allfeatures}} if $based ne "<>";
        @extrafeatures = @{$architectures{$arch}{features}} if defined($architectures{$arch});
        @extrafeatures = (@extrafeatures, split(',', $f));
        my @allfeatures = sort { $feature_ids{$a} <=> $feature_ids{$b} } (@basefeatures, @extrafeatures);

        $architectures{$arch} = {
            name => $arch,
            prettyname => $prettyname,
            id => $id,
            base => $based,
            features => \@extrafeatures,
            allfeatures => \@allfeatures,
            comment => $comment
        };
        push @architecture_names, $arch
            unless grep {$_ eq $arch} @architecture_names;
    } elsif (s/^xsave=//) {
        my ($name, $value, $required) = split /\s+/;
        push @xsaveStates,
            { id => $name, value => $value, required_for => $required, comment => $comment };
    } else {
        my ($name, $function, $bit, $depends) = split /\s+/;
        die("Unknown CPUID function \"$function\"")
            unless grep {$_ eq $function} @leafNames;
        if (my @match = grep { $_->{name} eq $name } @features) {
            die("internal error") if scalar @match != 1;
            next if $match[0]->{function} eq $function &&
                $match[0]->{bit} eq $bit && $match[0]->{depends} eq $depends;
            die("Duplicate feature \"$name\" with different details. " .
                "Previously was $match[0]->{function} bit $match[0]->{bit}.");
        }

        my $id = uc($name);
        $id =~ s/[^A-Z0-9_]/_/g;
        push @features,
        { name => $name, depends => $depends, id => $id, bit => $bit, leaf => $function, comment => $comment };
        $feature_ids{$name} = $i;
        ++$i;
        die("Too many features to fit a 64-bit integer") if $i > 64;
    }
}
close FH;

# Print the header output
my $headername = "";
my $headerguard = "";
if ($headername = shift @ARGV) {

    $headerguard = uc($headername);
    $headerguard =~ s/[^A-Z0-9_]/_/g;

    print qq|// This is a generated file. DO NOT EDIT.
// Please see $0
#ifndef $headerguard
#define $headerguard

#include <stdint.h>|;
} else {
    $debug = 1;
}

# Print the feature list
my $lastleaf;
for (my $i = 0; $i < scalar @features; ++$i) {
    my $feature = $features[$i];
    # Leaf header:
    printf "\n// in %s:\n", $leaves{$feature->{leaf}}
        if $feature->{leaf} ne $lastleaf;
    $lastleaf = $feature->{leaf};

    # Feature
    printf "#define cpu_feature_%-31s (UINT64_C(1) << %d)\n", lc($feature->{id}), $i;
}

# Print the architecture list
print "\n// CPU architectures";
for (@architecture_names) {
    my $arch = $architectures{$_};
    my $base = $arch->{base};
    if ($base eq "<>") {
        $base = "0";
    } else {
        $base =~ s/[^A-Za-z0-9_]/_/g;
        $base = "cpu_" . $base;
    }

    printf "#define cpu_%-19s (%s", lc($arch->{id}), lc($base);

    for my $f (@{$arch->{features}}) {
        my @match = grep { $_->{name} eq $f } @features;
        if (scalar @match == 1) {
            printf " \\\n%33s| cpu_feature_%s", " ", lc($match[0]->{id});
        } else {
            printf STDERR "%s: unknown feature '%s' for CPU '%s'\n", $0, $f, $arch->{name}
                if $debug;
        }
    }
    print ")";
}

print "\n// __attribute__ target strings for GCC and Clang";
for (my $i = 0; $i < scalar @features; ++$i) {
    my $feature = $features[$i];
    my $str = $feature->{name} . ',' . $feature->{depends};
    $str =~ s/,$//;
    printf "#define QT_FUNCTION_TARGET_STRING_%-17s \"%s\"\n",
        $feature->{id}, $str;
}
for (@architecture_names) {
    my $arch = $architectures{$_};
    my $base = $arch->{base};
    my $featurestr = "";
    if ($base ne "<>") {
        $featurestr = "QT_FUNCTION_TARGET_STRING_ARCH_" . uc($base);
    }

    my @features = @{$arch->{features}};
    #@features = map { defined($feature_ids{$_}) ? $_ : () } @features;
    if (scalar @features) {
        $featurestr .= ' ",' if length $featurestr;
        $featurestr .= '"' unless length $featurestr;
        $featurestr .= join(',', @features);
        $featurestr .= '"';
    }
    printf "#define QT_FUNCTION_TARGET_STRING_ARCH_%-12s %s\n", uc($arch->{id}), $featurestr;
}

print q{
static const uint64_t _compilerCpuFeatures = 0};

# And print the compiler-enabled features part:
for (my $i = 0; $i < scalar @features; ++$i) {
    my $feature = $features[$i];
    printf
        "#ifdef __%s__\n" .
        "         | cpu_feature_%s\n" .
        "#endif\n",
        $feature->{id}, lc($feature->{id});
}

print '        ;';
if ($headerguard ne "") {
    print q|
#if (defined __cplusplus) && __cplusplus >= 201103L
enum X86CpuFeatures : uint64_t {|;

    for (@features) {
        my $line = sprintf "CpuFeature%s = cpu_feature_%s,", $_->{id}, lc($_->{id});
        if ($_->{comment} ne "") {
            printf "    %-56s ///< %s\n", $line, $_->{comment};
        } else {
            print "    $line";
        }
    }

print qq|}; // enum X86CpuFeatures

enum X86CpuArchitectures : uint64_t {|;

    for (@architecture_names) {
        my $arch = $architectures{$_};
        my $name = $arch->{name};
        $name =~ s/[^A-Za-z0-9]//g;
        my $line = sprintf "CpuArch%s = cpu_%s,", $name, lc($arch->{id});
        if ($arch->{comment} ne "") {
            printf "    %-56s ///< %s\n", $line, $arch->{comment};
        } else {
            print "    $line";
        }
    }

    print qq|}; // enum X86cpuArchitectures
#endif /* C++11 */\n|;
};

print "// -- implementation start --\n";
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
printf "\nstatic const %s features_indices[] = {",
    $offset > 255 ? "uint16_t" : "uint8_t";
for (my $j = 0; $j < scalar @offsets; ++$j) {
    printf "%s%3d,",
        $j % 8 ? " " : "\n    ", $offsets[$j];
}
print "\n};";

# Print the locator enum and table
print "\nenum X86CpuidLeaves {";
map { print "    $_," } @leafNames;
print "    X86CpuidMaxLeaf\n};";

my $type = scalar keys %leaves > 8 ? "uint16_t" : "uint8_t";
printf "\nstatic const %s x86_locators[] = {\n",
    $type, $type;
for (my $j = 0; $j < scalar @features; ++$j) {
    my $feature = $features[$j];
    printf "    %s*32 + %2d, %s// %s\n",
        $feature->{leaf}, $feature->{bit}, ' ' x (24 - length($feature->{leaf})), $feature->{name};
}
print '};';

# Generate the processor name listing, sorted by feature length
my %sorted_archs;
for (@architecture_names) {
    my $arch = $architectures{$_};
    my $key = sprintf "%02d_%s", scalar(@{$arch->{allfeatures}}), join(',', @{$arch->{allfeatures}});
    $sorted_archs{$key} = $arch;
}
print qq|
struct X86Architecture
{
    uint64_t features;
    char name[$maxarchnamelen + 1];
};

static const struct X86Architecture x86_architectures[] = {|;
for (sort keys %sorted_archs) {
    my $arch = $sorted_archs{$_};
    next if $arch->{base} eq "<>";
    printf "    { cpu_%s, \"%s\" },\n", $arch->{id}, $arch->{prettyname};
}
print "};";

# Produce the list of XSAVE states
print "\nenum XSaveBits {";
my $xsaveEnumPrefix = "XSave_";
for my $state (@xsaveStates) {
    my $value = $state->{value};
    unless ($value =~ /^0x/) {
        # Compound value
        $value = join(" | ", map { $xsaveEnumPrefix . $_ } split(/\|/, $value));
    }
    printf "    %s%-12s = %s,", $xsaveEnumPrefix, $state->{id}, $value;
    printf "%s// %s", ' ' x (18 - length($value)), $state->{comment}
        if $state->{comment} ne '';
    printf "\n";
};
print "};";

# Produce a list of features require extended XSAVE state
my $xsaveRequirementMapping;
for my $state (@xsaveStates) {
    my $xsaveReqPrefix = "XSaveReq_";
    my @required_for = split /,/, $state->{required_for};
    next unless scalar @required_for;

    my $prefix = sprintf "\n// List of features requiring %s%s\nstatic const uint64_t %s%s = 0",
        $xsaveEnumPrefix, $state->{id}, $xsaveReqPrefix, $state->{id};

    # match either the feature name or one of its requirements against list
    # of features that this state is required for
    for my $feature (@features) {
        my $id = lc($feature->{id});
        my $required = 0;
        for my $requirement (@required_for) {
            my @depends = split /,/, "$id," . $feature->{depends};
            $required = grep { $_ eq $requirement } @depends;
            last if $required;
        }
        printf "$prefix\n        | cpu_feature_%s", $id if $required;
        $prefix = "" if $required;
    }

    if ($prefix eq "") {
        # we printed something
        print ";";
        $xsaveRequirementMapping .= sprintf "    { %s%s, %s%s },\n",
            $xsaveReqPrefix, $state->{id}, $xsaveEnumPrefix, $state->{id};
    }
}

# Finally, make a table
printf qq|
struct XSaveRequirementMapping
{
    uint64_t cpu_features;
    uint64_t xsave_state;
};

static const struct XSaveRequirementMapping xsave_requirements[] = {
%s};

// -- implementation end --
#endif /* $headerguard */\n|, $xsaveRequirementMapping if $xsaveRequirementMapping ne "";
