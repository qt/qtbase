/****************************************************************************
**
** Copyright (C) 2013 Digia Plc and/or its subsidiary(-ies).
** Contact: http://www.qt-project.org/legal
**
** This file is part of the test suite of the Qt Toolkit.
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

// This test verifies the behavior of d3dcompiler_qt, which is only built when ANGLE is enabled

#include <QCryptographicHash>
#include <QDir>
#include <QFuture>
#include <QObject>
#include <QStandardPaths>
#include <QTemporaryDir>
#include <QTest>
#include <QThread>

#if defined(Q_OS_WIN)

#include <d3dcommon.h>

#ifndef Q_OS_WINRT
#define loadLibrary(library) LoadLibrary(library)
#else
#define loadLibrary(library) LoadPackagedLibrary(library, NULL)
#endif

#ifdef D3DCOMPILER_DLL
#undef D3DCOMPILER_DLL
#endif

#ifdef QT_NO_DEBUG
#define D3DCOMPILER_DLL L"d3dcompiler_qt"
#else
#define D3DCOMPILER_DLL L"d3dcompiler_qtd"
#endif

#define getCompilerFunc(dll) reinterpret_cast<D3DCompileFunc>(GetProcAddress(dll, "D3DCompile"))

typedef HRESULT (WINAPI *D3DCompileFunc)(const void *data, SIZE_T data_size, const char *filename,
        const D3D_SHADER_MACRO *defines, ID3DInclude *include, const char *entrypoint,
        const char *target, UINT sflags, UINT eflags, ID3DBlob **shader, ID3DBlob **error_messages);

static const wchar_t *compilerDlls[] = {
    L"d3dcompiler_47.dll",
    L"d3dcompiler_46.dll",
    L"d3dcompiler_45.dll",
    L"d3dcompiler_44.dll",
    L"d3dcompiler_43.dll",
    0
};

static const char hlsl[] =
    "uniform SamplerState Sampler : register(s0);\n"
    "uniform Texture2D Texture : register(t0);\n"
    "float4 main(in float4 gl_Position : SV_POSITION, in float2 coord : TEXCOORD0) : SV_TARGET0\n"
    "{\n"
    "return Texture.Sample(Sampler, coord);\n"
    "}\n";

static inline QByteArray blobToByteArray(ID3DBlob *blob)
{
    return blob ? QByteArray::fromRawData(reinterpret_cast<const char *>(blob->GetBufferPointer()), blob->GetBufferSize()) : QByteArray();
}

class CompileRunner : public QThread
{
public:
    CompileRunner(D3DCompileFunc d3dCompile, const QByteArray &data, ID3DBlob **shader, ID3DBlob **error)
        : m_d3dCompile(d3dCompile), m_data(data), m_shader(shader), m_error(error)
    {
    }

    HRESULT result() const
    {
        return m_result;
    }

private:
    void run()
    {
        m_result = m_d3dCompile(m_data.constData(), m_data.size(), 0, 0, 0, "main", "ps_4_0", 0, 0, m_shader, m_error);
    }

    HRESULT m_result;
    D3DCompileFunc m_d3dCompile;
    QByteArray m_data;
    ID3DBlob **m_shader;
    ID3DBlob **m_error;
};

class tst_d3dcompiler : public QObject
{
    Q_OBJECT
private slots:
    void initTestCase();
    void init();
    void cleanup();
    void service_data();
    void service();
    void offlineCompile();
    void onlineCompile();

private:
    QString blobPath();

    HMODULE d3dcompiler_qt;
    HMODULE d3dcompiler_win;

    D3DCompileFunc d3dCompile;

    QTemporaryDir tempDir;
};

QString tst_d3dcompiler::blobPath()
{
    QDir path;
    if (qEnvironmentVariableIsSet("QT_D3DCOMPILER_DIR"))
        path.setPath(qgetenv("QT_D3DCOMPILER_DIR"));
    else
        path.setPath(QStandardPaths::writableLocation(QStandardPaths::DataLocation) + QStringLiteral("/d3dcompiler"));

    path.mkdir(QStringLiteral("binary"));
    path.mkdir(QStringLiteral("source"));

    return path.absolutePath();
}

void tst_d3dcompiler::initTestCase()
{
    QVERIFY(tempDir.isValid());
}

void tst_d3dcompiler::init()
{
    qunsetenv("QT_D3DCOMPILER_DIR");
    qunsetenv("QT_D3DCOMPILER_TIMEOUT");
    qunsetenv("QT_D3DCOMPILER_DISABLE_DLL");
}

void tst_d3dcompiler::cleanup()
{
    FreeLibrary(d3dcompiler_qt);
    FreeLibrary(d3dcompiler_win);

    QDir path(blobPath());
    foreach (const QString &entry, path.entryList(QStringList(), QDir::Files|QDir::NoDotAndDotDot))
        path.remove(entry);
    foreach (const QString &entry, path.entryList(QStringList(), QDir::Dirs|QDir::NoDotAndDotDot)) {
        QDir dir(path.absoluteFilePath(entry + QStringLiteral("/")));
        dir.removeRecursively();
    }
}

void tst_d3dcompiler::service_data()
{
    QTest::addColumn<QByteArray>("compilerDir");
    QTest::addColumn<bool>("exists");
    QTest::addColumn<HRESULT>("result");

    // Don't test the default case, as it would clutter the AppData directory
    //QTest::newRow("default") << QByteArrayLiteral("") << true << E_ABORT;
    QTest::newRow("temporary") << QFile::encodeName(tempDir.path()) << true << E_ABORT;
    QTest::newRow("invalid") << QByteArrayLiteral("ZZ:\\") << false << E_FAIL;
    QTest::newRow("empty") << QByteArrayLiteral("") << false << E_FAIL;
}

void tst_d3dcompiler::service()
{
    QFETCH(QByteArray, compilerDir);
    QFETCH(bool, exists);
    QFETCH(HRESULT, result);
    qputenv("QT_D3DCOMPILER_DIR", compilerDir);
    qputenv("QT_D3DCOMPILER_DISABLE_DLL", QByteArrayLiteral("1"));
    const QDir path = blobPath();

    if (exists) {
        // Activate service
        QVERIFY(path.exists());
    } else {
        QVERIFY(!path.exists());
    }

    // Run compiler (fast fail)
    d3dcompiler_qt = loadLibrary(D3DCOMPILER_DLL);
    QVERIFY(d3dcompiler_qt);
    d3dCompile = getCompilerFunc(d3dcompiler_qt);
    QVERIFY(d3dCompile);

    qputenv("QT_D3DCOMPILER_TIMEOUT", "1");
    const QByteArray data(hlsl);
    ID3DBlob *shader = 0, *errorMessage = 0;
    HRESULT hr = d3dCompile(data.constData(), data.size(), 0, 0, 0, "main", "ps_4_0", 0, 0, &shader, &errorMessage);
    QVERIFY2(hr == result, blobToByteArray(errorMessage));

    // Check that passthrough works
    if (hr == S_OK) {
        for (int i = 0; compilerDlls[i]; ++i) {
            d3dcompiler_win = loadLibrary(compilerDlls[i]);
            if (d3dcompiler_win)
                break;
        }
        QVERIFY(d3dcompiler_win);
        d3dCompile = getCompilerFunc(d3dcompiler_win);
        QVERIFY(d3dCompile);

        // Compile a shader to compare with
        ID3DBlob *reference = 0;
        HRESULT hr = d3dCompile(data.constData(), data.size(), 0, 0, 0, "main", "ps_4_0", 0, 0, &reference, &errorMessage);
        QVERIFY2(SUCCEEDED(hr), blobToByteArray(errorMessage));

        QByteArray shaderData(reinterpret_cast<const char *>(shader->GetBufferPointer()), shader->GetBufferSize());
        QByteArray referenceData(reinterpret_cast<const char *>(reference->GetBufferPointer()), reference->GetBufferSize());
        reference->Release();
        QCOMPARE(shaderData, referenceData);
    } else {
        QVERIFY(FAILED(hr));
    }

    if (shader)
        shader->Release();
}

void tst_d3dcompiler::offlineCompile()
{
    qputenv("QT_D3DCOMPILER_DIR", QFile::encodeName(tempDir.path()));
    qputenv("QT_D3DCOMPILER_DISABLE_DLL", QByteArrayLiteral("1"));

    for (int i = 0; compilerDlls[i]; ++i) {
        d3dcompiler_win = loadLibrary(compilerDlls[i]);
        if (d3dcompiler_win)
            break;
    }
    QVERIFY(d3dcompiler_win);
    d3dCompile = getCompilerFunc(d3dcompiler_win);
    QVERIFY(d3dCompile);

    // Compile a shader to place in binary directory
    const QByteArray data(hlsl);
    ID3DBlob *shader = 0, *errorMessage = 0;
    HRESULT hr = d3dCompile(data.constData(), data.size(), 0, 0, 0, "main", "ps_4_0", 0, 0, &shader, &errorMessage);
    QVERIFY2(SUCCEEDED(hr), blobToByteArray(errorMessage));
    QVERIFY(shader);

    QDir outputPath(blobPath());
    QVERIFY(outputPath.exists());
    QVERIFY(outputPath.exists(QStringLiteral("binary")));
    outputPath.cd(QStringLiteral("binary"));
    QFile output(outputPath.absoluteFilePath(QCryptographicHash::hash(data, QCryptographicHash::Sha1).toHex()));
    QVERIFY(output.open(QFile::WriteOnly));
    output.write(reinterpret_cast<const char *>(shader->GetBufferPointer()), shader->GetBufferSize());
    shader->Release();

    // Run compiler
    d3dcompiler_qt = loadLibrary(D3DCOMPILER_DLL);
    QVERIFY(d3dcompiler_qt);
    d3dCompile = getCompilerFunc(d3dcompiler_qt);
    QVERIFY(d3dCompile);

    hr = d3dCompile(data.constData(), data.size(), 0, 0, 0, "main", "ps_4_0", 0, 0, &shader, &errorMessage);
    // Returns S_FALSE if a cached shader was found
    QVERIFY2(hr == S_FALSE, blobToByteArray(errorMessage));
}

void tst_d3dcompiler::onlineCompile()
{
    qputenv("QT_D3DCOMPILER_DIR", QFile::encodeName(tempDir.path()));
    qputenv("QT_D3DCOMPILER_TIMEOUT", QByteArray::number(3000));
    qputenv("QT_D3DCOMPILER_DISABLE_DLL", QByteArrayLiteral("1"));

    QByteArray data(hlsl);

    const QDir path = blobPath();

    // Activate service
    QVERIFY(path.exists());

    d3dcompiler_qt = loadLibrary(D3DCOMPILER_DLL);
    QVERIFY(d3dcompiler_qt);
    D3DCompileFunc concurrentCompile = getCompilerFunc(d3dcompiler_qt);
    QVERIFY(d3dCompile);

    // Run async
    ID3DBlob *shader = 0, *errorMessage = 0;
    CompileRunner runner(concurrentCompile, data, &shader, &errorMessage);
    runner.start();

    // Wait for source to appear
    QVERIFY(path.exists());
    QVERIFY(path.exists(QStringLiteral("source")));
    QVERIFY(path.exists(QStringLiteral("binary")));

    const QString fileName = QCryptographicHash::hash(data, QCryptographicHash::Sha1).toHex()
            + QStringLiteral("!main!ps_4_0!0");
    QFile input(path.absoluteFilePath(QStringLiteral("source/") + fileName));
    QTRY_VERIFY_WITH_TIMEOUT(input.exists(), 3000);
    QTRY_VERIFY_WITH_TIMEOUT(input.isOpen() || input.open(QFile::ReadOnly), 1000);

    // Compile passed source
    const QByteArray inputData = input.readAll();
    for (int i = 0; compilerDlls[i]; ++i) {
        d3dcompiler_win = loadLibrary(compilerDlls[i]);
        if (d3dcompiler_win)
            break;
    }
    QVERIFY(d3dcompiler_win);
    d3dCompile = getCompilerFunc(d3dcompiler_win);
    QVERIFY(d3dCompile);
    ID3DBlob *reference = 0, *errorMessage2 = 0;
    HRESULT hr = d3dCompile(inputData.constData(), inputData.size(), 0, 0, 0, "main", "ps_4_0", 0, 0, &reference, &errorMessage2);
    QVERIFY2(SUCCEEDED(hr), blobToByteArray(errorMessage2));
    const QByteArray referenceData(reinterpret_cast<const char *>(reference->GetBufferPointer()), reference->GetBufferSize());
    reference->Release();

    // Write to output directory
    QFile output(path.absoluteFilePath(QStringLiteral("binary/") + fileName));
    QVERIFY(output.open(QFile::WriteOnly));
    output.write(referenceData);
    output.close();

    // All done
    QVERIFY(runner.wait(3000));
    hr = runner.result();
    QVERIFY2(hr == S_FALSE, blobToByteArray(errorMessage2));
    const QByteArray resultData(reinterpret_cast<const char *>(shader->GetBufferPointer()), shader->GetBufferSize());
    shader->Release();
    QVERIFY(referenceData == resultData);
}

QTEST_MAIN(tst_d3dcompiler)
#include "tst_d3dcompiler.moc"

#endif // Q_OS_WIN
