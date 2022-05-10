// Copyright (C) 2019 Volker Krause <vkrause@kde.org>
// Copyright (C) 2021 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#include "androidcontentfileengine.h"

#include <QtCore/qcoreapplication.h>
#include <QtCore/qjnienvironment.h>
#include <QtCore/qjniobject.h>

#include <QDebug>

using namespace QNativeInterface;
using namespace Qt::StringLiterals;

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
        openModeStr += u'r';
    }
    if (openMode & QFileDevice::WriteOnly) {
        openModeStr += u'w';
    }
    if (openMode & QFileDevice::Truncate) {
        openModeStr += u't';
    } else if (openMode & QFileDevice::Append) {
        openModeStr += u'a';
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
            const qsizetype pos = m_file.lastIndexOf(u'/');
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
    if (!fileName.startsWith("content"_L1))
        return nullptr;

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
