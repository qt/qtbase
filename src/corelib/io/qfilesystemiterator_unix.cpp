/****************************************************************************
**
** Copyright (C) 2016 The Qt Company Ltd.
** Contact: https://www.qt.io/licensing/
**
** This file is part of the QtCore module of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:LGPL$
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company. For licensing terms
** and conditions see https://www.qt.io/terms-conditions. For further
** information use the contact form at https://www.qt.io/contact-us.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 3 as published by the Free Software
** Foundation and appearing in the file LICENSE.LGPL3 included in the
** packaging of this file. Please review the following information to
** ensure the GNU Lesser General Public License version 3 requirements
** will be met: https://www.gnu.org/licenses/lgpl-3.0.html.
**
** GNU General Public License Usage
** Alternatively, this file may be used under the terms of the GNU
** General Public License version 2.0 or (at your option) the GNU General
** Public license version 3 or any later version approved by the KDE Free
** Qt Foundation. The licenses are as published by the Free Software
** Foundation and appearing in the file LICENSE.GPL2 and LICENSE.GPL3
** included in the packaging of this file. Please review the following
** information to ensure the GNU General Public License requirements will
** be met: https://www.gnu.org/licenses/gpl-2.0.html and
** https://www.gnu.org/licenses/gpl-3.0.html.
**
** $QT_END_LICENSE$
**
****************************************************************************/

#include "qplatformdefs.h"
#include "qfilesystemiterator_p.h"

#if QT_CONFIG(textcodec)
#  include <qtextcodec.h>
#  include <private/qutfcodec_p.h>
#endif

#ifndef QT_NO_FILESYSTEMITERATOR

#include <memory>

#include <stdlib.h>
#include <errno.h>

QT_BEGIN_NAMESPACE

static bool checkNameDecodable(const char *d_name, qsizetype len)
{
    // This function is called in a loop from advance() below, but the loop is
    // usually run only once.

#if QT_CONFIG(textcodec)
    // We identify the codecs by their RFC 2978 MIBenum values. In this
    // function:
    //  3   US-ASCII (ANSI X3.4-1986)
    //  4   Latin1 (ISO-8859-1)
    // 106  UTF-8
    QTextCodec *codec = QTextCodec::codecForLocale();
#  ifdef QT_LOCALE_IS_UTF8
    int mibEnum = 106;
#  else
    int mibEnum = 4;                // Latin 1
    if (codec)
        mibEnum = codec->mibEnum();
#  endif
    if (Q_LIKELY(mibEnum == 106))   // UTF-8
        return QUtf8::isValidUtf8(d_name, len).isValidUtf8;
    if (mibEnum == 3)               // US-ASCII
        return QtPrivate::isAscii(QLatin1String(d_name, len));
    if (mibEnum == 4)               // Latin 1
        return true;

    // fall back to generic QTextCodec
    QTextCodec::ConverterState cs(QTextCodec::IgnoreHeader);
    codec->toUnicode(d_name, len, &cs);
    return cs.invalidChars == 0 && cs.remainingChars == 0;
#else
    // if we have no text codecs, then QString::fromLocal8Bit is fromLatin1
    return true;
#endif
}

QFileSystemIterator::QFileSystemIterator(const QFileSystemEntry &entry, QDir::Filters filters,
                                         const QStringList &nameFilters, QDirIterator::IteratorFlags flags)
    : nativePath(entry.nativeFilePath())
    , dir(nullptr)
    , dirEntry(nullptr)
    , lastError(0)
{
    Q_UNUSED(filters)
    Q_UNUSED(nameFilters)
    Q_UNUSED(flags)

    if ((dir = QT_OPENDIR(nativePath.constData())) == nullptr) {
        lastError = errno;
    } else {
        if (!nativePath.endsWith('/'))
            nativePath.append('/');
    }
}

QFileSystemIterator::~QFileSystemIterator()
{
    if (dir)
        QT_CLOSEDIR(dir);
}

bool QFileSystemIterator::advance(QFileSystemEntry &fileEntry, QFileSystemMetaData &metaData)
{
    if (!dir)
        return false;

    for (;;) {
        dirEntry = QT_READDIR(dir);

        if (dirEntry) {
            qsizetype len = strlen(dirEntry->d_name);
            if (checkNameDecodable(dirEntry->d_name, len)) {
                fileEntry = QFileSystemEntry(nativePath + QByteArray(dirEntry->d_name, len), QFileSystemEntry::FromNativePath());
                metaData.fillFromDirEnt(*dirEntry);
                return true;
            }
        } else {
            break;
        }
    }

    lastError = errno;
    return false;
}

QT_END_NAMESPACE

#endif // QT_NO_FILESYSTEMITERATOR
