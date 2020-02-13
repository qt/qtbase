/****************************************************************************
**
** Copyright (C) 2016 The Qt Company Ltd.
** Contact: https://www.qt.io/licensing/
**
** This file is part of the test suite of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:GPL-EXCEPT$
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company. For licensing terms
** and conditions see https://www.qt.io/terms-conditions. For further
** information use the contact form at https://www.qt.io/contact-us.
**
** GNU General Public License Usage
** Alternatively, this file may be used under the terms of the GNU
** General Public License version 3 as published by the Free Software
** Foundation with exceptions as appearing in the file LICENSE.GPL3-EXCEPT
** included in the packaging of this file. Please review the following
** information to ensure the GNU General Public License requirements will
** be met: https://www.gnu.org/licenses/gpl-3.0.html.
**
** $QT_END_LICENSE$
**
****************************************************************************/

#include <QtTest/QtTest>
#include <qtranslator.h>
#include <qfile.h>

class tst_QTranslator : public QObject
{
    Q_OBJECT

public:
    tst_QTranslator();
protected:
    bool eventFilter(QObject *obj, QEvent *event);
private slots:
    void initTestCase();

    void load_data();
    void load();
    void threadLoad();
    void testLanguageChange();
    void plural();
    void translate_qm_file_generated_with_msgfmt();
    void loadDirectory();
    void dependencies();
    void translationInThreadWhileInstallingTranslator();

private:
    int languageChangeEventCounter;
    QSharedPointer<QTemporaryDir> dataDir;
};

tst_QTranslator::tst_QTranslator()
    : languageChangeEventCounter(0)
{
    qApp->installEventFilter(this);
}

void tst_QTranslator::initTestCase()
{
#if defined(Q_OS_ANDROID) && !defined(Q_OS_ANDROID_EMBEDDED)
    QString sourceDir(":/android_testdata/");
    QDirIterator it(sourceDir, QDirIterator::Subdirectories);
    while (it.hasNext()) {
        it.next();

        QFileInfo sourceFileInfo = it.fileInfo();
        if (!sourceFileInfo.isDir()) {
            QFileInfo destinationFileInfo(QStandardPaths::writableLocation(QStandardPaths::CacheLocation) + QLatin1Char('/') + sourceFileInfo.filePath().mid(sourceDir.length()));

            if (!destinationFileInfo.exists()) {
                QVERIFY(QDir().mkpath(destinationFileInfo.path()));
                QVERIFY(QFile::copy(sourceFileInfo.filePath(), destinationFileInfo.filePath()));
            }
        }
    }

    QDir::setCurrent(QStandardPaths::writableLocation(QStandardPaths::CacheLocation));
#endif

    // chdir into the directory containing our testdata,
    // to make the code simpler (load testdata via relative paths)
#ifdef Q_OS_WINRT
    // ### TODO: Use this for all platforms in 5.7
    dataDir = QEXTRACTTESTDATA(QStringLiteral("/"));
    QVERIFY2(!dataDir.isNull(), qPrintable("Could not extract test data"));
    QVERIFY2(QDir::setCurrent(dataDir->path()), qPrintable("Could not chdir to " + dataDir->path()));
#else // !Q_OS_WINRT
    QString testdata_dir = QFileInfo(QFINDTESTDATA("hellotr_la.qm")).absolutePath();
    QVERIFY2(QDir::setCurrent(testdata_dir), qPrintable("Could not chdir to " + testdata_dir));
#endif // !Q_OS_WINRT

}

bool tst_QTranslator::eventFilter(QObject *, QEvent *event)
{
    if (event->type() == QEvent::LanguageChange)
        ++languageChangeEventCounter;
    return false;
}

void tst_QTranslator::load_data()
{
    QTest::addColumn<QString>("filepath");
    QTest::addColumn<bool>("isEmpty");
    QTest::addColumn<QString>("translation");
    QTest::addColumn<QString>("language");

    QTest::newRow("hellotr_la") << "hellotr_la.qm" << false << "Hallo Welt!" << "de";
    QTest::newRow("hellotr_empty") << "hellotr_empty.qm" << true << "" << "";
}

