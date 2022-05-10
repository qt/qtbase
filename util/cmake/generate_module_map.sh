#!/usr/bin/bash
# Copyright (C) 2018 The Qt Company Ltd.
# SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

pro_files=$(find . -name \*.pro)

for f in ${pro_files}; do
    if grep "^load(qt_module)" "${f}" > /dev/null ; then
        target=$(grep "TARGET" "${f}" | cut -d'=' -f2 | sed -e "s/\s*//g")
        module=$(basename ${f})
        echo "'${module%.pro}': '${target}',"
    fi
done
