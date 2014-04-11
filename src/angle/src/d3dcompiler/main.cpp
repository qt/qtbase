/****************************************************************************
**
** Copyright (C) 2013 Digia Plc and/or its subsidiary(-ies).
** Contact: http://www.qt-project.org/legal
**
** This file is part of the plugins of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:LGPL$
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and Digia.  For licensing terms and
** conditions see http://qt.digia.com/licensing.  For further information
** use the contact form at http://qt.digia.com/contact-us.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 2.1 as published by the Free Software
** Foundation and appearing in the file LICENSE.LGPL included in the
** packaging of this file.  Please review the following information to
** ensure the GNU Lesser General Public License version 2.1 requirements
** will be met: http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, Digia gives you certain additional
** rights.  These rights are described in the Digia Qt LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
** GNU General Public License Usage
** Alternatively, this file may be used under the terms of the GNU
** General Public License version 3.0 as published by the Free Software
** Foundation and appearing in the file LICENSE.GPL included in the
** packaging of this file.  Please review the following information to
** ensure the GNU General Public License version 3.0 requirements will be
** met: http://www.gnu.org/copyleft/gpl.html.
**
**
** $QT_END_LICENSE$
**
****************************************************************************/

#include <QByteArray>
#include <QCryptographicHash>
#include <QDateTime>
#include <QDir>
#include <QElapsedTimer>
#include <QFile>
#include <QLoggingCategory>
#include <QStandardPaths>
#include <QThread>

#include <qt_windows.h>
#include <d3dcommon.h>

#ifdef D3DCOMPILER_LINKED
namespace D3D {
#  include <d3dcompiler.h>
}
#endif // D3DCOMPILER_LINKED

Q_LOGGING_CATEGORY(QT_D3DCOMPILER, "qt.angle.d3dcompiler")

namespace D3DCompiler {

typedef HRESULT (WINAPI *D3DCompileFunc)(const void *data, SIZE_T data_size, const char *filename,
                                         const D3D_SHADER_MACRO *defines, ID3DInclude *include, const char *entrypoint,
                                         const char *target, UINT sflags, UINT eflags, ID3DBlob **shader, ID3DBlob **error_messages);
static D3DCompileFunc compile;

class Blob : public ID3DBlob
{
public:
    Blob(const QByteArray &data) : m_data(data)
    {
        IIDFromString(L"00000000-0000-0000-C000-000000000046", &IID_IUnknown);
        IIDFromString(L"8BA5FB08-5195-40e2-AC58-0D989C3A0102", &IID_ID3DBlob);
    }

    virtual ~Blob()
    {
    }

    // ID3DBlob
    LPVOID __stdcall GetBufferPointer()
    {
        return m_data.data();
    }

    SIZE_T __stdcall GetBufferSize()
    {
        return m_data.size();
    }

    // IUnknown
    HRESULT __stdcall QueryInterface(REFIID riid, void **ppv)
    {
        IUnknown *out = 0;
        if (riid == IID_IUnknown)
            out = static_cast<IUnknown*>(this);
        else if (riid == IID_ID3DBlob)
            out = this;

        *ppv = out;
        if (!out)
            return E_NOINTERFACE;

        out->AddRef();
        return S_OK;
    }

    ULONG __stdcall AddRef()
    {
        return ++m_ref;
    }

    ULONG __stdcall Release()
    {
        ULONG ref = --m_ref;
        if (!m_ref)
            delete this;

        return ref;
    }

private:
    QByteArray m_data;
    ULONG m_ref;

