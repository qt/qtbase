// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#include <qglobal.h>
#include "qstylesheetstyle_p.h"

#if QT_CONFIG(style_stylesheet)

#include "private/qcssutil_p.h"
#include <qdebug.h>
#include <qdir.h>
#include <qapplication.h>
#if QT_CONFIG(menu)
#include <qmenu.h>
#endif
#if QT_CONFIG(menubar)
#include <qmenubar.h>
#endif
#include <qpainter.h>
#include <qstyleoption.h>
#if QT_CONFIG(lineedit)
#include <qlineedit.h>
#endif
#if QT_CONFIG(textedit)
#include <qtextedit.h>
#include <qplaintextedit.h>
#endif
#include <private/qwindowsstyle_p.h>
#if QT_CONFIG(combobox)
#include <qcombobox.h>
#endif
#include "private/qcssparser_p.h"
#include "private/qmath_p.h"
#include <qabstractscrollarea.h>
#include "private/qabstractscrollarea_p.h"
#if QT_CONFIG(tooltip)
#include <qtooltip.h>
#endif
#include <qshareddata.h>
#if QT_CONFIG(toolbutton)
#include <qtoolbutton.h>
#endif
#if QT_CONFIG(scrollbar)
#include <qscrollbar.h>
#endif
#if QT_CONFIG(abstractslider)
#include <qabstractslider.h>
#endif
#include <qstring.h>
#include <qfile.h>
#if QT_CONFIG(checkbox)
#include <qcheckbox.h>
#endif
#if QT_CONFIG(itemviews)
#include <qheaderview.h>
#endif
#include <private/qwindowsstyle_p_p.h>
#if QT_CONFIG(animation)
#include <private/qstyleanimation_p.h>
#endif
#if QT_CONFIG(tabbar)
#include <private/qtabbar_p.h>
#endif
#include <QMetaProperty>
#if QT_CONFIG(mainwindow)
#include <qmainwindow.h>
#endif
#if QT_CONFIG(dockwidget)
#include <qdockwidget.h>
#endif
#if QT_CONFIG(mdiarea)
#include <qmdisubwindow.h>
#endif
#if QT_CONFIG(dialog)
#include <qdialog.h>
#endif
#include <private/qwidget_p.h>
#if QT_CONFIG(spinbox)
#include <QAbstractSpinBox>
#endif
#if QT_CONFIG(label)
#include <QLabel>
#endif
#include "qdrawutil.h"

#include <limits.h>
#if QT_CONFIG(toolbar)
#include <QtWidgets/qtoolbar.h>
#endif
#if QT_CONFIG(pushbutton)
#include <QtWidgets/qpushbutton.h>
#endif

#include <QtGui/qpainterpath.h>
#include <QtGui/qscreen.h>

#include <QtCore/private/qduplicatetracker_p.h>

QT_BEGIN_NAMESPACE

using namespace Qt::StringLiterals;

using namespace QCss;


class QStyleSheetStylePrivate : public QWindowsStylePrivate
{
    Q_DECLARE_PUBLIC(QStyleSheetStyle)
public:
    QStyleSheetStylePrivate() { }
};


static QStyleSheetStyleCaches *styleSheetCaches = nullptr;

/* RECURSION_GUARD:
 * the QStyleSheetStyle is a proxy. If used with others proxy style, we may end up with something like:
 * QStyleSheetStyle -> ProxyStyle -> QStyleSheetStyle -> OriginalStyle
 * Recursion may happen if the style call the widget()->style() again.
 * Not to mention the performance penalty of having two lookup of rules.
 *
 * The first instance of QStyleSheetStyle will set globalStyleSheetStyle to itself. The second one
 * will notice the globalStyleSheetStyle is not istelf and call its base style directly.
 */
static const QStyleSheetStyle *globalStyleSheetStyle = nullptr;
class QStyleSheetStyleRecursionGuard
{
    public:
        QStyleSheetStyleRecursionGuard(const QStyleSheetStyle *that)
            :  guarded(globalStyleSheetStyle == nullptr)
            {
                if (guarded) globalStyleSheetStyle = that;
            }
        ~QStyleSheetStyleRecursionGuard() { if (guarded) globalStyleSheetStyle = nullptr; }
        bool guarded;
};
#define RECURSION_GUARD(RETURN) \
    if (globalStyleSheetStyle != 0 && globalStyleSheetStyle != this) { RETURN; } \
    QStyleSheetStyleRecursionGuard recursion_guard(this);

enum PseudoElement {
    PseudoElement_None,
    PseudoElement_DownArrow,
    PseudoElement_UpArrow,
    PseudoElement_LeftArrow,
    PseudoElement_RightArrow,
    PseudoElement_Indicator,
    PseudoElement_ExclusiveIndicator,
    PseudoElement_PushButtonMenuIndicator,
    PseudoElement_ComboBoxDropDown,
    PseudoElement_ComboBoxArrow,
    PseudoElement_Item,
    PseudoElement_SpinBoxUpButton,
    PseudoElement_SpinBoxUpArrow,
    PseudoElement_SpinBoxDownButton,
    PseudoElement_SpinBoxDownArrow,
    PseudoElement_GroupBoxTitle,
    PseudoElement_GroupBoxIndicator,
    PseudoElement_ToolButtonMenu,
    PseudoElement_ToolButtonMenuArrow,
    PseudoElement_ToolButtonMenuIndicator,
    PseudoElement_ToolBoxTab,
    PseudoElement_ScrollBarSlider,
    PseudoElement_ScrollBarAddPage,
    PseudoElement_ScrollBarSubPage,
    PseudoElement_ScrollBarAddLine,
    PseudoElement_ScrollBarSubLine,
    PseudoElement_ScrollBarFirst,
    PseudoElement_ScrollBarLast,
    PseudoElement_ScrollBarUpArrow,
    PseudoElement_ScrollBarDownArrow,
    PseudoElement_ScrollBarLeftArrow,
    PseudoElement_ScrollBarRightArrow,
    PseudoElement_SplitterHandle,
    PseudoElement_ToolBarHandle,
    PseudoElement_ToolBarSeparator,
    PseudoElement_MenuScroller,
    PseudoElement_MenuTearoff,
    PseudoElement_MenuCheckMark,
    PseudoElement_MenuSeparator,
    PseudoElement_MenuIcon,
    PseudoElement_MenuRightArrow,
    PseudoElement_TreeViewBranch,
    PseudoElement_HeaderViewSection,
    PseudoElement_HeaderViewUpArrow,
    PseudoElement_HeaderViewDownArrow,
    PseudoElement_ProgressBarChunk,
    PseudoElement_TabBarTab,
    PseudoElement_TabBarScroller,
    PseudoElement_TabBarTear,
    PseudoElement_SliderGroove,
    PseudoElement_SliderHandle,
    PseudoElement_SliderAddPage,
    PseudoElement_SliderSubPage,
    PseudoElement_SliderTickmark,
    PseudoElement_TabWidgetPane,
    PseudoElement_TabWidgetTabBar,
    PseudoElement_TabWidgetLeftCorner,
    PseudoElement_TabWidgetRightCorner,
    PseudoElement_DockWidgetTitle,
    PseudoElement_DockWidgetCloseButton,
    PseudoElement_DockWidgetFloatButton,
    PseudoElement_DockWidgetSeparator,
    PseudoElement_MdiCloseButton,
    PseudoElement_MdiMinButton,
    PseudoElement_MdiNormalButton,
    PseudoElement_TitleBar,
    PseudoElement_TitleBarCloseButton,
    PseudoElement_TitleBarMinButton,
    PseudoElement_TitleBarMaxButton,
    PseudoElement_TitleBarShadeButton,
    PseudoElement_TitleBarUnshadeButton,
    PseudoElement_TitleBarNormalButton,
    PseudoElement_TitleBarContextHelpButton,
    PseudoElement_TitleBarSysMenu,
    PseudoElement_ViewItem,
    PseudoElement_ViewItemIcon,
    PseudoElement_ViewItemText,
    PseudoElement_ViewItemIndicator,
    PseudoElement_ScrollAreaCorner,
    PseudoElement_TabBarTabCloseButton,
    NumPseudoElements
};

struct PseudoElementInfo {
    QStyle::SubControl subControl;
    const char name[19];
};

static const PseudoElementInfo knownPseudoElements[NumPseudoElements] = {
    { QStyle::SC_None, "" },
    { QStyle::SC_None, "down-arrow" },
    { QStyle::SC_None, "up-arrow" },
    { QStyle::SC_None, "left-arrow" },
    { QStyle::SC_None, "right-arrow" },
    { QStyle::SC_None, "indicator" },
    { QStyle::SC_None, "indicator" },
    { QStyle::SC_None, "menu-indicator" },
    { QStyle::SC_ComboBoxArrow, "drop-down" },
    { QStyle::SC_ComboBoxArrow, "down-arrow" },
    { QStyle::SC_None, "item" },
    { QStyle::SC_SpinBoxUp, "up-button" },
    { QStyle::SC_SpinBoxUp, "up-arrow" },
    { QStyle::SC_SpinBoxDown, "down-button" },
    { QStyle::SC_SpinBoxDown, "down-arrow" },
    { QStyle::SC_GroupBoxLabel, "title" },
    { QStyle::SC_GroupBoxCheckBox, "indicator" },
    { QStyle::SC_ToolButtonMenu, "menu-button" },
    { QStyle::SC_ToolButtonMenu, "menu-arrow" },
    { QStyle::SC_None, "menu-indicator" },
    { QStyle::SC_None, "tab" },
    { QStyle::SC_ScrollBarSlider, "handle" },
    { QStyle::SC_ScrollBarAddPage, "add-page" },
    { QStyle::SC_ScrollBarSubPage, "sub-page" },
    { QStyle::SC_ScrollBarAddLine, "add-line" },
    { QStyle::SC_ScrollBarSubLine, "sub-line" },
    { QStyle::SC_ScrollBarFirst, "first" },
    { QStyle::SC_ScrollBarLast, "last" },
    { QStyle::SC_ScrollBarSubLine, "up-arrow" },
    { QStyle::SC_ScrollBarAddLine, "down-arrow" },
    { QStyle::SC_ScrollBarSubLine, "left-arrow" },
    { QStyle::SC_ScrollBarAddLine, "right-arrow" },
    { QStyle::SC_None, "handle" },
    { QStyle::SC_None, "handle" },
    { QStyle::SC_None, "separator" },
    { QStyle::SC_None, "scroller" },
    { QStyle::SC_None, "tearoff" },
    { QStyle::SC_None, "indicator" },
    { QStyle::SC_None, "separator" },
    { QStyle::SC_None, "icon" },
    { QStyle::SC_None, "right-arrow" },
    { QStyle::SC_None, "branch" },
    { QStyle::SC_None, "section" },
    { QStyle::SC_None, "down-arrow" },
    { QStyle::SC_None, "up-arrow" },
    { QStyle::SC_None, "chunk" },
    { QStyle::SC_None, "tab" },
    { QStyle::SC_None, "scroller" },
    { QStyle::SC_None, "tear" },
    { QStyle::SC_SliderGroove, "groove" },
    { QStyle::SC_SliderHandle, "handle" },
    { QStyle::SC_None, "add-page" },
    { QStyle::SC_None, "sub-page"},
    { QStyle::SC_SliderTickmarks, "tick-mark" },
    { QStyle::SC_None, "pane" },
    { QStyle::SC_None, "tab-bar" },
    { QStyle::SC_None, "left-corner" },
    { QStyle::SC_None, "right-corner" },
    { QStyle::SC_None, "title" },
    { QStyle::SC_None, "close-button" },
    { QStyle::SC_None, "float-button" },
    { QStyle::SC_None, "separator" },
    { QStyle::SC_MdiCloseButton, "close-button" },
    { QStyle::SC_MdiMinButton, "minimize-button" },
    { QStyle::SC_MdiNormalButton, "normal-button" },
    { QStyle::SC_TitleBarLabel, "title" },
    { QStyle::SC_TitleBarCloseButton, "close-button" },
    { QStyle::SC_TitleBarMinButton, "minimize-button" },
    { QStyle::SC_TitleBarMaxButton, "maximize-button" },
    { QStyle::SC_TitleBarShadeButton, "shade-button" },
    { QStyle::SC_TitleBarUnshadeButton, "unshade-button" },
    { QStyle::SC_TitleBarNormalButton, "normal-button" },
    { QStyle::SC_TitleBarContextHelpButton, "contexthelp-button" },
    { QStyle::SC_TitleBarSysMenu, "sys-menu" },
    { QStyle::SC_None, "item" },
    { QStyle::SC_None, "icon" },
    { QStyle::SC_None, "text" },
    { QStyle::SC_None, "indicator" },
    { QStyle::SC_None, "corner" },
    { QStyle::SC_None, "close-button" },
};


struct QStyleSheetBorderImageData : public QSharedData
{
    QStyleSheetBorderImageData()
        : horizStretch(QCss::TileMode_Unknown), vertStretch(QCss::TileMode_Unknown)
    {
        for (int i = 0; i < 4; i++)
            cuts[i] = -1;
    }
    int cuts[4];
    QPixmap pixmap;
    QImage image;
    QCss::TileMode horizStretch, vertStretch;
};

struct QStyleSheetBackgroundData : public QSharedData
{
    QStyleSheetBackgroundData(const QBrush& b, const QPixmap& p, QCss::Repeat r,
                              Qt::Alignment a, QCss::Origin o, Attachment t, QCss::Origin c)
        : brush(b), pixmap(p), repeat(r), position(a), origin(o), attachment(t), clip(c) { }

    bool isTransparent() const {
        if (brush.style() != Qt::NoBrush)
            return !brush.isOpaque();
        return pixmap.isNull() ? false : pixmap.hasAlpha();
    }
    QBrush brush;
    QPixmap pixmap;
    QCss::Repeat repeat;
    Qt::Alignment position;
    QCss::Origin origin;
    QCss::Attachment attachment;
    QCss::Origin clip;
};

struct QStyleSheetBorderData : public QSharedData
{
    QStyleSheetBorderData() : bi(nullptr)
    {
        for (int i = 0; i < 4; i++) {
            borders[i] = 0;
            styles[i] = QCss::BorderStyle_None;
        }
    }

    QStyleSheetBorderData(int *b, QBrush *c, QCss::BorderStyle *s, QSize *r) : bi(nullptr)
    {
        for (int i = 0; i < 4; i++) {
            borders[i] = b[i];
            styles[i] = s[i];
            colors[i] = c[i];
            radii[i] = r[i];
        }
    }

    int borders[4];
    QBrush colors[4];
    QCss::BorderStyle styles[4];
    QSize radii[4]; // topleft, topright, bottomleft, bottomright

    const QStyleSheetBorderImageData *borderImage() const
    { return bi; }
    bool hasBorderImage() const { return bi!=nullptr; }

    QSharedDataPointer<QStyleSheetBorderImageData> bi;

    bool isOpaque() const
    {
        for (int i = 0; i < 4; i++) {
            if (styles[i] == QCss::BorderStyle_Native || styles[i] == QCss::BorderStyle_None)
                continue;
            if (styles[i] >= QCss::BorderStyle_Dotted && styles[i] <= QCss::BorderStyle_DotDotDash
                && styles[i] != BorderStyle_Solid)
                return false;
            if (!colors[i].isOpaque())
                return false;
            if (!radii[i].isEmpty())
                return false;
        }
        if (bi != nullptr && bi->pixmap.hasAlpha())
            return false;
        return true;
    }
};


struct QStyleSheetOutlineData : public QStyleSheetBorderData
{
    QStyleSheetOutlineData()
    {
        for (int i = 0; i < 4; i++) {
            offsets[i] = 0;
        }
    }

    QStyleSheetOutlineData(int *b, QBrush *c, QCss::BorderStyle *s, QSize *r, int *o)
            : QStyleSheetBorderData(b, c, s, r)
    {
        for (int i = 0; i < 4; i++) {
            offsets[i] = o[i];
        }
    }

    int offsets[4];
};

struct QStyleSheetBoxData : public QSharedData
{
    QStyleSheetBoxData(int *m, int *p, int s) : spacing(s)
    {
        for (int i = 0; i < 4; i++) {
            margins[i] = m[i];
            paddings[i] = p[i];
        }
    }

    int margins[4];
    int paddings[4];

    int spacing;
};

struct QStyleSheetPaletteData : public QSharedData
{
    QStyleSheetPaletteData(const QBrush &fg, const QBrush &sfg, const QBrush &sbg,
                           const QBrush &abg, const QBrush &pfg)
        : foreground(fg), selectionForeground(sfg), selectionBackground(sbg),
          alternateBackground(abg), placeholderForeground(pfg) { }

    QBrush foreground;
    QBrush selectionForeground;
    QBrush selectionBackground;
    QBrush alternateBackground;
    QBrush placeholderForeground;
};

struct QStyleSheetGeometryData : public QSharedData
{
    QStyleSheetGeometryData(int w, int h, int minw, int minh, int maxw, int maxh)
        : minWidth(minw), minHeight(minh), width(w), height(h), maxWidth(maxw), maxHeight(maxh) { }

    int minWidth, minHeight, width, height, maxWidth, maxHeight;
};

struct QStyleSheetPositionData : public QSharedData
{
    QStyleSheetPositionData(int l, int t, int r, int b, Origin o, Qt::Alignment p, QCss::PositionMode m, Qt::Alignment a = { })
        : left(l), top(t), bottom(b), right(r), origin(o), position(p), mode(m), textAlignment(a) { }

    int left, top, bottom, right;
    Origin origin;
    Qt::Alignment position;
    QCss::PositionMode mode;
    Qt::Alignment textAlignment;
};

struct QStyleSheetImageData : public QSharedData
{
    QStyleSheetImageData(const QIcon &i, Qt::Alignment a, const QSize &sz)
        : icon(i), alignment(a), size(sz) { }

    QIcon icon;
    Qt::Alignment alignment;
    QSize size;
};

class QRenderRule
{
public:
    QRenderRule() : features(0), hasFont(false), pal(nullptr), b(nullptr), bg(nullptr), bd(nullptr), ou(nullptr), geo(nullptr), p(nullptr), img(nullptr), clipset(0) { }
    QRenderRule(const QList<QCss::Declaration> &, const QObject *);

    QRect borderRect(const QRect &r) const;
    QRect outlineRect(const QRect &r) const;
    QRect paddingRect(const QRect &r) const;
    QRect contentsRect(const QRect &r) const;

    enum { Margin = 1, Border = 2, Padding = 4, All=Margin|Border|Padding };
    QRect boxRect(const QRect &r, int flags = All) const;
    QSize boxSize(const QSize &s, int flags = All) const;
    QRect originRect(const QRect &rect, Origin origin) const;

    QPainterPath borderClip(QRect rect);
    void drawBorder(QPainter *, const QRect&);
    void drawOutline(QPainter *, const QRect&);
    void drawBorderImage(QPainter *, const QRect&);
    void drawBackground(QPainter *, const QRect&, const QPoint& = QPoint(0, 0));
    void drawBackgroundImage(QPainter *, const QRect&, QPoint = QPoint(0, 0));
    void drawFrame(QPainter *, const QRect&);
    void drawImage(QPainter *p, const QRect &rect);
    void drawRule(QPainter *, const QRect&);
    void configurePalette(QPalette *, QPalette::ColorGroup, const QWidget *, bool);
    void configurePalette(QPalette *p, QPalette::ColorRole fr, QPalette::ColorRole br);

    const QStyleSheetPaletteData *palette() const { return pal; }
    const QStyleSheetBoxData *box() const { return b; }
    const QStyleSheetBackgroundData *background() const { return bg; }
    const QStyleSheetBorderData *border() const { return bd; }
    const QStyleSheetOutlineData *outline() const { return ou; }
    const QStyleSheetGeometryData *geometry() const { return geo; }
    const QStyleSheetPositionData *position() const { return p; }
    const QStyleSheetImageData *icon() const { return iconPtr; }

    bool hasModification() const;

    bool hasPalette() const { return pal != nullptr; }
    bool hasBackground() const { return bg != nullptr && (!bg->pixmap.isNull() || bg->brush.style() != Qt::NoBrush); }
    bool hasGradientBackground() const { return bg && bg->brush.style() >= Qt::LinearGradientPattern
                                                   && bg->brush.style() <= Qt::ConicalGradientPattern; }

    bool hasNativeBorder() const {
        return bd == nullptr
               || (!bd->hasBorderImage() && bd->styles[0] == BorderStyle_Native);
    }

    bool hasNativeOutline() const {
        return (ou == nullptr
                || (!ou->hasBorderImage() && ou->styles[0] == BorderStyle_Native));
    }

    bool baseStyleCanDraw() const {
        if (!hasBackground() || (background()->brush.style() == Qt::NoBrush && bg->pixmap.isNull()))
            return true;
        if (bg && !bg->pixmap.isNull())
            return false;
        if (hasGradientBackground())
            return features & StyleFeature_BackgroundGradient;
        return features & StyleFeature_BackgroundColor;
    }

    bool hasBox() const { return b != nullptr; }
    bool hasBorder() const { return bd != nullptr; }
    bool hasOutline() const { return ou != nullptr; }
    bool hasPosition() const { return p != nullptr; }
    bool hasGeometry() const { return geo != nullptr; }
    bool hasDrawable() const { return !hasNativeBorder() || hasBackground() || hasImage(); }
    bool hasImage() const { return img != nullptr; }
    bool hasIcon() const { return iconPtr != nullptr; }

    QSize minimumContentsSize() const
    { return geo ? QSize(geo->minWidth, geo->minHeight) : QSize(0, 0); }
    QSize minimumSize() const
    { return boxSize(minimumContentsSize()); }

    QSize contentsSize() const
    { return geo ? QSize(geo->width, geo->height)
                 : ((img && img->size.isValid()) ? img->size : QSize()); }
    QSize contentsSize(const QSize &sz) const
    {
        QSize csz = contentsSize();
        if (csz.width() == -1) csz.setWidth(sz.width());
        if (csz.height() == -1) csz.setHeight(sz.height());
        return csz;
    }
    bool hasContentsSize() const
    { return (geo && (geo->width != -1 || geo->height != -1)) || (img && img->size.isValid()); }

    QSize size() const { return boxSize(contentsSize()); }
    QSize size(const QSize &sz) const { return boxSize(contentsSize(sz)); }
    QSize adjustSize(const QSize &sz)
    {
        if (!geo)
            return sz;
        QSize csz = contentsSize();
        if (csz.width() == -1) csz.setWidth(sz.width());
        if (csz.height() == -1) csz.setHeight(sz.height());
        if (geo->maxWidth != -1 && csz.width() > geo->maxWidth) csz.setWidth(geo->maxWidth);
        if (geo->maxHeight != -1 && csz.height() > geo->maxHeight) csz.setHeight(geo->maxHeight);
        csz=csz.expandedTo(QSize(geo->minWidth, geo->minHeight));
        return csz;
    }

    bool hasStyleHint(const QString &sh) const { return styleHints.contains(sh); }
    QVariant styleHint(const QString &sh) const { return styleHints.value(sh); }

    void fixupBorder(int);

    // Shouldn't be here
    void setClip(QPainter *p, const QRect &rect);
    void unsetClip(QPainter *);

public:
    int features;
    QBrush defaultBackground;
    QFont font; // Be careful using this font directly. Prefer using font.resolve( )
    bool hasFont;

    QHash<QString, QVariant> styleHints;

    QSharedDataPointer<QStyleSheetPaletteData> pal;
    QSharedDataPointer<QStyleSheetBoxData> b;
    QSharedDataPointer<QStyleSheetBackgroundData> bg;
    QSharedDataPointer<QStyleSheetBorderData> bd;
    QSharedDataPointer<QStyleSheetOutlineData> ou;
    QSharedDataPointer<QStyleSheetGeometryData> geo;
    QSharedDataPointer<QStyleSheetPositionData> p;
    QSharedDataPointer<QStyleSheetImageData> img;
    QSharedDataPointer<QStyleSheetImageData> iconPtr;

    int clipset;
    QPainterPath clipPath;
};
Q_DECLARE_TYPEINFO(QRenderRule, Q_RELOCATABLE_TYPE);

///////////////////////////////////////////////////////////////////////////////////////////
static const char knownStyleHints[][45] = {
    "activate-on-singleclick",
    "alignment",
    "arrow-keys-navigate-into-children",
    "backward-icon",
    "button-layout",
    "cd-icon",
    "combobox-list-mousetracking",
    "combobox-popup",
    "computer-icon",
    "desktop-icon",
    "dialog-apply-icon",
    "dialog-cancel-icon",
    "dialog-close-icon",
    "dialog-discard-icon",
    "dialog-help-icon",
    "dialog-no-icon",
    "dialog-ok-icon",
    "dialog-open-icon",
    "dialog-reset-icon",
    "dialog-save-icon",
    "dialog-yes-icon",
    "dialogbuttonbox-buttons-have-icons",
    "directory-closed-icon",
    "directory-icon",
    "directory-link-icon",
    "directory-open-icon",
    "dither-disable-text",
    "dockwidget-close-icon",
    "downarrow-icon",
    "dvd-icon",
    "etch-disabled-text",
    "file-icon",
    "file-link-icon",
    "filedialog-backward-icon", // unused
    "filedialog-contentsview-icon",
    "filedialog-detailedview-icon",
    "filedialog-end-icon",
    "filedialog-infoview-icon",
    "filedialog-listview-icon",
    "filedialog-new-directory-icon",
    "filedialog-parent-directory-icon",
    "filedialog-start-icon",
    "floppy-icon",
    "forward-icon",
    "gridline-color",
    "harddisk-icon",
    "home-icon",
    "lineedit-clear-button-icon",
    "icon-size",
    "leftarrow-icon",
    "lineedit-password-character",
    "lineedit-password-mask-delay",
    "mdi-fill-space-on-maximize",
    "menu-scrollable",
    "menubar-altkey-navigation",
    "menubar-separator",
    "messagebox-critical-icon",
    "messagebox-information-icon",
    "messagebox-question-icon",
    "messagebox-text-interaction-flags",
    "messagebox-warning-icon",
    "mouse-tracking",
    "network-icon",
    "opacity",
    "paint-alternating-row-colors-for-empty-area",
    "rightarrow-icon",
    "scrollbar-contextmenu",
    "scrollbar-leftclick-absolute-position",
    "scrollbar-middleclick-absolute-position",
    "scrollbar-roll-between-buttons",
    "scrollbar-scroll-when-pointer-leaves-control",
    "scrollview-frame-around-contents",
    "show-decoration-selected",
    "spinbox-click-autorepeat-rate",
    "spincontrol-disable-on-bounds",
    "tabbar-elide-mode",
    "tabbar-prefer-no-arrows",
    "titlebar-close-icon",
    "titlebar-contexthelp-icon",
    "titlebar-maximize-icon",
    "titlebar-menu-icon",
    "titlebar-minimize-icon",
    "titlebar-normal-icon",
    "titlebar-shade-icon",
    "titlebar-show-tooltips-on-buttons",
    "titlebar-unshade-icon",
    "toolbutton-popup-delay",
    "trash-icon",
    "uparrow-icon",
    "widget-animation-duration"
};

static const int numKnownStyleHints = sizeof(knownStyleHints)/sizeof(knownStyleHints[0]);

static QList<QVariant> subControlLayout(const QString& layout)
{
    QList<QVariant> buttons;
    for (int i = 0; i < layout.size(); i++) {
        int button = layout[i].toLatin1();
        switch (button) {
        case 'm':
            buttons.append(PseudoElement_MdiMinButton);
            buttons.append(PseudoElement_TitleBarMinButton);
            break;
        case 'M':
            buttons.append(PseudoElement_TitleBarMaxButton);
            break;
        case 'X':
            buttons.append(PseudoElement_MdiCloseButton);
            buttons.append(PseudoElement_TitleBarCloseButton);
            break;
        case 'N':
            buttons.append(PseudoElement_MdiNormalButton);
            buttons.append(PseudoElement_TitleBarNormalButton);
            break;
        case 'I':
            buttons.append(PseudoElement_TitleBarSysMenu);
            break;
        case 'T':
            buttons.append(PseudoElement_TitleBar);
            break;
        case 'H':
            buttons.append(PseudoElement_TitleBarContextHelpButton);
            break;
        case 'S':
            buttons.append(PseudoElement_TitleBarShadeButton);
            break;
        default:
            buttons.append(button);
            break;
        }
    }
    return buttons;
}

namespace {
    struct ButtonInfo {
        QRenderRule rule;
        int element;
        int offset;
        int where;
        int width;
    };
}
template <> class QTypeInfo<ButtonInfo> : public QTypeInfoMerger<ButtonInfo, QRenderRule, int> {};

QHash<QStyle::SubControl, QRect> QStyleSheetStyle::titleBarLayout(const QWidget *w, const QStyleOptionTitleBar *tb) const
{
    QHash<QStyle::SubControl, QRect> layoutRects;
    const bool isMinimized = tb->titleBarState & Qt::WindowMinimized;
    const bool isMaximized = tb->titleBarState & Qt::WindowMaximized;
    QRenderRule subRule = renderRule(w, tb);
    QRect cr = subRule.contentsRect(tb->rect);
    QList<QVariant> layout = subRule.styleHint("button-layout"_L1).toList();
    if (layout.isEmpty())
        layout = subControlLayout("I(T)HSmMX"_L1);

    int offsets[3] = { 0, 0, 0 };
    enum Where { Left, Right, Center, NoWhere } where = Left;
    QList<ButtonInfo> infos;
    const int numLayouts = layout.size();
    infos.reserve(numLayouts);
    for (int i = 0; i < numLayouts; i++) {
        const int element = layout[i].toInt();
        if (element == '(') {
            where = Center;
        } else if (element == ')') {
            where = Right;
        } else {
            ButtonInfo info;
            info.element = element;
            switch (element) {
            case PseudoElement_TitleBar:
                if (!(tb->titleBarFlags & (Qt::WindowTitleHint | Qt::WindowSystemMenuHint)))
                    continue;
                break;
            case PseudoElement_TitleBarContextHelpButton:
                if (!(tb->titleBarFlags & Qt::WindowContextHelpButtonHint))
                    continue;
                break;
            case PseudoElement_TitleBarMinButton:
                if (!(tb->titleBarFlags & Qt::WindowMinimizeButtonHint))
                    continue;
                if (isMinimized)
                    info.element = PseudoElement_TitleBarNormalButton;
                break;
            case PseudoElement_TitleBarMaxButton:
                if (!(tb->titleBarFlags & Qt::WindowMaximizeButtonHint))
                    continue;
                if (isMaximized)
                    info.element = PseudoElement_TitleBarNormalButton;
                break;
            case PseudoElement_TitleBarShadeButton:
                if (!(tb->titleBarFlags & Qt::WindowShadeButtonHint))
                    continue;
                if (isMinimized)
                    info.element = PseudoElement_TitleBarUnshadeButton;
                break;
            case PseudoElement_TitleBarCloseButton:
            case PseudoElement_TitleBarSysMenu:
                if (!(tb->titleBarFlags & Qt::WindowSystemMenuHint))
                    continue;
                break;
            default:
                continue;
            }
            if (info.element == PseudoElement_TitleBar) {
                info.width = tb->fontMetrics.horizontalAdvance(tb->text) + 6;
                subRule.geo = new QStyleSheetGeometryData(info.width, tb->fontMetrics.height(), -1, -1, -1, -1);
            } else {
                subRule = renderRule(w, tb, info.element);
                info.width = subRule.size().width();
            }
            info.rule = subRule;
            info.offset = offsets[where];
            info.where = where;
            offsets[where] += info.width;

            infos.append(std::move(info));
        }
    }

    for (int i = 0; i < infos.size(); i++) {
        const ButtonInfo &info = infos[i];
        QRect lr = cr;
        switch (info.where) {
        case Center: {
            lr.setLeft(cr.left() + offsets[Left]);
            lr.setRight(cr.right() - offsets[Right]);
            QRect r(0, 0, offsets[Center], lr.height());
            r.moveCenter(lr.center());
            r.setLeft(r.left()+info.offset);
            r.setWidth(info.width);
            lr = r;
            break; }
        case Left:
            lr.translate(info.offset, 0);
            lr.setWidth(info.width);
            break;
        case Right:
            lr.moveLeft(cr.right() + 1 - offsets[Right] + info.offset);
            lr.setWidth(info.width);
            break;
        default:
            break;
        }
        QStyle::SubControl control = knownPseudoElements[info.element].subControl;
        layoutRects[control] = positionRect(w, info.rule, info.element, lr, tb->direction);
    }

    return layoutRects;
}

static QStyle::StandardPixmap subControlIcon(int pe)
{
    switch (pe) {
    case PseudoElement_MdiCloseButton: return QStyle::SP_TitleBarCloseButton;
    case PseudoElement_MdiMinButton: return QStyle::SP_TitleBarMinButton;
    case PseudoElement_MdiNormalButton: return QStyle::SP_TitleBarNormalButton;
    case PseudoElement_TitleBarCloseButton: return QStyle::SP_TitleBarCloseButton;
    case PseudoElement_TitleBarMinButton: return QStyle::SP_TitleBarMinButton;
    case PseudoElement_TitleBarMaxButton: return QStyle::SP_TitleBarMaxButton;
    case PseudoElement_TitleBarShadeButton: return QStyle::SP_TitleBarShadeButton;
    case PseudoElement_TitleBarUnshadeButton: return QStyle::SP_TitleBarUnshadeButton;
    case PseudoElement_TitleBarNormalButton: return QStyle::SP_TitleBarNormalButton;
    case PseudoElement_TitleBarContextHelpButton: return QStyle::SP_TitleBarContextHelpButton;
    default: break;
    }
    return QStyle::SP_CustomBase;
}

