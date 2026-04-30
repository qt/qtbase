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
#include <private/qglobal_p.h>  // for widgets feature test
#if QT_CONFIG(widgets)
#include <QWidget>
#endif
#include <QSignalSpy>
#include <QtCore/private/qexpected_p.h>

template <typename T>
using QJniResult = q23::expected<T, jthrowable>;
using namespace Qt::StringLiterals;

Q_DECLARE_JNI_CLASS(Display, "android/view/Display")
Q_DECLARE_JNI_CLASS(Point, "android/graphics/Point")
Q_DECLARE_JNI_CLASS(Rect, "android/graphics/Rect")
Q_DECLARE_JNI_CLASS(View, "android/view/View")
Q_DECLARE_JNI_CLASS(Window, "android/view/Window")
Q_DECLARE_JNI_CLASS(WindowInsets, "android/view/WindowInsets")
Q_DECLARE_JNI_CLASS(Insets, "android/view/Insets")
Q_DECLARE_JNI_CLASS(GraphicsInsets, "android/graphics/Insets")
Q_DECLARE_JNI_CLASS(DisplayCutout, "android/view/DisplayCutout")
Q_DECLARE_JNI_CLASS(WindowManager, "android/view/WindowManager")
Q_DECLARE_JNI_CLASS(WindowMetrics, "android/view/WindowMetrics")
Q_DECLARE_JNI_CLASS(ApplicationInfo, "android/content/pm/ApplicationInfo")
Q_DECLARE_JNI_CLASS(WindowInsetsType, "android/view/WindowInsets$Type")
Q_DECLARE_JNI_CLASS(QtActivityLoader, "org/qtproject/qt/android/QtActivityLoader")

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
    void gracefullyFailLoadingMissingLibrary();
#if QT_CONFIG(widgets)
    void safeAreaWithWindowFlagsAndStates_data();
    void safeAreaWithWindowFlagsAndStates();
    void testFullScreenDimensions();
    void orientationChange();
#endif
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

