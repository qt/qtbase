// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#ifndef CACHEKEYS_H
#define CACHEKEYS_H

#include "option.h"
#include "project.h"
#include <qstring.h>
#include <qstringlist.h>
#include <qfile.h>
#include <qfileinfo.h>
#include <qhash.h>

#include <functional>

QT_BEGIN_NAMESPACE

// -------------------------------------------------------------------------------------------------
struct FixStringCacheKey
{
    mutable size_t hash;
    QString string, pwd;
    uchar flags;
    FixStringCacheKey(const QString &s, uchar f)
    {
        hash = 0;
        pwd = qmake_getpwd();
        string = s;
        flags = f;
    }
    bool operator==(const FixStringCacheKey &f) const
    {
        return (hashCode() == f.hashCode() &&
                f.flags == flags &&
                f.string == string &&
                f.pwd == pwd);
    }
    inline size_t hashCode() const {
        if(!hash)
            hash = qHash(string) ^ qHash(flags) /*^ qHash(pwd)*/;
        return hash;
    }
};
QT_BEGIN_INCLUDE_NAMESPACE
namespace std {
template <> struct hash<QT_PREPEND_NAMESPACE(FixStringCacheKey)>
{
    size_t operator()(const QT_PREPEND_NAMESPACE(FixStringCacheKey) &key) const noexcept
    { return key.hashCode(); }
};
} // namespace std
QT_END_INCLUDE_NAMESPACE

// -------------------------------------------------------------------------------------------------
struct FileInfoCacheKey
{
    mutable size_t hash;
    QString file, pwd;
    FileInfoCacheKey(const QString &f)
    {
        hash = 0;
        if(isRelativePath(f))
            pwd = qmake_getpwd();
        file = f;
    }
    bool operator==(const FileInfoCacheKey &f) const
    {
        return (hashCode() == f.hashCode() && f.file == file &&
                f.pwd == pwd);
    }
    inline size_t hashCode() const {
        if(!hash)
            hash = qHash(file) /*^ qHash(pwd)*/;
        return hash;
    }
    inline bool isRelativePath(const QString &file) {
        int length = file.size();
        if (!length)
            return true;

        const QChar c0 = file.at(0);
        const QChar c1 = length >= 2 ? file.at(1) : QChar::Null;
        return !(c0 == QLatin1Char('/')
                || c0 == QLatin1Char('\\')
                || (c0.isLetter() && c1 == QLatin1Char(':'))
                || (c0 == QLatin1Char('/') && c1 == QLatin1Char('/'))
                || (c0 == QLatin1Char('\\') && c1 == QLatin1Char('\\')));
    }
};
QT_BEGIN_INCLUDE_NAMESPACE
namespace std {
template <> struct hash<QT_PREPEND_NAMESPACE(FileInfoCacheKey)>
{
    size_t operator()(const QT_PREPEND_NAMESPACE(FileInfoCacheKey) &key) const noexcept
    { return key.hashCode(); }
};
} // namespace std
QT_END_INCLUDE_NAMESPACE

// -------------------------------------------------------------------------------------------------
template <typename T>
inline void qmakeDeleteCacheClear(void *i) { delete reinterpret_cast<T*>(i); }

inline void qmakeFreeCacheClear(void *i) { free(i); }

typedef void (*qmakeCacheClearFunc)(void *);
void qmakeAddCacheClear(qmakeCacheClearFunc func, void **);
void qmakeClearCaches();

QT_END_NAMESPACE

#endif // CACHEKEYS_H