void tst_QTranslator::load()
{
    QFETCH(QString, filepath);
    QFETCH(bool, isEmpty);
    QFETCH(QString, translation);
    QFETCH(QString, language);

    {
        QTranslator tor;
        QVERIFY(tor.load(QFileInfo(filepath).baseName()));
        QCOMPARE(tor.isEmpty(), isEmpty);
        QCOMPARE(tor.translate("QPushButton", "Hello world!"), translation);
        QCOMPARE(tor.filePath(), filepath);
        QCOMPARE(tor.language(), language);
    }

    {
        QFile file(filepath);
        file.open(QFile::ReadOnly);
        QByteArray data = file.readAll();
        QTranslator tor;
        QVERIFY(tor.load((const uchar *)data.constData(), data.length()));
        QCOMPARE(tor.isEmpty(), isEmpty);
        QCOMPARE(tor.translate("QPushButton", "Hello world!"), translation);
        QCOMPARE(tor.filePath(), "");
        QCOMPARE(tor.language(), language);
    }

    {
        QTranslator tor;
        QString path = QString(":/tst_qtranslator/%1").arg(filepath);
        QVERIFY(tor.load(path));
        QCOMPARE(tor.isEmpty(), isEmpty);
        QCOMPARE(tor.translate("QPushButton", "Hello world!"), translation);
        QCOMPARE(tor.filePath(), path);
        QCOMPARE(tor.language(), language);
    }
}

class TranslatorThread : public QThread
{
    void run() {
        QTranslator tor( 0 );
        tor.load("hellotr_la");

        if (tor.isEmpty())
            qFatal("Could not load translation");
        if (tor.translate("QPushButton", "Hello world!") !=  QLatin1String("Hallo Welt!"))
            qFatal("Test string was not translated correctlys");
    }
};


void tst_QTranslator::threadLoad()
{
    TranslatorThread thread;
    thread.start();
    QVERIFY(thread.wait(10 * 1000));
}

void tst_QTranslator::testLanguageChange()
{
    languageChangeEventCounter = 0;

    QTranslator *tor = new QTranslator;
    tor->load("hellotr_la.qm");
    qApp->sendPostedEvents();
    qApp->sendPostedEvents();
    QCOMPARE(languageChangeEventCounter, 0);

    tor->load("doesn't exist, same as clearing");
    qApp->sendPostedEvents();
    qApp->sendPostedEvents();
    QCOMPARE(languageChangeEventCounter, 0);

    tor->load("hellotr_la.qm");
    qApp->sendPostedEvents();
    qApp->sendPostedEvents();
    QCOMPARE(languageChangeEventCounter, 0);

    qApp->installTranslator(tor);
    qApp->sendPostedEvents();
    qApp->sendPostedEvents();
    QCOMPARE(languageChangeEventCounter, 1);

    tor->load("doesn't exist, same as clearing");
    qApp->sendPostedEvents();
    qApp->sendPostedEvents();
    QCOMPARE(languageChangeEventCounter, 2);

    tor->load("hellotr_la.qm");
    qApp->sendPostedEvents();
    qApp->sendPostedEvents();
    QCOMPARE(languageChangeEventCounter, 3);

    qApp->removeTranslator(tor);
    qApp->sendPostedEvents();
    qApp->sendPostedEvents();
    QCOMPARE(languageChangeEventCounter, 4);

    tor->load("doesn't exist, same as clearing");
    qApp->sendPostedEvents();
    qApp->sendPostedEvents();
    QCOMPARE(languageChangeEventCounter, 4);

    qApp->installTranslator(tor);
    qApp->sendPostedEvents();
    qApp->sendPostedEvents();
    QCOMPARE(languageChangeEventCounter, 4);

    tor->load("hellotr_la.qm");
    qApp->sendPostedEvents();
    qApp->sendPostedEvents();
    QCOMPARE(languageChangeEventCounter, 5);

    delete tor;
    tor = 0;
    qApp->sendPostedEvents();
    qApp->sendPostedEvents();
    QCOMPARE(languageChangeEventCounter, 6);
}


void tst_QTranslator::plural()
{

    QTranslator tor( 0 );
    tor.load("hellotr_la");
    QVERIFY(!tor.isEmpty());
    QCoreApplication::installTranslator(&tor);
    QCOMPARE(QCoreApplication::translate("QPushButton", "Hello %n world(s)!", 0, 0), QLatin1String("Hallo 0 Welten!"));
    QCOMPARE(QCoreApplication::translate("QPushButton", "Hello %n world(s)!", 0, 1), QLatin1String("Hallo 1 Welt!"));
    QCOMPARE(QCoreApplication::translate("QPushButton", "Hello %n world(s)!", 0, 2), QLatin1String("Hallo 2 Welten!"));
}

