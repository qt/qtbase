// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only

#include <jni.h>

#include <QTest>
#include <QGuiApplication>
#include <QtCore/qnativeinterface.h>
#include <QtCore/qjniobject.h>
#include <QtCore/qdiriterator.h>
#include <QScreen>
#include <QWindow>
#include <qpa/qplatformscreen.h>
#include <qpa/qplatformnativeinterface.h>
#include <QtCore/qdiriterator.h>
#include <private/qglobal_p.h>  // for widgets feature test
#if QT_CONFIG(widgets)
#include <QWidget>
#endif
#include <QSignalSpy>
#include <QtCore/private/qexpected_p.h>
#include <QtCore/qscopeguard.h>

#include <utility>

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
Q_DECLARE_JNI_CLASS(ViewParent, "android/view/ViewParent")
Q_DECLARE_JNI_CLASS(ViewGroup, "android/view/ViewGroup")
Q_DECLARE_JNI_CLASS(LayoutParams, "android/view/ViewGroup$LayoutParams")
Q_DECLARE_JNI_CLASS(WindowLayoutParams, "android/view/WindowManager$LayoutParams")
Q_DECLARE_JNI_CLASS(IBinder, "android/os/IBinder")
Q_DECLARE_JNI_CLASS(InputMethodManager, "android/view/inputmethod/InputMethodManager")
Q_DECLARE_JNI_CLASS(TestEditText, "org/qtproject/qt/android/TestEditText")
Q_DECLARE_JNI_CLASS(QtLayoutParams, "org/qtproject/qt/android/QtLayout$LayoutParams")
Q_DECLARE_JNI_CLASS(QtWindowInsetsController, "org/qtproject/qt/android/QtWindowInsetsController")

