// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#pragma once

#include <qpa/qplatformscreen.h>
#include <qpa/qplatformscreen_p.h>
#include <QtCore/QString>

#include <xcb/xcb.h>
#include <xcb/randr.h>
#include <xcb/xfixes.h>

#include "qxcbobject.h"

#include <private/qfontengine_p.h>

#include <QtGui/private/qedidparser_p.h>

#include <memory>

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

    void updateWorkArea();
    QRect availableGeometry(const QRect &screenGeometry) const;

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

    bool setDpiFromXSettings(const QVariant &property);

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

    friend class QXcbConnection;
};

class Q_XCB_EXPORT QXcbScreen : public QXcbObject, public QPlatformScreen
                              , public QNativeInterface::Private::QXcbScreen
{
public:
    QXcbScreen(QXcbConnection *connection, QXcbVirtualDesktop *virtualDesktop,
               xcb_randr_output_t outputId, xcb_randr_get_output_info_reply_t *outputInfo);
    QXcbScreen(QXcbConnection *connection, QXcbVirtualDesktop *virtualDesktop,
               xcb_randr_monitor_info_t *monitorInfo, xcb_timestamp_t timestamp = XCB_NONE);
    ~QXcbScreen();

    QString getOutputName(xcb_randr_get_output_info_reply_t *outputInfo);
    QString getName(xcb_randr_monitor_info_t *monitorInfo);

    QPixmap grabWindow(WId window, int x, int y, int width, int height) const override;

    QWindow *topLevelAt(const QPoint &point) const override;

    QString manufacturer() const override;
    QString model() const override;
    QString serialNumber() const override;

    QRect geometry() const override { return m_geometry; }
    QRect availableGeometry() const override;
    int depth() const override { return screen()->root_depth; }
    QImage::Format format() const override;
    QColorSpace colorSpace() const override { return m_colorSpace; }
    QSizeF physicalSize() const override { return m_sizeMillimeters; }
    QDpi logicalDpi() const override;
    QDpi logicalBaseDpi() const override { return QDpi(96, 96); }
    QPlatformCursor *cursor() const override;
    qreal refreshRate() const override { return m_refreshRate; }
    Qt::ScreenOrientation orientation() const override { return m_orientation; }
    QList<QPlatformScreen *> virtualSiblings() const override { return m_virtualDesktop->screens(); }
    QXcbVirtualDesktop *virtualDesktop() const { return m_virtualDesktop; }

    void setPrimary(bool primary) { m_primary = primary; }
    bool isPrimary() const { return m_primary; }

    int screenNumber() const { return m_virtualDesktop->number(); }
    int virtualDesktopNumber() const override { return screenNumber(); }

    xcb_screen_t *screen() const { return m_virtualDesktop->screen(); }
    xcb_window_t root() const { return screen()->root; }
    xcb_randr_output_t output() const { return m_output; }
    xcb_randr_crtc_t crtc() const { return m_crtc; }
    xcb_randr_mode_t mode() const { return m_mode; }

    QList<xcb_randr_output_t> outputs() const { return m_outputs; }
    QList<xcb_randr_crtc_t> crtcs() const { return m_crtcs; }

    void setOutput(xcb_randr_output_t outputId,
                   xcb_randr_get_output_info_reply_t *outputInfo);
    void setCrtc(xcb_randr_crtc_t crtc) { m_crtc = crtc; }
    void setMonitor(xcb_randr_monitor_info_t *monitorInfo, xcb_timestamp_t timestamp = XCB_NONE);
    QString defaultName();
    bool isPrimaryInXScreen();

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

    void updateColorSpaceAndEdid();
    QByteArray getOutputProperty(xcb_atom_t atom) const;
    QByteArray getEdid() const;

    QXcbVirtualDesktop *m_virtualDesktop;
    xcb_randr_monitor_info_t *m_monitor;
    xcb_randr_output_t m_output;
    xcb_randr_crtc_t m_crtc;
    xcb_randr_mode_t m_mode = XCB_NONE;
    bool m_primary = false;

    bool m_singlescreen = false;

    QList<xcb_randr_output_t> m_outputs;
    QList<xcb_randr_crtc_t> m_crtcs;

    QString m_outputName;
    QSizeF m_outputSizeMillimeters;
    QSizeF m_sizeMillimeters;
    QRect m_geometry;
    QRect m_availableGeometry;
    QColorSpace m_colorSpace;
    Qt::ScreenOrientation m_orientation = Qt::PrimaryOrientation;
    std::unique_ptr<QXcbCursor> m_cursor;
    qreal m_refreshRate = 60.0;
    QEdidParser m_edid;

    friend class QXcbConnection;
    friend class QXcbVirtualDesktop;
};

#ifndef QT_NO_DEBUG_STREAM
Q_GUI_EXPORT QDebug operator<<(QDebug, const QXcbScreen *);
#endif

QT_END_NAMESPACE
