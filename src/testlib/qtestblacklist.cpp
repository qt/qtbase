// Copyright (C) 2022 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only
#include "qtestblacklist_p.h"
#include "qtestresult_p.h"

#include <QtTest/qtestcase.h>
#include <QtCore/qbytearray.h>
#include <QtCore/qfile.h>
#include <QtCore/qset.h>
#include <QtCore/qcoreapplication.h>
#include <QtCore/qvariant.h>
#include <QtCore/QSysInfo>
#include <QtCore/QOperatingSystemVersion>

#include <set>

QT_BEGIN_NAMESPACE

using namespace Qt::StringLiterals;

/*
  The BLACKLIST file format is a grouped listing of keywords.

  Blank lines and everything after # is simply ignored.  An initial #-line
  referring to this documentation is kind to readers.  Comments can also be used
  to indicate the reasons for ignoring particular cases.

  Each blacklist line is interpreted as a list of keywords in an AND-relationship.
  To blacklist a test for multiple platforms (OR-relationship), use separate lines.

  The key "ci" applies only when run by COIN. The key "cmake" applies when Qt
  is built using CMake. Other keys name platforms, operating systems,
  distributions, tool-chains or architectures; a ! prefix reverses what it
  checks. A version, joined to a key (at present, only for distributions and
  for msvc) with a hyphen, limits the key to the specific version. A keyword
  line matches if every key on it applies to the present run. Successive lines
  are alternate conditions for ignoring a test.

  Ungrouped lines at the beginning of a file apply to the whole testcase. A
  group starts with a [square-bracketed] identification of a test function to
  ignore. For data-driven tests, this identification can be narrowed by the
  inclusion of global and local data row tags, separated from the function name
  and each other by colons. If both global and function-specific data rows tags
  are supplied, the global one comes first (as in the tag reported in test
  output, albeit in parentheses after the function name). Even when a test does
  have global and local data tags, you can omit either or both. (If a global
  data row's name coincides with that of a local data row, some unintended
  matches may result; try to keep your data-row tags distinct.)

  Subsequent lines give conditions for ignoring this test. You need at least
  one or the group has no effect.

        # See qtbase/src/testlib/qtestblacklist.cpp for format
        # Test doesn't work on QNX at all
        qnx

        # QTBUG-12345
        [testFunction]
        linux
        windows 64bit

        # Flaky in COIN on macOS, not reproducible by developers
        [testSlowly]
        macos ci

        # Needs basic C++11 support
        [testfunction2:testData]
        msvc-2010

        [getFile:withProxy SSL:localhost]
        android

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
#if defined(Q_OS_WIN)
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
#ifdef Q_OS_WEBOS
            << "webos"
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
#  elif _MSC_VER <= 1929
            << "msvc-2019"
#  else
            << "msvc-2022"
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

            << "cmake"
            ;

            QCoreApplication *app = QCoreApplication::instance();
            if (app) {
                const QVariant platformName = app->property("platformName");
                if (platformName.isValid())
                    set << platformName.toByteArray();
            }

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
        // backwards compatibility with Qt 5
        if (distributionName == "macos") {
            if (result.find(distributionName) == result.end())
                result.insert("osx");
            const auto version = QOperatingSystemVersion::current();
            if (version.majorVersion() >= 11)
                distributionRelease = QByteArray::number(version.majorVersion());
        }
        if (!distributionRelease.isEmpty()) {
            QByteArray versioned = distributionName + "-" + distributionRelease;
            if (result.find(versioned) == result.end())
                result.insert(versioned);
            if (distributionName == "macos") {
                QByteArray versioned = "osx-" + distributionRelease;
                if (result.find(versioned) == result.end())
                    result.insert(versioned);
            }
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
            function = line.mid(1, line.size() - 2);
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

void checkBlackLists(const char *slot, const char *data, const char *global)
{
    bool ignore = ignoreAll;

    if (!ignore && ignoredTests) {
        QByteArray s = slot;
        ignore = ignoredTests->find(s) != ignoredTests->end();
        if (!ignore && data) {
            s = (s + ':') + data;
            ignore = ignoredTests->find(s) != ignoredTests->end();
        }

        if (!ignore && global) {
            s = slot + ":"_ba + global;
            ignore = ignoredTests->find(s) != ignoredTests->end();
            if (!ignore && data) {
                s = (s + ':') + data;
                ignore = ignoredTests->find(s) != ignoredTests->end();
            }
        }
    }

    QTestResult::setBlacklistCurrentTest(ignore);
}

} // QTestPrivate

QT_END_NAMESPACE
