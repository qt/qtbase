/****************************************************************************
**
** Copyright (C) 2016 The Qt Company Ltd.
** Contact: https://www.qt.io/licensing/
**
** This file is part of the plugins of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:LGPL$
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company. For licensing terms
** and conditions see https://www.qt.io/terms-conditions. For further
** information use the contact form at https://www.qt.io/contact-us.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 3 as published by the Free Software
** Foundation and appearing in the file LICENSE.LGPL3 included in the
** packaging of this file. Please review the following information to
** ensure the GNU Lesser General Public License version 3 requirements
** will be met: https://www.gnu.org/licenses/lgpl-3.0.html.
**
** GNU General Public License Usage
** Alternatively, this file may be used under the terms of the GNU
** General Public License version 2.0 or (at your option) the GNU General
** Public license version 3 or any later version approved by the KDE Free
** Qt Foundation. The licenses are as published by the Free Software
** Foundation and appearing in the file LICENSE.GPL2 and LICENSE.GPL3
** included in the packaging of this file. Please review the following
** information to ensure the GNU General Public License requirements will
** be met: https://www.gnu.org/licenses/gpl-2.0.html and
** https://www.gnu.org/licenses/gpl-3.0.html.
**
** $QT_END_LICENSE$
**
****************************************************************************/

#include "qxcbscreen.h"
#include "qxcbwindow.h"
#include "qxcbcursor.h"
#include "qxcbimage.h"
#include "qnamespace.h"
#include "qxcbxsettings.h"

#include <stdio.h>

#include <QDebug>
#include <QtAlgorithms>

#include <qpa/qwindowsysteminterface.h>
#include <private/qmath_p.h>
#include <QtGui/private/qhighdpiscaling_p.h>

QT_BEGIN_NAMESPACE

QXcbVirtualDesktop::QXcbVirtualDesktop(QXcbConnection *connection, xcb_screen_t *screen, int number)
    : QXcbObject(connection)
    , m_screen(screen)
    , m_number(number)
    , m_xSettings(Q_NULLPTR)
{
    const QByteArray cmAtomName =  "_NET_WM_CM_S" + QByteArray::number(m_number);
    m_net_wm_cm_atom = connection->internAtom(cmAtomName.constData());
    m_compositingActive = connection->getSelectionOwner(m_net_wm_cm_atom);

    m_workArea = getWorkArea();
}

QXcbVirtualDesktop::~QXcbVirtualDesktop()
{
    delete m_xSettings;
}

QXcbScreen *QXcbVirtualDesktop::screenAt(const QPoint &pos) const
{
    const auto screens = connection()->screens();
    for (QXcbScreen *screen : screens) {
        if (screen->virtualDesktop() == this && screen->geometry().contains(pos))
            return screen;
    }
    return Q_NULLPTR;
}

void QXcbVirtualDesktop::addScreen(QPlatformScreen *s)
{
    ((QXcbScreen *) s)->isPrimary() ? m_screens.prepend(s) : m_screens.append(s);
}

void QXcbVirtualDesktop::setPrimaryScreen(QPlatformScreen *s)
{
    const int idx = m_screens.indexOf(s);
    Q_ASSERT(idx > -1);
    m_screens.swap(0, idx);
}

QXcbXSettings *QXcbVirtualDesktop::xSettings() const
{
    if (!m_xSettings) {
        QXcbVirtualDesktop *self = const_cast<QXcbVirtualDesktop *>(this);
        self->m_xSettings = new QXcbXSettings(self);
    }
    return m_xSettings;
}

bool QXcbVirtualDesktop::compositingActive() const
{
    if (connection()->hasXFixes())
        return m_compositingActive;
    else
        return connection()->getSelectionOwner(m_net_wm_cm_atom);
}

void QXcbVirtualDesktop::handleXFixesSelectionNotify(xcb_xfixes_selection_notify_event_t *notify_event)
{
    if (notify_event->selection == m_net_wm_cm_atom)
        m_compositingActive = notify_event->owner;
}

void QXcbVirtualDesktop::subscribeToXFixesSelectionNotify()
{
    if (connection()->hasXFixes()) {
        const uint32_t mask = XCB_XFIXES_SELECTION_EVENT_MASK_SET_SELECTION_OWNER |
                              XCB_XFIXES_SELECTION_EVENT_MASK_SELECTION_WINDOW_DESTROY |
                              XCB_XFIXES_SELECTION_EVENT_MASK_SELECTION_CLIENT_CLOSE;
        Q_XCB_CALL(xcb_xfixes_select_selection_input_checked(xcb_connection(), connection()->getQtSelectionOwner(), m_net_wm_cm_atom, mask));
    }
}

