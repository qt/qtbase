/****************************************************************************
**
** Copyright (C) 2015 The Qt Company Ltd.
** Contact: http://www.qt.io/licensing/
**
** This file is part of the test suite of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:LGPL21$
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company. For licensing terms
** and conditions see http://www.qt.io/terms-conditions. For further
** information use the contact form at http://www.qt.io/contact-us.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 2.1 or version 3 as published by the Free
** Software Foundation and appearing in the file LICENSE.LGPLv21 and
** LICENSE.LGPLv3 included in the packaging of this file. Please review the
** following information to ensure the GNU Lesser General Public License
** requirements will be met: https://www.gnu.org/licenses/lgpl.html and
** http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** As a special exception, The Qt Company gives you certain additional
** rights. These rights are described in The Qt Company LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
** $QT_END_LICENSE$
**
****************************************************************************/


#include <QtTest/QtTest>

#include <QtCore/QSettings>
#include <private/qsettings_p.h>
#include <QtCore/QCoreApplication>
#include <QtCore/QDateTime>
#include <QtCore/QtGlobal>
#include <QtCore/QMetaType>
#include <QtCore/QString>
#include <QtCore/QDir>
#include <QtCore/QThread>
#include <QtCore/QSysInfo>
#include <QtGui/QKeySequence>

#include <cctype>
#include <stdlib.h>
#if defined(Q_OS_WIN) && defined(Q_CC_GNU)
// need for unlink on mingw
#include <io.h>
#endif

#if defined(Q_OS_WIN)
#include <QtCore/qt_windows.h>
#else
#include <unistd.h>
#endif

Q_DECLARE_METATYPE(QSettings::Format)

#ifndef QSETTINGS_P_H_VERSION
#define QSETTINGS_P_H_VERSION 1
#endif

QT_FORWARD_DECLARE_CLASS(QSettings)

static inline bool canWriteNativeSystemSettings()
{
#if defined(Q_OS_WIN) && !defined(Q_OS_WINRT)
    HKEY key;
    const LONG result = RegOpenKeyEx(HKEY_LOCAL_MACHINE, L"Software", 0, KEY_WRITE, &key);
    if (result == ERROR_SUCCESS)
        RegCloseKey(key);
    else
        qErrnoWarning(result, "RegOpenKeyEx failed");
    return result == ERROR_SUCCESS;
#else // Q_OS_WIN && !Q_OS_WINRT
    return true;
#endif
}

static const char insufficientPermissionSkipMessage[] = "Insufficient permissions for this test.";

class tst_QSettings : public QObject
{
    Q_OBJECT

public:
    tst_QSettings();

public slots:
    void initTestCase();
    void cleanup() { cleanupTestFiles(); }
private slots:
    void getSetCheck();
    void ctor_data();
    void ctor();
    void beginGroup();
    void setValue();
    void remove();
    void contains();
    void sync();
    void setFallbacksEnabled();
    void setFallbacksEnabled_data();
    void fromFile_data();
    void fromFile();
    void testArrays_data();
    void testArrays();
    void testCaseSensitivity_data();
    void testCaseSensitivity();
    void testErrorHandling_data();
    void testErrorHandling();
    void testChildKeysAndGroups_data();
    void testChildKeysAndGroups();
    void testUpdateRequestEvent();
    void testThreadSafety();
    void testEmptyData();
    void testEmptyKey();
    void testResourceFiles();
    void testRegistryShortRootNames();
    void trailingWhitespace();
#ifdef Q_OS_MAC
    void fileName();
#endif
    void isWritable_data();
    void isWritable();
    void registerFormat();
    void setPath();
    void setDefaultFormat();
    void dontCreateNeedlessPaths();
#if !defined(Q_OS_WIN) && !defined(QT_QSETTINGS_ALWAYS_CASE_SENSITIVE_AND_FORGET_ORIGINAL_KEY_ORDER)
    void dontReorderIniKeysNeedlessly();
#endif
#if defined(Q_OS_WIN) && !defined(Q_OS_WINRT)
    void consistentRegistryStorage();
#endif

#ifdef QT_BUILD_INTERNAL
    void allKeys_data();
    void allKeys();
    void childGroups_data();
    void childGroups();
    void childKeys_data();
    void childKeys();
    void setIniCodec();
    void testIniParsing_data();
    void testIniParsing();
    void testEscapes();
    void testNormalizedKey_data();
    void testNormalizedKey();
    void testVariantTypes_data();
    void testVariantTypes();
#endif
    void rainersSyncBugOnMac_data();
    void rainersSyncBugOnMac();
    void recursionBug();

    void testByteArray_data();
    void testByteArray();
    void iniCodec();
    void bom();

private:
    void cleanupTestFiles();

    const bool m_canWriteNativeSystemSettings;
};

// Testing get/set functions
void tst_QSettings::getSetCheck()
{
    QSettings obj1;
    // bool QSettings::fallbacksEnabled()
    // void QSettings::setFallbacksEnabled(bool)
    obj1.setFallbacksEnabled(false);
    QCOMPARE(false, obj1.fallbacksEnabled());
    obj1.setFallbacksEnabled(true);
    QCOMPARE(true, obj1.fallbacksEnabled());
}

static QString settingsPath(const char *path = Q_NULLPTR)
{
    // Temporary path for files that are specified explicitly in the constructor.
#ifndef Q_OS_WINRT
    static const QString tempPath = QDir::tempPath() + QLatin1String("/tst_QSettings");
#else
    static const QString tempPath = QStandardPaths::writableLocation(QStandardPaths::AppLocalDataLocation)
        + QLatin1String("/tst_QSettings");
#endif
    return path && *path ? tempPath + QLatin1Char('/') + QLatin1String(path) : tempPath;
}

static bool readCustom1File(QIODevice &device, QSettings::SettingsMap &map)
{
    QDataStream in(&device);
    quint32 magic;
    in >> magic;
    in >> map;
    return (magic == 0x01010101 && in.status() == QDataStream::Ok);
}

static bool writeCustom1File(QIODevice &device, const QSettings::SettingsMap &map)
{
    QDataStream out(&device);
    out << quint32(0x01010101);
    out << map;
    return out.status() == QDataStream::Ok;
}

static bool readCustom2File(QIODevice &device, QSettings::SettingsMap &map)
{
    QDataStream in(&device);
    quint64 magic;
    in >> magic;
    in >> map;
    return (magic == Q_UINT64_C(0x0202020202020202) && in.status() == QDataStream::Ok);
}

static bool writeCustom2File(QIODevice &device, const QSettings::SettingsMap &map)
{
    QDataStream out(&device);
    out << Q_UINT64_C(0x0202020202020202);
    out << map;
    return out.status() == QDataStream::Ok;
}

static bool readCustom3File(QIODevice &device, QSettings::SettingsMap &map)
{
    QTextStream in(&device);
    QString tag;
    in >> tag;
    if (tag == "OK") {
        map.insert("retval", "OK");
        return true;
    } else {
        return false;
    }
}

static bool writeCustom3File(QIODevice &device, const QSettings::SettingsMap &map)
{
    QTextStream out(&device);
    if (map.value("retval") != "OK")
        return false;

    out << "OK";
    return true;
}

static void populateWithFormats()
{
    QTest::addColumn<QSettings::Format>("format");

    QTest::newRow("native") << QSettings::NativeFormat;
    QTest::newRow("ini") << QSettings::IniFormat;
    QTest::newRow("custom1") << QSettings::CustomFormat1;
    QTest::newRow("custom2") << QSettings::CustomFormat2;
}

tst_QSettings::tst_QSettings()
    : m_canWriteNativeSystemSettings(canWriteNativeSystemSettings())
{
    QStandardPaths::setTestModeEnabled(true);
}

void tst_QSettings::initTestCase()
{
    if (!m_canWriteNativeSystemSettings)
        qWarning("The test is not running with administrative rights. Some tests will be skipped.");
    QSettings::Format custom1 = QSettings::registerFormat("custom1", readCustom1File, writeCustom1File);
    QSettings::Format custom2 = QSettings::registerFormat("custom2", readCustom2File, writeCustom2File
#ifndef QT_QSETTINGS_ALWAYS_CASE_SENSITIVE_AND_FORGET_ORIGINAL_KEY_ORDER
                                                          , Qt::CaseInsensitive
#endif
                                                          );
    QVERIFY(custom1 == QSettings::CustomFormat1);
    QVERIFY(custom2 == QSettings::CustomFormat2);

    cleanupTestFiles();
}

void tst_QSettings::cleanupTestFiles()
{
    QSettings::setSystemIniPath(settingsPath("__system__"));
    QSettings::setUserIniPath(settingsPath("__user__"));

    QDir settingsDir(settingsPath());
    if (settingsDir.exists())
        QVERIFY(settingsDir.removeRecursively());

#if defined(Q_OS_WIN) && !defined(Q_OS_WINRT)
    QSettings("HKEY_CURRENT_USER\\Software\\software.org", QSettings::NativeFormat).clear();
    QSettings("HKEY_CURRENT_USER\\Software\\other.software.org", QSettings::NativeFormat).clear();
    QSettings("HKEY_CURRENT_USER\\Software\\foo", QSettings::NativeFormat).clear();
    QSettings("HKEY_CURRENT_USER\\Software\\bar", QSettings::NativeFormat).clear();
    QSettings("HKEY_CURRENT_USER\\Software\\bat", QSettings::NativeFormat).clear();
    QSettings("HKEY_CURRENT_USER\\Software\\baz", QSettings::NativeFormat).clear();
    if (m_canWriteNativeSystemSettings) {
        QSettings("HKEY_LOCAL_MACHINE\\Software\\software.org", QSettings::NativeFormat).clear();
        QSettings("HKEY_LOCAL_MACHINE\\Software\\other.software.org", QSettings::NativeFormat).clear();
        QSettings("HKEY_LOCAL_MACHINE\\Software\\foo", QSettings::NativeFormat).clear();
        QSettings("HKEY_LOCAL_MACHINE\\Software\\bar", QSettings::NativeFormat).clear();
        QSettings("HKEY_LOCAL_MACHINE\\Software\\bat", QSettings::NativeFormat).clear();
        QSettings("HKEY_LOCAL_MACHINE\\Software\\baz", QSettings::NativeFormat).clear();
    }
#elif defined(Q_OS_DARWIN) || defined(Q_OS_WINRT)
    QSettings(QSettings::UserScope, "software.org", "KillerAPP").clear();
    QSettings(QSettings::SystemScope, "software.org", "KillerAPP").clear();
    QSettings(QSettings::UserScope, "other.software.org", "KillerAPP").clear();
    QSettings(QSettings::SystemScope, "other.software.org", "KillerAPP").clear();
    QSettings(QSettings::UserScope, "software.org").clear();
    QSettings(QSettings::SystemScope, "software.org").clear();
    QSettings(QSettings::UserScope, "other.software.org").clear();
    QSettings(QSettings::SystemScope, "other.software.org").clear();
#endif

    const QString foo(QLatin1String("foo"));

#if defined(Q_OS_WINRT)
    QSettings(foo, QSettings::NativeFormat).clear();
    QFile fooFile(QStandardPaths::writableLocation(QStandardPaths::AppLocalDataLocation) + QLatin1Char('/') + foo);
#else
    QFile fooFile(foo);
#endif
    if (fooFile.exists())
        QVERIFY2(fooFile.remove(), qPrintable(fooFile.errorString()));
}

/*
    Test the constructors and the assignment operator.
*/

void tst_QSettings::ctor_data()
{
    populateWithFormats();
}