QRenderRule::QRenderRule(const QList<Declaration> &declarations, const QObject *object)
    : features(0),
      hasFont(false),
      pal(nullptr),
      b(nullptr),
      bg(nullptr),
      bd(nullptr),
      ou(nullptr),
      geo(nullptr),
      p(nullptr),
      img(nullptr),
      clipset(0)
{
    QPalette palette = QGuiApplication::palette(); // ###: ideally widget's palette
    ValueExtractor v(declarations, palette);
    features = v.extractStyleFeatures();

    int w = -1, h = -1, minw = -1, minh = -1, maxw = -1, maxh = -1;
    if (v.extractGeometry(&w, &h, &minw, &minh, &maxw, &maxh))
        geo = new QStyleSheetGeometryData(w, h, minw, minh, maxw, maxh);

    int left = 0, top = 0, right = 0, bottom = 0;
    Origin origin = Origin_Unknown;
    Qt::Alignment position;
    QCss::PositionMode mode = PositionMode_Unknown;
    Qt::Alignment textAlignment;
    if (v.extractPosition(&left, &top, &right, &bottom, &origin, &position, &mode, &textAlignment))
        p = new QStyleSheetPositionData(left, top, right, bottom, origin, position, mode, textAlignment);

    int margins[4], paddings[4], spacing = -1;
    for (int i = 0; i < 4; i++)
        margins[i] = paddings[i] = 0;
    if (v.extractBox(margins, paddings, &spacing))
        b = new QStyleSheetBoxData(margins, paddings, spacing);

    int borders[4];
    QBrush colors[4];
    QCss::BorderStyle styles[4];
    QSize radii[4];
    for (int i = 0; i < 4; i++) {
        borders[i] = 0;
        styles[i] = BorderStyle_None;
    }
    if (v.extractBorder(borders, colors, styles, radii))
        bd = new QStyleSheetBorderData(borders, colors, styles, radii);

    int offsets[4];
    for (int i = 0; i < 4; i++) {
        borders[i] = offsets[i] = 0;
        styles[i] = BorderStyle_None;
    }
    if (v.extractOutline(borders, colors, styles, radii, offsets))
        ou = new QStyleSheetOutlineData(borders, colors, styles, radii, offsets);

    QBrush brush;
    QString uri;
    Repeat repeat = Repeat_XY;
    Qt::Alignment alignment = Qt::AlignTop | Qt::AlignLeft;
    Attachment attachment = Attachment_Scroll;
    origin = Origin_Padding;
    Origin clip = Origin_Border;
    if (v.extractBackground(&brush, &uri, &repeat, &alignment, &origin, &attachment, &clip)) {
        QPixmap pixmap = QStyleSheetStyle::loadPixmap(uri, object);
        if (!uri.isEmpty() && pixmap.isNull())
            qWarning("Could not create pixmap from %s", qPrintable(QDir::toNativeSeparators(uri)));
        bg = new QStyleSheetBackgroundData(brush, pixmap, repeat, alignment, origin, attachment, clip);
    }

    QBrush sfg, fg, pfg;
    QBrush sbg, abg;
    if (v.extractPalette(&fg, &sfg, &sbg, &abg, &pfg))
        pal = new QStyleSheetPaletteData(fg, sfg, sbg, abg, pfg);

    QIcon imgIcon;
    alignment = Qt::AlignCenter;
    QSize imgSize;
    if (v.extractImage(&imgIcon, &alignment, &imgSize))
        img = new QStyleSheetImageData(imgIcon, alignment, imgSize);

    QIcon icon;
    QSize size;
    if (v.extractIcon(&icon, &size))
        iconPtr = new QStyleSheetImageData(icon, Qt::AlignCenter, size);

    int adj = -255;
    hasFont = v.extractFont(&font, &adj);

#if QT_CONFIG(tooltip)
    if (object && qstrcmp(object->metaObject()->className(), "QTipLabel") == 0)
        palette = QToolTip::palette();
#endif

    for (int i = 0; i < declarations.size(); i++) {
        const Declaration& decl = declarations.at(i);
        if (decl.d->propertyId == BorderImage) {
            QString uri;
            QCss::TileMode horizStretch, vertStretch;
            int cuts[4];

            decl.borderImageValue(&uri, cuts, &horizStretch, &vertStretch);
            if (uri.isEmpty() || uri == "none"_L1) {
                if (bd && bd->bi)
                    bd->bi->pixmap = QPixmap();
            } else {
                if (!bd)
                    bd = new QStyleSheetBorderData;
                if (!bd->bi)
                    bd->bi = new QStyleSheetBorderImageData;

                QStyleSheetBorderImageData *bi = bd->bi;
                bi->pixmap = QStyleSheetStyle::loadPixmap(uri, object);
                for (int i = 0; i < 4; i++)
                    bi->cuts[i] = cuts[i];
                bi->horizStretch = horizStretch;
                bi->vertStretch = vertStretch;
            }
        } else if (decl.d->propertyId == QtBackgroundRole) {
            if (bg && bg->brush.style() != Qt::NoBrush)
                continue;
            int role = decl.d->values.at(0).variant.toInt();
            if (role >= Value_FirstColorRole && role <= Value_LastColorRole)
                defaultBackground = palette.color((QPalette::ColorRole)(role-Value_FirstColorRole));
        } else if (decl.d->property.startsWith("qproperty-"_L1, Qt::CaseInsensitive)) {
            // intentionally left blank...
        } else if (decl.d->propertyId == UnknownProperty) {
            bool knownStyleHint = false;
            for (int i = 0; i < numKnownStyleHints; i++) {
                QLatin1StringView styleHint(knownStyleHints[i]);
                if (decl.d->property.compare(styleHint) == 0) {
                    QString hintName = QString(styleHint);
                    QVariant hintValue;
                    if (hintName.endsWith("alignment"_L1)) {
                        hintValue = (int) decl.alignmentValue();
                    } else if (hintName.endsWith("color"_L1)) {
                        hintValue = (int) decl.colorValue().rgba();
                    } else if (hintName.endsWith("size"_L1)) {
                        // Check only for the 'em' case
                        const QString valueString = decl.d->values.at(0).variant.toString();
                        const bool isEmSize = valueString.endsWith(u"em", Qt::CaseInsensitive);
                        if (isEmSize || valueString.endsWith(u"ex", Qt::CaseInsensitive)) {
                            // 1em == size of font; 1ex == xHeight of font
                            // See lengthValueFromData helper in qcssparser.cpp
                            QFont fontForSize(font);
                            // if no font is specified, then use the widget font if possible
                            if (const QWidget *widget; !hasFont && (widget = qobject_cast<const QWidget*>(object)))
                                fontForSize = widget->font();

                            const QFontMetrics fontMetrics(fontForSize);
                            qreal pixelSize = isEmSize ? fontMetrics.height() : fontMetrics.xHeight();

                            // Transform size according to the 'em'/'ex' value
                            qreal emexSize = {};
                            if (decl.realValue(&emexSize, isEmSize ? "em" : "ex") && emexSize > 0) {
                                pixelSize *= emexSize;
                                const QSizeF newSize(pixelSize, pixelSize);
                                decl.d->parsed = QVariant::fromValue<QSizeF>(newSize);
                                hintValue = newSize;
                            } else {
                                qWarning("Invalid '%s' size for %s. Skipping.",
                                         isEmSize ? "em" : "ex", qPrintable(valueString));
                            }
                        } else {
                            // Normal case where we receive a 'px' or 'pt' unit
                            hintValue = decl.sizeValue();
                        }
                    } else if (hintName.endsWith("icon"_L1)) {
                        hintValue = decl.iconValue();
                    } else if (hintName == "button-layout"_L1 && decl.d->values.size() != 0
                               && decl.d->values.at(0).type == QCss::Value::String) {
                        hintValue = subControlLayout(decl.d->values.at(0).variant.toString());
                    } else {
                        int integer;
                        decl.intValue(&integer);
                        hintValue = integer;
                    }
                    styleHints[decl.d->property] = hintValue;
                    knownStyleHint = true;
                    break;
                }
            }
            if (!knownStyleHint)
                qWarning("Unknown property %s", qPrintable(decl.d->property));
        }
    }

    if (hasBorder()) {
        if (const QWidget *widget = qobject_cast<const QWidget *>(object)) {
            QStyleSheetStyle *style = const_cast<QStyleSheetStyle *>(globalStyleSheetStyle);
            if (!style)
                style = qt_styleSheet(widget->style());
            if (style)
                fixupBorder(style->nativeFrameWidth(widget));
        }
        if (border()->hasBorderImage())
            defaultBackground = QBrush();
    }
}

QRect QRenderRule::borderRect(const QRect& r) const
{
    if (!hasBox())
        return r;
    const int* m = box()->margins;
    return r.adjusted(m[LeftEdge], m[TopEdge], -m[RightEdge], -m[BottomEdge]);
}

QRect QRenderRule::outlineRect(const QRect& r) const
{
    QRect br = borderRect(r);
    if (!hasOutline())
        return br;
    const int *b = outline()->borders;
    return r.adjusted(b[LeftEdge], b[TopEdge], -b[RightEdge], -b[BottomEdge]);
}

QRect QRenderRule::paddingRect(const QRect& r) const
{
    QRect br = borderRect(r);
    if (!hasBorder())
        return br;
    const int *b = border()->borders;
    return br.adjusted(b[LeftEdge], b[TopEdge], -b[RightEdge], -b[BottomEdge]);
}

QRect QRenderRule::contentsRect(const QRect& r) const
{
    QRect pr = paddingRect(r);
    if (!hasBox())
        return pr;
    const int *p = box()->paddings;
    return pr.adjusted(p[LeftEdge], p[TopEdge], -p[RightEdge], -p[BottomEdge]);
}

QRect QRenderRule::boxRect(const QRect& cr, int flags) const
{
    QRect r = cr;
    if (hasBox()) {
        if (flags & Margin) {
            const int *m = box()->margins;
            r.adjust(-m[LeftEdge], -m[TopEdge], m[RightEdge], m[BottomEdge]);
        }
        if (flags & Padding) {
            const int *p = box()->paddings;
            r.adjust(-p[LeftEdge], -p[TopEdge], p[RightEdge], p[BottomEdge]);
        }
    }
    if (hasBorder() && (flags & Border)) {
        const int *b = border()->borders;
        r.adjust(-b[LeftEdge], -b[TopEdge], b[RightEdge], b[BottomEdge]);
    }
    return r;
}

QSize QRenderRule::boxSize(const QSize &cs, int flags) const
{
    QSize bs = boxRect(QRect(QPoint(0, 0), cs), flags).size();
    if (cs.width() < 0) bs.setWidth(-1);
    if (cs.height() < 0) bs.setHeight(-1);
    return bs;
}

void QRenderRule::fixupBorder(int nativeWidth)
{
    if (bd == nullptr)
        return;

    if (!bd->hasBorderImage() || bd->bi->pixmap.isNull()) {
        bd->bi = nullptr;
        // ignore the color, border of edges that have none border-style
        QBrush color = pal ? pal->foreground : QBrush();
        const bool hasRadius = bd->radii[0].isValid() || bd->radii[1].isValid()
                               || bd->radii[2].isValid() || bd->radii[3].isValid();
        for (int i = 0; i < 4; i++) {
            if ((bd->styles[i] == BorderStyle_Native) && hasRadius)
                bd->styles[i] = BorderStyle_None;

            switch (bd->styles[i]) {
            case BorderStyle_None:
                // border-style: none forces width to be 0
                bd->colors[i] = QBrush();
                bd->borders[i] = 0;
                break;
            case BorderStyle_Native:
                if (bd->borders[i] == 0)
                    bd->borders[i] = nativeWidth;
                Q_FALLTHROUGH();
            default:
                if (bd->colors[i].style() == Qt::NoBrush) // auto-acquire 'color'
                    bd->colors[i] = color;
                break;
            }
        }

        return;
    }

    // inspect the border image
    QStyleSheetBorderImageData *bi = bd->bi;
    if (bi->cuts[0] == -1) {
        for (int i = 0; i < 4; i++) // assume, cut = border
            bi->cuts[i] = int(border()->borders[i]);
    }
}

void QRenderRule::drawBorderImage(QPainter *p, const QRect& rect)
{
    setClip(p, rect);
    static const Qt::TileRule tileMode2TileRule[] = {
        Qt::StretchTile, Qt::RoundTile, Qt::StretchTile, Qt::RepeatTile, Qt::StretchTile };

    const QStyleSheetBorderImageData *borderImageData = border()->borderImage();
    const int *targetBorders = border()->borders;
    const int *sourceBorders = borderImageData->cuts;
    QMargins sourceMargins(sourceBorders[LeftEdge], sourceBorders[TopEdge],
                           sourceBorders[RightEdge], sourceBorders[BottomEdge]);
    QMargins targetMargins(targetBorders[LeftEdge], targetBorders[TopEdge],
                           targetBorders[RightEdge], targetBorders[BottomEdge]);

    bool wasSmoothPixmapTransform = p->renderHints() & QPainter::SmoothPixmapTransform;
    p->setRenderHint(QPainter::SmoothPixmapTransform);
    qDrawBorderPixmap(p, rect, targetMargins, borderImageData->pixmap,
                      QRect(QPoint(), borderImageData->pixmap.size()), sourceMargins,
                      QTileRules(tileMode2TileRule[borderImageData->horizStretch], tileMode2TileRule[borderImageData->vertStretch]));
    p->setRenderHint(QPainter::SmoothPixmapTransform, wasSmoothPixmapTransform);
    unsetClip(p);
}

QRect QRenderRule::originRect(const QRect &rect, Origin origin) const
{
    switch (origin) {
    case Origin_Padding:
        return paddingRect(rect);
    case Origin_Border:
        return borderRect(rect);
    case Origin_Content:
        return contentsRect(rect);
    case Origin_Margin:
    default:
        return rect;
    }
}

void QRenderRule::drawBackgroundImage(QPainter *p, const QRect &rect, QPoint off)
{
    if (!hasBackground())
        return;

    const QPixmap& bgp = background()->pixmap;
    if (bgp.isNull())
        return;

    setClip(p, borderRect(rect));

    if (background()->origin != background()->clip) {
        p->save();
        p->setClipRect(originRect(rect, background()->clip), Qt::IntersectClip);
    }

    if (background()->attachment == Attachment_Fixed)
        off = QPoint(0, 0);

    QSize bgpSize = bgp.size() / bgp.devicePixelRatio();
    int bgpHeight = bgpSize.height();
    int bgpWidth = bgpSize.width();
    QRect r = originRect(rect, background()->origin);
    QRect aligned = QStyle::alignedRect(Qt::LeftToRight, background()->position, bgpSize, r);
    QRect inter = aligned.translated(-off).intersected(r);

    switch (background()->repeat) {
    case Repeat_Y:
        p->drawTiledPixmap(inter.x(), r.y(), inter.width(), r.height(), bgp,
                           inter.x() - aligned.x() + off.x(),
                           bgpHeight - int(aligned.y() - r.y()) % bgpHeight + off.y());
        break;
    case Repeat_X:
        p->drawTiledPixmap(r.x(), inter.y(), r.width(), inter.height(), bgp,
                           bgpWidth - int(aligned.x() - r.x())%bgpWidth + off.x(),
                           inter.y() - aligned.y() + off.y());
        break;
    case Repeat_XY:
        p->drawTiledPixmap(r, bgp,
                            QPoint(bgpWidth - int(aligned.x() - r.x())% bgpWidth + off.x(),
                                   bgpHeight - int(aligned.y() - r.y())%bgpHeight + off.y()));
        break;
    case Repeat_None:
    default:
        p->drawPixmap(inter.x(), inter.y(), bgp, inter.x() - aligned.x() + off.x(),
                      inter.y() - aligned.y() + off.y(), bgp.width() , bgp.height());
        break;
    }


    if (background()->origin != background()->clip)
        p->restore();

    unsetClip(p);
}

void QRenderRule::drawOutline(QPainter *p, const QRect &rect)
{
    if (!hasOutline())
        return;

    bool wasAntialiased = p->renderHints() & QPainter::Antialiasing;
    p->setRenderHint(QPainter::Antialiasing);
    qDrawBorder(p, rect, ou->styles, ou->borders, ou->colors, ou->radii);
    p->setRenderHint(QPainter::Antialiasing, wasAntialiased);
}

void QRenderRule::drawBorder(QPainter *p, const QRect& rect)
{
    if (!hasBorder())
        return;

    if (border()->hasBorderImage()) {
        drawBorderImage(p, rect);
        return;
    }

    bool wasAntialiased = p->renderHints() & QPainter::Antialiasing;
    p->setRenderHint(QPainter::Antialiasing);
    qDrawBorder(p, rect, bd->styles, bd->borders, bd->colors, bd->radii);
    p->setRenderHint(QPainter::Antialiasing, wasAntialiased);
}

QPainterPath QRenderRule::borderClip(QRect r)
{
    if (!hasBorder())
        return QPainterPath();

    QSize tlr, trr, blr, brr;
    qNormalizeRadii(r, bd->radii, &tlr, &trr, &blr, &brr);
    if (tlr.isNull() && trr.isNull() && blr.isNull() && brr.isNull())
        return QPainterPath();

    const QRectF rect(r);
    const int *borders = border()->borders;
    QPainterPath path;
    qreal curY = rect.y() + borders[TopEdge]/2.0;
    path.moveTo(rect.x() + tlr.width(), curY);
    path.lineTo(rect.right() - trr.width(), curY);
    qreal curX = rect.right() - borders[RightEdge]/2.0;
    path.arcTo(curX - 2*trr.width() + borders[RightEdge], curY,
               trr.width()*2 - borders[RightEdge], trr.height()*2 - borders[TopEdge], 90, -90);

    path.lineTo(curX, rect.bottom() - brr.height());
    curY = rect.bottom() - borders[BottomEdge]/2.0;
    path.arcTo(curX - 2*brr.width() + borders[RightEdge], curY - 2*brr.height() + borders[BottomEdge],
               brr.width()*2 - borders[RightEdge], brr.height()*2 - borders[BottomEdge], 0, -90);

    path.lineTo(rect.x() + blr.width(), curY);
    curX = rect.left() + borders[LeftEdge]/2.0;
    path.arcTo(curX, rect.bottom() - 2*blr.height() + borders[BottomEdge]/2.0,
               blr.width()*2 - borders[LeftEdge], blr.height()*2 - borders[BottomEdge], 270, -90);

    path.lineTo(curX, rect.top() + tlr.height());
    path.arcTo(curX, rect.top() + borders[TopEdge]/2.0,
               tlr.width()*2 - borders[LeftEdge], tlr.height()*2 - borders[TopEdge], 180, -90);

    path.closeSubpath();
    return path;
}

/*! \internal
  Clip the painter to the border (in case we are using radius border)
 */
void QRenderRule::setClip(QPainter *p, const QRect &rect)
{
    if (clipset++)
        return;
    clipPath = borderClip(rect);
    if (!clipPath.isEmpty()) {
        p->save();
        p->setClipPath(clipPath, Qt::IntersectClip);
    }
}

void QRenderRule::unsetClip(QPainter *p)
{
    if (--clipset)
        return;
    if (!clipPath.isEmpty())
        p->restore();
}

void QRenderRule::drawBackground(QPainter *p, const QRect& rect, const QPoint& off)
{
    QBrush brush = hasBackground() ? background()->brush : QBrush();
    if (brush.style() == Qt::NoBrush)
        brush = defaultBackground;

    if (brush.style() != Qt::NoBrush) {
        Origin origin = hasBackground() ? background()->clip : Origin_Border;
        // ### fix for  gradients
        const QPainterPath &borderPath = borderClip(originRect(rect, origin));
        if (!borderPath.isEmpty()) {
            // Drawn instead of being used as clipping path for better visual quality
            bool wasAntialiased = p->renderHints() & QPainter::Antialiasing;
            p->setRenderHint(QPainter::Antialiasing);
            p->fillPath(borderPath, brush);
            p->setRenderHint(QPainter::Antialiasing, wasAntialiased);
        } else {
            p->fillRect(originRect(rect, origin), brush);
        }
    }

    drawBackgroundImage(p, rect, off);
}

void QRenderRule::drawFrame(QPainter *p, const QRect& rect)
{
    drawBackground(p, rect);
    if (hasBorder())
        drawBorder(p, borderRect(rect));
}

void QRenderRule::drawImage(QPainter *p, const QRect &rect)
{
    if (!hasImage())
        return;
    img->icon.paint(p, rect, img->alignment);
}

void QRenderRule::drawRule(QPainter *p, const QRect& rect)
{
    drawFrame(p, rect);
    drawImage(p, contentsRect(rect));
}

// *shudder* , *horror*, *whoa* <-- what you might feel when you see the functions below
void QRenderRule::configurePalette(QPalette *p, QPalette::ColorRole fr, QPalette::ColorRole br)
{
    if (bg && bg->brush.style() != Qt::NoBrush) {
        if (br != QPalette::NoRole)
            p->setBrush(br, bg->brush);
        p->setBrush(QPalette::Window, bg->brush);
        if (bg->brush.style() == Qt::SolidPattern) {
            p->setBrush(QPalette::Light, bg->brush.color().lighter(115));
            p->setBrush(QPalette::Midlight, bg->brush.color().lighter(107));
            p->setBrush(QPalette::Dark, bg->brush.color().darker(150));
            p->setBrush(QPalette::Shadow, bg->brush.color().darker(300));
        }
    }

    if (!hasPalette())
        return;

    if (pal->foreground.style() != Qt::NoBrush) {
        if (fr != QPalette::NoRole)
            p->setBrush(fr, pal->foreground);
        p->setBrush(QPalette::WindowText, pal->foreground);
        p->setBrush(QPalette::Text, pal->foreground);
    }
    if (pal->selectionBackground.style() != Qt::NoBrush)
        p->setBrush(QPalette::Highlight, pal->selectionBackground);
    if (pal->selectionForeground.style() != Qt::NoBrush)
        p->setBrush(QPalette::HighlightedText, pal->selectionForeground);
    if (pal->alternateBackground.style() != Qt::NoBrush)
        p->setBrush(QPalette::AlternateBase, pal->alternateBackground);
}

void setDefault(QPalette *palette, QPalette::ColorGroup group, QPalette::ColorRole role,
                const QBrush &defaultBrush, const QWidget *widget)
{
    const QPalette &widgetPalette = widget->palette();
    if (widgetPalette.isBrushSet(group, role))
        palette->setBrush(group, role, widgetPalette.brush(group, role));
    else
        palette->setBrush(group, role, defaultBrush);
}

void QRenderRule::configurePalette(QPalette *p, QPalette::ColorGroup cg, const QWidget *w, bool embedded)
{
    if (bg && bg->brush.style() != Qt::NoBrush) {
        p->setBrush(cg, QPalette::Base, bg->brush); // for windows, windowxp
        p->setBrush(cg, QPalette::Button, bg->brush); // for plastique
        p->setBrush(cg, w->backgroundRole(), bg->brush);
        p->setBrush(cg, QPalette::Window, bg->brush);
    }

    if (embedded) {
        /* For embedded widgets (ComboBox, SpinBox and ScrollArea) we want the embedded widget
         * to be transparent when we have a transparent background or border image */
        if ((hasBackground() && background()->isTransparent())
            || (hasBorder() && border()->hasBorderImage() && !border()->borderImage()->pixmap.isNull()))
            p->setBrush(cg, w->backgroundRole(), Qt::NoBrush);
    }

    if (!hasPalette())
        return;

    if (pal->foreground.style() != Qt::NoBrush) {
        setDefault(p, cg, QPalette::ButtonText, pal->foreground, w);
        setDefault(p, cg, w->foregroundRole(), pal->foreground, w);
        setDefault(p, cg, QPalette::WindowText, pal->foreground, w);
        setDefault(p, cg, QPalette::Text, pal->foreground, w);
        QColor phColor(pal->foreground.color());
        phColor.setAlpha((phColor.alpha() + 1) / 2);
        QBrush placeholder = pal->foreground;
        placeholder.setColor(phColor);
        setDefault(p, cg, QPalette::PlaceholderText, placeholder, w);
    }
    if (pal->selectionBackground.style() != Qt::NoBrush)
        p->setBrush(cg, QPalette::Highlight, pal->selectionBackground);
    if (pal->selectionForeground.style() != Qt::NoBrush)
        p->setBrush(cg, QPalette::HighlightedText, pal->selectionForeground);
    if (pal->alternateBackground.style() != Qt::NoBrush)
        p->setBrush(cg, QPalette::AlternateBase, pal->alternateBackground);
    if (pal->placeholderForeground.style() != Qt::NoBrush)
        p->setBrush(cg, QPalette::PlaceholderText, pal->placeholderForeground);
}

bool QRenderRule::hasModification() const
{
    return hasPalette() ||
           hasBackground() ||
           hasGradientBackground() ||
           !hasNativeBorder() ||
           !hasNativeOutline() ||
           hasBox() ||
           hasPosition() ||
           hasGeometry() ||
           hasImage() ||
           hasFont ||
           !styleHints.isEmpty();
}

///////////////////////////////////////////////////////////////////////////////
// Style rules
#define OBJECT_PTR(x) (static_cast<QObject *>(x.ptr))

static inline QObject *parentObject(const QObject *obj)
{
#if QT_CONFIG(tooltip)
    if (qobject_cast<const QLabel *>(obj) && qstrcmp(obj->metaObject()->className(), "QTipLabel") == 0) {
        QObject *p = qvariant_cast<QObject *>(obj->property("_q_stylesheet_parent"));
        if (p)
            return p;
    }
#endif
    return obj->parent();
}

class QStyleSheetStyleSelector : public StyleSelector
{
public:
    QStyleSheetStyleSelector() { }

    QStringList nodeNames(NodePtr node) const override
    {
        if (isNullNode(node))
            return QStringList();
        const QMetaObject *metaObject = OBJECT_PTR(node)->metaObject();
#if QT_CONFIG(tooltip)
        if (qstrcmp(metaObject->className(), "QTipLabel") == 0)
            return QStringList("QToolTip"_L1);
#endif
        QStringList result;
        do {
            result += QString::fromLatin1(metaObject->className()).replace(u':', u'-');
            metaObject = metaObject->superClass();
        } while (metaObject != nullptr);
        return result;
    }
    QString attributeValue(NodePtr node, const QCss::AttributeSelector& aSelector) const override
    {
        if (isNullNode(node))
            return QString();

        const QString &name = aSelector.name;
        QHash<QString, QString> &cache = m_attributeCache[OBJECT_PTR(node)];
        QHash<QString, QString>::const_iterator cacheIt = cache.constFind(name);
        if (cacheIt != cache.constEnd())
            return cacheIt.value();

        QVariant value;
        QString valueStr;
        QObject *obj = OBJECT_PTR(node);
        const int propertyIndex = obj->metaObject()->indexOfProperty(name.toLatin1());
        if (propertyIndex == -1) {
            value = obj->property(name.toLatin1()); // might be a dynamic property
            if (!value.isValid()) {
                if (name == "class"_L1) {
                    QString className = QString::fromLatin1(obj->metaObject()->className());
                    if (className.contains(u':'))
                        className.replace(u':', u'-');
                    valueStr = className;
                } else if (name == "style"_L1) {
                    QWidget *w = qobject_cast<QWidget *>(obj);
                    QStyleSheetStyle *proxy = w ? qt_styleSheet(w->style()) : nullptr;
                    if (proxy)
                        valueStr = QString::fromLatin1(proxy->baseStyle()->metaObject()->className());
                }
            }
        } else {
            const QMetaProperty property = obj->metaObject()->property(propertyIndex);
            value = property.read(obj);
            // support Qt 5 selector syntax, which required the integer enum value
            if (property.isEnumType()) {
                bool isNumber;
                aSelector.value.toInt(&isNumber);
                if (isNumber)
                    value.convert(QMetaType::fromType<int>());
            }
        }
        if (value.isValid()) {
            valueStr = (value.userType() == QMetaType::QStringList
                        || value.userType() == QMetaType::QVariantList)
                        ? value.toStringList().join(u' ')
                        : value.toString();
        }
        cache[name] = valueStr;
        return valueStr;
    }
    bool nodeNameEquals(NodePtr node, const QString& nodeName) const override
    {
        if (isNullNode(node))
            return false;
        const QMetaObject *metaObject = OBJECT_PTR(node)->metaObject();
#if QT_CONFIG(tooltip)
        if (qstrcmp(metaObject->className(), "QTipLabel") == 0)
            return nodeName == "QToolTip"_L1;
#endif
        do {
            const auto *uc = reinterpret_cast<const char16_t *>(nodeName.constData());
            const auto *e = uc + nodeName.size();
            const uchar *c = (const uchar *)metaObject->className();
            while (*c && uc != e && (*uc == *c || (*c == ':' && *uc == '-'))) {
                ++uc;
                ++c;
            }
            if (uc == e && !*c)
                return true;
            metaObject = metaObject->superClass();
        } while (metaObject != nullptr);
        return false;
    }
    bool hasAttributes(NodePtr) const override
    { return true; }
    QStringList nodeIds(NodePtr node) const override
    { return isNullNode(node) ? QStringList() : QStringList(OBJECT_PTR(node)->objectName()); }
    bool isNullNode(NodePtr node) const override
    { return node.ptr == nullptr; }
    NodePtr parentNode(NodePtr node) const override
    { NodePtr n; n.ptr = isNullNode(node) ? nullptr : parentObject(OBJECT_PTR(node)); return n; }
    NodePtr previousSiblingNode(NodePtr) const override
    { NodePtr n; n.ptr = nullptr; return n; }
    NodePtr duplicateNode(NodePtr node) const override
    { return node; }
    void freeNode(NodePtr) const override
    { }

private:
    mutable QHash<const QObject *, QHash<QString, QString> > m_attributeCache;
};

QList<QCss::StyleRule> QStyleSheetStyle::styleRules(const QObject *obj) const
{
    QHash<const QObject *, QList<StyleRule>>::const_iterator cacheIt =
            styleSheetCaches->styleRulesCache.constFind(obj);
    if (cacheIt != styleSheetCaches->styleRulesCache.constEnd())
        return cacheIt.value();

    if (!initObject(obj)) {
        return QList<StyleRule>();
    }

    QStyleSheetStyleSelector styleSelector;

    StyleSheet defaultSs;
    QHash<const void *, StyleSheet>::const_iterator defaultCacheIt = styleSheetCaches->styleSheetCache.constFind(baseStyle());
    if (defaultCacheIt == styleSheetCaches->styleSheetCache.constEnd()) {
        defaultSs = getDefaultStyleSheet();
        QStyle *bs = baseStyle();
        styleSheetCaches->styleSheetCache.insert(bs, defaultSs);
        QObject::connect(bs, SIGNAL(destroyed(QObject*)), styleSheetCaches, SLOT(styleDestroyed(QObject*)), Qt::UniqueConnection);
    } else {
        defaultSs = defaultCacheIt.value();
    }
    styleSelector.styleSheets += defaultSs;

    if (!qApp->styleSheet().isEmpty()) {
        StyleSheet appSs;
        QHash<const void *, StyleSheet>::const_iterator appCacheIt = styleSheetCaches->styleSheetCache.constFind(qApp);
        if (appCacheIt == styleSheetCaches->styleSheetCache.constEnd()) {
            QString ss = qApp->styleSheet();
            if (ss.startsWith("file:///"_L1))
                ss.remove(0, 8);
            parser.init(ss, qApp->styleSheet() != ss);
            if (Q_UNLIKELY(!parser.parse(&appSs)))
                qWarning("Could not parse application stylesheet");
            appSs.origin = StyleSheetOrigin_Inline;
            appSs.depth = 1;
            styleSheetCaches->styleSheetCache.insert(qApp, appSs);
        } else {
            appSs = appCacheIt.value();
        }
        styleSelector.styleSheets += appSs;
    }

    QList<QCss::StyleSheet> objectSs;
    for (const QObject *o = obj; o; o = parentObject(o)) {
        QString styleSheet = o->property("styleSheet").toString();
        if (styleSheet.isEmpty())
            continue;
        StyleSheet ss;
        QHash<const void *, StyleSheet>::const_iterator objCacheIt = styleSheetCaches->styleSheetCache.constFind(o);
        if (objCacheIt == styleSheetCaches->styleSheetCache.constEnd()) {
            parser.init(styleSheet);
            if (!parser.parse(&ss)) {
                parser.init("* {"_L1 + styleSheet + u'}');
                if (Q_UNLIKELY(!parser.parse(&ss)))
                   qWarning() << "Could not parse stylesheet of object" << o;
            }
            ss.origin = StyleSheetOrigin_Inline;
            styleSheetCaches->styleSheetCache.insert(o, ss);
        } else {
            ss = objCacheIt.value();
        }
        objectSs.append(ss);
    }

    for (int i = 0; i < objectSs.size(); i++)
        objectSs[i].depth = objectSs.size() - i + 2;

    styleSelector.styleSheets += objectSs;

    StyleSelector::NodePtr n;
    n.ptr = const_cast<QObject *>(obj);
    QList<QCss::StyleRule> rules = styleSelector.styleRulesForNode(n);
    styleSheetCaches->styleRulesCache.insert(obj, rules);
    return rules;
}

/////////////////////////////////////////////////////////////////////////////////////////
// Rendering rules
static QList<Declaration> declarations(const QList<StyleRule> &styleRules, const QString &part,
                                       quint64 pseudoClass = PseudoClass_Unspecified)
{
    QList<Declaration> decls;
    for (int i = 0; i < styleRules.size(); i++) {
        const Selector& selector = styleRules.at(i).selectors.at(0);
        // Rules with pseudo elements don't cascade. This is an intentional
        // diversion for CSS
        if (part.compare(selector.pseudoElement(), Qt::CaseInsensitive) != 0)
            continue;
        quint64 negated = 0;
        quint64 cssClass = selector.pseudoClass(&negated);
        if ((pseudoClass == PseudoClass_Any) || (cssClass == PseudoClass_Unspecified)
            || ((((cssClass & pseudoClass) == cssClass)) && ((negated & pseudoClass) == 0)))
            decls += styleRules.at(i).declarations;
    }
    return decls;
}

int QStyleSheetStyle::nativeFrameWidth(const QWidget *w)
{
    QStyle *base = baseStyle();

#if QT_CONFIG(spinbox)
    if (qobject_cast<const QAbstractSpinBox *>(w))
        return base->pixelMetric(QStyle::PM_SpinBoxFrameWidth, nullptr, w);
#endif

#if QT_CONFIG(combobox)
    if (qobject_cast<const QComboBox *>(w))
        return base->pixelMetric(QStyle::PM_ComboBoxFrameWidth, nullptr, w);
#endif

#if QT_CONFIG(menu)
    if (qobject_cast<const QMenu *>(w))
        return base->pixelMetric(QStyle::PM_MenuPanelWidth, nullptr, w);
#endif

#if QT_CONFIG(menubar)
    if (qobject_cast<const QMenuBar *>(w))
        return base->pixelMetric(QStyle::PM_MenuBarPanelWidth, nullptr, w);
#endif
#ifndef QT_NO_FRAME
    if (const QFrame *frame = qobject_cast<const QFrame *>(w)) {
        if (frame->frameShape() == QFrame::NoFrame)
            return 0;
    }
#endif

    if (qstrcmp(w->metaObject()->className(), "QTipLabel") == 0)
        return base->pixelMetric(QStyle::PM_ToolTipLabelFrameWidth, nullptr, w);

    return base->pixelMetric(QStyle::PM_DefaultFrameWidth, nullptr, w);
}

