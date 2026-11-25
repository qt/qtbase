// Copyright (C) 2026 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only

#include <QtTest/QTest>

#include <QtCore/QDir>
#include <QtCore/QFile>
#include <QtCore/QLibraryInfo>
#include <QtCore/QProcess>
#include <QtCore/QTemporaryDir>

#include <filesystem>
#include <system_error>

using namespace Qt::StringLiterals;

// Exit codes of syncqt, see the ErrorCodes enum in src/tools/syncqt/main.cpp.
static constexpr int NoError = 0;
static constexpr int SyncFailed = 2;

// A module with sources in <root>/src and build output in <root>/build. Optionally, all paths
// passed to syncqt go through a symlink to the real root, like /tmp on macOS.
class FakeModule
{
public:
    explicit FakeModule(bool viaSymlink = false)
    {
        if (!m_tempDir.isValid())
            return;
        const QString realRoot = m_tempDir.filePath(u"real"_s);
        if (!QDir().mkpath(realRoot + u"/src"_s) || !QDir().mkpath(realRoot + u"/build"_s))
            return;
        m_root = realRoot;
        if (viaSymlink) {
            const QString link = m_tempDir.filePath(u"link"_s);
            std::error_code ec;
            std::filesystem::create_directory_symlink(QFile::encodeName(realRoot).toStdString(),
                                                      QFile::encodeName(link).toStdString(), ec);
            m_root = ec ? QString() : link;
        }
    }

    bool isValid() const { return !m_root.isEmpty(); }
    QString sourceDir() const { return m_root + u"/src"_s; }
    QString buildDir() const { return m_root + u"/build"_s; }
    QString includeDir() const { return buildDir() + u"/include/QtFoo"_s; }

    QString writeSource(const QString &fileName, const QByteArray &content) const
    {
        const QString path = sourceDir() + u'/' + fileName;
        QFile file(path);
        if (!file.open(QIODevice::WriteOnly) || file.write(content) != content.size())
            return {};
        return path;
    }

    QStringList arguments() const
    {
        const QString versioned = includeDir() + u"/6.13.0/QtFoo"_s;
        return {
            u"-module"_s, u"QtFoo"_s,
            u"-moduleVersion"_s, u"6.13.0"_s,
            u"-sourceDir"_s, sourceDir(),
            u"-binaryDir"_s, buildDir(),
            u"-privateHeadersFilter"_s, u".+_p(ch)?\\.h"_s,
            u"-includeDir"_s, includeDir(),
            u"-privateIncludeDir"_s, versioned + u"/private"_s,
            u"-qpaIncludeDir"_s, versioned + u"/qpa"_s,
            u"-rhiIncludeDir"_s, versioned + u"/rhi"_s,
            u"-ssgIncludeDir"_s, versioned + u"/ssg"_s,
            u"-spiIncludeDir"_s, versioned + u"/spi"_s,
            u"-stagingDir"_s, includeDir() + u"/.syncqt_staging"_s,
            u"-knownModules"_s, u"Core"_s,
        };
    }

private:
    QTemporaryDir m_tempDir;
    QString m_root;
};

class tst_Syncqt : public QObject
{
    Q_OBJECT

private slots:
    void initTestCase();
    void privateIncludeInPublicHeader();
    void privateIncludeInPrivateHeader();
    void generatedHeaderIncludesPrivateHeader_data();
    void generatedHeaderIncludesPrivateHeader();
    void missingNamespace();
    void pragmaOnce();

private:
    int runSyncqt(const QStringList &arguments, QByteArray *errorOutput = nullptr);

    QString m_syncqt;
};

void tst_Syncqt::initTestCase()
{
    m_syncqt = QLibraryInfo::path(QLibraryInfo::LibraryExecutablesPath) + u"/syncqt"_s;
#ifdef Q_OS_WIN
    m_syncqt += u".exe"_s;
#endif
    QVERIFY2(QFile::exists(m_syncqt), qPrintable(m_syncqt));
}

int tst_Syncqt::runSyncqt(const QStringList &arguments, QByteArray *errorOutput)
{
    QProcess process;
    process.start(m_syncqt, arguments);
    if (!process.waitForFinished(30000) || process.exitStatus() != QProcess::NormalExit) {
        qWarning() << "syncqt did not finish:" << process.errorString();
        return -1;
    }
    const QByteArray stderrOutput = process.readAllStandardError();
    if (errorOutput)
        *errorOutput = stderrOutput;
    else if (!stderrOutput.isEmpty())
        qDebug().noquote() << stderrOutput;
    return process.exitCode();
}

