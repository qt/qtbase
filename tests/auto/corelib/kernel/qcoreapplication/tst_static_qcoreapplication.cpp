// Copyright (C) 2024 Intel Corporation.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only

#include <QtTest/QTest>
#include <private/qcoreapplication_p.h>
#include <private/qhooks_p.h>

#ifdef QT_WIDGETS_LIB
#  define tst_Static_QCoreApplication tst_Static_QApplication
using App = QApplication;
#elif defined(QT_GUI_LIB)
#  define tst_Static_QCoreApplication tst_Static_QGuiApplication
using App = QGuiApplication;
#else
using App = QCoreApplication;
#endif

#include "maybeshow.h"

class tst_Static_QCoreApplication : public QObject
{
    Q_OBJECT
public:
    ~tst_Static_QCoreApplication() {}
private Q_SLOTS:
    void staticApplication();
};

void tst_Static_QCoreApplication::staticApplication()
{
    // This test only verifies that the destructor, when run inside of exit(),
    // will not crash.
    static int argc = 1;
    const char *argv[] = { staticMetaObject.className(), nullptr };
    static App app(argc, const_cast<char **>(argv));
    [[maybe_unused]] static auto w = maybeShowSomething();
}

struct HookManager
{
    static inline QBasicAtomicInt objectCount = {};
    static void addObject(QObject *o)
    {
        // printf("+ %p = %s\n", o, o->metaObject()->className());
        objectCount.ref();
        Q_UNUSED(o);
    }
    static void removeObject(QObject *o)
    {
        objectCount.deref();
        // printf("- %p\n", o);
        Q_UNUSED(o);
    }

    HookManager()
    {
        qtHookData[QHooks::AddQObject] = reinterpret_cast<quintptr>(&addObject);
        qtHookData[QHooks::RemoveQObject] = reinterpret_cast<quintptr>(&removeObject);
    }

    ~HookManager()
    {
        qtHookData[QHooks::AddQObject] = 0;
        qtHookData[QHooks::RemoveQObject] = 0;

        auto app = qApp;
        if (app)
            qFatal("qApp was not destroyed = %p", app);
#if !defined(QT_GUI_LIB) && !defined(Q_OS_WIN)
        // Only tested for QtCore/QCoreApplication on Unix. QtGui has statics
        // with QObject that haven't been cleaned up.
        // For QtCore, we expect exactly one object: the QAdoptedThread for
        // represents the main thread.
        if (int c = objectCount.loadRelaxed(); c > 1)
            qFatal("%d objects still alive", c);
#endif
    }
};
static HookManager hookManager;

QTEST_APPLESS_MAIN(tst_Static_QCoreApplication)
#include "tst_static_qcoreapplication.moc"

