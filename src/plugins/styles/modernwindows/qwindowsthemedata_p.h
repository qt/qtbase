// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#ifndef QWINDOWSTHEMEDATA_P_H
#define QWINDOWSTHEMEDATA_P_H

//
//  W A R N I N G
//  -------------
//
// This file is not part of the Qt API.  It exists purely as an
// implementation detail.  This header file may change from version to
// version without notice, or even be removed.
//
// We mean it.
//

#include <qwidget.h>
#include <qt_windows.h>
#include <uxtheme.h>
#include <vssym32.h>
#include <limits.h>


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

QT_BEGIN_NAMESPACE

// Declarations -----------------------------------------------------------------------------------
class QWindowsThemeData
{
public:
    explicit QWindowsThemeData(const QWidget *w = nullptr, QPainter *p = nullptr, int themeIn = -1,
                         int part = 0, int state = 0, const QRect &r = QRect())
        : widget(w), painter(p), theme(themeIn), partId(part), stateId(state),
          mirrorHorizontally(false), mirrorVertically(false), noBorder(false),
          noContent(false), invertPixels(false), rect(r)
    {}

    HRGN mask(QWidget *widget);
    HTHEME handle();
    bool isValid();
    QSizeF size();

    static QSizeF themeSize(const QWidget *w = nullptr, QPainter *p = nullptr,
                            int themeIn = -1, int part = 0, int state = 0);
    static RECT toRECT(const QRect &qr);

    QMarginsF margins(const QRect &rect, int propId = TMT_CONTENTMARGINS);
    QMarginsF margins(int propId = TMT_CONTENTMARGINS);

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
    uint invertPixels : 1;
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
    ThemeMapKey(const QWindowsThemeData &data)
        : theme(data.theme), partId(data.partId), stateId(data.stateId),
        noBorder(data.noBorder), noContent(data.noContent) {}

};

inline size_t qHash(const ThemeMapKey &key)
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
QDebug operator<<(QDebug d, const QWindowsThemeData &t);
QDebug operator<<(QDebug d, const ThemeMapKey &k);
QDebug operator<<(QDebug d, const ThemeMapData &td);
#endif

inline QSizeF QWindowsThemeData::size()
{
    QSizeF result(0, 0);
    if (isValid()) {
        SIZE size;
        if (SUCCEEDED(GetThemePartSize(handle(), nullptr, partId, stateId, nullptr, TS_TRUE, &size)))
            result = QSize(size.cx, size.cy);
    }
    return result;
}

inline QSizeF QWindowsThemeData::themeSize(const QWidget *w, QPainter *p, int themeIn, int part, int state)
{
    QWindowsThemeData theme(w, p, themeIn, part, state);
    return theme.size();
}

inline QMarginsF QWindowsThemeData::margins(const QRect &qRect, int propId)
{
    QMarginsF result(0, 0, 0 ,0);
    if (isValid()) {
        MARGINS margins;
        RECT rect = QWindowsThemeData::toRECT(qRect);
        if (SUCCEEDED(GetThemeMargins(handle(), nullptr, partId, stateId, propId, &rect, &margins)))
            result = QMargins(margins.cxLeftWidth, margins.cyTopHeight, margins.cxRightWidth, margins.cyBottomHeight);
    }
    return result;
}

inline QMarginsF QWindowsThemeData::margins(int propId)
{
    QMarginsF result(0, 0, 0 ,0);
    if (isValid()) {
        MARGINS margins;
        if (SUCCEEDED(GetThemeMargins(handle(), nullptr, partId, stateId, propId, nullptr, &margins)))
            result = QMargins(margins.cxLeftWidth, margins.cyTopHeight, margins.cxRightWidth, margins.cyBottomHeight);
    }
    return result;
}

QT_END_NAMESPACE

#endif // QWINDOWSTHEMEDATA_P_H