QRect QXcbVirtualDesktop::getWorkArea() const
{
    QRect r;
    xcb_get_property_reply_t * workArea =
        xcb_get_property_reply(xcb_connection(),
            xcb_get_property_unchecked(xcb_connection(), false, screen()->root,
                             atom(QXcbAtom::_NET_WORKAREA),
                             XCB_ATOM_CARDINAL, 0, 1024), NULL);
    if (workArea && workArea->type == XCB_ATOM_CARDINAL && workArea->format == 32 && workArea->value_len >= 4) {
        // If workArea->value_len > 4, the remaining ones seem to be for WM's virtual desktops
        // (don't mess with QXcbVirtualDesktop which represents an X screen).
        // But QScreen doesn't know about that concept.  In reality there could be a
        // "docked" panel (with _NET_WM_STRUT_PARTIAL atom set) on just one desktop.
        // But for now just assume the first 4 values give us the geometry of the
        // "work area", AKA "available geometry"
        uint32_t *geom = (uint32_t*)xcb_get_property_value(workArea);
        r = QRect(geom[0], geom[1], geom[2], geom[3]);
    } else {
        r = QRect(QPoint(), size());
    }
    free(workArea);
    return r;
}

void QXcbVirtualDesktop::updateWorkArea()
{
    QRect workArea = getWorkArea();
    if (m_workArea != workArea) {
        m_workArea = workArea;
        for (QPlatformScreen *screen : qAsConst(m_screens))
            ((QXcbScreen *)screen)->updateAvailableGeometry();
    }
}

static inline QSizeF sizeInMillimeters(const QSize &size, const QDpi &dpi)
{
    return QSizeF(Q_MM_PER_INCH * size.width() / dpi.first,
                  Q_MM_PER_INCH * size.height() / dpi.second);
}