void tst_QSettings::ctor()
{
    QFETCH(QSettings::Format, format);

    if (!m_canWriteNativeSystemSettings && format == QSettings::NativeFormat)
        QSKIP(insufficientPermissionSkipMessage);

    {
        QSettings settings1(format, QSettings::UserScope, "software.org", "KillerAPP");
        QSettings settings2(format, QSettings::UserScope, "software.org");
        QSettings settings3(format, QSettings::SystemScope, "software.org", "KillerAPP");
        QSettings settings4(format, QSettings::SystemScope, "software.org");

        QSettings settings5(format, QSettings::UserScope, "software.org", "KillerAPP");
        QSettings settings6(format, QSettings::UserScope, "software.org");
        QSettings settings7(format, QSettings::SystemScope, "software.org", "KillerAPP");
        QSettings settings8(format, QSettings::SystemScope, "software.org");

        // test QSettings::format() while we're at it
        QVERIFY(settings1.format() == format);
        QVERIFY(settings2.format() == format);
        QVERIFY(settings3.format() == format);
        QVERIFY(settings4.format() == format);

        // test QSettings::scope() while we're at it
        QVERIFY(settings1.scope() == QSettings::UserScope);
        QVERIFY(settings2.scope() == QSettings::UserScope);
        QVERIFY(settings3.scope() == QSettings::SystemScope);
        QVERIFY(settings4.scope() == QSettings::SystemScope);

        // test QSettings::organizationName() while we're at it
        QVERIFY(settings1.organizationName() == "software.org");
        QVERIFY(settings2.organizationName() == "software.org");
        QVERIFY(settings3.organizationName() == "software.org");
        QVERIFY(settings4.organizationName() == "software.org");

        // test QSettings::applicationName() while we're at it
        QCOMPARE(settings1.applicationName(), QString("KillerAPP"));
        QVERIFY(settings2.applicationName().isEmpty());
        QVERIFY(settings3.applicationName() == "KillerAPP");
        QVERIFY(settings4.applicationName().isEmpty());

#if !defined(Q_OS_BLACKBERRY)
        /*
            Go forwards.
        */
        settings4.setValue("key 1", QString("doodah"));
        QCOMPARE(settings1.value("key 1").toString(), QString("doodah"));
        QCOMPARE(settings2.value("key 1").toString(), QString("doodah"));
        QCOMPARE(settings3.value("key 1").toString(), QString("doodah"));
        QCOMPARE(settings4.value("key 1").toString(), QString("doodah"));

        settings3.setValue("key 1", QString("blah"));
        QCOMPARE(settings1.value("key 1").toString(), QString("blah"));
        QCOMPARE(settings2.value("key 1").toString(), QString("doodah"));
        QCOMPARE(settings3.value("key 1").toString(), QString("blah"));
        QCOMPARE(settings4.value("key 1").toString(), QString("doodah"));

        settings2.setValue("key 1", QString("whoa"));
        QCOMPARE(settings1.value("key 1").toString(), QString("whoa"));
        QCOMPARE(settings2.value("key 1").toString(), QString("whoa"));
        QCOMPARE(settings3.value("key 1").toString(), QString("blah"));
        QCOMPARE(settings4.value("key 1").toString(), QString("doodah"));

        settings1.setValue("key 1", QString("gurgle"));
        QCOMPARE(settings1.value("key 1").toString(), QString("gurgle"));
        QCOMPARE(settings2.value("key 1").toString(), QString("whoa"));
        QCOMPARE(settings3.value("key 1").toString(), QString("blah"));
        QCOMPARE(settings4.value("key 1").toString(), QString("doodah"));

        /*
            Test the copies.
        */
        QCOMPARE(settings5.value("key 1").toString(), settings1.value("key 1").toString());
        QCOMPARE(settings6.value("key 1").toString(), settings2.value("key 1").toString());
        QCOMPARE(settings7.value("key 1").toString(), settings3.value("key 1").toString());
        QCOMPARE(settings8.value("key 1").toString(), settings4.value("key 1").toString());

        /*
            Go backwards.
        */

        settings2.setValue("key 1", QString("bilboh"));
        QCOMPARE(settings1.value("key 1").toString(), QString("gurgle"));
        QCOMPARE(settings2.value("key 1").toString(), QString("bilboh"));
        QCOMPARE(settings3.value("key 1").toString(), QString("blah"));
        QCOMPARE(settings4.value("key 1").toString(), QString("doodah"));

        settings3.setValue("key 1", QString("catha"));
        QCOMPARE(settings1.value("key 1").toString(), QString("gurgle"));
        QCOMPARE(settings2.value("key 1").toString(), QString("bilboh"));
        QCOMPARE(settings3.value("key 1").toString(), QString("catha"));
        QCOMPARE(settings4.value("key 1").toString(), QString("doodah"));

        settings4.setValue("key 1", QString("quirko"));
        QCOMPARE(settings1.value("key 1").toString(), QString("gurgle"));
        QCOMPARE(settings2.value("key 1").toString(), QString("bilboh"));
        QCOMPARE(settings3.value("key 1").toString(), QString("catha"));
        QCOMPARE(settings4.value("key 1").toString(), QString("quirko"));
#else
        /*
            No fallback mechanism and a single scope on Blackberry OS
        */
        settings2.setValue("key 1", QString("whoa"));
        QCOMPARE(settings2.value("key 1").toString(), QString("whoa"));
        QCOMPARE(settings4.value("key 1").toString(), QString("whoa"));
        QVERIFY(!settings1.contains("key 1"));
        QVERIFY(!settings3.contains("key 1"));

        settings1.setValue("key 1", QString("blah"));
        QCOMPARE(settings1.value("key 1").toString(), QString("blah"));
        QCOMPARE(settings2.value("key 1").toString(), QString("whoa"));
        QCOMPARE(settings3.value("key 1").toString(), QString("blah"));
        QCOMPARE(settings4.value("key 1").toString(), QString("whoa"));
#endif

        /*
            Test the copies again.
        */
        QCOMPARE(settings5.value("key 1").toString(), settings1.value("key 1").toString());
        QCOMPARE(settings6.value("key 1").toString(), settings2.value("key 1").toString());
        QCOMPARE(settings7.value("key 1").toString(), settings3.value("key 1").toString());
        QCOMPARE(settings8.value("key 1").toString(), settings4.value("key 1").toString());

        /*
            "General" is a problem key for .ini files.
        */
        settings1.setValue("General", 1);
        settings1.setValue("%General", 2);
        settings1.setValue("alpha", 3);
        settings1.setValue("General/alpha", 4);
        settings1.setValue("%General/alpha", 5);
        settings1.setValue("alpha/General", 6);
        settings1.setValue("alpha/%General", 7);
        settings1.setValue("General/General", 8);
        settings1.setValue("General/%General", 9);
        settings1.setValue("%General/General", 10);
        settings1.setValue("%General/%General", 11);
    }

    {
        /*
            Test that the data was stored on disk after all instances
            of QSettings are destroyed.
        */

        QSettings settings1(format, QSettings::UserScope, "software.org", "KillerAPP");
        QSettings settings2(format, QSettings::UserScope, "software.org");
        QSettings settings3(format, QSettings::SystemScope, "software.org", "KillerAPP");
        QSettings settings4(format, QSettings::SystemScope, "software.org");

#if !defined(Q_OS_BLACKBERRY)
        QCOMPARE(settings1.value("key 1").toString(), QString("gurgle"));
        QCOMPARE(settings2.value("key 1").toString(), QString("bilboh"));
        QCOMPARE(settings3.value("key 1").toString(), QString("catha"));
        QCOMPARE(settings4.value("key 1").toString(), QString("quirko"));
#else
        QCOMPARE(settings1.value("key 1").toString(), QString("blah"));
        QCOMPARE(settings2.value("key 1").toString(), QString("whoa"));
        QCOMPARE(settings3.value("key 1").toString(), QString("blah"));
        QCOMPARE(settings4.value("key 1").toString(), QString("whoa"));
#endif

        /*
            Test problem keys.
        */

        QCOMPARE(settings1.value("General").toInt(), 1);
        QCOMPARE(settings1.value("%General").toInt(), 2);
        QCOMPARE(settings1.value("alpha").toInt(), 3);
        QCOMPARE(settings1.value("General/alpha").toInt(), 4);
        QCOMPARE(settings1.value("%General/alpha").toInt(), 5);
        QCOMPARE(settings1.value("alpha/General").toInt(), 6);
        QCOMPARE(settings1.value("alpha/%General").toInt(), 7);
        QCOMPARE(settings1.value("General/General").toInt(), 8);
        QCOMPARE(settings1.value("General/%General").toInt(), 9);
        QCOMPARE(settings1.value("%General/General").toInt(), 10);
        QCOMPARE(settings1.value("%General/%General").toInt(), 11);

        /*
            Test that the organization and product parameters are
            case-insensitive on case-insensitive file systems.
        */
        QSettings settings5(format, QSettings::UserScope, "SoftWare.ORG", "killerApp");

        bool caseSensitive = true;
#if defined(Q_OS_MAC)
        if (format == QSettings::NativeFormat) {
            // more details in QMacSettingsPrivate::QMacSettingsPrivate(), organization was comify()-ed
            caseSensitive = settings5.fileName().contains("SoftWare.ORG");;
        } else {
            caseSensitive = pathconf(QDir::currentPath().toLatin1().constData(), _PC_CASE_SENSITIVE);
        }
#elif defined(Q_OS_WIN32) || defined(Q_OS_WINRT)
        caseSensitive = false;
#endif
        if (caseSensitive)
            QVERIFY(!settings5.contains("key 1"));
        else
            QVERIFY(settings5.contains("key 1"));
    }

    {
        QSettings settings1(settingsPath("custom/custom.conf"), QSettings::IniFormat);
        settings1.beginGroup("alpha/beta");
        settings1.setValue("geometry", -7);
        settings1.setValue("geometry/x", 1);
        settings1.setValue("geometry/y", 2);
        QSettings settings2(settingsPath("custom/custom.conf"), QSettings::IniFormat);
        settings1.setValue("geometry/width", 3);
        settings2.setValue("alpha/beta/geometry/height", 4);
        settings2.setValue("alpha/gamma/splitter", 5);
        settings1.endGroup();

        // test QSettings::scope() while we're at it
        QVERIFY(settings1.scope() == QSettings::UserScope);

        // test QSettings::organizationName() while we're at it
        QVERIFY(settings1.organizationName().isEmpty());

        // test QSettings::applicationName() while we're at it
        QVERIFY(settings1.organizationName().isEmpty());

        QSettings settings3(settingsPath("custom/custom2.conf"), QSettings::IniFormat);
        settings3.beginGroup("doodley/beta");
        settings3.setValue("geometry", -7);
        settings3.setValue("geometry/x", 1);
        settings3.setValue("geometry/y", 2);
        settings3.setValue("geometry/width", 3);
        settings3.setValue("geometry/height", 4);
        settings3.endGroup();
        settings3.setValue("alpha/gamma/splitter", 5);

        QCOMPARE(settings1.value("alpha/beta/geometry").toInt(), -7);
        QCOMPARE(settings1.value("alpha/beta/geometry/x").toInt(), 1);
        QCOMPARE(settings1.value("alpha/beta/geometry/y").toInt(), 2);
        QCOMPARE(settings1.value("alpha/beta/geometry/width").toInt(), 3);
        QCOMPARE(settings1.value("alpha/beta/geometry/height").toInt(), 4);
        QCOMPARE(settings1.value("alpha/gamma/splitter").toInt(), 5);
        QCOMPARE(settings1.allKeys().count(), 6);

        QCOMPARE(settings2.value("alpha/beta/geometry").toInt(), -7);
        QCOMPARE(settings2.value("alpha/beta/geometry/x").toInt(), 1);
        QCOMPARE(settings2.value("alpha/beta/geometry/y").toInt(), 2);
        QCOMPARE(settings2.value("alpha/beta/geometry/width").toInt(), 3);
        QCOMPARE(settings2.value("alpha/beta/geometry/height").toInt(), 4);
        QCOMPARE(settings2.value("alpha/gamma/splitter").toInt(), 5);
        QCOMPARE(settings2.allKeys().count(), 6);
    }

    {
        QSettings settings1(settingsPath("custom/custom.conf"), QSettings::IniFormat);
        QCOMPARE(settings1.value("alpha/beta/geometry").toInt(), -7);
        QCOMPARE(settings1.value("alpha/beta/geometry/x").toInt(), 1);
        QCOMPARE(settings1.value("alpha/beta/geometry/y").toInt(), 2);
        QCOMPARE(settings1.value("alpha/beta/geometry/width").toInt(), 3);
        QCOMPARE(settings1.value("alpha/beta/geometry/height").toInt(), 4);
        QCOMPARE(settings1.value("alpha/gamma/splitter").toInt(), 5);
        QCOMPARE(settings1.allKeys().count(), 6);
    }

    {
        // QSettings's default constructor is native by default
        if (format == QSettings::NativeFormat) {
            QCoreApplication::instance()->setOrganizationName("");
            QCoreApplication::instance()->setApplicationName("");
            QSettings settings;
#if defined(Q_OS_MAC) || defined(Q_OS_WINRT)
            QEXPECT_FAIL("native", "Default settings on Mac/WinRT are valid, despite organization domain, name, and app name being null", Continue);
#endif
            QCOMPARE(settings.status(), QSettings::AccessError);
            QCoreApplication::instance()->setOrganizationName("software.org");
            QCoreApplication::instance()->setApplicationName("KillerAPP");
            QSettings settings2;
            QCOMPARE(settings2.status(), QSettings::NoError);
            QSettings settings3("software.org", "KillerAPP");
            QCOMPARE(settings2.fileName(), settings3.fileName());
            QCoreApplication::instance()->setOrganizationName("");
            QCoreApplication::instance()->setApplicationName("");
        }

        QSettings settings(format, QSettings::UserScope, "", "");
#if defined(Q_OS_MAC) || defined(Q_OS_WINRT)
        QEXPECT_FAIL("native", "Default settings on Mac/WinRT are valid, despite organization domain, name, and app name being null", Continue);
#endif
        QCOMPARE(settings.status(), QSettings::AccessError);
        QSettings settings2(format, QSettings::UserScope, "software.org", "KillerAPP");
        QCOMPARE(settings2.status(), QSettings::NoError);

        // test QSettings::format() while we're at it
        QVERIFY(settings.format() == format);
        QVERIFY(settings2.format() == format);

        // test QSettings::scope() while we're at it
        QVERIFY(settings.scope() == QSettings::UserScope);
        QVERIFY(settings2.scope() == QSettings::UserScope);

        // test QSettings::organizationName() while we're at it
        QVERIFY(settings.organizationName().isEmpty());
        QVERIFY(settings2.organizationName() == "software.org");

        // test QSettings::applicationName() while we're at it
        QVERIFY(settings.applicationName().isEmpty());
        QVERIFY(settings2.applicationName() == "KillerAPP");
    }
}

void tst_QSettings::testByteArray_data()
{
    QTest::addColumn<QByteArray>("data");

    QByteArray bytes("Hello world!");

    QTest::newRow("latin1") << bytes;
#ifndef QT_NO_COMPRESS
    QTest::newRow("compressed") << qCompress(bytes);
#endif
    QTest::newRow("with \\0") << bytes + '\0' + bytes;
}

void tst_QSettings::testByteArray()
{
    QFETCH(QByteArray, data);

    // write
    {
        QSettings settings("QtProject", "tst_qsettings");
        settings.setValue("byteArray", data);
    }
    // read
    {
        QSettings settings("QtProject", "tst_qsettings");
        QByteArray ret = settings.value("byteArray", data).toByteArray();
        QCOMPARE(ret, data);
    }
}

void tst_QSettings::iniCodec()
{
    {
        QSettings settings("QtProject", "tst_qsettings");
        settings.setIniCodec("cp1251");
        QByteArray ba;
        ba.resize(256);
        for (int i = 0; i < ba.size(); i++)
            ba[i] = i;
        settings.setValue("array",ba);
    }
    {
        QSettings settings("QtProject", "tst_qsettings");
        settings.setIniCodec("cp1251");
        QByteArray ba = settings.value("array").toByteArray();
        QCOMPARE(ba.size(), 256);
        for (int i = 0; i < ba.size(); i++)
            QCOMPARE((uchar)ba.at(i), (uchar)i);
    }

}

void tst_QSettings::bom()
{
    QSettings s(":/bom.ini", QSettings::IniFormat);
    QStringList allkeys = s.allKeys();
    QCOMPARE(allkeys.size(), 2);
    QVERIFY(allkeys.contains("section1/foo1"));
    QVERIFY(allkeys.contains("section2/foo2"));
}

void tst_QSettings::testErrorHandling_data()
{
    QTest::addColumn<int>("filePerms"); // -1 means file should not exist
    QTest::addColumn<int>("dirPerms");
    QTest::addColumn<int>("statusAfterCtor");
    QTest::addColumn<bool>("shouldBeEmpty");
    QTest::addColumn<int>("statusAfterGet");
    QTest::addColumn<int>("statusAfterSetAndSync");

    //                         file    dir     afterCtor                      empty     afterGet                      afterSetAndSync
    QTest::newRow("0600 0700") << 0600 << 0700 << (int)QSettings::NoError     << false << (int)QSettings::NoError     << (int)QSettings::NoError;

    QTest::newRow("0400 0700") << 0400 << 0700 << (int)QSettings::NoError
        << false << (int)QSettings::NoError     << (int)QSettings::AccessError;
    QTest::newRow("0200 0700") << 0200 << 0700 << (int)QSettings::AccessError
        << true  << (int)QSettings::AccessError << (int)QSettings::AccessError;

    QTest::newRow("  -1 0700") <<   -1 << 0700 << (int)QSettings::NoError     << true  << (int)QSettings::NoError     << (int)QSettings::NoError;

    QTest::newRow("  -1 0000") <<   -1 << 0000 << (int)QSettings::NoError     << true  << (int)QSettings::NoError     << (int)QSettings::AccessError;
    QTest::newRow("  -1 0100") <<   -1 << 0100 << (int)QSettings::NoError     << true  << (int)QSettings::NoError     << (int)QSettings::AccessError;
    QTest::newRow("0600 0100") << 0600 << 0100 << (int)QSettings::NoError     << false << (int)QSettings::NoError     << (int)QSettings::NoError;
    QTest::newRow("  -1 0300") <<   -1 << 0300 << (int)QSettings::NoError     << true  << (int)QSettings::NoError     << (int)QSettings::NoError;
    QTest::newRow("0600 0300") << 0600 << 0300 << (int)QSettings::NoError     << false << (int)QSettings::NoError     << (int)QSettings::NoError;
    QTest::newRow("  -1 0500") <<   -1 << 0500 << (int)QSettings::NoError     << true  << (int)QSettings::NoError     << (int)QSettings::AccessError;
    QTest::newRow("0600 0500") << 0600 << 0500 << (int)QSettings::NoError     << false << (int)QSettings::NoError     << (int)QSettings::NoError;
}

void tst_QSettings::testErrorHandling()
{
#ifdef Q_OS_WIN
    QSKIP("Windows doesn't support most file modes, including read-only directories, so this test is moot.");
#elif defined(Q_OS_UNIX)
#if !defined(Q_OS_VXWORKS)  // VxWorks does not have users/groups
    if (::getuid() == 0)
#endif
        QSKIP("Running this test as root doesn't work, since file perms do not bother him");
#else
    QFETCH(int, filePerms);
    QFETCH(int, dirPerms);
    QFETCH(int, statusAfterCtor);
    QFETCH(bool, shouldBeEmpty);
    QFETCH(int, statusAfterGet);
    QFETCH(int, statusAfterSetAndSync);

    system(QString("chmod 700 %1 2>/dev/null").arg(settingsPath("someDir")).toLatin1());
    system(QString("chmod -R u+rwx %1 2>/dev/null").arg(settingsPath("someDir")).toLatin1());
    system(QString("rm -fr %1").arg(settingsPath("someDir")).toLatin1());

    // prepare a file with some settings
    if (filePerms != -1) {
        QSettings settings(settingsPath("someDir/someSettings.ini"), QSettings::IniFormat);
        QCOMPARE((int) settings.status(), (int) QSettings::NoError);

        settings.beginGroup("alpha/beta");
        settings.setValue("geometry", -7);
        settings.setValue("geometry/x", 1);
        settings.setValue("geometry/y", 2);
        settings.setValue("geometry/width", 3);
        settings.setValue("geometry/height", 4);
        settings.endGroup();
        settings.setValue("alpha/gamma/splitter", 5);
    } else {
        system(QString("mkdir -p %1").arg(settingsPath("someDir")).toLatin1());
    }

    if (filePerms != -1) {
        system(QString("chmod %1 %2")
                    .arg(QString::number(filePerms, 8))
                    .arg(settingsPath("someDir/someSettings.ini"))
                    .toLatin1());
    }
    system(QString("chmod %1 %2")
                .arg(QString::number(dirPerms, 8))
                .arg(settingsPath("someDir"))
                .toLatin1());

    // the test
    {
        QConfFile::clearCache();
        QSettings settings(settingsPath("someDir/someSettings.ini"), QSettings::IniFormat);
        QCOMPARE((int)settings.status(), statusAfterCtor);
        if (shouldBeEmpty) {
            QCOMPARE(settings.allKeys().count(), 0);
        } else {
            QVERIFY(settings.allKeys().count() > 0);
        }
        settings.value("alpha/beta/geometry");
        QCOMPARE((int)settings.status(), statusAfterGet);
        settings.setValue("alpha/beta/geometry", 100);
        QCOMPARE((int)settings.status(), statusAfterGet);
        QCOMPARE(settings.value("alpha/beta/geometry").toInt(), 100);
        settings.sync();
        QCOMPARE(settings.value("alpha/beta/geometry").toInt(), 100);
        QCOMPARE((int)settings.status(), statusAfterSetAndSync);
    }
#endif // !Q_OS_WIN
}

Q_DECLARE_METATYPE(QSettings::Status)

#ifdef QT_BUILD_INTERNAL
void tst_QSettings::testIniParsing_data()
{
    QTest::addColumn<QByteArray>("inicontent");
    QTest::addColumn<QString>("key");
    QTest::addColumn<QVariant>("expect");
    QTest::addColumn<QSettings::Status>("status");

    // Test "forgiving" parsing of entries not terminated with newline or unterminated strings
    QTest::newRow("good1")    << QByteArray("v=1\n")          << "v" << QVariant(1) << QSettings::NoError;
    QTest::newRow("good2")    << QByteArray("v=1\\\n2")       << "v" << QVariant(12) << QSettings::NoError;
    QTest::newRow("good3")    << QByteArray("v=1\\\r2")       << "v" << QVariant(12) << QSettings::NoError;
    QTest::newRow("good4")    << QByteArray("v=1\\\n\r2")     << "v" << QVariant(12) << QSettings::NoError;
    QTest::newRow("good5")    << QByteArray("v=1\\\r\n2")     << "v" << QVariant(12) << QSettings::NoError;
    QTest::newRow("good6")    << QByteArray("v  \t = \t 1\\\r\n2")     << "v" << QVariant(12) << QSettings::NoError;
    QTest::newRow("garbage1") << QByteArray("v")              << "v" << QVariant() << QSettings::FormatError;
    QTest::newRow("nonterm1") << QByteArray("v=str")          << "v" << QVariant("str") << QSettings::NoError;
    QTest::newRow("nonterm2") << QByteArray("v=\"str\"")      << "v" << QVariant("str") << QSettings::NoError;
    QTest::newRow("nonterm3") << QByteArray("v=\"str")        << "v" << QVariant("str") << QSettings::NoError;
    QTest::newRow("nonterm4") << QByteArray("v=\\")           << "v" << QVariant("") << QSettings::NoError;
    QTest::newRow("nonterm5") << QByteArray("u=s\nv=\"str")   << "v" << QVariant("str") << QSettings::NoError;
    QTest::newRow("nonterm6") << QByteArray("v=\"str\nw=ok")  << "v" << QVariant("str\nw=ok") << QSettings::NoError;
    QTest::newRow("nonterm7") << QByteArray("v=")             << "v" << QVariant("") << QSettings::NoError;
    QTest::newRow("nonterm8") << QByteArray("v=\"str\njnk")   << "v" << QVariant("str\njnk") << QSettings::NoError;
    QTest::newRow("nonterm9") << QByteArray("v=1\\")          << "v" << QVariant(1) << QSettings::NoError;
    QTest::newRow("nonterm10") << QByteArray("v=1\\\n")       << "v" << QVariant(1) << QSettings::NoError;
    QTest::newRow("nonterm11") << QByteArray("v=1\\\r")       << "v" << QVariant(1) << QSettings::NoError;
    QTest::newRow("nonterm12") << QByteArray("v=1\\\n\r")     << "v" << QVariant(1) << QSettings::NoError;
    QTest::newRow("nonterm13") << QByteArray("v=1\\\r\n")     << "v" << QVariant(1) << QSettings::NoError;
    QTest::newRow("nonterm14") << QByteArray("v=1\\\n\nx=2")  << "v" << QVariant(1) << QSettings::NoError;
    QTest::newRow("nonterm15") << QByteArray("v=1\\\r\rx=2")  << "v" << QVariant(1) << QSettings::NoError;
    QTest::newRow("nonterm16") << QByteArray("v=1\\\n\n\nx=2") << "v" << QVariant(1) << QSettings::NoError;
    QTest::newRow("nonterm17") << QByteArray("; foo\nv=1") << "v" << QVariant(1) << QSettings::NoError;
    QTest::newRow("nonterm18") << QByteArray("; foo\n\nv=1") << "v" << QVariant(1) << QSettings::NoError;
    QTest::newRow("nonterm19") << QByteArray("\nv=1;foo") << "v" << QVariant(1) << QSettings::NoError;
    QTest::newRow("nonterm20") << QByteArray("v=x ") << "v" << QVariant("x") << QSettings::NoError;
    QTest::newRow("nonterm21") << QByteArray("v=x ;") << "v" << QVariant("x") << QSettings::NoError;
}
#endif