void tst_Syncqt::privateIncludeInPublicHeader()
{
    FakeModule module;
    QVERIFY(module.isValid());
    const QString header = module.writeSource(u"qbar.h"_s, R"(#include "qfoo_p.h"
QT_BEGIN_NAMESPACE
QT_END_NAMESPACE
)"_ba);
    QVERIFY(!header.isEmpty());

    QByteArray errorOutput;
    QCOMPARE(runSyncqt(module.arguments() << u"-headers"_s << header, &errorOutput), SyncFailed);
    QVERIFY2(errorOutput.contains("includes private header qfoo_p.h"), errorOutput);
}

void tst_Syncqt::privateIncludeInPrivateHeader()
{
    FakeModule module;
    QVERIFY(module.isValid());
    const QString header = module.writeSource(u"qbar_p.h"_s, R"(//  W A R N I N G
// We mean it.
#include "qfoo_p.h"
)"_ba);
    QVERIFY(!header.isEmpty());

    QCOMPARE(runSyncqt(module.arguments() << u"-headers"_s << header), NoError);
}

void tst_Syncqt::generatedHeaderIncludesPrivateHeader_data()
{
    QTest::addColumn<bool>("viaSymlink");
    QTest::addColumn<bool>("markAsGenerated");
    QTest::addColumn<int>("expectedExitCode");

    QTest::newRow("not-generated") << false << false << SyncFailed;
    QTest::newRow("generated") << false << true << NoError;
    QTest::newRow("generated-via-symlink") << true << true << NoError;
}

// Generated headers, like the D-Bus adaptors created with qdbusxml2cpp, may include private
// headers. They end up in the source directory in in-source builds. QTBUG-128571
void tst_Syncqt::generatedHeaderIncludesPrivateHeader()
{
    QFETCH(bool, viaSymlink);
    QFETCH(bool, markAsGenerated);
    QFETCH(int, expectedExitCode);

#ifdef Q_OS_WIN
    if (viaSymlink)
        QSKIP("Directory symlinks need extra privileges on Windows.");
#endif
    FakeModule module(viaSymlink);
    QVERIFY(module.isValid());
    const QString header = module.writeSource(u"bus_interface.h"_s, R"(#include "qfoo_p.h"
)"_ba);
    QVERIFY(!header.isEmpty());

    QStringList arguments = module.arguments() << u"-headers"_s << header;
    if (markAsGenerated)
        arguments << u"-generatedHeaders"_s << header;
    QByteArray errorOutput;
    QCOMPARE(runSyncqt(arguments, &errorOutput), expectedExitCode);
    QCOMPARE(errorOutput.contains("includes private header"), expectedExitCode != NoError);
}

void tst_Syncqt::missingNamespace()
{
    FakeModule module;
    QVERIFY(module.isValid());
    const QString header = module.writeSource(u"qbar.h"_s, "class QBar;\n"_ba);
    QVERIFY(!header.isEmpty());
    const QStringList arguments = module.arguments() << u"-headers"_s << header;

    QByteArray errorOutput;
    QCOMPARE(runSyncqt(arguments, &errorOutput), NoError);
    QVERIFY2(errorOutput.contains("does not include QT_BEGIN_NAMESPACE"), errorOutput);

    QCOMPARE(runSyncqt(arguments + QStringList{ u"-warningsAreErrors"_s }, &errorOutput),
             SyncFailed);
    QVERIFY2(errorOutput.contains("does not include QT_BEGIN_NAMESPACE"), errorOutput);
}

void tst_Syncqt::pragmaOnce()
{
    FakeModule module;
    QVERIFY(module.isValid());
    const QString header = module.writeSource(u"qbar.h"_s, R"(#pragma once
QT_BEGIN_NAMESPACE
QT_END_NAMESPACE
)"_ba);
    QVERIFY(!header.isEmpty());

    QByteArray errorOutput;
    QCOMPARE(runSyncqt(module.arguments() << u"-headers"_s << header, &errorOutput), SyncFailed);
    QVERIFY2(errorOutput.contains("\"#pragma once\" is not allowed"), errorOutput);
}

QTEST_GUILESS_MAIN(tst_Syncqt)
#include "tst_syncqt.moc"
