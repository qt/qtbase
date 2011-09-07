/****************************************************************************
**
** Copyright (C) 2011 Nokia Corporation and/or its subsidiary(-ies).
** All rights reserved.
** Contact: Nokia Corporation (qt-info@nokia.com)
**
** This file is part of the test suite of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:LGPL$
** GNU Lesser General Public License Usage
** This file may be used under the terms of the GNU Lesser General Public
** License version 2.1 as published by the Free Software Foundation and
** appearing in the file LICENSE.LGPL included in the packaging of this
** file. Please review the following information to ensure the GNU Lesser
** General Public License version 2.1 requirements will be met:
** http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, Nokia gives you certain additional
** rights. These rights are described in the Nokia Qt LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
** GNU General Public License Usage
** Alternatively, this file may be used under the terms of the GNU General
** Public License version 3.0 as published by the Free Software Foundation
** and appearing in the file LICENSE.GPL included in the packaging of this
** file. Please review the following information to ensure the GNU General
** Public License version 3.0 requirements will be met:
** http://www.gnu.org/copyleft/gpl.html.
**
** Other Usage
** Alternatively, this file may be used in accordance with the terms and
** conditions contained in a signed written agreement between you and Nokia.
**
**
**
**
**
** $QT_END_LICENSE$
**
****************************************************************************/

#include <QDir>
#include <QFile>
#include <QRegExp>
#include <QStringList>
#include <QTest>
#include <QSet>
#include <QProcess>
#include <QDebug>

enum FindSubdirsMode {
    Flat = 0,
    Recursive
};

class tst_MakeTestSelfTest: public QObject
{
    Q_OBJECT

private slots:
    void tests_pro_files();
    void tests_pro_files_data();

    void naming_convention();
    void naming_convention_data();

    void make_check();

private:
    QStringList find_subdirs(QString const&, FindSubdirsMode, QString const& = QString());

    QSet<QString> all_test_classes;
};

bool looks_like_testcase(QString const&,QString*);
bool looks_like_subdirs(QString const&);
QStringList find_test_class(QString const&);

/* Verify that all tests are listed somewhere in one of the autotest .pro files */
void tst_MakeTestSelfTest::tests_pro_files()
{
    static QStringList lines;

    if (lines.isEmpty()) {
        QDir dir(SRCDIR "/..");
        QStringList proFiles = dir.entryList(QStringList() << "*.pro");
        foreach (QString const& proFile, proFiles) {
            QString filename = QString("%1/../%2").arg(SRCDIR).arg(proFile);
            QFile file(filename);
            if (!file.open(QIODevice::ReadOnly)) {
                QFAIL(qPrintable(QString("open %1: %2").arg(filename).arg(file.errorString())));
            }
            while (!file.atEnd()) {
                lines << file.readLine().trimmed();
            }
        }
    }

    QFETCH(QString, subdir);
    QRegExp re(QString("( |=|^|#)%1( |\\\\|$)").arg(QRegExp::escape(subdir)));
    foreach (const QString& line, lines) {
        if (re.indexIn(line) != -1) {
            return;
        }
    }



    QFAIL(qPrintable(QString(
        "Subdir `%1' is missing from tests/auto/*.pro\n"
        "This means the test won't be compiled or run on any platform.\n"
        "If this is intentional, please put the test name in a comment in one of the .pro files.").arg(subdir))
    );

}

void tst_MakeTestSelfTest::tests_pro_files_data()
{
    QTest::addColumn<QString>("subdir");
    QDir dir(SRCDIR "/..");
    QStringList subdirs = dir.entryList(QDir::AllDirs|QDir::NoDotAndDotDot);

    foreach (const QString& subdir, subdirs) {
        if (subdir == QString::fromLatin1("tmp")
            || subdir.startsWith(".")
            || !dir.exists(subdir + "/" + subdir + ".pro"))
        {
            continue;
        }
        QTest::newRow(qPrintable(subdir)) << subdir;
    }
}

QString format_list(QStringList const& list)
{
    if (list.count() == 1) {
        return list.at(0);
    }
    return QString("one of (%1)").arg(list.join(", "));
}