#ifdef QT_BUILD_INTERNAL
void tst_QSettings::testIniParsing()
{
    qRegisterMetaType<QSettings::Status>("QSettings::Status");

    QDir dir(settingsPath());
    QVERIFY(dir.mkpath("someDir"));
    QFile f(dir.path()+"/someDir/someSettings.ini");

    QFETCH(QByteArray, inicontent);
    QFETCH(QString, key);
    QFETCH(QVariant, expect);
    QFETCH(QSettings::Status, status);

    QVERIFY(f.open(QFile::WriteOnly));
    f.write(inicontent);
    f.close();

    QConfFile::clearCache();
    QSettings settings(settingsPath("someDir/someSettings.ini"), QSettings::IniFormat);

    if ( settings.status() == QSettings::NoError ) { // else no point proceeding
        QVariant v = settings.value(key);
        QVERIFY(v.canConvert(expect.type()));
        // check some types so as to give prettier error messages
        if ( v.type() == QVariant::String ) {
            QCOMPARE(v.toString(), expect.toString());
        } else if ( v.type() == QVariant::Int ) {
            QCOMPARE(v.toInt(), expect.toInt());
        } else {
            QCOMPARE(v, expect);
        }
    }

    QCOMPARE(settings.status(), status);
}
#endif

/*
    Tests beginGroup(), endGroup(), and group().
*/
void tst_QSettings::beginGroup()
{
    QSettings settings1(QSettings::UserScope, "software.org", "KillerAPP");
    QSettings settings2(QSettings::UserScope, "software.org", "KillerAPP");

    /*
      Let's start with some back and forthing.
    */

    settings1.beginGroup("alpha");
    QCOMPARE(settings1.group(), QString("alpha"));
    settings1.endGroup();
    QCOMPARE(settings1.group(), QString());
    settings1.beginGroup("/beta");
    QCOMPARE(settings1.group(), QString("beta"));
    settings1.endGroup();
    QCOMPARE(settings1.group(), QString());

    settings1.beginGroup("///gamma//");
    QCOMPARE(settings1.group(), QString("gamma"));
    settings1.endGroup();
    QCOMPARE(settings1.group(), QString());

    settings1.setValue("geometry", 5);
    QCOMPARE(settings1.value("geometry").toInt(), 5);
    QCOMPARE(settings1.value("/geometry///").toInt(), 5);
    QCOMPARE(settings2.value("geometry").toInt(), 5);
    QCOMPARE(settings2.value("/geometry///").toInt(), 5);

    /*
      OK, now start for real.
    */

    settings1.beginGroup("alpha");
    QCOMPARE(settings1.group(), QString("alpha"));
    settings1.setValue("geometry", 66);
    QCOMPARE(settings1.value("geometry").toInt(), 66);
    QCOMPARE(settings2.value("geometry").toInt(), 5);
    QCOMPARE(settings2.value("alpha/geometry").toInt(), 66);

    QSettings settings3(QSettings::UserScope, "software.org", "KillerAPP");
    settings3.beginGroup("alpha");
    QCOMPARE(settings3.value("geometry").toInt(), 66);

    settings1.beginGroup("/beta///");
    QCOMPARE(settings1.group(), QString("alpha/beta"));
    settings1.setValue("geometry", 777);
    QCOMPARE(settings1.value("geometry").toInt(), 777);
    QCOMPARE(settings2.value("geometry").toInt(), 5);
    QCOMPARE(settings2.value("alpha/geometry").toInt(), 66);
    QCOMPARE(settings2.value("alpha/beta/geometry").toInt(), 777);
    QCOMPARE(settings3.value("geometry").toInt(), 66);
    QCOMPARE(settings3.value("beta/geometry").toInt(), 777);

    settings3.beginGroup("gamma");
    settings3.setValue("geometry", 8888);
    QCOMPARE(settings3.value("geometry").toInt(), 8888);
    QCOMPARE(settings2.value("geometry").toInt(), 5);
    QCOMPARE(settings2.value("alpha/geometry").toInt(), 66);
    QCOMPARE(settings2.value("alpha/beta/geometry").toInt(), 777);
    QCOMPARE(settings2.value("alpha/gamma/geometry").toInt(), 8888);
    QCOMPARE(settings1.value("geometry").toInt(), 777);

    // endGroup() should do nothing if group() is empty
    for (int i = 0; i < 10; ++i) {
        QTest::ignoreMessage(QtWarningMsg, "QSettings::endGroup: No matching beginGroup()");
        settings2.endGroup();
    }
    QCOMPARE(settings2.value("geometry").toInt(), 5);
    QCOMPARE(settings2.value("alpha/geometry").toInt(), 66);
    QCOMPARE(settings2.value("alpha/beta/geometry").toInt(), 777);
    QCOMPARE(settings2.value("alpha/gamma/geometry").toInt(), 8888);

    QCOMPARE(settings1.group(), QString("alpha/beta"));
    settings1.endGroup();
    QCOMPARE(settings1.group(), QString("alpha"));
    settings1.endGroup();
    QCOMPARE(settings1.group(), QString());
    QCOMPARE(settings1.value("geometry").toInt(), 5);
    QCOMPARE(settings1.value("alpha/geometry").toInt(), 66);
    QCOMPARE(settings1.value("alpha/beta/geometry").toInt(), 777);
    QCOMPARE(settings1.value("alpha/gamma/geometry").toInt(), 8888);

    settings1.beginGroup("delta");
    QCOMPARE(settings1.group(), QString("delta"));
    settings1.beginGroup("");
    QCOMPARE(settings1.group(), QString("delta"));
    settings1.beginGroup("/");
    QCOMPARE(settings1.group(), QString("delta"));
    settings1.beginGroup("////");
    QCOMPARE(settings1.group(), QString("delta"));
    settings1.beginGroup("////omega///epsilon zeta eta  theta/ / /");
    QCOMPARE(settings1.group(), QString("delta/omega/epsilon zeta eta  theta/ / "));
    settings1.endGroup();
    QCOMPARE(settings1.group(), QString("delta"));
    settings1.endGroup();
    QCOMPARE(settings1.group(), QString("delta"));
    settings1.endGroup();
    QCOMPARE(settings1.group(), QString("delta"));
    settings1.endGroup();
    QCOMPARE(settings1.group(), QString("delta"));
    settings1.endGroup();
    QCOMPARE(settings1.group(), QString());
}

/*
    Tests setValue() and getXxx().
*/
void tst_QSettings::setValue()
{
    QSettings settings(QSettings::UserScope, "software.org", "KillerAPP");

    settings.setValue("key 2", (int)0x7fffffff);
    QCOMPARE(settings.value("key 2").toInt(), (int)0x7fffffff);
    QCOMPARE(settings.value("key 2").toString(), QString::number((int)0x7fffffff));
    settings.setValue("key 2", -1);
    QCOMPARE(settings.value("key 2").toInt(), -1);
    QCOMPARE(settings.value("key 2").toString(), QString("-1"));
    settings.setValue("key 2", (int)0x80000000);
    QCOMPARE(settings.value("key 2").toInt(), (int)0x80000000);
    settings.setValue("key 2", (int)0);
    QCOMPARE(settings.value("key 2", 123).toInt(), (int)0);
    settings.setValue("key 2", (int)12345);
    QCOMPARE(settings.value("key 2").toInt(), (int)12345);
    QCOMPARE(settings.value("no such key", 1234).toInt(), (int)1234);
    QCOMPARE(settings.value("no such key").toInt(), (int)0);

    settings.setValue("key 2", true);
    QCOMPARE(settings.value("key 2").toBool(), true);
    settings.setValue("key 2", false);
    QCOMPARE(settings.value("key 2", true).toBool(), false);
    settings.setValue("key 2", (int)1);
    QCOMPARE(settings.value("key 2").toBool(), true);
    settings.setValue("key 2", (int)-1);
    QCOMPARE(settings.value("key 2").toBool(), true);
    settings.setValue("key 2", (int)0);
    QCOMPARE(settings.value("key 2", true).toBool(), false);
    settings.setValue("key 2", QString("true"));
    QCOMPARE(settings.value("key 2").toBool(), true);
    settings.setValue("key 2", QString("false"));
    QCOMPARE(settings.value("key 2", true).toBool(), false);

    // The following block should not compile.
/*
    settings.setValue("key 2", "true");
    QCOMPARE(settings.value("key 2").toBool(), true);
    settings.setValue("key 2", "false");
    QCOMPARE(settings.value("key 2", true).toBool(), false);
    settings.setValue("key 2", "");
    QCOMPARE(settings.value("key 2", true).toBool(), true);
    settings.setValue("key 2", "");
    QCOMPARE(settings.value("key 2", false).toBool(), false);
    settings.setValue("key 2", "0.000e-00"); // cannot convert double to a bool
    QCOMPARE(settings.value("key 2", true).toBool(), true);
    settings.setValue("key 2", "0.000e-00");
    QCOMPARE(settings.value("key 2", false).toBool(), false);
*/

    settings.setValue("key 2", QStringList());
    QCOMPARE(settings.value("key 2").toStringList(), QStringList());
    settings.setValue("key 2", QStringList(""));
    QCOMPARE(settings.value("key 2").toStringList(), QStringList(""));
    settings.setValue("key 2", QStringList() << "" << "");
    QCOMPARE(settings.value("key 2").toStringList(), QStringList() << "" << "");
    settings.setValue("key 2", QStringList() << "" << "a" << "" << "bc" << "");
    QCOMPARE(settings.value("key 2").toStringList(), QStringList() << "" << "a" << "" << "bc" << "");

    settings.setValue("key 3", QList<QVariant>());
    QCOMPARE(settings.value("key 3").toList(), QList<QVariant>());
    settings.setValue("key 3", QList<QVariant>() << 1 << QString("a"));
    QCOMPARE(settings.value("key 3").toList(), QList<QVariant>() << 1 << QString("a"));

    QList<QVariant> outerList;
    outerList << 1 << QString("b");
    QList<QVariant> innerList = outerList;
    outerList.append(QVariant(innerList));
    outerList.append(QVariant(innerList));
    outerList << 2 << QString("c");
    innerList = outerList;
    outerList.append(QVariant(innerList));
    // outerList: [1, "b", [1, "b"], [1, "b"], 2, "c", [1, "b", [1, "b"], [1, "b"], 2, "c"]]

    settings.setValue("key 3", outerList);
    QCOMPARE(settings.value("key 3").toList(), outerList);
    QCOMPARE(settings.value("key 3").toList().size(), 7);

    QMap<QString, QVariant> map;
    map.insert("1", "one");
    map.insert("2", "two");
    map.insert("3", outerList);
    map.insert("5", "cinco");
    map.insert("10", "zehn");
    settings.setValue("key 4", map);
    QCOMPARE(settings.value("key 4").toMap(), map);
}

#ifdef QT_BUILD_INTERNAL
void tst_QSettings::testVariantTypes_data()
{
    populateWithFormats();
}
#endif

#ifdef QT_BUILD_INTERNAL
void tst_QSettings::testVariantTypes()
{
#define testVal(key, val, tp, rtype) \
    { \
        QSettings settings1(format, QSettings::UserScope, "software.org", "KillerAPP"); \
        settings1.setValue(key, QVariant::fromValue(val)); \
    } \
    QConfFile::clearCache(); \
    { \
        QSettings settings2(format, QSettings::UserScope, "software.org", "KillerAPP"); \
        QVariant v = settings2.value(key); \
        QVERIFY(qvariant_cast<tp >(v) == val); \
        QVERIFY(v.type() == QVariant::rtype); \
    }

    typedef QMap<QString, QVariant> TestVariantMap;

    QFETCH(QSettings::Format, format);

    TestVariantMap m2;
    m2.insert("ene", "due");
    m2.insert("rike", "fake");
    m2.insert("borba", "dorba");
    testVal("key2", m2, TestVariantMap, Map);

    QStringList l2;

    l2 << "ene" << "due" << "@Point(1 2)" << "@fake";
    testVal("key3", l2, QStringList, StringList);

    l2.clear();
    l2 << "ene" << "due" << "rike" << "fake";
    testVal("key3", l2, QStringList, StringList);

    QList<QVariant> l3;
    QDate date = QDate::currentDate();
    QTime time = QTime::currentTime();
    l3 << QString("ene") << 10 << QVariant::fromValue(QColor(1, 2, 3)) << QVariant(QRect(1, 2, 3, 4))
        << QVariant(QSize(4, 56)) << QVariant(QPoint(4, 2)) << true << false << date << time;
    testVal("key3", l3, QVariantList, List);

    testVal("key4", QString("hello"), QString, String);
    testVal("key5", QColor(1, 2, 3), QColor, Color);
    testVal("key6", QRect(1, 2, 3, 4), QRect, Rect);
    testVal("key7", QSize(4, 56), QSize, Size);
    testVal("key8", QPoint(4, 2), QPoint, Point);
    testVal("key10", date, QDate, Date);
    testVal("key11", time, QTime, Time);
    testVal("key12", QByteArray("foo bar"), QByteArray, ByteArray);

    {
        QSettings settings(format, QSettings::UserScope, "software.org", "KillerAPP");
        QVERIFY(!settings.contains("key99"));
        QCOMPARE(settings.value("key99"), QVariant());

        settings.setValue("key99", QVariant());
        QVERIFY(settings.contains("key99"));
        QCOMPARE(settings.value("key99"), QVariant());

        settings.setValue("key99", QVariant(1));
        QVERIFY(settings.contains("key99"));
        QCOMPARE(settings.value("key99"), QVariant(1));

        settings.setValue("key99", QVariant());
        QVERIFY(settings.contains("key99"));
        QCOMPARE(settings.value("key99"), QVariant());

        settings.remove("key99");
        QVERIFY(!settings.contains("key99"));
        QCOMPARE(settings.value("key99"), QVariant());
    }

    QList<QVariant> l4;
    l4 << QVariant(m2) << QVariant(l2) << QVariant(l3);
    testVal("key13", l4, QVariantList, List);
    QDateTime dt = QDateTime::currentDateTime();
    dt.setOffsetFromUtc(3600);
    testVal("key14", dt, QDateTime, DateTime);

    // We store key sequences as strings instead of binary variant blob, for improved
    // readability in the resulting format.
    if (format >= QSettings::InvalidFormat) {
        testVal("keysequence", QKeySequence(Qt::ControlModifier + Qt::Key_F1), QKeySequence, KeySequence);
    } else {
        testVal("keysequence", QKeySequence(Qt::ControlModifier + Qt::Key_F1), QString, String);
    }

#undef testVal
}
#endif

