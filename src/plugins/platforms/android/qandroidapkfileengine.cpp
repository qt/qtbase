// Copyright (C) 2024 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only
// Qt-Security score:critical reason:file-handling

#include "qandroidapkfileengine.h"

#include <QtCore/QCoreApplication>
#include <QtCore/QJniEnvironment>
#include <QtCore/QReadWriteLock>

#include <private/qjnihelpers_p.h>

QT_BEGIN_NAMESPACE

Q_DECLARE_JNI_CLASS(JFileInfo, "org/qtproject/qt/android/JFileInfo")
Q_DECLARE_JNI_CLASS(MappedByteBuffer, "java/nio/MappedByteBuffer")

using namespace Qt::StringLiterals;
using namespace QtJniTypes;
using namespace QNativeInterface;

typedef QList<QAndroidApkFileEngine::FileInfo> ApkFileInfos;
namespace {
struct ApkFileInfosGlobalData
{
    QReadWriteLock apkInfosLock;
    ApkFileInfos apkFileInfos;
};
}
Q_GLOBAL_STATIC(ApkFileInfosGlobalData, g_apkFileInfosGlobal)

static ApkFileInfos *apkFileInfos()
{
    {
        QReadLocker lock(&g_apkFileInfosGlobal->apkInfosLock);
        if (!g_apkFileInfosGlobal->apkFileInfos.isEmpty())
            return &g_apkFileInfosGlobal->apkFileInfos;
    }

    QWriteLocker lock(&g_apkFileInfosGlobal->apkInfosLock);
    ArrayList arrayList = QtApkFileEngine::callStaticMethod<ArrayList>(
                "getApkFileInfos", QAndroidApkFileEngine::apkPath());

    for (int i = 0; i < arrayList.callMethod<int>("size"); ++i) {
        JFileInfo jInfo = arrayList.callMethod<jobject>("get", i);
        QAndroidApkFileEngine::FileInfo info;
        info.relativePath = jInfo.getField<QString>("relativePath");
        info.size = jInfo.getField<jlong>("size");
        info.isDir = jInfo.getField<jboolean>("isDir");
        g_apkFileInfosGlobal->apkFileInfos.append(info);
    }

    return &g_apkFileInfosGlobal->apkFileInfos;
}

QAndroidApkFileEngine::QAndroidApkFileEngine(const QString &fileName)
    : m_apkFileEngine(QAndroidApplication::context())
{
    setFileName(fileName);

    QString relativePath = QAndroidApkFileEngine::relativePath(m_fileName);
    for (QAndroidApkFileEngine::FileInfo &info : *apkFileInfos()) {
        if (info.relativePath == relativePath) {
            m_fileInfo = &info;
            break;
        }
    }
}

QAndroidApkFileEngine::~QAndroidApkFileEngine()
{
    close();
}

QString QAndroidApkFileEngine::apkPath()
{
    static QString apkPath = QtApkFileEngine::callStaticMethod<QString>("getAppApkFilePath");
    return apkPath;
}

QString QAndroidApkFileEngine::relativePath(const QString &filePath)
{
    const static int apkPathPrefixSize = apkPath().size() + 2;
    return filePath.right(filePath.size() - apkPathPrefixSize);
}

bool QAndroidApkFileEngine::open(QIODevice::OpenMode openMode,
                                 std::optional<QFile::Permissions> permissions)
{
    Q_UNUSED(permissions);

    if (!(openMode & QIODevice::ReadOnly))
        return false;

    if (!m_apkFileEngine.isValid() || !m_fileInfo)
        return false;

    if (m_fileInfo->relativePath.isEmpty())
        return false;

    return m_apkFileEngine.callMethod<bool>("open", m_fileInfo->relativePath);
}

bool QAndroidApkFileEngine::close()
{
    return m_apkFileEngine.isValid() ? m_apkFileEngine.callMethod<bool>("close") : false;
}

qint64 QAndroidApkFileEngine::size() const
{
    return m_fileInfo ? m_fileInfo->size : -1;
}

qint64 QAndroidApkFileEngine::pos() const
{
    return m_apkFileEngine.isValid() ? m_apkFileEngine.callMethod<jlong>("pos") : -1;
}

bool QAndroidApkFileEngine::seek(qint64 pos)
{
    return m_apkFileEngine.isValid() ? m_apkFileEngine.callMethod<bool>("seek", jint(pos)) : false;
}