static quint64 pseudoClass(QStyle::State state)
{
    quint64 pc = 0;
    if (state & QStyle::State_Enabled) {
        pc |= PseudoClass_Enabled;
        if (state & QStyle::State_MouseOver)
            pc |= PseudoClass_Hover;
    } else {
        pc |= PseudoClass_Disabled;
    }
    if (state & QStyle::State_Active)
        pc |= PseudoClass_Active;
    if (state & QStyle::State_Window)
        pc |= PseudoClass_Window;
    if (state & QStyle::State_Sunken)
        pc |= PseudoClass_Pressed;
    if (state & QStyle::State_HasFocus)
        pc |= PseudoClass_Focus;
    if (state & QStyle::State_On)
        pc |= (PseudoClass_On | PseudoClass_Checked);
    if (state & QStyle::State_Off)
        pc |= (PseudoClass_Off | PseudoClass_Unchecked);
    if (state & QStyle::State_NoChange)
        pc |= PseudoClass_Indeterminate;
    if (state & QStyle::State_Selected)
        pc |= PseudoClass_Selected;
    if (state & QStyle::State_Horizontal)
        pc |= PseudoClass_Horizontal;
    else
        pc |= PseudoClass_Vertical;
    if (state & (QStyle::State_Open | QStyle::State_On | QStyle::State_Sunken))
        pc |= PseudoClass_Open;
    else
        pc |= PseudoClass_Closed;
    if (state & QStyle::State_Children)
        pc |= PseudoClass_Children;
    if (state & QStyle::State_Sibling)
        pc |= PseudoClass_Sibling;
    if (state & QStyle::State_ReadOnly)
        pc |= PseudoClass_ReadOnly;
    if (state & QStyle::State_Item)
        pc |= PseudoClass_Item;
#ifdef QT_KEYPAD_NAVIGATION
    if (state & QStyle::State_HasEditFocus)
        pc |= PseudoClass_EditFocus;
#endif
    return pc;
}

static void qt_check_if_internal_object(const QObject **obj, int *element)
{
#if !QT_CONFIG(dockwidget)
    Q_UNUSED(obj);
    Q_UNUSED(element);
#else
    if (*obj && qstrcmp((*obj)->metaObject()->className(), "QDockWidgetTitleButton") == 0) {
        if ((*obj)->objectName() == "qt_dockwidget_closebutton"_L1) {
            *element = PseudoElement_DockWidgetCloseButton;
        } else if ((*obj)->objectName() == "qt_dockwidget_floatbutton"_L1) {
            *element = PseudoElement_DockWidgetFloatButton;
        }
        *obj = (*obj)->parent();
    }
#endif
}

QRenderRule QStyleSheetStyle::renderRule(const QObject *obj, int element, quint64 state) const
{
    qt_check_if_internal_object(&obj, &element);
    QHash<quint64, QRenderRule> &cache = styleSheetCaches->renderRulesCache[obj][element];
    QHash<quint64, QRenderRule>::const_iterator cacheIt = cache.constFind(state);
    if (cacheIt != cache.constEnd())
        return cacheIt.value();

    if (!initObject(obj))
        return QRenderRule();

    quint64 stateMask = 0;
    const QList<StyleRule> rules = styleRules(obj);
    for (int i = 0; i < rules.size(); i++) {
        const Selector& selector = rules.at(i).selectors.at(0);
        quint64 negated = 0;
        stateMask |= selector.pseudoClass(&negated);
        stateMask |= negated;
    }

    cacheIt = cache.constFind(state & stateMask);
    if (cacheIt != cache.constEnd()) {
        QRenderRule newRule = cacheIt.value();
        cache[state] = newRule;
        return newRule;
    }


    const QString part = QLatin1StringView(knownPseudoElements[element].name);
    QList<Declaration> decls = declarations(rules, part, state);
    QRenderRule newRule(decls, obj);
    cache[state] = newRule;
    if ((state & stateMask) != state)
        cache[state&stateMask] = newRule;
    return newRule;
}

QRenderRule QStyleSheetStyle::renderRule(const QObject *obj, const QStyleOption *opt, int pseudoElement) const
{
    quint64 extraClass = 0;
    QStyle::State state = opt ? opt->state : QStyle::State(QStyle::State_None);

    if (const QStyleOptionComplex *complex = qstyleoption_cast<const QStyleOptionComplex *>(opt)) {
        if (pseudoElement != PseudoElement_None) {
            // if not an active subcontrol, just pass enabled/disabled
            QStyle::SubControl subControl = knownPseudoElements[pseudoElement].subControl;

            if (!(complex->activeSubControls & subControl))
                state &= (QStyle::State_Enabled | QStyle::State_Horizontal | QStyle::State_HasFocus);
        }

        switch (pseudoElement) {
        case PseudoElement_ComboBoxDropDown:
        case PseudoElement_ComboBoxArrow:
            state |= (complex->state & (QStyle::State_On|QStyle::State_ReadOnly));
            break;
        case PseudoElement_SpinBoxUpButton:
        case PseudoElement_SpinBoxDownButton:
        case PseudoElement_SpinBoxUpArrow:
        case PseudoElement_SpinBoxDownArrow:
#if QT_CONFIG(spinbox)
            if (const QStyleOptionSpinBox *sb = qstyleoption_cast<const QStyleOptionSpinBox *>(opt)) {
                bool on = false;
                bool up = pseudoElement == PseudoElement_SpinBoxUpButton
                          || pseudoElement == PseudoElement_SpinBoxUpArrow;
                if ((sb->stepEnabled & QAbstractSpinBox::StepUpEnabled) && up)
                    on = true;
                else if ((sb->stepEnabled & QAbstractSpinBox::StepDownEnabled) && !up)
                    on = true;
                state |= (on ? QStyle::State_On : QStyle::State_Off);
            }
#endif // QT_CONFIG(spinbox)
            break;
        case PseudoElement_GroupBoxTitle:
            state |= (complex->state & (QStyle::State_MouseOver | QStyle::State_Sunken));
            break;
        case PseudoElement_ToolButtonMenu:
        case PseudoElement_ToolButtonMenuArrow:
        case PseudoElement_ToolButtonMenuIndicator:
            state |= complex->state & QStyle::State_MouseOver;
            if (complex->state & QStyle::State_Sunken ||
                complex->activeSubControls & QStyle::SC_ToolButtonMenu)
                state |= QStyle::State_Sunken;
            break;
        case PseudoElement_SliderGroove:
            state |= complex->state & QStyle::State_MouseOver;
            break;
        default:
            break;
        }

        if (const QStyleOptionComboBox *combo = qstyleoption_cast<const QStyleOptionComboBox *>(opt)) {
            // QStyle::State_On is set when the popup is being shown
            // Propagate EditField Pressed state
            if (pseudoElement == PseudoElement_None
                && (complex->activeSubControls & QStyle::SC_ComboBoxEditField)
                && (!(state & QStyle::State_MouseOver))) {
                state |= QStyle::State_Sunken;
            }

            if (!combo->frame)
                extraClass |= PseudoClass_Frameless;
            if (!combo->editable)
                extraClass |= PseudoClass_ReadOnly;
            else
                extraClass |= PseudoClass_Editable;
#if QT_CONFIG(spinbox)
        } else if (const QStyleOptionSpinBox *spin = qstyleoption_cast<const QStyleOptionSpinBox *>(opt)) {
            if (!spin->frame)
                extraClass |= PseudoClass_Frameless;
#endif // QT_CONFIG(spinbox)
        } else if (const QStyleOptionGroupBox *gb = qstyleoption_cast<const QStyleOptionGroupBox *>(opt)) {
            if (gb->features & QStyleOptionFrame::Flat)
                extraClass |= PseudoClass_Flat;
            if (gb->lineWidth == 0)
                extraClass |= PseudoClass_Frameless;
        } else if (const QStyleOptionTitleBar *tb = qstyleoption_cast<const QStyleOptionTitleBar *>(opt)) {
            if (tb->titleBarState & Qt::WindowMinimized) {
                extraClass |= PseudoClass_Minimized;
            }
            else if (tb->titleBarState & Qt::WindowMaximized)
                extraClass |= PseudoClass_Maximized;
        }
    } else {
        // handle simple style options
        if (const QStyleOptionMenuItem *mi = qstyleoption_cast<const QStyleOptionMenuItem *>(opt)) {
            if (mi->menuItemType == QStyleOptionMenuItem::DefaultItem)
                extraClass |= PseudoClass_Default;
            if (mi->checkType == QStyleOptionMenuItem::Exclusive)
                extraClass |= PseudoClass_Exclusive;
            else if (mi->checkType == QStyleOptionMenuItem::NonExclusive)
                extraClass |= PseudoClass_NonExclusive;
            if (mi->checkType != QStyleOptionMenuItem::NotCheckable)
                extraClass |= (mi->checked) ? (PseudoClass_On|PseudoClass_Checked)
                                            : (PseudoClass_Off|PseudoClass_Unchecked);
        } else if (const QStyleOptionHeader *hdr = qstyleoption_cast<const QStyleOptionHeader *>(opt)) {
            if (hdr->position == QStyleOptionHeader::OnlyOneSection)
                extraClass |= PseudoClass_OnlyOne;
            else if (hdr->position == QStyleOptionHeader::Beginning)
                extraClass |= PseudoClass_First;
            else if (hdr->position == QStyleOptionHeader::End)
                extraClass |= PseudoClass_Last;
            else if (hdr->position == QStyleOptionHeader::Middle)
                extraClass |= PseudoClass_Middle;

            if (hdr->selectedPosition == QStyleOptionHeader::NextAndPreviousAreSelected)
                extraClass |= (PseudoClass_NextSelected | PseudoClass_PreviousSelected);
            else if (hdr->selectedPosition == QStyleOptionHeader::NextIsSelected)
                extraClass |= PseudoClass_NextSelected;
            else if (hdr->selectedPosition == QStyleOptionHeader::PreviousIsSelected)
                extraClass |= PseudoClass_PreviousSelected;
#if QT_CONFIG(tabwidget)
        } else if (const QStyleOptionTabWidgetFrame *tab = qstyleoption_cast<const QStyleOptionTabWidgetFrame *>(opt)) {
            switch (tab->shape) {
                case QTabBar::RoundedNorth:
                case QTabBar::TriangularNorth:
                    extraClass |= PseudoClass_Top;
                    break;
                case QTabBar::RoundedSouth:
                case QTabBar::TriangularSouth:
                    extraClass |= PseudoClass_Bottom;
                    break;
                case QTabBar::RoundedEast:
                case QTabBar::TriangularEast:
                    extraClass |= PseudoClass_Right;
                    break;
                case QTabBar::RoundedWest:
                case QTabBar::TriangularWest:
                    extraClass |= PseudoClass_Left;
                    break;
                default:
                    break;
            }
#endif
#if QT_CONFIG(tabbar)
        } else if (const QStyleOptionTab *tab = qstyleoption_cast<const QStyleOptionTab *>(opt)) {
            if (tab->position == QStyleOptionTab::OnlyOneTab)
                extraClass |= PseudoClass_OnlyOne;
            else if (tab->position == QStyleOptionTab::Beginning)
                extraClass |= PseudoClass_First;
            else if (tab->position == QStyleOptionTab::End)
                extraClass |= PseudoClass_Last;
            else if (tab->position == QStyleOptionTab::Middle)
                extraClass |= PseudoClass_Middle;

            if (tab->selectedPosition == QStyleOptionTab::NextIsSelected)
                extraClass |= PseudoClass_NextSelected;
            else if (tab->selectedPosition == QStyleOptionTab::PreviousIsSelected)
                extraClass |= PseudoClass_PreviousSelected;

            switch (tab->shape) {
                case QTabBar::RoundedNorth:
                case QTabBar::TriangularNorth:
                    extraClass |= PseudoClass_Top;
                    break;
                case QTabBar::RoundedSouth:
                case QTabBar::TriangularSouth:
                    extraClass |= PseudoClass_Bottom;
                    break;
                case QTabBar::RoundedEast:
                case QTabBar::TriangularEast:
                    extraClass |= PseudoClass_Right;
                    break;
                case QTabBar::RoundedWest:
                case QTabBar::TriangularWest:
                    extraClass |= PseudoClass_Left;
                    break;
                default:
                    break;
            }
#endif // QT_CONFIG(tabbar)
        } else if (const QStyleOptionButton *btn = qstyleoption_cast<const QStyleOptionButton *>(opt)) {
            if (btn->features & QStyleOptionButton::Flat)
                extraClass |= PseudoClass_Flat;
            if (btn->features & QStyleOptionButton::DefaultButton)
                extraClass |= PseudoClass_Default;
        } else if (const QStyleOptionFrame *frm = qstyleoption_cast<const QStyleOptionFrame *>(opt)) {
            if (frm->lineWidth == 0)
                extraClass |= PseudoClass_Frameless;
            if (frm->features & QStyleOptionFrame::Flat)
                extraClass |= PseudoClass_Flat;
        }
#if QT_CONFIG(toolbar)
        else if (const QStyleOptionToolBar *tb = qstyleoption_cast<const QStyleOptionToolBar *>(opt)) {
            if (tb->toolBarArea == Qt::LeftToolBarArea)
                extraClass |= PseudoClass_Left;
            else if (tb->toolBarArea == Qt::RightToolBarArea)
                extraClass |= PseudoClass_Right;
            else if (tb->toolBarArea == Qt::TopToolBarArea)
                extraClass |= PseudoClass_Top;
            else if (tb->toolBarArea == Qt::BottomToolBarArea)
                extraClass |= PseudoClass_Bottom;

            if (tb->positionWithinLine == QStyleOptionToolBar::Beginning)
                extraClass |= PseudoClass_First;
            else if (tb->positionWithinLine == QStyleOptionToolBar::Middle)
                extraClass |= PseudoClass_Middle;
            else if (tb->positionWithinLine == QStyleOptionToolBar::End)
                extraClass |= PseudoClass_Last;
            else if (tb->positionWithinLine == QStyleOptionToolBar::OnlyOne)
                extraClass |= PseudoClass_OnlyOne;
        }
#endif // QT_CONFIG(toolbar)
#if QT_CONFIG(toolbox)
        else if (const QStyleOptionToolBox *tb = qstyleoption_cast<const QStyleOptionToolBox *>(opt)) {
            if (tb->position == QStyleOptionToolBox::OnlyOneTab)
                extraClass |= PseudoClass_OnlyOne;
            else if (tb->position == QStyleOptionToolBox::Beginning)
                extraClass |= PseudoClass_First;
            else if (tb->position == QStyleOptionToolBox::End)
                extraClass |= PseudoClass_Last;
            else if (tb->position == QStyleOptionToolBox::Middle)
                extraClass |= PseudoClass_Middle;

            if (tb->selectedPosition == QStyleOptionToolBox::NextIsSelected)
                extraClass |= PseudoClass_NextSelected;
            else if (tb->selectedPosition == QStyleOptionToolBox::PreviousIsSelected)
                extraClass |= PseudoClass_PreviousSelected;
        }
#endif // QT_CONFIG(toolbox)
#if QT_CONFIG(dockwidget)
        else if (const QStyleOptionDockWidget *dw = qstyleoption_cast<const QStyleOptionDockWidget *>(opt)) {
            if (dw->verticalTitleBar)
                extraClass |= PseudoClass_Vertical;
            else
                extraClass |= PseudoClass_Horizontal;
            if (dw->closable)
                extraClass |= PseudoClass_Closable;
            if (dw->floatable)
                extraClass |= PseudoClass_Floatable;
            if (dw->movable)
                extraClass |= PseudoClass_Movable;
        }
#endif // QT_CONFIG(dockwidget)
#if QT_CONFIG(itemviews)
        else if (const QStyleOptionViewItem *vopt = qstyleoption_cast<const QStyleOptionViewItem *>(opt)) {
            if (vopt->features & QStyleOptionViewItem::Alternate)
                extraClass |= PseudoClass_Alternate;
            if (vopt->viewItemPosition == QStyleOptionViewItem::OnlyOne)
                extraClass |= PseudoClass_OnlyOne;
            else if (vopt->viewItemPosition == QStyleOptionViewItem::Beginning)
                extraClass |= PseudoClass_First;
            else if (vopt->viewItemPosition == QStyleOptionViewItem::End)
                extraClass |= PseudoClass_Last;
            else if (vopt->viewItemPosition == QStyleOptionViewItem::Middle)
                extraClass |= PseudoClass_Middle;

        }
#endif
#if QT_CONFIG(textedit)
        else if (const QPlainTextEdit *edit = qobject_cast<const QPlainTextEdit *>(obj)) {
            extraClass |= (edit->isReadOnly() ? PseudoClass_ReadOnly : PseudoClass_Editable);
        }
        else if (const QTextEdit *edit = qobject_cast<const QTextEdit *>(obj)) {
            extraClass |= (edit->isReadOnly() ? PseudoClass_ReadOnly : PseudoClass_Editable);
        }
#endif
#if QT_CONFIG(lineedit)
        // LineEdit sets Sunken flag to indicate Sunken frame (argh)
        if (const QLineEdit *lineEdit = qobject_cast<const QLineEdit *>(obj)) {
            state &= ~QStyle::State_Sunken;
            if (lineEdit->hasFrame()) {
                extraClass &= ~PseudoClass_Frameless;
            } else {
                extraClass |= PseudoClass_Frameless;
            }
        } else
#endif
        if (const QFrame *frm = qobject_cast<const QFrame *>(obj)) {
            if (frm->lineWidth() == 0)
                extraClass |= PseudoClass_Frameless;
        }
    }

    return renderRule(obj, pseudoElement, pseudoClass(state) | extraClass);
}

bool QStyleSheetStyle::hasStyleRule(const QObject *obj, int part) const
{
    QHash<int, bool> &cache = styleSheetCaches->hasStyleRuleCache[obj];
    QHash<int, bool>::const_iterator cacheIt = cache.constFind(part);
    if (cacheIt != cache.constEnd())
        return cacheIt.value();

    if (!initObject(obj))
        return false;

    const QList<StyleRule> &rules = styleRules(obj);
    if (part == PseudoElement_None) {
        bool result = obj && !rules.isEmpty();
        cache[part] = result;
        return result;
    }

    auto pseudoElement = QLatin1StringView(knownPseudoElements[part].name);
    for (int i = 0; i < rules.size(); i++) {
        const Selector& selector = rules.at(i).selectors.at(0);
        if (pseudoElement.compare(selector.pseudoElement(), Qt::CaseInsensitive) == 0) {
            cache[part] = true;
            return true;
        }
    }

    cache[part] = false;
    return false;
}

static Origin defaultOrigin(int pe)
{
    switch (pe) {
    case PseudoElement_ScrollBarAddPage:
    case PseudoElement_ScrollBarSubPage:
    case PseudoElement_ScrollBarAddLine:
    case PseudoElement_ScrollBarSubLine:
    case PseudoElement_ScrollBarFirst:
    case PseudoElement_ScrollBarLast:
    case PseudoElement_GroupBoxTitle:
    case PseudoElement_GroupBoxIndicator: // never used
    case PseudoElement_ToolButtonMenu:
    case PseudoElement_SliderAddPage:
    case PseudoElement_SliderSubPage:
        return Origin_Border;

    case PseudoElement_SpinBoxUpButton:
    case PseudoElement_SpinBoxDownButton:
    case PseudoElement_PushButtonMenuIndicator:
    case PseudoElement_ComboBoxDropDown:
    case PseudoElement_ToolButtonMenuIndicator:
    case PseudoElement_MenuCheckMark:
    case PseudoElement_MenuIcon:
    case PseudoElement_MenuRightArrow:
        return Origin_Padding;

    case PseudoElement_Indicator:
    case PseudoElement_ExclusiveIndicator:
    case PseudoElement_ComboBoxArrow:
    case PseudoElement_ScrollBarSlider:
    case PseudoElement_ScrollBarUpArrow:
    case PseudoElement_ScrollBarDownArrow:
    case PseudoElement_ScrollBarLeftArrow:
    case PseudoElement_ScrollBarRightArrow:
    case PseudoElement_SpinBoxUpArrow:
    case PseudoElement_SpinBoxDownArrow:
    case PseudoElement_ToolButtonMenuArrow:
    case PseudoElement_HeaderViewUpArrow:
    case PseudoElement_HeaderViewDownArrow:
    case PseudoElement_SliderGroove:
    case PseudoElement_SliderHandle:
        return Origin_Content;

    default:
        return Origin_Margin;
    }
}

static Qt::Alignment defaultPosition(int pe)
{
    switch (pe) {
    case PseudoElement_Indicator:
    case PseudoElement_ExclusiveIndicator:
    case PseudoElement_MenuCheckMark:
    case PseudoElement_MenuIcon:
        return Qt::AlignLeft | Qt::AlignVCenter;

    case PseudoElement_ScrollBarAddLine:
    case PseudoElement_ScrollBarLast:
    case PseudoElement_SpinBoxDownButton:
    case PseudoElement_PushButtonMenuIndicator:
    case PseudoElement_ToolButtonMenuIndicator:
        return Qt::AlignRight | Qt::AlignBottom;

    case PseudoElement_ScrollBarSubLine:
    case PseudoElement_ScrollBarFirst:
    case PseudoElement_SpinBoxUpButton:
    case PseudoElement_ComboBoxDropDown:
    case PseudoElement_ToolButtonMenu:
    case PseudoElement_DockWidgetCloseButton:
    case PseudoElement_DockWidgetFloatButton:
        return Qt::AlignRight | Qt::AlignTop;

    case PseudoElement_ScrollBarUpArrow:
    case PseudoElement_ScrollBarDownArrow:
    case PseudoElement_ScrollBarLeftArrow:
    case PseudoElement_ScrollBarRightArrow:
    case PseudoElement_SpinBoxUpArrow:
    case PseudoElement_SpinBoxDownArrow:
    case PseudoElement_ComboBoxArrow:
    case PseudoElement_DownArrow:
    case PseudoElement_UpArrow:
    case PseudoElement_LeftArrow:
    case PseudoElement_RightArrow:
    case PseudoElement_ToolButtonMenuArrow:
    case PseudoElement_SliderGroove:
        return Qt::AlignCenter;

    case PseudoElement_GroupBoxTitle:
    case PseudoElement_GroupBoxIndicator: // never used
        return Qt::AlignLeft | Qt::AlignTop;

    case PseudoElement_HeaderViewUpArrow:
    case PseudoElement_HeaderViewDownArrow:
    case PseudoElement_MenuRightArrow:
        return Qt::AlignRight | Qt::AlignVCenter;

    default:
        return { };
    }
}

QSize QStyleSheetStyle::defaultSize(const QWidget *w, QSize sz, const QRect& rect, int pe) const
{
    QStyle *base = baseStyle();

    switch (pe) {
    case PseudoElement_Indicator:
    case PseudoElement_MenuCheckMark:
        if (sz.width() == -1)
            sz.setWidth(base->pixelMetric(PM_IndicatorWidth, nullptr, w));
        if (sz.height() == -1)
            sz.setHeight(base->pixelMetric(PM_IndicatorHeight, nullptr, w));
        break;

    case PseudoElement_ExclusiveIndicator:
    case PseudoElement_GroupBoxIndicator:
        if (sz.width() == -1)
            sz.setWidth(base->pixelMetric(PM_ExclusiveIndicatorWidth, nullptr, w));
        if (sz.height() == -1)
            sz.setHeight(base->pixelMetric(PM_ExclusiveIndicatorHeight, nullptr, w));
        break;

    case PseudoElement_PushButtonMenuIndicator: {
        int pm = base->pixelMetric(PM_MenuButtonIndicator, nullptr, w);
        if (sz.width() == -1)
            sz.setWidth(pm);
        if (sz.height() == -1)
            sz.setHeight(pm);
                                      }
        break;

    case PseudoElement_ComboBoxDropDown:
        if (sz.width() == -1)
            sz.setWidth(16);
        break;

    case PseudoElement_ComboBoxArrow:
    case PseudoElement_DownArrow:
    case PseudoElement_UpArrow:
    case PseudoElement_LeftArrow:
    case PseudoElement_RightArrow:
    case PseudoElement_ToolButtonMenuArrow:
    case PseudoElement_ToolButtonMenuIndicator:
    case PseudoElement_MenuRightArrow:
        if (sz.width() == -1)
            sz.setWidth(13);
        if (sz.height() == -1)
            sz.setHeight(13);
        break;

    case PseudoElement_SpinBoxUpButton:
    case PseudoElement_SpinBoxDownButton:
        if (sz.width() == -1)
            sz.setWidth(16);
        if (sz.height() == -1)
            sz.setHeight(rect.height()/2);
        break;

    case PseudoElement_ToolButtonMenu:
        if (sz.width() == -1)
            sz.setWidth(base->pixelMetric(PM_MenuButtonIndicator, nullptr, w));
        break;

    case PseudoElement_HeaderViewUpArrow:
    case PseudoElement_HeaderViewDownArrow: {
        int pm = base->pixelMetric(PM_HeaderMargin, nullptr, w);
        if (sz.width() == -1)
            sz.setWidth(pm);
        if (sz.height() == 1)
            sz.setHeight(pm);
        break;
                                            }

    case PseudoElement_ScrollBarFirst:
    case PseudoElement_ScrollBarLast:
    case PseudoElement_ScrollBarAddLine:
    case PseudoElement_ScrollBarSubLine:
    case PseudoElement_ScrollBarSlider: {
        int pm = pixelMetric(QStyle::PM_ScrollBarExtent, nullptr, w);
        if (sz.width() == -1)
            sz.setWidth(pm);
        if (sz.height() == -1)
            sz.setHeight(pm);
        break;
                                        }

    case PseudoElement_DockWidgetCloseButton:
    case PseudoElement_DockWidgetFloatButton: {
        int iconSize = pixelMetric(PM_SmallIconSize, nullptr, w);
        return QSize(iconSize, iconSize);
                                              }

    default:
        break;
    }

    // expand to rectangle
    if (sz.height() == -1)
        sz.setHeight(rect.height());
    if (sz.width() == -1)
        sz.setWidth(rect.width());

    return sz;
}

static PositionMode defaultPositionMode(int pe)
{
    switch (pe) {
    case PseudoElement_ScrollBarFirst:
    case PseudoElement_ScrollBarLast:
    case PseudoElement_ScrollBarAddLine:
    case PseudoElement_ScrollBarSubLine:
    case PseudoElement_ScrollBarAddPage:
    case PseudoElement_ScrollBarSubPage:
    case PseudoElement_ScrollBarSlider:
    case PseudoElement_SliderGroove:
    case PseudoElement_SliderHandle:
    case PseudoElement_TabWidgetPane:
        return PositionMode_Absolute;
    default:
        return PositionMode_Static;
    }
}

QRect QStyleSheetStyle::positionRect(const QWidget *w, const QRenderRule &rule2, int pe,
                                     const QRect &originRect, Qt::LayoutDirection dir) const
{
    const QStyleSheetPositionData *p = rule2.position();
    PositionMode mode = (p && p->mode != PositionMode_Unknown) ? p->mode : defaultPositionMode(pe);
    Qt::Alignment position = (p && p->position != 0) ? p->position : defaultPosition(pe);
    QRect r;

    if (mode != PositionMode_Absolute) {
        QSize sz = defaultSize(w, rule2.size(), originRect, pe);
        sz = sz.expandedTo(rule2.minimumContentsSize());
        r = QStyle::alignedRect(dir, position, sz, originRect);
        if (p) {
            int left = p->left ? p->left : -p->right;
            int top = p->top ? p->top : -p->bottom;
            r.translate(dir == Qt::LeftToRight ? left : -left, top);
        }
    } else {
        r = p ? originRect.adjusted(dir == Qt::LeftToRight ? p->left : p->right, p->top,
                                   dir == Qt::LeftToRight ? -p->right : -p->left, -p->bottom)
              : originRect;
        if (rule2.hasContentsSize()) {
            QSize sz = rule2.size().expandedTo(rule2.minimumContentsSize());
            if (sz.width() == -1) sz.setWidth(r.width());
            if (sz.height() == -1) sz.setHeight(r.height());
            r = QStyle::alignedRect(dir, position, sz, r);
        }
    }
    return r;
}

QRect QStyleSheetStyle::positionRect(const QWidget *w, const QRenderRule& rule1, const QRenderRule& rule2, int pe,
                                     const QRect& rect, Qt::LayoutDirection dir) const
{
    const QStyleSheetPositionData *p = rule2.position();
    Origin origin = (p && p->origin != Origin_Unknown) ? p->origin : defaultOrigin(pe);
    QRect originRect = rule1.originRect(rect, origin);
    return positionRect(w, rule2, pe, originRect, dir);
}


/** \internal
   For widget that have an embedded widget (such as combobox) return that embedded widget.
   otherwise return the widget itself
 */
static QWidget *embeddedWidget(QWidget *w)
{
#if QT_CONFIG(combobox)
    if (QComboBox *cmb = qobject_cast<QComboBox *>(w)) {
        if (cmb->isEditable())
            return cmb->lineEdit();
        else
            return cmb;
    }
#endif

#if QT_CONFIG(spinbox)
    if (QAbstractSpinBox *sb = qobject_cast<QAbstractSpinBox *>(w))
        return sb->findChild<QLineEdit *>();
#endif

#if QT_CONFIG(scrollarea)
    if (QAbstractScrollArea *sa = qobject_cast<QAbstractScrollArea *>(w))
        return sa->viewport();
#endif

    return w;
}

/** \internal
  Returns the widget whose style rules apply to \a w.

  When \a w is an embedded widget, this is the container widget.
  For example, if w is a line edit embedded in a combobox, this returns the combobox.
  When \a w is not embedded, this function return \a w itself.

*/
static QWidget *containerWidget(const QWidget *w)
{
#if QT_CONFIG(lineedit)
    if (qobject_cast<const QLineEdit *>(w)) {
        //if the QLineEdit is an embeddedWidget, we need the rule of the real widget
#if QT_CONFIG(combobox)
        if (qobject_cast<const QComboBox *>(w->parentWidget()))
            return w->parentWidget();
#endif
#if QT_CONFIG(spinbox)
        if (qobject_cast<const QAbstractSpinBox *>(w->parentWidget()))
            return w->parentWidget();
#endif
    }
#endif // QT_CONFIG(lineedit)

#if QT_CONFIG(scrollarea)
    if (const QAbstractScrollArea *sa = qobject_cast<const QAbstractScrollArea *>(w->parentWidget())) {
        if (sa->viewport() == w)
            return w->parentWidget();
    }
#endif

    return const_cast<QWidget *>(w);
}

/** \internal
    returns \c true if the widget can NOT be styled directly
 */
static bool unstylable(const QWidget *w)
{
    if (w->windowType() == Qt::Desktop)
        return true;

    if (!w->styleSheet().isEmpty())
        return false;

    if (containerWidget(w) != w)
        return true;

#ifndef QT_NO_FRAME
    // detect QComboBoxPrivateContainer
    else if (qobject_cast<const QFrame *>(w)) {
        if (0
#if QT_CONFIG(combobox)
            || qobject_cast<const QComboBox *>(w->parentWidget())
#endif
           )
            return true;
    }
#endif

#if QT_CONFIG(tabbar)
    if (w->metaObject() == &QWidget::staticMetaObject
            && qobject_cast<const QTabBar*>(w->parentWidget()))
        return true; // The moving tab of a QTabBar
#endif

    return false;
}

static quint64 extendedPseudoClass(const QWidget *w)
{
    quint64 pc = w->isWindow() ? quint64(PseudoClass_Window) : 0;
#if QT_CONFIG(abstractslider)
    if (const QAbstractSlider *slider = qobject_cast<const QAbstractSlider *>(w)) {
        pc |= ((slider->orientation() == Qt::Vertical) ? PseudoClass_Vertical : PseudoClass_Horizontal);
    } else
#endif
#if QT_CONFIG(combobox)
    if (const QComboBox *combo = qobject_cast<const QComboBox *>(w)) {
        if (combo->isEditable())
        pc |= (combo->isEditable() ? PseudoClass_Editable : PseudoClass_ReadOnly);
    } else
#endif
#if QT_CONFIG(lineedit)
    if (const QLineEdit *edit = qobject_cast<const QLineEdit *>(w)) {
        pc |= (edit->isReadOnly() ? PseudoClass_ReadOnly : PseudoClass_Editable);
    } else
#endif
#if QT_CONFIG(textedit)
    if (const QTextEdit *edit = qobject_cast<const QTextEdit *>(w)) {
        pc |= (edit->isReadOnly() ? PseudoClass_ReadOnly : PseudoClass_Editable);
    } else
    if (const QPlainTextEdit *edit = qobject_cast<const QPlainTextEdit *>(w)) {
        pc |= (edit->isReadOnly() ? PseudoClass_ReadOnly : PseudoClass_Editable);
    } else
#endif
    {}
    return pc;
}

