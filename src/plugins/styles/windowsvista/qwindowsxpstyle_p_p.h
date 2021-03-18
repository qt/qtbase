/****************************************************************************
**
** Copyright (C) 2016 The Qt Company Ltd.
** Contact: https://www.qt.io/licensing/
**
** This file is part of the QtWidgets module of the Qt Toolkit.
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

#ifndef QWINDOWSXPSTYLE_P_P_H
#define QWINDOWSXPSTYLE_P_P_H

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
#include "qwindowsxpstyle_p.h"
#include <QtWidgets/private/qwindowsstyle_p_p.h>
#include <qmap.h>
#include <qt_windows.h>

#include <uxtheme.h>
#include <vssym32.h>

#include <limits.h>

QT_BEGIN_NAMESPACE

class QDebug;

// TMT_TEXTSHADOWCOLOR is wrongly defined in mingw
#if TMT_TEXTSHADOWCOLOR != 3818
#undef TMT_TEXTSHADOWCOLOR
#define TMT_TEXTSHADOWCOLOR 3818
#endif
#ifndef TST_NONE
#  define TST_NONE 0
#endif

// These defines are missing from the tmschema, but still exist as
// states for their parts
#ifndef MINBS_INACTIVE
#define MINBS_INACTIVE 5
#endif
#ifndef MAXBS_INACTIVE
#define MAXBS_INACTIVE 5
#endif
#ifndef RBS_INACTIVE
#define RBS_INACTIVE 5
#endif
#ifndef HBS_INACTIVE
#define HBS_INACTIVE 5
#endif
#ifndef CBS_INACTIVE
#define CBS_INACTIVE 5
#endif

// Uncomment define below to build debug assisting code, and output
// #define DEBUG_XP_STYLE

// Declarations -----------------------------------------------------------------------------------
class XPThemeData
{
public:
    explicit XPThemeData(const QWidget *w = nullptr, QPainter *p = nullptr, int themeIn = -1,
                         int part = 0, int state = 0, const QRect &r = QRect())
        : widget(w), painter(p), theme(themeIn), partId(part), stateId(state),
          mirrorHorizontally(false), mirrorVertically(false), noBorder(false),
          noContent(false), rect(r)
    {}

    HRGN mask(QWidget *widget);
    HTHEME handle();

    static RECT toRECT(const QRect &qr);
    bool isValid();

    QSizeF size();
    QMarginsF margins(const QRect &rect, int propId = TMT_CONTENTMARGINS);
    QMarginsF margins(int propId = TMT_CONTENTMARGINS);

    static QSizeF themeSize(const QWidget *w = nullptr, QPainter *p = nullptr, int themeIn = -1, int part = 0, int state = 0);
    static QMarginsF themeMargins(const QRect &rect, const QWidget *w = nullptr, QPainter *p = nullptr, int themeIn = -1,
                                  int part = 0, int state = 0, int propId = TMT_CONTENTMARGINS);
    static QMarginsF themeMargins(const QWidget *w = nullptr, QPainter *p = nullptr, int themeIn = -1,
                                  int part = 0, int state = 0, int propId = TMT_CONTENTMARGINS);

    const QWidget *widget;
    QPainter *painter;

    int theme;
    HTHEME htheme = nullptr;
    int partId;
    int stateId;

    uint mirrorHorizontally : 1;
    uint mirrorVertically : 1;
    uint noBorder : 1;
    uint noContent : 1;
    uint rotate = 0;
    QRect rect;
};

struct ThemeMapKey {
    int theme = 0;
    int partId = -1;
    int stateId = -1;
    bool noBorder = false;
    bool noContent = false;

    ThemeMapKey() = default;
    ThemeMapKey(const XPThemeData &data)
        : theme(data.theme), partId(data.partId), stateId(data.stateId),
        noBorder(data.noBorder), noContent(data.noContent) {}

};

inline uint qHash(const ThemeMapKey &key)
{ return key.theme ^ key.partId ^ key.stateId; }

inline bool operator==(const ThemeMapKey &k1, const ThemeMapKey &k2)
{
    return k1.theme == k2.theme
           && k1.partId == k2.partId
           && k1.stateId == k2.stateId;
}

enum AlphaChannelType {
    UnknownAlpha = -1,          // Alpha of part & state not yet known
    NoAlpha,                    // Totally opaque, no need to touch alpha (RGB)
    MaskAlpha,                  // Alpha channel must be fixed            (ARGB)
    RealAlpha                   // Proper alpha values from Windows       (ARGB_Premultiplied)
};

struct ThemeMapData {
    AlphaChannelType alphaType = UnknownAlpha; // Which type of alpha on part & state

    bool dataValid         : 1; // Only used to detect if hash value is ok
    bool partIsTransparent : 1;
    bool hasAlphaChannel   : 1; // True =  part & state has real Alpha
    bool wasAlphaSwapped   : 1; // True =  alpha channel needs to be swapped
    bool hadInvalidAlpha   : 1; // True =  alpha channel contained invalid alpha values

    ThemeMapData() : dataValid(false), partIsTransparent(false),
                     hasAlphaChannel(false), wasAlphaSwapped(false), hadInvalidAlpha(false) {}
};

#ifndef QT_NO_DEBUG_STREAM
QDebug operator<<(QDebug d, const XPThemeData &t);
QDebug operator<<(QDebug d, const ThemeMapKey &k);
QDebug operator<<(QDebug d, const ThemeMapData &td);
#endif

class QWindowsXPStylePrivate : public QWindowsStylePrivate
{
    Q_DECLARE_PUBLIC(QWindowsXPStyle)
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
        XpTreeViewTheme, // '+'/'-' shape treeview indicators (XP)
        WindowTheme,
        StatusTheme,
        VistaTreeViewTheme, // arrow shape treeview indicators (Vista) obtained from "explorer" theme.
        NThemes
    };

    QWindowsXPStylePrivate()
    { init(); }

    ~QWindowsXPStylePrivate()
    { cleanup(); }

    static int pixelMetricFromSystemDp(QStyle::PixelMetric pm, const QStyleOption *option = nullptr, const QWidget *widget = nullptr);
    static int fixedPixelMetric(QStyle::PixelMetric pm, const QStyleOption *option = nullptr, const QWidget *widget = nullptr);

    static HWND winId(const QWidget *widget);

    void init(bool force = false);
    void cleanup(bool force = false);
    void cleanupHandleMap();

    HBITMAP buffer(int w = 0, int h = 0);
    HDC bufferHDC()
    { return bufferDC;}

    static bool useXP(bool update = false);
    static QRect scrollBarGripperBounds(QStyle::State flags, const QWidget *widget, XPThemeData *theme);

    bool isTransparent(XPThemeData &themeData);
    QRegion region(XPThemeData &themeData);

    bool drawBackground(XPThemeData &themeData, qreal correctionFactor = 1);
    bool drawBackgroundThruNativeBuffer(XPThemeData &themeData, qreal aditionalDevicePixelRatio, qreal correctionFactor);
    bool drawBackgroundDirectly(HDC dc, XPThemeData &themeData, qreal aditionalDevicePixelRatio);

    bool hasAlphaChannel(const QRect &rect);
    bool fixAlphaChannel(const QRect &rect);
    bool swapAlphaChannel(const QRect &rect, bool allPixels = false);

    QRgb groupBoxTextColor = 0;
    QRgb groupBoxTextColorDisabled = 0;
    QRgb sliderTickColor = 0;
    bool hasInitColors = false;

    static HTHEME createTheme(int theme, HWND hwnd);
    static QString themeName(int theme);
    static inline bool hasTheme(int theme) { return theme >= 0 && theme < NThemes && m_themes[theme]; }
    static bool isItemViewDelegateLineEdit(const QWidget *widget);
    static bool isLineEditBaseColorSet(const QStyleOption *option, const QWidget *widget);

    QIcon dockFloat, dockClose;

private:
#ifdef DEBUG_XP_STYLE
    void dumpNativeDIB(int w, int h);
    void showProperties(XPThemeData &themeData);
#endif

    static bool initVistaTreeViewTheming();
    static void cleanupVistaTreeViewTheming();

    static QBasicAtomicInt ref;
    static bool use_xp;

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

inline QSizeF XPThemeData::size()
{
    QSizeF result(0, 0);
    if (isValid()) {
        SIZE size;
        if (SUCCEEDED(GetThemePartSize(handle(), nullptr, partId, stateId, nullptr, TS_TRUE, &size)))
            result = QSize(size.cx, size.cy);
    }
    return result;
}

inline QMarginsF XPThemeData::margins(const QRect &qRect, int propId)
{
    QMarginsF result(0, 0, 0 ,0);
    if (isValid()) {
        MARGINS margins;
        RECT rect = XPThemeData::toRECT(qRect);
        if (SUCCEEDED(GetThemeMargins(handle(), nullptr, partId, stateId, propId, &rect, &margins)))
            result = QMargins(margins.cxLeftWidth, margins.cyTopHeight, margins.cxRightWidth, margins.cyBottomHeight);
    }
    return result;
}

inline QMarginsF XPThemeData::margins(int propId)
{
    QMarginsF result(0, 0, 0 ,0);
    if (isValid()) {
        MARGINS margins;
        if (SUCCEEDED(GetThemeMargins(handle(), nullptr, partId, stateId, propId, nullptr, &margins)))
            result = QMargins(margins.cxLeftWidth, margins.cyTopHeight, margins.cxRightWidth, margins.cyBottomHeight);
    }
    return result;
}

inline QSizeF XPThemeData::themeSize(const QWidget *w, QPainter *p, int themeIn, int part, int state)
{
    XPThemeData theme(w, p, themeIn, part, state);
    return theme.size();
}

inline QMarginsF XPThemeData::themeMargins(const QRect &rect, const QWidget *w, QPainter *p, int themeIn,
                                           int part, int state, int propId)
{
    XPThemeData theme(w, p, themeIn, part, state);
    return theme.margins(rect, propId);
}

inline QMarginsF XPThemeData::themeMargins(const QWidget *w, QPainter *p, int themeIn,
                                           int part, int state, int propId)
{
    XPThemeData theme(w, p, themeIn, part, state);
    return theme.margins(propId);
}

QT_END_NAMESPACE

#endif //QWINDOWSXPSTYLE_P_P_H