QXcbScreen::QXcbScreen(QXcbConnection *connection, QXcbVirtualDesktop *virtualDesktop,
                       xcb_randr_output_t outputId, xcb_randr_get_output_info_reply_t *output,
                       const xcb_xinerama_screen_info_t *xineramaScreenInfo, int xineramaScreenIdx)
    : QXcbObject(connection)
    , m_virtualDesktop(virtualDesktop)
    , m_output(outputId)
    , m_crtc(output ? output->crtc : XCB_NONE)
    , m_mode(XCB_NONE)
    , m_primary(false)
    , m_rotation(XCB_RANDR_ROTATION_ROTATE_0)
    , m_outputName(getOutputName(output))
    , m_outputSizeMillimeters(output ? QSize(output->mm_width, output->mm_height) : QSize())
    , m_virtualSize(virtualDesktop->size())
    , m_virtualSizeMillimeters(virtualDesktop->physicalSize())
    , m_orientation(Qt::PrimaryOrientation)
    , m_refreshRate(60)
    , m_forcedDpi(-1)
    , m_pixelDensity(1)
    , m_hintStyle(QFontEngine::HintStyle(-1))
    , m_subpixelType(QFontEngine::SubpixelAntialiasingType(-1))
    , m_antialiasingEnabled(-1)
{
    if (connection->hasXRandr()) {
        xcb_randr_select_input(xcb_connection(), screen()->root, true);
        xcb_randr_get_crtc_info_cookie_t crtcCookie =
            xcb_randr_get_crtc_info_unchecked(xcb_connection(), m_crtc, output ? output->timestamp : 0);
        xcb_randr_get_crtc_info_reply_t *crtc =
            xcb_randr_get_crtc_info_reply(xcb_connection(), crtcCookie, NULL);
        if (crtc) {
            updateGeometry(QRect(crtc->x, crtc->y, crtc->width, crtc->height), crtc->rotation);
            updateRefreshRate(crtc->mode);
            free(crtc);
        }
    } else if (xineramaScreenInfo) {
        m_geometry = QRect(xineramaScreenInfo->x_org, xineramaScreenInfo->y_org,
                           xineramaScreenInfo->width, xineramaScreenInfo->height);
        m_availableGeometry = m_geometry & m_virtualDesktop->workArea();
        m_sizeMillimeters = sizeInMillimeters(m_geometry.size(), virtualDpi());
        if (xineramaScreenIdx > -1)
            m_outputName += QLatin1Char('-') + QString::number(xineramaScreenIdx);
    }

    if (m_geometry.isEmpty())
        m_geometry = QRect(QPoint(), m_virtualSize);

    if (m_availableGeometry.isEmpty())
        m_availableGeometry = m_geometry & m_virtualDesktop->workArea();

    if (m_sizeMillimeters.isEmpty())
        m_sizeMillimeters = m_virtualSizeMillimeters;

    readXResources();

    QScopedPointer<xcb_get_window_attributes_reply_t, QScopedPointerPodDeleter> rootAttribs(
        xcb_get_window_attributes_reply(xcb_connection(),
            xcb_get_window_attributes_unchecked(xcb_connection(), screen()->root), NULL));
    const quint32 existingEventMask = rootAttribs.isNull() ? 0 : rootAttribs->your_event_mask;

    const quint32 mask = XCB_CW_EVENT_MASK;
    const quint32 values[] = {
        // XCB_CW_EVENT_MASK
        XCB_EVENT_MASK_ENTER_WINDOW
        | XCB_EVENT_MASK_LEAVE_WINDOW
        | XCB_EVENT_MASK_PROPERTY_CHANGE
        | XCB_EVENT_MASK_STRUCTURE_NOTIFY // for the "MANAGER" atom (system tray notification).
        | existingEventMask // don't overwrite the event mask on the root window
    };

    xcb_change_window_attributes(xcb_connection(), screen()->root, mask, values);

    xcb_get_property_reply_t *reply =
        xcb_get_property_reply(xcb_connection(),
            xcb_get_property_unchecked(xcb_connection(), false, screen()->root,
                             atom(QXcbAtom::_NET_SUPPORTING_WM_CHECK),
                             XCB_ATOM_WINDOW, 0, 1024), NULL);

    if (reply && reply->format == 32 && reply->type == XCB_ATOM_WINDOW) {
        xcb_window_t windowManager = *((xcb_window_t *)xcb_get_property_value(reply));

        if (windowManager != XCB_WINDOW_NONE) {
            xcb_get_property_reply_t *windowManagerReply =
                xcb_get_property_reply(xcb_connection(),
                    xcb_get_property_unchecked(xcb_connection(), false, windowManager,
                                     atom(QXcbAtom::_NET_WM_NAME),
                                     atom(QXcbAtom::UTF8_STRING), 0, 1024), NULL);
            if (windowManagerReply && windowManagerReply->format == 8 && windowManagerReply->type == atom(QXcbAtom::UTF8_STRING)) {
                m_windowManagerName = QString::fromUtf8((const char *)xcb_get_property_value(windowManagerReply), xcb_get_property_value_length(windowManagerReply));
            }

            free(windowManagerReply);
        }
    }
    free(reply);

    const xcb_query_extension_reply_t *sync_reply = xcb_get_extension_data(xcb_connection(), &xcb_sync_id);
    if (!sync_reply || !sync_reply->present)
        m_syncRequestSupported = false;
    else
        m_syncRequestSupported = true;

    xcb_depth_iterator_t depth_iterator =
        xcb_screen_allowed_depths_iterator(screen());

    while (depth_iterator.rem) {
        xcb_depth_t *depth = depth_iterator.data;
        xcb_visualtype_iterator_t visualtype_iterator =
            xcb_depth_visuals_iterator(depth);

        while (visualtype_iterator.rem) {
            xcb_visualtype_t *visualtype = visualtype_iterator.data;
            m_visuals.insert(visualtype->visual_id, *visualtype);
            m_visualDepths.insert(visualtype->visual_id, depth->depth);
            xcb_visualtype_next(&visualtype_iterator);
        }

        xcb_depth_next(&depth_iterator);
    }

    m_cursor = new QXcbCursor(connection, this);
}

QXcbScreen::~QXcbScreen()
{
    delete m_cursor;
}

QString QXcbScreen::getOutputName(xcb_randr_get_output_info_reply_t *outputInfo)
{
    QString name;
    if (outputInfo) {
        name = QString::fromUtf8((const char*)xcb_randr_get_output_info_name(outputInfo),
                                 xcb_randr_get_output_info_name_length(outputInfo));
    } else {
        QByteArray displayName = connection()->displayName();
        int dotPos = displayName.lastIndexOf('.');
        if (dotPos != -1)
            displayName.truncate(dotPos);
        name = QString::fromLocal8Bit(displayName) + QLatin1Char('.')
                + QString::number(m_virtualDesktop->number());
    }
    return name;
}

