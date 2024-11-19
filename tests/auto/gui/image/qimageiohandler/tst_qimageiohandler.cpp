// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only


#include <QTest>

#include <qcoreapplication.h>
#include <qdebug.h>
#include <qhash.h>
#include <qimageiohandler.h>
#include <qfile.h>
#include <qimagereader.h>
#include <qlibraryinfo.h>
#include "pluginlog.h"

class tst_QImageIOHandler : public QObject
{
Q_OBJECT

public:
    tst_QImageIOHandler();
    virtual ~tst_QImageIOHandler();
    static void initMain()
    {
        QHashSeed::setDeterministicGlobalSeed();
    }

private slots:
    void getSetCheck();
    void hasCustomPluginFormat();
    void pluginRead_data();
    void pluginRead();
    void pluginNoAutoDetection_data();
    void pluginNoAutoDetection();
    void pluginDecideFromContent_data();
    void pluginDecideFromContent();
    void pluginDetectedFormat_data();
    void pluginDetectedFormat();
    void pluginWrongFormat_data();
    void pluginWrongFormat();
private:
    QString m_prefix;
    QList<QByteArray> m_supportedReadFormats;
    bool m_testPluginFoundLast = false;
};

class MyImageIOHandler : public QImageIOHandler
{
public:
    MyImageIOHandler() : QImageIOHandler() { }
    bool canRead() const override { return true; }
    bool read(QImage *) override { return true; }
};

tst_QImageIOHandler::tst_QImageIOHandler()
{
    m_prefix = QFINDTESTDATA("images/");
    m_supportedReadFormats = QImageReader::supportedImageFormats();
    m_testPluginFoundLast = QLibraryInfo::isSharedBuild();
}

tst_QImageIOHandler::~tst_QImageIOHandler()
{
}

// Testing get/set functions
void tst_QImageIOHandler::getSetCheck()
{
    MyImageIOHandler obj1;
    // QIODevice * QImageIOHandler::device()
    // void QImageIOHandler::setDevice(QIODevice *)
    QFile *var1 = new QFile;
    obj1.setDevice(var1);
    QCOMPARE(obj1.device(), (QIODevice *)var1);
    obj1.setDevice((QIODevice *)0);
    QCOMPARE(obj1.device(), (QIODevice *)0);
    delete var1;
}

Q_IMPORT_PLUGIN(TestImagePlugin)

void tst_QImageIOHandler::hasCustomPluginFormat()
{
    QVERIFY(m_supportedReadFormats.contains("foo"));
}

void tst_QImageIOHandler::pluginRead_data()
{
    QTest::addColumn<QString>("color");
    QTest::addColumn<QString>("suffix");
    QTest::addColumn<QStringList>("expectedPluginLog");

    QTest::newRow("testplugin") << "green" << "foo" << QStringList({ "formatname-matched" });
    QTest::newRow("overridden-builtin") << "red" << "png" << QStringList({ "formatname-matched" });
    QTest::newRow("builtin") << "black" << "bmp" << QStringList();
    QTest::newRow("no-suffix") << "blue" << "" << QStringList({ "contents-matched" });
    QTest::newRow("wrong-suffix") << "yellow" << "jpg" << QStringList({ "contents-matched" });
    QTest::newRow("unknown-suffix") << "black" << "bar" << QStringList({ "formatname-unmatched", "contents-unmatched" });
    if (m_supportedReadFormats.contains("jpeg"))
        QTest::newRow("wrong-suffix2") << "white" << "foo" << QStringList({ "formatname-matched" });
    if (m_supportedReadFormats.contains("gif")) {
        if (m_testPluginFoundLast)
            QTest::newRow("plugin-writeonly") << "cyan" << "gif" << QStringList();
        else
            QTest::newRow("plugin-writeonly") << "cyan" << "gif" << QStringList({ "formatname-matched" });
    }
}

