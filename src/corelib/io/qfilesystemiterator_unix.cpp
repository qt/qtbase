// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only
// Qt-Security score:significant reason:default

#include "qplatformdefs.h"
#include "qfilesystemiterator_p.h"

#ifndef QT_NO_FILESYSTEMITERATOR

#include <qvarlengtharray.h>

#include <memory>

#include <stdlib.h>
#include <errno.h>

QT_BEGIN_NAMESPACE

/*
    Native filesystem iterator, which uses ::opendir()/readdir()/dirent from the system
    libraries to iterate over the directory represented by \a entry.
*/
QFileSystemIterator::QFileSystemIterator(const QFileSystemEntry &entry)
    : dirPath(entry.filePath()),
      toUtf16(QStringDecoder::Utf8)
{
    dir.reset(QT_OPENDIR(entry.nativeFilePath().constData()));
    if (!dir) {
        lastError = errno;
    } else {
        if (!dirPath.endsWith(u'/'))
            dirPath.append(u'/');
    }
}

QFileSystemIterator::QFileSystemIterator(const QFileSystemEntry &entry, QDirListing::IteratorFlags)
    : QFileSystemIterator(entry)
{}

QFileSystemIterator::QFileSystemIterator(const QFileSystemEntry &entry, QDir::Filters)
    : QFileSystemIterator(entry)
{
}

QFileSystemIterator::~QFileSystemIterator() = default;

bool QFileSystemIterator::advance(QFileSystemEntry &fileEntry, QFileSystemMetaData &metaData)
{
    auto asFileEntry = [this](QStringView name) {
#ifdef Q_OS_DARWIN
        // must match QFile::decodeName
        QString normalized = name.toString().normalized(QString::NormalizationForm_C);
        name = normalized;
#endif
        return QFileSystemEntry(dirPath + name, QFileSystemEntry::FromInternalPath());
    };
    if (!dir)
        return false;

    for (;;) {
        // From readdir man page:
        // If the end of the directory stream is reached, NULL is returned and errno is
        // not changed. If an error occurs, NULL is returned and errno is set to indicate
        // the error. To distinguish end of stream from an error, set errno to zero before
        // calling readdir() and then check the value of errno if NULL is returned.
        errno = 0;
        dirEntry = QT_READDIR(dir.get());

        if (dirEntry) {
            // POSIX allows readdir() to return a file name in struct dirent that
            // extends past the end of the d_name array (it's a char[1] array on QNX, for
            // example). Therefore, we *must* call strlen() on it to get the actual length
            // of the file name. See:
            // https://pubs.opengroup.org/onlinepubs/9699919799/basedefs/dirent.h.html#tag_13_07_05
            QByteArrayView name(dirEntry->d_name, strlen(dirEntry->d_name));
            // name.size() is sufficient here, see QUtf8::convertToUnicode() for details
            QVarLengthArray<char16_t> buffer(name.size());
            auto *end = toUtf16.appendToBuffer(buffer.data(), name);
            buffer.resize(end - buffer.constData());
            if (!toUtf16.hasError()) {
                fileEntry = asFileEntry(buffer);
                metaData.fillFromDirEnt(*dirEntry);
                return true;
            } else {
                errno = EILSEQ; // Invalid or incomplete multibyte or wide character
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
