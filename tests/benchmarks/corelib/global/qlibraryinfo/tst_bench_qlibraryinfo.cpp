// Copyright (C) 2026 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only

#include <QCoreApplication>
#include <QLibraryInfo>
#include <QMetaEnum>
#include <QTest>

#include <QtCore/private/qlibraryinfo_p.h>

using namespace Qt::StringLiterals;

class tst_QLibraryInfo : public QObject
{
    Q_OBJECT
private Q_SLOTS:
    void initTestCase_data();
    void init();
    void cleanup();
    void paths_data();
    void paths();
    void platformPluginArguments_data();
    void platformPluginArguments();
};

void tst_QLibraryInfo::initTestCase_data()
{
    QTest::addColumn<bool>("withApp");
    QTest::newRow("without-qapp") << false;
    QTest::newRow("with-qapp") << true;
}

void tst_QLibraryInfo::init()
{
    // Only create or destroy the QCoreApplication when the live state doesn't
    // match the one this row wants. The global data loop runs all of a row's
    // local data (and benchmark retries) with the same state, so this is a
    // no-op except when actually switching between the two modes.
    QFETCH_GLOBAL(const bool, withApp);
    if (withApp && !QCoreApplication::instance()) {
        static int argc = 1;
        static char *argv[] = { const_cast<char *>(QTest::currentAppName()) };
        new QCoreApplication(argc, argv);
    } else if (!withApp && QCoreApplication::instance()) {
        delete QCoreApplication::instance();
    }
}

void tst_QLibraryInfo::cleanup()
{
    // Reset to the default configuration, undoing any manual qt.conf
    // installed by a benchmark, so rows don't affect each other.
#if QT_CONFIG(settings)
    QLibraryInfoPrivate::setQtconfManualPath(nullptr);
    QLibraryInfoPrivate::reload();
#endif
}

void tst_QLibraryInfo::paths_data()
{
    QTest::addColumn<QString>("qtConfPath");
    QTest::addColumn<QLibraryInfo::LibraryPath>("libraryPath");
    QTest::addColumn<bool>("cold");

    // Three resolution modes, to separate the cost of resolving the prefix
    // from the cost of reading values out of a qt.conf:
    //  - qt-relative:  no qt.conf; relative configure-time paths resolved
    //                  against the Qt prefix on every call.
    //  - app-relative: a qt.conf with a relative "Prefix = ."; values are
    //                  read via QSettings and resolved against the app prefix.
    //  - absolute:     an all-absolute qt.conf; read via QSettings, but hits
    //                  the early return that never resolves the prefix.
    // Comparing app-relative to absolute isolates the prefix-resolution cost,
    // as both go through qt.conf.
    const std::pair<const char *, QString> configs[] = {
        { "qt-relative", QString() },
        { "app-relative", u":/relative.qt.conf"_s },
        { "absolute", u":/absolute.qt.conf"_s },
    };

    // Each mode is measured both cold and cached:
    //  - cold:   the caches are dropped before each call (see paths()), so we
    //            measure the cost of an uncached first call.
    //  - cached: the configuration and resolved prefix are reused across
    //            calls, the typical steady-state cost.
    const std::pair<const char *, bool> caching[] = {
        { "cold", true },
        { "cached", false },
    };

    const QMetaEnum me = QMetaEnum::fromType<QLibraryInfo::LibraryPath>();
    for (const auto &[confTag, qtConfPath] : configs) {
        for (const auto &[cacheTag, cold] : caching) {
            for (int i = 0; i < me.keyCount(); ++i) {
                QTest::addRow("%s:%s:%s", confTag, cacheTag, me.key(i))
                    << qtConfPath << QLibraryInfo::LibraryPath(me.value(i)) << cold;
            }
        }
    }
}

void tst_QLibraryInfo::paths()
{
    QFETCH(QString, qtConfPath);
    QFETCH(const QLibraryInfo::LibraryPath, libraryPath);
    QFETCH(const bool, cold);

#if QT_CONFIG(settings)
    if (!qtConfPath.isEmpty())
        QLibraryInfoPrivate::setQtconfManualPath(&qtConfPath);
    // Reload so the cached configuration reflects the current qt.conf and
    // QCoreApplication state before we start measuring.
    QLibraryInfoPrivate::reload();
#else
    if (!qtConfPath.isEmpty())
        QSKIP("QSettings support is required to install a custom qt.conf.");
#endif

    QBENCHMARK {
        if (cold)
            QLibraryInfoPrivate::reload();
        [[maybe_unused]] const auto r = QLibraryInfo::paths(libraryPath);
    }
}

void tst_QLibraryInfo::platformPluginArguments_data()
{
    QTest::addColumn<QString>("qtConfPath");

    // Unlike paths(), this doesn't use the cached configuration: it calls
    // findConfiguration() on every call. With no qt.conf that's a cheap set
    // of lookups returning nothing; with a qt.conf it constructs and parses a
    // QSettings each time, which is the cost this variant exposes.
    QTest::newRow("default") << QString();
    QTest::newRow("qt.conf") << u":/platforms.qt.conf"_s;
}

void tst_QLibraryInfo::platformPluginArguments()
{
    QFETCH(QString, qtConfPath);

    if (!qtConfPath.isEmpty()) {
#if QT_CONFIG(settings)
        QLibraryInfoPrivate::setQtconfManualPath(&qtConfPath);
        QLibraryInfoPrivate::reload();
#else
        QSKIP("QSettings support is required to install a custom qt.conf.");
#endif
    }

    QBENCHMARK {
        [[maybe_unused]] const auto r = QLibraryInfo::platformPluginArguments(u"dummy"_s);
    }
}

QTEST_APPLESS_MAIN(tst_QLibraryInfo)

#include "tst_bench_qlibraryinfo.moc"
