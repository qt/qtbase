#!/usr/bin/env perl
#############################################################################
##
## Copyright (C) 2013 Digia Plc and/or its subsidiary(-ies).
## Contact: http://www.qt-project.org/legal
##
## This file is part of the test suite of the Qt Toolkit.
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
##
#############################################################################

use strict;
use Cwd;
use warnings;

# Usage: test.pl <SearchPath> <ExecutionMode> <TestResults> <Timeout [Default 300 seconds]>
# Variable declarations to keep strict happy
our $SEARCH_PATH;
our $EXEC_MODE;
our $EXE_PREFIX;
our $EXE_SUFFIX;
our $TIMEOUT;
our $REPORTDIR;
our $buryChildren;
our $timeoutChildren;
our $totalExecuted;
our $totalStarted;
our $totalTimedOut;
our $currentDirectory;
our $testRoot;

# Where do we run this script? What directory?
$SEARCH_PATH=$ARGV[0];
if(!$SEARCH_PATH)
{
    print "Please specify the search directory! \n";
    exit(0);
}

# We have four options:
# 'U': Unix
# 'W': Windows
# 'M': Mac
# 'E': Embedded
$EXEC_MODE=$ARGV[1];
if($EXEC_MODE =~ /^U$/)
{
    print "Using Unix execution mode\n";
    $EXE_PREFIX="./";
    $EXE_SUFFIX="";
} elsif($EXEC_MODE =~ /^W$/)
{
    print "Using Windows execution mode\n";
    $EXE_PREFIX="";
    $EXE_SUFFIX=".exe";
} elsif($EXEC_MODE =~ /^M$/)
{
    print "Using OSX execution mode\n";
    $EXE_PREFIX="/Contents/MacOS/";
    $EXE_SUFFIX=".app";
} elsif($EXEC_MODE =~ /^E$/)
{
    print "Using embedded execution mode\n";
    $EXE_PREFIX="./";
    $EXE_SUFFIX="";
} else {
    print "Unknown execution mode: $EXEC_MODE \n";
    print "Use: 'U' (Unix), 'W' (Windows), 'M' (MacOSX)\n";
    exit(0);
}
# We get the current directory, we 'll need it afterwards
$currentDirectory = getcwd();

$testRoot = Cwd::abs_path($SEARCH_PATH);

# We assume that by default goes to "reports" unless the user specifies it.
$REPORTDIR = $ARGV[2];
if(!$REPORTDIR)
{
    $REPORTDIR = $testRoot."/reports";
    mkdir $REPORTDIR;
} else {
    mkdir $REPORTDIR;
    $REPORTDIR = Cwd::abs_path($REPORTDIR);
}

# If given we use it, otherwise we default to 300 seconds.
$TIMEOUT = $ARGV[3];
if(!$TIMEOUT)
{
    $TIMEOUT=300;
}
print "Timeout value: $TIMEOUT\n";

# Initialize 'global' variables
$buryChildren = 0;
$timeoutChildren = 0;
$totalExecuted = 0;
$totalStarted = 0;
$totalTimedOut = 0;

# Install signal handlers and pray for the best
$SIG{'CHLD'} = 'handleDeath';
$SIG{'ALRM'} = 'handleTimeout';

handleDir($testRoot);

print " ** Statistics ** \n";
print " Tests started: $totalStarted \n";
print " Tests executed: $totalExecuted \n";
print " Tests timed out: $totalTimedOut \n";

sub handleDir {

    my ($dir) = @_;
    my $currentDir = getcwd();

    chdir($dir) || die("Could not chdir to $dir");
    my @components;
    my $command;
    @components = split(/\//, $dir);
    my $component = $components[$#components];

    $command = "tst_".$component;

    if ( -e $command.$EXE_SUFFIX )
    {
        executeTestCurrentDir($command);
    } else {
        opendir(DIR, $dir);
        my @files = readdir(DIR);
        closedir DIR;
        my $file;
        foreach $file (@files)
        {
            #skip hidden files
            next if (substr($file,0,1) eq ".");

            if ( -d $dir."/".$file)
            {
                handleDir($dir."/".$file)
            }

        }
    }
    chdir($currentDir);
}

sub executeTestCurrentDir {
    my ($command) = @_;
            print "Executing $command \n";
            my $myPid;
            $myPid = fork();
            if($myPid == 0)
            {
                my $realCommand;
                if($EXEC_MODE =~/^M$/)
                {
                    $realCommand = "./".$command.$EXE_SUFFIX.$EXE_PREFIX.$command;
                } else {
                    $realCommand = $EXE_PREFIX.$command.$EXE_SUFFIX;
                }
                my $outputRedirection = $REPORTDIR."/".$command.$EXE_SUFFIX.".xml";

                if($EXEC_MODE =~ /^E$/)
                {
                    exec($realCommand, "-qws", "-xml", "-o", $outputRedirection);
                } else {
                    exec($realCommand, "-xml", "-o", $outputRedirection);
                }
                exit(0);
            } elsif($myPid > 0 )
            {
                $totalStarted++;
                alarm($TIMEOUT);
                while(!$buryChildren && !$timeoutChildren)
                {
                    sleep 10;
                }
                if($buryChildren)
                {
                    my $value;
                    $value = waitpid($myPid , 0);
                    $buryChildren = 0;
                    $totalExecuted++;
                } elsif($timeoutChildren)
                {
                    kill 'INT', $myPid;
                    $timeoutChildren = 0;
                    $totalTimedOut++;
                } else {
                    # What?? If we get here we need to abort, this is totally unexpected
                    print "Wrong condition evaluated, aborting to avoid damages\n";
                    exit(0);
                }
                # We need to handle children killed because of timeout
                if($buryChildren)
                {
                    my $value;
                    $value = waitpid($myPid , 0);
                    $buryChildren = 0;
                }
            } else {
                print "Problems trying to execute $command";
            }

}

# This procedure takes care of handling dead children on due time
sub handleDeath {
    $buryChildren = 1;
}

# This takes care of timeouts
sub handleTimeout {
    $timeoutChildren = 1;
}

