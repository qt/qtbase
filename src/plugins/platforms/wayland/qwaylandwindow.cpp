// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#include "qwaylandwindow_p.h"

#include "qwaylandbuffer_p.h"
#include "qwaylandcursor_p.h"
#include "qwaylanddisplay_p.h"
#include "qwaylandsurface_p.h"
#include "qwaylandinputdevice_p.h"
#include "qwaylandfractionalscale_p.h"
#include "qwaylandscreen_p.h"
#include "qwaylandshellsurface_p.h"
#include "qwaylandsubsurface_p.h"
#include "qwaylandabstractdecoration_p.h"
#include "qwaylandplatformservices_p.h"
#include "qwaylandnativeinterface_p.h"
#include "qwaylanddecorationfactory_p.h"
#include "qwaylandshmbackingstore_p.h"
#include "qwaylandshellintegration_p.h"
#include "qwaylandviewport_p.h"
#include "qwaylandcolormanagement_p.h"

#include <QtCore/QFileInfo>
#include <QtCore/QPointer>
#include <QtCore/QRegularExpression>
#include <QtGui/QWindow>

#include <QGuiApplication>
#include <qpa/qwindowsysteminterface.h>
#include <QtGui/private/qguiapplication_p.h>
#include <QtGui/private/qwindow_p.h>

#include <QtCore/QDebug>
#include <QtCore/QThread>
#include <QtCore/private/qthread_p.h>

#include <QtWaylandClient/private/qwayland-fractional-scale-v1.h>

QT_BEGIN_NAMESPACE

using namespace Qt::StringLiterals;