QWindow *QXcbScreen::topLevelAt(const QPoint &p) const
{
    xcb_window_t root = screen()->root;

    int x = p.x();
    int y = p.y();

    xcb_window_t parent = root;
    xcb_window_t child = root;

    do {
        xcb_translate_coordinates_cookie_t translate_cookie =
            xcb_translate_coordinates_unchecked(xcb_connection(), parent, child, x, y);

        xcb_translate_coordinates_reply_t *translate_reply =
            xcb_translate_coordinates_reply(xcb_connection(), translate_cookie, NULL);

        if (!translate_reply) {
            return 0;
        }

        parent = child;
        child = translate_reply->child;
        x = translate_reply->dst_x;
        y = translate_reply->dst_y;

        free(translate_reply);

        if (!child || child == root)
            return 0;

        QPlatformWindow *platformWindow = connection()->platformWindowFromId(child);
        if (platformWindow)
            return platformWindow->window();
    } while (parent != child);

    return 0;
}

void QXcbScreen::windowShown(QXcbWindow *window)
{
    // Freedesktop.org Startup Notification
    if (!connection()->startupId().isEmpty() && window->window()->isTopLevel()) {
        sendStartupMessage(QByteArrayLiteral("remove: ID=") + connection()->startupId());
        connection()->clearStartupId();
    }
}

QSurfaceFormat QXcbScreen::surfaceFormatFor(const QSurfaceFormat &format) const
{
    const xcb_visualid_t xcb_visualid = connection()->hasDefaultVisualId() ? connection()->defaultVisualId()
                                                                           : screen()->root_visual;
    const xcb_visualtype_t *xcb_visualtype = visualForId(xcb_visualid);

    const int redSize = qPopulationCount(xcb_visualtype->red_mask);
    const int greenSize = qPopulationCount(xcb_visualtype->green_mask);
    const int blueSize = qPopulationCount(xcb_visualtype->blue_mask);

    QSurfaceFormat result = format;

    if (result.redBufferSize() < 0)
        result.setRedBufferSize(redSize);

    if (result.greenBufferSize() < 0)
        result.setGreenBufferSize(greenSize);

    if (result.blueBufferSize() < 0)
        result.setBlueBufferSize(blueSize);

    return result;
}

const xcb_visualtype_t *QXcbScreen::visualForFormat(const QSurfaceFormat &format) const
{
    const xcb_visualtype_t *candidate = nullptr;

    for (const xcb_visualtype_t &xcb_visualtype : m_visuals) {

        const int redSize = qPopulationCount(xcb_visualtype.red_mask);
        const int greenSize = qPopulationCount(xcb_visualtype.green_mask);
        const int blueSize = qPopulationCount(xcb_visualtype.blue_mask);
        const int alphaSize = depthOfVisual(xcb_visualtype.visual_id) - redSize - greenSize - blueSize;

        if (format.redBufferSize() != -1 && redSize != format.redBufferSize())
            continue;

        if (format.greenBufferSize() != -1 && greenSize != format.greenBufferSize())
            continue;

        if (format.blueBufferSize() != -1 && blueSize != format.blueBufferSize())
            continue;

        if (format.alphaBufferSize() != -1 && alphaSize != format.alphaBufferSize())
            continue;

        // Try to find a RGB visual rather than e.g. BGR or GBR
        if (qCountTrailingZeroBits(xcb_visualtype.blue_mask) == 0)
            return &xcb_visualtype;

        // In case we do not find anything we like, just remember the first one
        // and hope for the best:
        if (!candidate)
            candidate = &xcb_visualtype;
    }

    return candidate;
}

void QXcbScreen::sendStartupMessage(const QByteArray &message) const
{
    xcb_window_t rootWindow = root();

    xcb_client_message_event_t ev;
    ev.response_type = XCB_CLIENT_MESSAGE;
    ev.format = 8;
    ev.type = connection()->atom(QXcbAtom::_NET_STARTUP_INFO_BEGIN);
    ev.sequence = 0;
    ev.window = rootWindow;
    int sent = 0;
    int length = message.length() + 1; // include NUL byte
    const char *data = message.constData();
    do {
        if (sent == 20)
            ev.type = connection()->atom(QXcbAtom::_NET_STARTUP_INFO);

        const int start = sent;
        const int numBytes = qMin(length - start, 20);
        memcpy(ev.data.data8, data + start, numBytes);
        xcb_send_event(connection()->xcb_connection(), false, rootWindow, XCB_EVENT_MASK_PROPERTY_CHANGE, (const char *) &ev);

        sent += numBytes;
    } while (sent < length);
}

const xcb_visualtype_t *QXcbScreen::visualForId(xcb_visualid_t visualid) const
{
    QMap<xcb_visualid_t, xcb_visualtype_t>::const_iterator it = m_visuals.find(visualid);
    if (it == m_visuals.constEnd())
        return 0;
    return &*it;
}

