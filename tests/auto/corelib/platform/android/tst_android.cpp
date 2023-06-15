// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include <jni.h>

#include <QTest>
#include <QGuiApplication>
#include <QtCore/qnativeinterface.h>
#include <QtCore/qjniobject.h>
#include <QtCore/qdiriterator.h>
#include <QScreen>
#include <qpa/qplatformscreen.h>
#include <qpa/qplatformnativeinterface.h>
#include <QtCore/qdiriterator.h>

using namespace Qt::StringLiterals;

class tst_Android : public QObject
{
Q_OBJECT
private slots:
    void assetsRead();
    void assetsNotWritable();
    void assetsIterating();
    void testAndroidSdkVersion();
    void testAndroidActivity();
    void testRunOnAndroidMainThread();
    void testFullScreenDimensions();
};

void tst_Android::assetsRead()
{
    {
        QFile file(QStringLiteral("assets:/test.txt"));
        QVERIFY(file.open(QIODevice::ReadOnly));
        QCOMPARE(file.readAll(), QByteArray("FooBar"));
    }

    {
        QFile file(QStringLiteral("assets:/test.txt"));
        QVERIFY(file.open(QIODevice::ReadOnly | QIODevice::Text));
        QCOMPARE(file.readAll(), QByteArray("FooBar"));
    }
}

void tst_Android::assetsNotWritable()
{
    QFile file(QStringLiteral("assets:/test.txt"));
    QVERIFY(!file.open(QIODevice::WriteOnly));
    QVERIFY(!file.open(QIODevice::ReadWrite));
    QVERIFY(!file.open(QIODevice::Append));
}

void tst_Android::assetsIterating()
{
    QStringList assets = {"assets:/top_level_dir/file_in_top_dir.txt",
                          "assets:/top_level_dir/sub_dir",
                          "assets:/top_level_dir/sub_dir/file_in_sub_dir.txt",
                          "assets:/top_level_dir/sub_dir/sub_dir_2",
                          "assets:/top_level_dir/sub_dir/sub_dir_2/sub_dir_3",
                          "assets:/top_level_dir/sub_dir/sub_dir_2/sub_dir_3/file_in_sub_dir_3.txt"};

    // Note that we have an "assets:/top_level_dir/sub_dir/empty_sub_dir" in the test's
    // assets physical directory, but empty folders are not packaged in the built apk,
    // so it's expected to not have such folder be listed in the assets on runtime

    QDirIterator it("assets:/top_level_dir", QDirIterator::Subdirectories);
    QStringList iteratorAssets;
    while (it.hasNext())
        iteratorAssets.append(it.next());

    QVERIFY(assets == iteratorAssets);

    auto entryList = QDir{"assets:/"_L1}.entryList(QStringList{"*.txt"_L1});
    QCOMPARE(entryList.size(), 1);
    QCOMPARE(entryList[0], "test.txt"_L1);
}

void tst_Android::testAndroidSdkVersion()
{
    QVERIFY(QNativeInterface::QAndroidApplication::sdkVersion() > 0);
}

void tst_Android::testAndroidActivity()
{
    QJniObject activity = QNativeInterface::QAndroidApplication::context();
    QVERIFY(activity.isValid());
    QVERIFY(activity.callMethod<jboolean>("isTaskRoot"));
}

void tst_Android::testRunOnAndroidMainThread()
{
    // async void
    {
        int res = 0;
        QNativeInterface::QAndroidApplication::runOnAndroidMainThread([&res]{ res = 1; });
        QTRY_COMPARE(res, 1);
    }

    // sync void
    {
        int res = 0;
        auto task = QNativeInterface::QAndroidApplication::runOnAndroidMainThread([&res]{
            res = 1;
        });
        task.waitForFinished();
        QCOMPARE(res, 1);
    }

    // sync return value
    {
        auto task = QNativeInterface::QAndroidApplication::runOnAndroidMainThread([]{
            return 1;
        });
        task.waitForFinished();
        QVERIFY(task.isResultReadyAt(0));
        QCOMPARE(task.result().value<int>(), 1);
    }

    // nested calls
    {
        // nested async/async
        int res = 0;
        QNativeInterface::QAndroidApplication::runOnAndroidMainThread([&res]{
            QNativeInterface::QAndroidApplication::runOnAndroidMainThread([&res]{
                res = 3;
            });
        });
        QTRY_COMPARE(res, 3);

        // nested async/sync
        QNativeInterface::QAndroidApplication::runOnAndroidMainThread([&res]{
            QNativeInterface::QAndroidApplication::runOnAndroidMainThread([&res]{
                res = 5;
            }).waitForFinished();
        });
        QTRY_COMPARE(res, 5);

        // nested sync/sync
        QNativeInterface::QAndroidApplication::runOnAndroidMainThread([&res]{
            QNativeInterface::QAndroidApplication::runOnAndroidMainThread([&res]{
                res = 4;
            }).waitForFinished();
        }).waitForFinished();
        QCOMPARE(res, 4);


        // nested sync/async
        QNativeInterface::QAndroidApplication::runOnAndroidMainThread([&res]{
            QNativeInterface::QAndroidApplication::runOnAndroidMainThread([&res]{
                res = 6;
            });
        }).waitForFinished();
        QCOMPARE(res, 6);
    }

    // timeouts
    {
        auto task = QNativeInterface::QAndroidApplication::runOnAndroidMainThread([]{
            QThread::msleep(500);
            return 1;
        }, QDeadlineTimer(100));
        task.waitForFinished();
        QVERIFY(task.isCanceled());
        QVERIFY(task.isFinished());
        QVERIFY(!task.isResultReadyAt(0));

        auto task2 = QNativeInterface::QAndroidApplication::runOnAndroidMainThread([]{
            return 2;
        }, QDeadlineTimer(0));
        task2.waitForFinished();
        QVERIFY(task2.isCanceled());
        QVERIFY(task2.isFinished());
        QVERIFY(!task2.isResultReadyAt(0));

        QDeadlineTimer deadline(1000);
        auto task3 = QNativeInterface::QAndroidApplication::runOnAndroidMainThread([]{
            return 3;
        }, QDeadlineTimer(10000));
        task3.waitForFinished();
        QVERIFY(deadline.remainingTime() > 0);
        QVERIFY(task3.isFinished());
        QVERIFY(!task3.isCanceled());
        QVERIFY(task3.isResultReadyAt(0));
        QCOMPARE(task3.result().value<int>(), 3);
    }

    // cancelled future
    {
        auto task = QNativeInterface::QAndroidApplication::runOnAndroidMainThread([]{
            QThread::msleep(2000);
            return 1;
        });
        task.cancel();
        QVERIFY(task.isCanceled());
        task.waitForFinished();
        QVERIFY(task.isFinished());
        QVERIFY(!task.isResultReadyAt(0));
    }
}

