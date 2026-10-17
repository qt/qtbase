// Copyright (C) 2026 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only

#include <QTest>
#include <QGuiApplication>
#include <QtCore/qnativeinterface.h>
#include <QtCore/qjniobject.h>
#include <QtCore/qsemaphore.h>
#include <QtCore/qscopeguard.h>
#include <QtCore/qtimer.h>
#include <QtGui/qevent.h>
#include <QtGui/qwindow.h>
#include <QtGui/qopenglcontext.h>
#include <android/api-level.h>
#if QT_CONFIG(egl)
#include <EGL/egl.h>
#endif
#if QT_CONFIG(vulkan)
#include <QtGui/qvulkaninstance.h>
#endif
#include <atomic>
#include <functional>
#include <memory>

using namespace Qt::StringLiterals;

Q_DECLARE_JNI_CLASS(View, "android/view/View")
Q_DECLARE_JNI_CLASS(SurfaceTexture, "android/graphics/SurfaceTexture")
Q_DECLARE_JNI_CLASS(Surface, "android/view/Surface")
Q_DECLARE_JNI_CLASS(QtWindow, "org/qtproject/qt/android/QtWindow")
Q_DECLARE_JNI_CLASS(Runnable, "java/lang/Runnable")
Q_DECLARE_JNI_CLASS(QtNative, "org/qtproject/qt/android/QtNative")
Q_DECLARE_JNI_CLASS(SurfaceHolder, "android/view/SurfaceHolder")
Q_DECLARE_JNI_CLASS(Class, "java/lang/Class")
Q_DECLARE_JNI_CLASS(QtSurfaceTest, "org/qtproject/qt/android/QtSurfaceTest")

class tst_AndroidNativeSurfaces : public QObject
{
    Q_OBJECT
public:
    // The dispatcher caches this policy during application construction.
    static void initMain()
    {
        qputenv("QT_BLOCK_EVENT_LOOPS_WHEN_SUSPENDED", "1");
    }
private:
    void checkSurfaceRetirement(bool suspend, bool cancel);
private slots:
    void renderThreadTickerStopProcessesEvents();
    void surfaceLifecycleRapidCycleNoCrash();
    void surfaceRetainedUntilNativeTeardown_data();
    void surfaceRetainedUntilNativeTeardown();
    void surfaceFailureHandling_data();
    void surfaceFailureHandling();
#if QT_CONFIG(egl)
    void surfaceRedrawPreservesEglBinding_data();
    void surfaceRedrawPreservesEglBinding();
#endif
#if QT_CONFIG(vulkan)
    void surfaceLifecycleRapidCycleNoCrash_vulkan();
#endif
    void destroyWindowWhileBlockedOnAndroidThread();
    void surfaceRetirementCancellation();
};

namespace {
// Runs the JNI surface callbacks from the Android UI thread like Android does.
// Pumps events so queued surface changes reach the Qt thread.
template <typename Fn>
void runOnAndroidUiAndPumpEvents(Fn &&fn)
{
    auto future = QNativeInterface::QAndroidApplication::runOnAndroidMainThread(
            std::forward<Fn>(fn));
    QVERIFY(QTest::qWaitFor([&future] { return future.isFinished(); }, 5000));
}

struct SurfaceObserver : QObject {
    QtJniTypes::Surface m_surface;
    int m_notifications = 0;
    bool m_onGuiThread = false;
    bool m_validDuringNotification = false;