// sets up the geometry of the widget. We set a dynamic property when
// we modify the min/max size of the widget. The min/max size is restored
// to their original value when a new stylesheet that does not contain
// the CSS properties is set and when the widget has this dynamic property set.
// This way we don't trample on users who had setup a min/max size in code and
// don't use stylesheets at all.
void QStyleSheetStyle::setGeometry(QWidget *w)
{
    QRenderRule rule = renderRule(w, PseudoElement_None, PseudoClass_Enabled | extendedPseudoClass(w));
    const QStyleSheetGeometryData *geo = rule.geometry();
    if (w->property("_q_stylesheet_minw").toBool()
        && ((!rule.hasGeometry() || geo->minWidth == -1))) {
            w->setMinimumWidth(0);
            w->setProperty("_q_stylesheet_minw", QVariant());
    }
    if (w->property("_q_stylesheet_minh").toBool()
        && ((!rule.hasGeometry() || geo->minHeight == -1))) {
            w->setMinimumHeight(0);
            w->setProperty("_q_stylesheet_minh", QVariant());
    }
    if (w->property("_q_stylesheet_maxw").toBool()
        && ((!rule.hasGeometry() || geo->maxWidth == -1))) {
            w->setMaximumWidth(QWIDGETSIZE_MAX);
            w->setProperty("_q_stylesheet_maxw", QVariant());
    }
   if (w->property("_q_stylesheet_maxh").toBool()
        && ((!rule.hasGeometry() || geo->maxHeight == -1))) {
            w->setMaximumHeight(QWIDGETSIZE_MAX);
            w->setProperty("_q_stylesheet_maxh", QVariant());
    }


    if (rule.hasGeometry()) {
        if (geo->minWidth != -1) {
            w->setProperty("_q_stylesheet_minw", true);
            w->setMinimumWidth(rule.boxSize(QSize(qMax(geo->width, geo->minWidth), 0)).width());
        }
        if (geo->minHeight != -1) {
            w->setProperty("_q_stylesheet_minh", true);
            w->setMinimumHeight(rule.boxSize(QSize(0, qMax(geo->height, geo->minHeight))).height());
        }
        if (geo->maxWidth != -1) {
            w->setProperty("_q_stylesheet_maxw", true);
            w->setMaximumWidth(rule.boxSize(QSize(qMin(geo->width == -1 ? QWIDGETSIZE_MAX : geo->width,
                                                       geo->maxWidth == -1 ? QWIDGETSIZE_MAX : geo->maxWidth), 0)).width());
        }
        if (geo->maxHeight != -1) {
            w->setProperty("_q_stylesheet_maxh", true);
            w->setMaximumHeight(rule.boxSize(QSize(0, qMin(geo->height == -1 ? QWIDGETSIZE_MAX : geo->height,
                                                       geo->maxHeight == -1 ? QWIDGETSIZE_MAX : geo->maxHeight))).height());
        }
    }
}

void QStyleSheetStyle::setProperties(QWidget *w)
{
    // The final occurrence of each property is authoritative.
    // Set value for each property in the order of property final occurrence
    // since properties interact.

    const QList<Declaration> decls = declarations(styleRules(w), QString());
    QList<int> finals; // indices in reverse order of each property's final occurrence

    {
        // scan decls for final occurrence of each "qproperty"
        QDuplicateTracker<QString> propertySet(decls.size());
        for (int i = decls.size() - 1; i >= 0; --i) {
            const QString property = decls.at(i).d->property;
            if (!property.startsWith("qproperty-"_L1, Qt::CaseInsensitive))
                continue;
            if (!propertySet.hasSeen(property))
                finals.append(i);
        }
    }

    for (int i = finals.size() - 1; i >= 0; --i) {
        const Declaration &decl = decls.at(finals[i]);
        QStringView property = decl.d->property;
        property = property.mid(10); // strip "qproperty-"
        const auto propertyL1 = property.toLatin1();

        const QMetaObject *metaObject = w->metaObject();
        int index = metaObject->indexOfProperty(propertyL1);
        if (Q_UNLIKELY(index == -1)) {
            qWarning() << w << " does not have a property named " << property;
            continue;
        }
        const QMetaProperty metaProperty = metaObject->property(index);
        if (Q_UNLIKELY(!metaProperty.isWritable() || !metaProperty.isDesignable())) {
            qWarning() << w << " cannot design property named " << property;
            continue;
        }

        QVariant v;
        const QVariant value = w->property(propertyL1);
        switch (value.userType()) {
        case QMetaType::QIcon: v = decl.iconValue(); break;
        case QMetaType::QImage: v = QImage(decl.uriValue()); break;
        case QMetaType::QPixmap: v = QPixmap(decl.uriValue()); break;
        case QMetaType::QRect: v = decl.rectValue(); break;
        case QMetaType::QSize: v = decl.sizeValue(); break;
        case QMetaType::QColor: v = decl.colorValue(); break;
        case QMetaType::QBrush: v = decl.brushValue(); break;
#ifndef QT_NO_SHORTCUT
        case QMetaType::QKeySequence: v = QKeySequence(decl.d->values.at(0).variant.toString()); break;
#endif
        default: v = decl.d->values.at(0).variant; break;
        }

        if (propertyL1 == QByteArrayView("styleSheet") && value == v)
            continue;

        w->setProperty(propertyL1, v);
    }
}

void QStyleSheetStyle::setPalette(QWidget *w)
{
    struct RuleRoleMap {
        int state;
        QPalette::ColorGroup group;
    } map[3] = {
        { int(PseudoClass_Active | PseudoClass_Enabled), QPalette::Active },
        { PseudoClass_Disabled, QPalette::Disabled },
        { PseudoClass_Enabled, QPalette::Inactive }
    };

    const bool useStyleSheetPropagationInWidgetStyles =
        QCoreApplication::testAttribute(Qt::AA_UseStyleSheetPropagationInWidgetStyles);

    QPalette p;
    if (!useStyleSheetPropagationInWidgetStyles)
        p = w->palette();

    QWidget *ew = embeddedWidget(w);

    for (int i = 0; i < 3; i++) {
        QRenderRule rule = renderRule(w, PseudoElement_None, map[i].state | extendedPseudoClass(w));
        if (i == 0) {
            if (!w->property("_q_styleSheetWidgetFont").isValid()) {
                saveWidgetFont(w, w->d_func()->localFont());
            }
            updateStyleSheetFont(w);
            if (ew != w)
                updateStyleSheetFont(ew);
        }

        rule.configurePalette(&p, map[i].group, ew, ew != w);
    }

    if (!useStyleSheetPropagationInWidgetStyles || p.resolveMask() != 0) {
        QPalette wp = w->palette();
        styleSheetCaches->customPaletteWidgets.insert(w, {wp, p.resolveMask()});

        if (useStyleSheetPropagationInWidgetStyles) {
            p = p.resolve(wp);
            p.setResolveMask(p.resolveMask() | wp.resolveMask());
        }

        w->setPalette(p);
        if (ew != w)
            ew->setPalette(p);
    }
}

void QStyleSheetStyle::unsetPalette(QWidget *w)
{
    const bool useStyleSheetPropagationInWidgetStyles =
        QCoreApplication::testAttribute(Qt::AA_UseStyleSheetPropagationInWidgetStyles);

    const auto it = styleSheetCaches->customPaletteWidgets.find(w);
    if (it != styleSheetCaches->customPaletteWidgets.end()) {
        auto customizedPalette = std::move(*it);
        styleSheetCaches->customPaletteWidgets.erase(it);

        QPalette original;
        if (useStyleSheetPropagationInWidgetStyles)
            original = std::move(customizedPalette).reverted(w->palette());
        else
            original = customizedPalette.oldWidgetValue;

        w->setPalette(original);
        QWidget *ew = embeddedWidget(w);
        if (ew != w)
            ew->setPalette(original);
    }

    if (useStyleSheetPropagationInWidgetStyles) {
        unsetStyleSheetFont(w);
        QWidget *ew = embeddedWidget(w);
        if (ew != w)
            unsetStyleSheetFont(ew);
    } else {
        QVariant oldFont = w->property("_q_styleSheetWidgetFont");
        if (oldFont.isValid()) {
            w->setFont(qvariant_cast<QFont>(oldFont));
        }
    }

    if (styleSheetCaches->autoFillDisabledWidgets.contains(w)) {
        embeddedWidget(w)->setAutoFillBackground(true);
        styleSheetCaches->autoFillDisabledWidgets.remove(w);
    }
}

void QStyleSheetStyle::unsetStyleSheetFont(QWidget *w) const
{
    const auto it = styleSheetCaches->customFontWidgets.find(w);
    if (it != styleSheetCaches->customFontWidgets.end()) {
        auto customizedFont = std::move(*it);
        styleSheetCaches->customFontWidgets.erase(it);
        w->setFont(std::move(customizedFont).reverted(w->font()));
    }
}

static void updateObjects(const QList<const QObject *>& objects)
{
    if (!styleSheetCaches->styleRulesCache.isEmpty() || !styleSheetCaches->hasStyleRuleCache.isEmpty() || !styleSheetCaches->renderRulesCache.isEmpty()) {
        for (const QObject *object : objects) {
            styleSheetCaches->styleRulesCache.remove(object);
            styleSheetCaches->hasStyleRuleCache.remove(object);
            styleSheetCaches->renderRulesCache.remove(object);
        }
    }

    QEvent event(QEvent::StyleChange);
    for (const QObject *object : objects) {
        if (auto widget = qobject_cast<QWidget*>(const_cast<QObject*>(object))) {
            widget->style()->polish(widget);
            QCoreApplication::sendEvent(widget, &event);
            QList<const QObject *> children;
            children.reserve(widget->children().size() + 1);
            for (auto child: std::as_const(widget->children()))
                children.append(child);
            updateObjects(children);
        }
    }
}

/////////////////////////////////////////////////////////////////////////////////////////
// The stylesheet style
int QStyleSheetStyle::numinstances = 0;

QStyleSheetStyle::QStyleSheetStyle(QStyle *base)
    : QWindowsStyle(*new QStyleSheetStylePrivate), base(base), refcount(1)
{
    ++numinstances;
    if (numinstances == 1) {
        styleSheetCaches = new QStyleSheetStyleCaches;
    }
}

QStyleSheetStyle::~QStyleSheetStyle()
{
    --numinstances;
    if (numinstances == 0) {
        delete styleSheetCaches;
    }
}
QStyle *QStyleSheetStyle::baseStyle() const
{
    if (base)
        return base;
    if (QStyleSheetStyle *me = qt_styleSheet(QApplication::style()))
        return me->base;
    return QApplication::style();
}

void QStyleSheetStyleCaches::objectDestroyed(QObject *o)
{
    styleRulesCache.remove(o);
    hasStyleRuleCache.remove(o);
    renderRulesCache.remove(o);
    customPaletteWidgets.remove((const QWidget *)o);
    customFontWidgets.remove(static_cast<QWidget *>(o));
    styleSheetCache.remove(o);
    autoFillDisabledWidgets.remove((const QWidget *)o);
}

void QStyleSheetStyleCaches::styleDestroyed(QObject *o)
{
    styleSheetCache.remove(o);
}

/*!
 *  Make sure that the cache will be clean by connecting destroyed if needed.
 *  return false if the widget is not stylable;
 */
bool QStyleSheetStyle::initObject(const QObject *obj) const
{
    if (!obj)
        return false;
    if (const QWidget *w = qobject_cast<const QWidget*>(obj)) {
        if (w->testAttribute(Qt::WA_StyleSheet))
            return true;
        if (unstylable(w))
            return false;
        const_cast<QWidget *>(w)->setAttribute(Qt::WA_StyleSheet, true);
    }

    QObject::connect(obj, SIGNAL(destroyed(QObject*)), styleSheetCaches, SLOT(objectDestroyed(QObject*)), Qt::UniqueConnection);
    return true;
}

void QStyleSheetStyle::polish(QWidget *w)
{
    baseStyle()->polish(w);
    RECURSION_GUARD(return)

    if (!initObject(w))
        return;

    if (styleSheetCaches->styleRulesCache.contains(w)) {
        // the widget accessed its style pointer before polish (or repolish)
        // (example: the QAbstractSpinBox constructor ask for the stylehint)
        styleSheetCaches->styleRulesCache.remove(w);
        styleSheetCaches->hasStyleRuleCache.remove(w);
        styleSheetCaches->renderRulesCache.remove(w);
        styleSheetCaches->styleSheetCache.remove(w);
    }
    setGeometry(w);
    setProperties(w);
    unsetPalette(w);
    setPalette(w);

    //set the WA_Hover attribute if one of the selector depends of the hover state
    QList<StyleRule> rules = styleRules(w);
    for (int i = 0; i < rules.size(); i++) {
        const Selector& selector = rules.at(i).selectors.at(0);
        quint64 negated = 0;
        quint64 cssClass = selector.pseudoClass(&negated);
        if ( cssClass & PseudoClass_Hover || negated & PseudoClass_Hover) {
            w->setAttribute(Qt::WA_Hover);
            embeddedWidget(w)->setAttribute(Qt::WA_Hover);
            embeddedWidget(w)->setMouseTracking(true);
        }
    }


#if QT_CONFIG(scrollarea)
    if (QAbstractScrollArea *sa = qobject_cast<QAbstractScrollArea *>(w)) {
        QRenderRule rule = renderRule(sa, PseudoElement_None, PseudoClass_Enabled);
        if ((rule.hasBorder() && rule.border()->hasBorderImage())
            || (rule.hasBackground() && !rule.background()->pixmap.isNull())) {
            QObject::connect(sa->horizontalScrollBar(), SIGNAL(valueChanged(int)),
                             sa, SLOT(update()), Qt::UniqueConnection);
            QObject::connect(sa->verticalScrollBar(), SIGNAL(valueChanged(int)),
                             sa, SLOT(update()), Qt::UniqueConnection);
        }
    }
#endif

    QRenderRule rule = renderRule(w, PseudoElement_None, PseudoClass_Any);

    w->setAttribute(Qt::WA_StyleSheetTarget, rule.hasModification());

    if (rule.hasDrawable() || rule.hasBox()) {
        if (w->metaObject() == &QWidget::staticMetaObject
#if QT_CONFIG(itemviews)
              || qobject_cast<QHeaderView *>(w)
#endif
#if QT_CONFIG(tabbar)
              || qobject_cast<QTabBar *>(w)
#endif
#ifndef QT_NO_FRAME
              || qobject_cast<QFrame *>(w)
#endif
#if QT_CONFIG(mainwindow)
              || qobject_cast<QMainWindow *>(w)
#endif
#if QT_CONFIG(mdiarea)
              || qobject_cast<QMdiSubWindow *>(w)
#endif
#if QT_CONFIG(menubar)
              || qobject_cast<QMenuBar *>(w)
#endif
#if QT_CONFIG(dialog)
              || qobject_cast<QDialog *>(w)
#endif
                                           ) {
            w->setAttribute(Qt::WA_StyledBackground, true);
        }
        QWidget *ew = embeddedWidget(w);
        if (ew->autoFillBackground()) {
            ew->setAutoFillBackground(false);
            styleSheetCaches->autoFillDisabledWidgets.insert(w);
            if (ew != w) { //eg. viewport of a scrollarea
                //(in order to draw the background anyway in case we don't.)
                ew->setAttribute(Qt::WA_StyledBackground, true);
            }
        }
        if (!rule.hasBackground() || rule.background()->isTransparent() || rule.hasBox()
            || (!rule.hasNativeBorder() && !rule.border()->isOpaque()))
            w->setAttribute(Qt::WA_OpaquePaintEvent, false);
        if (rule.hasBox() || !rule.hasNativeBorder()
#if QT_CONFIG(pushbutton)
              || (qobject_cast<QPushButton *>(w))
#endif
            )
            w->setAttribute(Qt::WA_MacShowFocusRect, false);
    }
}

void QStyleSheetStyle::polish(QApplication *app)
{
    baseStyle()->polish(app);
}

void QStyleSheetStyle::polish(QPalette &pal)
{
    baseStyle()->polish(pal);
}

void QStyleSheetStyle::repolish(QWidget *w)
{
    QList<const QObject *> children;
    children.reserve(w->children().size() + 1);
    for (auto child: std::as_const(w->children()))
        children.append(child);
    children.append(w);
    styleSheetCaches->styleSheetCache.remove(w);
    updateObjects(children);
}

void QStyleSheetStyle::repolish(QApplication *app)
{
    Q_UNUSED(app);
    const QList<const QObject*> allObjects = styleSheetCaches->styleRulesCache.keys();
    styleSheetCaches->styleSheetCache.remove(qApp);
    styleSheetCaches->styleRulesCache.clear();
    styleSheetCaches->hasStyleRuleCache.clear();
    styleSheetCaches->renderRulesCache.clear();
    updateObjects(allObjects);
}

void QStyleSheetStyle::unpolish(QWidget *w)
{
    if (!w || !w->testAttribute(Qt::WA_StyleSheet)) {
        baseStyle()->unpolish(w);
        return;
    }

    styleSheetCaches->styleRulesCache.remove(w);
    styleSheetCaches->hasStyleRuleCache.remove(w);
    styleSheetCaches->renderRulesCache.remove(w);
    styleSheetCaches->styleSheetCache.remove(w);
    unsetPalette(w);
    setGeometry(w);
    w->setAttribute(Qt::WA_StyleSheetTarget, false);
    w->setAttribute(Qt::WA_StyleSheet, false);
    QObject::disconnect(w, nullptr, this, nullptr);
#if QT_CONFIG(scrollarea)
    if (QAbstractScrollArea *sa = qobject_cast<QAbstractScrollArea *>(w)) {
        QObject::disconnect(sa->horizontalScrollBar(), SIGNAL(valueChanged(int)),
                            sa, SLOT(update()));
        QObject::disconnect(sa->verticalScrollBar(), SIGNAL(valueChanged(int)),
                            sa, SLOT(update()));
    }
#endif
    baseStyle()->unpolish(w);
}

void QStyleSheetStyle::unpolish(QApplication *app)
{
    baseStyle()->unpolish(app);
    RECURSION_GUARD(return)
    styleSheetCaches->styleRulesCache.clear();
    styleSheetCaches->hasStyleRuleCache.clear();
    styleSheetCaches->renderRulesCache.clear();
    styleSheetCaches->styleSheetCache.remove(qApp);
}

void QStyleSheetStyle::drawComplexControl(ComplexControl cc, const QStyleOptionComplex *opt, QPainter *p,
                                          const QWidget *w) const
{
    RECURSION_GUARD(baseStyle()->drawComplexControl(cc, opt, p, w); return)

    QRenderRule rule = renderRule(w, opt);

    switch (cc) {
    case CC_ComboBox:
        if (const QStyleOptionComboBox *cmb = qstyleoption_cast<const QStyleOptionComboBox *>(opt)) {
            QStyleOptionComboBox cmbOpt(*cmb);
            cmbOpt.rect = rule.borderRect(opt->rect);
            if (rule.hasNativeBorder()) {
                rule.drawBackgroundImage(p, cmbOpt.rect);
                rule.configurePalette(&cmbOpt.palette, QPalette::ButtonText, QPalette::Button);
                bool customDropDown = (opt->subControls & QStyle::SC_ComboBoxArrow)
                                && (hasStyleRule(w, PseudoElement_ComboBoxDropDown) || hasStyleRule(w, PseudoElement_ComboBoxArrow));
                if (customDropDown)
                    cmbOpt.subControls &= ~QStyle::SC_ComboBoxArrow;
                if (rule.baseStyleCanDraw()) {
                    baseStyle()->drawComplexControl(cc, &cmbOpt, p, w);
                } else {
                    QWindowsStyle::drawComplexControl(cc, &cmbOpt, p, w);
                }
                if (!customDropDown)
                    return;
            } else {
                rule.drawRule(p, opt->rect);
            }

            if (opt->subControls & QStyle::SC_ComboBoxArrow) {
                QRenderRule subRule = renderRule(w, opt, PseudoElement_ComboBoxDropDown);
                if (subRule.hasDrawable()) {
                    QRect r = subControlRect(CC_ComboBox, opt, SC_ComboBoxArrow, w);
                    subRule.drawRule(p, r);
                    QRenderRule subRule2 = renderRule(w, opt, PseudoElement_ComboBoxArrow);
                    r = positionRect(w, subRule, subRule2, PseudoElement_ComboBoxArrow, r, opt->direction);
                    subRule2.drawRule(p, r);
                } else {
                    rule.configurePalette(&cmbOpt.palette, QPalette::ButtonText, QPalette::Button);
                    cmbOpt.subControls = QStyle::SC_ComboBoxArrow;
                    QWindowsStyle::drawComplexControl(cc, &cmbOpt, p, w);
                }
            }

            return;
        }
        break;

#if QT_CONFIG(spinbox)
    case CC_SpinBox:
        if (const QStyleOptionSpinBox *spin = qstyleoption_cast<const QStyleOptionSpinBox *>(opt)) {
            QStyleOptionSpinBox spinOpt(*spin);
            rule.configurePalette(&spinOpt.palette, QPalette::ButtonText, QPalette::Button);
            rule.configurePalette(&spinOpt.palette, QPalette::Text, QPalette::Base);
            spinOpt.rect = rule.borderRect(opt->rect);
            bool customUp = true, customDown = true;
            QRenderRule upRule = renderRule(w, opt, PseudoElement_SpinBoxUpButton);
            QRenderRule downRule = renderRule(w, opt, PseudoElement_SpinBoxDownButton);
            bool upRuleMatch = upRule.hasGeometry() || upRule.hasPosition();
            bool downRuleMatch = downRule.hasGeometry() || downRule.hasPosition();
            if (rule.hasNativeBorder() && !upRuleMatch && !downRuleMatch) {
                rule.drawBackgroundImage(p, spinOpt.rect);
                customUp = (opt->subControls & QStyle::SC_SpinBoxUp)
                        && (hasStyleRule(w, PseudoElement_SpinBoxUpButton) || hasStyleRule(w, PseudoElement_UpArrow));
                if (customUp)
                    spinOpt.subControls &= ~QStyle::SC_SpinBoxUp;
                customDown = (opt->subControls & QStyle::SC_SpinBoxDown)
                        && (hasStyleRule(w, PseudoElement_SpinBoxDownButton) || hasStyleRule(w, PseudoElement_DownArrow));
                if (customDown)
                    spinOpt.subControls &= ~QStyle::SC_SpinBoxDown;
                if (rule.baseStyleCanDraw()) {
                    baseStyle()->drawComplexControl(cc, &spinOpt, p, w);
                } else {
                    QWindowsStyle::drawComplexControl(cc, &spinOpt, p, w);
                }
                if (!customUp && !customDown)
                    return;
            } else {
                rule.drawRule(p, opt->rect);
            }

            if ((opt->subControls & QStyle::SC_SpinBoxUp) && customUp) {
                QRenderRule subRule = renderRule(w, opt, PseudoElement_SpinBoxUpButton);
                if (subRule.hasDrawable()) {
                    QRect r = subControlRect(CC_SpinBox, opt, SC_SpinBoxUp, w);
                    subRule.drawRule(p, r);
                    QRenderRule subRule2 = renderRule(w, opt, PseudoElement_SpinBoxUpArrow);
                    r = positionRect(w, subRule, subRule2, PseudoElement_SpinBoxUpArrow, r, opt->direction);
                    subRule2.drawRule(p, r);
                } else {
                    spinOpt.subControls = QStyle::SC_SpinBoxUp;
                    QWindowsStyle::drawComplexControl(cc, &spinOpt, p, w);
                }
            }

            if ((opt->subControls & QStyle::SC_SpinBoxDown) && customDown) {
                QRenderRule subRule = renderRule(w, opt, PseudoElement_SpinBoxDownButton);
                if (subRule.hasDrawable()) {
                    QRect r = subControlRect(CC_SpinBox, opt, SC_SpinBoxDown, w);
                    subRule.drawRule(p, r);
                    QRenderRule subRule2 = renderRule(w, opt, PseudoElement_SpinBoxDownArrow);
                    r = positionRect(w, subRule, subRule2, PseudoElement_SpinBoxDownArrow, r, opt->direction);
                    subRule2.drawRule(p, r);
                } else {
                    spinOpt.subControls = QStyle::SC_SpinBoxDown;
                    QWindowsStyle::drawComplexControl(cc, &spinOpt, p, w);
                }
            }
            return;
        }
        break;
#endif // QT_CONFIG(spinbox)

    case CC_GroupBox:
        if (const QStyleOptionGroupBox *gb = qstyleoption_cast<const QStyleOptionGroupBox *>(opt)) {

            QRect labelRect, checkBoxRect, titleRect, frameRect;
            bool hasTitle = (gb->subControls & QStyle::SC_GroupBoxCheckBox) || !gb->text.isEmpty();

            if (!rule.hasDrawable() && (!hasTitle || !hasStyleRule(w, PseudoElement_GroupBoxTitle))
                && !hasStyleRule(w, PseudoElement_Indicator) && !rule.hasBox() && !rule.hasFont && !rule.hasPalette()) {
                // let the native style draw the combobox if there is no style for it.
                break;
            }
            rule.drawBackground(p, opt->rect);

            QRenderRule titleRule = renderRule(w, opt, PseudoElement_GroupBoxTitle);
            bool clipSet = false;

            if (hasTitle) {
                labelRect = subControlRect(CC_GroupBox, opt, SC_GroupBoxLabel, w);
                //Some native style (such as mac) may return a too small rectangle (because they use smaller fonts),  so we may need to expand it a little bit.
                labelRect.setSize(labelRect.size().expandedTo(ParentStyle::subControlRect(CC_GroupBox, opt, SC_GroupBoxLabel, w).size()));
                if (gb->subControls & QStyle::SC_GroupBoxCheckBox) {
                    checkBoxRect = subControlRect(CC_GroupBox, opt, SC_GroupBoxCheckBox, w);
                    titleRect = titleRule.boxRect(checkBoxRect.united(labelRect));
                } else {
                    titleRect = titleRule.boxRect(labelRect);
                }
                if (!titleRule.hasBackground() || !titleRule.background()->isTransparent()) {
                    clipSet = true;
                    p->save();
                    p->setClipRegion(QRegion(opt->rect) - titleRect);
                }
            }

            frameRect = subControlRect(CC_GroupBox, opt, SC_GroupBoxFrame, w);
            QStyleOptionFrame frame;
            frame.QStyleOption::operator=(*gb);
            frame.features = gb->features;
            frame.lineWidth = gb->lineWidth;
            frame.midLineWidth = gb->midLineWidth;
            frame.rect = frameRect;
            drawPrimitive(PE_FrameGroupBox, &frame, p, w);

            if (clipSet)
                p->restore();

            // draw background and frame of the title
            if (hasTitle)
                titleRule.drawRule(p, titleRect);

            // draw the indicator
            if (gb->subControls & QStyle::SC_GroupBoxCheckBox) {
                QStyleOptionButton box;
                box.QStyleOption::operator=(*gb);
                box.rect = checkBoxRect;
                drawPrimitive(PE_IndicatorCheckBox, &box, p, w);
            }

            // draw the text
            if (!gb->text.isEmpty()) {
                int alignment = int(Qt::AlignCenter | Qt::TextShowMnemonic);
                if (!styleHint(QStyle::SH_UnderlineShortcut, opt, w)) {
                    alignment |= Qt::TextHideMnemonic;
                }

                QPalette pal = gb->palette;
                if (gb->textColor.isValid())
                    pal.setColor(QPalette::WindowText, gb->textColor);
                titleRule.configurePalette(&pal, QPalette::WindowText, QPalette::Window);
                drawItemText(p, labelRect,  alignment, pal, gb->state & State_Enabled,
                             gb->text, QPalette::WindowText);

                if (gb->state & State_HasFocus) {
                    QStyleOptionFocusRect fropt;
                    fropt.QStyleOption::operator=(*gb);
                    fropt.rect = labelRect;
                    drawPrimitive(PE_FrameFocusRect, &fropt, p, w);
                }
            }

                        return;
        }
        break;

    case CC_ToolButton:
        if (const QStyleOptionToolButton *tool = qstyleoption_cast<const QStyleOptionToolButton *>(opt)) {
            QStyleOptionToolButton toolOpt(*tool);
            rule.configurePalette(&toolOpt.palette, QPalette::ButtonText, QPalette::Button);
            toolOpt.font = rule.font.resolve(toolOpt.font);
            toolOpt.rect = rule.borderRect(opt->rect);
            const auto customArrowElement = [tool]{
                switch (tool->arrowType) {
                case Qt::DownArrow: return PseudoElement_DownArrow;
                case Qt::UpArrow: return PseudoElement_UpArrow;
                case Qt::LeftArrow: return PseudoElement_LeftArrow;
                case Qt::RightArrow: return PseudoElement_RightArrow;
                default: break;
                }
                return PseudoElement_None;
            };
            // if arrow/menu/indicators are requested, either draw them using the available rule,
            // or let the base style draw them; but not both
            const bool drawArrow = tool->features & QStyleOptionToolButton::Arrow;
            bool customArrow = drawArrow && hasStyleRule(w, customArrowElement());
            if (customArrow) {
                toolOpt.features &= ~QStyleOptionToolButton::Arrow;
                toolOpt.text = QString(); // we need to draw the arrow and the text ourselves
            }
            bool drawDropDown = tool->features & QStyleOptionToolButton::MenuButtonPopup;
            bool customDropDown = drawDropDown && hasStyleRule(w, PseudoElement_ToolButtonMenu);
            bool customDropDownArrow = false;
            bool drawMenuIndicator = tool->features & QStyleOptionToolButton::HasMenu;
            if (customDropDown) {
                toolOpt.subControls &= ~QStyle::SC_ToolButtonMenu;
                customDropDownArrow = hasStyleRule(w, PseudoElement_ToolButtonMenuArrow);
                if (customDropDownArrow)
                    toolOpt.features &= ~(QStyleOptionToolButton::Menu | QStyleOptionToolButton::HasMenu);
            }
            const bool customMenuIndicator = (!drawDropDown && drawMenuIndicator)
                                          && hasStyleRule(w, PseudoElement_ToolButtonMenuIndicator);
            if (customMenuIndicator)
                toolOpt.features &= ~QStyleOptionToolButton::HasMenu;

            if (rule.hasNativeBorder()) {
                if (tool->subControls & SC_ToolButton) {
                    //in some case (eg. the button is "auto raised") the style doesn't draw the background
                    //so we need to draw the background.
                    // use the same condition as in QCommonStyle
                    State bflags = tool->state & ~State_Sunken;
                    if (bflags & State_AutoRaise && (!(bflags & State_MouseOver) || !(bflags & State_Enabled)))
                            bflags &= ~State_Raised;
                    if (tool->state & State_Sunken && tool->activeSubControls & SC_ToolButton)
                            bflags |= State_Sunken;
                    if (!(bflags & (State_Sunken | State_On | State_Raised)))
                        rule.drawBackground(p, toolOpt.rect);
                }

                QStyleOptionToolButton nativeToolOpt(toolOpt);
                // don't draw natively if we have a custom rule for menu indicators and/or buttons
                if (customMenuIndicator)
                    nativeToolOpt.features &= ~(QStyleOptionToolButton::Menu | QStyleOptionToolButton::HasMenu);
                if (customDropDown || customDropDownArrow)
                    nativeToolOpt.features &= ~(QStyleOptionToolButton::Menu | QStyleOptionToolButton::HasMenu | QStyleOptionToolButton::MenuButtonPopup);
                // Let base or windows style draw the button, which will include the menu-button
                if (rule.baseStyleCanDraw() && !(tool->features & QStyleOptionToolButton::Arrow))
                    baseStyle()->drawComplexControl(cc, &nativeToolOpt, p, w);
                else
                    QWindowsStyle::drawComplexControl(cc, &nativeToolOpt, p, w);
                // if we did draw natively, don't draw custom
                if (nativeToolOpt.features & (QStyleOptionToolButton::Menu | QStyleOptionToolButton::HasMenu))
                    drawMenuIndicator = false;
                if (nativeToolOpt.features & QStyleOptionToolButton::MenuButtonPopup && !customDropDownArrow)
                    drawDropDown = false;
            } else {
                rule.drawRule(p, opt->rect);
                toolOpt.rect = rule.contentsRect(opt->rect);
                if (rule.hasFont)
                    toolOpt.font = rule.font.resolve(toolOpt.font);
                drawControl(CE_ToolButtonLabel, &toolOpt, p, w);
            }

            const QRect cr = toolOpt.rect;
            // Draw DropDownButton unless drawn before
            if (drawDropDown) {
                if (opt->subControls & QStyle::SC_ToolButtonMenu) {
                    QRenderRule subRule = renderRule(w, opt, PseudoElement_ToolButtonMenu);
                    QRect menuButtonRect = subControlRect(CC_ToolButton, opt, QStyle::SC_ToolButtonMenu, w);
                    if (subRule.hasDrawable()) {
                        subRule.drawRule(p, menuButtonRect);
                    } else {
                        toolOpt.rect = menuButtonRect;
                        baseStyle()->drawPrimitive(PE_IndicatorButtonDropDown, &toolOpt, p, w);
                    }

                    if (customDropDownArrow || drawMenuIndicator) {
                        QRenderRule arrowRule = renderRule(w, opt, PseudoElement_ToolButtonMenuArrow);
                        QRect arrowRect = arrowRule.hasGeometry()
                                        ? positionRect(w, arrowRule, PseudoElement_ToolButtonMenuArrow, menuButtonRect, toolOpt.direction)
                                        : arrowRule.contentsRect(menuButtonRect);
                        if (arrowRule.hasDrawable()) {
                            arrowRule.drawRule(p, arrowRect);
                        } else {
                            toolOpt.rect = arrowRect;
                            baseStyle()->drawPrimitive(QStyle::PE_IndicatorArrowDown, &toolOpt, p, w);
                        }
                    }
                }
            } else if (drawMenuIndicator) {
                QRenderRule subRule = renderRule(w, opt, PseudoElement_ToolButtonMenuIndicator);

                // content padding does not impact the indicator, so use the original rect to
                // calculate position of the sub element within the toplevel rule
                QRect r = positionRect(w, rule, subRule, PseudoElement_ToolButtonMenuIndicator, opt->rect, toolOpt.direction);
                if (subRule.hasDrawable()) {
                    subRule.drawRule(p, r);
                } else {
                    toolOpt.rect = r;
                    baseStyle()->drawPrimitive(QStyle::PE_IndicatorArrowDown, &toolOpt, p, w);
                }
            }
            toolOpt.rect = cr;

            // If we don't have a custom arrow, then the arrow will have been rendered
            // already by the base style when drawing the label.
            if (customArrow) {
                const auto arrowElement = customArrowElement();
                QRenderRule subRule = renderRule(w, opt, arrowElement);
                QRect arrowRect = subRule.hasGeometry() ? positionRect(w, subRule, arrowElement, toolOpt.rect, toolOpt.direction)
                                                        : subRule.contentsRect(toolOpt.rect);

                switch (toolOpt.toolButtonStyle) {
                case Qt::ToolButtonIconOnly:
                    break;
                case Qt::ToolButtonTextOnly:
                case Qt::ToolButtonTextBesideIcon:
                case Qt::ToolButtonTextUnderIcon: {
                    // The base style needs to lay out the contents and will render the styled
                    // arrow icons, unless the geometry is defined in the style sheet.
                    toolOpt.text = tool->text;
                    if (!subRule.hasGeometry())
                        toolOpt.features |= QStyleOptionToolButton::Arrow;
                    drawControl(CE_ToolButtonLabel, &toolOpt, p, w);
                    if (!subRule.hasGeometry())
                        return;
                    break;
                }
                case Qt::ToolButtonFollowStyle:
                    // QToolButton handles this, so must never happen
                    Q_ASSERT(false);
                    break;
                }
                subRule.drawRule(p, arrowRect);
            }
            return;
        }
        break;

#if QT_CONFIG(scrollbar)
    case CC_ScrollBar:
        if (const QStyleOptionSlider *sb = qstyleoption_cast<const QStyleOptionSlider *>(opt)) {
            if (!rule.hasDrawable()) {
                QStyleOptionSlider sbOpt(*sb);
                sbOpt.rect = rule.borderRect(opt->rect);
                rule.drawBackgroundImage(p, opt->rect);
                baseStyle()->drawComplexControl(cc, &sbOpt, p, w);
            } else {
                rule.drawRule(p, opt->rect);
                QWindowsStyle::drawComplexControl(cc, opt, p, w);
            }
            return;
        }
        break;
#endif // QT_CONFIG(scrollbar)

#if QT_CONFIG(slider)
    case CC_Slider:
        if (const QStyleOptionSlider *slider = qstyleoption_cast<const QStyleOptionSlider *>(opt)) {
            rule.drawRule(p, opt->rect);

            QRenderRule grooveSubRule = renderRule(w, opt, PseudoElement_SliderGroove);
            QRenderRule handleSubRule = renderRule(w, opt, PseudoElement_SliderHandle);
            if (!grooveSubRule.hasDrawable()) {
                QStyleOptionSlider slOpt(*slider);
                bool handleHasRule = handleSubRule.hasDrawable();
                // If the style specifies a different handler rule, draw the groove without the handler.
                if (handleHasRule)
                    slOpt.subControls &= ~SC_SliderHandle;
                baseStyle()->drawComplexControl(cc, &slOpt, p, w);
                if (!handleHasRule)
                    return;
            }

            QRect gr = subControlRect(cc, opt, SC_SliderGroove, w);
            if (slider->subControls & SC_SliderGroove) {
                grooveSubRule.drawRule(p, gr);
            }

            if (slider->subControls & SC_SliderHandle) {
                QRect hr = subControlRect(cc, opt, SC_SliderHandle, w);

                QRenderRule subRule1 = renderRule(w, opt, PseudoElement_SliderSubPage);
                if (subRule1.hasDrawable()) {
                    QRect r(gr.topLeft(),
                            slider->orientation == Qt::Horizontal
                                ? QPoint(hr.x()+hr.width()/2, gr.y()+gr.height() - 1)
                                : QPoint(gr.x()+gr.width() - 1, hr.y()+hr.height()/2));
                    subRule1.drawRule(p, r);
                }

                QRenderRule subRule2 = renderRule(w, opt, PseudoElement_SliderAddPage);
                if (subRule2.hasDrawable()) {
                    QRect r(slider->orientation == Qt::Horizontal
                                ? QPoint(hr.x()+hr.width()/2+1, gr.y())
                                : QPoint(gr.x(), hr.y()+hr.height()/2+1),
                            gr.bottomRight());
                    subRule2.drawRule(p, r);
                }

                handleSubRule.drawRule(p, handleSubRule.boxRect(hr, Margin));
            }

            if (slider->subControls & SC_SliderTickmarks) {
                // TODO...
            }

            return;
        }
        break;
#endif // QT_CONFIG(slider)

    case CC_MdiControls:
        if (hasStyleRule(w, PseudoElement_MdiCloseButton)
            || hasStyleRule(w, PseudoElement_MdiNormalButton)
            || hasStyleRule(w, PseudoElement_MdiMinButton)) {
            QList<QVariant> layout = rule.styleHint("button-layout"_L1).toList();
            if (layout.isEmpty())
                layout = subControlLayout("mNX"_L1);

            QStyleOptionComplex optCopy(*opt);
            optCopy.subControls = { };
            for (int i = 0; i < layout.size(); i++) {
                int layoutButton = layout[i].toInt();
                if (layoutButton < PseudoElement_MdiCloseButton
                    || layoutButton > PseudoElement_MdiNormalButton)
                    continue;
                QStyle::SubControl control = knownPseudoElements[layoutButton].subControl;
                if (!(opt->subControls & control))
                    continue;
                QRenderRule subRule = renderRule(w, opt, layoutButton);
                if (subRule.hasDrawable()) {
                    QRect rect = subRule.boxRect(subControlRect(CC_MdiControls, opt, control, w), Margin);
                    subRule.drawRule(p, rect);
                    QIcon icon = standardIcon(subControlIcon(layoutButton), opt);
                    icon.paint(p, subRule.contentsRect(rect), Qt::AlignCenter);
                } else {
                    optCopy.subControls |= control;
                }
            }

            if (optCopy.subControls)
                baseStyle()->drawComplexControl(CC_MdiControls, &optCopy, p, w);
            return;
        }
        break;

    case CC_TitleBar:
        if (const QStyleOptionTitleBar *tb = qstyleoption_cast<const QStyleOptionTitleBar *>(opt)) {
            QRenderRule subRule = renderRule(w, opt, PseudoElement_TitleBar);
            if (!subRule.hasDrawable() && !subRule.hasBox() && !subRule.hasBorder())
                break;
            subRule.drawRule(p, opt->rect);
            QHash<QStyle::SubControl, QRect> layout = titleBarLayout(w, tb);

            QRect ir;
            ir = layout[SC_TitleBarLabel];
            if (ir.isValid()) {
                if (subRule.hasPalette())
                    p->setPen(subRule.palette()->foreground.color());
                p->fillRect(ir, Qt::white);
                p->drawText(ir.x(), ir.y(), ir.width(), ir.height(), Qt::AlignLeft | Qt::AlignVCenter | Qt::TextSingleLine, tb->text);
            }

            QPixmap pm;

            ir = layout[SC_TitleBarSysMenu];
            if (ir.isValid()) {
                QRenderRule subSubRule = renderRule(w, opt, PseudoElement_TitleBarSysMenu);
                subSubRule.drawRule(p, ir);
                ir = subSubRule.contentsRect(ir);
                if (!tb->icon.isNull()) {
                    tb->icon.paint(p, ir);
                } else {
                    int iconSize = pixelMetric(PM_SmallIconSize, tb, w);
                    pm = standardIcon(SP_TitleBarMenuButton, nullptr, w).pixmap(iconSize, iconSize);
                    drawItemPixmap(p, ir, Qt::AlignCenter, pm);
                }
            }

            ir = layout[SC_TitleBarCloseButton];
            if (ir.isValid()) {
                QRenderRule subSubRule = renderRule(w, opt, PseudoElement_TitleBarCloseButton);
                subSubRule.drawRule(p, ir);

                QSize sz = subSubRule.contentsRect(ir).size();
                if ((tb->titleBarFlags & Qt::WindowType_Mask) == Qt::Tool)
                    pm = standardIcon(SP_DockWidgetCloseButton, nullptr, w).pixmap(sz);
                else
                    pm = standardIcon(SP_TitleBarCloseButton, nullptr, w).pixmap(sz);
                drawItemPixmap(p, ir, Qt::AlignCenter, pm);
            }

            int pes[] = {
                PseudoElement_TitleBarMaxButton,
                PseudoElement_TitleBarMinButton,
                PseudoElement_TitleBarNormalButton,
                PseudoElement_TitleBarShadeButton,
                PseudoElement_TitleBarUnshadeButton,
                PseudoElement_TitleBarContextHelpButton
            };

            for (unsigned int i = 0; i < sizeof(pes)/sizeof(int); i++) {
                int pe = pes[i];
                QStyle::SubControl sc = knownPseudoElements[pe].subControl;
                ir = layout[sc];
                if (!ir.isValid())
                    continue;
                QRenderRule subSubRule = renderRule(w, opt, pe);
                subSubRule.drawRule(p, ir);
                pm = standardIcon(subControlIcon(pe), nullptr, w).pixmap(subSubRule.contentsRect(ir).size());
                drawItemPixmap(p, ir, Qt::AlignCenter, pm);
            }

            return;
        }
        break;


    default:
        break;
    }

    baseStyle()->drawComplexControl(cc, opt, p, w);
}

