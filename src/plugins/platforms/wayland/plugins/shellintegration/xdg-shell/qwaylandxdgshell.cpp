// Copyright (C) 2017 The Qt Company Ltd.
// Copyright (C) 2017 Eurogiciel, author: <philippe.coval@eurogiciel.fr>
// Copyright (C) 2023 David Edmundson <davidedmundson@kde.org>
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#include "qwaylandxdgshell_p.h"

#include "qwaylandxdgexporterv2_p.h"
#include "qwaylandxdgdialogv1_p.h"
#include "qwaylandxdgtopleveliconv1_p.h"

#include <QtWaylandClient/private/qwaylanddisplay_p.h>
#include <QtWaylandClient/private/qwaylandwindow_p.h>
#include <QtWaylandClient/private/qwaylandinputdevice_p.h>
#include <QtWaylandClient/private/qwaylandscreen_p.h>
#include <QtWaylandClient/private/qwaylandcursor_p.h>
#include <QtWaylandClient/private/qwaylandabstractdecoration_p.h>

#include <QtGui/QGuiApplication>
#include <QtGui/private/qwindow_p.h>

QT_BEGIN_NAMESPACE

namespace QtWaylandClient {

QWaylandXdgSurface::Toplevel::Toplevel(QWaylandXdgSurface *xdgSurface)
    : QtWayland::xdg_toplevel(xdgSurface->get_toplevel())
    , m_xdgSurface(xdgSurface)
{
    QWindow *window = xdgSurface->window()->window();
    if (auto *decorationManager = m_xdgSurface->m_shell->decorationManager()) {
        if (!(window->flags() & Qt::FramelessWindowHint))
            m_decoration = decorationManager->createToplevelDecoration(object());
    }
    requestWindowStates(window->windowStates());
    requestWindowFlags(window->flags());
    if (auto transientParent = xdgSurface->window()->transientParent()) {
        auto parentSurface = qobject_cast<QWaylandXdgSurface *>(transientParent->shellSurface());
        if (parentSurface && parentSurface->m_toplevel)
            set_parent(parentSurface->m_toplevel->object());
    }

    // Always use XDG Dialog, a window could be assigned a parent through XDG Foreign.
    if (window->modality() != Qt::NonModal && m_xdgSurface->m_shell->m_xdgDialogWm) {
        m_xdgDialog.reset(m_xdgSurface->m_shell->m_xdgDialogWm->getDialog(object()));
        m_xdgDialog->set_modal();
    }
}

QWaylandXdgSurface::Toplevel::~Toplevel()
{
    // The protocol spec requires that the decoration object is deleted before xdg_toplevel.
    delete m_decoration;
    m_decoration = nullptr;

    if (isInitialized())
        destroy();
}

void QWaylandXdgSurface::Toplevel::applyConfigure()
{
    if (!(m_applied.states & (Qt::WindowMaximized|Qt::WindowFullScreen)))
        m_normalSize = m_xdgSurface->m_window->windowContentGeometry().size();

    if ((m_pending.states & Qt::WindowActive) && !(m_applied.states & Qt::WindowActive)
        && !m_xdgSurface->m_window->display()->isKeyboardAvailable())
        m_xdgSurface->m_window->display()->handleWindowActivated(m_xdgSurface->m_window);

    if (!(m_pending.states & Qt::WindowActive) && (m_applied.states & Qt::WindowActive)
        && !m_xdgSurface->m_window->display()->isKeyboardAvailable())
        m_xdgSurface->m_window->display()->handleWindowDeactivated(m_xdgSurface->m_window);

    m_xdgSurface->m_window->handleToplevelWindowTilingStatesChanged(m_toplevelStates);
    m_xdgSurface->m_window->handleWindowStatesChanged(m_pending.states);

    // If the width or height is zero, the client should decide the size on its own.
    QSize surfaceSize;

    if (m_pending.size.width() > 0) {
        surfaceSize.setWidth(m_pending.size.width());
    } else {
        if (Q_UNLIKELY(m_pending.states & (Qt::WindowMaximized | Qt::WindowFullScreen))) {
            qCWarning(lcQpaWayland) << "Configure event with maximized or fullscreen state contains invalid width:" << m_pending.size.width();
        } else {
            int width = m_normalSize.width();
            if (!m_pending.bounds.isEmpty())
                width = std::min(width, m_pending.bounds.width());
            surfaceSize.setWidth(width);
        }
    }

    if (m_pending.size.height() > 0) {
        surfaceSize.setHeight(m_pending.size.height());
    } else {
        if (Q_UNLIKELY(m_pending.states & (Qt::WindowMaximized | Qt::WindowFullScreen))) {
            qCWarning(lcQpaWayland) << "Configure event with maximized or fullscreen state contains invalid height:" << m_pending.size.height();
        } else {
            int height = m_normalSize.height();
            if (!m_pending.bounds.isEmpty())
                height = std::min(height, m_pending.bounds.height());
            surfaceSize.setHeight(height);
        }
    }

    m_applied = m_pending;

    if (!surfaceSize.isEmpty())
        m_xdgSurface->m_window->resizeFromApplyConfigure(surfaceSize.grownBy(m_xdgSurface->m_window->windowContentMargins()));

    qCDebug(lcQpaWayland) << "Applied pending xdg_toplevel configure event:" << m_applied.size
                          << "and" << m_applied.states
                          << ", suspended " << m_applied.suspended;
}

bool QWaylandXdgSurface::Toplevel::wantsDecorations()
{
    if (m_decoration && (m_decoration->pending() == QWaylandXdgToplevelDecorationV1::mode_server_side
                         || !m_decoration->isConfigured()))
        return false;

    return !(m_pending.states & Qt::WindowFullScreen);
}

void QWaylandXdgSurface::Toplevel::xdg_toplevel_configure_bounds(int32_t width, int32_t height)
{
    m_pending.bounds = QSize(width, height);
}

void QWaylandXdgSurface::Toplevel::xdg_toplevel_configure(int32_t width, int32_t height, wl_array *states)
{
    m_pending.size = QSize(width, height);

    auto *xdgStates = static_cast<uint32_t *>(states->data);
    size_t numStates = states->size / sizeof(uint32_t);

    m_pending.suspended = false;
    m_pending.states = Qt::WindowNoState;
    m_toplevelStates = QWaylandWindow::WindowNoState;

    for (size_t i = 0; i < numStates; i++) {
        switch (xdgStates[i]) {
        case XDG_TOPLEVEL_STATE_ACTIVATED:
            m_pending.states |= Qt::WindowActive;
            break;
        case XDG_TOPLEVEL_STATE_MAXIMIZED:
            m_pending.states |= Qt::WindowMaximized;
            break;
        case XDG_TOPLEVEL_STATE_FULLSCREEN:
            m_pending.states |= Qt::WindowFullScreen;
            break;
        case XDG_TOPLEVEL_STATE_TILED_LEFT:
            m_toplevelStates |= QWaylandWindow::WindowTiledLeft;
            break;
        case XDG_TOPLEVEL_STATE_TILED_RIGHT:
            m_toplevelStates |= QWaylandWindow::WindowTiledRight;
            break;
        case XDG_TOPLEVEL_STATE_TILED_TOP:
            m_toplevelStates |= QWaylandWindow::WindowTiledTop;
            break;
        case XDG_TOPLEVEL_STATE_TILED_BOTTOM:
            m_toplevelStates |= QWaylandWindow::WindowTiledBottom;
            break;
        case XDG_TOPLEVEL_STATE_SUSPENDED:
            m_pending.suspended = true;
            break;
        default:
            break;
        }
    }
    qCDebug(lcQpaWayland) << "Received xdg_toplevel.configure with" << m_pending.size
                          << "and" << m_pending.states
                          << ", suspended " << m_pending.suspended;
}

void QWaylandXdgSurface::Toplevel::xdg_toplevel_close()
{
    QWindowSystemInterface::handleCloseEvent(m_xdgSurface->m_window->window());
}

void QWaylandXdgSurface::Toplevel::requestWindowFlags(Qt::WindowFlags flags)
{
    if (m_decoration) {
        if (flags & Qt::FramelessWindowHint) {
            delete m_decoration;
            m_decoration = nullptr;
        } else {
            m_decoration->unsetMode();
        }
    }
}

void QWaylandXdgSurface::Toplevel::requestWindowStates(Qt::WindowStates states)
{
    // Re-send what's different from the applied state
    Qt::WindowStates changedStates = m_applied.states ^ states;

    // Minimized state is not reported by the protocol, so always send it
    if (states & Qt::WindowMinimized) {
        set_minimized();
        m_xdgSurface->window()->handleWindowStatesChanged(states & ~Qt::WindowMinimized);
        // The internal window state whilst minimized is not maximised or fullscreen, but we don't want to
        // update the compositors cached version of this state
        return;
    }

    if (changedStates & Qt::WindowMaximized) {
        if (states & Qt::WindowMaximized)
            set_maximized();
        else
            unset_maximized();
    }

    if (changedStates & Qt::WindowFullScreen) {
        if (states & Qt::WindowFullScreen) {
            auto screen = m_xdgSurface->window()->waylandScreen();
            if (screen) {
                set_fullscreen(screen->output());
            }
        } else
            unset_fullscreen();
    }


}

QtWayland::xdg_toplevel::resize_edge QWaylandXdgSurface::Toplevel::convertToResizeEdges(Qt::Edges edges)
{
    return static_cast<enum resize_edge>(
                ((edges & Qt::TopEdge) ? resize_edge_top : 0)
                | ((edges & Qt::BottomEdge) ? resize_edge_bottom : 0)
                | ((edges & Qt::LeftEdge) ? resize_edge_left : 0)
                | ((edges & Qt::RightEdge) ? resize_edge_right : 0));
}

QWaylandXdgSurface::Popup::Popup(QWaylandXdgSurface *xdgSurface, QWaylandWindow *parent,
                                 Positioner *positioner)
    : m_xdgSurface(xdgSurface)
    , m_parentXdgSurface(qobject_cast<QWaylandXdgSurface *>(parent->shellSurface()))
    , m_parent(parent)
{

    init(xdgSurface->get_popup(m_parentXdgSurface ? m_parentXdgSurface->object() : nullptr,
                               positioner->object()));
}

QWaylandXdgSurface::Popup::~Popup()
{
    if (isInitialized())
        destroy();

    if (m_grabbing) {
        m_grabbing = false;

        // Synthesize Qt enter/leave events for popup
        QWindow *leave = nullptr;
        if (m_xdgSurface && m_xdgSurface->window())
            leave = m_xdgSurface->window()->window();
        QWindowSystemInterface::handleLeaveEvent(leave);

        QWindow *enter = nullptr;
        if (m_parentXdgSurface && m_parentXdgSurface->window()) {
            enter = m_parentXdgSurface->window()->window();
            const auto pos = m_xdgSurface->window()->display()->waylandCursor()->pos();
            QWindowSystemInterface::handleEnterEvent(enter, enter->handle()->mapFromGlobal(pos), pos);
        }
    }
}

void QWaylandXdgSurface::Popup::applyConfigure()
{
    if (m_pendingGeometry.isValid()) {
        QRect geometryWithMargins = m_pendingGeometry.marginsAdded(m_xdgSurface->m_window->windowContentMargins());
        QMargins parentMargins = m_parent->windowContentMargins() - m_parent->clientSideMargins();
        QRect globalGeometry = geometryWithMargins.translated(m_parent->geometry().topLeft() + QPoint(parentMargins.left(), parentMargins.top()));
        m_xdgSurface->setGeometryFromApplyConfigure(globalGeometry.topLeft(), globalGeometry.size());
    }
    resetConfiguration();
}

void QWaylandXdgSurface::Popup::resetConfiguration()
{
    m_pendingGeometry = QRect();
}

void QWaylandXdgSurface::Popup::grab(QWaylandInputDevice *seat, uint serial)
{
    xdg_popup::grab(seat->wl_seat(), serial);
    m_grabbing = true;
}

void QWaylandXdgSurface::Popup::xdg_popup_configure(int32_t x, int32_t y, int32_t width, int32_t height)
{
    m_pendingGeometry = QRect(x, y, width, height);
}

void QWaylandXdgSurface::Popup::xdg_popup_popup_done()
{
    QWindowSystemInterface::handleCloseEvent(m_xdgSurface->m_window->window());
}

void QWaylandXdgSurface::Popup::xdg_popup_repositioned(uint32_t token)
{
    if (token == m_waitingForRepositionSerial)
        m_waitingForReposition = false;
}

QWaylandXdgSurface::QWaylandXdgSurface(QWaylandXdgShell *shell, ::xdg_surface *surface, QWaylandWindow *window)
    : QWaylandShellSurface(window)
    , xdg_surface(surface)
    , m_shell(shell)
    , m_window(window)
{
    QWaylandDisplay *display = window->display();
    Qt::WindowType type =  static_cast<Qt::WindowType>(int(window->windowFlags() & Qt::WindowType_Mask));
    auto *transientParent = window->transientParent();

    if (type == Qt::ToolTip)
        setPopup(transientParent);
    else if (type == Qt::Popup)
        setGrabPopup(transientParent, display->lastInputDevice(), display->lastInputSerial());
    else
        setToplevel();

    setSizeHints();
}

QWaylandXdgSurface::~QWaylandXdgSurface()
{
    if (m_toplevel) {
        delete m_toplevel;
        m_toplevel = nullptr;
    }
    if (m_popup) {
        delete m_popup;
        m_popup = nullptr;
    }
    destroy();
}

bool QWaylandXdgSurface::resize(QWaylandInputDevice *inputDevice, Qt::Edges edges)
{
    if (!m_toplevel || !m_toplevel->isInitialized())
        return false;

    auto resizeEdges = Toplevel::convertToResizeEdges(edges);
    m_toplevel->resize(inputDevice->wl_seat(), inputDevice->serial(), resizeEdges);
    return true;
}

bool QWaylandXdgSurface::move(QWaylandInputDevice *inputDevice)
{
    if (m_toplevel && m_toplevel->isInitialized()) {
        m_toplevel->move(inputDevice->wl_seat(), inputDevice->serial());
        return true;
    }
    return false;
}

bool QWaylandXdgSurface::showWindowMenu(QWaylandInputDevice *seat)
{
    if (m_toplevel && m_toplevel->isInitialized()) {
        QPoint position = seat->pointerSurfacePosition().toPoint();
        m_toplevel->show_window_menu(seat->wl_seat(), seat->serial(), position.x(), position.y());
        return true;
    }
    return false;
}

void QWaylandXdgSurface::setTitle(const QString &title)
{
    if (m_toplevel)
        m_toplevel->set_title(title);
}

void QWaylandXdgSurface::setAppId(const QString &appId)
{
    if (m_toplevel)
        m_toplevel->set_app_id(appId);

    m_appId = appId;
}

void QWaylandXdgSurface::setWindowFlags(Qt::WindowFlags flags)
{
    if (m_toplevel)
        m_toplevel->requestWindowFlags(flags);
}

bool QWaylandXdgSurface::isExposed() const
{
    if (m_toplevel && m_toplevel->m_applied.suspended)
        return false;

    // the popup repositioning specification is async
    // we need to defer commits between our resize request
    // and our new popup position being set
    if (m_popup && m_popup->m_waitingForReposition)
        return false;

    return m_configured;
}

void QWaylandXdgSurface::applyConfigure()
{
    // It is a redundant ack_configure, so skipped.
    if (m_pendingConfigureSerial == m_appliedConfigureSerial)
        return;

    m_appliedConfigureSerial = m_pendingConfigureSerial;

    m_configured = true;
    ack_configure(m_appliedConfigureSerial);

    if (m_toplevel)
        m_toplevel->applyConfigure();
    if (m_popup)
        m_popup->applyConfigure();

    setContentGeometry(window()->windowContentGeometry());
    window()->updateExposure();
}

bool QWaylandXdgSurface::wantsDecorations() const
{
    return m_toplevel && m_toplevel->wantsDecorations();
}

void QWaylandXdgSurface::propagateSizeHints()
{
    setSizeHints();
}

void QWaylandXdgSurface::setContentGeometry(const QRect &rect)
{
    if (!isExposed() || m_lastGeometry == rect)
        return;

    set_window_geometry(rect.x(), rect.y(), rect.width(), rect.height());
    m_lastGeometry = rect;
}

void QWaylandXdgSurface::setSizeHints()
{
    if (m_toplevel && m_window) {
        const QMargins margins = m_window->windowContentMargins() - m_window->clientSideMargins();
        const QSize minSize = m_window->windowMinimumSize().shrunkBy(margins);
        const QSize maxSize = m_window->windowMaximumSize().shrunkBy(margins);
        const int minWidth = qMax(0, minSize.width());
        const int minHeight = qMax(0, minSize.height());
        int maxWidth = qMax(0, maxSize.width());
        int maxHeight = qMax(0, maxSize.height());

        // It will not change min/max sizes if invalid.
        if (minWidth > maxWidth || minHeight > maxHeight)
            return;

        if (maxWidth == QWINDOWSIZE_MAX)
            maxWidth = 0;
        if (maxHeight == QWINDOWSIZE_MAX)
            maxHeight = 0;

        m_toplevel->set_min_size(minWidth, minHeight);
        m_toplevel->set_max_size(maxWidth, maxHeight);
    }
}

void *QWaylandXdgSurface::nativeResource(const QByteArray &resource)
{
    QByteArray lowerCaseResource = resource.toLower();
    if (lowerCaseResource == "xdg_surface")
        return object();
    else if (lowerCaseResource == "xdg_toplevel" && m_toplevel)
        return m_toplevel->object();
    else if (lowerCaseResource == "xdg_popup" && m_popup)
        return m_popup->object();
    return nullptr;
}

std::any QWaylandXdgSurface::surfaceRole() const
{
    if (m_toplevel)
        return m_toplevel->object();
    if (m_popup)
        return m_popup->object();
    return {};
}

void QWaylandXdgSurface::requestWindowStates(Qt::WindowStates states)
{
    if (m_toplevel)
        m_toplevel->requestWindowStates(states);
    else
        qCDebug(lcQpaWayland) << "Ignoring window states requested by non-toplevel zxdg_surface_v6.";
}

void QWaylandXdgSurface::setToplevel()
{
    Q_ASSERT(!m_toplevel && !m_popup);
    m_toplevel = new Toplevel(this);
}

void QWaylandXdgSurface::setPopup(QWaylandWindow *parent)
{
    Q_ASSERT(!m_toplevel && !m_popup);

    std::unique_ptr<Positioner> positioner = createPositioner(parent);
    m_popup = new Popup(this, parent, positioner.get());
}

void QWaylandXdgSurface::setGrabPopup(QWaylandWindow *parent, QWaylandInputDevice *device, int serial)
{
    setPopup(parent);
    m_popup->grab(device, serial);

    // Synthesize Qt enter/leave events for popup
    if (!parent)
        return;
    QWindow *leave = parent->window();
    QWindowSystemInterface::handleLeaveEvent(leave);

    QWindow *enter = nullptr;
    if (m_popup && m_popup->m_xdgSurface && m_popup->m_xdgSurface->window())
        enter = m_popup->m_xdgSurface->window()->window();

    if (enter) {
        const auto pos = m_popup->m_xdgSurface->window()->display()->waylandCursor()->pos();
        QWindowSystemInterface::handleEnterEvent(enter, enter->handle()->mapFromGlobal(pos), pos);
    }
}

void QWaylandXdgSurface::xdg_surface_configure(uint32_t serial)
{
    m_pendingConfigureSerial = serial;
    if (!m_configured) {
        // We have to do the initial applyConfigure() immediately, since that is the expose.
        applyConfigure();
    } else {
        // Later configures are probably resizes, so we have to queue them up for a time when we
        // are not painting to the window.
        m_window->applyConfigureWhenPossible();
    }
}

bool QWaylandXdgSurface::requestActivate()
{
    if (auto *activation = m_shell->activation()) {
        if (!m_activationToken.isEmpty()) {
            activation->activate(m_activationToken, window()->wlSurface());
            m_activationToken = {};
            return true;
        } else if (const auto token = qEnvironmentVariable("XDG_ACTIVATION_TOKEN"); !token.isEmpty()) {
            activation->activate(token, window()->wlSurface());
            qunsetenv("XDG_ACTIVATION_TOKEN");
            return true;
        } else {
            const auto focusWindow = QGuiApplication::focusWindow();
            // At least GNOME requires to request the token in order to get the
            // focus stealing prevention indication, so requestXdgActivationToken call
            // is still necessary in that case.
            const auto wlWindow = focusWindow ? static_cast<QWaylandWindow*>(focusWindow->handle()) : m_window;

            QString appId;
            if (const auto xdgSurface = qobject_cast<QWaylandXdgSurface *>(wlWindow->shellSurface()))
                appId = xdgSurface->m_appId;

            std::optional<uint32_t> serial;
            if (const auto seat = wlWindow->display()->lastInputDevice())
                serial = seat->serial();

            const auto tokenProvider = activation->requestXdgActivationToken(
                    wlWindow->display(), wlWindow->wlSurface(), serial, appId);
            connect(tokenProvider, &QWaylandXdgActivationTokenV1::done, this,
                    [this](const QString &token) {
                        m_shell->activation()->activate(token, window()->wlSurface());
                    });
            connect(tokenProvider, &QWaylandXdgActivationTokenV1::done, tokenProvider, &QObject::deleteLater);
            return true;
        }
    }
    return false;
}

bool QWaylandXdgSurface::requestActivateOnShow()
{
    const Qt::WindowType type = m_window->window()->type();
    if (type == Qt::ToolTip || type == Qt::Popup || type == Qt::SplashScreen)
        return false;

    const Qt::WindowFlags flags = m_window->window()->flags();
    if (flags & Qt::WindowDoesNotAcceptFocus)
        return false;

    if (m_window->window()->property("_q_showWithoutActivating").toBool())
        return false;

    return requestActivate();
}

void QWaylandXdgSurface::requestXdgActivationToken(quint32 serial)
{
    if (auto *activation = m_shell->activation()) {
        auto tokenProvider = activation->requestXdgActivationToken(
                m_shell->m_display, m_window->wlSurface(), serial, m_appId);
        connect(tokenProvider, &QWaylandXdgActivationTokenV1::done, m_window, &QWaylandWindow::xdgActivationTokenCreated);
        connect(tokenProvider, &QWaylandXdgActivationTokenV1::done, tokenProvider, &QObject::deleteLater);
    } else {
        QWaylandShellSurface::requestXdgActivationToken(serial);
    }
}

void QWaylandXdgSurface::setXdgActivationToken(const QString &token)
{
    if (m_shell->activation()) {
        m_activationToken = token;
    } else {
        qCWarning(lcQpaWayland) << "zxdg_activation_v1 not available";
    }
}

void QWaylandXdgSurface::setAlertState(bool enabled)
{
    if (m_alertState == enabled)
        return;

    m_alertState = enabled;

    if (!m_alertState)
        return;

    auto *activation = m_shell->activation();
    if (!activation)
        return;

    const auto tokenProvider = activation->requestXdgActivationToken(
            m_shell->m_display, m_window->wlSurface(), std::nullopt, m_appId);
    connect(tokenProvider, &QWaylandXdgActivationTokenV1::done, this,
            [this](const QString &token) {
                m_shell->activation()->activate(token, m_window->wlSurface());
            });
    connect(tokenProvider, &QWaylandXdgActivationTokenV1::done, tokenProvider, &QObject::deleteLater);
}

QString QWaylandXdgSurface::externWindowHandle()
{
    if (!m_toplevel || !m_shell->exporter()) {
        return QString();
    }
    if (!m_toplevel->m_exported) {
        auto *exporterWrapper = static_cast<zxdg_exporter_v2 *>(
                wl_proxy_create_wrapper(m_shell->exporter()->object()));
        auto exportQueue = wl_display_create_queue(m_shell->display()->wl_display());
        wl_proxy_set_queue(reinterpret_cast<wl_proxy *>(exporterWrapper), exportQueue);
        m_toplevel->m_exported.reset(new QWaylandXdgExportedV2(
                zxdg_exporter_v2_export_toplevel(exporterWrapper, m_window->wlSurface())));
        // handle events is sent immediately
        wl_display_roundtrip_queue(m_shell->display()->wl_display(), exportQueue);

        wl_proxy_set_queue(reinterpret_cast<wl_proxy *>(m_toplevel->m_exported->object()), nullptr);
        wl_proxy_wrapper_destroy(exporterWrapper);
        wl_event_queue_destroy(exportQueue);
    }
    return m_toplevel->m_exported->handle();
}

void QWaylandXdgSurface::setWindowPosition(const QPoint &position)
{
    Q_UNUSED(position);

    if (!m_popup)
        return;

    if (m_popup->version() < XDG_POPUP_REPOSITIONED_SINCE_VERSION)
        return;

    std::unique_ptr<Positioner> positioner = createPositioner(m_window->transientParent());
    m_popup->m_waitingForRepositionSerial++;
    m_popup->reposition(positioner->object(), m_popup->m_waitingForRepositionSerial);
    m_popup->m_waitingForReposition = true;
    window()->updateExposure();
}

std::unique_ptr<QWaylandXdgSurface::Positioner> QWaylandXdgSurface::createPositioner(QWaylandWindow *parent)
{
    std::unique_ptr<Positioner> positioner(new Positioner(m_shell));
    // set_popup expects a position relative to the parent
    QRect windowGeometry = m_window->windowContentGeometry();
    QMargins windowMargins = m_window->windowContentMargins() - m_window->clientSideMargins();
    QMargins parentMargins = parent->windowContentMargins() - parent->clientSideMargins();

    // These property overrides may be removed when public API becomes available
    QRect placementAnchor = m_window->window()->property("_q_waylandPopupAnchorRect").toRect();
    if (!placementAnchor.isValid()) {
        placementAnchor = QRect(m_window->geometry().topLeft() - parent->geometry().topLeft(), QSize(1,1));
    }
    placementAnchor.translate(windowMargins.left(), windowMargins.top());
    placementAnchor.translate(-parentMargins.left(), -parentMargins.top());

    uint32_t anchor = QtWayland::xdg_positioner::anchor_top_left;
    const QVariant anchorVariant = m_window->window()->property("_q_waylandPopupAnchor");
    if (anchorVariant.isValid()) {
        switch (anchorVariant.value<Qt::Edges>()) {
        case Qt::Edges():
            anchor = QtWayland::xdg_positioner::anchor_none;
            break;
        case Qt::TopEdge:
            anchor = QtWayland::xdg_positioner::anchor_top;
            break;
        case Qt::TopEdge | Qt::RightEdge:
            anchor = QtWayland::xdg_positioner::anchor_top_right;
            break;
        case Qt::RightEdge:
            anchor = QtWayland::xdg_positioner::anchor_right;
            break;
        case Qt::BottomEdge | Qt::RightEdge:
            anchor = QtWayland::xdg_positioner::anchor_bottom_right;
            break;
        case Qt::BottomEdge:
            anchor = QtWayland::xdg_positioner::anchor_bottom;
            break;
        case Qt::BottomEdge | Qt::LeftEdge:
            anchor = QtWayland::xdg_positioner::anchor_bottom_left;
            break;
        case Qt::LeftEdge:
            anchor = QtWayland::xdg_positioner::anchor_left;
            break;
        case Qt::TopEdge | Qt::LeftEdge:
            anchor = QtWayland::xdg_positioner::anchor_top_left;
            break;
        }
    }

    uint32_t gravity = QtWayland::xdg_positioner::gravity_bottom_right;
    const QVariant popupGravityVariant = m_window->window()->property("_q_waylandPopupGravity");
    if (popupGravityVariant.isValid()) {
        switch (popupGravityVariant.value<Qt::Edges>()) {
        case Qt::Edges():
            gravity = QtWayland::xdg_positioner::gravity_none;
            break;
        case Qt::TopEdge:
            gravity = QtWayland::xdg_positioner::gravity_top;
            break;
        case Qt::TopEdge | Qt::RightEdge:
            gravity = QtWayland::xdg_positioner::gravity_top_right;
            break;
        case Qt::RightEdge:
            gravity = QtWayland::xdg_positioner::gravity_right;
            break;
        case Qt::BottomEdge | Qt::RightEdge:
            gravity = QtWayland::xdg_positioner::gravity_bottom_right;
            break;
        case Qt::BottomEdge:
            gravity = QtWayland::xdg_positioner::gravity_bottom;
            break;
        case Qt::BottomEdge | Qt::LeftEdge:
            gravity = QtWayland::xdg_positioner::gravity_bottom_left;
            break;
        case Qt::LeftEdge:
            gravity = QtWayland::xdg_positioner::gravity_left;
            break;
        case Qt::TopEdge | Qt::LeftEdge:
            gravity = QtWayland::xdg_positioner::gravity_top_left;
            break;
        }
    }

    uint32_t constraintAdjustment = QtWayland::xdg_positioner::constraint_adjustment_slide_x | QtWayland::xdg_positioner::constraint_adjustment_slide_y;
    const QVariant constraintAdjustmentVariant = m_window->window()->property("_q_waylandPopupConstraintAdjustment");
    if (constraintAdjustmentVariant.isValid()) {
        constraintAdjustment = constraintAdjustmentVariant.toUInt();
    }

    positioner->set_anchor_rect(placementAnchor.x(),
                                placementAnchor.y(),
                                placementAnchor.width(),
                                placementAnchor.height());
    positioner->set_anchor(anchor);
    positioner->set_gravity(gravity);
    positioner->set_size(windowGeometry.width(), windowGeometry.height());
    positioner->set_constraint_adjustment(constraintAdjustment);
    return positioner;
}


void QWaylandXdgSurface::setIcon(const QIcon &icon)
{
    if (!m_shell->m_topLevelIconManager || !m_toplevel)
        return;

    m_shell->m_topLevelIconManager->setIcon(icon, m_toplevel->object());
}

QWaylandXdgShell::QWaylandXdgShell(QWaylandDisplay *display, QtWayland::xdg_wm_base *xdgWmBase)
    : m_display(display), m_xdgWmBase(xdgWmBase)
{
    display->addRegistryListener(&QWaylandXdgShell::handleRegistryGlobal, this);
}

QWaylandXdgShell::~QWaylandXdgShell()
{
    m_display->removeListener(&QWaylandXdgShell::handleRegistryGlobal, this);
}

void QWaylandXdgShell::handleRegistryGlobal(void *data, wl_registry *registry, uint id,
                                            const QString &interface, uint version)
{
    QWaylandXdgShell *xdgShell = static_cast<QWaylandXdgShell *>(data);
    if (interface == QLatin1String(QWaylandXdgDecorationManagerV1::interface()->name))
        xdgShell->m_xdgDecorationManager.reset(new QWaylandXdgDecorationManagerV1(registry, id, version));

    if (interface == QLatin1String(QWaylandXdgActivationV1::interface()->name)) {
        xdgShell->m_xdgActivation.reset(new QWaylandXdgActivationV1(registry, id, version));
    }

    if (interface == QLatin1String(QWaylandXdgExporterV2::interface()->name)) {
        xdgShell->m_xdgExporter.reset(new QWaylandXdgExporterV2(registry, id, version));
    }

    if (interface == QLatin1String(QWaylandXdgDialogWmV1::interface()->name)) {
        xdgShell->m_xdgDialogWm.reset(new QWaylandXdgDialogWmV1(registry, id, version));
    }
    if (interface == QLatin1String(QtWayland::xdg_toplevel_icon_manager_v1::interface()->name)) {
        xdgShell->m_topLevelIconManager.reset(
                new QWaylandXdgToplevelIconManagerV1(xdgShell->m_display, registry, id, version));
    }
}

QWaylandXdgSurface::Positioner::Positioner(QWaylandXdgShell *xdgShell)
    : QtWayland::xdg_positioner(xdgShell->m_xdgWmBase->create_positioner())
{
}

QWaylandXdgSurface::Positioner::~Positioner()
{
    destroy();
}

}

QT_END_NAMESPACE

#include "moc_qwaylandxdgshell_p.cpp"