    bool eventFilter(QObject *, QEvent *event) override
    {
        if (event->type() == QEvent::PlatformSurface
            && static_cast<QPlatformSurfaceEvent *>(event)->surfaceEventType()
                    == QPlatformSurfaceEvent::SurfaceAboutToBeDestroyed) {
            ++m_notifications;
            m_onGuiThread = QThread::currentThread() == qApp->thread();
            m_validDuringNotification = m_surface.callMethod<jboolean>("isValid");
        }
        return false;
    }
};

// Synthetic cycles quiesce this worker before releasing its native surface.
class RenderThreadTicker
{
public:
    // finish() runs on the render thread when the ticker stops.
    RenderThreadTicker(std::function<bool()> tick,
                       std::function<void()> finish = {})
        : m_finish(std::move(finish))
    {
        m_timer.setInterval(8);
        QObject::connect(&m_timer, &QTimer::timeout, &m_timer, [this, tick = std::move(tick)] {
            if (tick())
                m_ticks.fetch_add(1, std::memory_order_relaxed);
        });
        m_timer.moveToThread(&m_thread);
    }
    ~RenderThreadTicker()
    {
        if (!stop())
            qFatal("RenderThreadTicker did not stop before destruction");
    }

    QThread *thread() { return &m_thread; }
    int ticks() const { return m_ticks.load(); }

    void start()
    {
        m_thread.start();
        resume();
    }

    void resume()
    {
        QMetaObject::invokeMethod(&m_timer, [this] {
            m_timer.start();
        });
    }

    bool pause()
    {
        auto finished = std::make_shared<std::atomic<bool>>(false);
        QMetaObject::invokeMethod(&m_timer, [this, finished] {
            m_timer.stop();
            if (m_finish)
                m_finish();
            finished->store(true);
        });
        return QTest::qWaitFor([finished] { return finished->load(); }, 5000);
    }

    bool stop()
    {
        if (!m_thread.isRunning())
            return QTest::qWaitFor([this] { return m_thread.wait(0); }, 5000);
        if (!pause())
            return false;
        m_thread.quit();
        return QTest::qWaitFor([this] { return m_thread.wait(0); }, 5000);
    }

private:
    std::function<void()> m_finish;
    QThread m_thread;
    QTimer m_timer;
    std::atomic<int> m_ticks { 0 };
};

// Fires surfaceDestroyed and surfaceCreated like Android does, and checks that
// the window follows and the ticker works again with each new Surface.
void cycleSurface(QWindow &window, RenderThreadTicker &ticker, int cycles)
{
    using namespace QtJniTypes;
    QtWindow qtWindow(reinterpret_cast<jobject>(window.winId()));
    QVERIFY(qtWindow.isValid());
    const jint windowId = qtWindow.callMethod<jint>("getId");

    QTRY_VERIFY(ticker.ticks() > 10);

    SurfaceTexture surfaceTexture(jint(0));
    QVERIFY(surfaceTexture.isValid());
    surfaceTexture.callMethod<void>("setDefaultBufferSize", jint(64), jint(64));

    for (int i = 0; i < cycles; ++i) {
        QVERIFY(ticker.pause());
        runOnAndroidUiAndPumpEvents([windowId] {
            QtWindow::callStaticMethod<void>("surfaceDestroyed", windowId,
                    Runnable(static_cast<jobject>(nullptr)));
        });
        QTRY_VERIFY(!window.isExposed());

        Surface fresh(surfaceTexture);
        QVERIFY(fresh.isValid());
        runOnAndroidUiAndPumpEvents([windowId, fresh] {
            QtWindow::callStaticMethod<void>("surfaceCreated", windowId, fresh);
        });
        QTRY_VERIFY(window.isExposed());

        const int ticksBefore = ticker.ticks();
        ticker.resume();
        QTRY_VERIFY_WITH_TIMEOUT(ticker.ticks() > ticksBefore, 1000);
    }
}
} // anonymous namespace

// Checks that stopping the worker keeps GUI events flowing.
void tst_AndroidNativeSurfaces::renderThreadTickerStopProcessesEvents()
{
    QWindow window;
    QSemaphore entered;
    QSemaphore released;
    std::atomic<bool> timedOut = false;
    RenderThreadTicker ticker([&] {
        entered.release();
        if (!released.tryAcquire(1, 1000))
            timedOut.store(true);
        return true;
    });
    ticker.start();
    QVERIFY(QTest::qWaitFor([&] { return entered.available() > 0; }));
    QTimer::singleShot(0, &window, [&] { released.release(); });
    QVERIFY(ticker.stop());
    QVERIFY2(!timedOut.load(), "Stopping the ticker starved the GUI event needed by its worker");
}