void QStyleSheetStyle::renderMenuItemIcon(const QStyleOptionMenuItem *mi, QPainter *p, const QWidget *w,
                                          const QRect &rect, QRenderRule &subRule) const
{
    const QIcon::Mode mode = mi->state & QStyle::State_Enabled
                           ? (mi->state & QStyle::State_Selected ? QIcon::Active : QIcon::Normal)
                           : QIcon::Disabled;
    const bool checked = mi->checkType != QStyleOptionMenuItem::NotCheckable && mi->checked;
    const QPixmap pixmap(mi->icon.pixmap(pixelMetric(PM_SmallIconSize), mode,
                        checked ? QIcon::On : QIcon::Off));
    const int pixw = pixmap.width() / pixmap.devicePixelRatio();
    const int pixh = pixmap.height() / pixmap.devicePixelRatio();
    QRenderRule iconRule = renderRule(w, mi, PseudoElement_MenuIcon);
    if (!iconRule.hasGeometry()) {
        iconRule.geo = new QStyleSheetGeometryData(pixw, pixh, pixw, pixh, -1, -1);
    } else {
        iconRule.geo->width = pixw;
        iconRule.geo->height = pixh;
    }
    QRect iconRect = positionRect(w, subRule, iconRule, PseudoElement_MenuIcon, rect, mi->direction);
    if (mi->direction == Qt::LeftToRight)
        iconRect.moveLeft(iconRect.left());
    else
        iconRect.moveRight(iconRect.right());
    iconRule.drawRule(p, iconRect);
    QRect pmr(0, 0, pixw, pixh);
    pmr.moveCenter(iconRect.center());
    p->drawPixmap(pmr.topLeft(), pixmap);
}

void QStyleSheetStyle::drawControl(ControlElement ce, const QStyleOption *opt, QPainter *p,
                          const QWidget *w) const
{
    RECURSION_GUARD(baseStyle()->drawControl(ce, opt, p, w); return)

    QRenderRule rule = renderRule(w, opt);
    int pe1 = PseudoElement_None, pe2 = PseudoElement_None;
    bool fallback = false;

    switch (ce) {
    case CE_ToolButtonLabel:
        if (const QStyleOptionToolButton *btn = qstyleoption_cast<const QStyleOptionToolButton *>(opt)) {
            if (rule.hasBox() || btn->features & QStyleOptionToolButton::Arrow) {
                QWindowsStyle::drawControl(ce, opt, p, w);
            } else {
                QStyleOptionToolButton butOpt(*btn);
                rule.configurePalette(&butOpt.palette, QPalette::ButtonText, QPalette::Button);
                baseStyle()->drawControl(ce, &butOpt, p, w);
            }
            return;
        }
        break;

    case CE_FocusFrame:
        if (!rule.hasNativeBorder()) {
            rule.drawBorder(p, opt->rect);
            return;
        }
        break;

    case CE_PushButton:
        if (const QStyleOptionButton *btn = qstyleoption_cast<const QStyleOptionButton *>(opt)) {
            if (rule.hasDrawable() || rule.hasBox() || rule.hasPosition() || rule.hasPalette() ||
                    ((btn->features & QStyleOptionButton::HasMenu) && hasStyleRule(w, PseudoElement_PushButtonMenuIndicator))) {
                ParentStyle::drawControl(ce, opt, p, w);
                return;
            }
        }
        break;
    case CE_PushButtonBevel:
        if (const QStyleOptionButton *btn = qstyleoption_cast<const QStyleOptionButton *>(opt)) {
            QStyleOptionButton btnOpt(*btn);
            btnOpt.rect = rule.borderRect(opt->rect);
            if (rule.hasNativeBorder()) {
                rule.drawBackgroundImage(p, btnOpt.rect);
                rule.configurePalette(&btnOpt.palette, QPalette::ButtonText, QPalette::Button);
                bool customMenu = (btn->features & QStyleOptionButton::HasMenu
                                   && hasStyleRule(w, PseudoElement_PushButtonMenuIndicator));
                if (customMenu)
                    btnOpt.features &= ~QStyleOptionButton::HasMenu;
                if (rule.baseStyleCanDraw()) {
                    baseStyle()->drawControl(ce, &btnOpt, p, w);
                } else {
                    QWindowsStyle::drawControl(ce, &btnOpt, p, w);
                }
                rule.drawImage(p, rule.contentsRect(opt->rect));
                if (!customMenu)
                    return;
            } else {
                rule.drawRule(p, opt->rect);
            }

            if (btn->features & QStyleOptionButton::HasMenu) {
                QRenderRule subRule = renderRule(w, opt, PseudoElement_PushButtonMenuIndicator);
                QRect ir = positionRect(w, rule, subRule, PseudoElement_PushButtonMenuIndicator, opt->rect, opt->direction);
                if (subRule.hasDrawable()) {
                    subRule.drawRule(p, ir);
                } else {
                    btnOpt.rect = ir;
                    baseStyle()->drawPrimitive(PE_IndicatorArrowDown, &btnOpt, p, w);
                }
            }
        }
        return;

    case CE_PushButtonLabel:
        if (const QStyleOptionButton *button = qstyleoption_cast<const QStyleOptionButton *>(opt)) {
            QStyleOptionButton butOpt(*button);
            rule.configurePalette(&butOpt.palette, QPalette::ButtonText, QPalette::Button);

            const QFont oldFont = p->font();
            if (rule.hasFont)
                p->setFont(rule.font.resolve(p->font()));

            if (rule.hasPosition() || rule.hasIcon()) {
                uint tf = Qt::TextShowMnemonic;
                QRect textRect = button->rect;

                const uint horizontalAlignMask = Qt::AlignHCenter | Qt::AlignLeft | Qt::AlignRight;
                const uint verticalAlignMask = Qt::AlignVCenter | Qt::AlignTop | Qt::AlignBottom;

                if (rule.hasPosition() && rule.position()->textAlignment != 0) {
                    Qt::Alignment textAlignment = rule.position()->textAlignment;
                    tf |= (textAlignment & verticalAlignMask) ? (textAlignment & verticalAlignMask) : Qt::AlignVCenter;
                    tf |= (textAlignment & horizontalAlignMask) ? (textAlignment & horizontalAlignMask) : Qt::AlignHCenter;
                    if (!styleHint(SH_UnderlineShortcut, button, w))
                        tf |= Qt::TextHideMnemonic;
                } else {
                    tf |= Qt::AlignVCenter | Qt::AlignHCenter;
                }

                QIcon icon = rule.hasIcon() ? rule.icon()->icon : button->icon;
                if (!icon.isNull()) {
                    //Group both icon and text
                    QRect iconRect;
                    QIcon::Mode mode = button->state & State_Enabled ? QIcon::Normal : QIcon::Disabled;
                    if (mode == QIcon::Normal && button->state & State_HasFocus)
                        mode = QIcon::Active;
                    QIcon::State state = QIcon::Off;
                    if (button->state & State_On)
                        state = QIcon::On;

                    QPixmap pixmap = icon.pixmap(button->iconSize, mode, state);
                    int pixmapWidth = pixmap.width() / pixmap.devicePixelRatio();
                    int pixmapHeight = pixmap.height() / pixmap.devicePixelRatio();
                    int labelWidth = pixmapWidth;
                    int labelHeight = pixmapHeight;
                    int iconSpacing = 4;//### 4 is currently hardcoded in QPushButton::sizeHint()
                    int textWidth = button->fontMetrics.boundingRect(opt->rect, tf, button->text).width();
                    if (!button->text.isEmpty())
                        labelWidth += (textWidth + iconSpacing);

                    //Determine label alignment:
                    if (tf & Qt::AlignLeft) { /*left*/
                        iconRect = QRect(textRect.x(), textRect.y() + (textRect.height() - labelHeight) / 2,
                                         pixmapWidth, pixmapHeight);
                    } else if (tf & Qt::AlignHCenter) { /* center */
                        iconRect = QRect(textRect.x() + (textRect.width() - labelWidth) / 2,
                                         textRect.y() + (textRect.height() - labelHeight) / 2,
                                         pixmapWidth, pixmapHeight);
                    } else { /*right*/
                        iconRect = QRect(textRect.x() + textRect.width() - labelWidth,
                                         textRect.y() + (textRect.height() - labelHeight) / 2,
                                         pixmapWidth, pixmapHeight);
                    }

                    iconRect = visualRect(button->direction, textRect, iconRect);

                    // Left align, adjust the text-rect according to the icon instead
                    tf &= ~horizontalAlignMask;
                    tf |= Qt::AlignLeft;

                    if (button->direction == Qt::RightToLeft)
                        textRect.setRight(iconRect.left() - iconSpacing);
                    else
                        textRect.setLeft(iconRect.left() + iconRect.width() + iconSpacing);

                    if (button->state & (State_On | State_Sunken))
                        iconRect.translate(pixelMetric(PM_ButtonShiftHorizontal, opt, w),
                                           pixelMetric(PM_ButtonShiftVertical, opt, w));
                    p->drawPixmap(iconRect, pixmap);
                }

                if (button->state & (State_On | State_Sunken))
                    textRect.translate(pixelMetric(PM_ButtonShiftHorizontal, opt, w),
                                 pixelMetric(PM_ButtonShiftVertical, opt, w));

                if (button->features & QStyleOptionButton::HasMenu) {
                    int indicatorSize = pixelMetric(PM_MenuButtonIndicator, button, w);
                    if (button->direction == Qt::LeftToRight)
                        textRect = textRect.adjusted(0, 0, -indicatorSize, 0);
                    else
                        textRect = textRect.adjusted(indicatorSize, 0, 0, 0);
                }
                drawItemText(p, textRect, tf, butOpt.palette, (button->state & State_Enabled),
                             button->text, QPalette::ButtonText);
            } else {
                ParentStyle::drawControl(ce, &butOpt, p, w);
            }

            if (rule.hasFont)
                p->setFont(oldFont);
        }
        return;

    case CE_RadioButton:
    case CE_CheckBox:
        if (rule.hasBox() || !rule.hasNativeBorder() || rule.hasDrawable() || hasStyleRule(w, PseudoElement_Indicator)) {
            rule.drawRule(p, opt->rect);
            ParentStyle::drawControl(ce, opt, p, w);
            return;
        } else if (const QStyleOptionButton *btn = qstyleoption_cast<const QStyleOptionButton *>(opt)) {
            QStyleOptionButton butOpt(*btn);
            rule.configurePalette(&butOpt.palette, QPalette::ButtonText, QPalette::Button);
            baseStyle()->drawControl(ce, &butOpt, p, w);
            return;
        }
        break;
    case CE_RadioButtonLabel:
    case CE_CheckBoxLabel:
        if (const QStyleOptionButton *btn = qstyleoption_cast<const QStyleOptionButton *>(opt)) {
            QStyleOptionButton butOpt(*btn);
            rule.configurePalette(&butOpt.palette, QPalette::ButtonText, QPalette::Button);
            ParentStyle::drawControl(ce, &butOpt, p, w);
        }
        return;

    case CE_Splitter:
        pe1 = PseudoElement_SplitterHandle;
        break;

    case CE_ToolBar:
        if (rule.hasBackground()) {
            rule.drawBackground(p, opt->rect);
        }
        if (rule.hasBorder()) {
            rule.drawBorder(p, rule.borderRect(opt->rect));
        } else {
#if QT_CONFIG(toolbar)
            if (const QStyleOptionToolBar *tb = qstyleoption_cast<const QStyleOptionToolBar *>(opt)) {
                QStyleOptionToolBar newTb(*tb);
                newTb.rect = rule.borderRect(opt->rect);
                baseStyle()->drawControl(ce, &newTb, p, w);
            }
#endif // QT_CONFIG(toolbar)
        }
        return;

    case CE_MenuEmptyArea:
    case CE_MenuBarEmptyArea:
        if (rule.hasDrawable()) {
            // Drawn by PE_Widget
            return;
        }
        break;

    case CE_MenuTearoff:
    case CE_MenuScroller:
        if (const QStyleOptionMenuItem *m = qstyleoption_cast<const QStyleOptionMenuItem *>(opt)) {
            QStyleOptionMenuItem mi(*m);
            int pe = ce == CE_MenuTearoff ? PseudoElement_MenuTearoff : PseudoElement_MenuScroller;
            QRenderRule subRule = renderRule(w, opt, pe);
            mi.rect = subRule.contentsRect(opt->rect);
            rule.configurePalette(&mi.palette, QPalette::ButtonText, QPalette::Button);
            subRule.configurePalette(&mi.palette, QPalette::ButtonText, QPalette::Button);

            if (subRule.hasDrawable()) {
                subRule.drawRule(p, opt->rect);
            } else {
                baseStyle()->drawControl(ce, &mi, p, w);
            }
        }
        return;

    case CE_MenuItem:
        if (const QStyleOptionMenuItem *m = qstyleoption_cast<const QStyleOptionMenuItem *>(opt)) {
            QStyleOptionMenuItem mi(*m);

            int pseudo = (mi.menuItemType == QStyleOptionMenuItem::Separator) ? PseudoElement_MenuSeparator : PseudoElement_Item;
            QRenderRule subRule = renderRule(w, opt, pseudo);
            mi.rect = subRule.contentsRect(opt->rect);
            rule.configurePalette(&mi.palette, QPalette::ButtonText, QPalette::Button);
            rule.configurePalette(&mi.palette, QPalette::HighlightedText, QPalette::Highlight);
            subRule.configurePalette(&mi.palette, QPalette::ButtonText, QPalette::Button);
            subRule.configurePalette(&mi.palette, QPalette::HighlightedText, QPalette::Highlight);
            QFont oldFont = p->font();
            if (subRule.hasFont)
                p->setFont(subRule.font.resolve(mi.font));
            else
                p->setFont(mi.font);

            // We fall back to drawing with the style sheet code whenever at least one of the
            // items are styled in an incompatible way, such as having a background image.
            QRenderRule allRules = renderRule(w, PseudoElement_Item, PseudoClass_Any);

            if ((pseudo == PseudoElement_MenuSeparator) && subRule.hasDrawable()) {
                subRule.drawRule(p, opt->rect);
            } else if ((pseudo == PseudoElement_Item)
                        && (allRules.hasBox() || allRules.hasBorder() || subRule.hasFont
                            || (allRules.background() && !allRules.background()->pixmap.isNull()))) {
                subRule.drawRule(p, opt->rect);
                if (subRule.hasBackground()) {
                    mi.palette.setBrush(QPalette::Highlight, Qt::NoBrush);
                    mi.palette.setBrush(QPalette::Button, Qt::NoBrush);
                } else {
                    mi.palette.setBrush(QPalette::Highlight, mi.palette.brush(QPalette::Button));
                }
                mi.palette.setBrush(QPalette::HighlightedText, mi.palette.brush(QPalette::ButtonText));

                int textRectOffset = m->maxIconWidth;
                if (!mi.icon.isNull()) {
                    renderMenuItemIcon(&mi, p, w, opt->rect, subRule);
                } else if (mi.menuHasCheckableItems) {
                    const bool checkable = mi.checkType != QStyleOptionMenuItem::NotCheckable;
                    const bool checked = checkable ? mi.checked : false;

                    const QRenderRule subSubRule = renderRule(w, opt, PseudoElement_MenuCheckMark);
                    const QRect cmRect = positionRect(w, subRule, subSubRule, PseudoElement_MenuCheckMark, opt->rect, opt->direction);
                    if (checkable && (subSubRule.hasDrawable() || checked)) {
                        QStyleOptionMenuItem newMi = mi;
                        if (opt->state & QStyle::State_Enabled)
                            newMi.state |= State_Enabled;
                        if (mi.checked)
                            newMi.state |= State_On;
                        newMi.rect = cmRect;
                        drawPrimitive(PE_IndicatorMenuCheckMark, &newMi, p, w);
                    }
                    textRectOffset = std::max(textRectOffset, cmRect.width());
                }

                QRect textRect = subRule.contentsRect(opt->rect);
                textRect.setLeft(textRect.left() + textRectOffset);
                textRect.setWidth(textRect.width() - mi.reservedShortcutWidth);
                const QRect vTextRect = visualRect(opt->direction, m->rect, textRect);

                QStringView s(mi.text);
                p->setPen(mi.palette.buttonText().color());
                if (!s.isEmpty()) {
                    int text_flags = Qt::AlignLeft | Qt::AlignVCenter | Qt::TextShowMnemonic | Qt::TextDontClip | Qt::TextSingleLine;
                    if (!styleHint(SH_UnderlineShortcut, &mi, w))
                        text_flags |= Qt::TextHideMnemonic;
                    qsizetype t = s.indexOf(u'\t');
                    if (t >= 0) {
                        QRect vShortcutRect = visualRect(opt->direction, mi.rect,
                            QRect(textRect.topRight(), QPoint(mi.rect.right(), textRect.bottom())));
                        p->drawText(vShortcutRect, text_flags, s.mid(t + 1).toString());
                        s = s.left(t);
                    }
                    p->drawText(vTextRect, text_flags, s.left(t).toString());
                }

                if (mi.menuItemType == QStyleOptionMenuItem::SubMenu) {// draw sub menu arrow
                    PrimitiveElement arrow = (opt->direction == Qt::RightToLeft) ? PE_IndicatorArrowLeft : PE_IndicatorArrowRight;
                    QRenderRule subRule2 = renderRule(w, opt, PseudoElement_MenuRightArrow);
                    mi.rect = positionRect(w, subRule, subRule2, PseudoElement_MenuRightArrow, opt->rect, mi.direction);
                    drawPrimitive(arrow, &mi, p, w);
                }
            } else if (!mi.icon.isNull() && hasStyleRule(w, PseudoElement_MenuIcon)) {
                // we wouldn't be here if the item itself would be styled, so now we only want
                // the text from the default style, and then draw the icon ourselves.
                QStyleOptionMenuItem newMi = mi;
                newMi.icon = {};
                newMi.checkType = QStyleOptionMenuItem::NotCheckable;
                if (rule.baseStyleCanDraw() && subRule.baseStyleCanDraw())
                    baseStyle()->drawControl(ce, &newMi, p, w);
                else
                    ParentStyle::drawControl(ce, &newMi, p, w);
                renderMenuItemIcon(&mi, p, w, opt->rect, subRule);
            } else if (hasStyleRule(w, PseudoElement_MenuCheckMark) || hasStyleRule(w, PseudoElement_MenuRightArrow)) {
                QWindowsStyle::drawControl(ce, &mi, p, w);
                if (mi.checkType != QStyleOptionMenuItem::NotCheckable && !mi.checked) {
                    // We have a style defined, but QWindowsStyle won't draw anything if not checked.
                    // So we mimic what QWindowsStyle would do.
                    int checkcol = qMax<int>(mi.maxIconWidth, QWindowsStylePrivate::windowsCheckMarkWidth);
                    QRect vCheckRect = visualRect(opt->direction, mi.rect, QRect(mi.rect.x(), mi.rect.y(), checkcol, mi.rect.height()));
                    if (mi.state.testFlag(State_Enabled) && mi.state.testFlag(State_Selected)) {
                        qDrawShadePanel(p, vCheckRect, mi.palette, true, 1, &mi.palette.brush(QPalette::Button));
                    } else {
                        QBrush fill(mi.palette.light().color(), Qt::Dense4Pattern);
                        qDrawShadePanel(p, vCheckRect, mi.palette, true, 1, &fill);
                    }
                    QRenderRule subSubRule = renderRule(w, opt, PseudoElement_MenuCheckMark);
                    if (subSubRule.hasDrawable()) {
                        QStyleOptionMenuItem newMi(mi);
                        newMi.rect = visualRect(opt->direction, mi.rect, QRect(mi.rect.x() + QWindowsStylePrivate::windowsItemFrame,
                                                                               mi.rect.y() + QWindowsStylePrivate::windowsItemFrame,
                                                                               checkcol - 2 * QWindowsStylePrivate::windowsItemFrame,
                                                                               mi.rect.height() - 2 * QWindowsStylePrivate::windowsItemFrame));
                        drawPrimitive(PE_IndicatorMenuCheckMark, &newMi, p, w);
                    }
                }
            } else {
                if (rule.hasDrawable() && !subRule.hasDrawable() && !(opt->state & QStyle::State_Selected)) {
                    mi.palette.setColor(QPalette::Window, Qt::transparent);
                    mi.palette.setColor(QPalette::Button, Qt::transparent);
                }
                if (rule.baseStyleCanDraw() && subRule.baseStyleCanDraw()) {
                    baseStyle()->drawControl(ce, &mi, p, w);
                } else {
                    ParentStyle::drawControl(ce, &mi, p, w);
                }
            }

            p->setFont(oldFont);

            return;
        }
        return;

    case CE_MenuBarItem:
        if (const QStyleOptionMenuItem *m = qstyleoption_cast<const QStyleOptionMenuItem *>(opt)) {
            QStyleOptionMenuItem mi(*m);
            QRenderRule subRule = renderRule(w, opt, PseudoElement_Item);
            mi.rect = subRule.contentsRect(opt->rect);
            rule.configurePalette(&mi.palette, QPalette::ButtonText, QPalette::Button);
            subRule.configurePalette(&mi.palette, QPalette::ButtonText, QPalette::Button);

            if (subRule.hasDrawable()) {
                subRule.drawRule(p, opt->rect);
                QCommonStyle::drawControl(ce, &mi, p, w); // deliberate bypass of the base
            } else {
                if (rule.hasDrawable() && !(opt->state & QStyle::State_Selected)) {
                    // So that the menu bar background is not hidden by the items
                    mi.palette.setColor(QPalette::Window, Qt::transparent);
                    mi.palette.setColor(QPalette::Button, Qt::transparent);
                }
                baseStyle()->drawControl(ce, &mi, p, w);
            }
        }
        return;

#if QT_CONFIG(combobox)
    case CE_ComboBoxLabel:
        if (!rule.hasBox())
            break;
        if (const QStyleOptionComboBox *cb = qstyleoption_cast<const QStyleOptionComboBox *>(opt)) {
            QRect editRect = subControlRect(CC_ComboBox, cb, SC_ComboBoxEditField, w);
            p->save();
            p->setClipRect(editRect);
            if (!cb->currentIcon.isNull()) {
                int spacing = rule.hasBox() ? rule.box()->spacing : -1;
                if (spacing == -1)
                    spacing = 6;
                QIcon::Mode mode = cb->state & State_Enabled ? QIcon::Normal : QIcon::Disabled;
                QPixmap pixmap = cb->currentIcon.pixmap(cb->iconSize, mode);
                QRect iconRect(editRect);
                iconRect.setWidth(cb->iconSize.width());
                iconRect = alignedRect(cb->direction,
                                       Qt::AlignLeft | Qt::AlignVCenter,
                                       iconRect.size(), editRect);
                drawItemPixmap(p, iconRect, Qt::AlignCenter, pixmap);

                if (cb->direction == Qt::RightToLeft)
                        editRect.translate(-spacing - cb->iconSize.width(), 0);
                else
                        editRect.translate(cb->iconSize.width() + spacing, 0);
            }
            if (!cb->currentText.isEmpty() && !cb->editable) {
                QPalette styledPalette(cb->palette);
                rule.configurePalette(&styledPalette, QPalette::Text, QPalette::Base);
                drawItemText(p, editRect.adjusted(0, 0, 0, 0), cb->textAlignment, styledPalette,
                             cb->state & State_Enabled, cb->currentText, QPalette::Text);
            }
            p->restore();
            return;
        }
        break;
#endif // QT_CONFIG(combobox)

    case CE_Header:
        if (hasStyleRule(w, PseudoElement_HeaderViewUpArrow)
            || hasStyleRule(w, PseudoElement_HeaderViewDownArrow)) {
            ParentStyle::drawControl(ce, opt, p, w);
            return;
        }
        if (hasStyleRule(w, PseudoElement_HeaderViewSection)) {
            QRenderRule subRule = renderRule(w, opt, PseudoElement_HeaderViewSection);
            if (!subRule.hasNativeBorder() || !subRule.baseStyleCanDraw()
                || subRule.hasBackground() || subRule.hasPalette() || subRule.hasFont || subRule.hasBorder()) {
                ParentStyle::drawControl(ce, opt, p, w);
                return;
            }
        }
        break;
    case CE_HeaderSection:
        if (const QStyleOptionHeader *header = qstyleoption_cast<const QStyleOptionHeader *>(opt)) {
            QRenderRule subRule = renderRule(w, opt, PseudoElement_HeaderViewSection);
            if (subRule.hasNativeBorder()) {
                QStyleOptionHeader hdr(*header);
                subRule.configurePalette(&hdr.palette, QPalette::ButtonText, QPalette::Button);

                if (subRule.baseStyleCanDraw()) {
                    baseStyle()->drawControl(CE_HeaderSection, &hdr, p, w);
                } else {
                    QWindowsStyle::drawControl(CE_HeaderSection, &hdr, p, w);
                }
            } else {
                subRule.drawRule(p, opt->rect);
            }
            return;
        }
        break;

    case CE_HeaderLabel:
        if (const QStyleOptionHeader *header = qstyleoption_cast<const QStyleOptionHeader *>(opt)) {
          QStyleOptionHeaderV2 hdr;
          QStyleOptionHeader &v1Copy = hdr;
          if (auto v2 = qstyleoption_cast<const QStyleOptionHeaderV2 *>(opt))
              hdr = *v2;
          else
              v1Copy = *header;
            QRenderRule subRule = renderRule(w, opt, PseudoElement_HeaderViewSection);
            if ((hasStyleRule(w, PseudoElement_HeaderViewUpArrow)
             || hasStyleRule(w, PseudoElement_HeaderViewDownArrow))
             && hdr.sortIndicator != QStyleOptionHeader::None) {
                const QRect arrowRect = subElementRect(SE_HeaderArrow, opt, w);
                if (hdr.orientation == Qt::Horizontal)
                    hdr.rect.setWidth(hdr.rect.width() - arrowRect.width());
                else
                    hdr.rect.setHeight(hdr.rect.height() - arrowRect.height());
            }
            subRule.configurePalette(&hdr.palette, QPalette::ButtonText, QPalette::Button);
            if (subRule.hasFont) {
                QFont oldFont = p->font();
                p->setFont(subRule.font.resolve(p->font()));
                ParentStyle::drawControl(ce, &hdr, p, w);
                p->setFont(oldFont);
            } else {
                baseStyle()->drawControl(ce, &hdr, p, w);
            }
            return;
        }
        break;

    case CE_HeaderEmptyArea:
        if (rule.hasDrawable()) {
            return;
        }
        break;

    case CE_ProgressBar:
        QWindowsStyle::drawControl(ce, opt, p, w);
        return;

    case CE_ProgressBarGroove:
        if (!rule.hasNativeBorder()) {
            rule.drawRule(p, rule.boxRect(opt->rect, Margin));
            return;
        }
        break;

    case CE_ProgressBarContents: {
        QRenderRule subRule = renderRule(w, opt, PseudoElement_ProgressBarChunk);
        if (subRule.hasDrawable()) {
            if (const QStyleOptionProgressBar *pb = qstyleoption_cast<const QStyleOptionProgressBar *>(opt)) {
                p->save();
                p->setClipRect(pb->rect);

                qint64 minimum = qint64(pb->minimum);
                qint64 maximum = qint64(pb->maximum);
                qint64 progress = qint64(pb->progress);
                bool vertical = !(pb->state & QStyle::State_Horizontal);
                bool inverted = pb->invertedAppearance;

                QTransform m;
                QRect rect = pb->rect;
                if (vertical) {
                    rect = QRect(rect.y(), rect.x(), rect.height(), rect.width());
                    m.rotate(90);
                    m.translate(0, -(rect.height() + rect.y()*2));
                }

                bool reverse = ((!vertical && (pb->direction == Qt::RightToLeft)) || vertical);
                if (inverted)
                    reverse = !reverse;
                const bool indeterminate = pb->minimum == pb->maximum;
                const auto fillRatio = indeterminate ? 0.50 : double(progress - minimum) / (maximum - minimum);
                const auto fillWidth = static_cast<int>(rect.width() * fillRatio);
                int chunkWidth = fillWidth;
                if (subRule.hasContentsSize()) {
                    QSize sz = subRule.size();
                    chunkWidth = (opt->state & QStyle::State_Horizontal) ? sz.width() : sz.height();
                }

                QRect r = rect;
#if QT_CONFIG(animation)
                Q_D(const QWindowsStyle);
#endif
                if (pb->minimum == 0 && pb->maximum == 0) {
                    int chunkCount = fillWidth/chunkWidth;
                    int offset = 0;
#if QT_CONFIG(animation)
                    if (QProgressStyleAnimation *animation = qobject_cast<QProgressStyleAnimation*>(d->animation(opt->styleObject)))
                        offset = animation->animationStep() * 8 % rect.width();
                    else
                        d->startAnimation(new QProgressStyleAnimation(d->animationFps, opt->styleObject));
#endif
                    int x = reverse ? r.left() + r.width() - offset - chunkWidth : r.x() + offset;
                    while (chunkCount > 0) {
                        r.setRect(x, rect.y(), chunkWidth, rect.height());
                        r = m.mapRect(QRectF(r)).toRect();
                        subRule.drawRule(p, r);
                        x += reverse ? -chunkWidth : chunkWidth;
                        if (reverse ? x < rect.left() : x > rect.right())
                            break;
                        --chunkCount;
                    }

                    r = rect;
                    x = reverse ? r.right() - (r.left() - x - chunkWidth)
                                : r.left() + (x - r.right() - chunkWidth);
                    while (chunkCount > 0) {
                        r.setRect(x, rect.y(), chunkWidth, rect.height());
                        r = m.mapRect(QRectF(r)).toRect();
                        subRule.drawRule(p, r);
                        x += reverse ? -chunkWidth : chunkWidth;
                        --chunkCount;
                    };
                } else if (chunkWidth > 0) {
                    const auto ceil = [](qreal x) { return int(x) + (x > 0 && x != int(x)); };
                    const int chunkCount = ceil(qreal(fillWidth)/chunkWidth);
                    int x = reverse ? r.left() + r.width() - chunkWidth : r.x();

                    for (int i = 0; i < chunkCount; ++i) {
                        r.setRect(x, rect.y(), chunkWidth, rect.height());
                        r = m.mapRect(QRectF(r)).toRect();
                        subRule.drawRule(p, r);
                        x += reverse ? -chunkWidth : chunkWidth;
                    }
#if QT_CONFIG(animation)
                    d->stopAnimation(opt->styleObject);
#endif
                }

                p->restore();
                return;
            }
        }
                               }
        break;

    case CE_ProgressBarLabel:
        if (const QStyleOptionProgressBar *pb = qstyleoption_cast<const QStyleOptionProgressBar *>(opt)) {
            if (rule.hasBox() || rule.hasBorder() || hasStyleRule(w, PseudoElement_ProgressBarChunk)) {
                drawItemText(p, pb->rect, pb->textAlignment | Qt::TextSingleLine, pb->palette,
                             pb->state & State_Enabled, pb->text, QPalette::Text);
            } else {
                QStyleOptionProgressBar pbCopy(*pb);
                rule.configurePalette(&pbCopy.palette, QPalette::HighlightedText, QPalette::Highlight);
                baseStyle()->drawControl(ce, &pbCopy, p, w);
            }
            return;
        }
        break;

    case CE_SizeGrip:
        if (const QStyleOptionSizeGrip *sgOpt = qstyleoption_cast<const QStyleOptionSizeGrip *>(opt)) {
            if (rule.hasDrawable()) {
                rule.drawFrame(p, opt->rect);
                p->save();
                static constexpr int rotation[] = { 180, 270, 90, 0 };
                if (rotation[sgOpt->corner]) {
                    p->translate(opt->rect.center());
                    p->rotate(rotation[sgOpt->corner]);
                    p->translate(-opt->rect.center());
                }
                rule.drawImage(p, opt->rect);
                p->restore();
            } else {
                QStyleOptionSizeGrip sg(*sgOpt);
                sg.rect = rule.contentsRect(opt->rect);
                baseStyle()->drawControl(CE_SizeGrip, &sg, p, w);
            }
            return;
        }
        break;

    case CE_ToolBoxTab:
        QWindowsStyle::drawControl(ce, opt, p, w);
        return;

    case CE_ToolBoxTabShape: {
            QRenderRule subRule = renderRule(w, opt, PseudoElement_ToolBoxTab);
            if (subRule.hasDrawable()) {
                subRule.drawRule(p, opt->rect);
                return;
            }
                            }
        break;

    case CE_ToolBoxTabLabel:
        if (const QStyleOptionToolBox *box = qstyleoption_cast<const QStyleOptionToolBox *>(opt)) {
            QStyleOptionToolBox boxCopy(*box);
            QRenderRule subRule = renderRule(w, opt, PseudoElement_ToolBoxTab);
            subRule.configurePalette(&boxCopy.palette, QPalette::ButtonText, QPalette::Button);
            QFont oldFont = p->font();
            if (subRule.hasFont)
                p->setFont(subRule.font.resolve(p->font()));
            boxCopy.rect = subRule.contentsRect(opt->rect);
            if (subRule.hasImage()) {
                // the image is already drawn with CE_ToolBoxTabShape, adjust rect here
                const int iconExtent = proxy()->pixelMetric(QStyle::PM_SmallIconSize, box, w);
                boxCopy.rect.setLeft(boxCopy.rect.left() + iconExtent);
            }
            QWindowsStyle::drawControl(ce, &boxCopy, p , w);
            if (subRule.hasFont)
                p->setFont(oldFont);
            return;
        }
        break;

    case CE_ScrollBarAddPage:
        pe1 = PseudoElement_ScrollBarAddPage;
        break;

    case CE_ScrollBarSubPage:
        pe1 = PseudoElement_ScrollBarSubPage;
        break;

    case CE_ScrollBarAddLine:
        pe1 = PseudoElement_ScrollBarAddLine;
        pe2 = (opt->state & QStyle::State_Horizontal) ? PseudoElement_ScrollBarRightArrow : PseudoElement_ScrollBarDownArrow;
        fallback = true;
        break;

    case CE_ScrollBarSubLine:
        pe1 = PseudoElement_ScrollBarSubLine;
        pe2 = (opt->state & QStyle::State_Horizontal) ? PseudoElement_ScrollBarLeftArrow : PseudoElement_ScrollBarUpArrow;
        fallback = true;
        break;

    case CE_ScrollBarFirst:
        pe1 = PseudoElement_ScrollBarFirst;
        break;

    case CE_ScrollBarLast:
        pe1 = PseudoElement_ScrollBarLast;
        break;

    case CE_ScrollBarSlider:
        pe1 = PseudoElement_ScrollBarSlider;
        fallback = true;
        break;

#if QT_CONFIG(itemviews)
    case CE_ItemViewItem:
        if (const QStyleOptionViewItem *vopt = qstyleoption_cast<const QStyleOptionViewItem *>(opt)) {
            QRenderRule subRule = renderRule(w, opt, PseudoElement_ViewItem);
            QStyleOptionViewItem optCopy(*vopt);
            if (subRule.hasDrawable()) {
                subRule.configurePalette(&optCopy.palette, vopt->state & QStyle::State_Selected ? QPalette::HighlightedText : QPalette::Text,
                                                           vopt->state & QStyle::State_Selected ? QPalette::Highlight : QPalette::Base);
                QWindowsStyle::drawControl(ce, &optCopy, p, w);
            } else {
                p->save();
                if (hasStyleRule(w, PseudoElement_Indicator)) {
                    // there is a rule for the indicator, but no rule for the item itself (otherwise
                    // the previous path would have been taken); only draw the indicator using the
                    // rule (via QWindows/QCommonStyle), then let the base style handle the rest.
                    QStyleOptionViewItem optIndicator(*vopt);
                    subRule.configurePalette(&optIndicator.palette,
                                            vopt->state & QStyle::State_Selected
                                                        ? QPalette::HighlightedText
                                                        : QPalette::Text,
                                            vopt->state & QStyle::State_Selected
                                                        ? QPalette::Highlight
                                                        : QPalette::Base);
                    // only draw the indicator; no text or background
                    optIndicator.backgroundBrush = Qt::NoBrush; // no background
                    optIndicator.text.clear();
                    QWindowsStyle::drawControl(ce, &optIndicator, p, w);

                    // If the indicator has an icon, it has been drawn now.
                    // => don't draw it again.
                    optCopy.icon = QIcon();

                    // Now draw text, background, and highlight, but not the indicator  with the
                    // base style. Since we can't turn off HasCheckIndicator to prevent the base
                    // style from drawing the check indicator again (it would change how the item
                    // gets laid out) we have to clip the indicator that's already been painted.
                    const QRect checkRect = subElementRect(QStyle::SE_ItemViewItemCheckIndicator,
                                                           &optIndicator, w);
                    const QRegion clipRegion = QRegion(p->hasClipping() ? p->clipRegion()
                                                                        : QRegion(optIndicator.rect))
                                             - checkRect;
                    p->setClipRegion(clipRegion);
                }
                subRule.configurePalette(&optCopy.palette, QPalette::Text, QPalette::NoRole);
                baseStyle()->drawControl(ce, &optCopy, p, w);
                p->restore();
            }
            return;
        }
        break;
#endif // QT_CONFIG(itemviews)

#if QT_CONFIG(tabbar)
    case CE_TabBarTab:
        if (hasStyleRule(w, PseudoElement_TabBarTab)) {
            QWindowsStyle::drawControl(ce, opt, p, w);
            return;
        }
        break;

    case CE_TabBarTabLabel:
    case CE_TabBarTabShape:
        if (const QStyleOptionTab *tab = qstyleoption_cast<const QStyleOptionTab *>(opt)) {
            const auto foregroundRole = w ? w->foregroundRole() : QPalette::WindowText;
            QRenderRule subRule = renderRule(w, opt, PseudoElement_TabBarTab);
            QRect r = positionRect(w, subRule, PseudoElement_TabBarTab, opt->rect, opt->direction);
            if (ce == CE_TabBarTabShape && subRule.hasDrawable() && tab->shape < QTabBar::TriangularNorth) {
                subRule.drawRule(p, r);
                return;
            }
            QStyleOptionTab tabCopy(*tab);
            subRule.configurePalette(&tabCopy.palette, foregroundRole, QPalette::Base);
            QFont oldFont = p->font();
            if (subRule.hasFont)
                p->setFont(subRule.font.resolve(p->font()));
            if (subRule.hasBox() || !subRule.hasNativeBorder()) {
                tabCopy.rect = ce == CE_TabBarTabShape ? subRule.borderRect(r)
                                                       : subRule.contentsRect(r);
                QWindowsStyle::drawControl(ce, &tabCopy, p, w);
            } else {
                baseStyle()->drawControl(ce, &tabCopy, p, w);
            }
            if (subRule.hasFont)
                p->setFont(oldFont);

            return;
        }
       break;
#endif // QT_CONFIG(tabbar)

    case CE_ColumnViewGrip:
       if (rule.hasDrawable()) {
           rule.drawRule(p, opt->rect);
           return;
       }
       break;

    case CE_DockWidgetTitle:
       if (const QStyleOptionDockWidget *dwOpt = qstyleoption_cast<const QStyleOptionDockWidget *>(opt)) {
           QRenderRule subRule = renderRule(w, opt, PseudoElement_DockWidgetTitle);
           if (!subRule.hasDrawable() && !subRule.hasPosition())
               break;
           if (subRule.hasDrawable()) {
               subRule.drawRule(p, opt->rect);
           } else {
               QStyleOptionDockWidget dwCopy(*dwOpt);
               dwCopy.title = QString();
               baseStyle()->drawControl(ce, &dwCopy, p, w);
           }

           if (!dwOpt->title.isEmpty()) {
               QRect r = subElementRect(SE_DockWidgetTitleBarText, opt, w);
               if (dwOpt->verticalTitleBar) {
                   r = r.transposed();
                   p->save();
                   p->translate(r.left(), r.top() + r.width());
                   p->rotate(-90);
                   p->translate(-r.left(), -r.top());
                }
                r = subRule.contentsRect(r);

                Qt::Alignment alignment;
                if (subRule.hasPosition())
                    alignment = subRule.position()->textAlignment;
                if (alignment == 0)
                    alignment = Qt::AlignLeft;

                QString titleText = p->fontMetrics().elidedText(dwOpt->title, Qt::ElideRight, r.width());
                drawItemText(p, r,
                             alignment, dwOpt->palette,
                             dwOpt->state & State_Enabled, titleText,
                             QPalette::WindowText);

                if (dwOpt->verticalTitleBar)
                    p->restore();
            }

           return;
        }
        break;
    case CE_ShapedFrame:
        if (const QStyleOptionFrame *frm = qstyleoption_cast<const QStyleOptionFrame *>(opt)) {
            if (rule.hasNativeBorder()) {
                QStyleOptionFrame frmOpt(*frm);
                rule.configurePalette(&frmOpt.palette, QPalette::Text, QPalette::Base);
                frmOpt.rect = rule.borderRect(frmOpt.rect);
                baseStyle()->drawControl(ce, &frmOpt, p, w);
            }
            // else, borders are already drawn in PE_Widget
        }
        return;


    default:
        break;
    }

    if (pe1 != PseudoElement_None) {
        QRenderRule subRule = renderRule(w, opt, pe1);
        if (subRule.bg != nullptr || subRule.hasDrawable()) {
            //We test subRule.bg directly because hasBackground() would return false for background:none.
            //But we still don't want the default drawning in that case (example for QScrollBar::add-page) (task 198926)
            subRule.drawRule(p, opt->rect);
        } else if (fallback) {
            QWindowsStyle::drawControl(ce, opt, p, w);
            pe2 = PseudoElement_None;
        } else {
            baseStyle()->drawControl(ce, opt, p, w);
        }
        if (pe2 != PseudoElement_None) {
            QRenderRule subSubRule = renderRule(w, opt, pe2);
            QRect r = positionRect(w, subRule, subSubRule, pe2, opt->rect, opt->direction);
            subSubRule.drawRule(p, r);
        }
        return;
    }

    baseStyle()->drawControl(ce, opt, p, w);
}

