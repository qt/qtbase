#!/usr/bin/perl
#############################################################################
##
## Copyright (C) 2012 Nokia Corporation and/or its subsidiary(-ies).
## Contact: http://www.qt-project.org/
##
## This file is part of the S60 port of the Qt Toolkit.
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
##
## $QT_END_LICENSE$
##
#############################################################################

#######################################################################
#
# A script for setting binary capabilities based on .pkg file contents.
#
#######################################################################

#
# Note: Please make sure to output all changes done to the pkg file in a print statements
#       starting with "Patching: " to ease integration into IDEs!
#

use File::Copy;
use File::Spec;
use File::Path;

sub Usage() {
    print("This script can be used to set capabilities of all binaries\n");
    print("specified for deployment in a .pkg file.\n");
    print("If no capabilities are given, the binaries will be given the\n");
    print("capabilities supported by self-signed certificates.\n\n");
    print(" *** NOTE: If *_template.pkg file is given and one is using symbian-abld or\n");
    print(" symbian-sbsv2 platform, 'target-platform' is REQUIRED. ***\n\n");
    print(" *** NOTE2: When patching gcce binaries built with symbian-sbsv2 toolchain,\n");
    print(" armv5 must be specified as platform.\n");
    print("\nUsage: patch_capabilities.pl [-c|-t tmp_path] pkg_filename [target-platform [capability list]]\n");
    print("\nE.g. patch_capabilities.pl myapp_template.pkg release-armv5 \"All -TCB\"\n");
    print("\nThe parameter -c can be used to just check if package is compatible with self-signing\n");
    print("without actually doing any patching.\n");
    print("Explicit capability list cannot be used with -c parameter.\n");
    print("\nThe parameter -t can be used to specify a dir under which the temporary files are created.\n");
    print("Defaults to 'patch_capabilities_tmp' under the path to pkg file.\n");
    exit();
}

sub trim($) {
    my $string = shift;
    $string =~ s/^\s+//;
    $string =~ s/\s+$//;
    return $string;
}

my $epocroot = $ENV{EPOCROOT};
my $epocToolsDir = "";
if ($epocroot ne "") {
    $epocroot =~ s,\\,/,g;
    if ($epocroot =~ m,[^/]$,) {
        $epocroot = $epocroot."/";
    }
    $epocToolsDir = "${epocroot}epoc32/tools/";
}

my $nullDevice = "/dev/null";
$nullDevice = "NUL" if ($^O =~ /MSWin/);

my @capabilitiesToAllow = ("LocalServices", "NetworkServices", "ReadUserData", "UserEnvironment", "WriteUserData", "Location");
my @capabilitiesSpecified = ();