// Exercises retirement with the event dispatcher running and stopping.
void tst_AndroidNativeSurfaces::surfaceRetainedUntilNativeTeardown_data()
{
    QTest::addColumn<bool>("suspend");
    QTest::newRow("running") << false;
    QTest::newRow("stop-requested") << true;
}

// Checks that Android retains its producer until GUI teardown finishes.
void tst_AndroidNativeSurfaces::surfaceRetainedUntilNativeTeardown()
{
    QFETCH(bool, suspend);
    checkSurfaceRetirement(suspend, false);
}

void tst_AndroidNativeSurfaces::checkSurfaceRetirement(bool suspend, bool cancel)
{
    if (QNativeInterface::QAndroidApplication::sdkVersion() < __ANDROID_API_Q__)
        QSKIP("Independent SurfaceControl producers require Android 10");

    QWindow window;
    window.setSurfaceType(QSurface::OpenGLSurface);
    window.show();
    QVERIFY(QTest::qWaitForWindowExposed(&window));

    const QtJniTypes::QtWindow qtWindow(reinterpret_cast<jobject>(window.winId()));
    const auto surface = qtWindow.callMethod<QtJniTypes::Surface>("getSurface");
    QVERIFY(surface.isValid());
    QVERIFY(surface.callMethod<jboolean>("isValid"));
    // Retire the native consumer before cleanup, even after an assertion fails.
    auto cleanup = qScopeGuard([&window, surface] {
        window.destroy();
        auto released = QNativeInterface::QAndroidApplication::runOnAndroidMainThread(
                [surface] { surface.callMethod<void>("release"); });
        if (!QTest::qWaitFor([&] { return released.isFinished(); }, 5000))
            qFatal("Timed out cleaning the retained surface");
    });
    SurfaceObserver observer;
    observer.m_surface = surface;
    window.installEventFilter(&observer);

    struct Result {
        QSemaphore m_done;
        bool m_valid = false;
        QtJniTypes::View m_child;
    };
    auto result = std::make_shared<Result>();
    QCoreApplication::sendPostedEvents(qApp, QEvent::MetaCall);
    QNativeInterface::QAndroidApplication::runOnAndroidMainThread([=] {
        const auto updateState = [](Qt::ApplicationState state) {
            QtJniTypes::QtNative::callStaticMethod<void>("updateApplicationState", jint(state));
        };
        auto resume = qScopeGuard([&] {
            if (suspend)
                updateState(Qt::ApplicationActive);
        });
        if (suspend)
            updateState(Qt::ApplicationSuspended);
        result->m_child = qtWindow.callMethod<QtJniTypes::View>("getChildAt", jint(0));
        qtWindow.callMethod<void>("removeView", result->m_child);
        result->m_valid = surface.callMethod<jboolean>("isValid");
        result->m_done.release();
    });
    // Hold GUI delivery until detach completes, then optionally cancel it.
    const bool detached = result->m_done.tryAcquire(1, 5000);
    if (detached && cancel)
        QCoreApplication::removePostedEvents(qApp, QEvent::MetaCall);
    QVERIFY2(detached, "Android detach did not finish within five seconds");
    QVERIFY(result->m_child.isValid());
    QVERIFY2(result->m_valid, "Android released the producer before native teardown");
    if (cancel) {
        QVERIFY(surface.callMethod<jboolean>("isValid"));
        QCOMPARE(observer.m_notifications, 0);
        window.destroy();
    } else {
        QTRY_VERIFY(!window.isExposed());
    }
    QCOMPARE(observer.m_notifications, 1);
    QVERIFY(observer.m_onGuiThread);
    QVERIFY(observer.m_validDuringNotification);
    QTRY_VERIFY(!surface.callMethod<jboolean>("isValid"));
    if (cancel)
        return;

    runOnAndroidUiAndPumpEvents([=] {
        qtWindow.callMethod<void>("addView", result->m_child, jint(0));
    });
    QVERIFY(QTest::qWaitForWindowExposed(&window));
    const auto replacement = qtWindow.callMethod<QtJniTypes::Surface>("getSurface");
    QVERIFY(replacement.isValid());
    QVERIFY(replacement.callMethod<jboolean>("isValid"));
}

