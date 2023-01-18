// Copyright (C) 2023 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include <QTest>
#include <QtTest>

#include <QLibraryInfo>
#include <QLatin1StringView>
#include <QDir>
#include <QFileInfo>
#include <QProcess>
#include <QCryptographicHash>

#include <array>

using namespace Qt::Literals::StringLiterals;

class tst_qt_cmake_create : public QObject
{
    Q_OBJECT

public:
    tst_qt_cmake_create();

private slots:
    void init();
    void initTestCase();
    void generatingCMakeLists_data();
    void generatingCMakeLists();

private:
    QString m_testWorkDir;
    QString m_shell;
    QString m_cmd;
};

tst_qt_cmake_create::tst_qt_cmake_create() : m_testWorkDir(qApp->applicationDirPath()) { }

void tst_qt_cmake_create::initTestCase()
{
    QString binpath = QLibraryInfo::path(QLibraryInfo::BinariesPath);
#ifdef Q_OS_WINDOWS
    m_shell = QString("cmd.exe");
    m_cmd = QString("%1/qt-cmake-create.bat").arg(binpath);
#else
    m_shell = QString("/bin/sh");
    m_cmd = QString("%1/qt-cmake-create").arg(binpath);
    QVERIFY(QFile::exists(m_shell));
#endif

    QVERIFY(QFile::exists(m_cmd));
}

void tst_qt_cmake_create::init()
{
    QFETCH(QString, projectDirPath);
    QDir workDir(m_testWorkDir);
    QString fullProjectPath = m_testWorkDir + '/' + projectDirPath;
    if (workDir.exists(fullProjectPath)) {
        QDir projectDir(projectDirPath);
        projectDir.removeRecursively();
    }
    workDir.mkdir(projectDirPath);
    auto testDataPath = QFINDTESTDATA("testdata"_L1 + '/' + projectDirPath);
    QVERIFY(QFile::exists(testDataPath));

    for (const auto &fileInfo : QDir(testDataPath).entryInfoList(QDir::Files)) {
        QVERIFY(QFile::copy(fileInfo.absoluteFilePath(),
                            fullProjectPath + '/' + fileInfo.fileName()));
    }
}

void tst_qt_cmake_create::generatingCMakeLists_data()
{
    QTest::addColumn<QString>("projectDirPath");
    QTest::addColumn<bool>("expectPass");
    QTest::addColumn<QString>("workDir");

    const std::array<QLatin1StringView, 5> expectPass = {
        "cpp"_L1, "proto"_L1, "qml"_L1, "qrc"_L1, "ui"_L1,
    };

    const std::array<QString, 5> workDirs = {
        m_testWorkDir, ""_L1, m_testWorkDir, ""_L1, m_testWorkDir,
    };

    static_assert(expectPass.size() == workDirs.size());

    const QLatin1StringView expectFail[] = {
        "ui_only"_L1,
    };

    for (size_t i = 0; i < expectPass.size(); ++i) {
        const auto type = expectPass.at(i);
        QTest::addRow("tst_qt_cmake_create_%s", type.data())
                << QString("%1_project").arg(type) << true << workDirs.at(i);
    }

    for (const auto type : expectFail) {
        QTest::addRow("tst_qt_cmake_create_%s", type.data())
                << QString("%1_project").arg(type) << false << QString();
    }
}

void tst_qt_cmake_create::generatingCMakeLists()
{
    QFETCH(QString, projectDirPath);
    QFETCH(bool, expectPass);
    QFETCH(QString, workDir);

    QString fullProjectPath = m_testWorkDir + '/' + projectDirPath;
    QProcess command;
    QStringList arguments = {
#ifdef Q_OS_WINDOWS
        "/C"_L1,
#endif
        m_cmd
    };

    QString workingDirectory = fullProjectPath;
    if (!workDir.isEmpty()) {
        workingDirectory = workDir;
        arguments.append(fullProjectPath);
    }
    command.setProgram(m_shell);
    command.setArguments(arguments);
    command.setWorkingDirectory(workingDirectory);

    command.start();
    QVERIFY(command.waitForFinished());
    QCOMPARE(command.exitCode() == 0, expectPass);

    QFile actualFile = QFile(fullProjectPath + '/' + "CMakeLists.txt"_L1);

    // Skip the rest if we expect that qt-cmake-create should exit with error
    if (!expectPass) {
        QVERIFY(!actualFile.exists());
        return;
    }

    QFile expectedFile = QFile(fullProjectPath + '/' + "CMakeLists.txt.expected"_L1);
    QVERIFY(actualFile.open(QFile::ReadOnly));
    QVERIFY(expectedFile.open(QFile::ReadOnly));

    auto actualData = actualFile.readAll();
    actualData.replace(QByteArrayView("\r\n"), QByteArrayView("\n"));
    auto expectedData = expectedFile.readAll();
    expectedData.replace(QByteArrayView("\r\n"), QByteArrayView("\n"));

    static auto hash = [](const QByteArray &data) {
        return QCryptographicHash::hash(data, QCryptographicHash::Sha1).toHex();
    };
    QCOMPARE_EQ(hash(actualData), hash(expectedData));
}

QTEST_MAIN(tst_qt_cmake_create)
#include "tst_qt_cmake_create.moc"
