/****************************************************************************
**
** Copyright (C) 2016 The Qt Company Ltd.
** Contact: https://www.qt.io/licensing/
**
** This file is part of the QtGui module of the Qt Toolkit.
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

#include "qopenglprogrambinarycache_p.h"
#include <QOpenGLContext>
#include <QOpenGLExtraFunctions>
#include <QStandardPaths>
#include <QDir>
#include <QSaveFile>
#include <QLoggingCategory>

#ifdef Q_OS_UNIX
#include <sys/mman.h>
#include <private/qcore_unix_p.h>
#endif

QT_BEGIN_NAMESPACE

Q_DECLARE_LOGGING_CATEGORY(DBG_SHADER_CACHE)

#ifndef GL_PROGRAM_BINARY_LENGTH
#define GL_PROGRAM_BINARY_LENGTH          0x8741
#endif

const quint32 BINSHADER_MAGIC = 0x5174;
const quint32 BINSHADER_VERSION = 0x2;
const quint32 BINSHADER_QTVERSION = QT_VERSION;

struct GLEnvInfo
{
    GLEnvInfo();

    QByteArray glvendor;
    QByteArray glrenderer;
    QByteArray glversion;
};

GLEnvInfo::GLEnvInfo()
{
    QOpenGLContext *ctx = QOpenGLContext::currentContext();
    Q_ASSERT(ctx);
    QOpenGLFunctions *f = ctx->functions();
    const char *vendor = reinterpret_cast<const char *>(f->glGetString(GL_VENDOR));
    const char *renderer = reinterpret_cast<const char *>(f->glGetString(GL_RENDERER));
    const char *version = reinterpret_cast<const char *>(f->glGetString(GL_VERSION));
    if (vendor)
        glvendor = QByteArray(vendor);
    if (renderer)
        glrenderer = QByteArray(renderer);
    if (version)
        glversion = QByteArray(version);
}

static inline bool qt_ensureWritableDir(const QString &name)
{
    QDir::root().mkpath(name);
    return QFileInfo(name).isWritable();
}

QOpenGLProgramBinaryCache::QOpenGLProgramBinaryCache()
    : m_cacheWritable(false)
{
    const QString subPath = QLatin1String("/qtshadercache/");
    const QString sharedCachePath = QStandardPaths::writableLocation(QStandardPaths::GenericCacheLocation);
    if (!sharedCachePath.isEmpty()) {
        m_cacheDir = sharedCachePath + subPath;
        m_cacheWritable = qt_ensureWritableDir(m_cacheDir);
    }
    if (!m_cacheWritable) {
        m_cacheDir = QStandardPaths::writableLocation(QStandardPaths::CacheLocation) + subPath;
        m_cacheWritable = qt_ensureWritableDir(m_cacheDir);
    }
    qCDebug(DBG_SHADER_CACHE, "Cache location '%s' writable = %d", qPrintable(m_cacheDir), m_cacheWritable);
}

QString QOpenGLProgramBinaryCache::cacheFileName(const QByteArray &cacheKey) const
{
    return m_cacheDir + QString::fromUtf8(cacheKey);
}

#define BASE_HEADER_SIZE (int(3 * sizeof(quint32)))
#define FULL_HEADER_SIZE(stringsSize) (BASE_HEADER_SIZE + 12 + stringsSize + 8)
#define PADDING_SIZE(fullHeaderSize) (((fullHeaderSize + 3) & ~3) - fullHeaderSize)

static inline quint32 readUInt(const uchar **p)
{
    quint32 v;
    memcpy(&v, *p, sizeof(quint32));
    *p += sizeof(quint32);
    return v;
}

static inline QByteArray readStr(const uchar **p)
{
    quint32 len = readUInt(p);
    QByteArray ba = QByteArray::fromRawData(reinterpret_cast<const char *>(*p), len);
    *p += len;
    return ba;
}

bool QOpenGLProgramBinaryCache::verifyHeader(const QByteArray &buf) const
{
    if (buf.size() < BASE_HEADER_SIZE) {
        qCDebug(DBG_SHADER_CACHE, "Cached size too small");
        return false;
    }
    const uchar *p = reinterpret_cast<const uchar *>(buf.constData());
    if (readUInt(&p) != BINSHADER_MAGIC) {
        qCDebug(DBG_SHADER_CACHE, "Magic does not match");
        return false;
    }
    if (readUInt(&p) != BINSHADER_VERSION) {
        qCDebug(DBG_SHADER_CACHE, "Version does not match");
        return false;
    }
    if (readUInt(&p) != BINSHADER_QTVERSION) {
        qCDebug(DBG_SHADER_CACHE, "Qt version does not match");
        return false;
    }
    return true;
}

bool QOpenGLProgramBinaryCache::setProgramBinary(uint programId, uint blobFormat, const void *p, uint blobSize)
{
    QOpenGLExtraFunctions *funcs = QOpenGLContext::currentContext()->extraFunctions();
    while (funcs->glGetError() != GL_NO_ERROR) { }
    funcs->glProgramBinary(programId, blobFormat, p, blobSize);

    GLenum err = funcs->glGetError();
    if (err != GL_NO_ERROR) {
        qCDebug(DBG_SHADER_CACHE, "Program binary failed to load for program %u, size %d, "
                                  "format 0x%x, err = 0x%x",
                programId, blobSize, blobFormat, err);
        return false;
    }
    GLint linkStatus = 0;
    funcs->glGetProgramiv(programId, GL_LINK_STATUS, &linkStatus);
    if (linkStatus != GL_TRUE) {
        qCDebug(DBG_SHADER_CACHE, "Program binary failed to load for program %u, size %d, "
                                  "format 0x%x, linkStatus = 0x%x, err = 0x%x",
                programId, blobSize, blobFormat, linkStatus, err);
        return false;
    }

    qCDebug(DBG_SHADER_CACHE, "Program binary set for program %u, size %d, format 0x%x, err = 0x%x",
            programId, blobSize, blobFormat, err);
    return true;
}

#ifdef Q_OS_UNIX
class FdWrapper
{
public:
    FdWrapper(const QString &fn)
        : ptr(MAP_FAILED)
    {
        fd = qt_safe_open(QFile::encodeName(fn).constData(), O_RDONLY);
    }
    ~FdWrapper()
    {
        if (ptr != MAP_FAILED)
            munmap(ptr, mapSize);
        if (fd != -1)
            qt_safe_close(fd);
    }
    bool map()
    {
        off_t offs = lseek(fd, 0, SEEK_END);
        if (offs == (off_t) -1) {
            qErrnoWarning(errno, "lseek failed for program binary");
            return false;
        }
        mapSize = static_cast<size_t>(offs);
        ptr = mmap(nullptr, mapSize, PROT_READ, MAP_SHARED, fd, 0);
        return ptr != MAP_FAILED;
    }

    int fd;
    void *ptr;
    size_t mapSize;
};
#endif

class DeferredFileRemove
{
public:
    DeferredFileRemove(const QString &fn)
        : fn(fn),
          active(false)
    {
    }
    ~DeferredFileRemove()
    {
        if (active)
            QFile(fn).remove();
    }
    void setActive()
    {
        active = true;
    }

    QString fn;
    bool active;
};

bool QOpenGLProgramBinaryCache::load(const QByteArray &cacheKey, uint programId)
{
    if (m_memCache.contains(cacheKey)) {
        const MemCacheEntry *e = m_memCache[cacheKey];
        return setProgramBinary(programId, e->format, e->blob.constData(), e->blob.size());
    }

    QByteArray buf;
    const QString fn = cacheFileName(cacheKey);
    DeferredFileRemove undertaker(fn);
#ifdef Q_OS_UNIX
    FdWrapper fdw(fn);
    if (fdw.fd == -1)
        return false;
    char header[BASE_HEADER_SIZE];
    qint64 bytesRead = qt_safe_read(fdw.fd, header, BASE_HEADER_SIZE);
    if (bytesRead == BASE_HEADER_SIZE)
        buf = QByteArray::fromRawData(header, BASE_HEADER_SIZE);
#else
    QFile f(fn);
    if (!f.open(QIODevice::ReadOnly))
        return false;
    buf = f.read(BASE_HEADER_SIZE);
#endif

    if (!verifyHeader(buf)) {
        undertaker.setActive();
        return false;
    }

    const uchar *p;
#ifdef Q_OS_UNIX
    if (!fdw.map()) {
        undertaker.setActive();
        return false;
    }
    p = static_cast<const uchar *>(fdw.ptr) + BASE_HEADER_SIZE;
#else
    buf = f.readAll();
    p = reinterpret_cast<const uchar *>(buf.constData());
#endif

    GLEnvInfo info;

    QByteArray vendor = readStr(&p);
    if (vendor != info.glvendor) {
        // readStr returns non-null terminated strings just pointing to inside
        // 'p' so must print these via the stream qCDebug and not constData().
        qCDebug(DBG_SHADER_CACHE) << "GL_VENDOR does not match" << vendor << info.glvendor;
        undertaker.setActive();
        return false;
    }
    QByteArray renderer = readStr(&p);
    if (renderer != info.glrenderer) {
        qCDebug(DBG_SHADER_CACHE) << "GL_RENDERER does not match" << renderer << info.glrenderer;
        undertaker.setActive();
        return false;
    }
    QByteArray version = readStr(&p);
    if (version != info.glversion) {
        qCDebug(DBG_SHADER_CACHE) <<  "GL_VERSION does not match" << version << info.glversion;
        undertaker.setActive();
        return false;
    }

    quint32 blobFormat = readUInt(&p);
    quint32 blobSize = readUInt(&p);

    p += PADDING_SIZE(FULL_HEADER_SIZE(vendor.size() + renderer.size() + version.size()));

    return setProgramBinary(programId, blobFormat, p, blobSize)
        && m_memCache.insert(cacheKey, new MemCacheEntry(p, blobSize, blobFormat));
}

static inline void writeUInt(uchar **p, quint32 value)
{
    memcpy(*p, &value, sizeof(quint32));
    *p += sizeof(quint32);
}

static inline void writeStr(uchar **p, const QByteArray &str)
{
    writeUInt(p, str.size());
    memcpy(*p, str.constData(), str.size());
    *p += str.size();
}

void QOpenGLProgramBinaryCache::save(const QByteArray &cacheKey, uint programId)
{
    if (!m_cacheWritable)
        return;

    GLEnvInfo info;

    QOpenGLExtraFunctions *funcs = QOpenGLContext::currentContext()->extraFunctions();
    GLint blobSize = 0;
    while (funcs->glGetError() != GL_NO_ERROR) { }
    funcs->glGetProgramiv(programId, GL_PROGRAM_BINARY_LENGTH, &blobSize);

    const int headerSize = FULL_HEADER_SIZE(info.glvendor.size() + info.glrenderer.size() + info.glversion.size());

    // Add padding to make the blob start 4-byte aligned in order to support
    // OpenGL implementations on ARM that choke on non-aligned pointers passed
    // to glProgramBinary.
    const int paddingSize = PADDING_SIZE(headerSize);

    const int totalSize = headerSize + paddingSize + blobSize;

    qCDebug(DBG_SHADER_CACHE, "Program binary is %d bytes, err = 0x%x, total %d", blobSize, funcs->glGetError(), totalSize);
    if (!blobSize)
        return;

    QByteArray blob(totalSize, Qt::Uninitialized);
    uchar *p = reinterpret_cast<uchar *>(blob.data());

    writeUInt(&p, BINSHADER_MAGIC);
    writeUInt(&p, BINSHADER_VERSION);
    writeUInt(&p, BINSHADER_QTVERSION);

    writeStr(&p, info.glvendor);
    writeStr(&p, info.glrenderer);
    writeStr(&p, info.glversion);

    quint32 blobFormat = 0;
    uchar *blobFormatPtr = p;
    writeUInt(&p, blobFormat);
    writeUInt(&p, blobSize);

    for (int i = 0; i < paddingSize; ++i)
        *p++ = 0;

    GLint outSize = 0;
    funcs->glGetProgramBinary(programId, blobSize, &outSize, &blobFormat, p);
    if (blobSize != outSize) {
        qCDebug(DBG_SHADER_CACHE, "glGetProgramBinary returned size %d instead of %d", outSize, blobSize);
        return;
    }

    writeUInt(&blobFormatPtr, blobFormat);

    QSaveFile f(cacheFileName(cacheKey));
    if (f.open(QIODevice::WriteOnly | QIODevice::Truncate)) {
        f.write(blob);
        if (!f.commit())
            qCDebug(DBG_SHADER_CACHE, "Failed to write %s to shader cache", qPrintable(f.fileName()));
    } else {
        qCDebug(DBG_SHADER_CACHE, "Failed to create %s in shader cache", qPrintable(f.fileName()));
    }
}

QT_END_NAMESPACE
