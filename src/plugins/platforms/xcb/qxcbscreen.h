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

#ifndef QXCBSCREEN_H
#define QXCBSCREEN_H

#include <qpa/qplatformscreen.h>
#include <QtCore/QString>

#include <xcb/xcb.h>
#include <xcb/randr.h>
#include <xcb/xfixes.h>
#include <xcb/xinerama.h>

#include "qxcbobject.h"
#include "qxcbscreen.h"

#include <private/qfontengine_p.h>

#include <QtEdidSupport/private/qedidparser_p.h>

QT_BEGIN_NAMESPACE

class QXcbConnection;
class QXcbCursor;
class QXcbXSettings;
#ifndef QT_NO_DEBUG_STREAM
class QDebug;
#endif

class QXcbVirtualDesktop : public QXcbObject
{
public:
    QXcbVirtualDesktop(QXcbConnection *connection, xcb_screen_t *screen, int number);
    ~QXcbVirtualDesktop();

    xcb_screen_t *screen() const { return m_screen; }
    int number() const { return m_number; }
    QSize size() const { return QSize(m_screen->width_in_pixels, m_screen->height_in_pixels); }
    QSize physicalSize() const { return QSize(m_screen->width_in_millimeters, m_screen->height_in_millimeters); }
    QDpi dpi() const;
    xcb_window_t root() const { return m_screen->root; }
    QXcbScreen *screenAt(const QPoint &pos) const;

    QList<QPlatformScreen *> screens() const { return m_screens; }
    void setScreens(QList<QPlatformScreen *> &&sl) { m_screens = std::move(sl); }
    void removeScreen(QPlatformScreen *s) { m_screens.removeOne(s); }
    void addScreen(QPlatformScreen *s);
    void setPrimaryScreen(QPlatformScreen *s);

    QXcbXSettings *xSettings() const;

    bool compositingActive() const;

    QRect workArea() const { return m_workArea; }
    void updateWorkArea();

    void handleXFixesSelectionNotify(xcb_xfixes_selection_notify_event_t *notify_event);
    void subscribeToXFixesSelectionNotify();

    void handleScreenChange(xcb_randr_screen_change_notify_event_t *change_event);

    int forcedDpi() const { return m_forcedDpi; }
    QFontEngine::HintStyle hintStyle() const { return m_hintStyle; }
    QFontEngine::SubpixelAntialiasingType subpixelType() const { return m_subpixelType; }
    int antialiasingEnabled() const { return m_antialiasingEnabled; }

    QString windowManagerName() const { return m_windowManagerName; }

    QSurfaceFormat surfaceFormatFor(const QSurfaceFormat &format) const;

    const xcb_visualtype_t *visualForFormat(const QSurfaceFormat &format) const;
    const xcb_visualtype_t *visualForId(xcb_visualid_t) const;
    quint8 depthOfVisual(xcb_visualid_t) const;
    xcb_colormap_t colormapForVisual(xcb_visualid_t) const;

private:
    QRect getWorkArea() const;

    static bool xResource(const QByteArray &identifier,
                          const QByteArray &expectedIdentifier,
                          QByteArray &stringValue);
    void readXResources();

    xcb_screen_t *m_screen;
    const int m_number;
    QList<QPlatformScreen *> m_screens;

    QXcbXSettings *m_xSettings = nullptr;
    xcb_atom_t m_net_wm_cm_atom = 0;
    bool m_compositingActive = false;

    QRect m_workArea;

    int m_forcedDpi = -1;
    QFontEngine::HintStyle m_hintStyle = QFontEngine::HintStyle(-1);
    QFontEngine::SubpixelAntialiasingType m_subpixelType = QFontEngine::SubpixelAntialiasingType(-1);
    int m_antialiasingEnabled = -1;
    QString m_windowManagerName;
    QMap<xcb_visualid_t, xcb_visualtype_t> m_visuals;
    QMap<xcb_visualid_t, quint8> m_visualDepths;
    mutable QMap<xcb_visualid_t, xcb_colormap_t> m_visualColormaps;
    uint16_t m_rotation = 0;
};

class Q_XCB_EXPORT QXcbScreen : public QXcbObject, public QPlatformScreen
{
public:
    QXcbScreen(QXcbConnection *connection, QXcbVirtualDesktop *virtualDesktop,
               xcb_randr_output_t outputId, xcb_randr_get_output_info_reply_t *outputInfo,
               const xcb_xinerama_screen_info_t *xineramaScreenInfo = nullptr, int xineramaScreenIdx = -1);
    ~QXcbScreen();