    // These symbols might be missing, so define them here
    IID IID_IUnknown;
    IID IID_ID3DBlob;
};

static bool loadCompiler()
{
#ifndef D3DCOMPILER_LINKED
    static HMODULE d3dcompiler = 0;
    if (!d3dcompiler) {
        const wchar_t *dllNames[] = {
            L"d3dcompiler_47.dll",
            L"d3dcompiler_46.dll",
            L"d3dcompiler_45.dll",
            L"d3dcompiler_44.dll",
            L"d3dcompiler_43.dll",
            0
        };
        for (int i = 0; const wchar_t *name = dllNames[i]; ++i) {
#ifndef Q_OS_WINRT
            d3dcompiler = LoadLibrary(name);
#else
            d3dcompiler = LoadPackagedLibrary(name, NULL);
#endif
            if (d3dcompiler) {
                qCDebug(QT_D3DCOMPILER) << "Found" << QString::fromWCharArray(name);
                D3DCompiler::compile = reinterpret_cast<D3DCompiler::D3DCompileFunc>(GetProcAddress(d3dcompiler, "D3DCompile"));
                if (D3DCompiler::compile) {
                    qCDebug(QT_D3DCOMPILER) << "Loaded" << QString::fromWCharArray(name);
                    break;
                }
                qCDebug(QT_D3DCOMPILER) << "Failed to load" << QString::fromWCharArray(name);
            }
        }

        if (!d3dcompiler)
            qCDebug(QT_D3DCOMPILER) << "Unable to load D3D shader compiler.";
    }
#else // !D3DCOMPILER_LINKED
    compile = &D3D::D3DCompile;
#endif // D3DCOMPILER_LINKED
    return bool(compile);
}

static QString cacheKeyFor(const void *data)
{
    return QString::fromUtf8(QCryptographicHash::hash(reinterpret_cast<const char *>(data), QCryptographicHash::Sha1).toHex());
}

} // namespace D3DCompiler

#ifdef __MINGW32__
extern "C"
#endif
__declspec(dllexport) HRESULT WINAPI D3DCompile(
        const void *, SIZE_T, const char *, const D3D_SHADER_MACRO *, ID3DInclude *,
        const char *, const char *, UINT, UINT, ID3DBlob **, ID3DBlob **);

