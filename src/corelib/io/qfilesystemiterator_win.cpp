// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#include "qfilesystemiterator_p.h"
#include "qfilesystemengine_p.h"
#include "qoperatingsystemversion.h"
#include "qplatformdefs.h"

#include <QtCore/qt_windows.h>

QT_BEGIN_NAMESPACE

using namespace Qt::StringLiterals;

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
    Q_UNUSED(nameFilters);
    Q_UNUSED(flags);
    if (nativePath.endsWith(u".lnk"_s) && !QFileSystemEngine::isDirPath(dirPath, nullptr)) {
        QFileSystemMetaData metaData;
        QFileSystemEntry link = QFileSystemEngine::getLinkTarget(entry, metaData);
        nativePath = link.nativeFilePath();
    }
    if (!nativePath.endsWith(u'\\'))
        nativePath.append(u'\\');
    nativePath.append(u'*');
    // In MSVC2015+ case we prepend //?/ for longer file-name support
    if (!dirPath.endsWith(u'/'))
        dirPath.append(u'/');
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
        dwAdditionalFlags = 2;  // FIND_FIRST_EX_LARGE_FETCH
        infoLevel = 1 ;         // FindExInfoBasic;
        int searchOps =  0;         // FindExSearchNameMatch
        if (onlyDirs)
            searchOps = 1 ;         // FindExSearchLimitToDirectories
        findFileHandle = FindFirstFileEx((const wchar_t *)nativePath.utf16(), FINDEX_INFO_LEVELS(infoLevel), &findData,
                                         FINDEX_SEARCH_OPS(searchOps), 0, dwAdditionalFlags);
        if (findFileHandle == INVALID_HANDLE_VALUE) {
            if (nativePath.startsWith("\\\\?\\UNC\\"_L1)) {
                const auto parts = QStringView{nativePath}.split(u'\\', Qt::SkipEmptyParts);
                if (parts.count() == 4 && QFileSystemEngine::uncListSharesOnServer(
                        "\\\\"_L1 + parts.at(2), &uncShares)) {
                    if (uncShares.isEmpty())
                        return false; // No shares found in the server
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
        if (!fileName.endsWith(".lnk"_L1)) {
            metaData.fillFromFindData(findData, true);
        }
        return true;
    }
    return false;
}

QT_END_NAMESPACE