    QString getOutputName(xcb_randr_get_output_info_reply_t *outputInfo);

    QPixmap grabWindow(WId window, int x, int y, int width, int height) const override;

    QWindow *topLevelAt(const QPoint &point) const override;

    QString manufacturer() const override;
    QString model() const override;
    QString serialNumber() const override;

    QRect geometry() const override { return m_geometry; }
    QRect availableGeometry() const override;
    int depth() const override { return screen()->root_depth; }
    QImage::Format format() const override;
    QSizeF physicalSize() const override { return m_sizeMillimeters; }
    QDpi logicalDpi() const override;
    QDpi logicalBaseDpi() const override { return QDpi(96, 96); };
    QPlatformCursor *cursor() const override;
    qreal refreshRate() const override { return m_refreshRate; }
    Qt::ScreenOrientation orientation() const override { return m_orientation; }
    QList<QPlatformScreen *> virtualSiblings() const override { return m_virtualDesktop->screens(); }
    QXcbVirtualDesktop *virtualDesktop() const { return m_virtualDesktop; }

    void setPrimary(bool primary) { m_primary = primary; }
    bool isPrimary() const { return m_primary; }

    int screenNumber() const { return m_virtualDesktop->number(); }
    static int virtualDesktopNumberStatic(const QScreen *screen);

    xcb_screen_t *screen() const { return m_virtualDesktop->screen(); }
    xcb_window_t root() const { return screen()->root; }
    xcb_randr_output_t output() const { return m_output; }
    xcb_randr_crtc_t crtc() const { return m_crtc; }
    xcb_randr_mode_t mode() const { return m_mode; }

    void setOutput(xcb_randr_output_t outputId,
                   xcb_randr_get_output_info_reply_t *outputInfo);
    void setCrtc(xcb_randr_crtc_t crtc) { m_crtc = crtc; }

    void windowShown(QXcbWindow *window);
    QString windowManagerName() const { return m_virtualDesktop->windowManagerName(); }

    QSurfaceFormat surfaceFormatFor(const QSurfaceFormat &format) const;

    const xcb_visualtype_t *visualForFormat(const QSurfaceFormat &format) const { return m_virtualDesktop->visualForFormat(format); }
    const xcb_visualtype_t *visualForId(xcb_visualid_t visualid) const;
    xcb_colormap_t colormapForVisual(xcb_visualid_t visualid) const { return m_virtualDesktop->colormapForVisual(visualid); }
    quint8 depthOfVisual(xcb_visualid_t visualid) const { return m_virtualDesktop->depthOfVisual(visualid); }

    QString name() const override { return m_outputName; }

    void updateGeometry(const QRect &geometry, uint8_t rotation);
    void updateGeometry(xcb_timestamp_t timestamp = XCB_TIME_CURRENT_TIME);
    void updateAvailableGeometry();
    void updateRefreshRate(xcb_randr_mode_t mode);

    QFontEngine::HintStyle hintStyle() const { return m_virtualDesktop->hintStyle(); }
    QFontEngine::SubpixelAntialiasingType subpixelType() const { return m_virtualDesktop->subpixelType(); }
    int antialiasingEnabled() const { return m_virtualDesktop->antialiasingEnabled(); }

    QXcbXSettings *xSettings() const;

private:
    void sendStartupMessage(const QByteArray &message) const;
    int forcedDpi() const;

    QByteArray getOutputProperty(xcb_atom_t atom) const;
    QByteArray getEdid() const;

    QXcbVirtualDesktop *m_virtualDesktop;
    xcb_randr_output_t m_output;
    xcb_randr_crtc_t m_crtc;
    xcb_randr_mode_t m_mode = XCB_NONE;
    bool m_primary = false;

    QString m_outputName;
    QSizeF m_outputSizeMillimeters;
    QSizeF m_sizeMillimeters;
    QRect m_geometry;
    QRect m_availableGeometry;
    Qt::ScreenOrientation m_orientation = Qt::PrimaryOrientation;
    QXcbCursor *m_cursor;
    qreal m_refreshRate = 60.0;
    QEdidParser m_edid;
};

#ifndef QT_NO_DEBUG_STREAM
Q_GUI_EXPORT QDebug operator<<(QDebug, const QXcbScreen *);
#endif

QT_END_NAMESPACE

#endif