// Covers failed setup, teardown and release callbacks, including combined errors.
void tst_AndroidNativeSurfaces::surfaceFailureHandling_data()
{
    QTest::addColumn<QByteArray>("method");
    QTest::newRow("failed-primer") << QByteArray("failedPrimer");
    QTest::newRow("failed-detach") << QByteArray("failedDetach");
    QTest::newRow("failed-detach-and-callback") << QByteArray("failedDetachAndCallback");
    QTest::newRow("failed-removal") << QByteArray("failedRemoval");
    QTest::newRow("failed-initialization") << QByteArray("failedInitialization");
    QTest::newRow("completion-accounting") << QByteArray("retirementAccounting");
    QTest::newRow("removal-and-release-errors") << QByteArray("retirementRemovalFailure");
}

// Checks that failures preserve exception details and complete every release once.
void tst_AndroidNativeSurfaces::surfaceFailureHandling()
{
    QFETCH(QByteArray, method);
    if ((method.startsWith("failedDetach") || method == "failedInitialization")
        && QNativeInterface::QAndroidApplication::sdkVersion() < __ANDROID_API_Q__)
        QSKIP("Independent SurfaceControl producers require Android 10");
    struct Result {
        QSemaphore m_done;
        QString m_failure;
    };
    auto result = std::make_shared<Result>();
    QNativeInterface::QAndroidApplication::runOnAndroidMainThread([=] {
        auto signal = qScopeGuard([result] { result->m_done.release(); });
        auto context = QNativeInterface::QAndroidApplication::context();
        auto failure = QtJniTypes::QtSurfaceTest::callStaticMethod<QtJniTypes::String>(
                method.constData(), context);
        result->m_failure = failure.isValid() ? failure.toString() : u"Java fault fixture failed"_s;
    });
    // Keep native callbacks queued until the Java accounting checks finish.
    QVERIFY2(result->m_done.tryAcquire(1, 5000), "Android fault fixture did not finish");
    QVERIFY2(result->m_failure.isEmpty(), qPrintable(result->m_failure));
}

#if QT_CONFIG(egl)
// Covers both an empty binding and distinct current read and draw surfaces.
void tst_AndroidNativeSurfaces::surfaceRedrawPreservesEglBinding_data()
{
    QTest::addColumn<bool>("currentContext");
    QTest::newRow("no-context") << false;
    QTest::newRow("existing-context") << true;
}