void QStyleSheetStyle::drawItemPixmap(QPainter *p, const QRect &rect, int alignment, const
                                  QPixmap &pixmap) const
{
    baseStyle()->drawItemPixmap(p, rect, alignment, pixmap);
}

void QStyleSheetStyle::drawItemText(QPainter *painter, const QRect& rect, int alignment, const QPalette &pal,
                                bool enabled, const QString& text, QPalette::ColorRole textRole) const
{
    baseStyle()->drawItemText(painter, rect, alignment, pal, enabled, text, textRole);
}

void QStyleSheetStyle::drawPrimitive(PrimitiveElement pe, const QStyleOption *opt, QPainter *p,
                                     const QWidget *w) const
{
    RECURSION_GUARD(baseStyle()->drawPrimitive(pe, opt, p, w); return)

    int pseudoElement = PseudoElement_None;
    QRenderRule rule = renderRule(w, opt);
    QRect rect = opt->rect;

    switch (pe) {

    case PE_FrameStatusBarItem: {
        QRenderRule subRule = renderRule(w ? w->parentWidget() : nullptr, opt, PseudoElement_Item);
        if (subRule.hasDrawable()) {
            subRule.drawRule(p, opt->rect);
            return;
        }
        break;
                            }

    case PE_IndicatorArrowDown:
        pseudoElement = PseudoElement_DownArrow;
        break;

    case PE_IndicatorArrowUp:
        pseudoElement = PseudoElement_UpArrow;
        break;

    case PE_IndicatorRadioButton:
        pseudoElement = PseudoElement_ExclusiveIndicator;
        break;

    case PE_IndicatorItemViewItemCheck:
        pseudoElement = PseudoElement_ViewItemIndicator;
        break;

    case PE_IndicatorCheckBox:
        pseudoElement = PseudoElement_Indicator;
        break;

    case PE_IndicatorHeaderArrow:
        if (const QStyleOptionHeader *hdr = qstyleoption_cast<const QStyleOptionHeader *>(opt)) {
            pseudoElement = hdr->sortIndicator == QStyleOptionHeader::SortUp
                ? PseudoElement_HeaderViewUpArrow
                : PseudoElement_HeaderViewDownArrow;
        }
        break;

    case PE_PanelButtonTool:
    case PE_PanelButtonCommand:
#if QT_CONFIG(abstractbutton)
        if (qobject_cast<const QAbstractButton *>(w) && rule.hasBackground() && rule.hasNativeBorder()) {
            //the window style will draw the borders
            ParentStyle::drawPrimitive(pe, opt, p, w);
            if (!rule.background()->pixmap.isNull() || rule.hasImage()) {
                rule.drawRule(p, rule.boxRect(opt->rect, QRenderRule::Margin).adjusted(1,1,-1,-1));
            }
            return;
        }
#endif
        if (!rule.hasNativeBorder()) {
            rule.drawRule(p, rule.boxRect(opt->rect, QRenderRule::Margin));
            return;
        }
        break;

    case PE_IndicatorButtonDropDown: {
        QRenderRule subRule = renderRule(w, opt, PseudoElement_ToolButtonMenu);
        if (!subRule.hasNativeBorder()) {
            rule.drawBorder(p, opt->rect);
            return;
        }
        break;
                                     }

    case PE_FrameDefaultButton:
        if (rule.hasNativeBorder()) {
            if (rule.baseStyleCanDraw())
                break;
            QWindowsStyle::drawPrimitive(pe, opt, p, w);
        }
        return;

    case PE_FrameWindow:
    case PE_FrameDockWidget:
    case PE_Frame:
        if (const QStyleOptionFrame *frm = qstyleoption_cast<const QStyleOptionFrame *>(opt)) {
            if (rule.hasNativeBorder()) {
                QStyleOptionFrame frmOpt(*frm);
                rule.configurePalette(&frmOpt.palette, QPalette::Text, QPalette::Base);
                baseStyle()->drawPrimitive(pe, &frmOpt, p, w);
            } else {
                rule.drawBorder(p, rule.borderRect(opt->rect));
            }
        }
        return;

    case PE_PanelLineEdit:
        if (const QStyleOptionFrame *frm = qstyleoption_cast<const QStyleOptionFrame *>(opt)) {
            // Fall back to container widget's render rule
            if (w) {
                if (QWidget *container = containerWidget(w); container != w) {
                    QRenderRule containerRule = renderRule(container, opt);
                    if (!containerRule.hasNativeBorder() || !containerRule.baseStyleCanDraw())
                        return;
                    rule = containerRule;
                }
            }

            if (rule.hasNativeBorder()) {
                QStyleOptionFrame frmOpt(*frm);
                rule.configurePalette(&frmOpt.palette, QPalette::Text, QPalette::Base);
                frmOpt.rect = rule.borderRect(frmOpt.rect);
                if (rule.baseStyleCanDraw()) {
                    rule.drawBackgroundImage(p, opt->rect);
                    baseStyle()->drawPrimitive(pe, &frmOpt, p, w);
                } else {
                    rule.drawBackground(p, opt->rect);
                    if (frmOpt.lineWidth > 0)
                        baseStyle()->drawPrimitive(PE_FrameLineEdit, &frmOpt, p, w);
                }
            } else {
                rule.drawRule(p, opt->rect);
            }
        }
        return;

    case PE_Widget:
        if (w && !rule.hasDrawable()) {
            QWidget *container = containerWidget(w);
            if (styleSheetCaches->autoFillDisabledWidgets.contains(container)
                && (container == w || !renderRule(container, opt).hasBackground())) {
                //we do not have a background, but we disabled the autofillbackground anyway. so fill the background now.
                // (this may happen if we have rules like :focus)
                p->fillRect(opt->rect, opt->palette.brush(w->backgroundRole()));
            }
            break;
        }
#if QT_CONFIG(scrollarea)
        if (const QAbstractScrollArea *sa = qobject_cast<const QAbstractScrollArea *>(w)) {
            const QAbstractScrollAreaPrivate *sap = sa->d_func();
            rule.drawBackground(p, opt->rect, sap->contentsOffset());
            if (rule.hasBorder()) {
                QRect brect = rule.borderRect(opt->rect);
                if (styleHint(QStyle::SH_ScrollView_FrameOnlyAroundContents, opt, w)) {
                    QRect r = brect.adjusted(0, 0, sa->verticalScrollBar()->isVisible() ? -sa->verticalScrollBar()->width() : 0,
                                             sa->horizontalScrollBar()->isVisible() ? -sa->horizontalScrollBar()->height() : 0);
                    brect = QStyle::visualRect(opt->direction, brect, r);
                }
                rule.drawBorder(p, brect);
            }
            break;
        }
#endif
        Q_FALLTHROUGH();
    case PE_PanelMenu:
    case PE_PanelStatusBar:
        if (rule.hasDrawable()) {
            rule.drawRule(p, opt->rect);
            return;
        }
    break;

    case PE_FrameMenu:
        if (rule.hasDrawable()) {
            // Drawn by PE_PanelMenu
            return;
        }
        break;

    case PE_PanelMenuBar:
    if (rule.hasDrawable()) {
        // Drawn by PE_Widget
        return;
    }
    break;

    case PE_IndicatorToolBarSeparator:
    case PE_IndicatorToolBarHandle: {
        PseudoElement ps = pe == PE_IndicatorToolBarHandle ? PseudoElement_ToolBarHandle : PseudoElement_ToolBarSeparator;
        QRenderRule subRule = renderRule(w, opt, ps);
        if (subRule.hasDrawable()) {
            subRule.drawRule(p, opt->rect);
            return;
        }
                                    }
        break;

    case PE_IndicatorMenuCheckMark:
        pseudoElement = PseudoElement_MenuCheckMark;
        break;

    case PE_IndicatorArrowLeft:
        pseudoElement = PseudoElement_LeftArrow;
        break;

    case PE_IndicatorArrowRight:
        pseudoElement = PseudoElement_RightArrow;
        break;

    case PE_IndicatorColumnViewArrow:
#if QT_CONFIG(itemviews)
        if (const QStyleOptionViewItem *viewOpt = qstyleoption_cast<const QStyleOptionViewItem *>(opt)) {
            bool reverse = (viewOpt->direction == Qt::RightToLeft);
            pseudoElement = reverse ? PseudoElement_LeftArrow : PseudoElement_RightArrow;
        } else
#endif
        {
            pseudoElement = PseudoElement_RightArrow;
        }
        break;

#if QT_CONFIG(itemviews)
    case PE_IndicatorBranch:
        if (const QStyleOptionViewItem *vopt = qstyleoption_cast<const QStyleOptionViewItem *>(opt)) {
            QRenderRule subRule = renderRule(w, opt, PseudoElement_TreeViewBranch);
            if (subRule.hasDrawable()) {
                proxy()->drawPrimitive(PE_PanelItemViewRow, vopt, p, w);
                subRule.drawRule(p, opt->rect);
            } else {
                baseStyle()->drawPrimitive(pe, vopt, p, w);
            }
        }
        return;
#endif // QT_CONFIG(itemviews)

    case PE_PanelTipLabel:
        if (!rule.hasDrawable())
            break;

        if (const QStyleOptionFrame *frmOpt = qstyleoption_cast<const QStyleOptionFrame *>(opt)) {
            if (rule.hasNativeBorder()) {
                rule.drawBackground(p, opt->rect);
                QStyleOptionFrame optCopy(*frmOpt);
                optCopy.rect = rule.borderRect(opt->rect);
                optCopy.palette.setBrush(QPalette::Window, Qt::NoBrush); // oh dear
                baseStyle()->drawPrimitive(pe, &optCopy, p, w);
            } else {
                rule.drawRule(p, opt->rect);
            }
        }
        return;

    case PE_FrameGroupBox:
        if (rule.hasNativeBorder())
            break;
        rule.drawBorder(p, opt->rect);
        return;

#if QT_CONFIG(tabwidget)
    case PE_FrameTabWidget:
        if (const QStyleOptionTabWidgetFrame *frm = qstyleoption_cast<const QStyleOptionTabWidgetFrame *>(opt)) {
            QRenderRule subRule = renderRule(w, opt, PseudoElement_TabWidgetPane);
            if (subRule.hasNativeBorder()) {
                subRule.drawBackground(p, opt->rect);
                QStyleOptionTabWidgetFrame frmCopy(*frm);
                subRule.configurePalette(&frmCopy.palette, QPalette::WindowText, QPalette::Window);
                baseStyle()->drawPrimitive(pe, &frmCopy, p, w);
            } else {
                subRule.drawRule(p, opt->rect);
            }
            return;
        }
        break;
#endif // QT_CONFIG(tabwidget)

    case PE_IndicatorProgressChunk:
        pseudoElement = PseudoElement_ProgressBarChunk;
        break;

    case PE_IndicatorTabTear:
        pseudoElement = PseudoElement_TabBarTear;
        break;

    case PE_FrameFocusRect:
        if (!rule.hasNativeOutline()) {
            rule.drawOutline(p, opt->rect);
            return;
        }
        break;

    case PE_IndicatorDockWidgetResizeHandle:
        pseudoElement = PseudoElement_DockWidgetSeparator;
        break;

    case PE_PanelItemViewRow:
        // For compatibility reasons, QTreeView draws different parts of
        // the background of an item row separately, before calling the
        // delegate to draw the item. The row background of an item is
        // however not separately styleable through a style sheet, but
        // only indirectly through the background of the item. To get the
        // same background for all parts drawn by QTreeView, we have to
        // use the background rule for the item here.
        if (renderRule(w, opt, PseudoElement_ViewItem).hasBackground())
            pseudoElement = PseudoElement_ViewItem;
        break;
    case PE_PanelItemViewItem:
        pseudoElement = PseudoElement_ViewItem;
        break;

    case PE_PanelScrollAreaCorner:
        pseudoElement = PseudoElement_ScrollAreaCorner;
        break;

    case PE_IndicatorSpinDown:
    case PE_IndicatorSpinMinus:
        pseudoElement = PseudoElement_SpinBoxDownArrow;
        break;

    case PE_IndicatorSpinUp:
    case PE_IndicatorSpinPlus:
        pseudoElement = PseudoElement_SpinBoxUpArrow;
        break;
#if QT_CONFIG(tabbar)
    case PE_IndicatorTabClose:
        if (w) {
            // QMacStyle needs a real widget, not its parent - to implement
            // 'document mode' properly, drawing nothing if a tab is not hovered.
            baseStyle()->setProperty("_q_styleSheetRealCloseButton", QVariant::fromValue((void *)w));
            w = w->parentWidget(); //match on the QTabBar instead of the CloseButton
        }
        pseudoElement = PseudoElement_TabBarTabCloseButton;
#endif

    default:
        break;
    }

    if (pseudoElement != PseudoElement_None) {
        QRenderRule subRule = renderRule(w, opt, pseudoElement);
        if (subRule.hasDrawable()) {
            subRule.drawRule(p, rect);
        } else {
            baseStyle()->drawPrimitive(pe, opt, p, w);
        }
    } else {
        baseStyle()->drawPrimitive(pe, opt, p, w);
    }

    if (baseStyle()->property("_q_styleSheetRealCloseButton").toBool())
        baseStyle()->setProperty("_q_styleSheetRealCloseButton", QVariant());
}

QPixmap QStyleSheetStyle::generatedIconPixmap(QIcon::Mode iconMode, const QPixmap& pixmap,
                                          const QStyleOption *option) const
{
    return baseStyle()->generatedIconPixmap(iconMode, pixmap, option);
}

QStyle::SubControl QStyleSheetStyle::hitTestComplexControl(ComplexControl cc, const QStyleOptionComplex *opt,
                                 const QPoint &pt, const QWidget *w) const
{
    RECURSION_GUARD(return baseStyle()->hitTestComplexControl(cc, opt, pt, w))
    switch (cc) {
    case CC_TitleBar:
        if (const QStyleOptionTitleBar *tb = qstyleoption_cast<const QStyleOptionTitleBar *>(opt)) {
            QRenderRule rule = renderRule(w, opt, PseudoElement_TitleBar);
            if (rule.hasDrawable() || rule.hasBox() || rule.hasBorder()) {
                QHash<QStyle::SubControl, QRect> layout = titleBarLayout(w, tb);
                QRect r;
                QStyle::SubControl sc = QStyle::SC_None;
                uint ctrl = SC_TitleBarSysMenu;
                while (ctrl <= SC_TitleBarLabel) {
                    r = layout[QStyle::SubControl(ctrl)];
                    if (r.isValid() && r.contains(pt)) {
                        sc = QStyle::SubControl(ctrl);
                        break;
                    }
                    ctrl <<= 1;
                }
                return sc;
            }
        }
        break;

    case CC_MdiControls:
        if (hasStyleRule(w, PseudoElement_MdiCloseButton)
            || hasStyleRule(w, PseudoElement_MdiNormalButton)
            || hasStyleRule(w, PseudoElement_MdiMinButton))
            return QWindowsStyle::hitTestComplexControl(cc, opt, pt, w);
        break;

    case CC_ScrollBar: {
        QRenderRule rule = renderRule(w, opt);
        if (!rule.hasDrawable() && !rule.hasBox())
            break;
                       }
        Q_FALLTHROUGH();
    case CC_SpinBox:
    case CC_GroupBox:
    case CC_ComboBox:
    case CC_Slider:
    case CC_ToolButton:
        return QWindowsStyle::hitTestComplexControl(cc, opt, pt, w);
    default:
        break;
    }

    return baseStyle()->hitTestComplexControl(cc, opt, pt, w);
}

QRect QStyleSheetStyle::itemPixmapRect(const QRect &rect, int alignment, const QPixmap &pixmap) const
{
    return baseStyle()->itemPixmapRect(rect, alignment, pixmap);
}

QRect QStyleSheetStyle::itemTextRect(const QFontMetrics &metrics, const QRect& rect, int alignment,
                                 bool enabled, const QString& text) const
{
    return baseStyle()->itemTextRect(metrics, rect, alignment, enabled, text);
}