void tst_QTranslator::translate_qm_file_generated_with_msgfmt()
{
    QTranslator translator;
    translator.load("msgfmt_from_po");
    qApp->installTranslator(&translator);

    QCOMPARE(QCoreApplication::translate("", "Intro"), QLatin1String("Einleitung"));
    // The file is converted from a po file, thus it does not have any context info.
    // The following should then not be translated
    QCOMPARE(QCoreApplication::translate("contekst", "Intro"), QLatin1String("Intro"));
    QCOMPARE(QCoreApplication::translate("contekst", "Intro\0\0"), QLatin1String("Intro"));
    QCOMPARE(QCoreApplication::translate("contekst", "Intro\0x"), QLatin1String("Intro"));
    QCOMPARE(QCoreApplication::translate("", "Intro\0\0"), QLatin1String("Einleitung"));
    QCOMPARE(QCoreApplication::translate("", "Intro\0x"), QLatin1String("Einleitung"));

    qApp->removeTranslator(&translator);
}

void tst_QTranslator::loadDirectory()
{
    QString current_base = QDir::current().dirName();
    QVERIFY(QFileInfo("../" + current_base).isDir());

    QTranslator tor;
    tor.load(current_base, "..");
    QVERIFY(tor.isEmpty());
}

void tst_QTranslator::dependencies()
{
    {
        // load
        QTranslator tor;
        tor.load("dependencies_la");
        QVERIFY(!tor.isEmpty());
        QCOMPARE(tor.translate("QPushButton", "Hello world!"), QLatin1String("Hallo Welt!"));

        // plural
        QCoreApplication::installTranslator(&tor);
        QCOMPARE(QCoreApplication::translate("QPushButton", "Hello %n world(s)!", 0, 0), QLatin1String("Hallo 0 Welten!"));
        QCOMPARE(QCoreApplication::translate("QPushButton", "Hello %n world(s)!", 0, 1), QLatin1String("Hallo 1 Welt!"));
        QCOMPARE(QCoreApplication::translate("QPushButton", "Hello %n world(s)!", 0, 2), QLatin1String("Hallo 2 Welten!"));

        // pick up translation from the file with dependencies
        QCOMPARE(tor.translate("QPushButton", "It's a small world"), QLatin1String("Es ist eine kleine Welt"));
    }

    {
        QTranslator tor( 0 );
        QFile file("dependencies_la.qm");
        file.open(QFile::ReadOnly);
        QByteArray data = file.readAll();
        tor.load((const uchar *)data.constData(), data.length());
        QVERIFY(!tor.isEmpty());
        QCOMPARE(tor.translate("QPushButton", "Hello world!"), QLatin1String("Hallo Welt!"));
    }
}

struct TranslateThread : public QThread
{
    bool ok = false;
    QAtomicInt terminate;
    QMutex startupLock;
    QWaitCondition runningCondition;

    void run() {
        bool startSignalled = false;

        while (terminate.loadRelaxed() == 0) {
            const QString result =  QCoreApplication::translate("QPushButton", "Hello %n world(s)!", 0, 0);

            if (!startSignalled) {
                QMutexLocker startupLocker(&startupLock);
                runningCondition.wakeAll();
                startSignalled = true;
            }

            ok = (result == QLatin1String("Hallo 0 Welten!"))
                  || (result == QLatin1String("Hello 0 world(s)!"));
            if (!ok)
                break;
        }
    }
};

void tst_QTranslator::translationInThreadWhileInstallingTranslator()
{
    TranslateThread thread;

    QMutexLocker startupLocker(&thread.startupLock);

    thread.start();

    thread.runningCondition.wait(&thread.startupLock);

    QTranslator *tor = new QTranslator;
    tor->load("hellotr_la");
    QCoreApplication::installTranslator(tor);

    ++thread.terminate;

    QVERIFY(thread.wait());
    QVERIFY(thread.ok);
}

QTEST_MAIN(tst_QTranslator)
#include "tst_qtranslator.moc"
