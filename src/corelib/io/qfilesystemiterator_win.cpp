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

#include "qfilesystemiterator_p.h"
#include "qfilesystemengine_p.h"
#include "qoperatingsystemversion.h"
#include "qplatformdefs.h"
#include "qvector.h"

#include <QtCore/qt_windows.h>

QT_BEGIN_NAMESPACE

bool done = true;

QFileSystemIterator::QFileSystemIterator(const QFileSystemEntry &entry, QDir::Filters filters,
                                         const QStringList &nameFilters, QDirIterator::IteratorFlags flags)
    : nativePath(entry.nativeFilePath())
    , dirPath(entry.filePath())
    , findFileHandle(INVALID_HANDLE_VALUE)
    , uncFallback(false)
    , uncShareIndex(0)
    , onlyDirs(false)
{
    Q_UNUSED(nameFilters)
    Q_UNUSED(flags)
    if (nativePath.endsWith(QLatin1String(".lnk"))) {
        QFileSystemMetaData metaData;
        QFileSystemEntry link = QFileSystemEngine::getLinkTarget(entry, metaData);
        nativePath = link.nativeFilePath();
    }
    if (!nativePath.endsWith(QLatin1Char('\\')))
        nativePath.append(QLatin1Char('\\'));
    nativePath.append(QLatin1Char('*'));
    // In MSVC2015+ case we prepend //?/ for longer file-name support
    if (!dirPath.endsWith(QLatin1Char('/')))
        dirPath.append(QLatin1Char('/'));
    if ((filters & (QDir::Dirs|QDir::Drives)) && (!(filters & (QDir::Files))))
        onlyDirs = true;
}

QFileSystemIterator::~QFileSystemIterator()
{
   if (findFileHandle != INVALID_HANDLE_VALUE)
        FindClose(findFileHandle);
}

bool QFileSystemIterator::advance(QFileSystemEntry &fileEntry, QFileSystemMetaData &metaData)
{
    bool haveData = false;
    WIN32_FIND_DATA findData;

    if (findFileHandle == INVALID_HANDLE_VALUE && !uncFallback) {
        haveData = true;
        int infoLevel = 0 ;         // FindExInfoStandard;
        DWORD dwAdditionalFlags  = 0;
        if (QOperatingSystemVersion::current() >= QOperatingSystemVersion::Windows7) {
            dwAdditionalFlags = 2;  // FIND_FIRST_EX_LARGE_FETCH
            infoLevel = 1 ;         // FindExInfoBasic;
        }
        int searchOps =  0;         // FindExSearchNameMatch
        if (onlyDirs)
            searchOps = 1 ;         // FindExSearchLimitToDirectories
        findFileHandle = FindFirstFileEx((const wchar_t *)nativePath.utf16(), FINDEX_INFO_LEVELS(infoLevel), &findData,
                                         FINDEX_SEARCH_OPS(searchOps), 0, dwAdditionalFlags);
        if (findFileHandle == INVALID_HANDLE_VALUE) {
            if (nativePath.startsWith(QLatin1String("\\\\?\\UNC\\"))) {
                const QVector<QStringRef> parts = nativePath.splitRef(QLatin1Char('\\'), QString::SkipEmptyParts);
                if (parts.count() == 4 && QFileSystemEngine::uncListSharesOnServer(
                        QLatin1String("\\\\") + parts.at(2), &uncShares)) {
                    if (uncShares.isEmpty())
                        return false; // No shares found in the server
                    else
                        uncFallback = true;
                }
            }
        }
    }
    if (findFileHandle == INVALID_HANDLE_VALUE && !uncFallback)
        return false;
    // Retrieve the new file information.
    if (!haveData) {
        if (uncFallback) {
            if (++uncShareIndex >= uncShares.count())
                return false;
        } else {
            if (!FindNextFile(findFileHandle, &findData))
                return false;
        }
    }
    // Create the new file system entry & meta data.
    if (uncFallback) {
        fileEntry = QFileSystemEntry(dirPath + uncShares.at(uncShareIndex));
        metaData.fillFromFileAttribute(FILE_ATTRIBUTE_DIRECTORY);
        return true;
    } else {
        QString fileName = QString::fromWCharArray(findData.cFileName);
        fileEntry = QFileSystemEntry(dirPath + fileName);
        metaData = QFileSystemMetaData();
        if (!fileName.endsWith(QLatin1String(".lnk"))) {
            metaData.fillFromFindData(findData, true);
        }
        return true;
    }
    return false;
}

QT_END_NAMESPACE