int QStyleSheetStyle::pixelMetric(PixelMetric m, const QStyleOption *opt, const QWidget *w) const
{
    RECURSION_GUARD(return baseStyle()->pixelMetric(m, opt, w))

    QRenderRule rule = renderRule(w, opt);
    QRenderRule subRule;

    switch (m) {
    case PM_MenuButtonIndicator:
#if QT_CONFIG(toolbutton)
        // QToolButton adds this directly to the width
        if (qobject_cast<const QToolButton *>(w)) {
            if (rule.hasBox() || !rule.hasNativeBorder())
                return 0;
            if (const auto *tbOpt = qstyleoption_cast<const QStyleOptionToolButton*>(opt)) {
                if (tbOpt->features & QStyleOptionToolButton::MenuButtonPopup)
                    subRule = renderRule(w, opt, PseudoElement_ToolButtonMenu);
                else
                    subRule = renderRule(w, opt, PseudoElement_ToolButtonMenuIndicator);
                if (subRule.hasContentsSize())
                    return subRule.size().width();
            }
            break;
        }
#endif
        subRule = renderRule(w, opt, PseudoElement_PushButtonMenuIndicator);
        if (subRule.hasContentsSize())
            return subRule.size().width();
        break;

    case PM_ButtonShiftHorizontal:
    case PM_ButtonShiftVertical:
    case PM_ButtonMargin:
    case PM_ButtonDefaultIndicator:
        if (rule.hasBox())
            return 0;
        break;

    case PM_DefaultFrameWidth:
        if (!rule.hasNativeBorder())
            return rule.border()->borders[LeftEdge];
        break;

    case PM_ExclusiveIndicatorWidth:
    case PM_IndicatorWidth:
    case PM_ExclusiveIndicatorHeight:
    case PM_IndicatorHeight:
        subRule = renderRule(w, opt, PseudoElement_Indicator);
        if (subRule.hasContentsSize()) {
            return (m == PM_ExclusiveIndicatorWidth) || (m == PM_IndicatorWidth)
                        ? subRule.size().width() : subRule.size().height();
        }
        break;

    case PM_DockWidgetFrameWidth:
    case PM_ToolTipLabelFrameWidth: // border + margin + padding (support only one width)
        if (!rule.hasDrawable())
            break;

        return (rule.border() ? rule.border()->borders[LeftEdge] : 0)
                + (rule.hasBox() ? rule.box()->margins[LeftEdge] + rule.box()->paddings[LeftEdge]: 0);

    case PM_ToolBarFrameWidth:
        if (rule.hasBorder() || rule.hasBox())
            return (rule.border() ? rule.border()->borders[LeftEdge] : 0)
                   + (rule.hasBox() ? rule.box()->paddings[LeftEdge]: 0);
        break;

    case PM_MenuPanelWidth:
    case PM_MenuBarPanelWidth:
        if (rule.hasBorder() || rule.hasBox())
            return (rule.border() ? rule.border()->borders[LeftEdge] : 0)
                   + (rule.hasBox() ? rule.box()->margins[LeftEdge]: 0);
        break;


    case PM_MenuHMargin:
    case PM_MenuBarHMargin:
        if (rule.hasBox())
            return rule.box()->paddings[LeftEdge];
        break;

    case PM_MenuVMargin:
    case PM_MenuBarVMargin:
        if (rule.hasBox())
            return rule.box()->paddings[TopEdge];
        break;

    case PM_DockWidgetTitleBarButtonMargin:
    case PM_ToolBarItemMargin:
        if (rule.hasBox())
            return rule.box()->margins[TopEdge];
        break;

    case PM_ToolBarItemSpacing:
    case PM_MenuBarItemSpacing:
        if (rule.hasBox() && rule.box()->spacing != -1)
            return rule.box()->spacing;
        break;

    case PM_MenuTearoffHeight:
    case PM_MenuScrollerHeight: {
        PseudoElement ps = m == PM_MenuTearoffHeight ? PseudoElement_MenuTearoff : PseudoElement_MenuScroller;
        subRule = renderRule(w, opt, ps);
        if (subRule.hasContentsSize())
            return subRule.size().height();
        break;
                                }

    case PM_ToolBarExtensionExtent:
        break;

    case PM_SplitterWidth:
    case PM_ToolBarSeparatorExtent:
    case PM_ToolBarHandleExtent: {
        PseudoElement ps;
        if (m == PM_ToolBarHandleExtent) ps = PseudoElement_ToolBarHandle;
        else if (m == PM_SplitterWidth) ps = PseudoElement_SplitterHandle;
        else ps = PseudoElement_ToolBarSeparator;
        subRule = renderRule(w, opt, ps);
        if (subRule.hasContentsSize()) {
            QSize sz = subRule.size();
            return (opt && opt->state & QStyle::State_Horizontal) ? sz.width() : sz.height();
        }
        break;
                                 }

    case PM_RadioButtonLabelSpacing:
        if (rule.hasBox() && rule.box()->spacing != -1)
            return rule.box()->spacing;
        break;
    case PM_CheckBoxLabelSpacing:
#if QT_CONFIG(checkbox)
        if (qobject_cast<const QCheckBox *>(w)) {
            if (rule.hasBox() && rule.box()->spacing != -1)
                return rule.box()->spacing;
        }
#endif
        // assume group box
        subRule = renderRule(w, opt, PseudoElement_GroupBoxTitle);
        if (subRule.hasBox() && subRule.box()->spacing != -1)
            return subRule.box()->spacing;
        break;

#if QT_CONFIG(scrollbar)
    case PM_ScrollBarExtent:
        if (rule.hasContentsSize()) {
            QSize sz = rule.size();
            if (const QStyleOptionSlider *sb = qstyleoption_cast<const QStyleOptionSlider *>(opt))
                return sb->orientation == Qt::Horizontal ? sz.height() : sz.width();
            return sz.width() == -1 ? sz.height() : sz.width();
        }
        break;

    case PM_ScrollBarSliderMin:
        if (hasStyleRule(w, PseudoElement_ScrollBarSlider)) {
            subRule = renderRule(w, opt, PseudoElement_ScrollBarSlider);
            QSize msz = subRule.minimumSize();
            if (const QStyleOptionSlider *sb = qstyleoption_cast<const QStyleOptionSlider *>(opt))
                return sb->orientation == Qt::Horizontal ? msz.width() : msz.height();
            return msz.width() == -1 ? msz.height() : msz.width();
        }
        break;

    case PM_ScrollView_ScrollBarSpacing:
        if (!rule.hasNativeBorder() || rule.hasBox())
            return 0;
        break;

    case PM_ScrollView_ScrollBarOverlap:
        if (!proxy()->styleHint(SH_ScrollBar_Transient, opt, w))
            return 0;
        break;
#endif // QT_CONFIG(scrollbar)


    case PM_ProgressBarChunkWidth:
        subRule = renderRule(w, opt, PseudoElement_ProgressBarChunk);
        if (subRule.hasContentsSize()) {
            QSize sz = subRule.size();
            return (opt->state & QStyle::State_Horizontal)
                   ? sz.width() : sz.height();
        }
        break;

#if QT_CONFIG(tabwidget)
    case PM_TabBarTabHSpace:
    case PM_TabBarTabVSpace:
        subRule = renderRule(w, opt, PseudoElement_TabBarTab);
        if (subRule.hasBox() || subRule.hasBorder())
            return 0;
        break;

    case PM_TabBarScrollButtonWidth:
        subRule = renderRule(w, opt, PseudoElement_TabBarScroller);
        if (subRule.hasContentsSize()) {
            QSize sz = subRule.size();
            return (sz.width() != -1 ? sz.width() : sz.height()) / 2;
        }
        break;

    case PM_TabBarTabShiftHorizontal:
    case PM_TabBarTabShiftVertical:
        subRule = renderRule(w, opt, PseudoElement_TabBarTab);
        if (subRule.hasBox())
            return 0;
        break;

    case PM_TabBarBaseOverlap: {
        const QWidget *tabWidget = qobject_cast<const QTabWidget *>(w);
        if (!tabWidget && w)
            tabWidget = w->parentWidget();
        if (hasStyleRule(tabWidget, PseudoElement_TabWidgetPane)) {
            return 0;
        }
        break;
    }
#endif // QT_CONFIG(tabwidget)

    case PM_SliderThickness: // horizontal slider's height (sizeHint)
    case PM_SliderLength: // minimum length of slider
        if (rule.hasContentsSize()) {
            bool horizontal = opt->state & QStyle::State_Horizontal;
            if (m == PM_SliderThickness) {
                QSize sz = rule.size();
                return horizontal ? sz.height() : sz.width();
            } else {
                QSize msz = rule.minimumContentsSize();
                return horizontal ? msz.width() : msz.height();
            }
        }
        break;

    case PM_SliderControlThickness: {
        QRenderRule subRule = renderRule(w, opt, PseudoElement_SliderHandle);
        if (!subRule.hasContentsSize())
            break;
        QSize size = subRule.size();
        return (opt->state & QStyle::State_Horizontal) ? size.height() : size.width();
                                    }

    case PM_ToolBarIconSize:
    case PM_ListViewIconSize:
    case PM_IconViewIconSize:
    case PM_TabBarIconSize:
    case PM_MessageBoxIconSize:
    case PM_ButtonIconSize:
    case PM_SmallIconSize:
        if (rule.hasStyleHint("icon-size"_L1))
            return rule.styleHint("icon-size"_L1).toSize().width();
        break;

    case PM_DockWidgetTitleMargin: {
        QRenderRule subRule = renderRule(w, opt, PseudoElement_DockWidgetTitle);
        if (!subRule.hasBox())
            break;
        return (subRule.border() ? subRule.border()->borders[TopEdge] : 0)
                + (subRule.hasBox() ? subRule.box()->margins[TopEdge] + subRule.box()->paddings[TopEdge]: 0);
                                   }

    case PM_DockWidgetSeparatorExtent: {
        QRenderRule subRule = renderRule(w, opt, PseudoElement_DockWidgetSeparator);
        if (!subRule.hasContentsSize())
            break;
        QSize sz = subRule.size();
        return qMax(sz.width(), sz.height());
                                        }

    case PM_TitleBarHeight: {
        QRenderRule subRule = renderRule(w, opt, PseudoElement_TitleBar);
        if (subRule.hasContentsSize())
            return subRule.size().height();
        else if (subRule.hasBox() || subRule.hasBorder()) {
            QFontMetrics fm = opt ?  opt->fontMetrics : w->fontMetrics();
            return subRule.size(QSize(0, fm.height())).height();
        }
        break;
                            }

    case PM_MdiSubWindowFrameWidth:
        if (rule.hasBox() || rule.hasBorder()) {
            return (rule.border() ? rule.border()->borders[LeftEdge] : 0)
                   + (rule.hasBox() ? rule.box()->paddings[LeftEdge]+rule.box()->margins[LeftEdge]: 0);
        }
        break;

    case PM_MdiSubWindowMinimizedWidth: {
        QRenderRule subRule = renderRule(w, PseudoElement_None, PseudoClass_Minimized);
        int width = subRule.size().width();
        if (width != -1)
            return width;
        break;
                                     }
    default:
        break;
    }

    return baseStyle()->pixelMetric(m, opt, w);
}

QSize QStyleSheetStyle::sizeFromContents(ContentsType ct, const QStyleOption *opt,
                                         const QSize &csz, const QWidget *w) const
{
    RECURSION_GUARD(return baseStyle()->sizeFromContents(ct, opt, csz, w))

    QRenderRule rule = renderRule(w, opt);
    QSize sz = rule.adjustSize(csz);

    switch (ct) {
#if QT_CONFIG(spinbox)
    case CT_SpinBox:
        if (const QStyleOptionSpinBox *spinbox = qstyleoption_cast<const QStyleOptionSpinBox *>(opt)) {
            if (spinbox->buttonSymbols != QAbstractSpinBox::NoButtons) {
                // Add some space for the up/down buttons
                QRenderRule subRule = renderRule(w, opt, PseudoElement_SpinBoxUpButton);
                if (subRule.hasDrawable()) {
                    QRect r = positionRect(w, rule, subRule, PseudoElement_SpinBoxUpButton,
                                           opt->rect, opt->direction);
                    sz.rwidth() += r.width();
                } else {
                    QSize defaultUpSize = defaultSize(w, subRule.size(), spinbox->rect, PseudoElement_SpinBoxUpButton);
                    sz.rwidth() += defaultUpSize.width();
                }
            }
            if (rule.hasBox() || rule.hasBorder() || !rule.hasNativeBorder())
                sz = rule.boxSize(sz);
            return sz;
        }
        break;
#endif // QT_CONFIG(spinbox)
    case CT_ToolButton:
        if (rule.hasBox() || !rule.hasNativeBorder() || !rule.baseStyleCanDraw())
            sz += QSize(3, 3); // ### broken QToolButton
        Q_FALLTHROUGH();
    case CT_ComboBox:
    case CT_PushButton:
        if (rule.hasBox() || !rule.hasNativeBorder()) {
            if (ct == CT_ComboBox) {
                //add some space for the drop down.
                QRenderRule subRule = renderRule(w, opt, PseudoElement_ComboBoxDropDown);
                QRect comboRect = positionRect(w, rule, subRule, PseudoElement_ComboBoxDropDown, opt->rect, opt->direction);
                //+2 because there is hardcoded margins in QCommonStyle::drawControl(CE_ComboBoxLabel)
                sz += QSize(comboRect.width() + 2, 0);
            }
            return rule.boxSize(sz);
        }
        sz = rule.baseStyleCanDraw() ? baseStyle()->sizeFromContents(ct, opt, sz, w)
                                     : QWindowsStyle::sizeFromContents(ct, opt, sz, w);
        return rule.boxSize(sz, Margin);

    case CT_HeaderSection: {
            if (const QStyleOptionHeader *hdr = qstyleoption_cast<const QStyleOptionHeader *>(opt)) {
                QRenderRule subRule = renderRule(w, opt, PseudoElement_HeaderViewSection);
                if (subRule.hasGeometry() || subRule.hasBox() || !subRule.hasNativeBorder() || subRule.hasFont) {
                    sz = subRule.adjustSize(csz);
                    if (!sz.isValid()) {
                        // Try to set the missing values based on the base style.
                        const auto baseSize = baseStyle()->sizeFromContents(ct, opt, sz, w);
                        if (sz.width() < 0)
                            sz.setWidth(baseSize.width());
                        if (sz.height() < 0)
                            sz.setHeight(baseSize.height());
                    }
                    if (!subRule.hasGeometry()) {
                        QSize nativeContentsSize;
                        bool nullIcon = hdr->icon.isNull();
                        const int margin = pixelMetric(QStyle::PM_HeaderMargin, hdr, w);
                        int iconSize = nullIcon ? 0 : pixelMetric(QStyle::PM_SmallIconSize, hdr, w);
                        QFontMetrics fm = hdr->fontMetrics;
                        if (subRule.hasFont) {
                            QFont styleFont = w ? subRule.font.resolve(w->font()) : subRule.font;
                            fm = QFontMetrics(styleFont);
                        }
                        const QSize txt = fm.size(0, hdr->text);
                        nativeContentsSize.setHeight(margin + qMax(iconSize, txt.height()) + margin);
                        nativeContentsSize.setWidth((nullIcon ? 0 : margin) + iconSize
                                                    + (hdr->text.isNull() ? 0 : margin) + txt.width() + margin);
                        sz = sz.expandedTo(nativeContentsSize);
                    }
                    return subRule.size(sz);
                }
                sz = subRule.baseStyleCanDraw() ? baseStyle()->sizeFromContents(ct, opt, sz, w)
                                                : QWindowsStyle::sizeFromContents(ct, opt, sz, w);
                if (hasStyleRule(w, PseudoElement_HeaderViewDownArrow)
                 || hasStyleRule(w, PseudoElement_HeaderViewUpArrow)) {
                    const QRect arrowRect = subElementRect(SE_HeaderArrow, opt, w);
                    if (hdr->orientation == Qt::Horizontal)
                        sz.rwidth() += arrowRect.width();
                    else
                        sz.rheight() += arrowRect.height();
                }
                return sz;
            }
        }
        break;
    case CT_GroupBox:
    case CT_LineEdit:
#if QT_CONFIG(spinbox)
        if (qobject_cast<QAbstractSpinBox *>(w ? w->parentWidget() : nullptr))
            return csz; // we only care about the size hint of the line edit
#endif
        if (rule.hasBox() || !rule.hasNativeBorder()) {
            return rule.boxSize(sz);
        }
        break;

    case CT_CheckBox:
    case CT_RadioButton:
        if (const QStyleOptionButton *btn = qstyleoption_cast<const QStyleOptionButton *>(opt)) {
            if (rule.hasBox() || rule.hasBorder() || hasStyleRule(w, PseudoElement_Indicator)) {
                bool isRadio = (ct == CT_RadioButton);
                int iw = pixelMetric(isRadio ? PM_ExclusiveIndicatorWidth
                                             : PM_IndicatorWidth, btn, w);
                int ih = pixelMetric(isRadio ? PM_ExclusiveIndicatorHeight
                                             : PM_IndicatorHeight, btn, w);

                int spacing = pixelMetric(isRadio ? PM_RadioButtonLabelSpacing
                                                  : PM_CheckBoxLabelSpacing, btn, w);
                sz.setWidth(sz.width() + iw + spacing);
                sz.setHeight(qMax(sz.height(), ih));
                return rule.boxSize(sz);
            }
        }
        break;

    case CT_Menu:
    case CT_MenuBar: // already has everything!
    case CT_ScrollBar:
        if (rule.hasBox() || rule.hasBorder())
            return sz;
        break;

    case CT_MenuItem:
        if (const QStyleOptionMenuItem *mi = qstyleoption_cast<const QStyleOptionMenuItem *>(opt)) {
            PseudoElement pe = (mi->menuItemType == QStyleOptionMenuItem::Separator)
                                    ? PseudoElement_MenuSeparator : PseudoElement_Item;
            QRenderRule subRule = renderRule(w, opt, pe);
            if ((pe == PseudoElement_MenuSeparator) && subRule.hasContentsSize()) {
                return QSize(sz.width(), subRule.size().height());
            }
            if ((pe == PseudoElement_Item) && (subRule.hasBox() || subRule.hasBorder() || subRule.hasFont)) {
                QSize sz(csz);
                if (mi->text.contains(u'\t'))
                    sz.rwidth() += 12; //as in QCommonStyle
                if (!mi->icon.isNull()) {
                    const int pmSmall = pixelMetric(PM_SmallIconSize);
                    const QSize pmSize = mi->icon.actualSize(QSize(pmSmall, pmSmall));
                    sz.rwidth() += std::max(mi->maxIconWidth, pmSize.width()) + 4;
                } else if (mi->menuHasCheckableItems) {
                    QRenderRule subSubRule = renderRule(w, opt, PseudoElement_MenuCheckMark);
                    QRect checkmarkRect = positionRect(w, subRule, subSubRule, PseudoElement_MenuCheckMark, opt->rect, opt->direction);
                    sz.rwidth() += std::max(mi->maxIconWidth, checkmarkRect.width()) + 4;
                } else {
                    sz.rwidth() += mi->maxIconWidth;
                }
                if (subRule.hasFont) {
                    QFontMetrics fm(subRule.font.resolve(mi->font));
                    const QRect r = fm.boundingRect(QRect(), Qt::TextSingleLine | Qt::TextShowMnemonic, mi->text);
                    sz = sz.expandedTo(r.size());
                }
                return subRule.boxSize(subRule.adjustSize(sz));
            }
        }
        break;

    case CT_Splitter:
    case CT_MenuBarItem: {
        PseudoElement pe = (ct == CT_Splitter) ? PseudoElement_SplitterHandle : PseudoElement_Item;
        QRenderRule subRule = renderRule(w, opt, pe);
        if (subRule.hasBox() || subRule.hasBorder())
            return subRule.boxSize(sz);
        break;
                        }

    case CT_ProgressBar:
    case CT_SizeGrip:
        return (rule.hasContentsSize())
            ? rule.size(sz)
            : rule.boxSize(baseStyle()->sizeFromContents(ct, opt, sz, w));
        break;

    case CT_Slider:
        if (rule.hasBorder() || rule.hasBox() || rule.hasGeometry())
            return rule.boxSize(sz);
        break;

#if QT_CONFIG(tabbar)
    case CT_TabBarTab: {
        QRenderRule subRule = renderRule(w, opt, PseudoElement_TabBarTab);
        if (subRule.hasBox() || !subRule.hasNativeBorder() || subRule.hasFont) {
            int spaceForIcon = 0;
            bool vertical = false;
            QString text;
            if (const QStyleOptionTab *tab = qstyleoption_cast<const QStyleOptionTab *>(opt)) {
                if (!tab->icon.isNull())
                    spaceForIcon = 6 /* icon offset */ + 4 /* spacing */ + 2 /* magic */; // ###: hardcoded to match with common style
                vertical = verticalTabs(tab->shape);
                text = tab->text;
            }
            if (subRule.hasBox() || !subRule.hasNativeBorder())
                sz = csz + QSize(vertical ? 0 : spaceForIcon, vertical ? spaceForIcon : 0);
            if (subRule.hasFont) {
                // first we remove the space needed for the text using the default font
                const QSize oldTextSize = opt->fontMetrics.size(Qt::TextShowMnemonic, text);
                (vertical ? sz.rheight() : sz.rwidth()) -= oldTextSize.width();

                // then we add the space needed when using the rule font to the relevant
                // dimension, and constraint the other dimension to the maximum to make
                // sure we don't grow, but also don't clip icons or buttons.
                const QFont ruleFont = subRule.font.resolve(w->font());
                const QFontMetrics fm(ruleFont);
                const QSize textSize = fm.size(Qt::TextShowMnemonic, text);
                if (vertical) {
                    sz.rheight() += textSize.width();
                    sz.rwidth() = qMax(textSize.height(), sz.width());
                } else {
                    sz.rwidth() += textSize.width();
                    sz.rheight() = qMax(textSize.height(), sz.height());
                }
            }

            return subRule.boxSize(subRule.adjustSize(sz));
        }
        sz = subRule.adjustSize(csz);
        break;
    }
#endif // QT_CONFIG(tabbar)

    case CT_MdiControls:
        if (const QStyleOptionComplex *ccOpt = qstyleoption_cast<const QStyleOptionComplex *>(opt)) {
            if (!hasStyleRule(w, PseudoElement_MdiCloseButton)
                && !hasStyleRule(w, PseudoElement_MdiNormalButton)
                && !hasStyleRule(w, PseudoElement_MdiMinButton))
                break;

            QList<QVariant> layout = rule.styleHint("button-layout"_L1).toList();
            if (layout.isEmpty())
                layout = subControlLayout("mNX"_L1);

            int width = 0, height = 0;
            for (int i = 0; i < layout.size(); i++) {
                int layoutButton = layout[i].toInt();
                if (layoutButton < PseudoElement_MdiCloseButton
                    || layoutButton > PseudoElement_MdiNormalButton)
                    continue;
                QStyle::SubControl sc = knownPseudoElements[layoutButton].subControl;
                if (!(ccOpt->subControls & sc))
                    continue;
                QRenderRule subRule = renderRule(w, opt, layoutButton);
                QSize sz = subRule.size();
                width += sz.width();
                height = qMax(height, sz.height());
            }

            return QSize(width, height);
        }
        break;

#if QT_CONFIG(itemviews)
    case CT_ItemViewItem: {
        QRenderRule subRule = renderRule(w, opt, PseudoElement_ViewItem);
        sz = baseStyle()->sizeFromContents(ct, opt, csz, w);
        sz = subRule.adjustSize(sz);
        if (subRule.hasBox() || subRule.hasBorder())
            sz = subRule.boxSize(sz);
        return sz;
                      }
#endif // QT_CONFIG(itemviews)

    default:
        break;
    }

    return baseStyle()->sizeFromContents(ct, opt, sz, w);
}

/*!
    \internal
*/
static QLatin1StringView propertyNameForStandardPixmap(QStyle::StandardPixmap sp)
{
    switch (sp) {
        case QStyle::SP_TitleBarMenuButton: return "titlebar-menu-icon"_L1;
        case QStyle::SP_TitleBarMinButton: return "titlebar-minimize-icon"_L1;
        case QStyle::SP_TitleBarMaxButton: return "titlebar-maximize-icon"_L1;
        case QStyle::SP_TitleBarCloseButton: return "titlebar-close-icon"_L1;
        case QStyle::SP_TitleBarNormalButton: return "titlebar-normal-icon"_L1;
        case QStyle::SP_TitleBarShadeButton: return "titlebar-shade-icon"_L1;
        case QStyle::SP_TitleBarUnshadeButton: return "titlebar-unshade-icon"_L1;
        case QStyle::SP_TitleBarContextHelpButton: return "titlebar-contexthelp-icon"_L1;
        case QStyle::SP_DockWidgetCloseButton: return "dockwidget-close-icon"_L1;
        case QStyle::SP_MessageBoxInformation: return "messagebox-information-icon"_L1;
        case QStyle::SP_MessageBoxWarning: return "messagebox-warning-icon"_L1;
        case QStyle::SP_MessageBoxCritical: return "messagebox-critical-icon"_L1;
        case QStyle::SP_MessageBoxQuestion: return "messagebox-question-icon"_L1;
        case QStyle::SP_DesktopIcon: return "desktop-icon"_L1;
        case QStyle::SP_TrashIcon: return "trash-icon"_L1;
        case QStyle::SP_ComputerIcon: return "computer-icon"_L1;
        case QStyle::SP_DriveFDIcon: return "floppy-icon"_L1;
        case QStyle::SP_DriveHDIcon: return "harddisk-icon"_L1;
        case QStyle::SP_DriveCDIcon: return "cd-icon"_L1;
        case QStyle::SP_DriveDVDIcon: return "dvd-icon"_L1;
        case QStyle::SP_DriveNetIcon: return "network-icon"_L1;
        case QStyle::SP_DirOpenIcon: return "directory-open-icon"_L1;
        case QStyle::SP_DirClosedIcon: return "directory-closed-icon"_L1;
        case QStyle::SP_DirLinkIcon: return "directory-link-icon"_L1;
        case QStyle::SP_FileIcon: return "file-icon"_L1;
        case QStyle::SP_FileLinkIcon: return "file-link-icon"_L1;
        case QStyle::SP_FileDialogStart: return "filedialog-start-icon"_L1;
        case QStyle::SP_FileDialogEnd: return "filedialog-end-icon"_L1;
        case QStyle::SP_FileDialogToParent: return "filedialog-parent-directory-icon"_L1;
        case QStyle::SP_FileDialogNewFolder: return "filedialog-new-directory-icon"_L1;
        case QStyle::SP_FileDialogDetailedView: return "filedialog-detailedview-icon"_L1;
        case QStyle::SP_FileDialogInfoView: return "filedialog-infoview-icon"_L1;
        case QStyle::SP_FileDialogContentsView: return "filedialog-contentsview-icon"_L1;
        case QStyle::SP_FileDialogListView: return "filedialog-listview-icon"_L1;
        case QStyle::SP_FileDialogBack: return "filedialog-backward-icon"_L1;
        case QStyle::SP_DirIcon: return "directory-icon"_L1;
        case QStyle::SP_DialogOkButton: return "dialog-ok-icon"_L1;
        case QStyle::SP_DialogCancelButton: return "dialog-cancel-icon"_L1;
        case QStyle::SP_DialogHelpButton: return "dialog-help-icon"_L1;
        case QStyle::SP_DialogOpenButton: return "dialog-open-icon"_L1;
        case QStyle::SP_DialogSaveButton: return "dialog-save-icon"_L1;
        case QStyle::SP_DialogCloseButton: return "dialog-close-icon"_L1;
        case QStyle::SP_DialogApplyButton: return "dialog-apply-icon"_L1;
        case QStyle::SP_DialogResetButton: return "dialog-reset-icon"_L1;
        case QStyle::SP_DialogDiscardButton: return "dialog-discard-icon"_L1;
        case QStyle::SP_DialogYesButton: return "dialog-yes-icon"_L1;
        case QStyle::SP_DialogNoButton: return "dialog-no-icon"_L1;
        case QStyle::SP_ArrowUp: return "uparrow-icon"_L1;
        case QStyle::SP_ArrowDown: return "downarrow-icon"_L1;
        case QStyle::SP_ArrowLeft: return "leftarrow-icon"_L1;
        case QStyle::SP_ArrowRight: return "rightarrow-icon"_L1;
        case QStyle::SP_ArrowBack: return "backward-icon"_L1;
        case QStyle::SP_ArrowForward: return "forward-icon"_L1;
        case QStyle::SP_DirHomeIcon: return "home-icon"_L1;
        case QStyle::SP_LineEditClearButton: return "lineedit-clear-button-icon"_L1;
        default: return ""_L1;
    }
}

QIcon QStyleSheetStyle::standardIcon(StandardPixmap standardIcon, const QStyleOption *opt,
                                     const QWidget *w) const
{
    RECURSION_GUARD(return baseStyle()->standardIcon(standardIcon, opt, w))
    QString s = propertyNameForStandardPixmap(standardIcon);
    if (!s.isEmpty()) {
        QRenderRule rule = renderRule(w, opt);
        if (rule.hasStyleHint(s))
            return qvariant_cast<QIcon>(rule.styleHint(s));
    }
    return baseStyle()->standardIcon(standardIcon, opt, w);
}

QPalette QStyleSheetStyle::standardPalette() const
{
    return baseStyle()->standardPalette();
}

QPixmap QStyleSheetStyle::standardPixmap(StandardPixmap standardPixmap, const QStyleOption *opt,
                                         const QWidget *w) const
{
    RECURSION_GUARD(return baseStyle()->standardPixmap(standardPixmap, opt, w))
    QString s = propertyNameForStandardPixmap(standardPixmap);
    if (!s.isEmpty()) {
        QRenderRule rule = renderRule(w, opt);
        if (rule.hasStyleHint(s)) {
            QIcon icon = qvariant_cast<QIcon>(rule.styleHint(s));
            return icon.pixmap(16, 16); // ###: unhard-code this if someone complains
        }
    }
    return baseStyle()->standardPixmap(standardPixmap, opt, w);
}

int QStyleSheetStyle::layoutSpacing(QSizePolicy::ControlType control1, QSizePolicy::ControlType control2,
                          Qt::Orientation orientation, const QStyleOption *option,
                          const QWidget *widget) const
{
    return baseStyle()->layoutSpacing(control1, control2, orientation, option, widget);
}

int QStyleSheetStyle::styleHint(StyleHint sh, const QStyleOption *opt, const QWidget *w,
                           QStyleHintReturn *shret) const
{
    RECURSION_GUARD(return baseStyle()->styleHint(sh, opt, w, shret))
    // Prevent endless loop if somebody use isActiveWindow property as selector.
    // QWidget::isActiveWindow uses this styleHint to determine if the window is active or not
    if (sh == SH_Widget_ShareActivation)
        return baseStyle()->styleHint(sh, opt, w, shret);

    QRenderRule rule = renderRule(w, opt);
    QString s;
    switch (sh) {
        case SH_LineEdit_PasswordCharacter: s = "lineedit-password-character"_L1; break;
        case SH_LineEdit_PasswordMaskDelay: s = "lineedit-password-mask-delay"_L1; break;
        case SH_DitherDisabledText: s = "dither-disabled-text"_L1; break;
        case SH_EtchDisabledText: s = "etch-disabled-text"_L1; break;
        case SH_ItemView_ActivateItemOnSingleClick: s = "activate-on-singleclick"_L1; break;
        case SH_ItemView_ShowDecorationSelected: s = "show-decoration-selected"_L1; break;
        case SH_Table_GridLineColor: s = "gridline-color"_L1; break;
        case SH_DialogButtonLayout: s = "button-layout"_L1; break;
        case SH_ToolTipLabel_Opacity: s = "opacity"_L1; break;
        case SH_ComboBox_Popup: s = "combobox-popup"_L1; break;
        case SH_ComboBox_ListMouseTracking: s = "combobox-list-mousetracking"_L1; break;
        case SH_MenuBar_AltKeyNavigation: s = "menubar-altkey-navigation"_L1; break;
        case SH_Menu_Scrollable: s = "menu-scrollable"_L1; break;
        case SH_DrawMenuBarSeparator: s = "menubar-separator"_L1; break;
        case SH_MenuBar_MouseTracking: s = "mouse-tracking"_L1; break;
        case SH_SpinBox_ClickAutoRepeatRate: s = "spinbox-click-autorepeat-rate"_L1; break;
        case SH_SpinControls_DisableOnBounds: s = "spincontrol-disable-on-bounds"_L1; break;
        case SH_MessageBox_TextInteractionFlags: s = "messagebox-text-interaction-flags"_L1; break;
        case SH_ToolButton_PopupDelay: s = "toolbutton-popup-delay"_L1; break;
        case SH_ToolBox_SelectedPageTitleBold:
            if (renderRule(w, opt, PseudoElement_ToolBoxTab).hasFont)
                return 0;
            break;
        case SH_GroupBox_TextLabelColor:
            if (rule.hasPalette() && rule.palette()->foreground.style() != Qt::NoBrush)
                return rule.palette()->foreground.color().rgba();
            break;
        case SH_ScrollView_FrameOnlyAroundContents: s = "scrollview-frame-around-contents"_L1; break;
        case SH_ScrollBar_ContextMenu: s = "scrollbar-contextmenu"_L1; break;
        case SH_ScrollBar_LeftClickAbsolutePosition: s = "scrollbar-leftclick-absolute-position"_L1; break;
        case SH_ScrollBar_MiddleClickAbsolutePosition: s = "scrollbar-middleclick-absolute-position"_L1; break;
        case SH_ScrollBar_RollBetweenButtons: s = "scrollbar-roll-between-buttons"_L1; break;
        case SH_ScrollBar_ScrollWhenPointerLeavesControl: s = "scrollbar-scroll-when-pointer-leaves-control"_L1; break;
        case SH_TabBar_Alignment:
#if QT_CONFIG(tabwidget)
            if (qobject_cast<const QTabWidget *>(w)) {
                rule = renderRule(w, opt, PseudoElement_TabWidgetTabBar);
                if (rule.hasPosition())
                    return rule.position()->position;
            }
#endif // QT_CONFIG(tabwidget)
            s = "alignment"_L1;
            break;
#if QT_CONFIG(tabbar)
        case SH_TabBar_CloseButtonPosition:
            rule = renderRule(w, opt, PseudoElement_TabBarTabCloseButton);
            if (rule.hasPosition()) {
                Qt::Alignment align = rule.position()->position;
                if (align & Qt::AlignLeft || align & Qt::AlignTop)
                    return QTabBar::LeftSide;
                if (align & Qt::AlignRight || align & Qt::AlignBottom)
                    return QTabBar::RightSide;
            }
            break;
#endif
        case SH_TabBar_ElideMode: s = "tabbar-elide-mode"_L1; break;
        case SH_TabBar_PreferNoArrows: s = "tabbar-prefer-no-arrows"_L1; break;
        case SH_ComboBox_PopupFrameStyle:
#if QT_CONFIG(combobox)
            if (qobject_cast<const QComboBox *>(w)) {
                QAbstractItemView *view = w->findChild<QAbstractItemView *>();
                if (view) {
                    view->ensurePolished();
                    QRenderRule subRule = renderRule(view, PseudoElement_None);
                    if (subRule.hasBox() || !subRule.hasNativeBorder())
                        return QFrame::NoFrame;
                }
            }
#endif // QT_CONFIG(combobox)
            break;
        case SH_DialogButtonBox_ButtonsHaveIcons: s = "dialogbuttonbox-buttons-have-icons"_L1; break;
        case SH_Workspace_FillSpaceOnMaximize: s = "mdi-fill-space-on-maximize"_L1; break;
        case SH_TitleBar_NoBorder:
            if (rule.hasBorder())
                return !rule.border()->borders[LeftEdge];
            break;
        case SH_TitleBar_AutoRaise: { // plain absurd
            QRenderRule subRule = renderRule(w, opt, PseudoElement_TitleBar);
            if (subRule.hasDrawable())
                return 1;
            break;
                                   }
        case SH_ItemView_ArrowKeysNavigateIntoChildren: s = "arrow-keys-navigate-into-children"_L1; break;
        case SH_ItemView_PaintAlternatingRowColorsForEmptyArea: s = "paint-alternating-row-colors-for-empty-area"_L1; break;
        case SH_TitleBar_ShowToolTipsOnButtons: s = "titlebar-show-tooltips-on-buttons"_L1; break;
        case SH_Widget_Animation_Duration: s = "widget-animation-duration"_L1; break;
        case SH_ScrollBar_Transient:
            if (!rule.hasNativeBorder() || rule.hasBox() || rule.hasDrawable())
                return 0;
            break;
        default: break;
    }
    if (!s.isEmpty() && rule.hasStyleHint(s)) {
        return rule.styleHint(s).toInt();
    }

    return baseStyle()->styleHint(sh, opt, w, shret);
}

