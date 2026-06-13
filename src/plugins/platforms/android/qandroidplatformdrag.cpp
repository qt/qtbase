// Copyright (C) 2026 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only
// Qt-Security score:significant reason:default

#include "qandroidplatformdrag.h"

#if QT_CONFIG(draganddrop)

#include "androidjnimain.h"
#include "qandroidplatformwindow.h"

#include <QtCore/QEventLoop>
#include <QtCore/QJniEnvironment>
#include <QtCore/QJniObject>
#include <QtCore/QLoggingCategory>
#include <QtCore/QMetaMethod>
#include <QtCore/QMimeData>
#include <QtCore/QThread>
#include <QtCore/QUrl>
#include <QtCore/qcoreapplication_platform.h>
#include <QtCore/private/qjnihelpers_p.h>
#include <QtGui/QDrag>
#include <QtGui/QGuiApplication>
#include <QtGui/QPixmap>
#include <qpa/qwindowsysteminterface.h>

QT_BEGIN_NAMESPACE

using namespace Qt::StringLiterals;
using namespace QtJniTypes;

Q_DECLARE_JNI_CLASS(Bitmap, "android/graphics/Bitmap")
Q_DECLARE_JNI_CLASS(FileProvider, "androidx/core/content/FileProvider")

Q_LOGGING_CATEGORY(lcQpaDrag, "qt.qpa.drag")

QAndroidPlatformDrag *QAndroidPlatformDrag::s_currentDrag = nullptr;

// Exposes a local file through FileProvider as a content:// URI that other apps can read.
QString QAndroidPlatformDrag::shareableUri(const QString &value)
{
    const QUrl url(value.trimmed());
    if (!url.isLocalFile())
        return value;
    auto *iface = qApp->nativeInterface<QNativeInterface::QAndroidApplication>();
    if (!iface)
        return {};
    const Context context = iface->context();
    if (!context.isValid())
        return {};

    const QString authority = context.callMethod<QString>("getPackageName") + ".qtprovider"_L1;
    const File file(url.toLocalFile());
    const auto uri = FileProvider::callStaticMethod<Uri>("getUriForFile", context, authority, file);
    if (!uri.isValid())
        return {};

    return uri.toString();
}

// See https://developer.android.com/reference/android/view/DragEvent
namespace {
enum AndroidDragAction : int {
    ActionDragStarted = 1,
    ActionDragLocation = 2,
    ActionDrop = 3,
    ActionDragEnded = 4,
    ActionDragEntered = 5,
    ActionDragExited = 6,
};
}

QAndroidPlatformDrag::QAndroidPlatformDrag()
{
    s_currentDrag = this;
    m_dragManager = QtDragManager::callStaticMethod<QtDragManager>("getInstance");
    const jlong self = static_cast<jlong>(reinterpret_cast<quintptr>(this));
    QtDragManager::callStaticMethod<void>("setNativePointer", self);
}

QAndroidPlatformDrag::~QAndroidPlatformDrag()
{
    // Clear only if still ours, so a freshly created integration is not zeroed.
    if (s_currentDrag == this)
        s_currentDrag = nullptr;
    const jlong self = static_cast<jlong>(reinterpret_cast<quintptr>(this));
    QtDragManager::callStaticMethod<void>("clearNativePointer", self);
}

bool QAndroidPlatformDrag::ownsDragObject() const
{
    return false;
}

