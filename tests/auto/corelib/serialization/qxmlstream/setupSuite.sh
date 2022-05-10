#!/bin/sh
# Copyright (C) 2016 The Qt Company Ltd.
# SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

# 
# Hello!
#
# You don't have to run this script unless you are actually updating the test suite.
# For precaution, we therefore have this exit call.


# CVS is retarded when it comes to reverting changes. Remove files it has moved.
find XML-Test-Suite/ -name ".*.?.*" | xargs rm

cd XML-Test-Suite

export CVSROOT=":pserver:anonymous@dev.w3.org:/sources/public"
cvs -q up -C

git checkout -- `find -name "Entries"` # They only contain CVS timestamps.
xmllint --valid --noent xmlconf/xmlconf.xml --output xmlconf/finalCatalog.xml
