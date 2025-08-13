// Copyright (C) 2016 Klarälvdalens Datakonsult AB (KDAB).
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only


#include "qwaylanddatadevice_p.h"

#include "qwaylanddatadevicemanager_p.h"
#include "qwaylanddataoffer_p.h"
#include "qwaylanddatasource_p.h"
#include "qwaylanddnd_p.h"
#include "qwaylandinputdevice_p.h"
#include "qwaylanddisplay_p.h"
#include "qwaylandabstractdecoration_p.h"
#include "qwaylandsurface_p.h"

#include <QtWaylandClient/private/qwayland-xdg-toplevel-drag-v1.h>

#include <QtCore/QMimeData>
#include <QtGui/QGuiApplication>
#include <QtGui/private/qguiapplication_p.h>

#if QT_CONFIG(clipboard)
#include <qpa/qplatformclipboard.h>
#endif
#include <qpa/qplatformdrag.h>
#include <qpa/qwindowsysteminterface.h>

QT_BEGIN_NAMESPACE

namespace QtWaylandClient {

using namespace Qt::StringLiterals;

QWaylandDataDevice::QWaylandDataDevice(QWaylandDataDeviceManager *manager, QWaylandInputDevice *inputDevice)
    : QObject(inputDevice)
    , QtWayland::wl_data_device(manager->get_data_device(inputDevice->wl_seat()))
    , m_display(manager->display())
    , m_inputDevice(inputDevice)
{
}

QWaylandDataDevice::~QWaylandDataDevice()
{
    if (version() >= WL_DATA_DEVICE_RELEASE_SINCE_VERSION)
        release();
    else
        wl_data_device_destroy(object());
}

QWaylandDataOffer *QWaylandDataDevice::selectionOffer() const
{
    return m_selectionOffer.data();
}

void QWaylandDataDevice::invalidateSelectionOffer()
{
    if (m_selectionOffer.isNull())
        return;

    m_selectionOffer.reset();

#if QT_CONFIG(clipboard)
    QGuiApplicationPrivate::platformIntegration()->clipboard()->emitChanged(QClipboard::Clipboard);
#endif
}

QWaylandDataSource *QWaylandDataDevice::selectionSource() const
{
    return m_selectionSource.data();
}

void QWaylandDataDevice::setSelectionSource(QWaylandDataSource *source)
{
    if (source)
        connect(source, &QWaylandDataSource::cancelled, this, &QWaylandDataDevice::selectionSourceCancelled);
    set_selection(source ? source->object() : nullptr, m_inputDevice->serial());
    m_selectionSource.reset(source);
}

#if QT_CONFIG(draganddrop)
QWaylandDataOffer *QWaylandDataDevice::dragOffer() const
{
    return m_dragOffer.data();
}

bool QWaylandDataDevice::startDrag(QMimeData *mimeData, Qt::DropActions supportedActions, QWaylandWindow *icon)
{
    auto *origin = m_display->lastInputWindow();

    if (!origin) {
        qCDebug(lcQpaWayland) << "Couldn't start a drag because the origin window could not be found.";
        return false;
    }

    // dragging data without mimetypes is a legal operation in Qt terms
    // but Wayland uses a mimetype to determine if a drag is accepted or not
    // In this rare case, insert a placeholder
    if (mimeData->formats().isEmpty())
        mimeData->setData("application/x-qt-avoid-empty-placeholder"_L1, QByteArray("1"));

    static const QString plain = QStringLiteral("text/plain");
    static const QString utf8 = QStringLiteral("text/plain;charset=utf-8");

    if (mimeData->hasFormat(plain) && !mimeData->hasFormat(utf8))
        mimeData->setData(utf8, mimeData->data(plain));

    m_dragSource.reset(new QWaylandDataSource(m_display->dndSelectionHandler(), mimeData));

    if (version() >= 3)
        m_dragSource->set_actions(dropActionsToWl(supportedActions));

    connect(m_dragSource.data(), &QWaylandDataSource::cancelled, this, &QWaylandDataDevice::dragSourceCancelled);
    connect(m_dragSource.data(), &QWaylandDataSource::dndResponseUpdated, this, [this](bool accepted, Qt::DropAction action) {
            auto drag = static_cast<QWaylandDrag *>(QGuiApplicationPrivate::platformIntegration()->drag());
            if (!drag->currentDrag()) {
                return;
            }
            // in old versions drop action is not set, so we guess
            if (m_dragSource->version() < 3) {
                drag->setResponse(accepted);
            } else {
                QPlatformDropQtResponse response(accepted, action);
                drag->setResponse(response);
            }
    });
    connect(m_dragSource.data(), &QWaylandDataSource::dndDropped, this,
            [this](bool accepted, Qt::DropAction action) {
                QPlatformDropQtResponse response(accepted, action);
                if (m_toplevelDrag) {
                    // If the widget was dropped but the drag not accepted it
                    // should be its own window in the future. To distinguish
                    // from canceling mid-drag the drag is accepted here as the
                    // we know if the widget is over a zone where it can be
                    // incorporated or not
                    response = { true, Qt::MoveAction };
                }
                static_cast<QWaylandDrag *>(QGuiApplicationPrivate::platformIntegration()->drag())
                        ->setDropResponse(response);
            });
    connect(m_dragSource.data(), &QWaylandDataSource::finished, this, [this]() {
        static_cast<QWaylandDrag *>(QGuiApplicationPrivate::platformIntegration()->drag())->finishDrag();
        if (m_toplevelDrag) {
            m_toplevelDrag->destroy();
            m_toplevelDrag = nullptr;
        }
    });

    if (mimeData->hasFormat("application/x-qt-mainwindowdrag-window"_L1)
        && m_display->xdgToplevelDragManager()) {
        qintptr dockWindowPtr;
        QPoint offset;
        QDataStream windowStream(mimeData->data("application/x-qt-mainwindowdrag-window"_L1));
        windowStream >> dockWindowPtr;
        QWindow *dockWindow = reinterpret_cast<QWindow *>(dockWindowPtr);
        QDataStream offsetStream(mimeData->data("application/x-qt-mainwindowdrag-position"_L1));
        offsetStream >> offset;
        if (auto waylandWindow = static_cast<QWaylandWindow *>(dockWindow->handle())) {
            if (auto toplevel = waylandWindow->surfaceRole<xdg_toplevel>()) {
                m_toplevelDrag = new QtWayland::xdg_toplevel_drag_v1(
                        m_display->xdgToplevelDragManager()->get_xdg_toplevel_drag(
                                m_dragSource->object()));
                m_toplevelDrag->attach(toplevel, offset.x(), offset.y());
            }
        }
    }

    start_drag(m_dragSource->object(), origin->wlSurface(), icon->wlSurface(), m_display->lastInputSerial());
    return true;
}

void QWaylandDataDevice::cancelDrag()
{
    m_dragSource.reset();
}
#endif

void QWaylandDataDevice::data_device_data_offer(struct ::wl_data_offer *id)
{
    new QWaylandDataOffer(m_display, id);
}

#if QT_CONFIG(draganddrop)
void QWaylandDataDevice::data_device_drop()
{
    QDrag *drag = static_cast<QWaylandDrag *>(QGuiApplicationPrivate::platformIntegration()->drag())->currentDrag();

    QMimeData *dragData = nullptr;
    Qt::DropActions supportedActions;
    if (drag) {
        dragData = drag->mimeData();
        supportedActions = drag->supportedActions();
    } else if (m_dragOffer) {
        dragData = m_dragOffer->mimeData();
        supportedActions = m_dragOffer->supportedActions();
    } else {
        return;
    }

    QPlatformDropQtResponse response = QWindowSystemInterface::handleDrop(m_dragWindow, dragData, m_dragPoint, supportedActions,
                                                                          QGuiApplication::mouseButtons(),
                                                                          m_inputDevice->modifiers());

    // re-evaluate as it could have changed during user code above
    drag = static_cast<QWaylandDrag *>(QGuiApplicationPrivate::platformIntegration()->drag())->currentDrag();
    if (drag) {
        auto platformDrag =  static_cast<QWaylandDrag *>(QGuiApplicationPrivate::platformIntegration()->drag());
        platformDrag->setDropResponse(response);
        platformDrag->finishDrag();
    } else if (m_dragOffer) {
        m_dragOffer->finish();
    }
}

void QWaylandDataDevice::data_device_enter(uint32_t serial, wl_surface *surface, wl_fixed_t x, wl_fixed_t y, wl_data_offer *id)
{
    auto *dragWaylandWindow = surface ? QWaylandWindow::fromWlSurface(surface) : nullptr;
    if (!dragWaylandWindow)
        return; // Ignore foreign surfaces

    m_dragWindow = dragWaylandWindow->window();
    m_dragPoint = calculateDragPosition(x, y, m_dragWindow);
    m_enterSerial = serial;

    QMimeData *dragData = nullptr;
    Qt::DropActions supportedActions;

    m_dragOffer.reset(static_cast<QWaylandDataOffer *>(wl_data_offer_get_user_data(id)));
    QDrag *drag = static_cast<QWaylandDrag *>(QGuiApplicationPrivate::platformIntegration()->drag())->currentDrag();
    if (drag) {
        dragData = drag->mimeData();
        supportedActions = drag->supportedActions();
    } else if (m_dragOffer) {
        dragData = m_dragOffer->mimeData();
        supportedActions = m_dragOffer->supportedActions();
    }

    const QPlatformDragQtResponse &response = QWindowSystemInterface::handleDrag(
            m_dragWindow, dragData, m_dragPoint, supportedActions, QGuiApplication::mouseButtons(),
            m_inputDevice->modifiers());
    if (drag) {
        static_cast<QWaylandDrag *>(QGuiApplicationPrivate::platformIntegration()->drag())->setResponse(response);
    }

    sendResponse(supportedActions, response);
}

void QWaylandDataDevice::data_device_leave()
{
    if (m_dragWindow)
        QWindowSystemInterface::handleDrag(m_dragWindow, nullptr, QPoint(), Qt::IgnoreAction,
                                           QGuiApplication::mouseButtons(),
                                           m_inputDevice->modifiers());

    QDrag *drag = static_cast<QWaylandDrag *>(QGuiApplicationPrivate::platformIntegration()->drag())->currentDrag();
    if (!drag) {
        m_dragOffer.reset();
    }
}

void QWaylandDataDevice::data_device_motion(uint32_t time, wl_fixed_t x, wl_fixed_t y)
{
    Q_UNUSED(time);

    QDrag *drag = static_cast<QWaylandDrag *>(QGuiApplicationPrivate::platformIntegration()->drag())->currentDrag();

    if (!drag && !m_dragOffer)
        return;

    if (!m_dragWindow)
        return;

    m_dragPoint = calculateDragPosition(x, y, m_dragWindow);

    QMimeData *dragData = nullptr;
    Qt::DropActions supportedActions;
    if (drag) {
        dragData = drag->mimeData();
        supportedActions = drag->supportedActions();
    } else {
        dragData = m_dragOffer->mimeData();
        supportedActions = m_dragOffer->supportedActions();
    }

    const QPlatformDragQtResponse response = QWindowSystemInterface::handleDrag(m_dragWindow, dragData, m_dragPoint, supportedActions,
                                                                          QGuiApplication::mouseButtons(),
                                                                          m_inputDevice->modifiers());

    if (drag) {
        static_cast<QWaylandDrag *>(QGuiApplicationPrivate::platformIntegration()->drag())->setResponse(response);
    }

    sendResponse(supportedActions, response);
}
#endif // QT_CONFIG(draganddrop)

void QWaylandDataDevice::data_device_selection(wl_data_offer *id)
{
    if (id)
        m_selectionOffer.reset(static_cast<QWaylandDataOffer *>(wl_data_offer_get_user_data(id)));
    else
        m_selectionOffer.reset();

#if QT_CONFIG(clipboard)
    QGuiApplicationPrivate::platformIntegration()->clipboard()->emitChanged(QClipboard::Clipboard);
#endif
}

void QWaylandDataDevice::selectionSourceCancelled()
{
    m_selectionSource.reset();
#if QT_CONFIG(clipboard)
    QGuiApplicationPrivate::platformIntegration()->clipboard()->emitChanged(QClipboard::Clipboard);
#endif
}

#if QT_CONFIG(draganddrop)
void QWaylandDataDevice::dragSourceCancelled()
{
    static_cast<QWaylandDrag *>(QGuiApplicationPrivate::platformIntegration()->drag())->finishDrag();
    m_dragSource.reset();
    if (m_toplevelDrag) {
        m_toplevelDrag->destroy();
        m_toplevelDrag = nullptr;
    }
}

QPoint QWaylandDataDevice::calculateDragPosition(int x, int y, QWindow *wnd) const
{
    QPoint pnt(wl_fixed_to_int(x), wl_fixed_to_int(y));
    if (wnd) {
        QWaylandWindow *wwnd = static_cast<QWaylandWindow*>(m_dragWindow->handle());
        if (wwnd && wwnd->decoration()) {
            pnt -= QPoint(wwnd->decoration()->margins().left(),
                          wwnd->decoration()->margins().top());
        }
    }
    return pnt;
}

void QWaylandDataDevice::sendResponse(Qt::DropActions supportedActions, const QPlatformDragQtResponse &response)
{
    if (response.isAccepted()) {
        if (version() >= 3)
            m_dragOffer->set_actions(dropActionsToWl(supportedActions), dropActionsToWl(response.acceptedAction()));

        m_dragOffer->accept(m_enterSerial, m_dragOffer->firstFormat());
    } else {
        m_dragOffer->accept(m_enterSerial, QString());
    }
}

int QWaylandDataDevice::dropActionsToWl(Qt::DropActions actions)
{

    int wlActions = WL_DATA_DEVICE_MANAGER_DND_ACTION_NONE;
    if (actions & Qt::CopyAction)
        wlActions |= WL_DATA_DEVICE_MANAGER_DND_ACTION_COPY;
    if (actions & (Qt::MoveAction | Qt::TargetMoveAction))
        wlActions |= WL_DATA_DEVICE_MANAGER_DND_ACTION_MOVE;

    // wayland does not support LinkAction at the time of writing
    return wlActions;
}


#endif // QT_CONFIG(draganddrop)

}

QT_END_NAMESPACE

#include "moc_qwaylanddatadevice_p.cpp"