void tst_Android::gracefullyFailLoadingMissingLibrary()
{
    using namespace QtJniTypes;
    using namespace Qt::StringLiterals;

    auto *iface = qGuiApp->nativeInterface<QNativeInterface::QAndroidApplication>();
    QVERIFY(iface);
    QtJniTypes::Activity activity = iface->context().object();
    QVERIFY(activity.isValid());
    auto loader = QtActivityLoader::callStaticMethod<QJniResult<QtActivityLoader>>(
            "getActivityLoader", activity);
    QVERIFY(loader);
    const auto result = loader->callMethod<QJniResult<String>>(
        "loadLibraryHelper", u"invalid-libname"_s);
    QVERIFY(result);
    QVERIFY(!result->isValid());
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

#if QT_CONFIG(widgets)
void tst_Android::safeAreaWithWindowFlagsAndStates_data()
{
    QTest::addColumn<Qt::WindowStates>("windowStates");
    QTest::addColumn<Qt::WindowFlags>("windowFlags");

    QTest::newRow("Normal")
        << Qt::WindowStates(Qt::WindowNoState)
        << Qt::WindowFlags();

    QTest::newRow("Expanded Client Area")
        << Qt::WindowStates(Qt::WindowNoState)
        << Qt::WindowFlags(Qt::ExpandedClientAreaHint);

    QTest::newRow("Fullscreen")
        << Qt::WindowStates(Qt::WindowFullScreen)
        << Qt::WindowFlags();

    QTest::newRow("Fullscreen and Expanded Client Area")
        << Qt::WindowStates(Qt::WindowFullScreen)
        << Qt::WindowFlags(Qt::ExpandedClientAreaHint);
}

void tst_Android::safeAreaWithWindowFlagsAndStates()
{
    QFETCH(Qt::WindowStates, windowStates);
    QFETCH(Qt::WindowFlags, windowFlags);

    QWidget widget;
    QPalette palette = widget.palette();
    palette.setColor(QPalette::Window, Qt::red);
    widget.setAutoFillBackground(true);
    widget.setPalette(palette);
    widget.setWindowFlags(windowFlags);

    const bool fullscreen = windowStates & Qt::WindowFullScreen;
    if (fullscreen)
        widget.showFullScreen();
    else
        widget.show();

    QVERIFY(QTest::qWaitForWindowExposed(&widget));

    using namespace QtJniTypes;
    const int sdkVersion = QNativeInterface::QAndroidApplication::sdkVersion();
    auto activity = QNativeInterface::QAndroidApplication::context();
    Window window = activity.callMethod<Window>("getWindow");
    View decorView = window.callMethod<View>("getDecorView");
    WindowInsets insets = decorView.callMethod<WindowInsets>("getRootWindowInsets");
    QVERIFY(insets.isValid());

    // Detect camera cutout
    bool cameraCutout = false;
    if (sdkVersion >= __ANDROID_API_R__) {
        if (insets.isValid()) {
            DisplayCutout cutout = insets.callMethod<DisplayCutout>("getDisplayCutout");
            if (cutout.isValid()) {
                const int top = cutout.callMethod<jint>("getSafeInsetTop");
                const int left = cutout.callMethod<jint>("getSafeInsetLeft");
                const int right = cutout.callMethod<jint>("getSafeInsetRight");
                const int bottom = cutout.callMethod<jint>("getSafeInsetBottom");
                cameraCutout = (top > 0) || (left > 0) || (right > 0) || (bottom > 0);
            }
        }
    } else {
        // Android 9 and 10 cutout API support was buggy
        cameraCutout = true;
    }

    int topStableInset = 0;
    if (sdkVersion >= __ANDROID_API_R__) {
        jint systemBarsType = WindowInsetsType::callStaticMethod<jint>("systemBars");
        jint displayCutoutType = WindowInsetsType::callStaticMethod<jint>("displayCutout");
        jint combinedType = systemBarsType | displayCutoutType;

        GraphicsInsets insetsIgnoreVisibility = insets.callMethod<GraphicsInsets>(
            "getInsetsIgnoringVisibility", combinedType);
        QVERIFY(insetsIgnoreVisibility.isValid());
        topStableInset = insetsIgnoreVisibility.getField<jint>("top");
    } else {
        topStableInset = insets.callMethod<jint>("getStableInsetTop");
    }

    // Android 15 enables edge-to-edge by default, however not on Qt CI, so let's rely on
    // the reported top stable inset of decor view to judge whether edge-to-edge is enabled.
    bool edgeToEdge = sdkVersion >= __ANDROID_API_V__ && topStableInset != 0;

    const bool expandedClientArea = windowFlags & Qt::ExpandedClientAreaHint;
    const bool normalMode = !expandedClientArea && !fullscreen;

    if ((normalMode && !edgeToEdge) || (fullscreen && !cameraCutout)) {
        QTRY_COMPARE(widget.windowHandle()->safeAreaMargins(), QMargins());
    } else {
        QTRY_COMPARE_NE(widget.windowHandle()->safeAreaMargins(), QMargins());

        // Make sure the margins we get are the same as the system bars sizes,
        // that way we make sure we don't end up with margins bigger than expected.
        // So, retrieve the static system bars height.

        // Other margins can vary between Android versions, so let's only check for top
        qreal dpr = widget.devicePixelRatio();
        QCOMPARE_LE(widget.windowHandle()->safeAreaMargins().top(), qRound(topStableInset / dpr));
    }
}

// QTBUG-107604
void tst_Android::testFullScreenDimensions()
{
    QJniObject activity = QNativeInterface::QAndroidApplication::context();
    QVERIFY(activity.isValid());

    QJniObject windowManager = activity.callMethod<QtJniTypes::WindowManager>("getWindowManager");
    QVERIFY(windowManager.isValid());
    auto display = windowManager.callMethod<QtJniTypes::Display>("getDefaultDisplay");
    QVERIFY(display.isValid());

    const int sdkVersion = QNativeInterface::QAndroidApplication::sdkVersion();

    auto appSize = [=]() {
        if (sdkVersion >= __ANDROID_API_R__) {
            using namespace QtJniTypes;
            auto windowMetrics = windowManager.callMethod<WindowMetrics>("getCurrentWindowMetrics");
            auto bounds = windowMetrics.callMethod<Rect>("getBounds");
            return QSize(bounds.callMethod<int>("width"), bounds.callMethod<int>("height"));
        } else {
            QtJniTypes::Point jappSize{};
            display.callMethod<void>("getSize", jappSize);
            return QSize(jappSize.getField<jint>("x"), jappSize.getField<jint>("y"));
        }
    }();

    auto realSize = [=]() {
        QtJniTypes::Point jrealSize{};
        display.callMethod<void>("getRealSize", jrealSize);
        return QSize(jrealSize.getField<jint>("x"), jrealSize.getField<jint>("y"));
    }();

    auto targetSdkVersion = [=]() {
        const auto appContext = activity.callMethod<QtJniTypes::Context>("getApplicationContext");
        const auto appInfo = appContext.callMethod<QtJniTypes::ApplicationInfo>("getApplicationInfo");
        return appInfo.getField<jint>("targetSdkVersion");
    }();

    auto insetsSize = [=]() {
        auto window = activity.callMethod<QtJniTypes::Window>("getWindow");
        auto decorView = window.callMethod<QtJniTypes::View>("getDecorView");
        auto insets = decorView.callMethod<QtJniTypes::WindowInsets>("getRootWindowInsets");

        if (sdkVersion >= __ANDROID_API_V__  && targetSdkVersion >= __ANDROID_API_V__) {
            // Android 15 apps take all screen geomtry with edge-to-edge
            return QSize(0, 0);
        } else {
            // pre-Android 15 app size is screen size minus the system bars
            int insetRight = insets.callMethod<jint>("getSystemWindowInsetRight");
            int insetLeft = insets.callMethod<jint>("getSystemWindowInsetLeft");
            int insetTop = insets.callMethod<jint>("getSystemWindowInsetTop");
            int insetBottom = insets.callMethod<jint>("getSystemWindowInsetBottom");

            return QSize(insetRight + insetLeft, insetTop + insetBottom);
        }
    }();

    QWidget widget;
    QPalette palette = widget.palette();
    palette.setColor(QPalette::Window, Qt::red);
    widget.setAutoFillBackground(true);
    widget.setPalette(palette);
    QPlatformScreen *screen = QGuiApplication::primaryScreen()->handle();
    {
        // Normal Window
        // available geometry == app size (depending on the Android version)
        widget.showNormal();
        QTRY_COMPARE(screen->availableGeometry().size(), appSize - insetsSize);
        QTRY_COMPARE(screen->geometry().size(), realSize);
    }

    {
        // Fullscreen Window
        // available geometry == full display size (system bars hidden)
        widget.showFullScreen();
        QTRY_COMPARE(screen->availableGeometry().size(), realSize);
        QTRY_COMPARE(screen->geometry().size(), realSize);
        widget.showNormal();
    }

    {
        // Window with Qt::ExpandedClientAreaHint
        // available geometry == full display size (system bars visible but drawable under)
        widget.setWindowFlags(widget.windowFlags() | Qt::ExpandedClientAreaHint);
        widget.show();
        QTRY_COMPARE(screen->availableGeometry().size(), realSize);
        QTRY_COMPARE(screen->geometry().size(), realSize);
        widget.showNormal();
    }

    {
        // Maximized Window
        // available geometry == full display size (system bars visible but drawable under)
        widget.showMaximized();
        QTRY_COMPARE(screen->availableGeometry().size(), realSize);
        QTRY_COMPARE(screen->geometry().size(), realSize);
    }
}

void tst_Android::orientationChange()
{
    if (QNativeInterface::QAndroidApplication::sdkVersion() == __ANDROID_API_P__)
        QSKIP("Android 9 orientation changes callbacks are buggy (QTBUG-124890).");

    QWidget widget;
    QPalette palette = widget.palette();
    palette.setColor(QPalette::Window, Qt::red);
    widget.setAutoFillBackground(true);
    widget.setPalette(palette);
    widget.show();

    QScreen *screen = QGuiApplication::primaryScreen();
    QSignalSpy orientationSpy(screen, &QScreen::orientationChanged);

    auto context = QNativeInterface::QAndroidApplication::context();

    enum NativeOrientation {
        Landscape = 0,
        Portrait = 1,
        InvertedLandscape = 8,
        InvertedPortrait = 9
    };

    auto nativeOrientation = [](Qt::ScreenOrientation orientation) {
        switch (orientation) {
        case(Qt::LandscapeOrientation):
            return Landscape;
        case(Qt::PortraitOrientation):
            return Portrait;
        case(Qt::InvertedLandscapeOrientation):
            return InvertedLandscape;
        case(Qt::InvertedPortraitOrientation):
            return InvertedPortrait;
        default:
            return Portrait;
        }
    };

    auto requestOrientation = [nativeOrientation, context](Qt::ScreenOrientation expected) {
        context.callMethod("setRequestedOrientation", nativeOrientation(expected));
    };

    auto restoreOrientation = qScopeGuard([&] {
        requestOrientation(Qt::PortraitOrientation);
        orientationSpy.wait();
        QTRY_COMPARE(screen->orientation(), Qt::PortraitOrientation);
    });

    auto testOrientation = [&](Qt::ScreenOrientation expected, const QSize &screenSize) {
        requestOrientation(expected);
        orientationSpy.wait();
        QTRY_COMPARE(screen->orientation(), expected);
        QCOMPARE(orientationSpy.size(), 1);
        // For QTBUG-94459 to verify widget size consistency after orientation changes.
        // In general we can't guarantee the order though, since Android might send the
        // orientation and size change at any order, so we need to use QTRY_COMPARE().
        QTRY_COMPARE(screen->size(), screenSize);
        QTRY_COMPARE(widget.size(), screen->availableSize());
        orientationSpy.clear();
    };

    const QSize portraitSize = screen->size();
    const QSize landscapeSize = QSize(portraitSize.height(), portraitSize.width());

    // Sequential 90 degrees clock-wise rotations
    testOrientation(Qt::InvertedLandscapeOrientation, landscapeSize);
    testOrientation(Qt::InvertedPortraitOrientation, portraitSize);
    testOrientation(Qt::LandscapeOrientation, landscapeSize);
    testOrientation(Qt::PortraitOrientation, portraitSize);

    // Sequential 90 degrees counter-clockwise rotations
    testOrientation(Qt::LandscapeOrientation, landscapeSize);
    testOrientation(Qt::InvertedPortraitOrientation, portraitSize);
    testOrientation(Qt::InvertedLandscapeOrientation, landscapeSize);

    // 180 degree rotations
    testOrientation(Qt::InvertedPortraitOrientation, portraitSize);
    testOrientation(Qt::PortraitOrientation, portraitSize);
    testOrientation(Qt::InvertedLandscapeOrientation, landscapeSize);
    testOrientation(Qt::LandscapeOrientation, landscapeSize);
}
#endif // QT_CONFIG(widgets)

QTEST_MAIN(tst_Android)
#include "tst_android.moc"
