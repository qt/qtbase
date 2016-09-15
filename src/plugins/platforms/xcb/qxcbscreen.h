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
    xcb_window_t root() const { return m_screen->root; }
    QXcbScreen *screenAt(const QPoint &pos) const;

    QList<QPlatformScreen *> screens() const { return m_screens; }
    void setScreens(QList<QPlatformScreen *> sl) { m_screens = sl; }
    void removeScreen(QPlatformScreen *s) { m_screens.removeOne(s); }
    void addScreen(QPlatformScreen *s);
    void setPrimaryScreen(QPlatformScreen *s);

    QXcbXSettings *xSettings() const;

    bool compositingActive() const;

    QRect workArea() const { return m_workArea; }
    void updateWorkArea();

    void handleXFixesSelectionNotify(xcb_xfixes_selection_notify_event_t *notify_event);
    void subscribeToXFixesSelectionNotify();

private:
    QRect getWorkArea() const;

    xcb_screen_t *m_screen;
    int m_number;
    QList<QPlatformScreen *> m_screens;

    QXcbXSettings *m_xSettings;
    xcb_atom_t m_net_wm_cm_atom;
    bool m_compositingActive;

    QRect m_workArea;
};

class Q_XCB_EXPORT QXcbScreen : public QXcbObject, public QPlatformScreen
{
public:
    QXcbScreen(QXcbConnection *connection, QXcbVirtualDesktop *virtualDesktop,
               xcb_randr_output_t outputId, xcb_randr_get_output_info_reply_t *outputInfo,
               const xcb_xinerama_screen_info_t *xineramaScreenInfo = Q_NULLPTR, int xineramaScreenIdx = -1);
    ~QXcbScreen();

    QString getOutputName(xcb_randr_get_output_info_reply_t *outputInfo);

    QPixmap grabWindow(WId window, int x, int y, int width, int height) const Q_DECL_OVERRIDE;

    QWindow *topLevelAt(const QPoint &point) const Q_DECL_OVERRIDE;

    QRect geometry() const Q_DECL_OVERRIDE { return m_geometry; }
    QRect availableGeometry() const Q_DECL_OVERRIDE {return m_availableGeometry;}
    int depth() const Q_DECL_OVERRIDE { return screen()->root_depth; }
    QImage::Format format() const Q_DECL_OVERRIDE;
    QSizeF physicalSize() const Q_DECL_OVERRIDE { return m_sizeMillimeters; }
    QSize virtualSize() const { return m_virtualSize; }
    QSizeF physicalVirtualSize() const { return m_virtualSizeMillimeters; }
    QDpi virtualDpi() const;
    QDpi logicalDpi() const Q_DECL_OVERRIDE;
    qreal pixelDensity() const Q_DECL_OVERRIDE;
    QPlatformCursor *cursor() const Q_DECL_OVERRIDE;
    qreal refreshRate() const Q_DECL_OVERRIDE { return m_refreshRate; }
    Qt::ScreenOrientation orientation() const Q_DECL_OVERRIDE { return m_orientation; }
    QList<QPlatformScreen *> virtualSiblings() const Q_DECL_OVERRIDE { return m_virtualDesktop->screens(); }
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
    QString windowManagerName() const { return m_windowManagerName; }
    bool syncRequestSupported() const { return m_syncRequestSupported; }

    QSurfaceFormat surfaceFormatFor(const QSurfaceFormat &format) const;

    const xcb_visualtype_t *visualForFormat(const QSurfaceFormat &format) const;
    const xcb_visualtype_t *visualForId(xcb_visualid_t) const;
    quint8 depthOfVisual(xcb_visualid_t) const;

    QString name() const Q_DECL_OVERRIDE { return m_outputName; }

    void handleScreenChange(xcb_randr_screen_change_notify_event_t *change_event);
    void updateGeometry(const QRect &geom, uint8_t rotation);
    void updateGeometry(xcb_timestamp_t timestamp = XCB_TIME_CURRENT_TIME);
    void updateAvailableGeometry();
    void updateRefreshRate(xcb_randr_mode_t mode);

    void readXResources();

    QFontEngine::HintStyle hintStyle() const { return m_hintStyle; }
    QFontEngine::SubpixelAntialiasingType subpixelType() const { return m_subpixelType; }
    int antialiasingEnabled() const { return m_antialiasingEnabled; }

    QXcbXSettings *xSettings() const;

private:
    static bool xResource(const QByteArray &identifier,
                          const QByteArray &expectedIdentifier,
                          QByteArray &stringValue);
    void sendStartupMessage(const QByteArray &message) const;

    QXcbVirtualDesktop *m_virtualDesktop;
    xcb_randr_output_t m_output;
    xcb_randr_crtc_t m_crtc;
    xcb_randr_mode_t m_mode;
    bool m_primary;
    uint8_t m_rotation;

    QString m_outputName;
    QSizeF m_outputSizeMillimeters;
    QSizeF m_sizeMillimeters;
    QRect m_geometry;
    QRect m_availableGeometry;
    QSize m_virtualSize;
    QSizeF m_virtualSizeMillimeters;
    Qt::ScreenOrientation m_orientation;
    QString m_windowManagerName;
    bool m_syncRequestSupported;
    QMap<xcb_visualid_t, xcb_visualtype_t> m_visuals;
    QMap<xcb_visualid_t, quint8> m_visualDepths;
    QXcbCursor *m_cursor;
    int m_refreshRate;
    int m_forcedDpi;
    int m_pixelDensity;
    QFontEngine::HintStyle m_hintStyle;
    QFontEngine::SubpixelAntialiasingType m_subpixelType;
    int m_antialiasingEnabled;
};

#ifndef QT_NO_DEBUG_STREAM
Q_GUI_EXPORT QDebug operator<<(QDebug, const QXcbScreen *);
#endif

QT_END_NAMESPACE

#endif
