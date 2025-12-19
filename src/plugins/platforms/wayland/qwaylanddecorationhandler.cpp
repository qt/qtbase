// Copyright (C) 2025
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#include "qwaylandabstractdecoration_p.h"
#include "qwaylanddecorationhandler_p.h"

#include "qwaylandviewport_p.h"
#include "qwaylandwindow_p.h"
#include "qwaylandshmbackingstore_p.h"
#include "qwaylanddisplay_p.h"
#include "qwaylandinputdevice_p.h"
#include "qwaylanddecorationfactory_p.h"
#include "qwaylandplatformservices_p.h"

#include <QtGui/QImage>
#include <QtGui/QPainter>
#include <QtCore/QThread>
#include <QtGui/private/qguiapplication_p.h>
#include <QGuiApplication>

QT_BEGIN_NAMESPACE

using namespace Qt::StringLiterals;

namespace QtWaylandClient {

void QWaylandDecorationSurface::SubsurfaceDeleter::operator()(QtWayland::wl_subsurface *subsurface) const
{
    if (!subsurface)
        return;

    subsurface->destroy();
    delete subsurface;
}

QWaylandDecorationHandler::QWaylandDecorationHandler(QWaylandWindow *window)
    : mWindow(window)
{
}

QWaylandDecorationHandler::~QWaylandDecorationHandler()
{
}

void QWaylandDecorationHandler::reset()
{
    mWindowDecorationEnabled = false;
    mWindowDecoration.reset();
    mTopDecorationSurface.reset();
    mLeftDecorationSurface.reset();
    mRightDecorationSurface.reset();
    mBottomDecorationSurface.reset();
    mWindowDecorationEnabled = false;
}

QWaylandAbstractDecoration *QWaylandDecorationHandler::decoration() const
{
    return mWindowDecorationEnabled ? mWindowDecoration.get() : nullptr;
}

void QWaylandDecorationHandler::updateNeedsDecoration()
{
    Q_ASSERT_X(QThread::isMainThread(),
               "QWaylandDecorationHandler::createDecoration", "not called from main thread");
    if (!mWindow->display()->supportsWindowDecoration())
        return;

    static bool decorationPluginFailed = false;
    bool decoration = false;
    switch (mWindow->window()->type()) {
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
    if (mWindow->windowFlags() & Qt::FramelessWindowHint)
        decoration = false;
    if (mWindow->windowFlags() & Qt::BypassWindowManagerHint)
        decoration = false;
    if (mWindow->subSurfaceWindow())
        decoration = false;
    if (!mWindow->shellSurface() || !mWindow->shellSurface()->wantsDecorations())
        decoration = false;

    if (decoration && !decorationPluginFailed) {
        if (!mWindowDecorationEnabled) {
            if (mWindowDecoration) {
                mWindowDecoration.reset();
            }

            QStringList decorations = QWaylandDecorationFactory::keys();
            if (decorations.empty()) {
                qWarning() << "No decoration plugins available. Running with no decorations.";
                decorationPluginFailed = true;
                return;
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
                return;
            }

            mWindowDecoration->setWaylandWindow(mWindow);
            mWindowDecorationEnabled = true;
        }
    } else {
        reset();
    }

    return;
}


void QWaylandDecorationHandler::update()
{
    Q_ASSERT(mWindowDecorationEnabled && mWindowDecoration);
    auto decoration = mWindowDecoration.get();
    if (!decoration->isDirty())
        return;

    auto margins = decoration->margins();
    const QSize imageSize = mWindow->geometry().marginsAdded(margins).size();
    const int top = margins.top();
    const int bottom = margins.bottom();
    const int left = margins.left();
    const int right = margins.right();
    const int sideHeight = mWindow->geometry().height();

    updateTile(mTopDecorationSurface,
               QRect(-left, -top, imageSize.width(), top),
               QRect(0, 0, imageSize.width(), top),
               QWaylandDecorationSurface::Part::Top);
    updateTile(mLeftDecorationSurface,
               QRect(-left, 0, left, sideHeight),
               QRect(0, top, left, sideHeight),
               QWaylandDecorationSurface::Part::Left);
    updateTile(mRightDecorationSurface,
               QRect(mWindow->geometry().width(), 0, right, sideHeight),
               QRect(imageSize.width() - right, top, right, sideHeight),
               QWaylandDecorationSurface::Part::Right);
    updateTile(mBottomDecorationSurface,
               QRect(-left, mWindow->geometry().height(), imageSize.width(), bottom),
               QRect(0, imageSize.height() - bottom, imageSize.width(), bottom),
               QWaylandDecorationSurface::Part::Bottom);

    const auto paintTile = [](const std::unique_ptr<QWaylandDecorationSurface> &surface) {
        return !surface || surface->paint();
    };

    // if any buffer is busy, try again later
    if (!paintTile(mTopDecorationSurface)
        || !paintTile(mLeftDecorationSurface)
        || !paintTile(mRightDecorationSurface)
        || !paintTile(mBottomDecorationSurface)) {
        mWindow->requestForcedUpdate();
        return;
    }

    decoration->resetDirty();
}


void QWaylandDecorationHandler::updateTile(std::unique_ptr<QWaylandDecorationSurface> &surface,
                                           const QRect &windowGeometry,
                                           const QRect &paintGeometry,
                                           QWaylandDecorationSurface::Part part)
{
    if (paintGeometry.isEmpty()) {
        surface.reset();
        return;
    }

    if (!surface)
        surface.reset(new QWaylandDecorationSurface(mWindow, mWindowDecoration.get(), part));

    surface->setGeometry(windowGeometry, paintGeometry);
}

QWaylandDecorationSurface::QWaylandDecorationSurface(QWaylandWindow *parentWindow,
                                                     QWaylandAbstractDecoration *decoration,
                                                     Part part)
    : mParentWindow(parentWindow)
    , mWindowDecoration(decoration)
    , mPart(part)
{
    auto display = parentWindow->display();

    mSurface = std::make_unique<QWaylandSurface>(parentWindow->display());
    mViewport = std::make_unique<QWaylandViewport>(display->createViewport(mSurface.get()));
    mSurface->m_container = this;
    // avoiding the wrapper, as that assumes it's a window, we could split QWaylandSubsurface
    mSubsurface.reset(new QtWayland::wl_subsurface(display->createSubSurface(mSurface.get(),
                                                                             parentWindow->waylandSurface())));
    mSubsurface->place_below(parentWindow->surface());
}

QWaylandDecorationSurface::~QWaylandDecorationSurface()
{
}

void QWaylandDecorationSurface::setGeometry(const QRect &windowGeometry, const QRect &paintGeometry)
{
    createBuffer(paintGeometry.size());
    mWindowGeometry = windowGeometry;
    mPaintGeometry = paintGeometry;
    mSubsurface->set_position(windowGeometry.x(), windowGeometry.y());
    if (mViewport)
        mViewport->setDestination(paintGeometry.size());
}

bool QWaylandDecorationSurface::paint()
{
    auto *buffer = mBackBuffer.get();
    if (!buffer) {
        return true;
    }
    if (buffer->busy())
        return false;

    QPainter painter(buffer->image());
    painter.translate(-mPaintGeometry.topLeft());
    switch (mPart) {
    case Part::Top:
        mWindowDecoration->paintTop(&painter);
        break;
    case Part::Left:
        mWindowDecoration->paintLeft(&painter);
        break;
    case Part::Right:
        mWindowDecoration->paintRight(&painter);
        break;
    case Part::Bottom:
        mWindowDecoration->paintBottom(&painter);
        break;
    }
    painter.end();

    mSurface->attach(mBackBuffer->buffer(), 0, 0);
    mSurface->damage(0, 0, mBackBuffer->size().width(), mBackBuffer->size().height());
    mSurface->commit();
    std::swap(mFrontBuffer, mBackBuffer);
    return true;
}

void QWaylandDecorationSurface::createBuffer(const QSize &size)
{
    QImage::Format format = QImage::Format_ARGB32_Premultiplied;
    qreal scale = 1.0;
    if (mViewport)
        scale = mParentWindow->scale();

    const QSize bufferSize = size * scale;
    if (mFrontBuffer && mFrontBuffer->size() == bufferSize) {
        return;
    }
    mFrontBuffer = std::make_unique<QWaylandShmBuffer>(mParentWindow->display(), bufferSize, format, scale);
    mBackBuffer = std::make_unique<QWaylandShmBuffer>(mParentWindow->display(), bufferSize, format, scale);
}

void QWaylandDecorationSurface::handleMouse(QWaylandInputDevice *inputDevice, const QWaylandPointerEvent &e)
{
    mWindowDecoration->handleMouse(inputDevice,
                                   e.local + mPaintGeometry.topLeft(),
                                   e.global,
                                   e.buttons,
                                   e.modifiers);
}

QPointF QWaylandDecorationSurface::mapToGlobal(const QPointF &pointIn)
{
    return mParentWindow->mapInputToGlobal(pointIn + mWindowGeometry.topLeft());
}



} // namespace QtWaylandClient

QT_END_NAMESPACE