Qt::DropAction QAndroidPlatformDrag::drag(QDrag *drag)
{
    // A nested drag would overwrite m_dragLoop and never let the outer one finish.
    if (m_dragLoop) {
        qCWarning(lcQpaDrag, "A drag is already in progress; nested drags are not supported.");
        return Qt::IgnoreAction;
    }

    m_executedAction = Qt::IgnoreAction;

    // Without the manager, startDrag() is a no-op and nothing would wake the drag loop.
    if (!m_dragManager.isValid())
        return Qt::IgnoreAction;

    QWindow *window = QGuiApplication::focusWindow();
    if (!window || !window->handle()) {
        qCWarning(lcQpaDrag, "No focused window to start a drag.");
        return Qt::IgnoreAction;
    }

    auto *platformWindow = static_cast<QAndroidPlatformWindow *>(window->handle());
    QtWindow sourceView = platformWindow->nativeWindow();
    if (!sourceView.isValid())
        return Qt::IgnoreAction;

    const QMimeData *mimeData = drag->mimeData();
    QStringList mimeTypes;
    QStringList clipData;
    for (const QString &format : mimeData->formats()) {
        QString value;
        if (format == "text/uri-list"_L1) {
            // Expose only shareable URIs and report any local file that cannot be shared.
            QStringList uris;
            for (const QUrl &url : mimeData->urls()) {
                const QString uri = shareableUri(url.toString());
                if (uri.isEmpty()) {
                    qCWarning(lcQpaDrag, "Cannot expose non-shareable %s to other apps.",
                        qPrintable(url.toString()));
                    continue;
                }
                uris.append(uri);
            }
            value = uris.join(u'\n');
        } else if (format.startsWith("text/"_L1)) {
            value = QString::fromUtf8(mimeData->data(format));
        }
        // Skip empty formats so a receiver sees no phantom types.
        if (!value.isEmpty()) {
            mimeTypes.append(format);
            clipData.append(value);
        }
    }

    // Drag shadow from the QDrag pixmap.
    Bitmap shadowBitmap = QJniObject();
    QPixmap pixmap = drag->pixmap();
    if (pixmap.isNull() && drag->source()) {
        // Android renders to a SurfaceView, so the platform drag shadow builder
        // cannot capture window content. Grab the source as a pixmap instead.
        const QMetaObject *metaObject = drag->source()->metaObject();
        if (const int idx = metaObject->indexOfMethod("grab(QRect)"); idx >= 0) {
            metaObject->method(idx).invoke(drag->source(), Qt::DirectConnection, qReturnArg(pixmap),
                                           Q_ARG(QRect, QRect(QPoint(0, 0), QSize(-1, -1))));
        }
    }
    QPoint hotSpot;
    if (!pixmap.isNull()) {
        QJniEnvironment env;
        if (jobject bitmap = QtAndroid::createBitmap(pixmap.toImage(), env.jniEnv())) {
            shadowBitmap = Bitmap(bitmap);
            env->DeleteLocalRef(bitmap);
        }
        const qreal dpr = pixmap.devicePixelRatio();
        if (const QPoint dragHotSpot = drag->hotSpot(); !dragHotSpot.isNull())
            hotSpot = QPoint(qRound(dragHotSpot.x() * dpr), qRound(dragHotSpot.y() * dpr));
        else
            hotSpot = QPoint(pixmap.width() / 2, pixmap.height() / 2);
    }
    m_dragManager.callMethod<void>("startDrag", sourceView, mimeTypes, clipData,
                                   shadowBitmap, hotSpot.x(), hotSpot.y());

    QEventLoop loop;
    m_dragLoop = &loop;
    loop.exec();
    m_dragLoop = nullptr;

    return m_executedAction;
}

void QAndroidPlatformDrag::cancelDrag()
{
    if (m_dragManager.isValid())
        m_dragManager.callMethod<void>("cancelDrag");
    if (m_dragLoop)
        m_dragLoop->quit();
}

bool QAndroidPlatformDrag::handleNativeDragEvent(jlong nativePointer, int viewId, int action,
                                                 float x, float y, const QStringList &mimeTypes,
                                                 const QStringList &clipData, bool result)
{
    auto onGuiThread = [nativePointer, viewId, action, x, y, mimeTypes, clipData, result] {
        QAndroidPlatformDrag *drag = s_currentDrag;
        if (!drag || reinterpret_cast<quintptr>(drag) != quintptr(nativePointer))
            return false;
        return drag->deliverDragEvent(viewId, action, x, y, mimeTypes, clipData, result);
    };

    // onDragEvent() runs on the Android UI thread and QWSI must deliver it on the Qt GUI thread.
    if (QThread::currentThread() == qApp->thread())
        return onGuiThread();

    // ActionDragEnded needs no reply, so post it non-blocking.
    if (action == ActionDragEnded) {
        QMetaObject::invokeMethod(qApp, [onGuiThread] { onGuiThread(); }, Qt::QueuedConnection);
        return true;
    }

    QtAndroidPrivate::AndroidDeadlockProtector protector(u"QAndroidPlatformDrag"_s);
    if (!protector.acquire()) {
        qCWarning(lcQpaDrag, "Skipping drag event delivery to avoid a UI/Qt thread deadlock.");
        return false;
    }

    bool accepted = false;
    QMetaObject::invokeMethod(qApp, [&] {
        accepted = onGuiThread();
    }, Qt::BlockingQueuedConnection);
    return accepted;
}

