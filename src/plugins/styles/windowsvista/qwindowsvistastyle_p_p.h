// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#ifndef QWINDOWSVISTASTYLE_P_P_H
#define QWINDOWSVISTASTYLE_P_P_H

//
//  W A R N I N G
//  -------------
//
// This file is not part of the Qt API.  It exists for the convenience
// of qapplication_*.cpp, qwidget*.cpp and qfiledialog.cpp.  This header
// file may change from version to version without notice, or even be removed.
//
// We mean it.
//

#include <QtWidgets/private/qtwidgetsglobal_p.h>
#include "qwindowsvistastyle_p.h"
#include "qwindowsthemedata_p.h"
#include <private/qpaintengine_raster_p.h>
#include <qpaintengine.h>
#include <qwidget.h>
#include <qapplication.h>
#include <qpixmapcache.h>
#include <qstyleoption.h>
#include <QtWidgets/private/qwindowsstyle_p_p.h>
#include <qmap.h>

#if QT_CONFIG(pushbutton)
#include <qpushbutton.h>
#endif
#include <qradiobutton.h>
#if QT_CONFIG(lineedit)
#include <qlineedit.h>
#endif
#include <qgroupbox.h>
#if QT_CONFIG(toolbutton)
#include <qtoolbutton.h>
#endif
#if QT_CONFIG(spinbox)
#include <qspinbox.h>
#endif
#if QT_CONFIG(toolbar)
#include <qtoolbar.h>
#endif
#if QT_CONFIG(combobox)
#include <qcombobox.h>
#endif
#if QT_CONFIG(scrollbar)
#include <qscrollbar.h>
#endif
#if QT_CONFIG(progressbar)
#include <qprogressbar.h>
#endif
#if QT_CONFIG(dockwidget)
#include <qdockwidget.h>
#endif
#if QT_CONFIG(listview)
#include <qlistview.h>
#endif
#if QT_CONFIG(treeview)
#include <qtreeview.h>
#endif
#include <qtextedit.h>
#include <qmessagebox.h>
#if QT_CONFIG(dialogbuttonbox)
#include <qdialogbuttonbox.h>
#endif
#include <qinputdialog.h>
#if QT_CONFIG(tableview)
#include <qtableview.h>
#endif
#include <qdatetime.h>
#if QT_CONFIG(commandlinkbutton)
#include <qcommandlinkbutton.h>
#endif
#include <qlabel.h>
#include <qheaderview.h>
#include <uxtheme.h>

QT_BEGIN_NAMESPACE

class QWindowsVistaStylePrivate : public QWindowsStylePrivate
{
    Q_DECLARE_PUBLIC(QWindowsVistaStyle)

public:
    enum Theme {
        ButtonTheme,
        ComboboxTheme,
        EditTheme,
        HeaderTheme,
        ListViewTheme,
        MenuTheme,
        ProgressTheme,
        RebarTheme,
        ScrollBarTheme,
        SpinTheme,
        TabTheme,
        TaskDialogTheme,
        ToolBarTheme,
        ToolTipTheme,
        TrackBarTheme,
        WindowTheme,
        StatusTheme,
        VistaTreeViewTheme, // arrow shape treeview indicators (Vista) obtained from "explorer" theme.
        NThemes
    };

    QWindowsVistaStylePrivate()
    { init(); }

    ~QWindowsVistaStylePrivate()
    { cleanup(); }

    static HTHEME createTheme(int theme, HWND hwnd);
    static QString themeName(int theme);
    static inline bool hasTheme(int theme) { return theme >= 0 && theme < NThemes && m_themes[theme]; }
    static bool isItemViewDelegateLineEdit(const QWidget *widget);
    static int pixelMetricFromSystemDp(QStyle::PixelMetric pm, const QStyleOption *option = nullptr, const QWidget *widget = nullptr);
    static int fixedPixelMetric(QStyle::PixelMetric pm);
    static bool isLineEditBaseColorSet(const QStyleOption *option, const QWidget *widget);
    static HWND winId(const QWidget *widget);
    static bool useVista(bool update = false);
    static QBackingStore *backingStoreForWidget(const QWidget *widget);
    static HDC hdcForWidgetBackingStore(const QWidget *widget);

    void init(bool force = false);
    void cleanup(bool force = false);
    void cleanupHandleMap();

    HBITMAP buffer(int w = 0, int h = 0);
    HDC bufferHDC()
    { return bufferDC; }

    bool isTransparent(QWindowsThemeData &QWindowsThemeData);
    QRegion region(QWindowsThemeData &QWindowsThemeData);

    bool drawBackground(QWindowsThemeData &QWindowsThemeData, qreal correctionFactor = 1);
    bool drawBackgroundThruNativeBuffer(QWindowsThemeData &QWindowsThemeData, qreal aditionalDevicePixelRatio, qreal correctionFactor);
    bool drawBackgroundDirectly(HDC dc, QWindowsThemeData &QWindowsThemeData, qreal aditionalDevicePixelRatio);

    bool hasAlphaChannel(const QRect &rect);
    bool fixAlphaChannel(const QRect &rect);
    bool swapAlphaChannel(const QRect &rect, bool allPixels = false);

    QRgb groupBoxTextColor = 0;
    QRgb groupBoxTextColorDisabled = 0;
    QRgb sliderTickColor = 0;
    bool hasInitColors = false;
    QIcon dockFloat, dockClose;

    QTime animationTime() const;
    bool transitionsEnabled() const;

    static HTHEME openThemeForPrimaryScreenDpi(HWND hwnd, const wchar_t *name);

private:
    static bool initVistaTreeViewTheming();
    static void cleanupVistaTreeViewTheming();

    static QBasicAtomicInt ref;
    static bool useVistaTheme;

    QHash<ThemeMapKey, ThemeMapData> alphaCache;
    HDC bufferDC = nullptr;
    HBITMAP bufferBitmap = nullptr;
    HBITMAP nullBitmap = nullptr;
    uchar *bufferPixels = nullptr;
    int bufferW = 0;
    int bufferH = 0;

    static HWND m_vistaTreeViewHelper;
    static HTHEME m_themes[NThemes];
};

QT_END_NAMESPACE

#endif // QWINDOWSVISTASTYLE_P_P_H