void tst_QSettings::remove()
{
    QSettings settings0(QSettings::UserScope, "software.org", "KillerAPP");
    int initialNumKeys = settings0.allKeys().size();
    QCOMPARE(settings0.value("key 1", "123").toString(), QString("123"));
    settings0.remove("key 1");
    QCOMPARE(settings0.value("key 1", "456").toString(), QString("456"));

    settings0.setValue("key 1", "bubloo");
    QCOMPARE(settings0.value("key 1").toString(), QString("bubloo"));
    settings0.remove("key 2");
    QCOMPARE(settings0.value("key 1").toString(), QString("bubloo"));
    settings0.remove("key 1");
    QCOMPARE(settings0.value("key 1", "789").toString(), QString("789"));

    /*
      Make sure that removing a key removes all the subkeys.
    */
    settings0.setValue("alpha/beta/geometry", -7);
    settings0.setValue("alpha/beta/geometry/x", 1);
    settings0.setValue("alpha/beta/geometry/y", 2);
    settings0.setValue("alpha/beta/geometry/width", 3);
    settings0.setValue("alpha/beta/geometry/height", 4);
    settings0.setValue("alpha/gamma/splitter", 5);

    settings0.remove("alpha/beta/geometry/x");
    QCOMPARE(settings0.value("alpha/beta/geometry").toInt(), -7);
    QCOMPARE(settings0.value("alpha/beta/geometry/x", 999).toInt(), 999);
    QCOMPARE(settings0.value("alpha/beta/geometry/y").toInt(), 2);
    QCOMPARE(settings0.value("alpha/beta/geometry/width").toInt(), 3);
    QCOMPARE(settings0.value("alpha/beta/geometry/height").toInt(), 4);
    QCOMPARE(settings0.value("alpha/gamma/splitter").toInt(), 5);

    settings0.remove("alpha/beta/geometry");
    QCOMPARE(settings0.value("alpha/beta/geometry", 777).toInt(), 777);
    QCOMPARE(settings0.value("alpha/beta/geometry/x", 111).toInt(), 111);
    QCOMPARE(settings0.value("alpha/beta/geometry/y", 222).toInt(), 222);
    QCOMPARE(settings0.value("alpha/beta/geometry/width", 333).toInt(), 333);
    QCOMPARE(settings0.value("alpha/beta/geometry/height", 444).toInt(), 444);
    QCOMPARE(settings0.value("alpha/gamma/splitter").toInt(), 5);

    settings0.setValue("alpha/beta/geometry", -7);
    settings0.setValue("alpha/beta/geometry/x", 1);
    settings0.setValue("alpha/beta/geometry/y", 2);
    settings0.setValue("alpha/beta/geometry/width", 3);
    settings0.setValue("alpha/beta/geometry/height", 4);
    settings0.setValue("alpha/gamma/splitter", 5);
    QCOMPARE(settings0.allKeys().size(), initialNumKeys + 6);

    settings0.beginGroup("alpha/beta/geometry");
    settings0.remove("");
    settings0.endGroup();
    QVERIFY(!settings0.contains("alpha/beta/geometry"));
    QVERIFY(!settings0.contains("alpha/beta/geometry/x"));
    QVERIFY(!settings0.contains("alpha/beta/geometry/y"));
    QVERIFY(!settings0.contains("alpha/beta/geometry/width"));
    QVERIFY(!settings0.contains("alpha/beta/geometry/height"));
    QVERIFY(settings0.contains("alpha/gamma/splitter"));
    QCOMPARE(settings0.allKeys().size(), initialNumKeys + 1);

    settings0.beginGroup("alpha/beta");
    settings0.remove("");
    settings0.endGroup();
    QVERIFY(!settings0.contains("alpha/beta/geometry"));
    QVERIFY(!settings0.contains("alpha/beta/geometry/x"));
    QVERIFY(!settings0.contains("alpha/beta/geometry/y"));
    QVERIFY(!settings0.contains("alpha/beta/geometry/width"));
    QVERIFY(!settings0.contains("alpha/beta/geometry/height"));
    QVERIFY(settings0.contains("alpha/gamma/splitter"));
    QCOMPARE(settings0.allKeys().size(), initialNumKeys + 1);

    settings0.remove("");
    QVERIFY(!settings0.contains("alpha/gamma/splitter"));
    QCOMPARE(settings0.allKeys().size(), initialNumKeys);

    /*
      Do it again, but this time let's use setGroup().
    */

    settings0.setValue("alpha/beta/geometry", -7);
    settings0.setValue("alpha/beta/geometry/x", 1);
    settings0.setValue("alpha/beta/geometry/y", 2);
    settings0.setValue("alpha/beta/geometry/width", 3);
    settings0.setValue("alpha/beta/geometry/height", 4);
    settings0.setValue("alpha/gamma/splitter", 5);

    settings0.beginGroup("foo/bar/baz/doesn't");
    settings0.remove("exist");
    settings0.endGroup();
    QCOMPARE(settings0.value("alpha/beta/geometry").toInt(), -7);
    QCOMPARE(settings0.value("alpha/beta/geometry/x").toInt(), 1);
    QCOMPARE(settings0.value("alpha/beta/geometry/y").toInt(), 2);
    QCOMPARE(settings0.value("alpha/beta/geometry/width").toInt(), 3);
    QCOMPARE(settings0.value("alpha/beta/geometry/height").toInt(), 4);
    QCOMPARE(settings0.value("alpha/gamma/splitter").toInt(), 5);

    settings0.beginGroup("alpha/beta/geometry");
    settings0.remove("x");
    settings0.endGroup();
    QCOMPARE(settings0.value("alpha/beta/geometry").toInt(), -7);
    QCOMPARE(settings0.value("alpha/beta/geometry/x", 999).toInt(), 999);
    QCOMPARE(settings0.value("alpha/beta/geometry/y").toInt(), 2);
    QCOMPARE(settings0.value("alpha/beta/geometry/width").toInt(), 3);
    QCOMPARE(settings0.value("alpha/beta/geometry/height").toInt(), 4);
    QCOMPARE(settings0.value("alpha/gamma/splitter").toInt(), 5);

    settings0.remove("alpha/beta");
    QCOMPARE(settings0.value("alpha/beta/geometry", 777).toInt(), 777);
    QCOMPARE(settings0.value("alpha/beta/geometry/x", 111).toInt(), 111);
    QCOMPARE(settings0.value("alpha/beta/geometry/y", 222).toInt(), 222);
    QCOMPARE(settings0.value("alpha/beta/geometry/width", 333).toInt(), 333);
    QCOMPARE(settings0.value("alpha/beta/geometry/height", 444).toInt(), 444);
    QCOMPARE(settings0.value("alpha/gamma/splitter").toInt(), 5);

    settings0.clear();
    QCOMPARE(settings0.value("alpha/gamma/splitter", 888).toInt(), 888);

    /*
      OK, now let's check what happens if settings are spread across
      multiple files (user vs. global, product-specific vs.
      company-wide).
    */

    QSettings settings1(QSettings::UserScope, "software.org", "KillerAPP");
    QSettings settings2(QSettings::UserScope, "software.org");

    QScopedPointer<QSettings> settings3;
    QScopedPointer<QSettings> settings4;

    if (m_canWriteNativeSystemSettings) {
        settings3.reset(new QSettings(QSettings::SystemScope, "software.org", "KillerAPP"));
        settings4.reset(new QSettings(QSettings::SystemScope, "software.org"));
        settings3->setValue("key 1", "blah");
        settings4->setValue("key 1", "doodah");
    }

    settings2.setValue("key 1", "whoa");
    settings1.setValue("key 1", "gurgle");
    QCOMPARE(settings1.value("key 1").toString(), QString("gurgle"));
    QCOMPARE(settings2.value("key 1").toString(), QString("whoa"));

#if !defined(Q_OS_BLACKBERRY)
    if (m_canWriteNativeSystemSettings) {
        QCOMPARE(settings3->value("key 1").toString(), QString("blah"));
        QCOMPARE(settings4->value("key 1").toString(), QString("doodah"));
    }

    settings1.remove("key 1");
    QCOMPARE(settings1.value("key 1").toString(), QString("whoa"));
    QCOMPARE(settings2.value("key 1").toString(), QString("whoa"));
    if (m_canWriteNativeSystemSettings) {
        QCOMPARE(settings3->value("key 1").toString(), QString("blah"));
        QCOMPARE(settings4->value("key 1").toString(), QString("doodah"));
    }

    if (m_canWriteNativeSystemSettings) {
        settings2.remove("key 1");
        QCOMPARE(settings1.value("key 1").toString(), QString("blah"));
        QCOMPARE(settings2.value("key 1").toString(), QString("doodah"));
        QCOMPARE(settings3->value("key 1").toString(), QString("blah"));
        QCOMPARE(settings4->value("key 1").toString(), QString("doodah"));

        settings3->remove("key 1");
        QCOMPARE(settings1.value("key 1").toString(), QString("doodah"));
        QCOMPARE(settings2.value("key 1").toString(), QString("doodah"));
        QCOMPARE(settings3->value("key 1").toString(), QString("doodah"));
        QCOMPARE(settings4->value("key 1").toString(), QString("doodah"));

        settings4->remove("key 1");
        QVERIFY(!settings1.contains("key 1"));
        QVERIFY(!settings2.contains("key 1"));
        QVERIFY(!settings3->contains("key 1"));
        QVERIFY(!settings4->contains("key 1"));
    }
#else
    settings1.remove("key 1");
    QCOMPARE(settings2.value("key 1").toString(), QString("whoa"));

    settings2.remove("key 1");
    QVERIFY(!settings1.contains("key 1"));
    QVERIFY(!settings2.contains("key 1"));
#endif

    /*
      Get ready for the next part of the test.
    */

    settings1.clear();
    settings2.clear();
    if (m_canWriteNativeSystemSettings) {
        settings3->clear();
        settings4->clear();
    }

    settings1.sync();
    settings2.sync();

    if (m_canWriteNativeSystemSettings) {
        settings3->sync();
        settings4->sync();
    }

    /*
      Check that recursive removes work correctly when some of the
      keys are loaded from the file and others have been modified in
      memory (corresponds to originalKeys vs. addedKeys in the
      QSettingsFile code).
    */

    settings1.setValue("alpha/beta/geometry", -7);
    settings1.setValue("alpha/beta/geometry/x", 1);
    settings1.setValue("alpha/beta/geometry/y", 2);
    settings1.setValue("alpha/gamma/splitter", 5);
    settings1.sync();

    settings1.setValue("alpha/beta/geometry/width", 3);
    settings1.setValue("alpha/beta/geometry/height", 4);

    settings1.remove("alpha/beta/geometry/y");
    QVERIFY(settings1.contains("alpha/beta/geometry"));
    QVERIFY(settings1.contains("alpha/beta/geometry/x"));
    QVERIFY(!settings1.contains("alpha/beta/geometry/y"));
    QVERIFY(settings1.contains("alpha/beta/geometry/width"));
    QVERIFY(settings1.contains("alpha/beta/geometry/height"));
    QCOMPARE(settings1.allKeys().size(), initialNumKeys + 5);

    settings1.remove("alpha/beta/geometry/y");
    QCOMPARE(settings1.allKeys().size(), initialNumKeys + 5);

    settings1.remove("alpha/beta/geometry/height");
    QVERIFY(settings1.contains("alpha/beta/geometry"));
    QVERIFY(settings1.contains("alpha/beta/geometry/x"));
    QVERIFY(!settings1.contains("alpha/beta/geometry/y"));
    QVERIFY(settings1.contains("alpha/beta/geometry/width"));
    QVERIFY(!settings1.contains("alpha/beta/geometry/height"));
    QCOMPARE(settings1.allKeys().size(), initialNumKeys + 4);

    settings1.remove("alpha/beta/geometry");
    QVERIFY(!settings1.contains("alpha/beta/geometry"));
    QVERIFY(!settings1.contains("alpha/beta/geometry/x"));
    QVERIFY(!settings1.contains("alpha/beta/geometry/y"));
    QVERIFY(!settings1.contains("alpha/beta/geometry/width"));
    QVERIFY(!settings1.contains("alpha/beta/geometry/height"));
    QVERIFY(settings1.contains("alpha/gamma/splitter"));
    QCOMPARE(settings1.allKeys().size(), initialNumKeys + 1);

    settings1.sync();
    QVERIFY(!settings1.contains("alpha/beta/geometry"));
    QVERIFY(!settings1.contains("alpha/beta/geometry/x"));
    QVERIFY(!settings1.contains("alpha/beta/geometry/y"));
    QVERIFY(!settings1.contains("alpha/beta/geometry/width"));
    QVERIFY(!settings1.contains("alpha/beta/geometry/height"));
    QVERIFY(settings1.contains("alpha/gamma/splitter"));
    QCOMPARE(settings1.allKeys().size(), initialNumKeys + 1);
}

/*
    Tests contains() and keys().
*/
void tst_QSettings::contains()
{
    QSettings settings1(QSettings::UserScope, "software.org", "KillerAPP");
    int initialNumKeys = settings1.allKeys().size(); // 0 on all platforms but OS X.
    settings1.setValue("alpha/beta/geometry", -7);
    settings1.setValue("alpha/beta/geometry/x", 1);
    settings1.setValue("alpha/beta/geometry/y", 2);
    settings1.setValue("alpha/beta/geometry/width", 3);
    settings1.setValue("alpha/beta/geometry/height", 4);
    settings1.setValue("alpha/gamma/splitter", 5);
    settings1.setValue("alpha/gamma/splitter/ /", 5);

    QVERIFY(!settings1.contains("alpha"));
    QVERIFY(!settings1.contains("alpha/beta"));
    QVERIFY(!settings1.contains("///alpha///beta///"));
    QVERIFY(settings1.contains("alpha/beta/geometry"));
    QVERIFY(settings1.contains("///alpha///beta//geometry//"));
    QVERIFY(settings1.contains("alpha/beta/geometry/x"));
    QVERIFY(settings1.contains("alpha/beta/geometry/y"));
    QVERIFY(settings1.contains("alpha/beta/geometry/width"));
    QVERIFY(settings1.contains("alpha/beta/geometry/height"));
    QVERIFY(!settings1.contains("alpha/beta/geometry/height/foo/bar/doesn't/exist"));
    QVERIFY(!settings1.contains("alpha/gamma"));
    QVERIFY(settings1.contains("alpha/gamma/splitter"));
    QVERIFY(settings1.contains("alpha/gamma/splitter/ "));
    QVERIFY(settings1.contains("////alpha/gamma/splitter// ////"));

    settings1.beginGroup("alpha");
    QVERIFY(!settings1.contains("beta"));
    QVERIFY(!settings1.contains("/////beta///"));
    QVERIFY(settings1.contains("beta/geometry"));
    QVERIFY(settings1.contains("/////beta//geometry//"));
    QVERIFY(settings1.contains("beta/geometry/x"));
    QVERIFY(settings1.contains("beta/geometry/y"));
    QVERIFY(settings1.contains("beta/geometry/width"));
    QVERIFY(settings1.contains("beta/geometry/height"));
    QVERIFY(!settings1.contains("beta/geometry/height/foo/bar/doesn't/exist"));
    QVERIFY(!settings1.contains("gamma"));
    QVERIFY(settings1.contains("gamma/splitter"));
    QVERIFY(settings1.contains("gamma/splitter/ "));
    QVERIFY(settings1.contains("////gamma/splitter// ////"));

    settings1.beginGroup("beta/geometry");
    QVERIFY(settings1.contains("x"));
    QVERIFY(settings1.contains("y"));
    QVERIFY(settings1.contains("width"));
    QVERIFY(settings1.contains("height"));
    QVERIFY(!settings1.contains("height/foo/bar/doesn't/exist"));

    QStringList keys = settings1.allKeys();
    QStringList expectedResult = QStringList() << "x" << "y" << "width" << "height";
    keys.sort();
    expectedResult.sort();
    int i;
    QCOMPARE(keys, expectedResult);
    for (i = 0; i < keys.size(); ++i) {
        QVERIFY(settings1.contains(keys.at(i)));
    }

    settings1.endGroup();
    QVERIFY(settings1.group() == "alpha");
    keys = settings1.allKeys();
    QCOMPARE(keys.size(), expectedResult.size() + 3);
    for (i = 0; i < keys.size(); ++i) {
        QVERIFY(settings1.contains(keys.at(i)));
    }

    settings1.endGroup();
    QVERIFY(settings1.group().isEmpty());
    keys = settings1.allKeys();

    QCOMPARE(keys.size(), initialNumKeys + 7);
    for (i = 0; i < keys.size(); ++i) {
        QVERIFY(settings1.contains(keys.at(i)));
    }
}

void tst_QSettings::sync()
{
    /*
        What we're trying to test here is the case where two
        instances of the same application access the same preference
        files. We want to make sure that the results are 'merged',
        rather than having the last application overwrite settings
        set by the first application (like in Qt 3).

        This is only applicable to the INI format. The Windows
        registry and Mac's CFPreferences API should take care of this
        by themselves.
    */

    QSettings settings1(QSettings::IniFormat, QSettings::UserScope, "software.org");
    settings1.setValue("alpha/beta/geometry", -7);
    settings1.setValue("alpha/beta/geometry/x", 1);
    settings1.setValue("alpha/beta/geometry/y", 2);
    settings1.setValue("alpha/beta/geometry/width", 3);
    settings1.setValue("alpha/beta/geometry/height", 4);
    settings1.setValue("alpha/gamma/splitter", 5);
    settings1.sync(); // and it all goes into the file

    QSettings settings2(QSettings::IniFormat, QSettings::UserScope, "other.software.org");
    settings2.setValue("alpha/beta/geometry/x", 8);
    settings2.sync();

    settings2.setValue("moo/beta/geometry", -7);
    settings2.setValue("moo/beta/geometry/x", 1);
    settings2.setValue("moo/beta/geometry/y", 2);
    settings2.setValue("moo/beta/geometry/width", 3);
    settings2.setValue("moo/beta/geometry/height", 4);
    settings2.setValue("moo/gamma/splitter", 5);
    settings2.setValue("alpha/gamma/splitter", 15);
    settings2.remove("alpha/beta/geometry/x");
    settings2.remove("alpha/beta/geometry/y"); // should do nothing

    // Now "some other app" will change other.software.org.ini
    QString userConfDir = settingsPath("__user__") + QDir::separator();
#if !defined(Q_OS_WINCE) && !defined(Q_OS_WINRT)
    unlink((userConfDir + "other.software.org.ini").toLatin1());
    rename((userConfDir + "software.org.ini").toLatin1(),
           (userConfDir + "other.software.org.ini").toLatin1());
#else
    QFile::remove(userConfDir + "other.software.org.ini");
    QFile::rename(userConfDir + "software.org.ini" , userConfDir + "other.software.org.ini");
#endif

    settings2.sync();

    // And voila, we should be merged

    QCOMPARE(settings2.value("alpha/beta/geometry").toInt(), -7);
    QVERIFY(!settings2.contains("alpha/beta/geometry/x")); // <----- removed by settings2
    QCOMPARE(settings2.value("alpha/beta/geometry/y").toInt(), 2);
    QCOMPARE(settings2.value("alpha/beta/geometry/width").toInt(), 3);
    QCOMPARE(settings2.value("alpha/beta/geometry/height").toInt(), 4);
    QCOMPARE(settings2.value("alpha/gamma/splitter").toInt(), 15); // <---- set by settings2
    QCOMPARE(settings2.value("moo/beta/geometry").toInt(), -7);
    QCOMPARE(settings2.value("moo/beta/geometry/x").toInt(), 1);
    QCOMPARE(settings2.value("moo/beta/geometry/y").toInt(), 2);
    QCOMPARE(settings2.value("moo/beta/geometry/width").toInt(), 3);
    QCOMPARE(settings2.value("moo/beta/geometry/height").toInt(), 4);
    QCOMPARE(settings2.value("moo/gamma/splitter").toInt(), 5);
    QCOMPARE(settings2.allKeys().count(), 11);

    // Now, software.org.ini no longer exists, this is same as another app
    // clearing all settings.
    settings1.sync();
    QCOMPARE(settings1.allKeys().count(), 0);

    // Now "some other app" will change software.org.ini
    QVERIFY(QFile::rename((userConfDir + "other.software.org.ini").toLatin1(),
                          (userConfDir + "software.org.ini").toLatin1()));

    settings1.sync();
    QCOMPARE(settings1.value("alpha/beta/geometry").toInt(), -7);
    QCOMPARE(settings1.value("alpha/beta/geometry/y").toInt(), 2);
    QCOMPARE(settings1.value("alpha/beta/geometry/width").toInt(), 3);
    QCOMPARE(settings1.value("alpha/beta/geometry/height").toInt(), 4);
    QCOMPARE(settings1.value("alpha/gamma/splitter").toInt(), 15);
    QCOMPARE(settings1.value("moo/beta/geometry").toInt(), -7);
    QCOMPARE(settings1.value("moo/beta/geometry/x").toInt(), 1);
    QCOMPARE(settings1.value("moo/beta/geometry/y").toInt(), 2);
    QCOMPARE(settings1.value("moo/beta/geometry/width").toInt(), 3);
    QCOMPARE(settings1.value("moo/beta/geometry/height").toInt(), 4);
    QCOMPARE(settings1.value("moo/gamma/splitter").toInt(), 5);
    QCOMPARE(settings1.allKeys().count(), 11);
}