bool QAndroidPlatformDrag::deliverDragEvent(int viewId, int action, float x, float y,
                                            const QStringList &mimeTypes,
                                            const QStringList &clipData,
                                            bool result)
{
    // Quit the source's drag loop even if its window is gone, or drag() hangs.
    if (action == ActionDragEnded) {
        m_externalMimeData.reset();
        // A cross-app drop reports only success, not the action, so assume the default.
        if (result && m_executedAction == Qt::IgnoreAction) {
            QDrag *drag = currentDrag();
            m_executedAction = drag ? drag->defaultAction() : Qt::CopyAction;
        }
        if (m_dragLoop)
            m_dragLoop->quit();
        return true;
    }

    QWindow *window = QtAndroid::windowFromId(viewId);
    if (!window) {
        qCWarning(lcQpaDrag, "Cannot deliver drag event to null window with id %d.", viewId);
        return false;
    }

    // A same-app drag still has its QDrag and live mime data. A cross-process
    // drag has none, so reconstruct a QMimeData from the clip's mime types.
    const QPoint pos(qRound(x), qRound(y));
    const QMimeData *mimeData = nullptr;
    Qt::DropActions supportedActions = Qt::CopyAction;
    if (QDrag *drag = currentDrag()) {
        mimeData = drag->mimeData();
        supportedActions = drag->supportedActions();
        m_externalMimeData.reset();
    } else if (!mimeTypes.isEmpty()) {
        if (!m_externalMimeData)
            m_externalMimeData = std::make_unique<QMimeData>();
        m_externalMimeData->clear();
        for (qsizetype i = 0; i < mimeTypes.size(); ++i) {
            const QString data = clipData.size() > i ? clipData.at(i) : QString();
            m_externalMimeData->setData(mimeTypes.at(i), data.toUtf8());
        }
        mimeData = m_externalMimeData.get();
    }

    switch (action) {
    case ActionDragLocation: {
        const QPlatformDragQtResponse response = QWindowSystemInterface::handleDrag(
            window, mimeData, pos, supportedActions, Qt::LeftButton, Qt::NoModifier);
        return response.isAccepted();
    }
    case ActionDragExited: {
        const QPlatformDragQtResponse response = QWindowSystemInterface::handleDrag(
            window, nullptr, QPoint(), supportedActions, Qt::NoButton, Qt::NoModifier);
        return response.isAccepted();
    }
    case ActionDrop: {
        const QPlatformDropQtResponse response = QWindowSystemInterface::handleDrop(
            window, mimeData, pos, supportedActions, Qt::LeftButton, Qt::NoModifier);
        m_executedAction = response.isAccepted() ? response.acceptedAction() : Qt::IgnoreAction;
        return response.isAccepted();
    }
    case ActionDragEntered:
    default:
        return true;
    }
}

jboolean QAndroidPlatformDrag::onDragEvent(JNIEnv *env, jobject obj, jlong nativePointer,
                                           jint viewId, jint action, jfloat x, jfloat y,
                                           QJniArray<QString> mimeTypes,
                                           QJniArray<QString> clipData, jboolean result)
{
    Q_UNUSED(env)
    Q_UNUSED(obj)

    if (!nativePointer)
        return JNI_FALSE;

    const QStringList types = mimeTypes.toContainer();
    const QStringList data = clipData.toContainer();
    if (!handleNativeDragEvent(nativePointer, viewId, action, x, y, types, data, result))
        return JNI_FALSE;

    return JNI_TRUE;
}

bool QAndroidPlatformDrag::registerNatives(QJniEnvironment &env)
{
    bool success = env.registerNativeMethods(
            Traits<QtDragManager>::className(),
            { Q_JNI_NATIVE_SCOPED_METHOD(onDragEvent, QAndroidPlatformDrag) });
    if (!success) {
        qCCritical(lcQpaDrag, "Failed to register QtDragManager native methods");
        return false;
    }
    return true;
}

QT_END_NAMESPACE

#endif // QT_CONFIG(draganddrop)