// Checks that the redraw primer restores the Android UI thread EGL binding.
void tst_AndroidNativeSurfaces::surfaceRedrawPreservesEglBinding()
{
    QFETCH(bool, currentContext);
    if (QNativeInterface::QAndroidApplication::sdkVersion() < __ANDROID_API_Q__)
        QSKIP("Independent SurfaceControl producers require Android 10");

    QWindow window;
    window.setSurfaceType(QSurface::OpenGLSurface);
    window.show();
    QVERIFY(QTest::qWaitForWindowExposed(&window));
    const QtJniTypes::QtWindow javaWindow(reinterpret_cast<jobject>(window.winId()));
    auto type = QNativeInterface::QAndroidApplication::runOnAndroidMainThread(
            [javaWindow]() -> QVariant {
        const auto child = javaWindow.callMethod<QtJniTypes::View>("getChildAt", jint(0));
        return child.callMethod<QtJniTypes::Class>("getClass")
                .callMethod<QtJniTypes::String>("getName").toString();
    });
    QVERIFY(QTest::qWaitFor([&] { return type.isFinished(); }, 5000));
    if (!type.result().toString().endsWith("QtSurface"_L1))
        QSKIP("TextureView does not use the SurfaceView EGL primer");
    struct Result {
        bool m_setup = false;
        bool m_preserved = false;
        bool m_cleanup = true;
    };
    auto result = std::make_shared<Result>();
    auto future = QNativeInterface::QAndroidApplication::runOnAndroidMainThread([=] {
        const auto child = javaWindow.callMethod<QtJniTypes::View>("getChildAt", jint(0));
        const EGLDisplay display = eglGetDisplay(EGL_DEFAULT_DISPLAY);
        if (display == EGL_NO_DISPLAY || !eglInitialize(display, nullptr, nullptr))
            return;
        const EGLDisplay previousDisplay = eglGetCurrentDisplay();
        const EGLContext previousContext = eglGetCurrentContext();
        const EGLSurface previousDraw = eglGetCurrentSurface(EGL_DRAW);
        const EGLSurface previousRead = eglGetCurrentSurface(EGL_READ);
        EGLContext context = EGL_NO_CONTEXT;
        EGLSurface draw = EGL_NO_SURFACE;
        EGLSurface read = EGL_NO_SURFACE;
        const auto cleanup = qScopeGuard([&] {
            const auto restoreDisplay = previousDisplay == EGL_NO_DISPLAY
                    ? display : previousDisplay;
            result->m_cleanup = eglMakeCurrent(restoreDisplay, previousDraw,
                                               previousRead, previousContext);
            if (read != EGL_NO_SURFACE)
                result->m_cleanup = eglDestroySurface(display, read) && result->m_cleanup;
            if (draw != EGL_NO_SURFACE)
                result->m_cleanup = eglDestroySurface(display, draw) && result->m_cleanup;
            if (context != EGL_NO_CONTEXT)
                result->m_cleanup = eglDestroyContext(display, context) && result->m_cleanup;
        });
        if (currentContext) {
            EGLConfig config;
            EGLint count = 0;
            const EGLint attributes[] = { EGL_SURFACE_TYPE, EGL_PBUFFER_BIT,
                                         EGL_RENDERABLE_TYPE, EGL_OPENGL_ES2_BIT, EGL_NONE };
            if (!eglChooseConfig(display, attributes, &config, 1, &count) || count == 0)
                return;
            const EGLint contextAttributes[] = { EGL_CONTEXT_CLIENT_VERSION, 2, EGL_NONE };
            const EGLint surfaceAttributes[] = { EGL_WIDTH, 1, EGL_HEIGHT, 1, EGL_NONE };
            context = eglCreateContext(display, config, EGL_NO_CONTEXT, contextAttributes);
            draw = eglCreatePbufferSurface(display, config, surfaceAttributes);
            read = eglCreatePbufferSurface(display, config, surfaceAttributes);
            if (context == EGL_NO_CONTEXT || draw == EGL_NO_SURFACE || read == EGL_NO_SURFACE
                    || !eglMakeCurrent(display, draw, read, context)) {
                return;
            }
        }
        result->m_setup = true;
        const EGLDisplay expectedDisplay = eglGetCurrentDisplay();
        const EGLContext expectedContext = eglGetCurrentContext();
        const EGLSurface expectedDraw = eglGetCurrentSurface(EGL_DRAW);
        const EGLSurface expectedRead = eglGetCurrentSurface(EGL_READ);
        const auto holder = child.callMethod<QtJniTypes::SurfaceHolder>("getHolder");
        const QtJniTypes::Runnable finished(QJniObject("java/lang/Thread"));
        child.callMethod<void>("surfaceRedrawNeededAsync", holder, finished);
        result->m_preserved = eglGetCurrentDisplay() == expectedDisplay
                && eglGetCurrentContext() == expectedContext
                && eglGetCurrentSurface(EGL_DRAW) == expectedDraw
                && eglGetCurrentSurface(EGL_READ) == expectedRead;
    });
    QVERIFY(QTest::qWaitFor([&] { return future.isFinished(); }, 5000));
    QVERIFY2(result->m_setup, "Could not establish the UI thread EGL binding");
    QVERIFY2(result->m_cleanup, "Could not release the test EGL resources");
    QVERIFY2(result->m_preserved, "Surface redraw replaced the UI thread EGL binding");
}
#endif