void tst_MakeTestSelfTest::naming_convention()
{
    QFETCH(QString, subdir);
    QFETCH(QString, target);

    QDir dir(SRCDIR "/../" + subdir);

    QStringList cppfiles = dir.entryList(QStringList() << "*.h" << "*.cpp");
    if (cppfiles.isEmpty()) {
        // Common convention is to have test/test.pro and source files in parent dir
        if (dir.dirName() == "test") {
            dir.cdUp();
            cppfiles = dir.entryList(QStringList() << "*.h" << "*.cpp");
        }

        if (cppfiles.isEmpty()) {
            QSKIP("Couldn't locate source files for test", SkipSingle);
        }
    }

    QStringList possible_test_classes;
    foreach (QString const& file, cppfiles) {
        possible_test_classes << find_test_class(dir.path() + "/" + file);
    }

    if (possible_test_classes.isEmpty()) {
        QSKIP(qPrintable(QString("Couldn't locate test class in %1").arg(format_list(cppfiles))), SkipSingle);
    }

    QVERIFY2(possible_test_classes.contains(target), qPrintable(QString(
        "TARGET is %1, while test class appears to be %2.\n"
        "TARGET and test class _must_ match so that all testcase names can be accurately "
        "determined even if a test fails to compile or run.")
        .arg(target)
        .arg(format_list(possible_test_classes))
    ));

    QVERIFY2(!all_test_classes.contains(target), qPrintable(QString(
        "It looks like there are multiple tests named %1.\n"
        "This makes it impossible to separate results for these tests.\n"
        "Please ensure all tests are uniquely named.")
        .arg(target)
    ));

    all_test_classes << target;
}

void tst_MakeTestSelfTest::naming_convention_data()
{
    QTest::addColumn<QString>("subdir");
    QTest::addColumn<QString>("target");

    foreach (const QString& subdir, find_subdirs(SRCDIR "/../auto.pro", Recursive)) {
        if (QFileInfo(SRCDIR "/../" + subdir).isDir()) {
            QString target;
            if (looks_like_testcase(SRCDIR "/../" + subdir + "/" + QFileInfo(subdir).baseName() + ".pro", &target)) {
                QTest::newRow(qPrintable(subdir)) << subdir << target.toLower();
            }
        }
    }
}

/*
    Returns true if a .pro file seems to be for an autotest.
    Running qmake to figure this out takes too long.
*/
bool looks_like_testcase(QString const& pro_file, QString* target)
{
    QFile file(pro_file);
    if (!file.open(QIODevice::ReadOnly)) {
        return false;
    }

    *target = QString();

    bool loaded_qttest = false;

    do {
        QByteArray line = file.readLine();
        if (line.isEmpty()) {
            break;
        }

        line = line.trimmed();
        line.replace(' ', "");

        if (line == "load(qttest_p4)") {
            loaded_qttest = true;
        }

        if (line.startsWith("TARGET=")) {
            *target = QString::fromLatin1(line.mid(sizeof("TARGET=")-1));
            if (target->contains('/')) {
                *target = target->right(target->lastIndexOf('/')+1);
            }
        }

        if (loaded_qttest && !target->isEmpty()) {
            break;
        }
    } while(1);

    if (!loaded_qttest) {
        return false;
    }

    if (!target->isEmpty() && !target->startsWith("tst_")) {
        return false;
    }

    // If no target was set, default to tst_<dirname>
    if (target->isEmpty()) {
        *target = "tst_" + QFileInfo(pro_file).baseName();
    }

    return true;
}

/*
    Returns true if a .pro file seems to be a subdirs project.
    Running qmake to figure this out takes too long.
*/
bool looks_like_subdirs(QString const& pro_file)
{
    QFile file(pro_file);
    if (!file.open(QIODevice::ReadOnly)) {
        return false;
    }

    do {
        QByteArray line = file.readLine();
        if (line.isEmpty()) {
            break;
        }

        line = line.trimmed();
        line.replace(' ', "");

        if (line == "TEMPLATE=subdirs") {
            return true;
        }
    } while(1);

    return false;
}