quint8 QXcbScreen::depthOfVisual(xcb_visualid_t visualid) const
{
    QMap<xcb_visualid_t, quint8>::const_iterator it = m_visualDepths.find(visualid);
    if (it == m_visualDepths.constEnd())
        return 0;
    return *it;
}

QImage::Format QXcbScreen::format() const
{
    return qt_xcb_imageFormatForVisual(connection(), screen()->root_depth, visualForId(screen()->root_visual));
}

QDpi QXcbScreen::virtualDpi() const
{
    return QDpi(Q_MM_PER_INCH * m_virtualSize.width() / m_virtualSizeMillimeters.width(),
                Q_MM_PER_INCH * m_virtualSize.height() / m_virtualSizeMillimeters.height());
}


QDpi QXcbScreen::logicalDpi() const
{
    static const int overrideDpi = qEnvironmentVariableIntValue("QT_FONT_DPI");
    if (overrideDpi)
        return QDpi(overrideDpi, overrideDpi);

    if (m_forcedDpi > 0) {
        return QDpi(m_forcedDpi, m_forcedDpi);
    }
    return virtualDpi();
}

qreal QXcbScreen::pixelDensity() const
{
    return m_pixelDensity;
}

QPlatformCursor *QXcbScreen::cursor() const
{
    return m_cursor;
}

void QXcbScreen::setOutput(xcb_randr_output_t outputId,
                           xcb_randr_get_output_info_reply_t *outputInfo)
{
    m_output = outputId;
    m_crtc = outputInfo ? outputInfo->crtc : XCB_NONE;
    m_mode = XCB_NONE;
    m_outputName = getOutputName(outputInfo);
    // TODO: Send an event to the QScreen instance that the screen changed its name
}

int QXcbScreen::virtualDesktopNumberStatic(const QScreen *screen)
{
    if (screen && screen->handle())
        return static_cast<const QXcbScreen *>(screen->handle())->screenNumber();

    return 0;
}

/*!
    \brief handle the XCB screen change event and update properties

    On a mobile device, the ideal use case is that the accelerometer would
    drive the orientation. This could be achieved by using QSensors to read the
    accelerometer and adjusting the rotation in QML, or by reading the
    orientation from the QScreen object and doing the same, or in many other
    ways. However, on X we have the XRandR extension, which makes it possible
    to have the whole screen rotated, so that individual apps DO NOT have to
    rotate themselves. Apps could optionally use the
    QScreen::primaryOrientation property to optimize layout though.
    Furthermore, there is no support in X for accelerometer events anyway. So
    it makes more sense on a Linux system running X to just run a daemon which
    monitors the accelerometer and runs xrandr automatically to do the rotation,
    then apps do not have to be aware of it (but probably the window manager
    would resize them accordingly). updateGeometry() is written with this
    design in mind. Therefore the physical geometry, available geometry,
    virtual geometry, orientation and primaryOrientation should all change at
    the same time.  On a system which cannot rotate the whole screen, it would
    be correct for only the orientation (not the primary orientation) to
    change.
*/
void QXcbScreen::handleScreenChange(xcb_randr_screen_change_notify_event_t *change_event)
{
    // No need to do anything when screen rotation did not change - if any
    // xcb output geometry has changed, we will get RRCrtcChangeNotify and
    // RROutputChangeNotify events next
    if (change_event->rotation == m_rotation)
        return;

    m_rotation = change_event->rotation;
    switch (m_rotation) {
    case XCB_RANDR_ROTATION_ROTATE_0: // xrandr --rotate normal
        m_orientation = Qt::LandscapeOrientation;
        m_virtualSize.setWidth(change_event->width);
        m_virtualSize.setHeight(change_event->height);
        m_virtualSizeMillimeters.setWidth(change_event->mwidth);
        m_virtualSizeMillimeters.setHeight(change_event->mheight);
        break;
    case XCB_RANDR_ROTATION_ROTATE_90: // xrandr --rotate left
        m_orientation = Qt::PortraitOrientation;
        m_virtualSize.setWidth(change_event->height);
        m_virtualSize.setHeight(change_event->width);
        m_virtualSizeMillimeters.setWidth(change_event->mheight);
        m_virtualSizeMillimeters.setHeight(change_event->mwidth);
        break;
    case XCB_RANDR_ROTATION_ROTATE_180: // xrandr --rotate inverted
        m_orientation = Qt::InvertedLandscapeOrientation;
        m_virtualSize.setWidth(change_event->width);
        m_virtualSize.setHeight(change_event->height);
        m_virtualSizeMillimeters.setWidth(change_event->mwidth);
        m_virtualSizeMillimeters.setHeight(change_event->mheight);
        break;
    case XCB_RANDR_ROTATION_ROTATE_270: // xrandr --rotate right
        m_orientation = Qt::InvertedPortraitOrientation;
        m_virtualSize.setWidth(change_event->height);
        m_virtualSize.setHeight(change_event->width);
        m_virtualSizeMillimeters.setWidth(change_event->mheight);
        m_virtualSizeMillimeters.setHeight(change_event->mwidth);
        break;
    // We don't need to do anything with these, since QScreen doesn't store reflection state,
    // and Qt-based applications probably don't need to care about it anyway.
    case XCB_RANDR_ROTATION_REFLECT_X: break;
    case XCB_RANDR_ROTATION_REFLECT_Y: break;
    }

    updateGeometry(change_event->timestamp);

    QWindowSystemInterface::handleScreenOrientationChange(QPlatformScreen::screen(), m_orientation);

    QDpi ldpi = logicalDpi();
    QWindowSystemInterface::handleScreenLogicalDotsPerInchChange(QPlatformScreen::screen(), ldpi.first, ldpi.second);
}