void tst_QSettings::setFallbacksEnabled_data()
{
    populateWithFormats();
}

void tst_QSettings::setFallbacksEnabled()
{
    QFETCH(QSettings::Format, format);

    if (!m_canWriteNativeSystemSettings && format == QSettings::NativeFormat)
        QSKIP(insufficientPermissionSkipMessage);

    QSettings settings1(format, QSettings::UserScope, "software.org", "KillerAPP");
    QSettings settings2(format, QSettings::UserScope, "software.org");
    QSettings settings3(format, QSettings::SystemScope, "software.org", "KillerAPP");
    QSettings settings4(format, QSettings::SystemScope, "software.org");

    settings1.setValue("key 1", "alpha");
    settings2.setValue("key 1", "beta");
    settings3.setValue("key 1", "gamma");
    settings4.setValue("key 1", "delta");

    settings1.setValue("key 2", "alpha");
    settings2.setValue("key 2", "beta");
    settings3.setValue("key 2", "gamma");

    settings1.setValue("key 3", "alpha");
    settings3.setValue("key 3", "gamma");
    settings4.setValue("key 3", "delta");

    settings1.setValue("key 4", "alpha");
    settings2.setValue("key 4", "beta");
    settings4.setValue("key 4", "delta");

    settings2.setValue("key 5", "beta");
    settings3.setValue("key 5", "gamma");
    settings4.setValue("key 5", "delta");

    QVERIFY(settings1.fallbacksEnabled());
    QVERIFY(settings2.fallbacksEnabled());
    QVERIFY(settings3.fallbacksEnabled());
    QVERIFY(settings4.fallbacksEnabled());

    settings1.setFallbacksEnabled(false);
    settings2.setFallbacksEnabled(false);
    settings3.setFallbacksEnabled(false);
    settings4.setFallbacksEnabled(false);

    QVERIFY(!settings1.fallbacksEnabled());
    QVERIFY(!settings2.fallbacksEnabled());
    QVERIFY(!settings3.fallbacksEnabled());
    QVERIFY(!settings4.fallbacksEnabled());

    /*
        Make sure that the QSettings objects can still access their
        main associated file when fallbacks are turned off.
    */

#if !defined(Q_OS_BLACKBERRY)
    QCOMPARE(settings1.value("key 1").toString(), QString("alpha"));
    QCOMPARE(settings2.value("key 1").toString(), QString("beta"));
    QCOMPARE(settings3.value("key 1").toString(), QString("gamma"));
    QCOMPARE(settings4.value("key 1").toString(), QString("delta"));

    QCOMPARE(settings1.value("key 2").toString(), QString("alpha"));
    QCOMPARE(settings2.value("key 2").toString(), QString("beta"));
    QCOMPARE(settings3.value("key 2").toString(), QString("gamma"));
    QVERIFY(!settings4.contains("key 2"));

    QCOMPARE(settings1.value("key 3").toString(), QString("alpha"));
    QCOMPARE(settings3.value("key 3").toString(), QString("gamma"));
    QCOMPARE(settings4.value("key 3").toString(), QString("delta"));
    QVERIFY(!settings2.contains("key 3"));

    QCOMPARE(settings1.value("key 4").toString(), QString("alpha"));
    QCOMPARE(settings2.value("key 4").toString(), QString("beta"));
    QCOMPARE(settings4.value("key 4").toString(), QString("delta"));
    QVERIFY(!settings3.contains("key 4"));

    QCOMPARE(settings2.value("key 5").toString(), QString("beta"));
    QCOMPARE(settings3.value("key 5").toString(), QString("gamma"));
    QCOMPARE(settings4.value("key 5").toString(), QString("delta"));
    QVERIFY(!settings1.contains("key 5"));

    QCOMPARE(settings1.value("key 1").toString(), QString("alpha"));
    QCOMPARE(settings1.value("key 5").toString(), QString(""));
    QVERIFY(settings1.contains("key 1"));
    QVERIFY(!settings1.contains("key 5"));
#else
    QCOMPARE(settings1.value("key 1").toString(), QString("gamma"));
    QCOMPARE(settings2.value("key 1").toString(), QString("delta"));
    QCOMPARE(settings3.value("key 1").toString(), QString("gamma"));
    QCOMPARE(settings4.value("key 1").toString(), QString("delta"));

    QCOMPARE(settings1.value("key 2").toString(), QString("gamma"));
    QCOMPARE(settings2.value("key 2").toString(), QString("beta"));
    QCOMPARE(settings3.value("key 2").toString(), QString("gamma"));
    QCOMPARE(settings4.value("key 2").toString(), QString("beta"));

    QCOMPARE(settings1.value("key 3").toString(), QString("gamma"));
    QCOMPARE(settings2.value("key 3").toString(), QString("delta"));
    QCOMPARE(settings3.value("key 3").toString(), QString("gamma"));
    QCOMPARE(settings4.value("key 3").toString(), QString("delta"));
#endif
}

void tst_QSettings::testChildKeysAndGroups_data()
{
    populateWithFormats();
}

void tst_QSettings::testChildKeysAndGroups()
{
    QFETCH(QSettings::Format, format);

    QSettings settings1(format, QSettings::UserScope, "software.org");
    settings1.setFallbacksEnabled(false);
    settings1.setValue("alpha/beta/geometry", -7);
    settings1.setValue("alpha/beta/geometry/x", 1);
    settings1.setValue("alpha/beta/geometry/y", 2);
    settings1.setValue("alpha/beta/geometry/width", 3);
    settings1.setValue("alpha/beta/geometry/height", 4);
    settings1.setValue("alpha/gamma/splitter", 5);

    QCOMPARE(settings1.childKeys(), QStringList());
    QCOMPARE(settings1.childGroups(), QStringList() << "alpha");

    settings1.beginGroup("/alpha");
    QCOMPARE(settings1.childKeys(), QStringList());
    QStringList children = settings1.childGroups();
    children.sort();
    QCOMPARE(children, QStringList() << "beta" << "gamma");

    settings1.beginGroup("/beta");
    QCOMPARE(settings1.childKeys(), QStringList() << "geometry");
    QCOMPARE(settings1.childGroups(), QStringList() << "geometry");

    settings1.beginGroup("/geometry");
    children = settings1.childKeys();
    children.sort();
    QCOMPARE(children, QStringList()  << "height" << "width" << "x" << "y");
    QCOMPARE(settings1.childGroups(), QStringList());

    settings1.beginGroup("/width");
    QCOMPARE(settings1.childKeys(), QStringList());
    QCOMPARE(settings1.childGroups(), QStringList());

    settings1.endGroup();
    settings1.endGroup();
    settings1.endGroup();
    settings1.endGroup();

    {
        QSettings settings2("other.software.org");
        settings2.setValue("viewbar/foo/test1", "1");
        settings2.setValue("viewbar/foo/test2", "2");
        settings2.setValue("viewbar/foo/test3", "3");
        settings2.setValue("viewbar/foo/test4", "4");
        settings2.setValue("viewbar/foo/test5", "5");
        settings2.setValue("viewbar/bar/test1", "1");
        settings2.setValue("viewbar/bar/test2", "2");
        settings2.setValue("viewbar/bar/test3", "3");
        settings2.setValue("viewbar/bar/test4", "4");
        settings2.setValue("viewbar/bar/test5", "5");

        settings2.beginGroup("viewbar");
        QStringList l = settings2.childGroups();
        settings2.endGroup();
        l.sort();
        QCOMPARE(l, QStringList() << "bar" << "foo");
    }
}

void tst_QSettings::testUpdateRequestEvent()
{
#ifdef Q_OS_WINRT
    const QString oldCur = QDir::currentPath();
    QDir::setCurrent(QStandardPaths::writableLocation(QStandardPaths::AppLocalDataLocation));
#endif

    QFile::remove("foo");
    QVERIFY(!QFile::exists("foo"));

    QSettings settings1("foo", QSettings::IniFormat);
    QVERIFY(!QFile::exists("foo"));
    QVERIFY(QFileInfo("foo").size() == 0);
    settings1.setValue("key1", 1);
    QVERIFY(QFileInfo("foo").size() == 0);

    QTRY_VERIFY(QFileInfo("foo").size() > 0);

    settings1.remove("key1");
    QVERIFY(QFileInfo("foo").size() > 0);

    QTRY_VERIFY(QFileInfo("foo").size() == 0);

    settings1.setValue("key2", 2);
    QVERIFY(QFileInfo("foo").size() == 0);

    QTRY_VERIFY(QFileInfo("foo").size() > 0);

    settings1.clear();
    QVERIFY(QFileInfo("foo").size() > 0);

    QTRY_VERIFY(QFileInfo("foo").size() == 0);

#ifdef Q_OS_WINRT
    QDir::setCurrent(oldCur);
#endif
}

const int NumIterations = 5;
const int NumThreads = 4;
int numThreadSafetyFailures;

class SettingsThread : public QThread
{
public:
    void run();
    void start(int n) { param = n; QThread::start(); }

private:
    int param;
};

void SettingsThread::run()
{
    for (int i = 0; i < NumIterations; ++i) {
        QSettings settings("software.org", "KillerAPP");
        settings.setValue(QString::number((param * NumIterations) + i), param);
        settings.sync();
        if (settings.status() != QSettings::NoError) {
            QWARN(qPrintable(QString("Unexpected QSettings status %1").arg((int)settings.status())));
            ++numThreadSafetyFailures;
        }
    }
}

void tst_QSettings::testThreadSafety()
{
    SettingsThread threads[NumThreads];
    int i, j;

    numThreadSafetyFailures = 0;

    for (i = 0; i < NumThreads; ++i)
        threads[i].start(i + 1);
    for (i = 0; i < NumThreads; ++i)
        threads[i].wait();

    QSettings settings("software.org", "KillerAPP");
    for (i = 0; i < NumThreads; ++i) {
        int param = i + 1;
        for (j = 0; j < NumIterations; ++j) {
            QCOMPARE(settings.value(QString::number((param * NumIterations) + j)).toInt(), param);
        }
    }

    QCOMPARE(numThreadSafetyFailures, 0);
}

#ifdef QT_BUILD_INTERNAL
void tst_QSettings::testNormalizedKey_data()
{
    QTest::addColumn<QString>("inKey");
    QTest::addColumn<QString>("outKey");

    QTest::newRow("empty1") << "" << "";
    QTest::newRow("empty2") << "/" << "";
    QTest::newRow("empty3") << "//" << "";
    QTest::newRow("empty4") << "///" << "";

    QTest::newRow("a1") << "a" << "a";
    QTest::newRow("a2") << "/a" << "a";
    QTest::newRow("a3") << "a/" << "a";
    QTest::newRow("a4") << "//a" << "a";
    QTest::newRow("a5") << "a//" << "a";
    QTest::newRow("a6") << "///a" << "a";
    QTest::newRow("a7") << "a///" << "a";
    QTest::newRow("a8") << "///a/" << "a";
    QTest::newRow("a9") << "/a///" << "a";

    QTest::newRow("ab1") << "aaa/bbb" << "aaa/bbb";
    QTest::newRow("ab2") << "/aaa/bbb" << "aaa/bbb";
    QTest::newRow("ab3") << "aaa/bbb/" << "aaa/bbb";
    QTest::newRow("ab4") << "/aaa/bbb/" << "aaa/bbb";
    QTest::newRow("ab5") << "aaa///bbb" << "aaa/bbb";
    QTest::newRow("ab6") << "aaa///bbb/" << "aaa/bbb";
    QTest::newRow("ab7") << "/aaa///bbb/" << "aaa/bbb";
    QTest::newRow("ab8") << "////aaa///bbb////" << "aaa/bbb";
}
#endif

#ifdef QT_BUILD_INTERNAL
void tst_QSettings::testNormalizedKey()
{
    QFETCH(QString, inKey);
    QFETCH(QString, outKey);

    inKey.detach();

    QString result = QSettingsPrivate::normalizedKey(inKey);
    QCOMPARE(result, outKey);

    /*
        If the key is already normalized, we verify that outKey is
        just a shallow copy of the input string. This is an important
        optimization that shouldn't be removed accidentally.
    */
    if (inKey == outKey) {
        QVERIFY(!result.isDetached());
    } else {
        if (!result.isEmpty()) {
            QVERIFY(result.isDetached());
        }
    }
}
#endif

void tst_QSettings::testEmptyData()
{
    QString filename(settingsPath("empty.ini"));
    QFile::remove(filename);
    QVERIFY(!QFile::exists(filename));

    QString nullString;
    QString emptyString("");
    QStringList emptyList;
    QStringList list;
    QStringList list2;

    QVariantList emptyVList;
    QVariantList vList, vList2, vList3;

    list << emptyString << nullString;
    list2 << emptyString;
    vList << emptyString;
    vList2 << emptyString << nullString;
    vList3 << QString("foo");

    {
        QSettings settings(filename, QSettings::IniFormat);
        settings.setValue("nullString", nullString);
        settings.setValue("emptyString", emptyString);
        settings.setValue("emptyList", emptyList);
        settings.setValue("list", list);
        settings.setValue("list2", list2);
        settings.setValue("emptyVList", emptyVList);
        settings.setValue("vList", vList);
        settings.setValue("vList2", vList2);
        settings.setValue("vList3", vList3);
        QVERIFY(settings.status() == QSettings::NoError);
    }
    {
        QSettings settings(filename, QSettings::IniFormat);
        QCOMPARE(settings.value("nullString").toString(), nullString);
        QCOMPARE(settings.value("emptyString").toString(), emptyString);
        QCOMPARE(settings.value("emptyList").toStringList(), emptyList);
        QCOMPARE(settings.value("list").toStringList(), list);
        QCOMPARE(settings.value("list2").toStringList(), list2);
        QCOMPARE(settings.value("emptyVList").toList(), emptyVList);
        QCOMPARE(settings.value("vList").toList(), vList);
        QCOMPARE(settings.value("vList2").toList(), vList2);
        QCOMPARE(settings.value("vList3").toList(), vList3);
        QVERIFY(settings.status() == QSettings::NoError);
    }

    {
        QSettings settings("QtProject", "tst_qsettings");
        settings.setValue("nullString", nullString);
        settings.setValue("emptyString", emptyString);
        settings.setValue("emptyList", emptyList);
        settings.setValue("list", list);
        settings.setValue("list2", list2);
        settings.setValue("emptyVList", emptyVList);
        settings.setValue("vList", vList);
        settings.setValue("vList2", vList2);
        settings.setValue("vList3", vList3);
        QVERIFY(settings.status() == QSettings::NoError);
    }
    {
        QSettings settings("QtProject", "tst_qsettings");
        QCOMPARE(settings.value("nullString").toString(), nullString);
        QCOMPARE(settings.value("emptyString").toString(), emptyString);
        QCOMPARE(settings.value("emptyList").toStringList(), emptyList);
        QCOMPARE(settings.value("list").toStringList(), list);
        QCOMPARE(settings.value("list2").toStringList(), list2);
        QCOMPARE(settings.value("emptyVList").toList(), emptyVList);
        QCOMPARE(settings.value("vList").toList(), vList);
        QCOMPARE(settings.value("vList2").toList(), vList2);
        QCOMPARE(settings.value("vList3").toList(), vList3);
        QVERIFY(settings.status() == QSettings::NoError);
    }
    QFile::remove(filename);
}

void tst_QSettings::testEmptyKey()
{
    QSettings settings;
    QTest::ignoreMessage(QtWarningMsg, "QSettings::value: Empty key passed");
    const QVariant value = settings.value(QString());
    QCOMPARE(value, QVariant());
    QTest::ignoreMessage(QtWarningMsg, "QSettings::setValue: Empty key passed");
    settings.setValue(QString(), value);
}

void tst_QSettings::testResourceFiles()
{
    QSettings settings(":/resourcefile.ini", QSettings::IniFormat);
    QVERIFY(settings.status() == QSettings::NoError);
    QVERIFY(!settings.isWritable());
    QCOMPARE(settings.value("Field 1/Bottom").toInt(), 89);
    settings.setValue("Field 1/Bottom", 90);

    // the next two lines check the statu quo; another behavior would be possible
    QVERIFY(settings.status() == QSettings::NoError);
    QCOMPARE(settings.value("Field 1/Bottom").toInt(), 90);

    settings.sync();
    QVERIFY(settings.status() == QSettings::AccessError);
    QCOMPARE(settings.value("Field 1/Bottom").toInt(), 90);
}

void tst_QSettings::testRegistryShortRootNames()
{
#ifndef Q_OS_WIN
    QSKIP("This test is specific to the Windows registry only.");
#else
    QVERIFY(QSettings("HKEY_CURRENT_USER", QSettings::NativeFormat).childGroups() == QSettings("HKCU", QSettings::NativeFormat).childGroups());
    QVERIFY(QSettings("HKEY_LOCAL_MACHINE", QSettings::NativeFormat).childGroups() == QSettings("HKLM", QSettings::NativeFormat).childGroups());
    QVERIFY(QSettings("HKEY_CLASSES_ROOT", QSettings::NativeFormat).childGroups() == QSettings("HKCR", QSettings::NativeFormat).childGroups());
    QVERIFY(QSettings("HKEY_USERS", QSettings::NativeFormat).childGroups() == QSettings("HKU", QSettings::NativeFormat).childGroups());
#endif
}

void tst_QSettings::trailingWhitespace()
{
    {
        QSettings s("tst_QSettings_trailingWhitespace");
        s.setValue("trailingSpace", "x  ");
        s.setValue("trailingTab", "x\t");
        s.setValue("trailingNewline", "x\n");
    }
    {
        QSettings s("tst_QSettings_trailingWhitespace");
        QCOMPARE(s.value("trailingSpace").toString(), QLatin1String("x  "));
        QCOMPARE(s.value("trailingTab").toString(), QLatin1String("x\t"));
        QCOMPARE(s.value("trailingNewline").toString(), QLatin1String("x\n"));
        s.clear();
    }
}

void tst_QSettings::fromFile_data()
{
    populateWithFormats();
}