/*
    Returns a list of all subdirs in a given .pro file
*/
QStringList tst_MakeTestSelfTest::find_subdirs(QString const& pro_file, FindSubdirsMode mode, QString const& prefix)
{
    QStringList out;

    QByteArray features = qgetenv("QMAKEFEATURES");

    if (features.isEmpty()) {
        features = SRCDIR "/features";
    }
    else {
        features.prepend(SRCDIR "/features"
#ifdef Q_OS_WIN32
                ";"
#else
                ":"
#endif
        );
    }

    QStringList args;
    args << pro_file << "-o" << SRCDIR "/dummy_output" << "CONFIG+=dump_subdirs";

    /* Turn on every option there is, to ensure we process every single directory */
    args
        << "QT_CONFIG+=dbus"
        << "QT_CONFIG+=declarative"
        << "QT_CONFIG+=egl"
        << "QT_CONFIG+=multimedia"
        << "QT_CONFIG+=OdfWriter"
        << "QT_CONFIG+=opengl"
        << "QT_CONFIG+=openvg"
        << "QT_CONFIG+=phonon"
        << "QT_CONFIG+=private_tests"
        << "QT_CONFIG+=pulseaudio"
        << "QT_CONFIG+=script"
        << "QT_CONFIG+=svg"
        << "QT_CONFIG+=webkit"
        << "QT_CONFIG+=xmlpatterns"
        << "CONFIG+=mac"
        << "CONFIG+=embedded"
        << "CONFIG+=symbian"
    ;



    QString cmd_with_args = QString("qmake %1").arg(args.join(" "));

    QProcess proc;

    proc.setProcessChannelMode(QProcess::MergedChannels);

    QProcessEnvironment env = QProcessEnvironment::systemEnvironment();
    env.insert("QMAKEFEATURES", features);
    proc.setProcessEnvironment(env);

    proc.start("qmake", args);
    if (!proc.waitForStarted(10000)) {
        QTest::qFail(qPrintable(QString("Failed to run qmake: %1\nCommand: %2")
            .arg(proc.errorString())
            .arg(cmd_with_args)),
            __FILE__, __LINE__
        );
        return out;
    }
    if (!proc.waitForFinished(30000)) {
        QTest::qFail(qPrintable(QString("qmake did not finish within 30 seconds\nCommand: %1\nOutput: %2")
            .arg(proc.errorString())
            .arg(cmd_with_args)
            .arg(QString::fromLocal8Bit(proc.readAll()))),
            __FILE__, __LINE__
        );
        return out;
    }

    if (proc.exitStatus() != QProcess::NormalExit) {
        QTest::qFail(qPrintable(QString("qmake crashed\nCommand: %1\nOutput: %2")
            .arg(cmd_with_args)
            .arg(QString::fromLocal8Bit(proc.readAll()))),
            __FILE__, __LINE__
        );
        return out;
    }

    if (proc.exitCode() != 0) {
        QTest::qFail(qPrintable(QString("qmake exited with code %1\nCommand: %2\nOutput: %3")
            .arg(proc.exitCode())
            .arg(cmd_with_args)
            .arg(QString::fromLocal8Bit(proc.readAll()))),
            __FILE__, __LINE__
        );
        return out;
    }

    QList<QByteArray> lines = proc.readAll().split('\n');
    if (!lines.count()) {
        QTest::qFail(qPrintable(QString("qmake seems to have not output anything\nCommand: %1\n")
            .arg(cmd_with_args)),
            __FILE__, __LINE__
        );
        return out;
    }

    foreach (QByteArray const& line, lines) {
        static const QByteArray marker = "Project MESSAGE: subdir: ";
        if (line.startsWith(marker)) {
            QString subdir = QString::fromLocal8Bit(line.mid(marker.size()).trimmed());
            out << prefix + subdir;

            if (mode == Flat) {
                continue;
            }

            // Need full path to subdir
            QString subdir_filepath = subdir;
            subdir_filepath.prepend(QFileInfo(pro_file).path() + "/");

            // Add subdirs recursively
            if (subdir.endsWith(".pro") && looks_like_subdirs(subdir_filepath)) {
                // Need full path to .pro file
                out << find_subdirs(subdir_filepath, mode, prefix);
            }

            if (QFileInfo(subdir_filepath).isDir()) {
                subdir_filepath += "/" + subdir + ".pro";
                if (looks_like_subdirs(subdir_filepath)) {
                    out << find_subdirs(subdir_filepath, mode, prefix + subdir + "/");
                }
            }
        }
    }

    return out;
}