qint64 QAndroidApkFileEngine::read(char *data, qint64 maxlen)
{
    if (!m_apkFileEngine.isValid())
        return -1;

    QJniArray<jbyte> byteArray = m_apkFileEngine.callMethod<jbyte[]>("read", jlong(maxlen));

    QJniEnvironment env;
    env->GetByteArrayRegion(byteArray.arrayObject(), 0, byteArray.size(), (jbyte*)data);

    if (env.checkAndClearExceptions())
        return -1;

    return byteArray.size();
}

QAbstractFileEngine::FileFlags QAndroidApkFileEngine::fileFlags(FileFlags type) const
{
    if (m_fileInfo) {
        FileFlags commonFlags(ReadOwnerPerm|ReadUserPerm|ReadGroupPerm|ReadOtherPerm|ExistsFlag);
        if (m_fileInfo->isDir)
            return type & (DirectoryType | commonFlags);
        else
            return type & (FileType | commonFlags);
    }

    return {};
}

QString QAndroidApkFileEngine::fileName(FileName file) const
{
    switch (file) {
    case PathName:
    case AbsolutePathName:
    case CanonicalPathName:
    case DefaultName:
    case AbsoluteName:
    case CanonicalName:
        return m_fileName;
    case BaseName:
        return m_fileName.mid(m_fileName.lastIndexOf(u'/') + 1);
    default:
        return QString();
    }
}

void QAndroidApkFileEngine::setFileName(const QString &file)
{
    m_fileName = file;
    if (m_fileName.endsWith(u'/'))
        m_fileName.chop(1);
}

uchar *QAndroidApkFileEngine::map(qint64 offset, qint64 size, QFileDevice::MemoryMapFlags flags)
{
    if (flags & QFile::MapPrivateOption) {
        qCritical() << "Mapping an in-APK file with private mode is not supported.";
        return nullptr;
    }
    if (!m_apkFileEngine.isValid())
        return nullptr;

    const MappedByteBuffer mappedBuffer = m_apkFileEngine.callMethod<MappedByteBuffer>(
                "getMappedByteBuffer", jlong(offset),  jlong(size));

    if (!mappedBuffer.isValid())
        return nullptr;

    void *address = QJniEnvironment::getJniEnv()->GetDirectBufferAddress(mappedBuffer.object());

    return const_cast<uchar *>(reinterpret_cast<const uchar *>(address));
}

bool QAndroidApkFileEngine::extension(Extension extension, const ExtensionOption *option,
                                      ExtensionReturn *output)
{
    if (extension == MapExtension) {
        const auto *options = static_cast<const MapExtensionOption *>(option);
        auto *returnValue = static_cast<MapExtensionReturn *>(output);
        returnValue->address = map(options->offset, options->size, options->flags);
        return (returnValue->address != nullptr);
    }
    return false;
}

bool QAndroidApkFileEngine::supportsExtension(Extension extension) const
{
    if (extension == MapExtension)
        return true;
    return false;
}

#ifndef QT_NO_FILESYSTEMITERATOR
QAbstractFileEngine::IteratorUniquePtr QAndroidApkFileEngine::beginEntryList(
        const QString &path, QDirListing::IteratorFlags filters, const QStringList &filterNames)
{
    return std::make_unique<QAndroidApkFileEngineIterator>(path, filters, filterNames);
}

QAndroidApkFileEngineIterator::QAndroidApkFileEngineIterator(
        const QString &path, QDirListing::IteratorFlags filters, const QStringList &filterNames)
    : QAbstractFileEngineIterator(path, filters, filterNames)
{
    const QString relativePath = QAndroidApkFileEngine::relativePath(path);
    for (QAndroidApkFileEngine::FileInfo &info : *apkFileInfos()) {
        if (info.relativePath.startsWith(relativePath))
            m_infos.append(&info);
    }
}

QAndroidApkFileEngineIterator::~QAndroidApkFileEngineIterator() { }

bool QAndroidApkFileEngineIterator::advance()
{
    if (!m_infos.isEmpty() && m_index < m_infos.size() - 1) {
        ++m_index;
        return true;
    }

    return false;
}

QString QAndroidApkFileEngineIterator::currentFileName() const
{
    return m_infos.at(m_index)->relativePath;
}

QString QAndroidApkFileEngineIterator::currentFilePath() const
{
    return QAndroidApkFileEngine::apkPath() + "!/" + currentFileName();
}
#endif

std::unique_ptr<QAbstractFileEngine>
QAndroidApkFileEngineHandler::create(const QString &fileName) const
{
    if (QtAndroidPrivate::resolveApkPath(fileName).isEmpty())
        return {};

    return std::make_unique<QAndroidApkFileEngine>(fileName);
}

QT_END_NAMESPACE
