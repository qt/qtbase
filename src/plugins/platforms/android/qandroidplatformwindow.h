// Copyright (C) 2014 BogDan Vatra <bogdan@kde.org>
// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only
// Qt-Security score:significant reason:default

#ifndef ANDROIDPLATFORMWINDOW_H
#define ANDROIDPLATFORMWINDOW_H
#include <qobject.h>
#include <qrect.h>
#include <qpa/qplatformwindow.h>
#include <QtCore/qjnienvironment.h>
#include <QtCore/qjniobject.h>
#include <QtCore/qjnitypes.h>
#include <QtCore/qloggingcategory.h>
#include <jni.h>

struct ANativeWindow;

QT_BEGIN_NAMESPACE

Q_DECLARE_LOGGING_CATEGORY(lcQpaWindow)
Q_DECLARE_JNI_CLASS(QtWindow, "org/qtproject/qt/android/QtWindow")
Q_DECLARE_JNI_CLASS(Surface, "android/view/Surface")
Q_DECLARE_JNI_CLASS(Insets, "android/graphics/Insets")
Q_DECLARE_JNI_CLASS(Runnable, "java/lang/Runnable")

class QAndroidPlatformScreen;

// Surface lifecycle. The Qt GUI thread owns m_nativeWindow and the expose
// state. JNI surface callbacks are queued unless Qt is parked.
// When Qt destroys a window it releases the GPU surface itself first.
// The render thread creates its EGL/Vk surface lazily from m_nativeWindow, and
// isExposed() stays false until a Surface arrives.
//
// m_nativeWindow needs no atomics. The GUI thread writes it only while the
// render thread is paused by SurfaceAboutToBeDestroyed, and the render thread
// reads it only after the posted expose event, which orders the write.

class QAndroidPlatformWindow: public QPlatformWindow
{
public:
    enum class SurfaceContainer {
        SurfaceView,
        TextureView
    };

    explicit QAndroidPlatformWindow(QWindow *window);
    void initialize() override;

    ~QAndroidPlatformWindow();
    void lower() override;
    void raise() override;

    void setVisible(bool visible) override;

    void setWindowState(Qt::WindowStates state) override;
    void setWindowFlags(Qt::WindowFlags flags) override;
    void setParent(const QPlatformWindow *window) override;

    WId winId() const override;

    bool setMouseGrabEnabled(bool grab) override { Q_UNUSED(grab); return false; }
    bool setKeyboardGrabEnabled(bool grab) override { Q_UNUSED(grab); return false; }

    QAndroidPlatformScreen *platformScreen() const;

    QMargins safeAreaMargins() const override;
    void setSafeAreaMargins(const QMargins safeMargins);

    void propagateSizeHints() override;
    void requestActivateWindow() override;
    void applySystemUi(Qt::WindowStates states, Qt::WindowFlags flags);
    void updateFocusedEditText();
    inline bool isRaster() const { return m_isRaster; }
    bool isExposed() const override;
    QtJniTypes::QtWindow nativeWindow() const { return m_nativeQtWindow; }

    virtual void applicationStateChanged(Qt::ApplicationState);
    void sendChildExposeEvents() const;
    bool isChildOfQtWindow() const;
    void updateDerivedSafeAreaMargins();
    int nativeViewId() const { return m_nativeViewId; }

    static bool registerNatives(QJniEnvironment &env);

    // GUI-thread handlers for the queued JNI surface callbacks.
    void onSurfaceCreated(QtJniTypes::Surface surface);
    void onRedrawNeeded();
    void onSurfaceDestroyed();

protected:
    void setGeometry(const QRect &rect) override;
    void updateNativeWindow(bool visible);
    bool blockedByModal() const;
    bool isEmbeddingContainer() const;

    // Ends Qt's use of the Surface. Runs on the GUI thread, or inline while
    // Qt is parked.
    void releaseSurface();

    // Also removes the Surface from Java, which doesn't report that back.
    void destroySurface();

    // Fetches the Surface from Java without blocking, so makeCurrent() right
    // after QWindow::create() works. GUI thread only.
    void ensureNativeWindow();

    // Written on the GUI thread, read on the render thread after the expose
    // event that follows the write.
    ANativeWindow *m_nativeWindow = nullptr;

    // Between Android's surfaceCreated and surfaceDestroyed. Unlike
    // m_nativeWindow, it drops before the GPU surface is torn down.
    bool m_surfaceValid = false;

    bool m_isRaster;

    int m_nativeViewId = -1;
    QtJniTypes::QtWindow m_nativeQtWindow;
    SurfaceContainer m_surfaceContainerType = SurfaceContainer::SurfaceView;
    QtJniTypes::QtWindow m_nativeParentQtWindow;

    QMargins m_safeAreaMargins;

private:
    // The EGL or Vulkan surface, which has to go before the ANativeWindow.
    virtual void destroyWindowSurface() {}

    static void surfaceCreated(JNIEnv *env, jobject obj, jint windowId, QtJniTypes::Surface surface);
    Q_DECLARE_JNI_NATIVE_METHOD_IN_CURRENT_SCOPE(surfaceCreated)
    static void surfaceRedrawNeeded(JNIEnv *env, jobject obj, jint windowId,
                                    QtJniTypes::Runnable drawingFinished);
    Q_DECLARE_JNI_NATIVE_METHOD_IN_CURRENT_SCOPE(surfaceRedrawNeeded)
    static void surfaceDestroyed(JNIEnv *env, jobject obj, jint windowId,
                                 QtJniTypes::Runnable surfaceReleased);
    Q_DECLARE_JNI_NATIVE_METHOD_IN_CURRENT_SCOPE(surfaceDestroyed)
    static void windowFocusChanged(JNIEnv *env, jobject object, jboolean focus, jint windowId);
    Q_DECLARE_JNI_NATIVE_METHOD_IN_CURRENT_SCOPE(windowFocusChanged)
    static void safeAreaMarginsChanged(JNIEnv *env, jobject obj, QtJniTypes::Insets insets, jint  id);
    Q_DECLARE_JNI_NATIVE_METHOD_IN_CURRENT_SCOPE(safeAreaMarginsChanged)
    static void applySystemUiNative(JNIEnv *env, jobject obj);
    Q_DECLARE_JNI_NATIVE_METHOD_IN_CURRENT_SCOPE(applySystemUiNative)

    static QAndroidPlatformWindow *findWindowById(jint windowId);
};

QT_END_NAMESPACE
#endif // ANDROIDPLATFORMWINDOW_H