QRect QStyleSheetStyle::subControlRect(ComplexControl cc, const QStyleOptionComplex *opt, SubControl sc,
                              const QWidget *w) const
{
    RECURSION_GUARD(return baseStyle()->subControlRect(cc, opt, sc, w))

    QRenderRule rule = renderRule(w, opt);
    switch (cc) {
    case CC_ComboBox:
        if (const QStyleOptionComboBox *cb = qstyleoption_cast<const QStyleOptionComboBox *>(opt)) {
            if (rule.hasBox() || !rule.hasNativeBorder()) {
                switch (sc) {
                case SC_ComboBoxFrame: return rule.borderRect(opt->rect);
                case SC_ComboBoxEditField:
                    {
                        QRenderRule subRule = renderRule(w, opt, PseudoElement_ComboBoxDropDown);
                        QRect r = rule.contentsRect(opt->rect);
                        QRect r2 = positionRect(w, rule, subRule, PseudoElement_ComboBoxDropDown,
                                opt->rect, opt->direction);
                        if (subRule.hasPosition() && subRule.position()->position & Qt::AlignLeft) {
                            return visualRect(opt->direction, r, r.adjusted(r2.width(),0,0,0));
                        } else {
                            return visualRect(opt->direction, r, r.adjusted(0,0,-r2.width(),0));
                        }
                    }
                case SC_ComboBoxArrow: {
                    QRenderRule subRule = renderRule(w, opt, PseudoElement_ComboBoxDropDown);
                    return positionRect(w, rule, subRule, PseudoElement_ComboBoxDropDown, opt->rect, opt->direction);
                                                                           }
                case SC_ComboBoxListBoxPopup:
                default:
                    return baseStyle()->subControlRect(cc, opt, sc, w);
                }
            }

            QStyleOptionComboBox comboBox(*cb);
            comboBox.rect = rule.borderRect(opt->rect);
            return rule.baseStyleCanDraw() ? baseStyle()->subControlRect(cc, &comboBox, sc, w)
                                           : QWindowsStyle::subControlRect(cc, &comboBox, sc, w);
        }
        break;

#if QT_CONFIG(spinbox)
    case CC_SpinBox:
        if (const QStyleOptionSpinBox *spin = qstyleoption_cast<const QStyleOptionSpinBox *>(opt)) {
            QRenderRule upRule = renderRule(w, opt, PseudoElement_SpinBoxUpButton);
            QRenderRule downRule = renderRule(w, opt, PseudoElement_SpinBoxDownButton);
            bool ruleMatch = rule.hasBox() || !rule.hasNativeBorder();
            bool upRuleMatch = upRule.hasGeometry() || upRule.hasPosition();
            bool downRuleMatch = downRule.hasGeometry() || downRule.hasPosition();
            if (ruleMatch || upRuleMatch || downRuleMatch) {
                switch (sc) {
                case SC_SpinBoxFrame:
                    return rule.borderRect(opt->rect);
                case SC_SpinBoxEditField:
                    {
                        QRect r = rule.contentsRect(opt->rect);
                        // Use the widest button on each side to determine edit field size.
                        Qt::Alignment upAlign, downAlign;

                        upAlign = upRule.hasPosition() ? upRule.position()->position
                                : Qt::Alignment(Qt::AlignRight);
                        upAlign = resolveAlignment(opt->direction, upAlign);

                        downAlign = downRule.hasPosition() ? downRule.position()->position
                                : Qt::Alignment(Qt::AlignRight);
                        downAlign = resolveAlignment(opt->direction, downAlign);

                        const bool hasButtons = (spin->buttonSymbols != QAbstractSpinBox::NoButtons);
                        const int upSize = hasButtons
                                ? subControlRect(CC_SpinBox, opt, SC_SpinBoxUp, w).width() : 0;
                        const int downSize = hasButtons
                                ? subControlRect(CC_SpinBox, opt, SC_SpinBoxDown, w).width() : 0;

                        int widestL = qMax((upAlign & Qt::AlignLeft) ? upSize : 0,
                                (downAlign & Qt::AlignLeft) ? downSize : 0);
                        int widestR = qMax((upAlign & Qt::AlignRight) ? upSize : 0,
                                (downAlign & Qt::AlignRight) ? downSize : 0);
                        r.setRight(r.right() - widestR);
                        r.setLeft(r.left() + widestL);
                        return r;
                    }
                case SC_SpinBoxDown:
                    if (downRuleMatch)
                        return positionRect(w, rule, downRule, PseudoElement_SpinBoxDownButton,
                                opt->rect, opt->direction);
                    break;
                case SC_SpinBoxUp:
                    if (upRuleMatch)
                        return positionRect(w, rule, upRule, PseudoElement_SpinBoxUpButton,
                                opt->rect, opt->direction);
                    break;
                default:
                    break;
                }

                return baseStyle()->subControlRect(cc, opt, sc, w);
            }

            QStyleOptionSpinBox spinBox(*spin);
            spinBox.rect = rule.borderRect(opt->rect);
            return rule.baseStyleCanDraw() ? baseStyle()->subControlRect(cc, &spinBox, sc, w)
                                           : QWindowsStyle::subControlRect(cc, &spinBox, sc, w);
        }
        break;
#endif // QT_CONFIG(spinbox)

    case CC_GroupBox:
        if (const QStyleOptionGroupBox *gb = qstyleoption_cast<const QStyleOptionGroupBox *>(opt)) {
            switch (sc) {
            case SC_GroupBoxFrame:
            case SC_GroupBoxContents: {
                if (rule.hasBox() || !rule.hasNativeBorder()) {
                    return sc == SC_GroupBoxFrame ? rule.borderRect(opt->rect)
                                                  : rule.contentsRect(opt->rect);
                }
                QStyleOptionGroupBox groupBox(*gb);
                groupBox.rect = rule.borderRect(opt->rect);
                return baseStyle()->subControlRect(cc, &groupBox, sc, w);
            }
            default:
            case SC_GroupBoxLabel:
            case SC_GroupBoxCheckBox: {
                QRenderRule indRule = renderRule(w, opt, PseudoElement_GroupBoxIndicator);
                QRenderRule labelRule = renderRule(w, opt, PseudoElement_GroupBoxTitle);
                if (!labelRule.hasPosition() && !labelRule.hasGeometry() && !labelRule.hasBox()
                    && !labelRule.hasBorder() && !indRule.hasContentsSize()) {
                    QStyleOptionGroupBox groupBox(*gb);
                    groupBox.rect = rule.borderRect(opt->rect);
                    return baseStyle()->subControlRect(cc, &groupBox, sc, w);
                }
                int tw = opt->fontMetrics.horizontalAdvance(gb->text);
                int th = opt->fontMetrics.height();
                int spacing = pixelMetric(QStyle::PM_CheckBoxLabelSpacing, opt, w);
                int iw = pixelMetric(QStyle::PM_IndicatorWidth, opt, w);
                int ih = pixelMetric(QStyle::PM_IndicatorHeight, opt, w);

                if (gb->subControls & QStyle::SC_GroupBoxCheckBox) {
                    tw = tw + iw + spacing;
                    th = qMax(th, ih);
                }
                if (!labelRule.hasGeometry()) {
                    labelRule.geo = new QStyleSheetGeometryData(tw, th, tw, th, -1, -1);
                } else {
                    labelRule.geo->width = tw;
                    labelRule.geo->height = th;
                }
                if (!labelRule.hasPosition()) {
                    labelRule.p = new QStyleSheetPositionData(0, 0, 0, 0, defaultOrigin(PseudoElement_GroupBoxTitle),
                                                              gb->textAlignment, PositionMode_Static);
                }
                QRect r = positionRect(w, rule, labelRule, PseudoElement_GroupBoxTitle,
                                      opt->rect, opt->direction);
                if (gb->subControls & SC_GroupBoxCheckBox) {
                    r = labelRule.contentsRect(r);
                    if (sc == SC_GroupBoxLabel) {
                        r.setLeft(r.left() + iw + spacing);
                        r.setTop(r.center().y() - th/2);
                    } else {
                        r = QRect(r.left(), r.center().y() - ih/2, iw, ih);
                    }
                    return r;
                } else {
                    return labelRule.contentsRect(r);
                }
            }
            } // switch
        }
        break;

    case CC_ToolButton:
        if (const QStyleOptionToolButton *tb = qstyleoption_cast<const QStyleOptionToolButton *>(opt)) {
            if (rule.hasBox() || !rule.hasNativeBorder()) {
                switch (sc) {
                case SC_ToolButton: return rule.borderRect(opt->rect);
                case SC_ToolButtonMenu: {
                    QRenderRule subRule = renderRule(w, opt, PseudoElement_ToolButtonMenu);
                    return positionRect(w, rule, subRule, PseudoElement_ToolButtonMenu, opt->rect, opt->direction);
                                                                            }
                default:
                    break;
                }
            }

            QStyleOptionToolButton tool(*tb);
            tool.rect = rule.borderRect(opt->rect);
            return rule.baseStyleCanDraw() ? baseStyle()->subControlRect(cc, &tool, sc, w)
                                           : QWindowsStyle::subControlRect(cc, &tool, sc, w);
            }
            break;

#if QT_CONFIG(scrollbar)
    case CC_ScrollBar:
        if (const QStyleOptionSlider *sb = qstyleoption_cast<const QStyleOptionSlider *>(opt)) {
            QStyleOptionSlider styleOptionSlider(*sb);
            styleOptionSlider.rect = rule.borderRect(opt->rect);
            if (rule.hasDrawable() || rule.hasBox()) {
                QRect grooveRect;
                if (!rule.hasBox()) {
                    grooveRect = rule.baseStyleCanDraw() ? baseStyle()->subControlRect(cc, sb, SC_ScrollBarGroove, w)
                                 : QWindowsStyle::subControlRect(cc, sb, SC_ScrollBarGroove, w);
                } else {
                    grooveRect = rule.contentsRect(opt->rect);
                }

                PseudoElement pe = PseudoElement_None;

                switch (sc) {
                case SC_ScrollBarGroove:
                    return grooveRect;
                case SC_ScrollBarAddPage:
                case SC_ScrollBarSubPage:
                case SC_ScrollBarSlider: {
                    QRect contentRect = grooveRect;
                    if (hasStyleRule(w, PseudoElement_ScrollBarSlider)) {
                        QRenderRule sliderRule = renderRule(w, opt, PseudoElement_ScrollBarSlider);
                        Origin origin = sliderRule.hasPosition() ? sliderRule.position()->origin : defaultOrigin(PseudoElement_ScrollBarSlider);
                        contentRect = rule.originRect(opt->rect, origin);
                    }
                    int maxlen = (styleOptionSlider.orientation == Qt::Horizontal) ? contentRect.width() : contentRect.height();
                    int sliderlen;
                    if (sb->maximum != sb->minimum) {
                        uint range = sb->maximum - sb->minimum;
                        sliderlen = (qint64(sb->pageStep) * maxlen) / (range + sb->pageStep);

                        int slidermin = pixelMetric(PM_ScrollBarSliderMin, sb, w);
                        if (sliderlen < slidermin || range > INT_MAX / 2)
                            sliderlen = slidermin;
                        if (sliderlen > maxlen)
                            sliderlen = maxlen;
                    } else {
                        sliderlen = maxlen;
                    }
                    int sliderstart = (styleOptionSlider.orientation == Qt::Horizontal ? contentRect.left() : contentRect.top())
                        + sliderPositionFromValue(sb->minimum, sb->maximum, sb->sliderPosition,
                                                  maxlen - sliderlen, sb->upsideDown);

                    QRect sr = (sb->orientation == Qt::Horizontal)
                               ? QRect(sliderstart, contentRect.top(), sliderlen, contentRect.height())
                               : QRect(contentRect.left(), sliderstart, contentRect.width(), sliderlen);
                    if (sc == SC_ScrollBarSubPage)
                        sr = QRect(contentRect.topLeft(), sb->orientation == Qt::Horizontal ? sr.bottomLeft() : sr.topRight());
                    else if (sc == SC_ScrollBarAddPage)
                        sr = QRect(sb->orientation == Qt::Horizontal ? sr.topRight() : sr.bottomLeft(), contentRect.bottomRight());
                    return visualRect(styleOptionSlider.direction, grooveRect, sr);
                }
                case SC_ScrollBarAddLine: pe = PseudoElement_ScrollBarAddLine; break;
                case SC_ScrollBarSubLine: pe = PseudoElement_ScrollBarSubLine; break;
                case SC_ScrollBarFirst: pe = PseudoElement_ScrollBarFirst;  break;
                case SC_ScrollBarLast: pe = PseudoElement_ScrollBarLast; break;
                default: break;
                }
                if (hasStyleRule(w,pe)) {
                    QRenderRule subRule = renderRule(w, opt, pe);
                    if (subRule.hasPosition() || subRule.hasGeometry() || subRule.hasBox()) {
                        const QStyleSheetPositionData *pos = subRule.position();
                        QRect originRect = grooveRect;
                        if (rule.hasBox()) {
                            Origin origin = (pos && pos->origin != Origin_Unknown) ? pos->origin : defaultOrigin(pe);
                            originRect = rule.originRect(opt->rect, origin);
                        }
                        return positionRect(w, subRule, pe, originRect, styleOptionSlider.direction);
                    }
                }
            }
            return rule.baseStyleCanDraw() ? baseStyle()->subControlRect(cc, &styleOptionSlider, sc, w)
                                           : QWindowsStyle::subControlRect(cc, &styleOptionSlider, sc, w);
        }
        break;
#endif // QT_CONFIG(scrollbar)

#if QT_CONFIG(slider)
    case CC_Slider:
        if (const QStyleOptionSlider *slider = qstyleoption_cast<const QStyleOptionSlider *>(opt)) {
            QRenderRule subRule = renderRule(w, opt, PseudoElement_SliderGroove);
            if (!subRule.hasDrawable())
                break;
            subRule.img = nullptr;
            QRect gr = positionRect(w, rule, subRule, PseudoElement_SliderGroove, opt->rect, opt->direction);
            switch (sc) {
            case SC_SliderGroove:
                return gr;
            case SC_SliderHandle: {
                bool horizontal = slider->orientation & Qt::Horizontal;
                QRect cr = subRule.contentsRect(gr);
                QRenderRule subRule2 = renderRule(w, opt, PseudoElement_SliderHandle);
                int len = horizontal ? subRule2.size().width() : subRule2.size().height();
                subRule2.img = nullptr;
                subRule2.geo = nullptr;
                cr = positionRect(w, subRule2, PseudoElement_SliderHandle, cr, opt->direction);
                int thickness = horizontal ? cr.height() : cr.width();
                int sliderPos = sliderPositionFromValue(slider->minimum, slider->maximum, slider->sliderPosition,
                                                        (horizontal ? cr.width() : cr.height()) - len, slider->upsideDown);
                cr = horizontal ? QRect(cr.x() + sliderPos, cr.y(), len, thickness)
                                  : QRect(cr.x(), cr.y() + sliderPos, thickness, len);
                return subRule2.borderRect(cr);
                break; }
            case SC_SliderTickmarks:
                // TODO...
            default:
                break;
            }
        }
        break;
#endif // QT_CONFIG(slider)

    case CC_MdiControls:
        if (hasStyleRule(w, PseudoElement_MdiCloseButton)
            || hasStyleRule(w, PseudoElement_MdiNormalButton)
            || hasStyleRule(w, PseudoElement_MdiMinButton)) {
            QList<QVariant> layout = rule.styleHint("button-layout"_L1).toList();
            if (layout.isEmpty())
                layout = subControlLayout("mNX"_L1);

            int x = 0, width = 0;
            QRenderRule subRule;
            for (int i = 0; i < layout.size(); i++) {
                int layoutButton = layout[i].toInt();
                if (layoutButton < PseudoElement_MdiCloseButton
                    || layoutButton > PseudoElement_MdiNormalButton)
                    continue;
                QStyle::SubControl control = knownPseudoElements[layoutButton].subControl;
                if (!(opt->subControls & control))
                    continue;
                subRule = renderRule(w, opt, layoutButton);
                width = subRule.size().width();
                if (sc == control)
                    break;
                x += width;
            }

            return subRule.borderRect(QRect(x, opt->rect.top(), width, opt->rect.height()));
        }
        break;

    case CC_TitleBar:
        if (const QStyleOptionTitleBar *tb = qstyleoption_cast<const QStyleOptionTitleBar *>(opt)) {
            QRenderRule subRule = renderRule(w, opt, PseudoElement_TitleBar);
            if (!subRule.hasDrawable() && !subRule.hasBox() && !subRule.hasBorder())
                break;
            QHash<QStyle::SubControl, QRect> layoutRects = titleBarLayout(w, tb);
            return layoutRects.value(sc);
        }
        break;

    default:
        break;
    }

    return baseStyle()->subControlRect(cc, opt, sc, w);
}

QRect QStyleSheetStyle::subElementRect(SubElement se, const QStyleOption *opt, const QWidget *w) const
{
    RECURSION_GUARD(return baseStyle()->subElementRect(se, opt, w))

    QRenderRule rule = renderRule(w, opt);
#if QT_CONFIG(tabbar)
    int pe = PseudoElement_None;
#endif

    switch (se) {
    case SE_PushButtonContents:
    case SE_PushButtonBevel:
    case SE_PushButtonFocusRect:
        if (const QStyleOptionButton *btn = qstyleoption_cast<const QStyleOptionButton *>(opt)) {
            if (rule.hasBox() || !rule.hasNativeBorder()) {
                return visualRect(opt->direction, opt->rect, se == SE_PushButtonBevel
                                                                ? rule.borderRect(opt->rect)
                                                                : rule.contentsRect(opt->rect));
            }
            return rule.baseStyleCanDraw() ? baseStyle()->subElementRect(se, btn, w)
                                           : QWindowsStyle::subElementRect(se, btn, w);
        }
        break;

    case SE_LineEditContents:
    case SE_FrameContents:
    case SE_ShapedFrameContents:
        if (rule.hasBox() || !rule.hasNativeBorder()) {
            return visualRect(opt->direction, opt->rect, rule.contentsRect(opt->rect));
        }
        break;

    case SE_CheckBoxIndicator:
    case SE_RadioButtonIndicator:
        if (rule.hasBox() || rule.hasBorder() || hasStyleRule(w, PseudoElement_Indicator)) {
            PseudoElement pe = se == SE_CheckBoxIndicator ? PseudoElement_Indicator : PseudoElement_ExclusiveIndicator;
            QRenderRule subRule = renderRule(w, opt, pe);
            return positionRect(w, rule, subRule, pe, opt->rect, opt->direction);
        }
        break;

    case SE_CheckBoxContents:
    case SE_RadioButtonContents:
        if (rule.hasBox() || rule.hasBorder() || hasStyleRule(w, PseudoElement_Indicator)) {
            bool isRadio = se == SE_RadioButtonContents;
            QRect ir = subElementRect(isRadio ? SE_RadioButtonIndicator : SE_CheckBoxIndicator,
                                      opt, w);
            ir = visualRect(opt->direction, opt->rect, ir);
            int spacing = pixelMetric(isRadio ? PM_RadioButtonLabelSpacing : PM_CheckBoxLabelSpacing, nullptr, w);
            QRect cr = rule.contentsRect(opt->rect);
            ir.setRect(ir.left() + ir.width() + spacing, cr.y(),
                       cr.width() - ir.width() - spacing, cr.height());
            return visualRect(opt->direction, opt->rect, ir);
        }
        break;

    case SE_ToolBoxTabContents:
        if (w && hasStyleRule(w->parentWidget(), PseudoElement_ToolBoxTab)) {
            QRenderRule subRule = renderRule(w->parentWidget(), opt, PseudoElement_ToolBoxTab);
            return visualRect(opt->direction, opt->rect, subRule.contentsRect(opt->rect));
        }
        break;

    case SE_RadioButtonFocusRect:
    case SE_RadioButtonClickRect: // focusrect | indicator
        if (rule.hasBox() || rule.hasBorder() || hasStyleRule(w, PseudoElement_Indicator)) {
            return opt->rect;
        }
        break;

    case SE_CheckBoxFocusRect:
    case SE_CheckBoxClickRect: // relies on indicator and contents
        return ParentStyle::subElementRect(se, opt, w);

#if QT_CONFIG(itemviews)
    case SE_ItemViewItemCheckIndicator:
        if (!qstyleoption_cast<const QStyleOptionViewItem *>(opt)) {
            return subElementRect(SE_CheckBoxIndicator, opt, w);
        }
        Q_FALLTHROUGH();
    case SE_ItemViewItemText:
    case SE_ItemViewItemDecoration:
    case SE_ItemViewItemFocusRect:
        if (const QStyleOptionViewItem *vopt = qstyleoption_cast<const QStyleOptionViewItem *>(opt)) {
            QRenderRule subRule = renderRule(w, opt, PseudoElement_ViewItem);
            PseudoElement pe = PseudoElement_None;
            if (se == SE_ItemViewItemText || se == SE_ItemViewItemFocusRect)
                pe = PseudoElement_ViewItemText;
            else if (se == SE_ItemViewItemDecoration && vopt->features & QStyleOptionViewItem::HasDecoration)
                pe = PseudoElement_ViewItemIcon;
            else if (se == SE_ItemViewItemCheckIndicator && vopt->features & QStyleOptionViewItem::HasCheckIndicator)
                pe = PseudoElement_ViewItemIndicator;
            else
                break;
            if (subRule.hasGeometry() || subRule.hasBox() || !subRule.hasNativeBorder() || hasStyleRule(w, pe)) {
                QRenderRule subRule2 = renderRule(w, opt, pe);
                QStyleOptionViewItem optCopy(*vopt);
                optCopy.rect = subRule.contentsRect(vopt->rect);
                QRect rect = ParentStyle::subElementRect(se, &optCopy, w);
                return positionRect(w, subRule2, pe, rect, opt->direction);
            }
         }
        break;
#endif // QT_CONFIG(itemviews)

    case SE_HeaderArrow: {
        QRenderRule subRule = renderRule(w, opt, PseudoElement_HeaderViewUpArrow);
        if (subRule.hasPosition() || subRule.hasGeometry())
            return positionRect(w, rule, subRule, PseudoElement_HeaderViewUpArrow, opt->rect, opt->direction);
                         }
        break;

    case SE_HeaderLabel: {
        QRenderRule subRule = renderRule(w, opt, PseudoElement_HeaderViewSection);
        if (subRule.hasBox() || !subRule.hasNativeBorder())
            return subRule.contentsRect(opt->rect);
                         }
        break;

    case SE_ProgressBarGroove:
    case SE_ProgressBarContents:
    case SE_ProgressBarLabel:
        if (const QStyleOptionProgressBar *pb = qstyleoption_cast<const QStyleOptionProgressBar *>(opt)) {
            if (rule.hasBox() || !rule.hasNativeBorder() || rule.hasPosition() || hasStyleRule(w, PseudoElement_ProgressBarChunk)) {
                if (se == SE_ProgressBarGroove)
                    return rule.borderRect(pb->rect);
                else if (se == SE_ProgressBarContents)
                    return rule.contentsRect(pb->rect);

                QSize sz = pb->fontMetrics.size(0, pb->text);
                return QStyle::alignedRect(Qt::LeftToRight, rule.hasPosition() ? rule.position()->textAlignment : pb->textAlignment,
                                           sz, pb->rect);
            }
        }
        break;

#if QT_CONFIG(tabbar)
    case SE_TabWidgetLeftCorner:
        pe = PseudoElement_TabWidgetLeftCorner;
        Q_FALLTHROUGH();
    case SE_TabWidgetRightCorner:
        if (pe == PseudoElement_None)
            pe = PseudoElement_TabWidgetRightCorner;
        Q_FALLTHROUGH();
    case SE_TabWidgetTabBar:
        if (pe == PseudoElement_None)
            pe = PseudoElement_TabWidgetTabBar;
        Q_FALLTHROUGH();
    case SE_TabWidgetTabPane:
    case SE_TabWidgetTabContents:
        if (pe == PseudoElement_None)
            pe = PseudoElement_TabWidgetPane;

        if (hasStyleRule(w, pe)) {
            QRect r = QWindowsStyle::subElementRect(pe == PseudoElement_TabWidgetPane ? SE_TabWidgetTabPane : se, opt, w);
            QRenderRule subRule = renderRule(w, opt, pe);
            r = positionRect(w, subRule, pe, r, opt->direction);
            if (pe == PseudoElement_TabWidgetTabBar) {
                Q_ASSERT(opt);
                r = opt->rect.intersected(r);
            }
            if (se == SE_TabWidgetTabContents)
                r = subRule.contentsRect(r);
            return r;
        }
        break;

    case SE_TabBarScrollLeftButton:
    case SE_TabBarScrollRightButton:
        if (hasStyleRule(w, PseudoElement_TabBarScroller))
            return ParentStyle::subElementRect(se, opt, w);
        break;

    case SE_TabBarTearIndicator: {
        QRenderRule subRule = renderRule(w, opt, PseudoElement_TabBarTear);
        if (subRule.hasContentsSize()) {
            QRect r;
            if (const QStyleOptionTab *tab = qstyleoption_cast<const QStyleOptionTab *>(opt)) {
                switch (tab->shape) {
                case QTabBar::RoundedNorth:
                case QTabBar::TriangularNorth:
                case QTabBar::RoundedSouth:
                case QTabBar::TriangularSouth:
                    r.setRect(tab->rect.left(), tab->rect.top(), subRule.size().width(), opt->rect.height());
                    break;
                case QTabBar::RoundedWest:
                case QTabBar::TriangularWest:
                case QTabBar::RoundedEast:
                case QTabBar::TriangularEast:
                    r.setRect(tab->rect.left(), tab->rect.top(), opt->rect.width(), subRule.size().height());
                    break;
                default:
                    break;
                }
                r = visualRect(opt->direction, opt->rect, r);
            }
            return r;
        }
        break;
    }
    case SE_TabBarTabText:
    case SE_TabBarTabLeftButton:
    case SE_TabBarTabRightButton: {
        QRenderRule subRule = renderRule(w, opt, PseudoElement_TabBarTab);
        if (subRule.hasBox() || !subRule.hasNativeBorder() || subRule.hasFont) {
            if (se == SE_TabBarTabText) {
                if (const QStyleOptionTab *tab = qstyleoption_cast<const QStyleOptionTab *>(opt)) {
                    const QTabBar *bar = qobject_cast<const QTabBar *>(w);
                    const QRect optRect = bar && tab->tabIndex != -1 ? bar->tabRect(tab->tabIndex) : opt->rect;
                    const QRect r = positionRect(w, subRule, PseudoElement_TabBarTab, optRect, opt->direction);
                    QStyleOptionTab tabCopy(*tab);
                    if (subRule.hasFont) {
                        const QFont ruleFont = w ? subRule.font.resolve(w->font()) : subRule.font;
                        tabCopy.fontMetrics = QFontMetrics(ruleFont);
                    }
                    tabCopy.rect = subRule.contentsRect(r);
                    return ParentStyle::subElementRect(se, &tabCopy, w);
                }
            }
            return ParentStyle::subElementRect(se, opt, w);
        }
        break;
    }
#endif // QT_CONFIG(tabbar)

    case SE_DockWidgetCloseButton:
    case SE_DockWidgetFloatButton: {
        PseudoElement pe = (se == SE_DockWidgetCloseButton) ? PseudoElement_DockWidgetCloseButton : PseudoElement_DockWidgetFloatButton;
        QRenderRule subRule2 = renderRule(w, opt, pe);
        if (!subRule2.hasPosition())
            break;
        QRenderRule subRule = renderRule(w, opt, PseudoElement_DockWidgetTitle);
        return positionRect(w, subRule, subRule2, pe, opt->rect, opt->direction);
                                   }

#if QT_CONFIG(toolbar)
    case SE_ToolBarHandle:
        if (hasStyleRule(w, PseudoElement_ToolBarHandle))
            return ParentStyle::subElementRect(se, opt, w);
        break;
#endif // QT_CONFIG(toolbar)

    // On mac we make pixel adjustments to layouts which are not
    // desirable when you have custom style sheets on them
    case SE_CheckBoxLayoutItem:
    case SE_ComboBoxLayoutItem:
    case SE_DateTimeEditLayoutItem:
    case SE_LabelLayoutItem:
    case SE_ProgressBarLayoutItem:
    case SE_PushButtonLayoutItem:
    case SE_RadioButtonLayoutItem:
    case SE_SliderLayoutItem:
    case SE_SpinBoxLayoutItem:
    case SE_ToolButtonLayoutItem:
    case SE_FrameLayoutItem:
    case SE_GroupBoxLayoutItem:
    case SE_TabWidgetLayoutItem:
        if (!rule.hasNativeBorder())
            return opt->rect;
        break;

    default:
        break;
    }

    return baseStyle()->subElementRect(se, opt, w);
}

bool QStyleSheetStyle::event(QEvent *e)
{
    return (baseStyle()->event(e) && e->isAccepted()) || ParentStyle::event(e);
}

void QStyleSheetStyle::updateStyleSheetFont(QWidget* w) const
{
    // Qt's fontDialog relies on the font of the sample edit for its selection,
    // we should never override it.
    if (w->objectName() == "qt_fontDialog_sampleEdit"_L1)
        return;

    QWidget *container = containerWidget(w);
    QRenderRule rule = renderRule(container, PseudoElement_None,
            PseudoClass_Active | PseudoClass_Enabled | extendedPseudoClass(container));

    const bool useStyleSheetPropagationInWidgetStyles =
        QCoreApplication::testAttribute(Qt::AA_UseStyleSheetPropagationInWidgetStyles);

    if (useStyleSheetPropagationInWidgetStyles) {
        unsetStyleSheetFont(w);

        if (rule.font.resolveMask()) {
            QFont wf = w->d_func()->localFont();
            styleSheetCaches->customFontWidgets.insert(w, {wf, rule.font.resolveMask()});

            QFont font = rule.font.resolve(wf);
            font.setResolveMask(wf.resolveMask() | rule.font.resolveMask());
            w->setFont(font);
        }
    } else {
        QFont wf = w->d_func()->localFont();
        QFont font = rule.font.resolve(wf);
        font.setResolveMask(wf.resolveMask() | rule.font.resolveMask());

        if ((!w->isWindow() || w->testAttribute(Qt::WA_WindowPropagation))
            && isNaturalChild(w) && qobject_cast<QWidget *>(w->parent())) {

            font = font.resolve(static_cast<QWidget *>(w->parent())->font());
        }

        if (wf.resolveMask() == font.resolveMask() && wf == font)
            return;

        w->data->fnt = font;
        w->d_func()->directFontResolveMask = font.resolveMask();

        QEvent e(QEvent::FontChange);
        QCoreApplication::sendEvent(w, &e);
    }
}

void QStyleSheetStyle::saveWidgetFont(QWidget* w, const QFont& font) const
{
    w->setProperty("_q_styleSheetWidgetFont", font);
}

void QStyleSheetStyle::clearWidgetFont(QWidget* w) const
{
    w->setProperty("_q_styleSheetWidgetFont", QVariant());
}

// Polish palette that should be used for a particular widget, with particular states
// (eg. :focus, :hover, ...)
// this is called by widgets that paint themself in their paint event
// Returns \c true if there is a new palette in pal.
bool QStyleSheetStyle::styleSheetPalette(const QWidget* w, const QStyleOption* opt, QPalette* pal)
{
    if (!w || !opt || !pal)
        return false;

    RECURSION_GUARD(return false)

    w = containerWidget(w);

    QRenderRule rule = renderRule(w, PseudoElement_None, pseudoClass(opt->state) | extendedPseudoClass(w));
    if (!rule.hasPalette())
        return false;

    rule.configurePalette(pal, QPalette::NoRole, QPalette::NoRole);
    return true;
}

Qt::Alignment QStyleSheetStyle::resolveAlignment(Qt::LayoutDirection layDir, Qt::Alignment src)
{
    if (layDir == Qt::LeftToRight || src & Qt::AlignAbsolute)
        return src;

    if (src & Qt::AlignLeft) {
        src &= ~Qt::AlignLeft;
        src |= Qt::AlignRight;
    } else if (src & Qt::AlignRight) {
        src &= ~Qt::AlignRight;
        src |= Qt::AlignLeft;
    }
    src |= Qt::AlignAbsolute;
    return src;
}

// Returns whether the given QWidget has a "natural" parent, meaning that
// the parent contains this child as part of its normal operation.
// An example is the QTabBar inside a QTabWidget.
// This does not mean that any QTabBar which is a child of QTabWidget will
// match, only the one that was created by the QTabWidget initialization
// (and hence has the correct object name).
bool QStyleSheetStyle::isNaturalChild(const QObject *obj)
{
    if (obj->objectName().startsWith("qt_"_L1))
        return true;

    return false;
}

QPixmap QStyleSheetStyle::loadPixmap(const QString &fileName, const QObject *context)
{
    qreal ratio = -1.0;
    if (const QWidget *widget = qobject_cast<const QWidget *>(context)) {
        if (QScreen *screen = QApplication::screenAt(widget->mapToGlobal(QPoint(0, 0))))
            ratio = screen->devicePixelRatio();
    }

    if (ratio < 0) {
        if (const QApplication *app = qApp)
            ratio = app->devicePixelRatio();
        else
            ratio = 1.0;
    }

    qreal sourceDevicePixelRatio = 1.0;
    QString resolvedFileName = qt_findAtNxFile(fileName, ratio, &sourceDevicePixelRatio);
    QPixmap pixmap(resolvedFileName);
    pixmap.setDevicePixelRatio(sourceDevicePixelRatio);
    return pixmap;
}

QT_END_NAMESPACE

#include "moc_qstylesheetstyle_p.cpp"

#endif // QT_CONFIG(style_stylesheet)
