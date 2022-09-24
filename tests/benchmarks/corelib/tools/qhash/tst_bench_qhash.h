// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include <QString>

struct Qt4String : QString
{
    Qt4String() {}
    Qt4String(const QString &s) : QString(s) {}
};

QT_BEGIN_NAMESPACE
size_t qHash(const Qt4String &);
QT_END_NAMESPACE

struct Qt50String : QString
{
    Qt50String() {}
    Qt50String(const QString &s) : QString(s) {}
};

QT_BEGIN_NAMESPACE
size_t qHash(const Qt50String &, size_t seed = 0);
QT_END_NAMESPACE


struct JavaString : QString
{
    JavaString() {}
    JavaString(const QString &s) : QString(s) {}
};

QT_BEGIN_NAMESPACE
size_t qHash(const JavaString &);
QT_END_NAMESPACE