// Checks surface recreation with a worker that releases its EGL binding first.
void tst_AndroidNativeSurfaces::surfaceLifecycleRapidCycleNoCrash()
{
    QWindow window;
    window.setSurfaceType(QSurface::OpenGLSurface);
    window.show();

    QOpenGLContext ctx;
    QVERIFY(ctx.create());
    QTRY_VERIFY(window.isExposed());

    RenderThreadTicker ticker([&] {
        // Per tick, so the context picks up each new window surface.
        if (!ctx.makeCurrent(&window))
            return false;
        ctx.swapBuffers(&window);
        return true;
    }, [&] { ctx.doneCurrent(); });
    ctx.moveToThread(ticker.thread());
    ticker.start();

    cycleSurface(window, ticker, 5);
    QVERIFY(ticker.stop());
}

#if QT_CONFIG(vulkan)
// Checks Vulkan surface recreation while a worker accesses vkSurface().
void tst_AndroidNativeSurfaces::surfaceLifecycleRapidCycleNoCrash_vulkan()
{
    QVulkanInstance inst;
    if (!inst.create())
        QSKIP("No Vulkan instance available on this device");

    QWindow window;
    window.setSurfaceType(QSurface::VulkanSurface);
    window.setVulkanInstance(&inst);
    window.show();
    QTRY_VERIFY(window.isExposed());
    QVERIFY(inst.surfaceForWindow(&window) != VK_NULL_HANDLE);

    RenderThreadTicker ticker([&] {
        return inst.surfaceForWindow(&window) != VK_NULL_HANDLE;
    });
    ticker.start();

    cycleSurface(window, ticker, 5);
    QVERIFY(ticker.stop());
}
#endif // QT_CONFIG(vulkan)

// Checks teardown with the GUI blocked on Java, as in WebView construction.
void tst_AndroidNativeSurfaces::destroyWindowWhileBlockedOnAndroidThread()
{
    auto showWindow = [] {
        auto window = std::make_unique<QWindow>();
        window->setSurfaceType(QSurface::OpenGLSurface);
        window->show();
        return window;
    };

    auto first = showWindow();
    QVERIFY(QTest::qWaitForWindowExposed(first.get()));

    // Shown while the first window is up, so it gets the other surface
    // container type.
    auto second = showWindow();
    QVERIFY(QTest::qWaitForWindowExposed(second.get()));

    for (std::unique_ptr<QWindow> *window : { &second, &first }) {
        window->reset();
        // Waits without delivering events, like a synchronous call into Java.
        auto ran = std::make_shared<QSemaphore>();
        QNativeInterface::QAndroidApplication::runOnAndroidMainThread([ran] { ran->release(); });
        QVERIFY(ran->tryAcquire(1, QDeadlineTimer(5000)));
    }
}

// Runs last because cancelling queued calls can discard unrelated GUI work.
void tst_AndroidNativeSurfaces::surfaceRetirementCancellation()
{
    checkSurfaceRetirement(false, true);
}

QTEST_MAIN(tst_AndroidNativeSurfaces)
#include "tst_androidnativesurfaces.moc"