void setSystemUiVisibility(int visibility)
{
    QNativeInterface::QAndroidApplication::runOnAndroidMainThread([visibility] {
        QJniObject::callStaticMethod<void>("org/qtproject/qt/android/QtNative",
                                           "setSystemUiVisibility", "(I)V", visibility);
    }).waitForFinished();
}

// QTBUG-107604
void tst_Android::testFullScreenDimensions()
{
    static int SYSTEM_UI_VISIBILITY_NORMAL = 0;
    static int SYSTEM_UI_VISIBILITY_FULLSCREEN = 1;
    static int SYSTEM_UI_VISIBILITY_TRANSLUCENT = 2;

    // this will trigger new layout updates
    setSystemUiVisibility(SYSTEM_UI_VISIBILITY_FULLSCREEN);
    setSystemUiVisibility(SYSTEM_UI_VISIBILITY_NORMAL);

    QJniObject activity = QNativeInterface::QAndroidApplication::context();
    QVERIFY(activity.isValid());

    QJniObject windowManager =
            activity.callObjectMethod("getWindowManager", "()Landroid/view/WindowManager;");
    QVERIFY(windowManager.isValid());

    QJniObject display = windowManager.callObjectMethod("getDefaultDisplay", "()Landroid/view/Display;");
    QVERIFY(display.isValid());

    QJniObject appSize("android/graphics/Point");
    QVERIFY(appSize.isValid());

    display.callMethod<void>("getSize", "(Landroid/graphics/Point;)V", appSize.object());

    QJniObject realSize("android/graphics/Point");
    QVERIFY(realSize.isValid());

    display.callMethod<void>("getRealSize", "(Landroid/graphics/Point;)V", realSize.object());

    QPlatformScreen *screen = QGuiApplication::primaryScreen()->handle();

    {
        // Normal -
        // available geometry == app size (system bars visible and removed from available geometry)
        QCoreApplication::processEvents();
        QJniObject window = activity.callObjectMethod("getWindow", "()Landroid/view/Window;");
        QVERIFY(window.isValid());

        QJniObject decorView = window.callObjectMethod("getDecorView", "()Landroid/view/View;");
        QVERIFY(decorView.isValid());

        QJniObject insets =
                decorView.callObjectMethod("getRootWindowInsets", "()Landroid/view/WindowInsets;");
        QVERIFY(insets.isValid());

        int insetsWidth = insets.callMethod<jint>("getSystemWindowInsetRight")
                + insets.callMethod<jint>("getSystemWindowInsetLeft");

        int insetsHeight = insets.callMethod<jint>("getSystemWindowInsetTop")
                + insets.callMethod<jint>("getSystemWindowInsetBottom");

        QTRY_COMPARE(screen->availableGeometry().width(),
                     int(appSize.getField<jint>("x")) - insetsWidth);
        QTRY_COMPARE(screen->availableGeometry().height(),
                     int(appSize.getField<jint>("y")) - insetsHeight);

        QTRY_COMPARE(screen->geometry().width(), int(realSize.getField<jint>("x")));
        QTRY_COMPARE(screen->geometry().height(), int(realSize.getField<jint>("y")));
    }

    {
        setSystemUiVisibility(SYSTEM_UI_VISIBILITY_FULLSCREEN);

        // Fullscreen
        // available geometry == full display size (system bars hidden)
        QCoreApplication::processEvents();
        QTRY_COMPARE(screen->availableGeometry().width(), int(realSize.getField<jint>("x")));
        QTRY_COMPARE(screen->availableGeometry().height(), int(realSize.getField<jint>("y")));

        QTRY_COMPARE(screen->geometry().width(), int(realSize.getField<jint>("x")));
        QTRY_COMPARE(screen->geometry().height(), int(realSize.getField<jint>("y")));
    }

    {
        setSystemUiVisibility(SYSTEM_UI_VISIBILITY_TRANSLUCENT);

        // Translucent
        // available geometry == full display size (system bars visible but drawable under)
        QCoreApplication::processEvents();
        QTRY_COMPARE(screen->availableGeometry().width(), int(realSize.getField<jint>("x")));
        QTRY_COMPARE(screen->availableGeometry().height(), int(realSize.getField<jint>("y")));

        QTRY_COMPARE(screen->geometry().width(), int(realSize.getField<jint>("x")));
        QTRY_COMPARE(screen->geometry().height(), int(realSize.getField<jint>("y")));
    }
}

QTEST_MAIN(tst_Android)
#include "tst_android.moc"