class tst_Android : public QObject
{
Q_OBJECT
private slots:
    void assetsRead();
    void assetsNotWritable();
    void assetsIterating();
    void assetsDotSegments_data();
    void assetsDotSegments();
    void testAndroidSdkVersion();
    void testAndroidActivity();
    void testRunOnAndroidMainThread();
    void gracefullyFailLoadingMissingLibrary();
    void expandedClientAreaGeometry_data();
    void expandedClientAreaGeometry();
#if QT_CONFIG(widgets)
    void safeAreaWithWindowFlagsAndStates_data();
    void safeAreaWithWindowFlagsAndStates();
    void rootMarginsWithKeyboard();
    void rootMarginsWithoutEnforcement();
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

void tst_Android::assetsDotSegments_data()
{
    QTest::addColumn<QString>("path");
    QTest::addColumn<QString>("cleanPath");
    QTest::addColumn<bool>("isDir");

    QTest::newRow("dot in root") << "assets:/./test.txt" << "assets:/test.txt" << false;
    QTest::newRow("dot in subdir") << "assets:/top_level_dir/./file_in_top_dir.txt"
                                   << "assets:/top_level_dir/file_in_top_dir.txt" << false;
    QTest::newRow("dotdot") << "assets:/top_level_dir/sub_dir/../file_in_top_dir.txt"
                            << "assets:/top_level_dir/file_in_top_dir.txt" << false;
    QTest::newRow("dot before dir") << "assets:/top_level_dir/./sub_dir/"
                                    << "assets:/top_level_dir/sub_dir" << true;
    QTest::newRow("root dot") << "assets:/." << "assets:/" << true;
}

void tst_Android::assetsDotSegments()
{
    QFETCH(QString, path);
    QFETCH(QString, cleanPath);
    QFETCH(bool, isDir);

    const QFileInfo info(path);
    QVERIFY(info.exists());
    QCOMPARE(info.isDir(), isDir);
    QCOMPARE(info.absoluteFilePath(), cleanPath);
    if (isDir)
        return;

    QFile file(path);
    QVERIFY(file.open(QIODevice::ReadOnly));
    QFile cleanFile(cleanPath);
    QVERIFY(cleanFile.open(QIODevice::ReadOnly));
    QCOMPARE(file.readAll(), cleanFile.readAll());
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

void tst_Android::expandedClientAreaGeometry_data()
{
    QTest::addColumn<Qt::WindowState>("state");
    QTest::newRow("normal") << Qt::WindowNoState;
    QTest::newRow("maximized") << Qt::WindowMaximized;
    QTest::newRow("fullscreen") << Qt::WindowFullScreen;
}

void tst_Android::expandedClientAreaGeometry()
{
    QFETCH(Qt::WindowState, state);

    QWindow window;
    window.setFlags(Qt::Window | Qt::ExpandedClientAreaHint);
    const QRect screenGeometry = window.screen()->geometry();
    const QRect requestedGeometry(screenGeometry.topLeft() + QPoint(20, 20),
                                  screenGeometry.size() / 2);
    window.setGeometry(requestedGeometry);
    window.setWindowState(state);
    // show() would replace the requested state with Android's default.
    window.setVisible(true);
    QVERIFY(QTest::qWaitForWindowExposed(&window));
    QCOMPARE(window.windowState(), state);
    const QRect expectedGeometry = state == Qt::WindowNoState ? requestedGeometry : screenGeometry;
    QTRY_COMPARE(window.geometry(), expectedGeometry);

    window.hide();
    window.setVisible(true);
    QVERIFY(QTest::qWaitForWindowExposed(&window));
    QTRY_COMPARE(window.geometry(), expectedGeometry);
}

#if QT_CONFIG(widgets)
using WindowModes = QList<std::pair<Qt::WindowStates, Qt::WindowFlags>>;

void tst_Android::safeAreaWithWindowFlagsAndStates_data()
{
    QTest::addColumn<WindowModes>("windowModes");

    QTest::newRow("normal")
        << WindowModes{{Qt::WindowMaximized, Qt::Window}};
    QTest::newRow("expanded")
        << WindowModes{{Qt::WindowMaximized, Qt::Window | Qt::ExpandedClientAreaHint}};
    QTest::newRow("fullscreen")
        << WindowModes{{Qt::WindowFullScreen, Qt::Window}};
    QTest::newRow("fullscreen-expanded")
        << WindowModes{{Qt::WindowFullScreen, Qt::Window | Qt::ExpandedClientAreaHint}};
    QTest::newRow("normal-expanded-fullscreen-normal")
        << WindowModes{
            {Qt::WindowMaximized, Qt::Window},
            {Qt::WindowMaximized, Qt::Window | Qt::ExpandedClientAreaHint},
            {Qt::WindowFullScreen, Qt::Window},
            {Qt::WindowMaximized, Qt::Window}
        };
    QTest::newRow("expanded-normal")
        << WindowModes{
            {Qt::WindowMaximized, Qt::Window | Qt::ExpandedClientAreaHint},
            {Qt::WindowMaximized, Qt::Window}
        };
}

void tst_Android::safeAreaWithWindowFlagsAndStates()
{
    QFETCH(WindowModes, windowModes);

    using namespace QtJniTypes;
    auto *iface = qGuiApp->nativeInterface<QNativeInterface::QAndroidApplication>();
    QVERIFY(iface);
    Activity activity = iface->context().object();
    QVERIFY(activity.isValid());
    const int sdkVersion = iface->sdkVersion();
    const bool enforced = QtWindowInsetsController::callStaticMethod<jboolean>(
            "isEdgeToEdgeEnforced", Context(activity.object()));

    QWidget widget;
    QPalette palette = widget.palette();
    palette.setColor(QPalette::Window, Qt::red);
    widget.setAutoFillBackground(true);
    widget.setPalette(palette);

    for (const auto &[windowStates, windowFlags] : windowModes) {
        const bool fullscreen = windowStates & Qt::WindowFullScreen;
        widget.setWindowFlags(windowFlags);
        widget.ensurePolished();
        widget.setWindowState(windowStates);
        // show() would replace fullscreen with Android's default window state.
        widget.setVisible(true);

        QVERIFY(QTest::qWaitForWindowExposed(&widget));

        Window window = activity.callMethod<Window>("getWindow");
        View decorView = window.callMethod<View>("getDecorView");
        WindowInsets insets = decorView.callMethod<WindowInsets>("getRootWindowInsets");
        QVERIFY(insets.isValid());

        bool cameraCutout = false;
        if (sdkVersion >= __ANDROID_API_R__) {
            DisplayCutout cutout = insets.callMethod<DisplayCutout>("getDisplayCutout");
            if (cutout.isValid()) {
                const int top = cutout.callMethod<jint>("getSafeInsetTop");
                const int left = cutout.callMethod<jint>("getSafeInsetLeft");
                const int right = cutout.callMethod<jint>("getSafeInsetRight");
                const int bottom = cutout.callMethod<jint>("getSafeInsetBottom");
                cameraCutout = (top > 0) || (left > 0) || (right > 0) || (bottom > 0);
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

        const bool expandedClientArea = windowFlags & Qt::ExpandedClientAreaHint;
        const bool normalMode = !expandedClientArea && !fullscreen;

        if (normalMode || (fullscreen && !cameraCutout)) {
            QTRY_COMPARE(widget.windowHandle()->safeAreaMargins(), QMargins());
        } else {
            QTRY_COMPARE_NE(widget.windowHandle()->safeAreaMargins(), QMargins());

            // Other margins vary between Android versions, so only check the top.
            qreal dpr = widget.devicePixelRatio();
            QCOMPARE_LE(widget.windowHandle()->safeAreaMargins().top(),
                        qRound(topStableInset / dpr));
        }

        QWindow child(widget.windowHandle());
        child.setGeometry(QRect(QPoint(-20, -20), widget.size() + QSize(40, 40)));
        child.show();
        QVERIFY(QTest::qWaitForWindowExposed(&child));
        QTRY_COMPARE(child.safeAreaMargins(), widget.windowHandle()->safeAreaMargins());
        child.setGeometry(QRect(QPoint(), widget.size()));
        QTRY_COMPARE(child.safeAreaMargins(), widget.windowHandle()->safeAreaMargins());
        child.hide();

        if (!enforced)
            continue;

        const QSize decorSize(decorView.callMethod<jint>("getWidth"),
                              decorView.callMethod<jint>("getHeight"));
        QSize nativeSize = decorSize;
        if (normalMode) {
            const jint types = WindowInsetsType::callStaticMethod<jint>("systemBars")
                    | WindowInsetsType::callStaticMethod<jint>("displayCutout");
            GraphicsInsets safe = insets.callMethod<GraphicsInsets>(
                    "getInsetsIgnoringVisibility", types);
            QVERIFY(safe.isValid());
            const QSize insetSize(safe.getField<jint>("left") + safe.getField<jint>("right"),
                                  safe.getField<jint>("top") + safe.getField<jint>("bottom"));
            nativeSize -= insetSize;
        }
        QTRY_COMPARE(widget.size(), nativeSize / widget.devicePixelRatio());
    }
}

static QMargins rootMargins(const QtJniTypes::ViewGroup &root)
{
    const auto params = root.callMethod<QtJniTypes::LayoutParams>("getLayoutParams");
    return QMargins(params.getField<jint>("leftMargin"), params.getField<jint>("topMargin"),
                    params.getField<jint>("rightMargin"), params.getField<jint>("bottomMargin"));
}

void tst_Android::rootMarginsWithKeyboard()
{
    using namespace QtJniTypes;
    auto *iface = qGuiApp->nativeInterface<QNativeInterface::QAndroidApplication>();
    QVERIFY(iface);
    Activity activity = iface->context().object();
    QVERIFY(activity.isValid());
    const bool enforced = QtWindowInsetsController::callStaticMethod<jboolean>(
            "isEdgeToEdgeEnforced", Context(activity.object()));
    if (!enforced)
        QSKIP("Requires enforced edge-to-edge");

    QWidget widget;
    widget.showMaximized();
    QVERIFY(QTest::qWaitForWindowExposed(&widget));
    QTRY_COMPARE(widget.windowHandle()->safeAreaMargins(), QMargins());

    View view(reinterpret_cast<jobject>(widget.winId()));
    ViewGroup root(view.callMethod<ViewParent>("getParent").object());
    QVERIFY(root.isValid());
    QVERIFY(root.callMethod<LayoutParams>("getLayoutParams").isValid());
    const QMargins initialMargins = rootMargins(root);

    Window window = activity.callMethod<Window>("getWindow");
    auto attributes = window.callMethod<WindowLayoutParams>("getAttributes");
    const jint softInputMode = attributes.getField<jint>("softInputMode");
    InputMethodManager manager(activity.callMethod<jobject>(
        "getSystemService", u"input_method"_s).object());
    QVERIFY(manager.isValid());

    constexpr jint adjustMask = 0xf0;
    constexpr jint adjustResize = 0x10;
    auto created = iface->runOnAndroidMainThread(
        [activity, root, window, softInputMode] {
            window.callMethod<void>("setSoftInputMode",
                                     (softInputMode & ~adjustMask) | adjustResize);
            // Avoid Qt input queries racing later EGL surface creation.
            auto editor = TestEditText::construct(Context(activity.object()));
            auto layout = QtLayoutParams::construct(jint(300), jint(100), jint(20), jint(20));
            root.callMethod<void>("addView", View(editor.object()), LayoutParams(layout.object()));
            editor.callMethod<jboolean>("requestFocus");
            return QVariant::fromValue(QJniObject(editor.object()));
        });
    QTRY_VERIFY(created.isFinished());
    const View editor(created.result().value<QJniObject>());
    QVERIFY(editor.isValid());

    const auto cleanup = [iface, root, editor, manager, window, softInputMode] {
        return iface->runOnAndroidMainThread(
            [root, editor, manager, window, softInputMode] {
                auto token = editor.callMethod<IBinder>("getWindowToken");
                manager.callMethod<jboolean>("hideSoftInputFromWindow", token, jint(0));
                editor.callMethod<void>("clearFocus");
                root.callMethod<void>("removeView", editor);
                window.callMethod<void>("setSoftInputMode", softInputMode);
            });
    };
    auto removeEditor = qScopeGuard([cleanup] { cleanup(); });

    QTRY_VERIFY(editor.callMethod<jboolean>("hasFocus"));
    QTRY_VERIFY(editor.callMethod<jboolean>("hasWindowFocus"));
    iface->runOnAndroidMainThread([editor, manager] {
        manager.callMethod<jboolean>("showSoftInput", editor, jint(0));
    });

    View decor = window.callMethod<View>("getDecorView");
    const jint ime = WindowInsetsType::callStaticMethod<jint>("ime");
    const auto keyboardVisible = [decor, ime] {
        WindowInsets insets = decor.callMethod<WindowInsets>("getRootWindowInsets");
        return insets.callMethod<jboolean>("isVisible", ime);
    };
    QTRY_VERIFY(keyboardVisible());
    QTRY_VERIFY(editor.getField<jboolean>("m_imeAnimationFinished"));
    QCOMPARE(rootMargins(root), initialMargins);
    auto cleaned = cleanup();
    QTRY_VERIFY(cleaned.isFinished());
    removeEditor.dismiss();
    QTRY_VERIFY(!keyboardVisible());
    QTRY_COMPARE(rootMargins(root), initialMargins);
}

void tst_Android::rootMarginsWithoutEnforcement()
{
    using namespace QtJniTypes;
    auto *iface = qGuiApp->nativeInterface<QNativeInterface::QAndroidApplication>();
    QVERIFY(iface);
    Activity activity = iface->context().object();
    QVERIFY(activity.isValid());
    const bool enforced = QtWindowInsetsController::callStaticMethod<jboolean>(
            "isEdgeToEdgeEnforced", Context(activity.object()));
    if (enforced)
        QSKIP("Requires app-managed edge-to-edge");

    auto result = iface->runOnAndroidMainThread([activity] {
        constexpr jint contentId = 0x01020002; // android.R.id.content
        ViewGroup content(activity.callMethod<View>("findViewById", contentId).object());
        ViewGroup root(content.callMethod<View>("getChildAt", jint(0)).object());
        auto params = root.callMethod<LayoutParams>("getLayoutParams");
        const QMargins original = rootMargins(root);
        params.callMethod<void>("setMargins", jint(1), jint(2), jint(3), jint(4));
        root.callMethod<void>("setLayoutParams", params);
        QtWindowInsetsController::callStaticMethod<void>("showExpanded", activity);
        const QMargins observed = rootMargins(root);
        params.callMethod<void>("setMargins", original.left(), original.top(),
                                original.right(), original.bottom());
        root.callMethod<void>("setLayoutParams", params);
        QtWindowInsetsController::callStaticMethod<void>("showNormal", activity);
        return QVariant::fromValue(observed);
    });
    QTRY_VERIFY(result.isFinished());
    QCOMPARE(result.result().value<QMargins>(), QMargins(1, 2, 3, 4));
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
            display.callMethod<void>("getRealSize", jappSize);
            return QSize(jappSize.getField<jint>("x"), jappSize.getField<jint>("y"));
        }
    };

    auto realSize = [=]() {
        QtJniTypes::Point jrealSize{};
        display.callMethod<void>("getRealSize", jrealSize);
        return QSize(jrealSize.getField<jint>("x"), jrealSize.getField<jint>("y"));
    }();

    auto insetsSize = [=]() {
        auto window = activity.callMethod<QtJniTypes::Window>("getWindow");
        auto decorView = window.callMethod<QtJniTypes::View>("getDecorView");
        auto insets = decorView.callMethod<QtJniTypes::WindowInsets>("getRootWindowInsets");

        if (sdkVersion >= __ANDROID_API_R__) {
            const jint types = QtJniTypes::WindowInsetsType::callStaticMethod<jint>("systemBars")
                | QtJniTypes::WindowInsetsType::callStaticMethod<jint>("displayCutout");
            auto safe = insets.callMethod<QtJniTypes::GraphicsInsets>("getInsets", types);
            return QSize(safe.getField<jint>("left") + safe.getField<jint>("right"),
                         safe.getField<jint>("top") + safe.getField<jint>("bottom"));
        } else {
            // Use legacy insets before WindowInsets.Type is available.
            int insetRight = insets.callMethod<jint>("getSystemWindowInsetRight");
            int insetLeft = insets.callMethod<jint>("getSystemWindowInsetLeft");
            int insetTop = insets.callMethod<jint>("getSystemWindowInsetTop");
            int insetBottom = insets.callMethod<jint>("getSystemWindowInsetBottom");

            return QSize(insetRight + insetLeft, insetTop + insetBottom);
        }
    };

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
        // Re-read on each retry, the insets at test start can be stale.
        QTRY_COMPARE(screen->availableGeometry().size(), appSize() - insetsSize());
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
    // Wait for the first paint, or the first rotation flakes on slower devices.
    QVERIFY(QTest::qWaitForWindowExposed(&widget));

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

    struct OrientationValues {
        const QSize screenSize;
        const qreal physicalDPI;
        const qreal physicalXDPI;
        const qreal physicalYDPI;
        const QSizeF physicalSize;
    };

    auto requestOrientation = [nativeOrientation, context](Qt::ScreenOrientation expected) {
        context.callMethod("setRequestedOrientation", nativeOrientation(expected));
    };

    auto restoreOrientation = qScopeGuard([&] {
        requestOrientation(Qt::PortraitOrientation);
        orientationSpy.wait();
        QTRY_COMPARE(screen->orientation(), Qt::PortraitOrientation);
    });

    auto testOrientation = [&](Qt::ScreenOrientation expected, const OrientationValues &orientationValues) {
        requestOrientation(expected);
        orientationSpy.wait();
        QTRY_COMPARE(screen->orientation(), expected);
        QCOMPARE(orientationSpy.size(), 1);
        // For QTBUG-94459 to verify widget size consistency after orientation changes.
        // In general we can't guarantee the order though, since Android might send the
        // orientation and size change at any order, so we need to use QTRY_COMPARE().
        QTRY_COMPARE(screen->size(), orientationValues.screenSize);
        QTRY_COMPARE(widget.size(), screen->availableSize());
        QTRY_COMPARE(screen->physicalDotsPerInch(), orientationValues.physicalDPI);
        QTRY_COMPARE(screen->physicalDotsPerInchX(), orientationValues.physicalXDPI);
        QTRY_COMPARE(screen->physicalDotsPerInchY(), orientationValues.physicalYDPI);
        QTRY_COMPARE(screen->physicalSize(), orientationValues.physicalSize);

        // Sample pixels at the centre, edges and corners. A missed expose or
        // stale size shows the clear colour instead of the widget background.
        QWindow *winHandle = widget.windowHandle();
        QVERIFY2(winHandle, "widget has no window handle after orientation change");
        QTRY_VERIFY_WITH_TIMEOUT(winHandle->isExposed(), 5000);
        QPixmap shot;
        QTRY_VERIFY_WITH_TIMEOUT(!(shot = widget.grab()).isNull(), 5000);
        // grab() returns physical pixels and widget.size() is logical, so
        // compare against the size scaled by the device pixel ratio.
        const QSize expectedShotSize = (QSizeF(widget.size()) * widget.devicePixelRatio()).toSize();
        QCOMPARE(shot.size(), expectedShotSize);
        const QImage frame = shot.toImage();
        const QColor expectedColor(Qt::red);
        const int w = frame.width();
        const int h = frame.height();
        // Inset by a few px from each edge to skip any anti-aliased seam.
        const int inset = 4;
        struct Sample { const char *name; QPoint at; };
        const Sample samples[] = {
            { "centre",       { w / 2,         h / 2         } },
            { "top-edge",     { w / 2,         inset         } },
            { "bottom-edge",  { w / 2,         h - 1 - inset } },
            { "left-edge",    { inset,         h / 2         } },
            { "right-edge",   { w - 1 - inset, h / 2         } },
            { "top-left",     { inset,         inset         } },
            { "top-right",    { w - 1 - inset, inset         } },
            { "bottom-left",  { inset,         h - 1 - inset } },
            { "bottom-right", { w - 1 - inset, h - 1 - inset } },
        };
        for (const Sample &s : samples) {
            const QColor got(frame.pixel(s.at));
            QVERIFY2(got == expectedColor,
                        qPrintable(QStringLiteral("sample %1 at (%2, %3) in %4x%5: got %6, expected %7")
                                        .arg(QLatin1StringView(s.name))
                                        .arg(s.at.x()).arg(s.at.y())
                                        .arg(w).arg(h)
                                        .arg(got.name()).arg(expectedColor.name())));
        }

        orientationSpy.clear();
    };

    // A device left rotated would skew the baselines below.
    if (screen->orientation() != Qt::PortraitOrientation) {
        requestOrientation(Qt::PortraitOrientation);
        QTRY_COMPARE(screen->orientation(), Qt::PortraitOrientation);
        QTRY_VERIFY(screen->size().height() > screen->size().width());
        orientationSpy.clear();
    }

    const QSize portraitSize = screen->size();
    const QSize landscapeSize = QSize(portraitSize.height(), portraitSize.width());
    // QTBUG-142560 - test that the screen's physical DPI remains consistent across orientation changes
    // Failure was caused by uncertain order of events during orientation change, where the screen
    // geometry change event could be processed before the orientation change event, which caused
    // the physical DPI to be recalculated based on the wrong screen orientation.
    const qreal initialPixelDensity = screen->physicalDotsPerInch();
    QCOMPARE_GT(initialPixelDensity, 0.0);
    const qreal initialDPIX = screen->physicalDotsPerInchX();
    QCOMPARE_GT(initialDPIX, 0.0);
    const qreal initialDPIY = screen->physicalDotsPerInchY();
    QCOMPARE_GT(initialDPIY, 0.0);
    const QSizeF initialPhysicalSizePort = screen->physicalSize();
    const QSizeF initialPhysicalSizeLand = initialPhysicalSizePort.transposed();

    const OrientationValues landscapeValues {landscapeSize, initialPixelDensity, initialDPIY,
        initialDPIX, initialPhysicalSizeLand};
    const OrientationValues portraitValues { portraitSize, initialPixelDensity, initialDPIX,
        initialDPIY, initialPhysicalSizePort};

    // Sequential 90 degrees clock-wise rotations
    testOrientation(Qt::InvertedLandscapeOrientation, landscapeValues);
    testOrientation(Qt::InvertedPortraitOrientation, portraitValues);
    testOrientation(Qt::LandscapeOrientation, landscapeValues);
    testOrientation(Qt::PortraitOrientation, portraitValues);

    // Sequential 90 degrees counter-clockwise rotations
    testOrientation(Qt::LandscapeOrientation, landscapeValues);
    testOrientation(Qt::InvertedPortraitOrientation, portraitValues);
    testOrientation(Qt::InvertedLandscapeOrientation, landscapeValues);

    // 180 degree rotations
    testOrientation(Qt::InvertedPortraitOrientation, portraitValues);
    testOrientation(Qt::PortraitOrientation, portraitValues);
    testOrientation(Qt::InvertedLandscapeOrientation, landscapeValues);
    testOrientation(Qt::LandscapeOrientation, landscapeValues);
}
#endif // QT_CONFIG(widgets)

QTEST_MAIN(tst_Android)
#include "tst_android.moc"
