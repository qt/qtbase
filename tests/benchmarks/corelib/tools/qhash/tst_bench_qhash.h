// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only

#include <QHashFunctions>
#include <QString>

struct OwningLatin1String : QByteArray
{
    OwningLatin1String() = default;
    OwningLatin1String(const QByteArray &a) : QByteArray(a) {}
    OwningLatin1String(QByteArray &&a) : QByteArray(std::move(a)) {}
};
QT_BEGIN_NAMESPACE
inline size_t qHash(const OwningLatin1String &s, size_t seed = 0)
{ return qHash(QLatin1StringView(s), seed); }
QT_END_NAMESPACE

struct Qt4String : QString
{
    Qt4String() {}
    Qt4String(const QString &s) : QString(s) {}
};

QT_BEGIN_NAMESPACE
size_t qHash(const Qt4String &, size_t = 0);
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
size_t qHash(const JavaString &, size_t = 0);
QT_END_NAMESPACE

