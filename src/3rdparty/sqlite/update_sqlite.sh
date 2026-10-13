#!/bin/bash

# Copyright (C) 2024 Christian Ehrlicher <ch.ehrlicher@gmx.de>
# SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only
#
# This is a small script fetches the sqlite tarball, unpacks it, extracts
# sqlite.c and sqlite.h and updates qt_attribution.json

version_maj=3
version_min=50
version_patch=4
year=2025

version=${version_maj}.${version_min}.${version_patch}
version_str=$(printf "%d%02d%02d00" ${version_maj} ${version_min} ${version_patch})
fn=sqlite-amalgamation-${version_str}
url=https://www.sqlite.org/${year}/${fn}.zip

#cleanup from previous attempt if there is something left
rm -rf ${fn}
rm -rf ${fn}.zip

#fetch
wget ${url}
if [ $? -ne 0 ]; then
  echo "Error fetching sqlite tarball from ${url}"
  exit
fi

#unpack
unzip ${fn}.zip

#get relevant files
cp ${fn}/sqlite3.c .
cp ${fn}/sqlite3.h .

sed -i qt_attribution.json -e "s#\"Version\": \".*\"#\"Version\": \"${version}\"#"
sed -i qt_attribution.json -e "s#\"DownloadLocation\": \".*\"#\"DownloadLocation\": \"${url}\"#"

#cleanup
rm -rf ${fn}
rm -rf ${fn}.zip

#stage
git add qt_attribution.json sqlite3.c sqlite3.h update_sqlite.sh