void tst_QSettings::fromFile()
{
    QFETCH(QSettings::Format, format);

    // Sandboxed WinRT applications cannot write into the
    // application directory. Hence reset the current
    // directory
#ifdef Q_OS_WINRT
    const QString oldCur = QDir::currentPath();
    QDir::setCurrent(QStandardPaths::writableLocation(QStandardPaths::AppLocalDataLocation));
#endif

    QFile::remove("foo");
    QVERIFY(!QFile::exists("foo"));

    QString path = "foo";

#if defined(Q_OS_WIN) && !defined(Q_OS_WINRT)
    if (format == QSettings::NativeFormat)
        path = "\\HKEY_CURRENT_USER\\Software\\foo";
#endif

    QStringList strList = QStringList() << "hope" << "destiny" << "chastity";

    {
        QSettings settings1(path, format);
        QVERIFY(settings1.allKeys().isEmpty());

        settings1.setValue("alpha", 1);
        settings1.setValue("alpha", 2);
        settings1.setValue("beta", strList);

        QSettings settings2(path, format);
        QCOMPARE(settings2.value("alpha").toInt(), 2);

        settings1.sync();
#if !defined(Q_OS_WIN)
        QVERIFY(QFile::exists("foo"));
#endif
        QCOMPARE(settings1.value("alpha").toInt(), 2);
        QCOMPARE(settings2.value("alpha").toInt(), 2);

        settings2.setValue("alpha", 3);
        settings2.setValue("gamma/foo.bar", 4);
        QCOMPARE(settings1.value("alpha").toInt(), 3);
        QCOMPARE(settings2.value("alpha").toInt(), 3);
        QCOMPARE(settings1.value("beta").toStringList(), strList);
        QCOMPARE(settings2.value("beta").toStringList(), strList);
        QCOMPARE(settings1.value("gamma/foo.bar").toInt(), 4);
        QCOMPARE(settings2.value("gamma/foo.bar").toInt(), 4);
    }

    {
        QSettings settings1(path, format);
        QCOMPARE(settings1.value("alpha").toInt(), 3);
        QCOMPARE(settings1.value("beta").toStringList(), strList);
        QCOMPARE(settings1.value("gamma/foo.bar").toInt(), 4);
        QCOMPARE(settings1.allKeys().size(), 3);
    }
#ifdef Q_OS_WINRT
    QDir::setCurrent(oldCur);
#endif
}

#ifdef QT_BUILD_INTERNAL
void tst_QSettings::setIniCodec()
{
    QByteArray expeContents4, expeContents5;
    QByteArray actualContents4, actualContents5;

    {
        QFile inFile(":/resourcefile4.ini");
        inFile.open(QIODevice::ReadOnly);
        expeContents4 = inFile.readAll();
        inFile.close();
    }

    {
        QFile inFile(":/resourcefile5.ini");
        inFile.open(QIODevice::ReadOnly);
        expeContents5 = inFile.readAll();
        inFile.close();
    }

    {
        QSettings settings4(QSettings::IniFormat, QSettings::UserScope, "software.org", "KillerAPP");
        settings4.setIniCodec("UTF-8");
        settings4.setValue(QLatin1String("Fa\xe7" "ade/QU\xc9" "BEC"), QLatin1String("Fa\xe7" "ade/QU\xc9" "BEC"));
        settings4.sync();

        QSettings settings5(QSettings::IniFormat, QSettings::UserScope, "other.software.org", "KillerAPP");
        settings5.setIniCodec("ISO 8859-1");
        settings5.setValue(QLatin1String("Fa\xe7" "ade/QU\xc9" "BEC"), QLatin1String("Fa\xe7" "ade/QU\xc9" "BEC"));
        settings5.sync();

        {
            QFile inFile(settings4.fileName());
            inFile.open(QIODevice::ReadOnly);
            actualContents4 = inFile.readAll();
            inFile.close();
        }

        {
            QFile inFile(settings5.fileName());
            inFile.open(QIODevice::ReadOnly);
            actualContents5 = inFile.readAll();
            inFile.close();
        }
    }

    QConfFile::clearCache();

#ifdef Q_OS_WIN
    QEXPECT_FAIL("", "QTBUG-25446", Abort);
#endif
    QCOMPARE(actualContents4, expeContents4);
    QCOMPARE(actualContents5, expeContents5);

    QSettings settings4(QSettings::IniFormat, QSettings::UserScope, "software.org", "KillerAPP");
    settings4.setIniCodec("UTF-8");
    QSettings settings5(QSettings::IniFormat, QSettings::UserScope, "other.software.org", "KillerAPP");
    settings5.setIniCodec("Latin-1");

    QCOMPARE(settings4.allKeys().count(), 1);
    QCOMPARE(settings5.allKeys().count(), 1);

    QCOMPARE(settings4.allKeys().first(), settings5.allKeys().first());
    QCOMPARE(settings4.value(settings4.allKeys().first()).toString(),
             settings5.value(settings5.allKeys().first()).toString());
}
#endif

static bool containsSubList(QStringList mom, QStringList son)
{
    for (int i = 0; i < son.size(); ++i) {
        if (!mom.contains(son.at(i)))
            return false;
    }
    return true;
}

void tst_QSettings::testArrays_data()
{
    populateWithFormats();
}

/*
    Tests beginReadArray(), beginWriteArray(), endArray(), and
    setArrayIndex().
*/
void tst_QSettings::testArrays()
{
    QFETCH(QSettings::Format, format);

    {
        QSettings settings1(format, QSettings::UserScope, "software.org", "KillerAPP");

        settings1.beginWriteArray("foo/bar", 3);
        settings1.setValue("bip", 1);
        settings1.setArrayIndex(0);
        settings1.setValue("ene", 2);
        settings1.setValue("due", 3);
        settings1.setValue("rike", 4);
        settings1.setArrayIndex(1);
        settings1.setValue("ene", 5);
        settings1.setValue("due", 6);
        settings1.setValue("rike", 7);
        settings1.setArrayIndex(2);
        settings1.setValue("ene", 8);
        settings1.setValue("due", 9);
        settings1.setValue("rike", 10);
        settings1.endArray();

        QStringList expectedList;
        expectedList
            << "foo/bar/bip"
            << "foo/bar/size"
            << "foo/bar/1/ene"
            << "foo/bar/1/due"
            << "foo/bar/1/rike"
            << "foo/bar/2/ene"
            << "foo/bar/2/due"
            << "foo/bar/2/rike"
            << "foo/bar/3/ene"
            << "foo/bar/3/due"
            << "foo/bar/3/rike";
        expectedList.sort();

        QStringList actualList = settings1.allKeys();
        actualList.sort();
        QVERIFY(containsSubList(actualList, expectedList));

        QCOMPARE(settings1.value("/foo/bar/bip").toInt(), 1);
        QCOMPARE(settings1.value("/foo/bar/1/ene").toInt(), 2);
        QCOMPARE(settings1.value("/foo/bar/1/due").toInt(), 3);
        QCOMPARE(settings1.value("/foo/bar/1/rike").toInt(), 4);
        QCOMPARE(settings1.value("/foo/bar/2/ene").toInt(), 5);
        QCOMPARE(settings1.value("/foo/bar/2/due").toInt(), 6);
        QCOMPARE(settings1.value("/foo/bar/2/rike").toInt(), 7);
        QCOMPARE(settings1.value("/foo/bar/3/ene").toInt(), 8);
        QCOMPARE(settings1.value("/foo/bar/3/due").toInt(), 9);
        QCOMPARE(settings1.value("/foo/bar/3/rike").toInt(), 10);

        settings1.beginGroup("/foo");
        int count = settings1.beginReadArray("bar");
        QCOMPARE(count, 3);
        QCOMPARE(settings1.value("bip").toInt(), 1);
        settings1.setArrayIndex(0);
        QCOMPARE(settings1.value("ene").toInt(), 2);
        QCOMPARE(settings1.value("due").toInt(), 3);
        QCOMPARE(settings1.value("rike").toInt(), 4);
        QCOMPARE(settings1.allKeys().count(), 3);
        settings1.setArrayIndex(1);
        QCOMPARE(settings1.value("ene").toInt(), 5);
        QCOMPARE(settings1.value("due").toInt(), 6);
        QCOMPARE(settings1.value("rike").toInt(), 7);
        QCOMPARE(settings1.allKeys().count(), 3);
        settings1.setArrayIndex(2);
        QCOMPARE(settings1.value("ene").toInt(), 8);
        QCOMPARE(settings1.value("due").toInt(), 9);
        QCOMPARE(settings1.value("rike").toInt(), 10);
        QCOMPARE(settings1.allKeys().count(), 3);

        settings1.endArray();
        settings1.endGroup();
    }
    /*
        Check that we get the arrays right when we load them again
    */

    {
        QSettings settings1(format, QSettings::UserScope, "software.org", "KillerAPP");

        QStringList expectedList;
        expectedList
            << "foo/bar/bip"
            << "foo/bar/size"
            << "foo/bar/1/ene"
            << "foo/bar/1/due"
            << "foo/bar/1/rike"
            << "foo/bar/2/ene"
            << "foo/bar/2/due"
            << "foo/bar/2/rike"
            << "foo/bar/3/ene"
            << "foo/bar/3/due"
            << "foo/bar/3/rike";
        expectedList.sort();

        QStringList actualList = settings1.allKeys();
        actualList.sort();
        QVERIFY(containsSubList(actualList, expectedList));

        QCOMPARE(settings1.value("/foo/bar/bip").toInt(), 1);
        QCOMPARE(settings1.value("/foo/bar/1/ene").toInt(), 2);
        QCOMPARE(settings1.value("/foo/bar/1/due").toInt(), 3);
        QCOMPARE(settings1.value("/foo/bar/1/rike").toInt(), 4);
        QCOMPARE(settings1.value("/foo/bar/2/ene").toInt(), 5);
        QCOMPARE(settings1.value("/foo/bar/2/due").toInt(), 6);
        QCOMPARE(settings1.value("/foo/bar/2/rike").toInt(), 7);
        QCOMPARE(settings1.value("/foo/bar/3/ene").toInt(), 8);
        QCOMPARE(settings1.value("/foo/bar/3/due").toInt(), 9);
        QCOMPARE(settings1.value("/foo/bar/3/rike").toInt(), 10);

        settings1.beginGroup("/foo");
        int count = settings1.beginReadArray("bar");
        QCOMPARE(count, 3);
        QCOMPARE(settings1.value("bip").toInt(), 1);
        settings1.setArrayIndex(0);
        QCOMPARE(settings1.value("ene").toInt(), 2);
        QCOMPARE(settings1.value("due").toInt(), 3);
        QCOMPARE(settings1.value("rike").toInt(), 4);
        QCOMPARE(settings1.allKeys().count(), 3);
        settings1.setArrayIndex(1);
        QCOMPARE(settings1.value("ene").toInt(), 5);
        QCOMPARE(settings1.value("due").toInt(), 6);
        QCOMPARE(settings1.value("rike").toInt(), 7);
        QCOMPARE(settings1.allKeys().count(), 3);
        settings1.setArrayIndex(2);
        QCOMPARE(settings1.value("ene").toInt(), 8);
        QCOMPARE(settings1.value("due").toInt(), 9);
        QCOMPARE(settings1.value("rike").toInt(), 10);
        QCOMPARE(settings1.allKeys().count(), 3);

        settings1.endArray();
        settings1.endGroup();
    }
    /*
        This code generates lots of warnings, but that's on purpose.
        Basically, we check that endGroup() can be used instead of
        endArray() and vice versa. This is not documented, but this
        is the behavior that we have chosen.
    */
    QTest::ignoreMessage(QtWarningMsg, "QSettings::setArrayIndex: Missing beginArray()");
    QTest::ignoreMessage(QtWarningMsg, "QSettings::setArrayIndex: Missing beginArray()");
    QTest::ignoreMessage(QtWarningMsg, "QSettings::setArrayIndex: Missing beginArray()");
    QTest::ignoreMessage(QtWarningMsg, "QSettings::setArrayIndex: Missing beginArray()");
    QTest::ignoreMessage(QtWarningMsg, "QSettings::setArrayIndex: Missing beginArray()");
    QTest::ignoreMessage(QtWarningMsg, "QSettings::endArray: Expected endGroup() instead");
    QTest::ignoreMessage(QtWarningMsg, "QSettings::endGroup: Expected endArray() instead");
    QTest::ignoreMessage(QtWarningMsg, "QSettings::endArray: Expected endGroup() instead");
    QTest::ignoreMessage(QtWarningMsg, "QSettings::endGroup: No matching beginGroup()");

    QSettings settings1(format, QSettings::UserScope, "software.org", "KillerAPP");
    settings1.clear();
    settings1.beginGroup("/alpha");
    QCOMPARE(settings1.group(), QString("alpha"));
    settings1.setArrayIndex(0);
    QCOMPARE(settings1.group(), QString("alpha"));
    settings1.setArrayIndex(1);
    QCOMPARE(settings1.group(), QString("alpha"));
    settings1.setArrayIndex(2);
    QCOMPARE(settings1.group(), QString("alpha"));
    settings1.beginGroup("/beta");
    QCOMPARE(settings1.group(), QString("alpha/beta"));
    settings1.beginGroup("");
    QCOMPARE(settings1.group(), QString("alpha/beta"));
    settings1.beginWriteArray("DO", 4);
    QCOMPARE(settings1.value("size").toInt(), 4);
    QCOMPARE(settings1.group(), QString("alpha/beta/DO"));
    settings1.setArrayIndex(0);
    QCOMPARE(settings1.group(), QString("alpha/beta/DO/1"));
    settings1.setArrayIndex(1);
    QCOMPARE(settings1.group(), QString("alpha/beta/DO/2"));
    settings1.beginGroup("1");
    QCOMPARE(settings1.group(), QString("alpha/beta/DO/2/1"));
    settings1.setArrayIndex(3);
    QCOMPARE(settings1.group(), QString("alpha/beta/DO/2/1"));
    settings1.setArrayIndex(4);
    QCOMPARE(settings1.group(), QString("alpha/beta/DO/2/1"));
    settings1.beginWriteArray("RE");
    QVERIFY(!settings1.contains("size"));
    QCOMPARE(settings1.group(), QString("alpha/beta/DO/2/1/RE"));
    settings1.setArrayIndex(0);
    QCOMPARE(settings1.group(), QString("alpha/beta/DO/2/1/RE/1"));
    settings1.setArrayIndex(1);
    QCOMPARE(settings1.group(), QString("alpha/beta/DO/2/1/RE/2"));
    settings1.endArray();
    QCOMPARE(settings1.group(), QString("alpha/beta/DO/2/1"));
    settings1.endArray();
    QCOMPARE(settings1.group(), QString("alpha/beta/DO/2"));
    settings1.setArrayIndex(2);
    QCOMPARE(settings1.group(), QString("alpha/beta/DO/3"));
    settings1.endGroup();
    QCOMPARE(settings1.group(), QString("alpha/beta"));
    settings1.endGroup();
    QCOMPARE(settings1.group(), QString("alpha/beta"));
    settings1.endGroup();
    QCOMPARE(settings1.group(), QString("alpha"));
    settings1.endArray();
    QCOMPARE(settings1.group(), QString());
    settings1.endGroup();
    QCOMPARE(settings1.group(), QString());
    /*
        Now, let's make sure that things work well if an array
        is spread across multiple files.
    */
    int i;

    settings1.clear();
    QSettings settings2(format, QSettings::UserScope, "software.org");

    QStringList threeStrings;
    threeStrings << "Uno" << "Dos" << "Tres";

    QStringList fiveStrings;
    fiveStrings << "alpha" << "beta" << "gamma" << "delta" << "epsilon";

    settings1.beginWriteArray("strings");
    for (i = threeStrings.size() - 1; i >= 0; --i) {
        settings1.setArrayIndex(i);
        settings1.setValue("fileName", threeStrings.at(i));
    }
    settings1.endArray();

    settings2.beginWriteArray("strings");
    for (i = fiveStrings.size() - 1; i >= 0; --i) {
        settings2.setArrayIndex(i);
        settings2.setValue("fileName", fiveStrings.at(i));
    }
    settings2.endArray();

    int size1 = settings1.beginReadArray("strings");
    QCOMPARE(size1, 3);
    QCOMPARE(settings1.value("size").toInt(), 3);

    for (i = 0; i < size1; ++i) {
        settings1.setArrayIndex(i);
        QString str = settings1.value("fileName").toString();
        QCOMPARE(str, threeStrings.at(i));
    }
    settings1.endArray();

    int size2 = settings2.beginReadArray("strings");
    QCOMPARE(size2, 5);
    QCOMPARE(settings2.value("size").toInt(), 5);

    for (i = 0; i < size2; ++i) {
        settings2.setArrayIndex(i);
        QString str = settings2.value("fileName").toString();
        QCOMPARE(str, fiveStrings.at(i));
    }
    settings2.endArray();

#if !defined (Q_OS_BLACKBERRY)
    size1 = settings1.beginReadArray("strings");
    QCOMPARE(size1, 3);

    // accessing entries beyond the end of settings1 goes to settings2
    for (i = size1; i < size2; ++i) {
        settings1.setArrayIndex(i);
        QString str = settings1.value("fileName").toString();
        QCOMPARE(str, fiveStrings.at(i));
    }
    settings1.endArray();
#endif
}

#ifdef QT_BUILD_INTERNAL
static QByteArray iniEscapedKey(const QString &str)
{
    QByteArray result;
    QSettingsPrivate::iniEscapedKey(str, result);
    return result;
}

static QString iniUnescapedKey(const QByteArray &ba)
{
    QString result;
    QSettingsPrivate::iniUnescapedKey(ba, 0, ba.size(), result);
    return result;
}

static QByteArray iniEscapedStringList(const QStringList &strList)
{
    QByteArray result;
    QSettingsPrivate::iniEscapedStringList(strList, result, 0);
    return result;
}

static QStringList iniUnescapedStringList(const QByteArray &ba)
{
    QStringList result;
    QString str;
#if QSETTINGS_P_H_VERSION >= 2
    bool isStringList = QSettingsPrivate::iniUnescapedStringList(ba, 0, ba.size(), str, result
#if QSETTINGS_P_H_VERSION >= 3
                                                                 , 0
#endif
                                                                    );
    if (!isStringList)
        result = QStringList(str);
#else
    QStringList *strList = QSettingsPrivate::iniUnescapedStringList(ba, 0, ba.size(), str);
    if (strList) {
        result = *strList;
        delete strList;
    } else {
        result = QStringList(str);
    }
#endif
    return result;
}
#endif

