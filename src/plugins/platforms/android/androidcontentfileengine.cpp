/****************************************************************************
**
** Copyright (C) 2019 Volker Krause <vkrause@kde.org>
** Contact: https://www.qt.io/licensing/
**
** This file is part of the plugins of the Qt Toolkit.
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

#include "androidcontentfileengine.h"

#include <private/qjni_p.h>
#include <private/qjnihelpers_p.h>

#include <QDebug>

AndroidContentFileEngine::AndroidContentFileEngine(const QString &f)
    : m_file(f)
{
    setFileName(f);
}

bool AndroidContentFileEngine::open(QIODevice::OpenMode openMode)
{
    QString openModeStr;
    if (openMode & QFileDevice::ReadOnly) {
        openModeStr += QLatin1Char('r');
    }
    if (openMode & QFileDevice::WriteOnly) {
        openModeStr += QLatin1Char('w');
    }
    if (openMode & QFileDevice::Truncate) {
        openModeStr += QLatin1Char('t');
    } else if (openMode & QFileDevice::Append) {
        openModeStr += QLatin1Char('a');
    }

    const auto fd = QJNIObjectPrivate::callStaticMethod<jint>("org/qtproject/qt5/android/QtNative",
        "openFdForContentUrl",
        "(Landroid/content/Context;Ljava/lang/String;Ljava/lang/String;)I",
        QtAndroidPrivate::context(),
        QJNIObjectPrivate::fromString(fileName(DefaultName)).object(),
        QJNIObjectPrivate::fromString(openModeStr).object());

    if (fd < 0) {
        return false;
    }

    return QFSFileEngine::open(openMode, fd, QFile::AutoCloseHandle);
}

qint64 AndroidContentFileEngine::size() const
{
    const jlong size = QJNIObjectPrivate::callStaticMethod<jlong>(
            "org/qtproject/qt5/android/QtNative", "getSize",
            "(Landroid/content/Context;Ljava/lang/String;)J", QtAndroidPrivate::context(),
            QJNIObjectPrivate::fromString(fileName(DefaultName)).object());
    return (qint64)size;
}

AndroidContentFileEngine::FileFlags AndroidContentFileEngine::fileFlags(FileFlags type) const
{
    FileFlags commonFlags(ReadOwnerPerm|ReadUserPerm|ReadGroupPerm|ReadOtherPerm|ExistsFlag);
    FileFlags flags;
    const bool isDir = QJNIObjectPrivate::callStaticMethod<jboolean>(
            "org/qtproject/qt5/android/QtNative", "checkIfDir",
            "(Landroid/content/Context;Ljava/lang/String;)Z", QtAndroidPrivate::context(),
            QJNIObjectPrivate::fromString(fileName(DefaultName)).object());
    // If it is a directory then we know it exists so there is no reason to explicitly check
    const bool exists = isDir ? true : QJNIObjectPrivate::callStaticMethod<jboolean>(
            "org/qtproject/qt5/android/QtNative", "checkFileExists",
            "(Landroid/content/Context;Ljava/lang/String;)Z", QtAndroidPrivate::context(),
            QJNIObjectPrivate::fromString(fileName(DefaultName)).object());
    if (!exists && !isDir)
        return flags;
    if (isDir) {
        flags = DirectoryType | commonFlags;
    } else {
        flags = FileType | commonFlags;
        const bool writable = QJNIObjectPrivate::callStaticMethod<jboolean>(
            "org/qtproject/qt5/android/QtNative", "checkIfWritable",
            "(Landroid/content/Context;Ljava/lang/String;)Z", QtAndroidPrivate::context(),
            QJNIObjectPrivate::fromString(fileName(DefaultName)).object());
        if (writable)
            flags |= WriteOwnerPerm|WriteUserPerm|WriteGroupPerm|WriteOtherPerm;
    }
    return type & flags;
}

QString AndroidContentFileEngine::fileName(FileName f) const
{
    switch (f) {
        case PathName:
        case AbsolutePathName:
        case CanonicalPathName:
        case DefaultName:
        case AbsoluteName:
        case CanonicalName:
            return m_file;
        case BaseName:
        {
            const int pos = m_file.lastIndexOf(QChar(QLatin1Char('/')));
            return m_file.mid(pos);
        }
        default:
            return QString();
    }
}

QAbstractFileEngine::Iterator *AndroidContentFileEngine::beginEntryList(QDir::Filters filters, const QStringList &filterNames)
{
    return new AndroidContentFileEngineIterator(filters, filterNames);
}

QAbstractFileEngine::Iterator *AndroidContentFileEngine::endEntryList()
{
    return nullptr;
}

AndroidContentFileEngineHandler::AndroidContentFileEngineHandler() = default;
AndroidContentFileEngineHandler::~AndroidContentFileEngineHandler() = default;

QAbstractFileEngine* AndroidContentFileEngineHandler::create(const QString &fileName) const
{
    if (!fileName.startsWith(QLatin1String("content"))) {
        return nullptr;
    }

    return new AndroidContentFileEngine(fileName);
}

AndroidContentFileEngineIterator::AndroidContentFileEngineIterator(QDir::Filters filters,
                                                                   const QStringList &filterNames)
    : QAbstractFileEngineIterator(filters, filterNames)
{
}

AndroidContentFileEngineIterator::~AndroidContentFileEngineIterator()
{
}

QString AndroidContentFileEngineIterator::next()
{
    if (!hasNext())
        return QString();
    ++m_index;
    return currentFilePath();
}

bool AndroidContentFileEngineIterator::hasNext() const
{
    if (m_index == -1) {
        if (path().isEmpty())
            return false;
        const bool isDir = QJNIObjectPrivate::callStaticMethod<jboolean>(
                             "org/qtproject/qt5/android/QtNative", "checkIfDir",
                             "(Landroid/content/Context;Ljava/lang/String;)Z",
                             QtAndroidPrivate::context(),
                             QJNIObjectPrivate::fromString(path()).object());
        if (isDir) {
            QJNIObjectPrivate objArray = QJNIObjectPrivate::callStaticObjectMethod("org/qtproject/qt5/android/QtNative",
                                           "listContentsFromTreeUri",
                                           "(Landroid/content/Context;Ljava/lang/String;)[Ljava/lang/String;",
                                           QtAndroidPrivate::context(),
                                           QJNIObjectPrivate::fromString(path()).object());
            if (objArray.isValid()) {
                QJNIEnvironmentPrivate env;
                const jsize length = env->GetArrayLength(static_cast<jarray>(objArray.object()));
                for (int i = 0; i != length; ++i) {
                    m_entries << QJNIObjectPrivate(env->GetObjectArrayElement(
                                static_cast<jobjectArray>(objArray.object()), i)).toString();
                }
            }
        }
        m_index = 0;
    }
    return m_index < m_entries.size();
}

QString AndroidContentFileEngineIterator::currentFileName() const
{
    if (m_index <= 0 || m_index > m_entries.size())
        return QString();
    return m_entries.at(m_index - 1);
}
