#!/bin/sh

# Copyright (C) 2025 Klaralvdalens Datakonsult AB (KDAB)
# SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

set -e

rm -rf tika
git clone --filter=blob:none --no-checkout https://github.com/apache/tika.git
cd tika
git sparse-checkout set tika-core/src/main/resources/org/apache/tika/mime
sha1=`git log -n 1 --pretty=format:"%H" -- tika-core/src/main/resources/org/apache/tika/mime`
cd ..

downloadURL="https://raw.githubusercontent.com/apache/tika/$sha1/tika-core/src/main/resources/org/apache/tika/mime/tika-mimetypes.xml"

wget "$downloadURL" -O tika-mimetypes.xml.orig
./process_tika_mimetypes.py

# Update attribution file
jsonfile="qt_attribution.json"
sed -i -E "s/(\"Version\":\s+\")[a-f0-9]+(\")/\1$sha1\2/" "$jsonfile"
sed -i -E "s|(https://github.com/apache/tika/blob/)[a-f0-9]+(/tika-core/src/main/resources/org/apache/tika/mime/tika-mimetypes.xml)|\1$sha1\2|" "$jsonfile"
year=$(date +%Y)
sed -i -E "s/(\"Copyright\":\s+\"Copyright )[0-9]{4}/\1$year/" "$jsonfile"

echo "Remember to run tst_QMimeDatabase now, at least to update the total mimetype count"
