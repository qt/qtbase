// Copyright (C) 2019 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#ifndef COLLECTJSON_H
#define COLLECTJSON_H

#include <qglobal.h>
#include <qstring.h>
#include <qstringlist.h>

QT_BEGIN_NAMESPACE

int collectJson(const QStringList &jsonFiles, const QString &outputFile, bool skipStdIn);

QT_END_NAMESPACE

#endif // COLLECTOJSON_H