HRESULT WINAPI D3DCompile(
        const void *data, SIZE_T data_size, const char *filename,
        const D3D_SHADER_MACRO *defines, ID3DInclude *include, const char *entrypoint,
        const char *target, UINT sflags, UINT eflags, ID3DBlob **shader, ID3DBlob **errorMsgs)
{
    // Shortcut to compile using the runtime compiler if it is available
    static bool compilerAvailable =
            !qgetenv("QT_D3DCOMPILER_DISABLE_DLL").toInt() && D3DCompiler::loadCompiler();
    if (compilerAvailable) {
        HRESULT hr = D3DCompiler::compile(data, data_size, filename, defines, include, entrypoint,
                                          target, sflags, eflags, shader, errorMsgs);
        return hr;
    }

    static bool initialized = false;
    static QStringList binaryPaths;
    static QString sourcePath;
    if (!initialized) {
        // Precompiled directory
        QString precompiledPath;
        if (qEnvironmentVariableIsSet("QT_D3DCOMPILER_BINARY_DIR"))
            precompiledPath = QString::fromLocal8Bit(qgetenv("QT_D3DCOMPILER_BINARY_DIR"));
        else
            precompiledPath = QStringLiteral(":/qt.d3dcompiler/"); // Default QRC path
        if (QDir(precompiledPath).exists())
            binaryPaths.append(precompiledPath);

        // Service directory
        QString base;
        if (qEnvironmentVariableIsSet("QT_D3DCOMPILER_DIR"))
            base = QString::fromLocal8Bit(qgetenv("QT_D3DCOMPILER_DIR"));

        if (base.isEmpty()) {
            const QString location = QStandardPaths::writableLocation(QStandardPaths::DataLocation);
            if (!location.isEmpty())
                base = location + QStringLiteral("/d3dcompiler");
        }

        // Create the directory structure
        QDir baseDir(base);
        if (!baseDir.exists()) {
            baseDir.cdUp();
            if (!baseDir.mkdir(QStringLiteral("d3dcompiler"))) {
                qCWarning(QT_D3DCOMPILER) << "Unable to create shader base directory:"
                                          << QDir::toNativeSeparators(baseDir.absolutePath());
                if (binaryPaths.isEmpty()) // No possibility to get a shader, abort
                    return E_FAIL;
            }
            baseDir.cd(QStringLiteral("d3dcompiler"));
        }

        if (!baseDir.exists(QStringLiteral("binary")) && !baseDir.mkdir(QStringLiteral("binary"))) {
            qCWarning(QT_D3DCOMPILER) << "Unable to create shader binary directory:"
                                      << QDir::toNativeSeparators(baseDir.absoluteFilePath(QStringLiteral("binary")));
            if (binaryPaths.isEmpty()) // No possibility to get a shader, abort
                return E_FAIL;
        } else {
            binaryPaths.append(baseDir.absoluteFilePath(QStringLiteral("binary/")));
        }

        if (!baseDir.exists(QStringLiteral("source")) && !baseDir.mkdir(QStringLiteral("source"))) {
            qCWarning(QT_D3DCOMPILER) << "Unable to create shader source directory:"
                                      << QDir::toNativeSeparators(baseDir.absoluteFilePath(QStringLiteral("source")));
        } else {
            sourcePath = baseDir.absoluteFilePath(QStringLiteral("source/"));
        }

        initialized = true;
    }

    QByteArray macros;
    if (const D3D_SHADER_MACRO *macro = defines) {
        while (macro) {
            macros.append('#').append(macro->Name).append(' ').append(macro->Definition).append('\n');
            ++macro;
        }
    }

    const QByteArray sourceData = macros + QByteArray::fromRawData(reinterpret_cast<const char *>(data), data_size);
    const QString fileName = D3DCompiler::cacheKeyFor(sourceData)
            + QLatin1Char('!') + QString::fromUtf8(entrypoint)
            + QLatin1Char('!') + QString::fromUtf8(target)
            + QLatin1Char('!') + QString::number(sflags);

    // Check if pre-compiled shader blob is available
    foreach (const QString &path, binaryPaths) {
        QString blobName = path + fileName;
        QFile blob(blobName);
        while (!blob.exists()) {
            // Progressively drop optional parts
            blobName.truncate(blobName.lastIndexOf(QLatin1Char('!')));
            if (blobName.isEmpty())
                break;
            blob.setFileName(blobName);
        }
        if (blob.exists()) {
            if (blob.open(QFile::ReadOnly)) {
                qCDebug(QT_D3DCOMPILER) << "Opening precompiled shader blob at" << blob.fileName();
                *shader = new D3DCompiler::Blob(blob.readAll());
                return S_FALSE;
            }
            qCDebug(QT_D3DCOMPILER) << "Found, but unable to open, precompiled shader blob at" << blob.fileName();
            break;
        }
    }

    // Shader blob is not available; write out shader source
    if (!sourcePath.isEmpty()) {
        // Dump source to source path; wait for blob to appear
        QFile source(sourcePath + fileName);
        if (!source.open(QFile::WriteOnly)) {
            qCDebug(QT_D3DCOMPILER) << "Unable to write shader source to file:" << source.fileName() << source.errorString();
            return E_ACCESSDENIED;
        }

        source.write(sourceData);
        qCDebug(QT_D3DCOMPILER) << "Wrote shader source, waiting for blob:" << source.fileName();
        source.close();

        qint64 timeout = qgetenv("QT_D3DCOMPILER_TIMEOUT").toLong();
        if (!timeout)
            timeout = 3000;

        QElapsedTimer timer;
        timer.start();
        QFile blob(binaryPaths.last() + fileName);
        while (!(blob.exists() && blob.open(QFile::ReadOnly)) && timer.elapsed() < timeout)
            QThread::msleep(100);

        if (blob.isOpen()) {
            *shader = new D3DCompiler::Blob(blob.readAll());
            return S_FALSE;
        }

        qCDebug(QT_D3DCOMPILER) << "Shader blob failed to materialize after" << timeout << "ms.";
        *errorMsgs = new D3DCompiler::Blob("Shader compilation timeout.");
        return E_ABORT;
    }

    *errorMsgs = new D3DCompiler::Blob("No shader compiler or service could be found.");
    return E_FAIL;
}