QString escapeWeirdChars(const QString &s)
{
    QString result;
    bool escapeNextDigit = false;

    for (int i = 0; i < s.length(); ++i) {
        QChar c = s.at(i);
        if (c.unicode() < ' ' || c.unicode() > '~'
            || (escapeNextDigit && c.unicode() >= '0' && c.unicode() <= 'f')) {
            result += QString("\\x%1").arg(c.unicode(), 0, 16);
            escapeNextDigit = true;
        } else {
            result += c;
            escapeNextDigit = false;
        }
    }

    return result;
}

#ifdef QT_BUILD_INTERNAL
void tst_QSettings::testEscapes()
{
    QSettings settings(QSettings::UserScope, "software.org", "KillerAPP");

#define testEscapedKey(plainKey, escKey) \
    QCOMPARE(iniEscapedKey(plainKey), QByteArray(escKey)); \
    QCOMPARE(iniUnescapedKey(escKey), QString(plainKey));

#define testUnescapedKey(escKey, plainKey, reescKey) \
    QCOMPARE(iniUnescapedKey(escKey), QString(plainKey)); \
    QCOMPARE(iniEscapedKey(plainKey), QByteArray(reescKey)); \
    QCOMPARE(iniUnescapedKey(reescKey), QString(plainKey));

#define testEscapedStringList(plainStrList, escStrList) \
    { \
        QStringList plainList(plainStrList); \
        QByteArray escList(escStrList); \
        QCOMPARE(iniEscapedStringList(plainList), escList); \
        QCOMPARE(iniUnescapedStringList(escList), plainList); \
    } \


#define testUnescapedStringList(escStrList, plainStrList, reescStrList) \
    { \
        QStringList plainList(plainStrList); \
        QByteArray escList(escStrList); \
        QByteArray reescList(reescStrList); \
        QCOMPARE(iniUnescapedStringList(escList), plainList); \
        QCOMPARE(iniEscapedStringList(plainList), reescList); \
        QCOMPARE(iniUnescapedStringList(reescList), plainList); \
    } \


#define testVariant(val, escStr, func) \
    { \
        QVariant v(val); \
        QString s = QSettingsPrivate::variantToString(v); \
        QCOMPARE(s, escStr); \
        QCOMPARE(QVariant(QSettingsPrivate::stringToVariant(escStr)), v); \
        QVERIFY((val) == v.func());                                     \
    }

#define testBadEscape(escStr, vStr) \
    { \
        QVariant v = QSettingsPrivate::stringToVariant(QString(escStr)); \
        QCOMPARE(v.toString(), QString(vStr)); \
    }

    testEscapedKey("", "");
    testEscapedKey(" ", "%20");
    testEscapedKey(" 0123 abcd ", "%200123%20abcd%20");
    testEscapedKey("~!@#$%^&*()_+.-/\\=", "%7E%21%40%23%24%25%5E%26%2A%28%29_%2B.-\\%5C%3D");
    testEscapedKey(QString() + QChar(0xabcd) + QChar(0x1234) + QChar(0x0081), "%UABCD%U1234%81");
    testEscapedKey(QString() + QChar(0xFE) + QChar(0xFF) + QChar(0x100) + QChar(0x101), "%FE%FF%U0100%U0101");

    testUnescapedKey("", "", "");
    testUnescapedKey("%20", " ", "%20");
    testUnescapedKey("/alpha/beta", "/alpha/beta", "\\alpha\\beta");
    testUnescapedKey("\\alpha\\beta", "/alpha/beta", "\\alpha\\beta");
    testUnescapedKey("%5Calpha%5Cbeta", "\\alpha\\beta", "%5Calpha%5Cbeta");
    testUnescapedKey("%", "%", "%25");
    testUnescapedKey("%f%!%%%%1x%x1%U%Uz%U123%U1234%1234%", QString("%f%!%%%%1x%x1%U%Uz%U123") + QChar(0x1234) + "\x12" + "34%",
                     "%25f%25%21%25%25%25%251x%25x1%25U%25Uz%25U123%U1234%1234%25");

    testEscapedStringList("", "");
    testEscapedStringList(" ", "\" \"");
    testEscapedStringList(";", "\";\"");
    testEscapedStringList(",", "\",\"");
    testEscapedStringList("=", "\"=\"");
    testEscapedStringList("abc-def", "abc-def");
    testEscapedStringList(QChar(0) + QString("0"), "\\0\\x30");
    testEscapedStringList("~!@#$%^&*()_+.-/\\=", "\"~!@#$%^&*()_+.-/\\\\=\"");
    testEscapedStringList("~!@#$%^&*()_+.-/\\", "~!@#$%^&*()_+.-/\\\\");
    testEscapedStringList(QString("\x7F") + "12aFz", "\\x7f\\x31\\x32\\x61\\x46z");
    testEscapedStringList(QString("   \t\n\\n") + QChar(0x123) + QChar(0x4567), "\"   \\t\\n\\\\n\\x123\\x4567\"");
    testEscapedStringList(QString("\a\b\f\n\r\t\v'\"?\001\002\x03\x04"), "\\a\\b\\f\\n\\r\\t\\v'\\\"?\\x1\\x2\\x3\\x4");
    testEscapedStringList(QStringList() << "," << ";" << "a" << "ab,  \tc, d ", "\",\", \";\", a, \"ab,  \\tc, d \"");

    /*
      Test .ini syntax that cannot be generated by QSettings (but can be entered by users).
    */
    testUnescapedStringList("", "", "");
    testUnescapedStringList("\"\"", "", "");
    testUnescapedStringList("\"abcdef\"", "abcdef", "abcdef");
    testUnescapedStringList("\"\\?\\'\\\"\"", "?'\"", "?'\\\"");
    testUnescapedStringList("\\0\\00\\000\\0000000\\1\\111\\11111\\x\\x0\\xABCDEFGH\\x0123456\\",
                            QString() + QChar(0) + QChar(0) + QChar(0) + QChar(0) + QChar(1)
                            + QChar(0111) + QChar(011111) + QChar(0) + QChar(0xCDEF) + "GH"
                            + QChar(0x3456),
                            "\\0\\0\\0\\0\\x1I\\x1249\\0\\xcdefGH\\x3456");
    testUnescapedStringList(QByteArray("\\c\\d\\e\\f\\g\\$\\*\\\0", 16), "\f", "\\f");
    testUnescapedStringList("\"a\",  \t\"bc \", \"  d\" , \"ef  \" ,,g,   hi  i,,, ,",
                            QStringList() << "a" << "bc " << "  d" << "ef  " << "" << "g" << "hi  i"
                                          << "" << "" << "" << "",
                            "a, \"bc \", \"  d\", \"ef  \", , g, hi  i, , , , ");
    testUnescapedStringList("a ,  b   ,   c   d   , efg   ",
                            QStringList() << "a" << "b" << "c   d" << "efg",
                            "a, b, c   d, efg");

    // streaming qvariant into a string
    testVariant(QString("Hello World!"), QString("Hello World!"), toString);
    testVariant(QString("Hello, World!"), QString("Hello, World!"), toString);
    testVariant(QString("@Hello World!"), QString("@@Hello World!"), toString);
    testVariant(QString("@@Hello World!"), QString("@@@Hello World!"), toString);
    testVariant(QByteArray("Hello World!"), QString("@ByteArray(Hello World!)"), toString);
    testVariant(QByteArray("@Hello World!"), QString("@ByteArray(@Hello World!)"), toString);
    testVariant(QVariant(100), QString("100"), toString);
    testVariant(QStringList() << "ene" << "due" << "rike", QString::fromLatin1("@Variant(\x0\x0\x0\xb\x0\x0\x0\x3\x0\x0\x0\x6\x0\x65\x0n\x0\x65\x0\x0\x0\x6\x0\x64\x0u\x0\x65\x0\x0\x0\x8\x0r\x0i\x0k\x0\x65)", 50), toStringList);
    testVariant(QRect(1, 2, 3, 4), QString("@Rect(1 2 3 4)"), toRect);
    testVariant(QSize(5, 6), QString("@Size(5 6)"), toSize);
    testVariant(QPoint(7, 8), QString("@Point(7 8)"), toPoint);

    testBadEscape("", "");
    testBadEscape("@", "@");
    testBadEscape("@@", "@");
    testBadEscape("@@@", "@@");
    testBadEscape(" ", " ");
    testBadEscape("@Rect", "@Rect");
    testBadEscape("@Rect(", "@Rect(");
    testBadEscape("@Rect()", "@Rect()");
    testBadEscape("@Rect)", "@Rect)");
    testBadEscape("@Rect(1 2 3)", "@Rect(1 2 3)");
    testBadEscape("@@Rect(1 2 3)", "@Rect(1 2 3)");
}
#endif

void tst_QSettings::testCaseSensitivity_data()
{
    populateWithFormats();
}

void tst_QSettings::testCaseSensitivity()
{
    QFETCH(QSettings::Format, format);

    for (int pass = 0; pass < 2; ++pass) {
        QSettings settings(format, QSettings::UserScope, "software.org", "KillerAPP");
        settings.beginGroup("caseSensitivity");

        bool cs = true;
#ifndef QT_QSETTINGS_ALWAYS_CASE_SENSITIVE_AND_FORGET_ORIGINAL_KEY_ORDER
        switch (format) {
        case QSettings::NativeFormat:
#ifdef Q_OS_DARWIN
            cs = true;
#else
            cs = false;
#endif
            break;
        case QSettings::IniFormat:
            cs = false;
            break;
        case QSettings::CustomFormat1:
            cs = true;
            break;
        case QSettings::CustomFormat2:
            cs = false;
            break;
        default:
            ;
        }
#endif

        if (pass == 0) {
            settings.setValue("key 1", 1);
            settings.setValue("KEY 1", 2);
            settings.setValue("key 2", 3);
        }

        for (int i = 0; i < 2; ++i) {
            QVERIFY(settings.contains("key 1"));
            QVERIFY(settings.contains("KEY 1"));
            QCOMPARE(settings.value("KEY 1").toInt(), 2);
/*            QVERIFY(settings.allKeys().contains("/KEY 1"));
            QVERIFY(settings.allKeys().contains("/key 2")); */

            if (cs) {
                QVERIFY(!settings.contains("kEy 1"));
                QCOMPARE(settings.value("key 1").toInt(), 1);
                QCOMPARE(settings.allKeys().size(), 3);
                QVERIFY(settings.allKeys().contains("key 1"));
            } else {
                QVERIFY(settings.contains("kEy 1"));
                QCOMPARE(settings.value("kEy 1").toInt(), 2);
                QCOMPARE(settings.value("key 1").toInt(), 2);
                QCOMPARE(settings.allKeys().size(), 2);
            }

            settings.sync();
        }

        settings.remove("KeY 1");

        if (cs) {
            QVERIFY(!settings.contains("KeY 1"));
            QVERIFY(settings.contains("key 1"));
            QVERIFY(settings.contains("KEY 1"));
            QCOMPARE(settings.value("key 1").toInt(), 1);
            QCOMPARE(settings.value("KEY 1").toInt(), 2);
            QCOMPARE(settings.allKeys().size(), 3);
        } else {
            QVERIFY(!settings.contains("KeY 1"));
            QVERIFY(!settings.contains("key 1"));
            QVERIFY(!settings.contains("KEY 1"));
            QCOMPARE(settings.allKeys().size(), 1);
        }
        settings.setValue("KEY 1", 2);
    }
}

#ifdef Q_OS_MAC
// Please write a fileName() test for the other platforms
void tst_QSettings::fileName()
{
    QSettings s1(QSettings::UserScope, "Apple", "Console");
    QSettings s2(QSettings::UserScope, "Apple");
    QSettings s3(QSettings::SystemScope, "Apple", "Console");
    QSettings s4(QSettings::SystemScope, "Apple");

    QCOMPARE(s1.fileName(), QDir::homePath() + "/Library/Preferences/com.apple.Console.plist");
    QCOMPARE(s2.fileName(), QDir::homePath() + "/Library/Preferences/com.apple.plist");
    QCOMPARE(s3.fileName(), QString("/Library/Preferences/com.apple.Console.plist"));
    QCOMPARE(s4.fileName(), QString("/Library/Preferences/com.apple.plist"));

    QSettings s5(QSettings::SystemScope, "Apple.com", "Console");
    QCOMPARE(s5.fileName(), QString("/Library/Preferences/com.apple.Console.plist"));

    QSettings s6(QSettings::SystemScope, "apple.com", "Console");
    QCOMPARE(s6.fileName(), QString("/Library/Preferences/com.apple.Console.plist"));

    QSettings s7(QSettings::SystemScope, "apple.Com", "Console");
    QCOMPARE(s7.fileName(), QString("/Library/Preferences/com.apple.Console.plist"));

    QSettings s8(QSettings::SystemScope, "apple.fr", "Console");
    QCOMPARE(s8.fileName(), QString("/Library/Preferences/fr.apple.Console.plist"));

    QSettings s9(QSettings::SystemScope, "apple.co.jp", "Console");
    QCOMPARE(s9.fileName(), QString("/Library/Preferences/jp.co.apple.Console.plist"));

    QSettings s10(QSettings::SystemScope, "apple.org", "Console");
    QCOMPARE(s10.fileName(), QString("/Library/Preferences/org.apple.Console.plist"));

    QSettings s11(QSettings::SystemScope, "apple.net", "Console");
    QCOMPARE(s11.fileName(), QString("/Library/Preferences/net.apple.Console.plist"));

    QSettings s12(QSettings::SystemScope, "apple.museum", "Console");
    QCOMPARE(s12.fileName(), QString("/Library/Preferences/museum.apple.Console.plist"));

    QSettings s13(QSettings::SystemScope, "apple.FR", "Console");
    QCOMPARE(s13.fileName(), QString("/Library/Preferences/fr.apple.Console.plist"));

    QSettings s14(QSettings::SystemScope, "apple.mUseum", "Console");
    QCOMPARE(s14.fileName(), QString("/Library/Preferences/museum.apple.Console.plist"));

    QSettings s15(QSettings::SystemScope, "apple.zz", "Console");
    QCOMPARE(s15.fileName(), QString("/Library/Preferences/zz.apple.Console.plist"));

    QSettings s15_prime(QSettings::SystemScope, "apple.foo", "Console");
    QCOMPARE(s15_prime.fileName(), QString("/Library/Preferences/com.apple-foo.Console.plist"));

    QSettings s16(QSettings::SystemScope, "apple.f", "Console");
    QCOMPARE(s16.fileName(), QString("/Library/Preferences/com.apple-f.Console.plist"));

    QSettings s17(QSettings::SystemScope, "apple.", "Console");
    QCOMPARE(s17.fileName(), QString("/Library/Preferences/com.apple.Console.plist"));

    QSettings s18(QSettings::SystemScope, "Foo, Inc.", "Console");
    QCOMPARE(s18.fileName(), QString("/Library/Preferences/com.foo-inc.Console.plist"));

    QSettings s19(QSettings::SystemScope, "Foo, Inc.com", "Console");
    QCOMPARE(s19.fileName(), QString("/Library/Preferences/com.foo, inc.Console.plist"));

    QSettings s20(QSettings::SystemScope, QLatin1String("   ") + QChar(0xbd) + QLatin1String("Foo//:/Barxxx  Baz!()#@.com"), "Console");
    QCOMPARE(s20.fileName(), QLatin1String("/Library/Preferences/com.   ") + QChar(0xbd) + QLatin1String("foo  : barxxx  baz!()#@.Console.plist"));

    QSettings s21(QSettings::SystemScope, QLatin1String("   ") + QChar(0xbd) + QLatin1String("Foo//:/Bar,,,  Baz!()#"), "Console");
    QCOMPARE(s21.fileName(), QString("/Library/Preferences/com.foo-bar-baz.Console.plist"));
}
#endif

void tst_QSettings::isWritable_data()
{
    populateWithFormats();
}

void tst_QSettings::isWritable()
{
    QFETCH(QSettings::Format, format);

    {
        QSettings s1(format, QSettings::UserScope, "software.org", "KillerAPP");
        s1.setValue("foo", 1);
        s1.sync();
        // that should create the file
    }

    {
        QSettings s1(format, QSettings::UserScope, "software.org", "KillerAPP");
        QVERIFY(s1.isWritable());
    }

    {
        QSettings s1(format, QSettings::SystemScope, "software.org", "KillerAPP");
        s1.setValue("foo", 1);
        s1.sync();
        // that should create the file, *if* we have the permissions
    }

    {
        QSettings s1(format, QSettings::SystemScope, "software.org", "KillerAPP");
        QSettings s2(format, QSettings::SystemScope, "software.org", "Something Different");
        QSettings s3(format, QSettings::SystemScope, "foo.org", "Something Different");

        if (s1.contains("foo")) {
#if defined(Q_OS_MACX)
            if (QSysInfo::macVersion() >= QSysInfo::MV_10_9) {
                QVERIFY(s1.isWritable());
                if (format == QSettings::NativeFormat) {
                    QVERIFY(!s2.isWritable());
                    QVERIFY(!s3.isWritable());
                } else {
                    QVERIFY(s2.isWritable());
                    QVERIFY(s3.isWritable());
                }
            } else if (QSysInfo::macVersion() >= QSysInfo::MV_10_7 &&
                       format == QSettings::NativeFormat) {
                QVERIFY(!s1.isWritable());
                QVERIFY(!s2.isWritable());
                QVERIFY(!s3.isWritable());
            } else
#endif
            {
                QVERIFY(s1.isWritable());
                QVERIFY(s2.isWritable());
                QVERIFY(s3.isWritable());
            }
        } else {
            QVERIFY(!s1.isWritable());
            QVERIFY(!s2.isWritable());
            QVERIFY(!s3.isWritable());
        }
    }
}

#ifdef QT_BUILD_INTERNAL
void tst_QSettings::childGroups_data()
{
    populateWithFormats();
}
#endif