void QXcbScreen::updateGeometry(xcb_timestamp_t timestamp)
{
    if (!connection()->hasXRandr())
        return;

    xcb_randr_get_crtc_info_cookie_t crtcCookie =
        xcb_randr_get_crtc_info_unchecked(xcb_connection(), m_crtc, timestamp);
    xcb_randr_get_crtc_info_reply_t *crtc =
        xcb_randr_get_crtc_info_reply(xcb_connection(), crtcCookie, NULL);
    if (crtc) {
        updateGeometry(QRect(crtc->x, crtc->y, crtc->width, crtc->height), crtc->rotation);
        free(crtc);
    }
}

void QXcbScreen::updateGeometry(const QRect &geom, uint8_t rotation)
{
    QRect xGeometry = geom;
    switch (rotation) {
    case XCB_RANDR_ROTATION_ROTATE_0: // xrandr --rotate normal
        m_orientation = Qt::LandscapeOrientation;
        m_sizeMillimeters = m_outputSizeMillimeters;
        break;
    case XCB_RANDR_ROTATION_ROTATE_90: // xrandr --rotate left
        m_orientation = Qt::PortraitOrientation;
        m_sizeMillimeters = m_outputSizeMillimeters.transposed();
        break;
    case XCB_RANDR_ROTATION_ROTATE_180: // xrandr --rotate inverted
        m_orientation = Qt::InvertedLandscapeOrientation;
        m_sizeMillimeters = m_outputSizeMillimeters;
        break;
    case XCB_RANDR_ROTATION_ROTATE_270: // xrandr --rotate right
        m_orientation = Qt::InvertedPortraitOrientation;
        m_sizeMillimeters = m_outputSizeMillimeters.transposed();
        break;
    }

    // It can be that physical size is unknown while virtual size
    // is known (probably back-calculated from DPI and resolution),
    // e.g. on VNC or with some hardware.
    if (m_sizeMillimeters.isEmpty())
        m_sizeMillimeters = sizeInMillimeters(xGeometry.size(), virtualDpi());

    qreal dpi = xGeometry.width() / physicalSize().width() * qreal(25.4);
    m_pixelDensity = qRound(dpi/96);
    m_geometry = QRect(xGeometry.topLeft(), xGeometry.size());
    m_availableGeometry = xGeometry & m_virtualDesktop->workArea();
    QWindowSystemInterface::handleScreenGeometryChange(QPlatformScreen::screen(), m_geometry, m_availableGeometry);
}

void QXcbScreen::updateAvailableGeometry()
{
    QRect availableGeometry = m_geometry & m_virtualDesktop->workArea();
    if (m_availableGeometry != availableGeometry) {
        m_availableGeometry = availableGeometry;
        QWindowSystemInterface::handleScreenGeometryChange(QPlatformScreen::screen(), m_geometry, m_availableGeometry);
    }
}

