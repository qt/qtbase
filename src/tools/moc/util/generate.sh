#!/bin/sh
# Copyright (C) 2016 The Qt Company Ltd.
# SPDX-License-Identifier: LicenseRef-Qt-Commercial

set -ex

qmake
make
cat licenseheader.cpp.in > ../keywords.cpp
cat licenseheader.cpp.in > ../ppkeywords.cpp
./generate_keywords >> ../keywords.cpp
./generate_keywords preprocessor >> ../ppkeywords.cpp
