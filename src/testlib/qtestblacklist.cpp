/****************************************************************************
**
** Copyright (C) 2016 The Qt Company Ltd.
** Contact: https://www.qt.io/licensing/
**
** This file is part of the QtTest module of the Qt Toolkit.
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
#include "qtestblacklist_p.h"
#include "qtestresult_p.h"

#include <QtTest/qtestcase.h>
#include <QtCore/qbytearray.h>
#include <QtCore/qfile.h>
#include <QtCore/qset.h>
#include <QtCore/qcoreapplication.h>
#include <QtCore/qvariant.h>
#include <QtCore/QSysInfo>

#include <set>

QT_BEGIN_NAMESPACE

/*
 The file format is simply a grouped listing of keywords
 Ungrouped entries at the beginning apply to the whole testcase
 Groups define testfunctions or specific test data to ignore.
 After the groups come a list of entries (one per line) that define
 for which platform/os combination to ignore the test result.
 All keys in a single line have to match to blacklist the test.

 mac
 [testFunction]
 linux
 windows 64bit
 [testfunction2:testData]
 msvc

 The known keys are listed below:
*/

static QSet<QByteArray> keywords()
{
    // this list can be extended with new keywords as required
   QSet<QByteArray> set = QSet<QByteArray>()
             << "*"
#ifdef Q_OS_LINUX
            << "linux"
#endif
#ifdef Q_OS_OSX
            << "osx"
#endif
#ifdef Q_OS_WIN
            << "windows"
#endif
#ifdef Q_OS_IOS
            << "ios"
#endif
#ifdef Q_OS_ANDROID
            << "android"
#endif
#ifdef Q_OS_QNX
            << "qnx"
#endif
#ifdef Q_OS_WINRT
            << "winrt"
#endif
#ifdef Q_OS_WINCE
            << "wince"
#endif

#if QT_POINTER_SIZE == 8
            << "64bit"
#else
            << "32bit"
#endif

#ifdef Q_CC_GNU
            << "gcc"
#endif
#ifdef Q_CC_CLANG
            << "clang"
#endif
#ifdef Q_CC_MSVC
            << "msvc"
    #ifdef _MSC_VER
        #if _MSC_VER == 1900
            << "msvc-2015"
        #elif _MSC_VER == 1800
            << "msvc-2013"
        #elif _MSC_VER == 1700
            << "msvc-2012"
        #elif _MSC_VER == 1600
            << "msvc-2010"
        #endif
    #endif
#endif

#ifdef Q_PROCESSOR_X86
            << "x86"
#endif
#ifdef Q_PROCESSOR_ARM
            << "arm"
#endif

#ifdef Q_AUTOTEST_EXPORT
            << "developer-build"
#endif
            ;

            QCoreApplication *app = QCoreApplication::instance();
            if (app) {
                const QVariant platformName = app->property("platformName");
                if (platformName.isValid())
                    set << platformName.toByteArray();
            }

            return set;
}

static bool checkCondition(const QByteArray &condition)
{
    static QSet<QByteArray> matchedConditions = keywords();
    QList<QByteArray> conds = condition.split(' ');

    QByteArray distributionName = QSysInfo::productType().toLower().toUtf8();
    QByteArray distributionRelease = QSysInfo::productVersion().toLower().toUtf8();
    if (!distributionName.isEmpty()) {
        if (matchedConditions.find(distributionName) == matchedConditions.end())
            matchedConditions.insert(distributionName);
        matchedConditions.insert(distributionName + "-" + distributionRelease);
    }

    for (int i = 0; i < conds.size(); ++i) {
        QByteArray c = conds.at(i);
        bool result = c.startsWith('!');
        if (result)
            c = c.mid(1);

        result ^= matchedConditions.contains(c);
        if (!result)
            return false;
    }
    return true;
}

static bool ignoreAll = false;
static std::set<QByteArray> *ignoredTests = 0;
static std::set<QByteArray> *gpuFeatures = 0;

Q_TESTLIB_EXPORT std::set<QByteArray> *(*qgpu_features_ptr)(const QString &) = 0;

static bool isGPUTestBlacklisted(const char *slot, const char *data = 0)
{
    const QByteArray disableKey = QByteArrayLiteral("disable_") + QByteArray(slot);
    if (gpuFeatures->find(disableKey) != gpuFeatures->end()) {
        QByteArray msg = QByteArrayLiteral("Skipped due to GPU blacklist: ") + disableKey;
        if (data)
            msg += ':' + QByteArray(data);
        QTest::qSkip(msg.constData(), __FILE__, __LINE__);
        return true;
    }
    return false;
}

namespace QTestPrivate {

void parseBlackList()
{
    QString filename = QTest::qFindTestData(QStringLiteral("BLACKLIST"));
    if (filename.isEmpty())
        return;
    QFile ignored(filename);
    if (!ignored.open(QIODevice::ReadOnly))
        return;

    QByteArray function;

    while (!ignored.atEnd()) {
        QByteArray line = ignored.readLine().simplified();
        if (line.isEmpty() || line.startsWith('#'))
            continue;
        if (line.startsWith('[')) {
            function = line.mid(1, line.length() - 2);
            continue;
        }
        bool condition = checkCondition(line);
        if (condition) {
            if (!function.size()) {
                ignoreAll = true;
            } else {
                if (!ignoredTests)
                    ignoredTests = new std::set<QByteArray>;
                ignoredTests->insert(function);
            }
        }
    }
}

void parseGpuBlackList()
{
    if (!qgpu_features_ptr)
        return;
    QString filename = QTest::qFindTestData(QStringLiteral("GPU_BLACKLIST"));
    if (filename.isEmpty())
        return;
    if (!gpuFeatures)
        gpuFeatures = qgpu_features_ptr(filename);
}

void checkBlackLists(const char *slot, const char *data)
{
    bool ignore = ignoreAll;

    if (!ignore && ignoredTests) {
        QByteArray s = slot;
        ignore = (ignoredTests->find(s) != ignoredTests->end());
        if (!ignore && data) {
            s += ':';
            s += data;
            ignore = (ignoredTests->find(s) != ignoredTests->end());
        }
    }

    QTestResult::setBlacklistCurrentTest(ignore);

    // Tests blacklisted in GPU_BLACKLIST are to be skipped. Just ignoring the result is
    // not sufficient since these are expected to crash or behave in undefined ways.
    if (!ignore && gpuFeatures) {
        QByteArray s_gpu = slot;
        ignore = isGPUTestBlacklisted(s_gpu, data);
        if (!ignore && data) {
            s_gpu += ':';
            s_gpu += data;
            isGPUTestBlacklisted(s_gpu);
        }
    }
}

}


QT_END_NAMESPACE