# If arguments were given to the script,
if (@ARGV)
{
    # Parse the first given script argument as a ".pkg" file name.
    my $pkgFileName = shift(@ARGV);
    my $justCheck = "";
    my $msgPrefix = "Patching:";
    my $tempPatchPath = "";

    if ($pkgFileName eq "-c") {
        $pkgFileName = shift(@ARGV);
        $justCheck = true;
        $msgPrefix = "Warning:";
    }

    if ($pkgFileName eq "-t") {
        $tempPatchPath = shift(@ARGV);
        $pkgFileName = shift(@ARGV);
    }

    my ($pkgVolume, $pkgPath, $pkgPlainFileName) = File::Spec->splitpath($pkgFileName);
    if ($tempPatchPath eq "") {
        $tempPatchPath = File::Spec->catpath($pkgVolume, $pkgPath."patch_capabilities_tmp", "");
    }

    mkpath($tempPatchPath);

    # These variables will only be set for template .pkg files.
    my $target;
    my $platform;

    # Check if using template .pkg and set target/platform variables
    if (($pkgFileName =~ m|_template\.pkg$|i) && -r($pkgFileName))
    {
        my $targetplatform;
        my $templateFile;
        my $templateContents;
        open($templateFile, "< $pkgFileName") or die ("Could not open $pkgFileName");
        $templateContents = <$templateFile>;
        close($templateFile);
        unless (($targetplatform = shift(@ARGV)) || $templateContents !~ /\$\(PLATFORM\)/)
        {
            Usage();
        }
        $targetplatform = "-" if (!$targetplatform);
        my @tmpvalues = split('-', $targetplatform);
        $target = $tmpvalues[0];
        $platform = $tmpvalues[1];

        # Convert visual target to real target (debug->udeb and release->urel)
        $target =~ s/debug/udeb/i;
        $target =~ s/release/urel/i;

        if (($platform =~ m/^gcce$/i) && ($ENV{SBS_HOME})) {
            # Print a informative note in case suspected misuse is detected.
            print "\nNote: You must use armv5 as platform when packaging gcce binaries built using symbian-sbsv2 mkspec.\n";
        }
    }

    # If the specified ".pkg" file exists (and can be read),
    if (($pkgFileName =~ m|\.pkg$|i) && -r($pkgFileName))
    {
        print ("\n");
        if ($justCheck) {
            print ("Checking");
        } else {
            print ("Patching");
        }
        print (" package file and relevant binaries...\n");

        if (!$justCheck) {
            # If there are more arguments given, parse them as capabilities.
            if (@ARGV)
            {
                @capabilitiesSpecified = ();
                while (@ARGV)
                {
                    push (@capabilitiesSpecified, pop(@ARGV));
                }
            }
        }

        # Start with no binaries listed.
        my @binaries = ();
        my $binariesDelimeter = "///";

        my $tempPkgFileName = $tempPatchPath."/__TEMP__".$pkgPlainFileName;

        if (!$justCheck) {
            unlink($tempPkgFileName);
            open (NEW_PKG, ">>".$tempPkgFileName);
        }
        open (PKG, "<".$pkgFileName);

        my $checkFailed = "";
        my $somethingPatched = "";

        # Parse each line.
        while (<PKG>)
        {
            my $line = $_;
            my $newLine = $line;

            # Patch pkg UID if it's in protected range
            if ($line =~ m/^\#.*\((0x[0-7][0-9a-fA-F]*)\).*$/)
            {
                my $oldUID = $1;
                print ("$msgPrefix UID $oldUID is not compatible with self-signing!\n");

                if ($justCheck) {
                    $checkFailed = true;
                } else {
                    my $newUID = $oldUID;
                    $newUID =~ s/0x./0xE/i;
                    $newLine =~ s/$oldUID/$newUID/;
                    print ("$msgPrefix Package UID changed to: $newUID.\n");
                    $somethingPatched = true;
                }
            }

            # If the line specifies a file, parse the source and destination locations.
            if ($line =~ m|^ *\"([^\"]+)\"\s*\-\s*\"([^\"]+)\"|)
            {
                my $sourcePath = $1;

                # If the given file is a binary, check the target and binary type (+ the actual filename) from its path.
                if ($sourcePath =~ m:\w+(\.dll|\.exe)$:i)
                {
                    # Do preprocessing for template pkg,
                    # In case of template pkg target and platform variables are set
                    if(length($target) && length($platform))
                    {
                        $sourcePath =~ s/\$\(PLATFORM\)/$platform/gm;
                        $sourcePath =~ s/\$\(TARGET\)/$target/gm;
                    }

                    my ($dummy1, $dummy2, $binaryBaseName) = File::Spec->splitpath($sourcePath);

                    if ($justCheck) {
                        push (@binaries, $binaryBaseName.$binariesDelimeter.$sourcePath);
                    } else {
                        # Copy original files over to patching dir
                        # Patching dir will be flat to make it cleanable with QMAKE_CLEAN, so path
                        # will be collapsed into the file name to avoid name collisions in the rare
                        # case where custom pkg rules are used to install files with same names from
                        # different directories (probably using platform checks to choose only one of them.)
                        my $patchedSourcePath = $sourcePath;
                        $patchedSourcePath =~ s/[\/\\:]/_/g;
                        $patchedSourcePath = "$tempPatchPath/$patchedSourcePath";
                        $newLine =~ s/^.*(\.dll|\.exe)(.*)(\.dll|\.exe)/\"$patchedSourcePath$2$3/i;

                        copy($sourcePath, $patchedSourcePath) or die "$sourcePath cannot be copied for patching.";
                        push (@binaries, $binaryBaseName.$binariesDelimeter.$patchedSourcePath);
                    }
                }
            }

            print NEW_PKG $newLine;

            chomp ($line);
        }

        close (PKG);
        if (!$justCheck) {
            close (NEW_PKG);

            unlink($pkgFileName);
            rename($tempPkgFileName, $pkgFileName);
        }
        print ("\n");

        my $baseCommandToExecute = "${epocToolsDir}elftran -vid 0x0 -capability \"%s\" ";

        # Actually set the capabilities of the listed binaries.
        foreach my $binariesItem(@binaries)
        {
            $binariesItem =~ m|^(.*)$binariesDelimeter(.*)$|;
            my $binaryBaseName = $1;
            my $binaryPath = $2;

            # Create the command line for setting the capabilities.
            my $commandToExecute = $baseCommandToExecute;
            my $executeNeeded = "";
            if (@capabilitiesSpecified)
            {
                $commandToExecute = sprintf($baseCommandToExecute, join(" ", @capabilitiesSpecified));
                $executeNeeded = true;
                my $capString = join(" ", @capabilitiesSpecified);
                print ("$msgPrefix Patching the the Vendor ID to 0 and the capabilities used to: \"$capString\" in \"$binaryBaseName\".\n");
            } else {
                # Test which capabilities are present and then restrict them to the allowed set.
                # This avoid raising the capabilities of apps that already have none.
                my $dllCaps;
                open($dllCaps, "${epocToolsDir}elftran -dump s $binaryPath |") or die ("ERROR: Could not execute elftran");
                my $capsFound = 0;
                my $originalVid;
                my @capabilitiesToSet;
                my $capabilitiesToAllow = join(" ", @capabilitiesToAllow);
                my @capabilitiesToDrop;
                while (<$dllCaps>) {
                    if (/^Secure ID: ([0-7][0-9a-fA-F]*)$/) {
                        my $exeSid = $1;
                        if ($binaryBaseName =~ /\.exe$/) {
                            # Installer refuses to install protected executables in a self signed package, so abort if one is detected.
                            # We can't simply just patch the executable SID, as any registration resources executable uses will be linked to it via SID.
                            print ("$msgPrefix Executable with SID in the protected range (0x$exeSid) detected: \"$binaryBaseName\". A self-signed sis with protected executables is not supported.\n\n");
                            $checkFailed = true;
                        }
                    }
                    if (/^Vendor ID: ([0-9a-fA-F]*)$/) {
                        $originalVid = "$1";
                    }
                    if (!$capsFound) {
                        $capsFound = 1 if (/Capabilities:/);
                    } else {
                        $_ = trim($_);
                        if ($capabilitiesToAllow =~ /$_/) {
                            push(@capabilitiesToSet, $_);
                            if (Location =~ /$_/i) {
                                print ("$msgPrefix \"Location\" capability detected for binary: \"$binaryBaseName\". This capability is not self-signable for S60 3rd edition feature pack 1 devices, so installing this package on those devices will most likely not work.\n\n");
                            }
                        } else {
                            push(@capabilitiesToDrop, $_);
                        }
                    }
                }
                close($dllCaps);
                if ($originalVid !~ "00000000") {
                    print ("$msgPrefix Non-zero vendor ID (0x$originalVid) is incompatible with self-signed packages in \"$binaryBaseName\"");
                    if ($justCheck) {
                        print (".\n\n");
                        $checkFailed = true;
                    } else {
                        print (", setting it to zero.\n\n");
                        $executeNeeded = true;
                    }
                }
                if ($#capabilitiesToDrop) {
                    my $capsToDropStr = join("\", \"", @capabilitiesToDrop);
                    $capsToDropStr =~ s/\", \"$//;

                    if ($justCheck) {
                        print ("$msgPrefix The following capabilities used in \"$binaryBaseName\" are not compatible with a self-signed package: \"$capsToDropStr\".\n\n");
                        $checkFailed = true;
                    } else {
                        if ($binaryBaseName =~ /\.exe$/) {
                            # While libraries often have capabilities they do not themselves need just to enable them to be loaded by wider variety of processes,
                            # executables are more likely to need every capability they have been assigned or they won't function correctly.
                            print ("$msgPrefix Executable with capabilities incompatible with self-signing detected: \"$binaryBaseName\". (Incompatible capabilities: \"$capsToDropStr\".) Reducing capabilities is only supported for libraries.\n");
                            $checkFailed = true;
                        } else {
                            print ("$msgPrefix The following capabilities used in \"$binaryBaseName\" are not compatible with a self-signed package and will be removed: \"$capsToDropStr\".\n");
                            $executeNeeded = true;
                        }
                    }
                }
                $commandToExecute = sprintf($baseCommandToExecute, join(" ", @capabilitiesToSet));
            }
            $commandToExecute .= $binaryPath;

            if ($executeNeeded) {
                # Actually execute the elftran command to set the capabilities.
                print ("\n");
                system ("$commandToExecute > $nullDevice");
                $somethingPatched = true;
            }
        }

        if ($checkFailed) {
            print ("\n");
            if ($justCheck) {
                print ("$msgPrefix The package is not compatible with self-signing.\n");
            } else {
                print ("$msgPrefix Unable to patch the package for self-singing.\n");
            }
            print ("Use a proper developer certificate for signing this package.\n\n");
            exit(1);
        }

        if ($justCheck) {
            print ("Package is compatible with self-signing.\n");
        } else {
            if ($somethingPatched) {
                print ("NOTE: A patched package may not work as expected due to reduced capabilities and other modifications,\n");
                print ("      so it should not be used for any kind of Symbian signing or distribution!\n");
                print ("      Use a proper certificate to avoid the need to patch the package.\n");
            } else {
                print ("No patching was required!\n");
            }
        }
        print ("\n");
    } else {
        Usage();
    }
}
else
{
    Usage();
}