void QXcbScreen::updateRefreshRate(xcb_randr_mode_t mode)
{
    if (!connection()->hasXRandr())
        return;

    if (m_mode == mode)
        return;

    // we can safely use get_screen_resources_current here, because in order to
    // get here, we must have called get_screen_resources before
    xcb_randr_get_screen_resources_current_cookie_t resourcesCookie =
        xcb_randr_get_screen_resources_current_unchecked(xcb_connection(), screen()->root);
    xcb_randr_get_screen_resources_current_reply_t *resources =
        xcb_randr_get_screen_resources_current_reply(xcb_connection(), resourcesCookie, NULL);
    if (resources) {
        xcb_randr_mode_info_iterator_t modesIter =
            xcb_randr_get_screen_resources_current_modes_iterator(resources);
        for (; modesIter.rem; xcb_randr_mode_info_next(&modesIter)) {
            xcb_randr_mode_info_t *modeInfo = modesIter.data;
            if (modeInfo->id == mode) {
                const uint32_t dotCount = modeInfo->htotal * modeInfo->vtotal;
                m_refreshRate = (dotCount != 0) ? modeInfo->dot_clock / dotCount : 0;
                m_mode = mode;
                break;
            }
        }

        free(resources);
        QWindowSystemInterface::handleScreenRefreshRateChange(QPlatformScreen::screen(), m_refreshRate);
    }
}

QPixmap QXcbScreen::grabWindow(WId window, int x, int y, int width, int height) const
{
    if (width == 0 || height == 0)
        return QPixmap();

    // TODO: handle multiple screens
    QXcbScreen *screen = const_cast<QXcbScreen *>(this);
    xcb_window_t root = screen->root();

    if (window == 0)
        window = root;

    xcb_get_geometry_cookie_t geometry_cookie = xcb_get_geometry_unchecked(xcb_connection(), window);

    xcb_get_geometry_reply_t *reply =
        xcb_get_geometry_reply(xcb_connection(), geometry_cookie, NULL);

    if (!reply) {
        return QPixmap();
    }

    if (width < 0)
        width = reply->width - x;
    if (height < 0)
        height = reply->height - y;

    geometry_cookie = xcb_get_geometry_unchecked(xcb_connection(), root);
    xcb_get_geometry_reply_t *root_reply =
        xcb_get_geometry_reply(xcb_connection(), geometry_cookie, NULL);

    if (!root_reply) {
        free(reply);
        return QPixmap();
    }

    if (reply->depth == root_reply->depth) {
        // if the depth of the specified window and the root window are the
        // same, grab pixels from the root window (so that we get the any
        // overlapping windows and window manager frames)

        // map x and y to the root window
        xcb_translate_coordinates_cookie_t translate_cookie =
            xcb_translate_coordinates_unchecked(xcb_connection(), window, root, x, y);

        xcb_translate_coordinates_reply_t *translate_reply =
            xcb_translate_coordinates_reply(xcb_connection(), translate_cookie, NULL);

        if (!translate_reply) {
            free(reply);
            free(root_reply);
            return QPixmap();
        }

        x = translate_reply->dst_x;
        y = translate_reply->dst_y;

        window = root;

        free(translate_reply);
        free(reply);
        reply = root_reply;
    } else {
        free(root_reply);
        root_reply = 0;
    }

    xcb_get_window_attributes_reply_t *attributes_reply =
        xcb_get_window_attributes_reply(xcb_connection(), xcb_get_window_attributes_unchecked(xcb_connection(), window), NULL);

    if (!attributes_reply) {
        free(reply);
        return QPixmap();
    }

    const xcb_visualtype_t *visual = screen->visualForId(attributes_reply->visual);
    free(attributes_reply);

    xcb_pixmap_t pixmap = xcb_generate_id(xcb_connection());
    xcb_create_pixmap(xcb_connection(), reply->depth, pixmap, window, width, height);

    uint32_t gc_value_mask = XCB_GC_SUBWINDOW_MODE;
    uint32_t gc_value_list[] = { XCB_SUBWINDOW_MODE_INCLUDE_INFERIORS };

    xcb_gcontext_t gc = xcb_generate_id(xcb_connection());
    xcb_create_gc(xcb_connection(), gc, pixmap, gc_value_mask, gc_value_list);

    xcb_copy_area(xcb_connection(), window, pixmap, gc, x, y, 0, 0, width, height);

    QPixmap result = qt_xcb_pixmapFromXPixmap(connection(), pixmap, width, height, reply->depth, visual);

    free(reply);
    xcb_free_gc(xcb_connection(), gc);
    xcb_free_pixmap(xcb_connection(), pixmap);

    return result;
}

static bool parseXftInt(const QByteArray& stringValue, int *value)
{
    Q_ASSERT(value != 0);
    bool ok;
    *value = stringValue.toInt(&ok);
    return ok;
}

static QFontEngine::HintStyle parseXftHintStyle(const QByteArray& stringValue)
{
    if (stringValue == "hintfull")
        return QFontEngine::HintFull;
    else if (stringValue == "hintnone")
        return QFontEngine::HintNone;
    else if (stringValue == "hintmedium")
        return QFontEngine::HintMedium;
    else if (stringValue == "hintslight")
        return QFontEngine::HintLight;

    return QFontEngine::HintStyle(-1);
}

