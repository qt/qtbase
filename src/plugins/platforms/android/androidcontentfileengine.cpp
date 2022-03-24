/****************************************************************************
**
** Copyright (C) 2019 Volker Krause <vkrause@kde.org>
** Copyright (C) 2021 The Qt Company Ltd.
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

#include <QtCore/qcoreapplication.h>
#include <QtCore/qjnienvironment.h>
#include <QtCore/qjniobject.h>

#include <QDebug>

using namespace QNativeInterface;

AndroidContentFileEngine::AndroidContentFileEngine(const QString &f)
    : m_file(f)
{
    setFileName(f);
}

bool AndroidContentFileEngine::open(QIODevice::OpenMode openMode,
                                    std::optional<QFile::Permissions> permissions)
{
    Q_UNUSED(permissions);
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

    m_pfd = QJniObject::callStaticObjectMethod("org/qtproject/qt/android/QtNative",
        "openParcelFdForContentUrl",
        "(Landroid/content/Context;Ljava/lang/String;Ljava/lang/String;)Landroid/os/ParcelFileDescriptor;",
        QAndroidApplication::context(),
        QJniObject::fromString(fileName(DefaultName)).object(),
        QJniObject::fromString(openModeStr).object());

    if (!m_pfd.isValid())
        return false;

    const auto fd = m_pfd.callMethod<jint>("getFd", "()I");

    if (fd < 0) {
        m_pfd.callMethod<void>("close", "()V");
        m_pfd = QJniObject();
        return false;
    }

    return QFSFileEngine::open(openMode, fd, QFile::DontCloseHandle);
}

bool AndroidContentFileEngine::close()
{
    if (m_pfd.isValid()) {
        m_pfd.callMethod<void>("close", "()V");
        m_pfd = QJniObject();
    }

    return QFSFileEngine::close();
}

qint64 AndroidContentFileEngine::size() const
{
    const jlong size = QJniObject::callStaticMethod<jlong>(
            "org/qtproject/qt/android/QtNative", "getSize",
            "(Landroid/content/Context;Ljava/lang/String;)J", QAndroidApplication::context(),
            QJniObject::fromString(fileName(DefaultName)).object());
    return (qint64)size;
}

AndroidContentFileEngine::FileFlags AndroidContentFileEngine::fileFlags(FileFlags type) const
{
    FileFlags commonFlags(ReadOwnerPerm|ReadUserPerm|ReadGroupPerm|ReadOtherPerm|ExistsFlag);
    FileFlags flags;
    const bool isDir = QJniObject::callStaticMethod<jboolean>(
            "org/qtproject/qt/android/QtNative", "checkIfDir",
            "(Landroid/content/Context;Ljava/lang/String;)Z", QAndroidApplication::context(),
            QJniObject::fromString(fileName(DefaultName)).object());
    // If it is a directory then we know it exists so there is no reason to explicitly check
    const bool exists = isDir ? true : QJniObject::callStaticMethod<jboolean>(
            "org/qtproject/qt/android/QtNative", "checkFileExists",
            "(Landroid/content/Context;Ljava/lang/String;)Z", QAndroidApplication::context(),
            QJniObject::fromString(fileName(DefaultName)).object());
    if (!exists && !isDir)
        return flags;
    if (isDir) {
        flags = DirectoryType | commonFlags;
    } else {
        flags = FileType | commonFlags;
        const bool writable = QJniObject::callStaticMethod<jboolean>(
            "org/qtproject/qt/android/QtNative", "checkIfWritable",
            "(Landroid/content/Context;Ljava/lang/String;)Z", QAndroidApplication::context(),
            QJniObject::fromString(fileName(DefaultName)).object());
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
        const bool isDir = QJniObject::callStaticMethod<jboolean>(
                             "org/qtproject/qt/android/QtNative", "checkIfDir",
                             "(Landroid/content/Context;Ljava/lang/String;)Z",
                             QAndroidApplication::context(),
                             QJniObject::fromString(path()).object());
        if (isDir) {
            QJniObject objArray = QJniObject::callStaticObjectMethod("org/qtproject/qt/android/QtNative",
                                           "listContentsFromTreeUri",
                                           "(Landroid/content/Context;Ljava/lang/String;)[Ljava/lang/String;",
                                           QAndroidApplication::context(),
                                           QJniObject::fromString(path()).object());
            if (objArray.isValid()) {
                QJniEnvironment env;
                const jsize length = env->GetArrayLength(objArray.object<jarray>());
                for (int i = 0; i != length; ++i) {
                    m_entries << QJniObject(env->GetObjectArrayElement(
                                objArray.object<jobjectArray>(), i)).toString();
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