void tst_QImageIOHandler::pluginRead()
{
    QFETCH(QString, color);
    QFETCH(QString, suffix);
    QFETCH(QStringList, expectedPluginLog);

    PluginLog::clear();
    QString filePath = m_prefix + color + (suffix.isEmpty() ? "" : "." + suffix);
    QImage img(filePath);
    QCOMPARE(PluginLog::size(), expectedPluginLog.size());
    for (int i = 0; i < PluginLog::size(); i++)
        QCOMPARE(PluginLog::item(i), expectedPluginLog.value(i));
    QVERIFY(!img.isNull());
    QCOMPARE(img.pixelColor(0, 0), QColor(color));
}

void tst_QImageIOHandler::pluginNoAutoDetection_data()
{
    QTest::addColumn<QString>("color");
    QTest::addColumn<QString>("suffix");
    QTest::addColumn<QStringList>("expectedPluginLog");
    QTest::addColumn<bool>("expectedSuccess");

    QTest::newRow("testplugin") << "green" << "foo" << QStringList({ "formatname-matched" }) << true;
    QTest::newRow("overridden-builtin") << "red" << "png" << QStringList({ "formatname-matched" }) << true;
    QTest::newRow("builtin") << "black" << "bmp" << QStringList() << true;
    QTest::newRow("no-suffix") << "blue" << "" << QStringList() << false;
    QTest::newRow("wrong-suffix") << "yellow" << "jpg" << QStringList() << false;
    QTest::newRow("unknown-suffix") << "black" << "bar" << QStringList() << false;
    QTest::newRow("wrong-suffix2") << "white" << "foo" << QStringList({ "formatname-matched" }) << false;
    if (m_supportedReadFormats.contains("gif")) {
        if (m_testPluginFoundLast)
            QTest::newRow("plugin-writeonly") << "cyan" << "gif" << QStringList() << true;
        else
            QTest::newRow("plugin-writeonly") << "cyan" << "gif" << QStringList({ "formatname-matched" }) << true;
    }
}

void tst_QImageIOHandler::pluginNoAutoDetection()
{
    QFETCH(QString, color);
    QFETCH(QString, suffix);
    QFETCH(QStringList, expectedPluginLog);
    QFETCH(bool, expectedSuccess);

    QString filePath = m_prefix + color + (suffix.isEmpty() ? "" : "." + suffix);
    {
        // Confirm that the file suffix is ignored, i.e. nothing happens if no format is set
        PluginLog::clear();
        QImageReader r(filePath);
        r.setAutoDetectImageFormat(false);
        QImage img = r.read();
        QVERIFY(img.isNull());
        QCOMPARE(PluginLog::size(), 0);
    }

    PluginLog::clear();
    QImageReader r(filePath, suffix.toLatin1());
    r.setAutoDetectImageFormat(false);
    QImage img = r.read();

    QCOMPARE(PluginLog::size(), expectedPluginLog.size());
    for (int i = 0; i < PluginLog::size(); i++)
        QCOMPARE(PluginLog::item(i), expectedPluginLog.value(i));
    QCOMPARE(!img.isNull(), expectedSuccess);
    if (expectedSuccess)
        QCOMPARE(img.pixelColor(0, 0), QColor(color));
}

void tst_QImageIOHandler::pluginDecideFromContent_data()
{
    QTest::addColumn<QString>("color");
    QTest::addColumn<QString>("suffix");
    QTest::addColumn<QStringList>("expectedPluginLog");

    QTest::newRow("testplugin") << "green" << "foo" << QStringList({ "contents-matched" });
    QTest::newRow("overridden-builtin") << "red" << "png" << QStringList({ "contents-matched" });
    QTest::newRow("builtin") << "black" << "bmp" << QStringList({ "contents-unmatched" });
    QTest::newRow("no-suffix") << "blue" << "" << QStringList({ "contents-matched" });
    QTest::newRow("wrong-suffix") << "yellow" << "jpg" << QStringList({ "contents-matched" });
    QTest::newRow("unknown-suffix") << "black" << "bar" << QStringList({ "contents-unmatched" });
    if (m_supportedReadFormats.contains("jpeg")) {
        if (m_testPluginFoundLast)
            QTest::newRow("wrong-suffix2") << "white" << "foo" << QStringList();
        else
            QTest::newRow("wrong-suffix2") << "white" << "foo" << QStringList({ "contents-unmatched" });
    }
}