#ifdef QT_BUILD_INTERNAL
void tst_QSettings::childGroups()
{
    QFETCH(QSettings::Format, format);

    const QSettings::Scope scope = m_canWriteNativeSystemSettings ? QSettings::SystemScope : QSettings::UserScope;

    {
        QSettings settings(format, scope, "software.org");
        settings.setValue("alpha", "1");
        settings.setValue("alpha/a", "2");
        settings.setValue("alpha/b", "3");
        settings.setValue("alpha/c", "4");
        settings.setValue("beta", "5");
        settings.setValue("gamma", "6");
        settings.setValue("gamma/d", "7");
        settings.setValue("gamma/d/e", "8");
        settings.setValue("gamma/f/g", "9");
        settings.setValue("omicron/h/i/j/x", "10");
        settings.setValue("omicron/h/i/k/y", "11");
        settings.setValue("zeta/z", "12");
    }

    for (int pass = 0; pass < 3; ++pass) {
        QConfFile::clearCache();
        QSettings settings(format, scope, "software.org");
        settings.setFallbacksEnabled(false);
        if (pass == 1) {
            settings.value("gamma/d");
        } else if (pass == 2) {
            settings.value("gamma");
        }

        settings.beginGroup("gamma");
        QStringList childGroups = settings.childGroups();
        childGroups.sort();
        QCOMPARE(childGroups, QStringList() << "d" << "f");
        settings.beginGroup("d");
        QCOMPARE(settings.childGroups(), QStringList());
        settings.endGroup();
        settings.endGroup();

        settings.beginGroup("alpha");
        QCOMPARE(settings.childGroups(), QStringList());
        settings.endGroup();

        settings.beginGroup("d");
        QCOMPARE(settings.childGroups(), QStringList());
        settings.endGroup();

        settings.beginGroup("/omicron///h/i///");
        childGroups = settings.childGroups();
        childGroups.sort();
        QCOMPARE(childGroups, QStringList() << "j" << "k");
        settings.endGroup();

        settings.beginGroup("////");
        childGroups = settings.childGroups();
        childGroups.sort();
        QCOMPARE(childGroups, QStringList() << "alpha" << "gamma" << "omicron" << "zeta");
        settings.endGroup();

        childGroups = settings.childGroups();
        childGroups.sort();
        QCOMPARE(childGroups, QStringList() << "alpha" << "gamma" << "omicron" << "zeta");
    }
}
#endif

#ifdef QT_BUILD_INTERNAL
void tst_QSettings::childKeys_data()
{
    populateWithFormats();
}
#endif

#ifdef QT_BUILD_INTERNAL
void tst_QSettings::childKeys()
{
    QFETCH(QSettings::Format, format);

    const QSettings::Scope scope = m_canWriteNativeSystemSettings ? QSettings::SystemScope : QSettings::UserScope;

    {
        QSettings settings(format, scope, "software.org");
        settings.setValue("alpha", "1");
        settings.setValue("alpha/a", "2");
        settings.setValue("alpha/b", "3");
        settings.setValue("alpha/c", "4");
        settings.setValue("beta", "5");
        settings.setValue("gamma", "6");
        settings.setValue("gamma/d", "7");
        settings.setValue("gamma/d/e", "8");
        settings.setValue("gamma/f/g", "9");
        settings.setValue("omicron/h/i/j/x", "10");
        settings.setValue("omicron/h/i/k/y", "11");
        settings.setValue("zeta/z", "12");
    }

    for (int pass = 0; pass < 3; ++pass) {
        QConfFile::clearCache();
        QSettings settings(format, scope, "software.org");
        settings.setFallbacksEnabled(false);
        if (pass == 1) {
            settings.value("gamma/d");
        } else if (pass == 2) {
            settings.value("gamma");
        }

        settings.beginGroup("gamma");
        QCOMPARE(settings.childKeys(), QStringList() << "d");
        settings.beginGroup("d");
        QCOMPARE(settings.childKeys(), QStringList() << "e");
        settings.endGroup();
        settings.endGroup();

        settings.beginGroup("alpha");
        QStringList childKeys = settings.childKeys();
        childKeys.sort();
        QCOMPARE(childKeys, QStringList() << "a" << "b" << "c");
        settings.endGroup();

        settings.beginGroup("d");
        QCOMPARE(settings.childKeys(), QStringList());
        settings.endGroup();

        settings.beginGroup("/omicron///h/i///");
        QCOMPARE(settings.childKeys(), QStringList());
        settings.endGroup();

        settings.beginGroup("////");
        childKeys = settings.childKeys();
        childKeys.sort();
        QCOMPARE(childKeys, QStringList() << "alpha" << "beta" << "gamma");
        settings.endGroup();

        childKeys = settings.childKeys();
        childKeys.sort();
        QCOMPARE(childKeys, QStringList() << "alpha" << "beta" << "gamma");
    }
}
#endif

#ifdef QT_BUILD_INTERNAL
void tst_QSettings::allKeys_data()
{
    populateWithFormats();
}
#endif

#ifdef QT_BUILD_INTERNAL
void tst_QSettings::allKeys()
{
    QFETCH(QSettings::Format, format);

    QStringList allKeys;
    allKeys << "alpha" << "alpha/a" << "alpha/b" << "alpha/c" << "beta" << "gamma" << "gamma/d"
            << "gamma/d/e" << "gamma/f/g" << "omicron/h/i/j/x" << "omicron/h/i/k/y" << "zeta/z";

    const QSettings::Scope scope = m_canWriteNativeSystemSettings ? QSettings::SystemScope : QSettings::UserScope;

    {
        QSettings settings(format, scope, "software.org");
        for (int i = 0; i < allKeys.size(); ++i)
            settings.setValue(allKeys.at(i), QString::number(i + 1));
    }

    for (int pass = 0; pass < 3; ++pass) {
        QConfFile::clearCache();
        QSettings settings(format, scope, "software.org");
        settings.setFallbacksEnabled(false);

        if (pass == 1) {
            settings.value("gamma/d");
        } else if (pass == 2) {
            settings.value("gamma");
        }

        settings.beginGroup("gamma");
        QStringList keys = settings.allKeys();
        keys.sort();
        QCOMPARE(keys, QStringList() << "d" << "d/e" << "f/g");
        settings.beginGroup("d");
        QCOMPARE(settings.allKeys(), QStringList() << "e");
        settings.endGroup();
        settings.endGroup();

        settings.beginGroup("alpha");
        keys = settings.allKeys();
        keys.sort();
        QCOMPARE(keys, QStringList() << "a" << "b" << "c");
        settings.endGroup();

        settings.beginGroup("d");
        QCOMPARE(settings.allKeys(), QStringList());
        settings.endGroup();

        settings.beginGroup("/omicron///h/i///");
        keys = settings.allKeys();
        keys.sort();
        QCOMPARE(keys, QStringList() << "j/x" << "k/y");
        settings.endGroup();

        settings.beginGroup("////");
        keys = settings.allKeys();
        keys.sort();
        QCOMPARE(keys, allKeys);
        settings.endGroup();

        keys = settings.allKeys();
        keys.sort();
        QCOMPARE(keys, allKeys);
    }
}
#endif

void tst_QSettings::registerFormat()
{
    QSettings settings1(QSettings::IniFormat, QSettings::UserScope, "software.org", "KillerAPP");
    QSettings settings2(QSettings::CustomFormat1, QSettings::UserScope, "software.org", "KillerAPP");

    QString fileName = settings1.fileName();
    fileName.chop(3); // "ini";
    fileName.append("custom1");
    QCOMPARE(settings2.fileName(), fileName);

    // OK, let's see if it can read a generated file of a custom type
    // Beware: readCustom3File() and writeCustom3File() have unintuitive behavior
    // so we can test error handling

    QSettings::Format custom3 = QSettings::registerFormat("custom3", readCustom3File, writeCustom3File);
    QVERIFY(custom3 == QSettings::CustomFormat3);

    QDir dir(settingsPath());
    QVERIFY(dir.mkpath("someDir"));
    QFile f(dir.path()+"/someDir/someSettings.custom3");

    QVERIFY(f.open(QFile::WriteOnly));
    f.write("OK");
    f.close();

    {
        QSettings settings(settingsPath("someDir/someSettings.custom3"), QSettings::CustomFormat3);
        QCOMPARE(settings.status(), QSettings::NoError);
        QCOMPARE(settings.value("retval").toString(), QString("OK"));
        QVERIFY(settings.isWritable());
    }

    QVERIFY(f.open(QFile::WriteOnly));
    f.write("NotOK");
    f.close();

    {
        QSettings settings(settingsPath("someDir/someSettings.custom3"), QSettings::CustomFormat3);
        QCOMPARE(settings.status(), QSettings::FormatError);
        QCOMPARE(settings.value("retval").toString(), QString());
        QVERIFY(settings.isWritable());
    }

    QVERIFY(f.open(QFile::WriteOnly));
    f.write("OK");
    f.close();

    {
        QSettings settings(settingsPath("someDir/someSettings.custom3"), QSettings::CustomFormat3);
        QCOMPARE(settings.status(), QSettings::NoError);
        settings.setValue("zzz", "bar");
        settings.sync();
        QCOMPARE(settings.status(), QSettings::NoError);

        settings.setValue("retval", "NotOK");
        settings.sync();
        QCOMPARE(settings.status(), QSettings::AccessError);

        QCOMPARE(settings.value("retval").toString(), QString("NotOK"));
        QVERIFY(settings.isWritable());
    }

    {
        QSettings settings(settingsPath("someDir/someSettings.custom3"), QSettings::CustomFormat4);
        QCOMPARE(settings.status(), QSettings::AccessError);
        QVERIFY(!settings.isWritable());
    }
}

void tst_QSettings::setPath()
{
#define TEST_PATH(doSet, ext, format, scope, path) \
    { \
    if (doSet) \
        QSettings::setPath(QSettings::format, QSettings::scope, settingsPath(path)); \
    QSettings settings1(QSettings::format, QSettings::scope, "software.org", "KillerAPP"); \
    QCOMPARE(QDir(settings1.fileName()), QDir(settingsPath(path) + QDir::separator() + "software.org" \
                                  + QDir::separator() + "KillerAPP." + ext)); \
    }

    /*
        The first pass checks that setPath() works; the second
        path checks that it has no bad side effects.
    */
    for (int i = 0; i < 2; ++i) {
#if !defined(Q_OS_BLACKBERRY)
#if !defined(Q_OS_WIN) && !defined(Q_OS_MAC)
        TEST_PATH(i == 0, "conf", NativeFormat, UserScope, "alpha")
        TEST_PATH(i == 0, "conf", NativeFormat, SystemScope, "beta")
#endif
        TEST_PATH(i == 0, "ini", IniFormat, UserScope, "gamma")
        TEST_PATH(i == 0, "ini", IniFormat, SystemScope, "omicron")
        TEST_PATH(i == 0, "custom1", CustomFormat1, UserScope, "epsilon")
        TEST_PATH(i == 0, "custom1", CustomFormat1, SystemScope, "zeta")
        TEST_PATH(i == 0, "custom2", CustomFormat2, UserScope, "eta")
        TEST_PATH(i == 0, "custom2", CustomFormat2, SystemScope, "iota")
#else // Q_OS_BLACKBERRY: no system scope
        TEST_PATH(i == 0, "conf", NativeFormat, UserScope, "alpha")
        TEST_PATH(i == 0, "ini", IniFormat, UserScope, "gamma")
        TEST_PATH(i == 0, "custom1", CustomFormat1, UserScope, "epsilon")
        TEST_PATH(i == 0, "custom2", CustomFormat2, UserScope, "eta")
#endif
    }
}

void tst_QSettings::setDefaultFormat()
{
    QVERIFY(QSettings::defaultFormat() == QSettings::NativeFormat);

    QSettings::setDefaultFormat(QSettings::CustomFormat1);
    QSettings settings1("org", "app");
    QSettings settings2(QSettings::SystemScope, "org", "app");
    QSettings settings3;

    QVERIFY(settings1.format() == QSettings::NativeFormat);
    QVERIFY(settings2.format() == QSettings::NativeFormat);
    QVERIFY(settings3.format() == QSettings::CustomFormat1);

    QSettings::setDefaultFormat(QSettings::NativeFormat);
    QVERIFY(QSettings::defaultFormat() == QSettings::NativeFormat);

    QVERIFY(settings1.format() == QSettings::NativeFormat);
    QVERIFY(settings2.format() == QSettings::NativeFormat);
    QVERIFY(settings3.format() == QSettings::CustomFormat1);
}

void tst_QSettings::dontCreateNeedlessPaths()
{
    QString path;
    {
        QSettings settings(QSettings::IniFormat, QSettings::UserScope, "Hello", "Test");
        QVariant val = settings.value("foo", "bar");
        path = settings.fileName();
    }

    QFileInfo fileInfo(path);
    QVERIFY(!fileInfo.dir().exists());
}

#if !defined(Q_OS_WIN) && !defined(QT_QSETTINGS_ALWAYS_CASE_SENSITIVE_AND_FORGET_ORIGINAL_KEY_ORDER)
// This Qt build does not preserve ordering, as a code size optimization.
void tst_QSettings::dontReorderIniKeysNeedlessly()
{

    /*
        This is a very strong test. It asserts that modifying
        resourcefile2.ini will lead to the exact contents of
        resourcefile3.ini. Right now it's run only on Unix
        systems, but that should be enough since the INI
        code (unlike this test) is platform-agnostic.

        Things that are tested:

            * keys are written in the same order that they were
              read in

            * new keys are put at the end of their respective
              sections
    */

    QFile inFile(":/resourcefile2.ini");
    inFile.open(QIODevice::ReadOnly);
    QByteArray contentsBefore = inFile.readAll();
    inFile.close();

    QByteArray expectedContentsAfter;

    {
        QFile inFile(":/resourcefile3.ini");
        inFile.open(QIODevice::ReadOnly);
        expectedContentsAfter = inFile.readAll();
        inFile.close();
    }

    QString outFileName;
    QString outFileName2;

    QTemporaryFile outFile;
    QVERIFY2(outFile.open(), qPrintable(outFile.errorString()));
    outFile.write(contentsBefore);
    outFileName = outFile.fileName();
    outFile.close();

    QSettings settings(outFileName, QSettings::IniFormat);
    QVERIFY(settings.status() == QSettings::NoError);
    QVERIFY(settings.isWritable());

    settings.setValue("Field 1/Bottom", 90);
    settings.setValue("Field 1/x", 1);
    settings.setValue("Field 1/y", 1);
    settings.setValue("Field 1/width", 1);
    settings.setValue("Field 1/height", 1);
    settings.sync();

    QFile outFile2(outFileName);
    QVERIFY(outFile2.open(QIODevice::ReadOnly));
    QCOMPARE(outFile2.readAll(), expectedContentsAfter);
    outFile2.close();
}
#endif

void tst_QSettings::rainersSyncBugOnMac_data()
{
    ctor_data();
}

void tst_QSettings::rainersSyncBugOnMac()
{
    QFETCH(QSettings::Format, format);

#if defined(Q_OS_OSX) || defined(Q_OS_WINRT)
    if (format == QSettings::NativeFormat)
        QSKIP("OSX does not support direct reads from and writes to .plist files, due to caching and background syncing. See QTBUG-34899.");
#endif

    QString fileName;

    {
        QSettings s1(format, QSettings::UserScope, "software.org", "KillerAPP");
        QCOMPARE(s1.value("key1", 5).toInt(), 5);
        fileName = s1.fileName();
    }

    {
        QSettings s2(fileName, format);
        s2.setValue("key1", 25);
    }

    {
        QSettings s3(format, QSettings::UserScope, "software.org", "KillerAPP");
        QCOMPARE(s3.value("key1", 30).toInt(), 25);
    }
}

void tst_QSettings::recursionBug()
{
    QPixmap pix(10,10);
    pix.fill("blue");

    {
        QSettings settings(settingsPath("starrunner.ini"), QSettings::IniFormat);
        settings.setValue("General/Pixmap", pix );
    }
}

#if defined(Q_OS_WIN) && !defined(Q_OS_WINRT)

static DWORD readKeyType(HKEY handle, const QString &rSubKey)
{
    DWORD dataType;
    DWORD dataSize;
    LONG res = RegQueryValueEx(handle, reinterpret_cast<const wchar_t *>(rSubKey.utf16()), 0, &dataType, 0, &dataSize);

    if (res == ERROR_SUCCESS)
        return dataType;

    return 0;
}

// This is a regression test for QTBUG-13249, where QSettings was storing
// signed integers as numeric values and unsigned integers as strings.
void tst_QSettings::consistentRegistryStorage()
{
    QSettings settings1(QSettings::UserScope, "software.org", "KillerAPP");

    qint32 x = 1024;
    settings1.setValue("qint32_value", (qint32)x);
    QCOMPARE(settings1.value("qint32_value").toInt(), (qint32)1024);
    settings1.setValue("quint32_value", (quint32)x);
    QCOMPARE(settings1.value("quint32_value").toUInt(), (quint32)1024);
    settings1.setValue("qint64_value", (qint64)x);
    QCOMPARE(settings1.value("qint64_value").toLongLong(), (qint64)1024);
    settings1.setValue("quint64_value", (quint64)x);
    QCOMPARE(settings1.value("quint64_value").toULongLong(), (quint64)1024);
    settings1.sync();

    HKEY handle;
    LONG res;
    QString keyName = "Software\\software.org\\KillerAPP";
    res = RegOpenKeyEx(HKEY_CURRENT_USER, reinterpret_cast<const wchar_t *>(keyName.utf16()), 0, KEY_READ, &handle);
    if (res == ERROR_SUCCESS)
    {
        DWORD dataType;
        dataType = readKeyType(handle, QString("qint32_value"));
        if (dataType != 0) {
            QCOMPARE((int)REG_DWORD, (int)dataType);
        }
        dataType = readKeyType(handle, QString("quint32_value"));
        if (dataType != 0) {
            QCOMPARE((int)REG_DWORD, (int)dataType);
        }
        dataType = readKeyType(handle, QString("qint64_value"));
        if (dataType != 0) {
            QCOMPARE((int)REG_QWORD, (int)dataType);
        }
        dataType = readKeyType(handle, QString("quint64_value"));
        if (dataType != 0) {
            QCOMPARE((int)REG_QWORD, (int)dataType);
        }
        RegCloseKey(handle);
    }
}
#endif

QTEST_MAIN(tst_QSettings)
#include "tst_qsettings.moc"
