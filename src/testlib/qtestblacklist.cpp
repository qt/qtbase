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
  The BLACKLIST file format is a grouped listing of keywords.

  Blank lines and everything after # is simply ignored.  An initial #-line
  referring to this documentation is kind to readers.  Comments can also be used
  to indicate the reasons for ignoring particular cases.

  The key "ci" applies only when run by COIN.  Other keys name platforms,
  operating systems, distributions, tool-chains or architectures; a !  prefix
  reverses what it checks.  A version, joined to a key (at present, only for
  distributions and for msvc) with a hyphen, limits the key to the specific
  version.  A keyword line matches if every key on it applies to the present
  run.  Successive lines are alternate conditions for ignoring a test.

  Ungrouped lines at the beginning of a file apply to the whole testcase.
  A group starts with a [square-bracketed] identification of a test function,
  optionally with (after a colon, the name of) a specific data set, to ignore.
  Subsequent lines give conditions for ignoring this test.

        # See qtbase/src/testlib/qtestblacklist.cpp for format
        # Test doesn't work on QNX at all
        qnx

        # QTBUG-12345
        [testFunction]
        linux
        windows 64bit

        # Flaky in COIN on macOS, not reproducible by developers
        [testSlowly]
        ci osx

        # Needs basic C++11 support
        [testfunction2:testData]
        msvc-2010

  QML test functions are identified using the following format:

        <TestCase name>::<function name>:<data tag>

  For example, to blacklist a QML test on RHEL 7.6:

        # QTBUG-12345
        [Button::test_display:TextOnly]
        ci rhel-7.6

  Keys are lower-case.  Distribution name and version are supported if
  QSysInfo's productType() and productVersion() return them.

  Keys can be added via the space-separated QTEST_ENVIRONMENT
  environment variable:

        QTEST_ENVIRONMENT=ci ./tst_stuff

  This can be used to "mock" a test environment. In the example above,
  we add "ci" to the list of keys for the test environment, making it
  possible to test BLACKLIST files that blacklist tests in a CI environment.

  The other known keys are listed below:
*/

static QSet<QByteArray> keywords()
{
    // this list can be extended with new keywords as required
   QSet<QByteArray> set = QSet<QByteArray>()
             << "*"
#ifdef Q_OS_LINUX
            << "linux"
#endif
#ifdef Q_OS_MACOS
            << "osx"
            << "macos"
#endif
#if defined(Q_OS_WIN) && !defined(Q_OS_WINRT)
            << "windows"
#endif
#ifdef Q_OS_IOS
            << "ios"
#endif
#ifdef Q_OS_TVOS
            << "tvos"
#endif
#ifdef Q_OS_WATCHOS
            << "watchos"
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
#  if _MSC_VER <= 1600
            << "msvc-2010"
#  elif _MSC_VER <= 1700
            << "msvc-2012"
#  elif _MSC_VER <= 1800
            << "msvc-2013"
#  elif _MSC_VER <= 1900
            << "msvc-2015"
#  elif _MSC_VER <= 1916
            << "msvc-2017"
#  else
            << "msvc-2019"
#  endif
#endif

#ifdef Q_PROCESSOR_X86
            << "x86"
#endif
#ifdef Q_PROCESSOR_ARM
            << "arm"
#endif

#ifdef QT_BUILD_INTERNAL
            << "developer-build"
#endif
            ;

#if QT_CONFIG(properties)
            QCoreApplication *app = QCoreApplication::instance();
            if (app) {
                const QVariant platformName = app->property("platformName");
                if (platformName.isValid())
                    set << platformName.toByteArray();
            }
#endif

            return set;
}

static QSet<QByteArray> activeConditions()
{
    QSet<QByteArray> result = keywords();

    QByteArray distributionName = QSysInfo::productType().toLower().toUtf8();
    QByteArray distributionRelease = QSysInfo::productVersion().toLower().toUtf8();
    if (!distributionName.isEmpty()) {
        if (result.find(distributionName) == result.end())
            result.insert(distributionName);
        if (!distributionRelease.isEmpty()) {
            QByteArray versioned = distributionName + "-" + distributionRelease;
            if (result.find(versioned) == result.end())
                result.insert(versioned);
        }
    }

    if (qEnvironmentVariableIsSet("QTEST_ENVIRONMENT")) {
        for (const QByteArray &k : qgetenv("QTEST_ENVIRONMENT").split(' '))
            result.insert(k);
    }

    return result;
}

static bool checkCondition(const QByteArray &condition)
{
    static const QSet<QByteArray> matchedConditions = activeConditions();
    QList<QByteArray> conds = condition.split(' ');

    for (QByteArray c : conds) {
        bool result = c.startsWith('!');
        if (result)
            c.remove(0, 1);

        result ^= matchedConditions.contains(c);
        if (!result)
            return false;
    }
    return true;
}

static bool ignoreAll = false;
static std::set<QByteArray> *ignoredTests = nullptr;

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
        QByteArray line = ignored.readLine();
        const int commentPosition = line.indexOf('#');
        if (commentPosition >= 0)
            line.truncate(commentPosition);
        line = line.simplified();
        if (line.isEmpty())
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
}

} // QTestPrivate

QT_END_NAMESPACE