void tst_QImageIOHandler::pluginDecideFromContent()
{
    QFETCH(QString, color);
    QFETCH(QString, suffix);
    QFETCH(QStringList, expectedPluginLog);

    QString filePath = m_prefix + color + (suffix.isEmpty() ? "" : "." + suffix);
    PluginLog::clear();
    QImageReader r(filePath, suffix.toLatin1());
    r.setDecideFormatFromContent(true);
    QImage img = r.read();

    QCOMPARE(PluginLog::size(), expectedPluginLog.size());
    for (int i = 0; i < PluginLog::size(); i++)
        QCOMPARE(PluginLog::item(i), expectedPluginLog.value(i));
    QVERIFY(!img.isNull());
    QCOMPARE(img.pixelColor(0, 0), QColor(color));
}

void tst_QImageIOHandler::pluginDetectedFormat_data()
{
    QTest::addColumn<QString>("color");
    QTest::addColumn<QString>("suffix");
    QTest::addColumn<QString>("fileFormat");

    QTest::newRow("testplugin") << "green" << "foo" << "foo";
    QTest::newRow("overridden-builtin") << "red" << "png" << "png";
    QTest::newRow("builtin") << "black" << "bmp" << "bmp";
    QTest::newRow("no-suffix") << "blue" << "" << "png";
    QTest::newRow("wrong-suffix") << "yellow" << "jpg" << "foo";
    QTest::newRow("unknown-suffix") << "black" << "bar" << "bmp";
    if (m_supportedReadFormats.contains("jpeg"))
        QTest::newRow("wrong-suffix2") << "white" << "foo" << "jpeg";
    if (m_supportedReadFormats.contains("gif"))
        QTest::newRow("plugin-writeonly") << "cyan" << "gif" << "gif";
}

void tst_QImageIOHandler::pluginDetectedFormat()
{
    QFETCH(QString, color);
    QFETCH(QString, suffix);
    QFETCH(QString, fileFormat);

    QString filePath = m_prefix + color + (suffix.isEmpty() ? "" : "." + suffix);
    QCOMPARE(QImageReader::imageFormat(filePath), fileFormat.toLatin1());
}

void tst_QImageIOHandler::pluginWrongFormat_data()
{
    QTest::addColumn<QString>("color");
    QTest::addColumn<QString>("suffix");
    QTest::addColumn<QString>("forceFormat");

    QTest::newRow("testplugin") << "green" << "foo" << "bmp";
    QTest::newRow("overridden-builtin") << "red" << "png" << "bmp";
    QTest::newRow("builtin") << "black" << "bmp" << "foo";
    QTest::newRow("no-suffix") << "blue" << "" << "bmp";
    QTest::newRow("wrong-suffix") << "yellow" << "jpg" << "bmp";
    QTest::newRow("unknown-suffix") << "black" << "bar" << "foo";
    QTest::newRow("wrong-suffix2") << "white" << "foo" << "bmp";
}

void tst_QImageIOHandler::pluginWrongFormat()
{
    QFETCH(QString, color);
    QFETCH(QString, suffix);
    QFETCH(QString, forceFormat);

    PluginLog::clear();
    QString filePath = m_prefix + color + (suffix.isEmpty() ? "" : "." + suffix);
    QImage img(filePath, forceFormat.toLatin1());
    QVERIFY(img.isNull());
}

QTEST_MAIN(tst_QImageIOHandler)

#include "tst_qimageiohandler.moc"
