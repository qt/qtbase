#!/bin/sh
# Copyright (C) 2016 The Qt Company Ltd.
# SPDX-License-Identifier: LicenseRef-Qt-Commercial

set -ex

qmake
make
cat licenseheader.txt > ../keywords.cpp
cat licenseheader.txt > ../ppkeywords.cpp
./generate_keywords >> ../keywords.cpp
./generate_keywords preprocessor >> ../ppkeywords.cpp