namespace QtWaylandClient {

Q_LOGGING_CATEGORY(lcWaylandBackingstore, "qt.qpa.wayland.backingstore")

QWaylandWindow *QWaylandWindow::mMouseGrab = nullptr;
QWaylandWindow *QWaylandWindow::mTopPopup = nullptr;

QWaylandWindow::QWaylandWindow(QWindow *window, QWaylandDisplay *display)
    : QPlatformWindow(window)
    , mDisplay(display)
    , mSurfaceLock(QReadWriteLock::Recursive)
    , mShellIntegration(display->shellIntegration())
    , mSurfaceFormat(window->requestedFormat())
{
    {
        bool ok;
        int frameCallbackTimeout = qEnvironmentVariableIntValue("QT_WAYLAND_FRAME_CALLBACK_TIMEOUT", &ok);
        if (ok)
            mFrameCallbackTimeout = frameCallbackTimeout;
    }

    initializeWlSurface();
    mFlags = window->flags();

    setWindowIcon(window->icon());

    connect(this, &QWaylandWindow::wlSurfaceCreated, this,
            &QNativeInterface::Private::QWaylandWindow::surfaceCreated);
    connect(this, &QWaylandWindow::wlSurfaceDestroyed, this,
            &QNativeInterface::Private::QWaylandWindow::surfaceDestroyed);
}

QWaylandWindow::~QWaylandWindow()
{
    mWindowDecoration.reset();
    reset();

    const QWindow *parent = window();
    const auto tlw = QGuiApplication::topLevelWindows();
    for (QWindow *w : tlw) {
        if (w->transientParent() == parent)
            QWindowSystemInterface::handleCloseEvent(w);
    }

    if (mMouseGrab == this) {
        mMouseGrab = nullptr;
    }
}

void QWaylandWindow::ensureSize()
{
    if (mBackingStore) {
        setBackingStore(mBackingStore);
        mBackingStore->recreateBackBufferIfNeeded();
    }
}

void QWaylandWindow::initWindow()
{
    resetFrameCallback();

    if (window()->type() == Qt::Desktop)
        return;

    if (shouldCreateSubSurface()) {
        Q_ASSERT(!mSubSurfaceWindow);

        auto *parent = static_cast<QWaylandWindow *>(QPlatformWindow::parent());
        if (!parent->mSurface)
            parent->initializeWlSurface();
        if (parent->wlSurface()) {
            if (::wl_subsurface *subsurface = mDisplay->createSubSurface(this, parent))
                mSubSurfaceWindow = new QWaylandSubSurface(this, parent, subsurface);
        }
    } else if (shouldCreateShellSurface()) {
        Q_ASSERT(!mShellSurface);
        Q_ASSERT(mShellIntegration);
        mTransientParent = guessTransientParent();
        if (mTransientParent) {
            if (window()->type() == Qt::Popup) {
                if (mTopPopup && mTopPopup != mTransientParent) {
                    qCWarning(lcQpaWayland) << "Creating a popup with a parent," << mTransientParent->window()
                                            << "which does not match the current topmost grabbing popup,"
                                            << mTopPopup->window() << "With some shell surface protocols, this"
                                            << "is not allowed. The wayland QPA plugin is currently handling"
                                            << "it by setting the parent to the topmost grabbing popup."
                                            << "Note, however, that this may cause positioning errors and"
                                            << "popups closing unxpectedly. Please fix the transient parent of the popup.";
                    mTransientParent = mTopPopup;
                }
                mTopPopup = this;
            }
        }

        mShellSurface = mShellIntegration->createShellSurface(this);
        if (mShellSurface) {
            if (mTransientParent) {
                if (window()->type() == Qt::ToolTip || window()->type() == Qt::Popup || window()->type() == Qt::Tool)
                    mTransientParent->addChildPopup(this);
            }

            // Set initial surface title
            setWindowTitle(window()->title());
            mShellSurface->setIcon(mWindowIcon);

            // The appId is the desktop entry identifier that should follow the
            // reverse DNS convention (see
            // http://standards.freedesktop.org/desktop-entry-spec/latest/ar01s02.html). According
            // to xdg-shell the appId is only the name, without the .desktop suffix.
            //
            // If the application specifies the desktop file name use that,
            // otherwise fall back to the executable name and prepend the
            // reversed organization domain when available.
            if (!QGuiApplication::desktopFileName().isEmpty()) {
                mShellSurface->setAppId(QGuiApplication::desktopFileName());
            } else {
                QFileInfo fi = QFileInfo(QCoreApplication::instance()->applicationFilePath());
                QStringList domainName =
                        QCoreApplication::instance()->organizationDomain().split(QLatin1Char('.'),
                                                                                 Qt::SkipEmptyParts);

                if (domainName.isEmpty()) {
                    mShellSurface->setAppId(fi.baseName());
                } else {
                    QString appId;
                    for (int i = 0; i < domainName.size(); ++i)
                        appId.prepend(QLatin1Char('.')).prepend(domainName.at(i));
                    appId.append(fi.baseName());
                    mShellSurface->setAppId(appId);
                }
            }
            // the user may have already set some window properties, so make sure to send them out
            for (auto it = m_properties.cbegin(); it != m_properties.cend(); ++it)
                mShellSurface->sendProperty(it.key(), it.value());

            emit surfaceRoleCreated();
        } else {
            qWarning("Could not create a shell surface object.");
        }
    }

    createDecoration();
    updateInputRegion();

    // Enable high-dpi rendering. Scale() returns the screen scale factor and will
    // typically be integer 1 (normal-dpi) or 2 (high-dpi). Call set_buffer_scale()
    // to inform the compositor that high-resolution buffers will be provided.
    if (mViewport)
        updateViewport();
    else if (mSurface->version() >= 3)
        mSurface->set_buffer_scale(std::ceil(scale()));

    QRect geometry = windowGeometry();
    QRect defaultGeometry = this->defaultGeometry();
    if (geometry.width() <= 0)
        geometry.setWidth(defaultGeometry.width());
    if (geometry.height() <= 0)
        geometry.setHeight(defaultGeometry.height());

    setGeometry_helper(geometry);
    setMask(window()->mask());
    if (mShellSurface) {
        mShellSurface->requestWindowStates(window()->windowStates());
        mShellSurface->setWindowFlags(mFlags);
    }
    handleContentOrientationChange(window()->contentOrientation());

    if (mShellSurface && mShellSurface->commitSurfaceRole())
        mSurface->commit();
}

void QWaylandWindow::setPendingImageDescription()
{
    mColorManagementSurface->setImageDescription(mPendingImageDescription.get());
}

void QWaylandWindow::initializeWlSurface()
{
    Q_ASSERT(!mSurface);
    {
        QWriteLocker lock(&mSurfaceLock);
        mSurface.reset(new QWaylandSurface(mDisplay));
        connect(mSurface.data(), &QWaylandSurface::screensChanged,
                this, &QWaylandWindow::handleScreensChanged);
        connect(mSurface.data(), &QWaylandSurface::preferredBufferScaleChanged,
                this, &QWaylandWindow::updateScale);
        connect(mSurface.data(), &QWaylandSurface::preferredBufferTransformChanged,
                this, &QWaylandWindow::updateBufferTransform);
        mSurface->m_window = this;
    }
    emit wlSurfaceCreated();

    if (mDisplay->fractionalScaleManager() && qApp->highDpiScaleFactorRoundingPolicy() == Qt::HighDpiScaleFactorRoundingPolicy::PassThrough) {
        mFractionalScale.reset(new QWaylandFractionalScale(mDisplay->fractionalScaleManager()->get_fractional_scale(mSurface->object())));

        connect(mFractionalScale.data(), &QWaylandFractionalScale::preferredScaleChanged,
                this, &QWaylandWindow::updateScale);
    }
    // The fractional scale manager check is needed to work around Gnome < 36 where viewports don't work
    // Right now viewports are only necessary when a fractional scale manager is used
    if (display()->viewporter() && display()->fractionalScaleManager()) {
        mViewport.reset(new QWaylandViewport(display()->createViewport(this)));
    }

    QColorSpace requestedColorSpace = window()->requestedFormat().colorSpace();
    if (requestedColorSpace != QColorSpace{} && mDisplay->colorManager()) {
        // TODO try a similar (same primaries + supported transfer function) color space if this fails?
        mPendingImageDescription = mDisplay->colorManager()->createImageDescription(requestedColorSpace);
        if (mPendingImageDescription) {
            if (!mColorManagementSurface)
                mColorManagementSurface = std::make_unique<ColorManagementSurface>(mDisplay->colorManager()->get_surface(surface()));
            connect(mPendingImageDescription.get(), &ImageDescription::ready, this, &QWaylandWindow::setPendingImageDescription, Qt::SingleShotConnection);
            mSurfaceFormat.setColorSpace(requestedColorSpace);
        } else {
            qCWarning(lcQpaWayland) << "couldn't create image description for requested color space" << requestedColorSpace;
        }
    }
}

void QWaylandWindow::setFormat(const QSurfaceFormat &format)
{
    const auto colorSpace = mSurfaceFormat.colorSpace();
    mSurfaceFormat = format;
    mSurfaceFormat.setColorSpace(colorSpace);
}

void QWaylandWindow::setShellIntegration(QWaylandShellIntegration *shellIntegration)
{
    Q_ASSERT(shellIntegration);
    if (mShellSurface) {
        qCWarning(lcQpaWayland) << "Cannot set shell integration while there's already a shell surface created";
        return;
    }
    mShellIntegration = shellIntegration;
}

bool QWaylandWindow::shouldCreateShellSurface() const
{
    if (!shellIntegration())
        return false;

    if (shouldCreateSubSurface())
        return false;

    if (window()->inherits("QShapedPixmapWindow"))
        return false;

    if (qEnvironmentVariableIsSet("QT_WAYLAND_USE_BYPASSWINDOWMANAGERHINT"))
        return !(window()->flags() & Qt::BypassWindowManagerHint);

    return true;
}

bool QWaylandWindow::shouldCreateSubSurface() const
{
    return QPlatformWindow::parent() != nullptr;
}

void QWaylandWindow::beginFrame()
{
    mSurfaceLock.lockForRead();
    mInFrameRender = true;
}

void QWaylandWindow::endFrame()
{
    mSurfaceLock.unlock();
    mInFrameRender = false;
}

void QWaylandWindow::reset()
{
    resetSurfaceRole();

    if (mSurface) {
        {
            QWriteLocker lock(&mSurfaceLock);
            invalidateSurface();
            mSurface.reset();
            mViewport.reset();
            mFractionalScale.reset();
            mColorManagementSurface.reset();
            mPendingImageDescription.reset();
        }
        emit wlSurfaceDestroyed();
    }


    mScale = std::nullopt;
    mOpaqueArea = QRegion();
    mMask = QRegion();

    mInputRegion = QRegion();
    mTransparentInputRegion = false;

    mDisplay->handleWindowDestroyed(this);
}

void QWaylandWindow::resetSurfaceRole()
{
     // Old Reset
    closeChildPopups();

    if (mTopPopup == this)
        mTopPopup = mTransientParent && (mTransientParent->window()->type() == Qt::Popup) ? mTransientParent : nullptr;
    if (mTransientParent)
        mTransientParent->removeChildPopup(this);
    mTransientParent = nullptr;
    delete std::exchange(mShellSurface, nullptr);
    delete std::exchange(mSubSurfaceWindow, nullptr);
    emit surfaceRoleDestroyed();

    resetFrameCallback();
    mInFrameRender = false;
    mWaitingToApplyConfigure = false;
    mExposed = false;
}

void QWaylandWindow::resetFrameCallback()
{
    {
        QMutexLocker lock(&mFrameSyncMutex);
        if (mFrameCallback) {
            wl_callback_destroy(mFrameCallback);
            mFrameCallback = nullptr;
        }
        mFrameCallbackElapsedTimer.invalidate();
        mWaitingForFrameCallback = false;
    }
    if (mFrameCallbackCheckIntervalTimerId != -1) {
        killTimer(mFrameCallbackCheckIntervalTimerId);
        mFrameCallbackCheckIntervalTimerId = -1;
    }
    mFrameCallbackTimedOut = false;
}

QWaylandWindow *QWaylandWindow::fromWlSurface(::wl_surface *surface)
{
    if (auto *s = QWaylandSurface::fromWlSurface(surface))
        return s->m_window;
    return nullptr;
}

WId QWaylandWindow::winId() const
{
    return reinterpret_cast<WId>(wlSurface());
}

void QWaylandWindow::setParent(const QPlatformWindow *parent)
{
    if (lastParent == parent)
        return;

    if (mSubSurfaceWindow && parent) { // new parent, but we were a subsurface already
        delete mSubSurfaceWindow;
        QWaylandWindow *p = const_cast<QWaylandWindow *>(static_cast<const QWaylandWindow *>(parent));
        mSubSurfaceWindow = new QWaylandSubSurface(this, p, mDisplay->createSubSurface(this, p));
    } else if ((!lastParent && parent) || (lastParent && !parent)) {
        // we're changing role, need to make a new wl_surface
        reset();
        initializeWlSurface();
        if (window()->isVisible()) {
            initWindow();
        }
    }
    lastParent = parent;
}

QString QWaylandWindow::windowTitle() const
{
    return mWindowTitle;
}

void QWaylandWindow::setWindowTitle(const QString &title)
{
    const QString separator = QString::fromUtf8(" \xe2\x80\x94 "); // unicode character U+2014, EM DASH
    const QString formatted = formatWindowTitle(title, separator);

    const int libwaylandMaxBufferSize = 4096;
    // Some parts of the buffer is used for metadata, so subtract 100 to be on the safe side.
    // Also, QString is in utf-16, which means that in the worst case each character will be
    // three bytes when converted to utf-8 (which is what libwayland uses), so divide by three.
    const int maxLength = libwaylandMaxBufferSize / 3 - 100;

    auto truncated = QStringView{formatted}.left(maxLength);
    if (truncated.size() < formatted.size()) {
        qCWarning(lcQpaWayland) << "Window titles longer than" << maxLength << "characters are not supported."
                                << "Truncating window title (from" << formatted.size() << "chars)";
    }

    mWindowTitle = truncated.toString();

    if (mShellSurface)
        mShellSurface->setTitle(mWindowTitle);

    if (mWindowDecorationEnabled && window()->isVisible())
        mWindowDecoration->update();
}

void QWaylandWindow::setWindowIcon(const QIcon &icon)
{
    mWindowIcon = icon;

    if (mWindowDecorationEnabled && window()->isVisible())
        mWindowDecoration->update();
    if (mShellSurface)
        mShellSurface->setIcon(icon);
}

QRect QWaylandWindow::defaultGeometry() const
{
    return QRect(QPoint(), QSize(500,500));
}

void QWaylandWindow::setGeometry_helper(const QRect &rect)
{
    QPlatformWindow::setGeometry(rect);
    if (mViewport)
        updateViewport();

    if (mSubSurfaceWindow) {
        QMargins m = static_cast<QWaylandWindow *>(QPlatformWindow::parent())->clientSideMargins();
        mSubSurfaceWindow->set_position(rect.x() + m.left(), rect.y() + m.top());

        QWaylandWindow *parentWindow = mSubSurfaceWindow->parent();
        if (parentWindow && parentWindow->isExposed()) {
            QRect parentExposeGeometry(QPoint(), parentWindow->geometry().size());
            parentWindow->sendExposeEvent(parentExposeGeometry);
        }
    }
}

void QWaylandWindow::setGeometry(const QRect &r)
{
    auto rect = r;
    if (fixedToplevelPositions && !QPlatformWindow::parent() && window()->type() != Qt::Popup
        && window()->type() != Qt::ToolTip && window()->type() != Qt::Tool) {
        rect.moveTo(screen()->geometry().topLeft());
    }
    setGeometry_helper(rect);

    if (mShellSurface) {
        if (!mInResizeFromApplyConfigure) {
            const QRect frameGeometry = r.marginsAdded(clientSideMargins()).marginsRemoved(windowContentMargins());
            if (qt_window_private(window())->positionAutomatic)
                mShellSurface->setWindowSize(frameGeometry.size());
            else
                mShellSurface->setWindowGeometry(frameGeometry);
        }
    }

    if (mShellSurface)
        mShellSurface->setContentGeometry(windowContentGeometry());

    if (isOpaque() && mMask.isEmpty())
        setOpaqueArea(QRect(QPoint(0, 0), rect.size()));


    if (window()->isVisible() && rect.isValid()) {
        ensureSize();
        if (mWindowDecorationEnabled)
            mWindowDecoration->update();

        QWindowSystemInterface::handleGeometryChange<QWindowSystemInterface::SynchronousDelivery>(window(), geometry());
        mSentInitialResize = true;
    }

    // Wayland has no concept of areas being exposed or not, only the entire window, when our geometry changes, we need to flag the new area as exposed
    // On other platforms (X11) the expose event would be received deferred from the X server
    // we want our behaviour to match, and only run after control has returned to the event loop
    QMetaObject::invokeMethod(this, &QWaylandWindow::synthesizeExposeOnGeometryChange, Qt::QueuedConnection);
}

void QWaylandWindow::synthesizeExposeOnGeometryChange()
{
    if (!isExposed())
        return;
    QRect exposeGeometry(QPoint(), geometry().size());
    if (exposeGeometry == mLastExposeGeometry)
        return;

    sendExposeEvent(exposeGeometry);
}

void QWaylandWindow::updateInputRegion()
{
    if (!mSurface)
        return;

    const bool transparentInputRegion = mFlags.testFlag(Qt::WindowTransparentForInput);

    QRegion inputRegion;
    if (!transparentInputRegion)
        inputRegion = mMask;

    if (mInputRegion == inputRegion && mTransparentInputRegion == transparentInputRegion)
        return;

    mInputRegion = inputRegion;
    mTransparentInputRegion = transparentInputRegion;

    if (mInputRegion.isEmpty() && !mTransparentInputRegion) {
        mSurface->set_input_region(nullptr);
    } else {
        struct ::wl_region *region = mDisplay->createRegion(mInputRegion);
        mSurface->set_input_region(region);
        wl_region_destroy(region);
    }
}

void QWaylandWindow::updateViewport()
{
    if (!surfaceSize().isEmpty())
        mViewport->setDestination(surfaceSize());
}

void QWaylandWindow::setGeometryFromApplyConfigure(const QPoint &globalPosition, const QSize &sizeWithMargins)
{
    QMargins margins = clientSideMargins();

    QPoint positionWithoutMargins = globalPosition + QPoint(margins.left(), margins.top());
    int widthWithoutMargins = qMax(sizeWithMargins.width() - (margins.left() + margins.right()), 1);
    int heightWithoutMargins = qMax(sizeWithMargins.height() - (margins.top() + margins.bottom()), 1);

    QRect geometry(positionWithoutMargins, QSize(widthWithoutMargins, heightWithoutMargins));

    mInResizeFromApplyConfigure = true;
    setGeometry(geometry);
    mInResizeFromApplyConfigure = false;
}

void QWaylandWindow::repositionFromApplyConfigure(const QPoint &globalPosition)
{
    QMargins margins = clientSideMargins();
    QPoint positionWithoutMargins = globalPosition + QPoint(margins.left(), margins.top());

    QRect geometry(positionWithoutMargins, windowGeometry().size());
    mInResizeFromApplyConfigure = true;
    setGeometry(geometry);
    mInResizeFromApplyConfigure = false;
}

void QWaylandWindow::resizeFromApplyConfigure(const QSize &sizeWithMargins, const QPoint &offset)
{
    QMargins margins = clientSideMargins();
    int widthWithoutMargins = qMax(sizeWithMargins.width() - (margins.left() + margins.right()), 1);
    int heightWithoutMargins = qMax(sizeWithMargins.height() - (margins.top() + margins.bottom()), 1);
    QRect geometry(windowGeometry().topLeft(), QSize(widthWithoutMargins, heightWithoutMargins));

    mOffset += offset;
    mInResizeFromApplyConfigure = true;
    setGeometry(geometry);
    mInResizeFromApplyConfigure = false;
}

void QWaylandWindow::sendExposeEvent(const QRect &rect)
{
    static bool sQtTestMode = qEnvironmentVariableIsSet("QT_QTESTLIB_RUNNING");
    mLastExposeGeometry = rect;

    if (sQtTestMode) {
        mExposeEventNeedsAttachedBuffer = true;
    }
    QWindowSystemInterface::handleExposeEvent<QWindowSystemInterface::SynchronousDelivery>(window(), rect);

    /**
      *  If an expose is not handled by application code, explicitly attach a buffer
      *  This primarily is a workaround for Qt unit tests using QWindow directly and
      *  wanting focus.
    */
    if (mExposed && mExposeEventNeedsAttachedBuffer && !rect.isNull()) {
        auto buffer = new QWaylandShmBuffer(mDisplay, rect.size(), QImage::Format_ARGB32);
        buffer->image()->fill(Qt::transparent);
        buffer->setDeleteOnRelease(true);
        commit(buffer, QRegion());
    }
}

QPlatformScreen *QWaylandWindow::calculateScreenFromSurfaceEvents() const
{
    QReadLocker lock(&mSurfaceLock);
    if (mSurface) {
        if (auto *screen = mSurface->oldestEnteredScreen())
            return screen;
    }
    return QPlatformWindow::screen();
}

void QWaylandWindow::setVisible(bool visible)
{
    // Workaround for issue where setVisible may be called with the same value twice
    if (lastVisible == visible)
        return;
    lastVisible = visible;

    if (visible) {
        setGeometry(windowGeometry());
        initWindow();
        updateExposure();
        if (mShellSurface)
            mShellSurface->requestActivateOnShow();
    } else {
        // make sure isExposed is false during the next event dispatch
        mExposed = false;
        sendExposeEvent(QRect());
        resetSurfaceRole();
        mSurface->attach(nullptr, 0, 0);
        mSurface->commit();
    }
}


void QWaylandWindow::raise()
{
    if (mShellSurface)
        mShellSurface->raise();
}


void QWaylandWindow::lower()
{
    if (mShellSurface)
        mShellSurface->lower();
}

void QWaylandWindow::setMask(const QRegion &mask)
{
    QReadLocker locker(&mSurfaceLock);
    if (!mSurface)
        return;

    if (mMask == mask)
        return;

    mMask = mask;

    updateInputRegion();

    if (isOpaque()) {
        if (mMask.isEmpty())
            setOpaqueArea(QRect(QPoint(0, 0), geometry().size()));
        else
            setOpaqueArea(mMask);
    }
}

void QWaylandWindow::setAlertState(bool enabled)
{
    if (mShellSurface)
        mShellSurface->setAlertState(enabled);
}

bool QWaylandWindow::isAlertState() const
{
    if (mShellSurface)
        return mShellSurface->isAlertState();

    return false;
}

void QWaylandWindow::applyConfigureWhenPossible()
{
    if (!mWaitingToApplyConfigure) {
        mWaitingToApplyConfigure = true;
        QMetaObject::invokeMethod(this, "applyConfigure", Qt::QueuedConnection);
    }
}

void QWaylandWindow::applyConfigure()
{
    if (!mWaitingToApplyConfigure)
        return;

    Q_ASSERT_X(QThread::currentThreadId() == QThreadData::get2(thread())->threadId.loadRelaxed(),
               "QWaylandWindow::applyConfigure", "not called from main thread");

    // If we're mid paint, use an exposeEvent to flush the current frame.
    // When this completes we know that no other frames will be rendering.
    // This could be improved in future as we 're blocking for not just the frame to finish but one additional extra frame.
    if (mInFrameRender)
        QWindowSystemInterface::handleExposeEvent<QWindowSystemInterface::SynchronousDelivery>(window(), QRect(QPoint(0, 0), geometry().size()));
    if (mShellSurface)
        mShellSurface->applyConfigure();

    mWaitingToApplyConfigure = false;
    if (mExposed)
        sendExposeEvent(QRect(QPoint(), geometry().size()));
    else
        // we still need to commit the configured ack for a hidden surface
        commit();
}

void QWaylandWindow::attach(QWaylandBuffer *buffer, int x, int y)
{
    QReadLocker locker(&mSurfaceLock);
    if (mSurface == nullptr)
        return;

    if (buffer) {
        Q_ASSERT(!buffer->committed());
        handleUpdate();
        buffer->setBusy(true);
        if (mSurface->version() >= WL_SURFACE_OFFSET_SINCE_VERSION) {
            mSurface->offset(x, y);
            mSurface->attach(buffer->buffer(), 0, 0);
        } else {
            mSurface->attach(buffer->buffer(), x, y);
        }
    } else {
        mSurface->attach(nullptr, 0, 0);
    }
}

void QWaylandWindow::attachOffset(QWaylandBuffer *buffer)
{
    attach(buffer, mOffset.x(), mOffset.y());
    mOffset = QPoint();
}

void QWaylandWindow::damage(const QRect &rect)
{
    QReadLocker locker(&mSurfaceLock);
    if (mSurface == nullptr)
        return;

    const qreal s = scale();
    if (mSurface->version() >= 4) {
        const QRect bufferRect =
                QRectF(s * rect.x(), s * rect.y(), s * rect.width(), s * rect.height())
                        .toAlignedRect();
        mSurface->damage_buffer(bufferRect.x(), bufferRect.y(), bufferRect.width(),
                                bufferRect.height());
    } else {
        mSurface->damage(rect.x(), rect.y(), rect.width(), rect.height());
    }
}

void QWaylandWindow::safeCommit(QWaylandBuffer *buffer, const QRegion &damage)
{
    if (isExposed()) {
        commit(buffer, damage);
    } else {
        buffer->setBusy(false);
    }
}

bool QWaylandWindow::allowsIndependentThreadedRendering() const
{
    return !mWaitingToApplyConfigure;
}

void QWaylandWindow::commit(QWaylandBuffer *buffer, const QRegion &damage)
{
    Q_ASSERT(isExposed());
    if (buffer->committed()) {
        mSurface->commit();
        qCDebug(lcWaylandBackingstore) << "Buffer already committed, not attaching.";
        return;
    }

    QReadLocker locker(&mSurfaceLock);
    if (!mSurface)
        return;

    attachOffset(buffer);
    if (mSurface->version() >= 4) {
        const qreal s = scale();
        for (const QRect &rect : damage) {
            const QRect bufferRect =
                    QRectF(s * rect.x(), s * rect.y(), s * rect.width(), s * rect.height())
                            .toAlignedRect();
            mSurface->damage_buffer(bufferRect.x(), bufferRect.y(), bufferRect.width(),
                                    bufferRect.height());
        }
    } else {
        for (const QRect &rect: damage)
            mSurface->damage(rect.x(), rect.y(), rect.width(), rect.height());
    }
    Q_ASSERT(!buffer->committed());
    buffer->setCommitted();
    mSurface->commit();
}

void QWaylandWindow::commit()
{
    QReadLocker locker(&mSurfaceLock);
    if (mSurface != nullptr)
        mSurface->commit();
}

const wl_callback_listener QWaylandWindow::callbackListener = {
    [](void *data, wl_callback *callback, uint32_t time) {
        Q_UNUSED(time);
        auto *window = static_cast<QWaylandWindow*>(data);
        window->handleFrameCallback(callback);
    }
};

void QWaylandWindow::handleFrameCallback(wl_callback* callback)
{
    QMutexLocker locker(&mFrameSyncMutex);
    if (!mFrameCallback) {
        // This means the callback is already unset by QWaylandWindow::reset.
        // The wl_callback object will be destroyed there too.
        return;
    }
    Q_ASSERT(callback == mFrameCallback);
    wl_callback_destroy(callback);
    mFrameCallback = nullptr;

    mWaitingForFrameCallback = false;
    mFrameCallbackElapsedTimer.invalidate();

    // The rest can wait until we can run it on the correct thread
    if (mWaitingForUpdateDelivery.testAndSetAcquire(false, true)) {
        // Queued connection, to make sure we don't call handleUpdate() from inside waitForFrameSync()
        // in the single-threaded case.
        QMetaObject::invokeMethod(this, &QWaylandWindow::doHandleFrameCallback, Qt::QueuedConnection);
    }
    mFrameSyncWait.notify_all();
}

void QWaylandWindow::doHandleFrameCallback()
{
    mWaitingForUpdateDelivery.storeRelease(false);
    bool wasExposed = isExposed();
    mFrameCallbackTimedOut = false;
    // Did setting mFrameCallbackTimedOut make the window exposed?
    updateExposure();
    if (wasExposed && hasPendingUpdateRequest())
        deliverUpdateRequest();

}

bool QWaylandWindow::waitForFrameSync(int timeout)
{
    QMutexLocker locker(&mFrameSyncMutex);

    QDeadlineTimer deadline(timeout);
    while (mWaitingForFrameCallback && mFrameSyncWait.wait(&mFrameSyncMutex, deadline)) { }

    if (mWaitingForFrameCallback) {
        qCDebug(lcWaylandBackingstore) << "Didn't receive frame callback in time, window should now be inexposed";
        mFrameCallbackTimedOut = true;
        mWaitingForUpdate = false;
        QMetaObject::invokeMethod(this, &QWaylandWindow::updateExposure, Qt::QueuedConnection);
    }

    return !mWaitingForFrameCallback;
}

QMargins QWaylandWindow::frameMargins() const
{
    if (mWindowDecorationEnabled)
        return mWindowDecoration->margins();
    else if (mShellSurface)
        return mShellSurface->serverSideFrameMargins();
    else
        return QPlatformWindow::frameMargins();
}

QMargins QWaylandWindow::clientSideMargins() const
{
    return mWindowDecorationEnabled ? mWindowDecoration->margins() : QMargins{};
}

void QWaylandWindow::setCustomMargins(const QMargins &margins) {
    const QMargins oldMargins = mCustomMargins;
    mCustomMargins = margins;
    propagateSizeHints();
    setGeometry(geometry().marginsRemoved(oldMargins).marginsAdded(margins));
}

/*!
 * Size, with decorations (including including eventual shadows) in wl_surface coordinates
 */
QSize QWaylandWindow::surfaceSize() const
{
    return geometry().marginsAdded(clientSideMargins()).size();
}

QMargins QWaylandWindow::windowContentMargins() const
{
    QMargins shadowMargins;

    if (mWindowDecorationEnabled)
        shadowMargins = mWindowDecoration->margins(QWaylandAbstractDecoration::ShadowsOnly);

    if (!mCustomMargins.isNull())
        shadowMargins += mCustomMargins;

    return shadowMargins;
}

/*!
 * Window geometry as defined by the xdg-shell spec (in wl_surface coordinates)
 * topLeft is where the shadow stops and the decorations border start.
 */
QRect QWaylandWindow::windowContentGeometry() const
{
    const QMargins margins = windowContentMargins();
    return QRect(QPoint(margins.left(), margins.top()), surfaceSize().shrunkBy(margins));
}

/*!
 * Converts from wl_surface coordinates to Qt window coordinates. Qt window
 * coordinates start inside (not including) the window decorations, while
 * wl_surface coordinates start at the first pixel of the buffer. Potentially,
 * this should be in the window shadow, although we don't have those. So for
 * now, it's the first pixel of the decorations.
 */
QPointF QWaylandWindow::mapFromWlSurface(const QPointF &surfacePosition) const
{
    const QMargins margins = clientSideMargins();
    return QPointF(surfacePosition.x() - margins.left(), surfacePosition.y() - margins.top());
}

wl_surface *QWaylandWindow::wlSurface() const
{
    QReadLocker locker(&mSurfaceLock);
    return mSurface ? mSurface->object() : nullptr;
}

QWaylandShellSurface *QWaylandWindow::shellSurface() const
{
    return mShellSurface;
}

std::any QWaylandWindow::_surfaceRole() const
{
    if (mSubSurfaceWindow)
        return mSubSurfaceWindow->object();
    if (mShellSurface)
        return mShellSurface->surfaceRole();
    return {};
}

QWaylandSubSurface *QWaylandWindow::subSurfaceWindow() const
{
    return mSubSurfaceWindow;
}

QWaylandScreen *QWaylandWindow::waylandScreen() const
{
    auto *platformScreen = QPlatformWindow::screen();
    Q_ASSERT(platformScreen);
    if (platformScreen->isPlaceholder())
        return nullptr;
    return static_cast<QWaylandScreen *>(platformScreen);
}

void QWaylandWindow::handleContentOrientationChange(Qt::ScreenOrientation orientation)
{
    mLastReportedContentOrientation = orientation;
    updateBufferTransform();
}

void QWaylandWindow::updateBufferTransform()
{
    QReadLocker locker(&mSurfaceLock);
    if (mSurface == nullptr || mSurface->version() < 2)
        return;

    wl_output_transform transform;
    Qt::ScreenOrientation screenOrientation = Qt::PrimaryOrientation;

    if (mSurface->version() >= 6) {
        const auto transform = mSurface->preferredBufferTransform().value_or(WL_OUTPUT_TRANSFORM_NORMAL);
        if (auto screen = waylandScreen())
            screenOrientation = screen->toScreenOrientation(transform, Qt::PrimaryOrientation);
    } else {
        if (auto screen = window()->screen())
            screenOrientation = screen->primaryOrientation();
    }

    const bool isPortrait = (screenOrientation == Qt::PortraitOrientation);

    switch (mLastReportedContentOrientation) {
    case Qt::PrimaryOrientation:
        transform = WL_OUTPUT_TRANSFORM_NORMAL;
        break;
    case Qt::LandscapeOrientation:
        transform = isPortrait ? WL_OUTPUT_TRANSFORM_270 : WL_OUTPUT_TRANSFORM_NORMAL;
        break;
    case Qt::PortraitOrientation:
        transform = isPortrait ? WL_OUTPUT_TRANSFORM_NORMAL : WL_OUTPUT_TRANSFORM_90;
        break;
    case Qt::InvertedLandscapeOrientation:
        transform = isPortrait ? WL_OUTPUT_TRANSFORM_90 : WL_OUTPUT_TRANSFORM_180;
        break;
    case Qt::InvertedPortraitOrientation:
        transform = isPortrait ? WL_OUTPUT_TRANSFORM_180 : WL_OUTPUT_TRANSFORM_270;
        break;
    default:
        Q_UNREACHABLE();
    }
    mSurface->set_buffer_transform(transform);
}

void QWaylandWindow::setOrientationMask(Qt::ScreenOrientations mask)
{
    if (mShellSurface)
        mShellSurface->setContentOrientationMask(mask);
}

void QWaylandWindow::setWindowState(Qt::WindowStates states)
{
    if (mShellSurface)
        mShellSurface->requestWindowStates(states);
}

void QWaylandWindow::setWindowFlags(Qt::WindowFlags flags)
{
    const bool wasPopup = mFlags.testFlag(Qt::Popup);
    const bool isPopup = flags.testFlag(Qt::Popup);

    mFlags = flags;
    // changing role is not allowed on XdgShell on the same wl_surface
    if (wasPopup != isPopup) {
        reset();
        initializeWlSurface();
        if (window()->isVisible()) {
            initWindow();
        }
    } else {
        if (mShellSurface)
            mShellSurface->setWindowFlags(flags);
    }

    createDecoration();

    QReadLocker locker(&mSurfaceLock);
    updateInputRegion();
}

Qt::WindowFlags QWaylandWindow::windowFlags() const
{
    return mFlags;
}

bool QWaylandWindow::createDecoration()
{
    Q_ASSERT_X(QThread::currentThreadId() == QThreadData::get2(thread())->threadId.loadRelaxed(),
               "QWaylandWindow::createDecoration", "not called from main thread");
    // TODO: client side decorations do not work with Vulkan backend.
    if (window()->surfaceType() == QSurface::VulkanSurface)
        return false;
    if (!mDisplay->supportsWindowDecoration())
        return false;

    static bool decorationPluginFailed = false;
    bool decoration = false;
    switch (window()->type()) {
        case Qt::Window:
        case Qt::Widget:
        case Qt::Dialog:
        case Qt::Tool:
        case Qt::Drawer:
            decoration = true;
            break;
        default:
            break;
    }
    if (mFlags & Qt::FramelessWindowHint)
        decoration = false;
    if (mFlags & Qt::BypassWindowManagerHint)
        decoration = false;
    if (mSubSurfaceWindow)
        decoration = false;
    if (!mShellSurface || !mShellSurface->wantsDecorations())
        decoration = false;

    bool hadDecoration = mWindowDecorationEnabled;
    if (decoration && !decorationPluginFailed) {
        if (!mWindowDecorationEnabled) {
            if (mWindowDecoration) {
                mWindowDecoration.reset();
            }

            QStringList decorations = QWaylandDecorationFactory::keys();
            if (decorations.empty()) {
                qWarning() << "No decoration plugins available. Running with no decorations.";
                decorationPluginFailed = true;
                return false;
            }

            QString targetKey;
            QByteArray decorationPluginName = qgetenv("QT_WAYLAND_DECORATION");
            if (!decorationPluginName.isEmpty()) {
                targetKey = QString::fromLocal8Bit(decorationPluginName);
                if (!decorations.contains(targetKey)) {
                    qWarning() << "Requested decoration " << targetKey << " not found, falling back to default";
                    targetKey = QString(); // fallthrough
                }
            }

            if (targetKey.isEmpty()) {
                auto unixServices = dynamic_cast<QDesktopUnixServices *>(
                    QGuiApplicationPrivate::platformIntegration()->services());
                const QList<QByteArray> desktopNames = unixServices->desktopEnvironment().split(':');
                if (desktopNames.contains("GNOME")) {
                    if (decorations.contains("adwaita"_L1))
                        targetKey = "adwaita"_L1;
                    else if (decorations.contains("gnome"_L1))
                        targetKey = "gnome"_L1;
                } else {
                    // Do not use Adwaita/GNOME decorations on other DEs
                    decorations.removeAll("adwaita"_L1);
                    decorations.removeAll("gnome"_L1);
                }
            }

            if (targetKey.isEmpty())
                targetKey = decorations.first(); // first come, first served.

            mWindowDecoration.reset(QWaylandDecorationFactory::create(targetKey, QStringList()));
            if (!mWindowDecoration) {
                qWarning() << "Could not create decoration from factory! Running with no decorations.";
                decorationPluginFailed = true;
                return false;
            }
            mWindowDecoration->setWaylandWindow(this);
            mWindowDecorationEnabled = true;
        }
    } else {
        mWindowDecorationEnabled = false;
    }

    if (hadDecoration != mWindowDecorationEnabled) {
        for (QWaylandSubSurface *subsurf : std::as_const(mChildren)) {
            QPoint pos = subsurf->window()->geometry().topLeft();
            QMargins m = frameMargins();
            subsurf->set_position(pos.x() + m.left(), pos.y() + m.top());
        }
        setGeometry(geometry());

        // creating a decoration changes our margins which in turn change size hints
        propagateSizeHints();

        // This is a special case where the buffer is recreated, but since
        // the content rect remains the same, the widgets remain the same
        // size and are not redrawn, leaving the new buffer empty. As a simple
        // work-around, we trigger a full extra update whenever the client-side
        // window decorations are toggled while the window is showing.
        window()->requestUpdate();
    }

    return mWindowDecoration.get();
}

QWaylandAbstractDecoration *QWaylandWindow::decoration() const
{
    return mWindowDecorationEnabled ? mWindowDecoration.get() : nullptr;
}

static QWaylandWindow *closestShellSurfaceWindow(QWindow *window)
{
    while (window) {
        auto w = static_cast<QWaylandWindow *>(window->handle());
        if (w && w->shellSurface())
            return w;
        window = window->transientParent() ? window->transientParent() : window->parent();
    }
    return nullptr;
}

QWaylandWindow *QWaylandWindow::transientParent() const
{
    return mTransientParent;
}

QWaylandWindow *QWaylandWindow::guessTransientParent() const
{
    // Take the closest window with a shell surface, since the transient parent may be a
    // QWidgetWindow or some other window without a shell surface, which is then not able to
    // get mouse events.
    if (auto transientParent = closestShellSurfaceWindow(window()->transientParent()))
        return transientParent;

    if (window()->type() == Qt::Popup) {
        if (mTopPopup)
            return mTopPopup;
    }

    if (window()->type() == Qt::ToolTip || window()->type() == Qt::Popup) {
        if (auto lastInputWindow = display()->lastInputWindow())
            return closestShellSurfaceWindow(lastInputWindow->window());
    }

    return nullptr;
}

void QWaylandWindow::handleMouse(QWaylandInputDevice *inputDevice, const QWaylandPointerEvent &e)
{
    // There's currently no way to get info about the actual hardware device in use.
    // At least we get the correct seat.
    const QPointingDevice *device = QPointingDevice::primaryPointingDevice(inputDevice->seatname());
    if (e.type == QEvent::Leave) {
        if (mWindowDecorationEnabled) {
            if (mMouseEventsInContentArea)
                QWindowSystemInterface::handleLeaveEvent(window());
        } else {
            QWindowSystemInterface::handleLeaveEvent(window());
        }
#if QT_CONFIG(cursor)
        restoreMouseCursor(inputDevice);
#endif
        return;
    }

#if QT_CONFIG(cursor)
    if (e.type == QEvent::Enter) {
        restoreMouseCursor(inputDevice);
    }
#endif

    if (mWindowDecorationEnabled) {
        handleMouseEventWithDecoration(inputDevice, e);
    } else {
        switch (e.type) {
            case QEvent::Enter:
                QWindowSystemInterface::handleEnterEvent(window(), e.local, e.global);
#if QT_CONFIG(cursor)
                mDisplay->waylandCursor()->setPosFromEnterEvent(e.global.toPoint());
#endif
                break;
            case QEvent::MouseButtonPress:
            case QEvent::MouseButtonRelease:
            case QEvent::MouseMove:
                QWindowSystemInterface::handleMouseEvent(window(), e.timestamp, device, e.local, e.global, e.buttons, e.button, e.type, e.modifiers);
                break;
            case QEvent::Wheel:
                QWindowSystemInterface::handleWheelEvent(window(), e.timestamp, device, e.local, e.global,
                                                         e.pixelDelta, e.angleDelta, e.modifiers,
                                                         e.phase, e.source, e.inverted);
                break;
        default:
            Q_UNREACHABLE();
        }
    }
}

#ifndef QT_NO_GESTURES
void QWaylandWindow::handleSwipeGesture(QWaylandInputDevice *inputDevice,
                                        const QWaylandPointerGestureSwipeEvent &e)
{
    switch (e.state) {
        case Qt::GestureStarted:
            if (mGestureState != GestureNotActive)
                qCWarning(lcQpaWaylandInput) << "Unexpected GestureStarted while already active";

            if (mWindowDecorationEnabled && !mMouseEventsInContentArea) {
                // whole gesture sequence will be ignored
                mGestureState = GestureActiveInDecoration;
                return;
            }

            mGestureState = GestureActiveInContentArea;
            QWindowSystemInterface::handleGestureEvent(window(), e.timestamp,
                                                       inputDevice->mTouchPadDevice,
                                                       Qt::BeginNativeGesture,
                                                       e.local, e.global, e.fingers);
            break;
        case Qt::GestureUpdated:
            if (mGestureState != GestureActiveInContentArea)
                return;

            if (!e.delta.isNull()) {
                QWindowSystemInterface::handleGestureEventWithValueAndDelta(
                            window(), e.timestamp, inputDevice->mTouchPadDevice,
                            Qt::PanNativeGesture,
                            0, e.delta, e.local, e.global, e.fingers);
            }
            break;
        case Qt::GestureFinished:
        case Qt::GestureCanceled:
            if (mGestureState == GestureActiveInDecoration) {
                mGestureState = GestureNotActive;
                return;
            }

            if (mGestureState != GestureActiveInContentArea)
                qCWarning(lcQpaWaylandInput) << "Unexpected" << (e.state == Qt::GestureFinished ? "GestureFinished" : "GestureCanceled");

            mGestureState = GestureNotActive;

            // There's currently no way to expose cancelled gestures to the rest of Qt, so
            // this part of information is lost.
            QWindowSystemInterface::handleGestureEvent(window(), e.timestamp,
                                                       inputDevice->mTouchPadDevice,
                                                       Qt::EndNativeGesture,
                                                       e.local, e.global, e.fingers);
            break;
        default:
            break;
    }
}

void QWaylandWindow::handlePinchGesture(QWaylandInputDevice *inputDevice,
                                        const QWaylandPointerGesturePinchEvent &e)
{
    switch (e.state) {
        case Qt::GestureStarted:
            if (mGestureState != GestureNotActive)
                qCWarning(lcQpaWaylandInput) << "Unexpected GestureStarted while already active";

            if (mWindowDecorationEnabled && !mMouseEventsInContentArea) {
                // whole gesture sequence will be ignored
                mGestureState = GestureActiveInDecoration;
                return;
            }

            mGestureState = GestureActiveInContentArea;
            QWindowSystemInterface::handleGestureEvent(window(), e.timestamp,
                                                       inputDevice->mTouchPadDevice,
                                                       Qt::BeginNativeGesture,
                                                       e.local, e.global, e.fingers);
            break;
        case Qt::GestureUpdated:
            if (mGestureState != GestureActiveInContentArea)
                return;

            if (!e.delta.isNull()) {
                QWindowSystemInterface::handleGestureEventWithValueAndDelta(
                            window(), e.timestamp, inputDevice->mTouchPadDevice,
                            Qt::PanNativeGesture,
                            0, e.delta, e.local, e.global, e.fingers);
            }
            if (e.rotation_delta != 0) {
                QWindowSystemInterface::handleGestureEventWithRealValue(window(), e.timestamp,
                                                                        inputDevice->mTouchPadDevice,
                                                                        Qt::RotateNativeGesture,
                                                                        e.rotation_delta,
                                                                        e.local, e.global, e.fingers);
            }
            if (e.scale_delta != 0) {
                QWindowSystemInterface::handleGestureEventWithRealValue(window(), e.timestamp,
                                                                        inputDevice->mTouchPadDevice,
                                                                        Qt::ZoomNativeGesture,
                                                                        e.scale_delta,
                                                                        e.local, e.global, e.fingers);
            }
            break;
        case Qt::GestureFinished:
        case Qt::GestureCanceled:
            if (mGestureState == GestureActiveInDecoration) {
                mGestureState = GestureNotActive;
                return;
            }

            if (mGestureState != GestureActiveInContentArea)
                qCWarning(lcQpaWaylandInput) << "Unexpected" << (e.state == Qt::GestureFinished ? "GestureFinished" : "GestureCanceled");

            mGestureState = GestureNotActive;

            // There's currently no way to expose cancelled gestures to the rest of Qt, so
            // this part of information is lost.
            QWindowSystemInterface::handleGestureEvent(window(), e.timestamp,
                                                       inputDevice->mTouchPadDevice,
                                                       Qt::EndNativeGesture,
                                                       e.local, e.global, e.fingers);
            break;
        default:
            break;
    }
}
#endif // #ifndef QT_NO_GESTURES


bool QWaylandWindow::touchDragDecoration(QWaylandInputDevice *inputDevice, const QPointF &local, const QPointF &global, QEventPoint::State state, Qt::KeyboardModifiers mods)
{
    if (!mWindowDecorationEnabled)
        return false;
    return mWindowDecoration->handleTouch(inputDevice, local, global, state, mods);
}

bool QWaylandWindow::handleTabletEventDecoration(QWaylandInputDevice *inputDevice,
                                                 const QPointF &local, const QPointF &global,
                                                 Qt::MouseButtons buttons,
                                                 Qt::KeyboardModifiers modifiers)
{
    if (!mWindowDecorationEnabled)
        return false;
    return mWindowDecoration->handleMouse(inputDevice, local, global, buttons, modifiers);
}

void QWaylandWindow::handleMouseEventWithDecoration(QWaylandInputDevice *inputDevice, const QWaylandPointerEvent &e)
{
    // There's currently no way to get info about the actual hardware device in use.
    // At least we get the correct seat.
    const QPointingDevice *device = QPointingDevice::primaryPointingDevice(inputDevice->seatname());
    if (mMousePressedInContentArea == Qt::NoButton &&
        mWindowDecoration->handleMouse(inputDevice, e.local, e.global, e.buttons, e.modifiers)) {
        if (mMouseEventsInContentArea) {
            QWindowSystemInterface::handleLeaveEvent(window());
            mMouseEventsInContentArea = false;
        }
        return;
    }

    QMargins marg = clientSideMargins();
    QRect windowRect(0 + marg.left(),
                     0 + marg.top(),
                     geometry().size().width(),
                     geometry().size().height());
    if (windowRect.contains(e.local.toPoint()) || mMousePressedInContentArea != Qt::NoButton) {
        const QPointF localTranslated = mapFromWlSurface(e.local);
        QPointF globalTranslated = e.global;
        globalTranslated.setX(globalTranslated.x() - marg.left());
        globalTranslated.setY(globalTranslated.y() - marg.top());
        if (!mMouseEventsInContentArea) {
#if QT_CONFIG(cursor)
            restoreMouseCursor(inputDevice);
#endif
            QWindowSystemInterface::handleEnterEvent(window());
        }

        switch (e.type) {
            case QEvent::Enter:
                QWindowSystemInterface::handleEnterEvent(window(), localTranslated, globalTranslated);
#if QT_CONFIG(cursor)
                mDisplay->waylandCursor()->setPosFromEnterEvent(e.global.toPoint());
#endif
                break;
            case QEvent::MouseButtonPress:
            case QEvent::MouseButtonRelease:
            case QEvent::MouseMove:
                QWindowSystemInterface::handleMouseEvent(window(), e.timestamp, device, localTranslated, globalTranslated, e.buttons, e.button, e.type, e.modifiers);
                break;
            case QEvent::Wheel: {
                QWindowSystemInterface::handleWheelEvent(window(), e.timestamp, device,
                                                         localTranslated, globalTranslated,
                                                         e.pixelDelta, e.angleDelta, e.modifiers,
                                                         e.phase, e.source, e.inverted);
                break;
            }
            default:
                Q_UNREACHABLE();
        }

        mMouseEventsInContentArea = true;
        mMousePressedInContentArea = e.buttons;
    } else {
        if (mMouseEventsInContentArea) {
            QWindowSystemInterface::handleLeaveEvent(window());
            mMouseEventsInContentArea = false;
        }
    }
}

void QWaylandWindow::handleScreensChanged()
{
    QPlatformScreen *newScreen = calculateScreenFromSurfaceEvents();

    if (!newScreen || newScreen->screen() == window()->screen())
        return;

    QWindowSystemInterface::handleWindowScreenChanged<QWindowSystemInterface::SynchronousDelivery>(window(), newScreen->QPlatformScreen::screen());

    if (fixedToplevelPositions && !QPlatformWindow::parent() && window()->type() != Qt::Popup
        && window()->type() != Qt::ToolTip && window()->type() != Qt::Tool
        && geometry().topLeft() != newScreen->geometry().topLeft()) {
        auto geometry = this->geometry();
        geometry.moveTo(newScreen->geometry().topLeft());
        setGeometry(geometry);
    }

    updateScale();
    updateBufferTransform();
}

void QWaylandWindow::updateScale()
{
    if (mFractionalScale) {
        qreal preferredScale = mFractionalScale->preferredScale().value_or(1.0);
        preferredScale = std::max<qreal>(1.0, preferredScale);
        Q_ASSERT(mViewport);
        setScale(preferredScale);
        return;
    }

    if (mSurface && mSurface->version() >= 6) {
        auto preferredScale = mSurface->preferredBufferScale().value_or(1);
        preferredScale = std::max(1, preferredScale);
        setScale(preferredScale);
        return;
    }

    int scale = screen()->isPlaceholder() ? 1 : static_cast<QWaylandScreen *>(screen())->scale();
    setScale(scale);
}

void QWaylandWindow::setScale(qreal newScale)
{
    if (mScale.has_value() && qFuzzyCompare(mScale.value(), newScale))
        return;
    mScale = newScale;

    if (mSurface) {
        if (mViewport)
            updateViewport();
        else if (mSurface->version() >= 3)
            mSurface->set_buffer_scale(std::ceil(newScale));
    }
    ensureSize();

    QWindowSystemInterface::handleWindowDevicePixelRatioChanged<QWindowSystemInterface::SynchronousDelivery>(window());
    if (isExposed()) {
        // redraw at the new DPR
        window()->requestUpdate();
        sendExposeEvent(QRect(QPoint(), geometry().size()));
    }
}

#if QT_CONFIG(cursor)
void QWaylandWindow::restoreMouseCursor(QWaylandInputDevice *device)
{
    if (const QCursor *overrideCursor = QGuiApplication::overrideCursor()) {
        applyCursor(device, *overrideCursor);
    } else if (mHasStoredCursor) {
        applyCursor(device, mStoredCursor);
    } else {
        applyCursor(device, QCursor(Qt::ArrowCursor));
    }
}

void QWaylandWindow::resetStoredCursor()
{
    mHasStoredCursor = false;
}

// Keep track of application set cursors on each window
void QWaylandWindow::setStoredCursor(const QCursor &cursor) {
    mStoredCursor = cursor;
    mHasStoredCursor = true;
}

void QWaylandWindow::applyCursor(QWaylandInputDevice *device, const QCursor &cursor) {
    if (!device || !device->pointer() || device->pointer()->focusWindow() != this)
        return;

    int fallbackBufferScale = qCeil(devicePixelRatio());
    device->setCursor(&cursor, {}, fallbackBufferScale);
}
#endif

void QWaylandWindow::requestActivateWindow()
{
    if (mShellSurface)
        mShellSurface->requestActivate();
}

bool QWaylandWindow::calculateExposure() const
{
    if (!window()->isVisible())
        return false;

    if (mFrameCallbackTimedOut)
        return false;

    if (mShellSurface)
        return mShellSurface->isExposed();

    if (mSubSurfaceWindow)
        return mSubSurfaceWindow->parent()->isExposed();

    return !(shouldCreateShellSurface() || shouldCreateSubSurface());
}

void QWaylandWindow::updateExposure()
{
    bool exposed = calculateExposure();
    if (exposed == mExposed)
        return;

    mExposed = exposed;

    if (!exposed)
        sendExposeEvent(QRect());
    else
        sendExposeEvent(QRect(QPoint(), geometry().size()));

    for (QWaylandSubSurface *subSurface : std::as_const(mChildren)) {
        auto subWindow = subSurface->window();
        subWindow->updateExposure();
    }
}

bool QWaylandWindow::isExposed() const
{
    return mExposed;
}

bool QWaylandWindow::isActive() const
{
    return mDisplay->isWindowActivated(this);
}

qreal QWaylandWindow::scale() const
{
    return devicePixelRatio();
}

qreal QWaylandWindow::devicePixelRatio() const
{
    return mScale.value_or(waylandScreen() ? waylandScreen()->scale() : 1);
}

bool QWaylandWindow::setMouseGrabEnabled(bool grab)
{
    if (window()->type() != Qt::Popup) {
        qWarning("This plugin supports grabbing the mouse only for popup windows");
        return false;
    }

    mMouseGrab = grab ? this : nullptr;
    return true;
}

QWaylandWindow::ToplevelWindowTilingStates QWaylandWindow::toplevelWindowTilingStates() const
{
    return mLastReportedToplevelWindowTilingStates;
}

void QWaylandWindow::handleToplevelWindowTilingStatesChanged(ToplevelWindowTilingStates states)
{
    mLastReportedToplevelWindowTilingStates = states;
}

Qt::WindowStates QWaylandWindow::windowStates() const
{
    return mLastReportedWindowStates;
}

void QWaylandWindow::handleWindowStatesChanged(Qt::WindowStates states)
{
    createDecoration();
    Qt::WindowStates statesWithoutActive = states & ~Qt::WindowActive;
    Qt::WindowStates lastStatesWithoutActive = mLastReportedWindowStates & ~Qt::WindowActive;
    QWindowSystemInterface::handleWindowStateChanged(window(), statesWithoutActive,
                                                     lastStatesWithoutActive);
    mLastReportedWindowStates = states;
}

void QWaylandWindow::sendProperty(const QString &name, const QVariant &value)
{
    m_properties.insert(name, value);
    QWaylandNativeInterface *nativeInterface = static_cast<QWaylandNativeInterface *>(
                QGuiApplication::platformNativeInterface());
    nativeInterface->emitWindowPropertyChanged(this, name);
    if (mShellSurface)
        mShellSurface->sendProperty(name, value);
}

void QWaylandWindow::setProperty(const QString &name, const QVariant &value)
{
    m_properties.insert(name, value);
    QWaylandNativeInterface *nativeInterface = static_cast<QWaylandNativeInterface *>(
                QGuiApplication::platformNativeInterface());
    nativeInterface->emitWindowPropertyChanged(this, name);
}

QVariantMap QWaylandWindow::properties() const
{
    return m_properties;
}

QVariant QWaylandWindow::property(const QString &name)
{
    return m_properties.value(name);
}

QVariant QWaylandWindow::property(const QString &name, const QVariant &defaultValue)
{
    return m_properties.value(name, defaultValue);
}

#ifdef QT_PLATFORM_WINDOW_HAS_VIRTUAL_SET_BACKING_STORE
void QWaylandWindow::setBackingStore(QPlatformBackingStore *store)
{
    mBackingStore = dynamic_cast<QWaylandShmBackingStore *>(store);
}
#endif

void QWaylandWindow::timerEvent(QTimerEvent *event)
{
    if (event->timerId() != mFrameCallbackCheckIntervalTimerId)
        return;

    {
        QMutexLocker lock(&mFrameSyncMutex);

        const bool callbackTimerValid = mFrameCallbackElapsedTimer.isValid();
        const bool callbackTimerExpired = callbackTimerValid && mFrameCallbackElapsedTimer.hasExpired(mFrameCallbackTimeout);
        if (!callbackTimerValid || callbackTimerExpired) {
            killTimer(mFrameCallbackCheckIntervalTimerId);
            mFrameCallbackCheckIntervalTimerId = -1;
        }
        if (!callbackTimerValid || !callbackTimerExpired) {
            return;
        }
        mFrameCallbackElapsedTimer.invalidate();
    }

    qCDebug(lcWaylandBackingstore) << "Didn't receive frame callback in time, window should now be inexposed";
    mFrameCallbackTimedOut = true;
    mWaitingForUpdate = false;
    updateExposure();
}

void QWaylandWindow::requestUpdate()
{
    qCDebug(lcWaylandBackingstore) << "requestUpdate";
    Q_ASSERT(hasPendingUpdateRequest()); // should be set by QPA

    // If we have a frame callback all is good and will be taken care of there
    {
        QMutexLocker locker(&mFrameSyncMutex);
        if (mWaitingForFrameCallback)
            return;
    }

    // If we've already called deliverUpdateRequest(), but haven't seen any attach+commit/swap yet
    // This is a somewhat redundant behavior and might indicate a bug in the calling code, so log
    // here so we can get this information when debugging update/frame callback issues.
    // Continue as nothing happened, though.
    if (mWaitingForUpdate)
        qCDebug(lcWaylandBackingstore) << "requestUpdate called twice without committing anything";

    // Some applications (such as Qt Quick) depend on updates being delivered asynchronously,
    // so use invokeMethod to delay the delivery a bit.
    QMetaObject::invokeMethod(this, [this] {
        // Things might have changed in the meantime
        {
            QMutexLocker locker(&mFrameSyncMutex);
            if (mWaitingForFrameCallback)
                return;
        }
        if (hasPendingUpdateRequest())
            deliverUpdateRequest();
    }, Qt::QueuedConnection);
}

// Should be called whenever we commit a buffer (directly through wl_surface.commit or indirectly
// with eglSwapBuffers) to know when it's time to commit the next one.
// Can be called from the render thread (without locking anything) so make sure to not make races in this method.
void QWaylandWindow::handleUpdate()
{
    mExposeEventNeedsAttachedBuffer = false;
    qCDebug(lcWaylandBackingstore) << "handleUpdate" << QThread::currentThread();

    // TODO: Should sync subsurfaces avoid requesting frame callbacks?
    QReadLocker lock(&mSurfaceLock);
    if (!mSurface)
        return;

    QMutexLocker locker(&mFrameSyncMutex);
    if (mWaitingForFrameCallback)
        return;

    struct ::wl_surface *wrappedSurface = reinterpret_cast<struct ::wl_surface *>(wl_proxy_create_wrapper(mSurface->object()));
    wl_proxy_set_queue(reinterpret_cast<wl_proxy *>(wrappedSurface), mDisplay->frameEventQueue());
    mFrameCallback = wl_surface_frame(wrappedSurface);
    wl_proxy_wrapper_destroy(wrappedSurface);
    wl_callback_add_listener(mFrameCallback, &QWaylandWindow::callbackListener, this);
    mWaitingForFrameCallback = true;
    mWaitingForUpdate = false;

    // Start a timer for handling the case when the compositor stops sending frame callbacks.
    if (mFrameCallbackTimeout > 0) {
        QMetaObject::invokeMethod(this, [this] {
            QMutexLocker locker(&mFrameSyncMutex);

            if (mWaitingForFrameCallback) {
                if (mFrameCallbackCheckIntervalTimerId < 0)
                    mFrameCallbackCheckIntervalTimerId = startTimer(mFrameCallbackTimeout);
                mFrameCallbackElapsedTimer.start();
            }
        }, Qt::QueuedConnection);
    }
}

void QWaylandWindow::deliverUpdateRequest()
{
    qCDebug(lcWaylandBackingstore) << "deliverUpdateRequest";
    mWaitingForUpdate = true;
    QPlatformWindow::deliverUpdateRequest();
}

void QWaylandWindow::addAttachOffset(const QPoint point)
{
    mOffset += point;
}

void QWaylandWindow::propagateSizeHints()
{
    if (mShellSurface)
        mShellSurface->propagateSizeHints();
}

bool QWaylandWindow::startSystemResize(Qt::Edges edges)
{
    if (auto *seat = display()->lastInputDevice()) {
        bool rc = mShellSurface && mShellSurface->resize(seat, edges);
        seat->handleEndDrag();
        return rc;
    }
    return false;
}

bool QtWaylandClient::QWaylandWindow::startSystemMove()
{
    if (auto seat = display()->lastInputDevice()) {
        bool rc = mShellSurface && mShellSurface->move(seat);
        seat->handleEndDrag();
        return rc;
    }
    return false;
}

bool QWaylandWindow::isOpaque() const
{
    return window()->requestedFormat().alphaBufferSize() <= 0;
}

void QWaylandWindow::setOpaqueArea(const QRegion &opaqueArea)
{
    const QRegion translatedOpaqueArea = opaqueArea.translated(clientSideMargins().left(), clientSideMargins().top());

    if (translatedOpaqueArea == mOpaqueArea || !mSurface)
        return;

    mOpaqueArea = translatedOpaqueArea;

    struct ::wl_region *region = mDisplay->createRegion(translatedOpaqueArea);
    mSurface->set_opaque_region(region);
    wl_region_destroy(region);
}

void QWaylandWindow::requestXdgActivationToken(uint serial)
{
    if (!mShellSurface) {
        qCWarning(lcQpaWayland) << "requestXdgActivationToken is called with no surface role created, emitting synthetic signal";
        Q_EMIT xdgActivationTokenCreated({});
        return;
    }
    mShellSurface->requestXdgActivationToken(serial);
}

void QWaylandWindow::setXdgActivationToken(const QString &token)
{
    if (mShellSurface)
        mShellSurface->setXdgActivationToken(token);
    else
        qCWarning(lcQpaWayland) << "setXdgActivationToken is called with no surface role created, token" << token << "discarded";
}

void QWaylandWindow::addChildPopup(QWaylandWindow *child)
{
    if (mShellSurface)
        mShellSurface->attachPopup(child->shellSurface());
    mChildPopups.append(child);
}

void QWaylandWindow::removeChildPopup(QWaylandWindow *child)
{
    if (mShellSurface)
        mShellSurface->detachPopup(child->shellSurface());
    mChildPopups.removeAll(child);
}

void QWaylandWindow::closeChildPopups() {
    while (!mChildPopups.isEmpty()) {
        auto popup = mChildPopups.takeLast();
        popup->resetSurfaceRole();
    }
}

void QWaylandWindow::reinit()
{
    if (window()->isVisible()) {
        initWindow();
        if (hasPendingUpdateRequest())
            deliverUpdateRequest();
    }
}

bool QWaylandWindow::windowEvent(QEvent *event)
{
    if (event->type() == QEvent::ApplicationPaletteChange
        || event->type() == QEvent::ApplicationFontChange) {
        if (mWindowDecorationEnabled && window()->isVisible())
            mWindowDecoration->update();
    } else if (event->type() == QEvent::WindowUnblocked) {
        // QtGui sends leave event to window under cursor when modal window opens, so we have
        // to send enter event when modal closes and window has cursor and gets unblocked.
        if (auto *inputDevice = mDisplay->lastInputDevice(); inputDevice && inputDevice->pointerFocus() == this) {
            const auto pos = mDisplay->waylandCursor()->pos();
            QWindowSystemInterface::handleEnterEvent(window(), mapFromGlobalF(pos), pos);
        }
    }

    return QPlatformWindow::windowEvent(event);
}

QSurfaceFormat QWaylandWindow::format() const
{
    return mSurfaceFormat;
}

}

QT_END_NAMESPACE

#include "moc_qwaylandwindow_p.cpp"