static QFontEngine::SubpixelAntialiasingType parseXftRgba(const QByteArray& stringValue)
{
    if (stringValue == "none")
        return QFontEngine::Subpixel_None;
    else if (stringValue == "rgb")
        return QFontEngine::Subpixel_RGB;
    else if (stringValue == "bgr")
        return QFontEngine::Subpixel_BGR;
    else if (stringValue == "vrgb")
        return QFontEngine::Subpixel_VRGB;
    else if (stringValue == "vbgr")
        return QFontEngine::Subpixel_VBGR;

    return QFontEngine::SubpixelAntialiasingType(-1);
}

bool QXcbScreen::xResource(const QByteArray &identifier,
                           const QByteArray &expectedIdentifier,
                           QByteArray& stringValue)
{
    if (identifier.startsWith(expectedIdentifier)) {
        stringValue = identifier.mid(expectedIdentifier.size());
        return true;
    }
    return false;
}

void QXcbScreen::readXResources()
{
    int offset = 0;
    QByteArray resources;
    while(1) {
        xcb_get_property_reply_t *reply =
            xcb_get_property_reply(xcb_connection(),
                xcb_get_property_unchecked(xcb_connection(), false, screen()->root,
                                 XCB_ATOM_RESOURCE_MANAGER,
                                 XCB_ATOM_STRING, offset/4, 8192), NULL);
        bool more = false;
        if (reply && reply->format == 8 && reply->type == XCB_ATOM_STRING) {
            resources += QByteArray((const char *)xcb_get_property_value(reply), xcb_get_property_value_length(reply));
            offset += xcb_get_property_value_length(reply);
            more = reply->bytes_after != 0;
        }

        if (reply)
            free(reply);

        if (!more)
            break;
    }

    QList<QByteArray> split = resources.split('\n');
    for (int i = 0; i < split.size(); ++i) {
        const QByteArray &r = split.at(i);
        int value;
        QByteArray stringValue;
        if (xResource(r, "Xft.dpi:\t", stringValue)) {
            if (parseXftInt(stringValue, &value))
                m_forcedDpi = value;
        } else if (xResource(r, "Xft.hintstyle:\t", stringValue)) {
            m_hintStyle = parseXftHintStyle(stringValue);
        } else if (xResource(r, "Xft.antialias:\t", stringValue)) {
            if (parseXftInt(stringValue, &value))
                m_antialiasingEnabled = value;
        } else if (xResource(r, "Xft.rgba:\t", stringValue)) {
            m_subpixelType = parseXftRgba(stringValue);
        }
    }
}

QXcbXSettings *QXcbScreen::xSettings() const
{
    return m_virtualDesktop->xSettings();
}

static inline void formatRect(QDebug &debug, const QRect r)
{
    debug << r.width() << 'x' << r.height()
        << forcesign << r.x() << r.y() << noforcesign;
}

static inline void formatSizeF(QDebug &debug, const QSizeF s)
{
    debug << s.width() << 'x' << s.height() << "mm";
}

QDebug operator<<(QDebug debug, const QXcbScreen *screen)
{
    const QDebugStateSaver saver(debug);
    debug.nospace();
    debug << "QXcbScreen(" << (const void *)screen;
    if (screen) {
        debug << fixed << qSetRealNumberPrecision(1);
        debug << ", name=" << screen->name();
        debug << ", geometry=";
        formatRect(debug, screen->geometry());
        debug << ", availableGeometry=";
        formatRect(debug, screen->availableGeometry());
        debug << ", devicePixelRatio=" << screen->devicePixelRatio();
        debug << ", logicalDpi=" << screen->logicalDpi();
        debug << ", physicalSize=";
        formatSizeF(debug, screen->physicalSize());
        // TODO 5.6 if (debug.verbosity() > 2) {
        debug << ", screenNumber=" << screen->screenNumber();
        debug << ", virtualSize=" << screen->virtualSize().width() << 'x' << screen->virtualSize().height() << " (";
        formatSizeF(debug, screen->virtualSize());
        debug << "), orientation=" << screen->orientation();
        debug << ", depth=" << screen->depth();
        debug << ", refreshRate=" << screen->refreshRate();
        debug << ", root=" << hex << screen->root();
        debug << ", windowManagerName=" << screen->windowManagerName();
    }
    debug << ')';
    return debug;
}

QT_END_NAMESPACE
