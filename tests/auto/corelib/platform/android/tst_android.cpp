// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only

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
#include <QWidget>
#include <QSignalSpy>

using namespace Qt::StringLiterals;

Q_DECLARE_JNI_CLASS(Display, "android/view/Display")
Q_DECLARE_JNI_CLASS(Point, "android/graphics/Point")
Q_DECLARE_JNI_CLASS(Rect, "android/graphics/Rect")
Q_DECLARE_JNI_CLASS(View, "android/view/View")
Q_DECLARE_JNI_CLASS(Window, "android/view/Window")
Q_DECLARE_JNI_CLASS(WindowInsets, "android/view/WindowInsets")
Q_DECLARE_JNI_CLASS(WindowManager, "android/view/WindowManager")
Q_DECLARE_JNI_CLASS(WindowMetrics, "android/view/WindowMetrics")

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
    void orientationChange_data();
    void orientationChange();
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

// QTBUG-107604
void tst_Android::testFullScreenDimensions()
{
    QJniObject activity = QNativeInterface::QAndroidApplication::context();
    QVERIFY(activity.isValid());

    QJniObject windowManager = activity.callMethod<QtJniTypes::WindowManager>("getWindowManager");
    QVERIFY(windowManager.isValid());

    QJniObject display = windowManager.callMethod<QtJniTypes::Display>("getDefaultDisplay");
    QVERIFY(display.isValid());

    QSize appSize;
    if (QNativeInterface::QAndroidApplication::sdkVersion() >= __ANDROID_API_R__) {
        using namespace QtJniTypes;
        auto windowMetrics = windowManager.callMethod<WindowMetrics>("getCurrentWindowMetrics");
        auto bounds = windowMetrics.callMethod<Rect>("getBounds");
        appSize.setWidth(bounds.callMethod<int>("width"));
        appSize.setHeight(bounds.callMethod<int>("height"));
    } else {
        QtJniTypes::Point jappSize{};
        display.callMethod<void>("getSize", jappSize);
        appSize.setWidth(jappSize.getField<jint>("x"));
        appSize.setHeight(jappSize.getField<jint>("y"));
    }

    QtJniTypes::Point realSize{};
    QVERIFY(realSize.isValid());
    display.callMethod<void>("getRealSize", realSize);

    QWidget widget;
    QPlatformScreen *screen = QGuiApplication::primaryScreen()->handle();
    {
        // Normal -
        // available geometry == app size (system bars visible and removed from available geometry)
        widget.showNormal();
        QCoreApplication::processEvents();
        QJniObject window = activity.callMethod<QtJniTypes::Window>("getWindow");
        QVERIFY(window.isValid());

        QJniObject decorView = window.callMethod<QtJniTypes::View>("getDecorView");
        QVERIFY(decorView.isValid());

        QJniObject insets = decorView.callMethod<QtJniTypes::WindowInsets>("getRootWindowInsets");
        QVERIFY(insets.isValid());

        int insetRight = insets.callMethod<jint>("getSystemWindowInsetRight");
        int insetLeft = insets.callMethod<jint>("getSystemWindowInsetLeft");
        int insetsWidth = insetRight + insetLeft;

        int insetTop = insets.callMethod<jint>("getSystemWindowInsetTop");
        int insetBottom = insets.callMethod<jint>("getSystemWindowInsetBottom");
        int insetsHeight = insetTop + insetBottom;

        int expectedWidth = appSize.width() - insetsWidth;
        QTRY_COMPARE(screen->availableGeometry().width(), expectedWidth);

        int expectedHeight = appSize.height() - insetsHeight;
        QTRY_COMPARE(screen->availableGeometry().height(), expectedHeight);

        QTRY_COMPARE(screen->geometry().width(), realSize.getField<jint>("x"));
        QTRY_COMPARE(screen->geometry().height(), realSize.getField<jint>("y"));
    }

    {
        // Fullscreen
        // available geometry == full display size (system bars hidden)
        widget.showFullScreen();
        QCoreApplication::processEvents();
        QTRY_COMPARE(screen->availableGeometry().width(), realSize.getField<jint>("x"));
        QTRY_COMPARE(screen->availableGeometry().height(), realSize.getField<jint>("y"));

        QTRY_COMPARE(screen->geometry().width(), realSize.getField<jint>("x"));
        QTRY_COMPARE(screen->geometry().height(), realSize.getField<jint>("y"));
        widget.showNormal();
    }

    {
        // Translucent
        // available geometry == full display size (system bars visible but drawable under)
        widget.setWindowFlags(widget.windowFlags() | Qt::MaximizeUsingFullscreenGeometryHint);
        widget.show();
        QCoreApplication::processEvents();
        QTRY_COMPARE(screen->availableGeometry().width(), realSize.getField<jint>("x"));
        QTRY_COMPARE(screen->availableGeometry().height(), realSize.getField<jint>("y"));

        QTRY_COMPARE(screen->geometry().width(), realSize.getField<jint>("x"));
        QTRY_COMPARE(screen->geometry().height(), realSize.getField<jint>("y"));
        widget.showNormal();
    }

    {
        // Translucent
        // available geometry == full display size (system bars visible but drawable under)
        widget.showMaximized();
        QCoreApplication::processEvents();
        QTRY_COMPARE(screen->availableGeometry().width(), realSize.getField<jint>("x"));
        QTRY_COMPARE(screen->availableGeometry().height(), realSize.getField<jint>("y"));

        QTRY_COMPARE(screen->geometry().width(), realSize.getField<jint>("x"));
        QTRY_COMPARE(screen->geometry().height(), realSize.getField<jint>("y"));
    }
}