void tst_MakeTestSelfTest::make_check()
{
    /*
        Run `make check' over the whole tests tree with a custom TESTRUNNER,
        to verify that the TESTRUNNER mechanism works right.
    */
    QString testsDir(SRCDIR "/..");
    QString checktest(SRCDIR "/checktest/checktest");

#if defined(Q_OS_WIN32) || defined(Q_OS_MAC)
    if (qgetenv("RUN_SLOW_TESTS").isEmpty()) {
        QSKIP("This test is too slow to run by default on this OS. Set RUN_SLOW_TESTS=1 to run it.", SkipAll);
    }
#endif

#ifdef Q_OS_WIN32
    checktest.replace("/", "\\");
    checktest += ".exe";
#endif

    QProcess make;
    make.setWorkingDirectory(testsDir);

    QStringList arguments;
    arguments << "-k";
    arguments << "check";
    arguments << QString("TESTRUNNER=%1").arg(checktest);

    // find the right make; from externaltests.cpp
    static const char makes[] =
        "nmake.exe\0"
        "mingw32-make.exe\0"
        "gmake\0"
        "make\0"
    ;

    bool ok = false;
    for (const char *p = makes; *p; p += strlen(p) + 1) {
        make.start(p, arguments);
        if (make.waitForStarted()) {
            ok = true;
            break;
        }
    }

    if (!ok) {
        QFAIL("Could not find the right make tool in PATH");
    }

    QVERIFY(make.waitForFinished(1000 * 60 * 10));
    QCOMPARE(make.exitStatus(), QProcess::NormalExit);

    int pass = 0;
    QList<QByteArray> out = make.readAllStandardOutput().split('\n');
    QStringList fails;
    foreach (QByteArray line, out) {
        while (line.endsWith("\r")) {
            line.chop(1);
        }
        if (line.startsWith("CHECKTEST FAIL")) {
            fails << QString::fromLocal8Bit(line);
        }
        if (line.startsWith("CHECKTEST PASS")) {
            ++pass;
        }
    }

    // We can't check that the exit code of make is 0, because some tests
    // may have failed to compile, but that doesn't mean `make check' is broken.
    // We do assume there are at least this many unbroken tests, though.
    QVERIFY2(fails.count() == 0,
        qPrintable(QString("`make check' doesn't work for %1 tests:\n%2")
            .arg(fails.count()).arg(fails.join("\n")))
    );
    QVERIFY(pass > 50);
}

QStringList find_test_class(QString const& filename)
{
    QStringList out;

    QFile file(filename);
    if (!file.open(QIODevice::ReadOnly)) {
        return out;
    }

    static char const* klass_indicators[] = {
        "QTEST_MAIN(",
        "QTEST_APPLESS_MAIN(",
        "class",
        "staticconstcharklass[]=\"",  /* hax0r tests which define their own metaobject */
        0
    };

    do {
        QByteArray line = file.readLine();
        if (line.isEmpty()) {
            break;
        }

        line = line.trimmed();
        line.replace(' ', "");

        for (int i = 0; klass_indicators[i]; ++i) {
            char const* prefix = klass_indicators[i];
            if (!line.startsWith(prefix)) {
                continue;
            }
            QByteArray klass = line.mid(strlen(prefix));
            if (!klass.startsWith("tst_")) {
                continue;
            }
            for (int j = 0; j < klass.size(); ++j) {
                char c = klass[j];
                if (c == '_'
                        || (c >= '0' && c <= '9')
                        || (c >= 'A' && c <= 'Z')
                        || (c >= 'a' && c <= 'z')) {
                    continue;
                }
                else {
                    klass.truncate(j);
                    break;
                }
            }
            QString klass_str = QString::fromLocal8Bit(klass).toLower();
            if (!out.contains(klass_str))
                out << klass_str;
            break;
        }
    } while(1);

    return out;
}

QTEST_MAIN(tst_MakeTestSelfTest)
#include "tst_maketestselftest.moc"