void tst_Android::orientationChange_data()
{
    QTest::addColumn<int>("nativeOrientation");
    QTest::addColumn<Qt::ScreenOrientation>("expected");
    QTest::addColumn<QSize>("screenSize");

    const QSize portraitSize = QGuiApplication::primaryScreen()->size();
    const QSize landscapeSize = QSize(portraitSize.height(), portraitSize.width());

    // Rotations without 180 degree or inverted portrait, assuming that the device is in portrait
    // position. These are ok for Android 6(API 23), 8 (API 27) and 14 (API 34)
    QTest::newRow("InvertedLandscape") << 8 << Qt::InvertedLandscapeOrientation << landscapeSize;
    QTest::newRow("Portrait") << 1 << Qt::PortraitOrientation << portraitSize;
    QTest::newRow("Landscape") << 0 << Qt::LandscapeOrientation << landscapeSize;
    QTest::newRow("Portrait2") << 1 << Qt::PortraitOrientation << portraitSize;

    // Rotations over inverted portrait doing only 90 degree turns.
    QTest::newRow("InvertedLandscape2") << 8 << Qt::InvertedLandscapeOrientation << landscapeSize;
    QTest::newRow("InvertedPortrait") << 9 << Qt::InvertedPortraitOrientation << portraitSize;
    QTest::newRow("Landscape2") << 0 << Qt::LandscapeOrientation << landscapeSize;
    QTest::newRow("InvertedPortrait2") << 9 << Qt::InvertedPortraitOrientation << portraitSize;
    QTest::newRow("InvertedLandscape3") << 8 << Qt::InvertedLandscapeOrientation << landscapeSize;

    // Rotations with 180 degree turns.
    // Android 6 (API23) Does not understand these transitions.
    if (QNativeInterface::QAndroidApplication::sdkVersion() > __ANDROID_API_M__) {
        QTest::newRow("Landscape3") << 0 << Qt::LandscapeOrientation << landscapeSize;
        QTest::newRow("InvertedLandscape4")
                << 8 << Qt::InvertedLandscapeOrientation << landscapeSize;
        QTest::newRow("Portrait3") << 1 << Qt::PortraitOrientation << portraitSize;
    } else {
        qWarning() << "180 degree turn rotation test cases are not run on Android 6 (API 23) and "
                      "below.";
    }
    // Android 8 (API 27) does not understand portrait-'inverted portrait'-portrait transition.
    if (QNativeInterface::QAndroidApplication::sdkVersion() > __ANDROID_API_O_MR1__) {
        QTest::newRow("InvertedPortrait3") << 9 << Qt::InvertedPortraitOrientation << portraitSize;
        QTest::newRow("Portrait4") << 1 << Qt::PortraitOrientation << portraitSize;
    } else {
        qWarning() << "Portrait-'Inverted portrait'-Portrait rotation test cases are not run on "
                      "Android 8 (API 27) and below.";
    }
}

void tst_Android::orientationChange()
{
    QFETCH(int, nativeOrientation);
    QFETCH(Qt::ScreenOrientation, expected);
    QFETCH(QSize, screenSize);

    if (QNativeInterface::QAndroidApplication::sdkVersion() == __ANDROID_API_P__)
        QSKIP("Android 9 orientation changes callbacks are buggy (QTBUG-124890).");

    // For QTBUG-94459 to check that the widget size are consistent after orientation changes
    QWidget widget;
    widget.show();

    QScreen *screen = QGuiApplication::primaryScreen();
    QSignalSpy orientationSpy(screen, SIGNAL(orientationChanged(Qt::ScreenOrientation)));

    auto context = QNativeInterface::QAndroidApplication::context();
    context.callMethod<void>("setRequestedOrientation", nativeOrientation);

    orientationSpy.wait();
    QTRY_COMPARE(screen->orientation(), expected);
    QCOMPARE(orientationSpy.size(), 1);
    QCOMPARE(screen->size(), screenSize);
    QCOMPARE(widget.size(), screen->availableSize());
}

QTEST_MAIN(tst_Android)
#include "tst_android.moc"
