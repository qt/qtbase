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

/*
  Note: The qdoc comments for QMacStyle are contained in
  .../doc/src/qstyles.qdoc.
*/

#include <AppKit/AppKit.h>

#include "qmacstyle_mac_p.h"
#include "qmacstyle_mac_p_p.h"

#define QMAC_QAQUASTYLE_SIZE_CONSTRAIN
//#define DEBUG_SIZE_CONSTRAINT

#include <QtCore/qoperatingsystemversion.h>
#include <QtCore/qvariant.h>
#include <QtCore/qvarlengtharray.h>

#include <QtCore/private/qcore_mac_p.h>

#include <QtGui/qpainterpath.h>
#include <QtGui/private/qcoregraphics_p.h>
#include <QtGui/qpa/qplatformfontdatabase.h>
#include <QtGui/qpa/qplatformtheme.h>

#include <QtWidgets/private/qstyleanimation_p.h>

#if QT_CONFIG(mdiarea)
#include <QtWidgets/qmdisubwindow.h>
#endif
#if QT_CONFIG(scrollbar)
#include <QtWidgets/qscrollbar.h>
#endif
#if QT_CONFIG(tabbar)
#include <QtWidgets/private/qtabbar_p.h>
#endif
#if QT_CONFIG(wizard)
#include <QtWidgets/qwizard.h>
#endif

#include <cmath>

QT_USE_NAMESPACE

static QWindow *qt_getWindow(const QWidget *widget)
{
    return widget ? widget->window()->windowHandle() : 0;
}

@interface QT_MANGLE_NAMESPACE(QIndeterminateProgressIndicator) : NSProgressIndicator

@property (readonly, nonatomic) NSInteger animators;

- (instancetype)init;

- (void)startAnimation;
- (void)stopAnimation;

- (void)drawWithFrame:(CGRect)rect inView:(NSView *)view;

@end

QT_NAMESPACE_ALIAS_OBJC_CLASS(QIndeterminateProgressIndicator);

@implementation QIndeterminateProgressIndicator

- (instancetype)init
{
    if ((self = [super init])) {
        _animators = 0;
        self.indeterminate = YES;
        self.usesThreadedAnimation = NO;
        self.alphaValue = 0.0;
    }

    return self;
}

- (void)startAnimation
{
    if (_animators == 0) {
        self.hidden = NO;
        [super startAnimation:self];
    }
    ++_animators;
}

- (void)stopAnimation
{
    --_animators;
    if (_animators == 0) {
        [super stopAnimation:self];
        self.hidden = YES;
        [self removeFromSuperviewWithoutNeedingDisplay];
    }
}

- (void)drawWithFrame:(CGRect)rect inView:(NSView *)view
{
    // The alphaValue change is not strictly necessary, but feels safer.
    self.alphaValue = 1.0;
    if (self.superview != view)
        [view addSubview:self];
    if (!CGRectEqualToRect(self.frame, rect))
        self.frame = rect;
    [self drawRect:rect];
    self.alphaValue = 0.0;
}

@end

@interface QT_MANGLE_NAMESPACE(QVerticalSplitView) : NSSplitView
- (BOOL)isVertical;
@end

QT_NAMESPACE_ALIAS_OBJC_CLASS(QVerticalSplitView);

@implementation QVerticalSplitView
- (BOOL)isVertical
{
    return YES;
}
@end

// See render code in drawPrimitive(PE_FrameTabWidget)
@interface QT_MANGLE_NAMESPACE(QDarkNSBox) : NSBox
@end

QT_NAMESPACE_ALIAS_OBJC_CLASS(QDarkNSBox);

@implementation QDarkNSBox
- (instancetype)init
{
    if ((self = [super init])) {
        self.title = @"";
        self.titlePosition = NSNoTitle;
        self.boxType = NSBoxCustom;
        self.cornerRadius = 3;
        self.borderColor = [NSColor.controlColor colorWithAlphaComponent:0.1];
        self.fillColor = [NSColor.darkGrayColor colorWithAlphaComponent:0.2];
    }

    return self;
}

- (void)drawRect:(NSRect)rect
{
    [super drawRect:rect];
}
@end

QT_BEGIN_NAMESPACE

// The following constants are used for adjusting the size
// of push buttons so that they are drawn inside their bounds.
const int QMacStylePrivate::PushButtonLeftOffset = 6;
const int QMacStylePrivate::PushButtonRightOffset = 12;
const int QMacStylePrivate::PushButtonContentPadding = 6;

QVector<QPointer<QObject> > QMacStylePrivate::scrollBars;

// Title bar gradient colors for Lion were determined by inspecting PSDs exported
// using CoreUI's CoreThemeDocument; there is no public API to retrieve them

static QLinearGradient titlebarGradientActive()
{
    static QLinearGradient darkGradient = [](){
        QLinearGradient gradient;
        // FIXME: colors are chosen somewhat arbitrarily and could be fine-tuned,
        // or ideally determined by calling a native API.
        gradient.setColorAt(0, QColor(47, 47, 47));
        return gradient;
    }();
    static QLinearGradient lightGradient = [](){
        QLinearGradient gradient;
        gradient.setColorAt(0, QColor(235, 235, 235));
        gradient.setColorAt(0.5, QColor(210, 210, 210));
        gradient.setColorAt(0.75, QColor(195, 195, 195));
        gradient.setColorAt(1, QColor(180, 180, 180));
        return gradient;
    }();
    return qt_mac_applicationIsInDarkMode() ? darkGradient : lightGradient;
}

static QLinearGradient titlebarGradientInactive()
{
    static QLinearGradient darkGradient = [](){
        QLinearGradient gradient;
        gradient.setColorAt(1, QColor(42, 42, 42));
        return gradient;
    }();
    static QLinearGradient lightGradient = [](){
        QLinearGradient gradient;
        gradient.setColorAt(0, QColor(250, 250, 250));
        gradient.setColorAt(1, QColor(225, 225, 225));
        return gradient;
    }();
    return qt_mac_applicationIsInDarkMode() ? darkGradient : lightGradient;
}

#if QT_CONFIG(tabwidget)
/*
    Since macOS 10.14 AppKit is using transparency more extensively, especially for the
    dark theme. Inactive buttons, for example, are semi-transparent. And we use them to
    draw tab widget's tab bar. The combination of NSBox (also a part of tab widget)
    and these transparent buttons gives us an undesired side-effect: an outline of
    NSBox is visible through transparent buttons. To avoid this, we have this hack below:
    we clip the area where the line would be visible through the buttons. The area we
    want to clip away can be described as an intersection of the option's rect and
    the tab widget's tab bar rect. But some adjustments are required, since those rects
    are anyway adjusted during the rendering and they are not exactly what you'll see on
    the screen. Thus this switch-statement inside.
*/
static void clipTabBarFrame(const QStyleOption *option, const QMacStyle *style, CGContextRef ctx)
{
    Q_ASSERT(option);
    Q_ASSERT(style);
    Q_ASSERT(ctx);

    if (qt_mac_applicationIsInDarkMode()) {
        QTabWidget *tabWidget = qobject_cast<QTabWidget *>(option->styleObject);
        Q_ASSERT(tabWidget);

        QRect tabBarRect = style->subElementRect(QStyle::SE_TabWidgetTabBar, option, tabWidget).adjusted(2, 0, -3, 0);
        switch (tabWidget->tabPosition()) {
        case QTabWidget::South:
            tabBarRect.setY(tabBarRect.y() + tabBarRect.height() / 2);
            break;
        case QTabWidget::North:
        case QTabWidget::West:
            tabBarRect = tabBarRect.adjusted(0, 2, 0, -2);
            break;
        case QTabWidget::East:
            tabBarRect = tabBarRect.adjusted(tabBarRect.width() / 2, 2, tabBarRect.width() / 2, -2);
        }

        const QRegion clipPath = QRegion(option->rect) - tabBarRect;
        QVarLengthArray<CGRect, 3> cgRects;
        for (const QRect &qtRect : clipPath)
            cgRects.push_back(qtRect.toCGRect());
        if (cgRects.size())
            CGContextClipToRects(ctx, &cgRects[0], size_t(cgRects.size()));
    }
}
#endif

static const QColor titlebarSeparatorLineActive(111, 111, 111);
static const QColor titlebarSeparatorLineInactive(131, 131, 131);
static const QColor darkModeSeparatorLine(88, 88, 88);

// Gradient colors used for the dock widget title bar and
// non-unifed tool bar bacground.
static const QColor lightMainWindowGradientBegin(240, 240, 240);
static const QColor lightMainWindowGradientEnd(200, 200, 200);
static const QColor darkMainWindowGradientBegin(47, 47, 47);
static const QColor darkMainWindowGradientEnd(47, 47, 47);

static const int DisclosureOffset = 4;

static const qreal titleBarIconTitleSpacing = 5;
static const qreal titleBarTitleRightMargin = 12;
static const qreal titleBarButtonSpacing = 8;

// Tab bar colors
// active: window is active
// selected: tab is selected
// hovered: tab is hovered
bool isDarkMode() { return qt_mac_applicationIsInDarkMode(); }

#if QT_CONFIG(tabbar)
static const QColor lightTabBarTabBackgroundActive(190, 190, 190);
static const QColor darkTabBarTabBackgroundActive(38, 38, 38);
static const QColor tabBarTabBackgroundActive() { return isDarkMode() ? darkTabBarTabBackgroundActive : lightTabBarTabBackgroundActive; }

static const QColor lightTabBarTabBackgroundActiveHovered(178, 178, 178);
static const QColor darkTabBarTabBackgroundActiveHovered(32, 32, 32);
static const QColor tabBarTabBackgroundActiveHovered() { return isDarkMode() ? darkTabBarTabBackgroundActiveHovered : lightTabBarTabBackgroundActiveHovered; }

static const QColor lightTabBarTabBackgroundActiveSelected(211, 211, 211);
static const QColor darkTabBarTabBackgroundActiveSelected(52, 52, 52);
static const QColor tabBarTabBackgroundActiveSelected() { return isDarkMode() ? darkTabBarTabBackgroundActiveSelected : lightTabBarTabBackgroundActiveSelected; }

static const QColor lightTabBarTabBackground(227, 227, 227);
static const QColor darkTabBarTabBackground(38, 38, 38);
static const QColor tabBarTabBackground() { return isDarkMode() ? darkTabBarTabBackground : lightTabBarTabBackground; }

static const QColor lightTabBarTabBackgroundSelected(246, 246, 246);
static const QColor darkTabBarTabBackgroundSelected(52, 52, 52);
static const QColor tabBarTabBackgroundSelected() { return isDarkMode() ? darkTabBarTabBackgroundSelected : lightTabBarTabBackgroundSelected; }

static const QColor lightTabBarTabLineActive(160, 160, 160);
static const QColor darkTabBarTabLineActive(90, 90, 90);
static const QColor tabBarTabLineActive() { return isDarkMode() ? darkTabBarTabLineActive : lightTabBarTabLineActive; }

static const QColor lightTabBarTabLineActiveHovered(150, 150, 150);
static const QColor darkTabBarTabLineActiveHovered(90, 90, 90);
static const QColor tabBarTabLineActiveHovered() { return isDarkMode() ? darkTabBarTabLineActiveHovered : lightTabBarTabLineActiveHovered; }

static const QColor lightTabBarTabLine(210, 210, 210);
static const QColor darkTabBarTabLine(90, 90, 90);
static const QColor tabBarTabLine() { return isDarkMode() ? darkTabBarTabLine : lightTabBarTabLine; }

static const QColor lightTabBarTabLineSelected(189, 189, 189);
static const QColor darkTabBarTabLineSelected(90, 90, 90);
static const QColor tabBarTabLineSelected() { return isDarkMode() ? darkTabBarTabLineSelected : lightTabBarTabLineSelected; }

static const QColor tabBarCloseButtonBackgroundHovered(162, 162, 162);
static const QColor tabBarCloseButtonBackgroundPressed(153, 153, 153);
static const QColor tabBarCloseButtonBackgroundSelectedHovered(192, 192, 192);
static const QColor tabBarCloseButtonBackgroundSelectedPressed(181, 181, 181);
static const QColor tabBarCloseButtonCross(100, 100, 100);
static const QColor tabBarCloseButtonCrossSelected(115, 115, 115);

static const int closeButtonSize = 14;
static const qreal closeButtonCornerRadius = 2.0;
#endif // QT_CONFIG(tabbar)

#ifndef QT_NO_ACCESSIBILITY // This ifdef to avoid "unused function" warning.
QBrush brushForToolButton(bool isOnKeyWindow)
{
    // When a toolbutton in a toolbar is in the 'ON' state, we draw a
    // partially transparent background. The colors must be different
    // for 'Aqua' and 'DarkAqua' appearances though.
    if (isDarkMode())
        return isOnKeyWindow ? QColor(73, 73, 73, 100) : QColor(56, 56, 56, 100);

    return isOnKeyWindow ? QColor(0, 0, 0, 28) : QColor(0, 0, 0, 21);
}
#endif // QT_NO_ACCESSIBILITY


static const int headerSectionArrowHeight = 6;
static const int headerSectionSeparatorInset = 2;

// One for each of QStyleHelper::WidgetSizePolicy
static const QMarginsF comboBoxFocusRingMargins[3] = {
    { 0.5, 2, 3.5, 4 },
    { 0.5, 1, 2.5, 4 },
    { 0.5, 1.5, 2.5, 3.5 }
};

static const QMarginsF pullDownButtonShadowMargins[3] = {
    { 0.5, -1, 0.5, 2 },
    { 0.5, -1.5, 0.5, 2.5 },
    { 0.5, 0, 0.5, 1 }
};

static const QMarginsF pushButtonShadowMargins[3] = {
    { 1.5, -1.5, 1.5, 4.5 },
    { 1.5, -1, 1.5, 4 },
    { 1.5, 0.5, 1.5, 2.5 }
};

// These are frame heights as reported by Xcode 9's Interface Builder.
// Alignemnet rectangle's heights match for push and popup buttons
// with respective values 21, 18 and 15.

static const qreal comboBoxDefaultHeight[3] = {
    26, 22, 19
};

static const qreal pushButtonDefaultHeight[3] = {
    32, 28, 16
};

static const qreal popupButtonDefaultHeight[3] = {
    26, 22, 15
};

static const int toolButtonArrowSize = 7;
static const int toolButtonArrowMargin = 2;

static const qreal focusRingWidth = 3.5;

// An application can force 'Aqua' theme while the system theme is one of
// the 'Dark' variants. Since in Qt we sometimes use NSControls and even
// NSCells directly without attaching them to any view hierarchy, we have
// to set NSAppearance.currentAppearance to 'Aqua' manually, to make sure
// the correct rendering path is triggered. Apple recommends us to un-set
// the current appearance back after we finished with drawing. This is what
// AppearanceSync is for.

class AppearanceSync {
public:
    AppearanceSync()
    {
#if QT_MACOS_PLATFORM_SDK_EQUAL_OR_ABOVE(__MAC_10_14)
        if (QOperatingSystemVersion::current() >= QOperatingSystemVersion::MacOSMojave
            && !qt_mac_applicationIsInDarkMode()) {
            auto requiredAppearanceName = NSApplication.sharedApplication.effectiveAppearance.name;
            if (![NSAppearance.currentAppearance.name isEqualToString:requiredAppearanceName]) {
                previous = NSAppearance.currentAppearance;
                NSAppearance.currentAppearance = [NSAppearance appearanceNamed:requiredAppearanceName];
            }
        }
#endif // QT_MACOS_PLATFORM_SDK_EQUAL_OR_ABOVE(__MAC_10_14)
    }

    ~AppearanceSync()
    {
        if (previous)
            NSAppearance.currentAppearance = previous;
    }

private:
    NSAppearance *previous = nil;

    Q_DISABLE_COPY(AppearanceSync)
};

static bool setupScroller(NSScroller *scroller, const QStyleOptionSlider *sb)
{
    const qreal length = sb->maximum - sb->minimum + sb->pageStep;
    if (qFuzzyIsNull(length))
        return false;
    const qreal proportion = sb->pageStep / length;
    const qreal range = qreal(sb->maximum - sb->minimum);
    qreal value = range ? qreal(sb->sliderValue - sb->minimum) / range : 0;
    if (sb->orientation == Qt::Horizontal && sb->direction == Qt::RightToLeft)
        value = 1.0 - value;

    scroller.frame = sb->rect.toCGRect();
    scroller.floatValue = value;
    scroller.knobProportion = proportion;
    return true;
}

static bool setupSlider(NSSlider *slider, const QStyleOptionSlider *sl)
{
    if (sl->minimum >= sl->maximum)
        return false;

    slider.frame = sl->rect.toCGRect();
    slider.minValue = sl->minimum;
    slider.maxValue = sl->maximum;
    slider.intValue = sl->sliderPosition;
    slider.enabled = sl->state & QStyle::State_Enabled;
    if (sl->tickPosition != QSlider::NoTicks) {
        // Set numberOfTickMarks, but TicksBothSides will be treated differently
        int interval = sl->tickInterval;
        if (interval == 0) {
            interval = sl->pageStep;
            if (interval == 0)
                interval = sl->singleStep;
            if (interval == 0)
                interval = 1; // return false?
        }
        slider.numberOfTickMarks = 1 + ((sl->maximum - sl->minimum) / interval);

        const bool ticksAbove = sl->tickPosition == QSlider::TicksAbove;
        if (sl->orientation == Qt::Horizontal)
            slider.tickMarkPosition = ticksAbove ? NSTickMarkPositionAbove : NSTickMarkPositionBelow;
        else
            slider.tickMarkPosition = ticksAbove ? NSTickMarkPositionLeading : NSTickMarkPositionTrailing;
    } else {
        slider.numberOfTickMarks = 0;
    }

    // Ensure the values set above are reflected when asking
    // the cell for its metrics and to draw itself.
    [slider layoutSubtreeIfNeeded];

    return true;
}

static bool isInMacUnifiedToolbarArea(QWindow *window, int windowY)
{
    QPlatformNativeInterface *nativeInterface = QGuiApplication::platformNativeInterface();
    QPlatformNativeInterface::NativeResourceForIntegrationFunction function =
        nativeInterface->nativeResourceFunctionForIntegration("testContentBorderPosition");
    if (!function)
        return false; // Not Cocoa platform plugin.

    typedef bool (*TestContentBorderPositionFunction)(QWindow *, int);
    return (reinterpret_cast<TestContentBorderPositionFunction>(function))(window, windowY);
}


#if QT_CONFIG(tabbar)
static void drawTabCloseButton(QPainter *p, bool hover, bool selected, bool pressed, bool documentMode)
{
    p->setRenderHints(QPainter::Antialiasing);
    QRect rect(0, 0, closeButtonSize, closeButtonSize);
    const int width = rect.width();
    const int height = rect.height();

    if (hover) {
        // draw background circle
        QColor background;
        if (selected) {
            if (documentMode)
                background = pressed ? tabBarCloseButtonBackgroundSelectedPressed : tabBarCloseButtonBackgroundSelectedHovered;
            else
                background = QColor(255, 255, 255, pressed ? 150 : 100); // Translucent white
        } else {
            background = pressed ? tabBarCloseButtonBackgroundPressed : tabBarCloseButtonBackgroundHovered;
            if (!documentMode)
                background = background.lighter(pressed ? 135 : 140); // Lighter tab background, lighter color
        }

        p->setPen(Qt::transparent);
        p->setBrush(background);
        p->drawRoundedRect(rect, closeButtonCornerRadius, closeButtonCornerRadius);
    }

    // draw cross
    const int margin = 3;
    QPen crossPen;
    crossPen.setColor(selected ? (documentMode ? tabBarCloseButtonCrossSelected : Qt::white) : tabBarCloseButtonCross);
    crossPen.setWidthF(1.1);
    crossPen.setCapStyle(Qt::FlatCap);
    p->setPen(crossPen);
    p->drawLine(margin, margin, width - margin, height - margin);
    p->drawLine(margin, height - margin, width - margin, margin);
}

QRect rotateTabPainter(QPainter *p, QTabBar::Shape shape, QRect tabRect)
{
    const auto tabDirection = QMacStylePrivate::tabDirection(shape);
    if (QMacStylePrivate::verticalTabs(tabDirection)) {
        int newX, newY, newRot;
        if (tabDirection == QMacStylePrivate::East) {
            newX = tabRect.width();
            newY = tabRect.y();
            newRot = 90;
        } else {
            newX = 0;
            newY = tabRect.y() + tabRect.height();
            newRot = -90;
        }
        tabRect.setRect(0, 0, tabRect.height(), tabRect.width());
        QTransform transform;
        transform.translate(newX, newY);
        transform.rotate(newRot);
        p->setTransform(transform, true);
    }
    return tabRect;
}

void drawTabShape(QPainter *p, const QStyleOptionTab *tabOpt, bool isUnified, int tabOverlap)
{
    QRect rect = tabOpt->rect;
    if (QMacStylePrivate::verticalTabs(QMacStylePrivate::tabDirection(tabOpt->shape)))
        rect = rect.adjusted(-tabOverlap, 0, 0, 0);
    else
        rect = rect.adjusted(0, -tabOverlap, 0, 0);

    p->translate(rect.x(), rect.y());
    rect.moveLeft(0);
    rect.moveTop(0);
    const QRect tabRect = rotateTabPainter(p, tabOpt->shape, rect);

    const int width = tabRect.width();
    const int height = tabRect.height();
    const bool active = (tabOpt->state & QStyle::State_Active);
    const bool selected = (tabOpt->state & QStyle::State_Selected);

    const QRect bodyRect(1, 2, width - 2, height - 3);
    const QRect topLineRect(1, 0, width - 2, 1);
    const QRect bottomLineRect(1, height - 1, width - 2, 1);
    if (selected) {
        // fill body
        if (tabOpt->documentMode && isUnified) {
            p->save();
            p->setCompositionMode(QPainter::CompositionMode_Source);
            p->fillRect(tabRect, QColor(Qt::transparent));
            p->restore();
        } else if (active) {
            p->fillRect(bodyRect, tabBarTabBackgroundActiveSelected());
            // top line
            p->fillRect(topLineRect, tabBarTabLineSelected());
        } else {
            p->fillRect(bodyRect, tabBarTabBackgroundSelected());
        }
    } else {
        // when the mouse is over non selected tabs they get a new color
        const bool hover = (tabOpt->state & QStyle::State_MouseOver);
        if (hover) {
            // fill body
            p->fillRect(bodyRect, tabBarTabBackgroundActiveHovered());
            // bottom line
            p->fillRect(bottomLineRect, isDarkMode() ? QColor(Qt::black) : tabBarTabLineActiveHovered());
        }
    }

    // separator lines between tabs
    const QRect leftLineRect(0, 1, 1, height - 2);
    const QRect rightLineRect(width - 1, 1, 1, height - 2);
    const QColor separatorLineColor = active ? tabBarTabLineActive() : tabBarTabLine();
    p->fillRect(leftLineRect, separatorLineColor);
    p->fillRect(rightLineRect, separatorLineColor);
}

void drawTabBase(QPainter *p, const QStyleOptionTabBarBase *tbb, const QWidget *w)
{
    QRect r = tbb->rect;
    if (QMacStylePrivate::verticalTabs(QMacStylePrivate::tabDirection(tbb->shape)))
        r.setWidth(w->width());
    else
        r.setHeight(w->height());

    const QRect tabRect = rotateTabPainter(p, tbb->shape, r);
    const int width = tabRect.width();
    const int height = tabRect.height();
    const bool active = (tbb->state & QStyle::State_Active);

    // fill body
    const QRect bodyRect(0, 1, width, height - 1);
    const QColor bodyColor = active ? tabBarTabBackgroundActive() : tabBarTabBackground();
    p->fillRect(bodyRect, bodyColor);

    // top line
    const QRect topLineRect(0, 0, width, 1);
    const QColor topLineColor = active ? tabBarTabLineActive() : tabBarTabLine();
    p->fillRect(topLineRect, topLineColor);

    // bottom line
    const QRect bottomLineRect(0, height - 1, width, 1);
    bool isDocument = false;
    if (const QTabBar *tabBar = qobject_cast<const QTabBar*>(w))
        isDocument = tabBar->documentMode();
    const QColor bottomLineColor = isDocument && isDarkMode() ? QColor(Qt::black) : active ? tabBarTabLineActive() : tabBarTabLine();
    p->fillRect(bottomLineRect, bottomLineColor);
}
#endif

static QStyleHelper::WidgetSizePolicy getControlSize(const QStyleOption *option, const QWidget *widget)
{
    const auto wsp = QStyleHelper::widgetSizePolicy(widget, option);
    if (wsp == QStyleHelper::SizeDefault)
        return QStyleHelper::SizeLarge;

    return wsp;
}

#if QT_CONFIG(treeview)
static inline bool isTreeView(const QWidget *widget)
{
    return (widget && widget->parentWidget() &&
            qobject_cast<const QTreeView *>(widget->parentWidget()));
}
#endif

static QString qt_mac_removeMnemonics(const QString &original)
{
    QString returnText(original.size(), 0);
    int finalDest = 0;
    int currPos = 0;
    int l = original.length();
    while (l) {
        if (original.at(currPos) == QLatin1Char('&')) {
            ++currPos;
            --l;
            if (l == 0)
                break;
        } else if (original.at(currPos) == QLatin1Char('(') && l >= 4 &&
                   original.at(currPos + 1) == QLatin1Char('&') &&
                   original.at(currPos + 2) != QLatin1Char('&') &&
                   original.at(currPos + 3) == QLatin1Char(')')) {
            /* remove mnemonics its format is "\s*(&X)" */
            int n = 0;
            while (finalDest > n && returnText.at(finalDest - n - 1).isSpace())
                ++n;
            finalDest -= n;
            currPos += 4;
            l -= 4;
            continue;
        }
        returnText[finalDest] = original.at(currPos);
        ++currPos;
        ++finalDest;
        --l;
    }
    returnText.truncate(finalDest);
    return returnText;
}

static bool qt_macWindowMainWindow(const QWidget *window)
{
    if (QWindow *w = window->windowHandle()) {
        if (w->handle()) {
            if (NSWindow *nswindow = static_cast<NSWindow*>(QGuiApplication::platformNativeInterface()->nativeResourceForWindow(QByteArrayLiteral("nswindow"), w))) {
                return [nswindow isMainWindow];
            }
        }
    }
    return false;
}

/*****************************************************************************
  QMacCGStyle globals
 *****************************************************************************/
const int macItemFrame         = 2;    // menu item frame width
const int macItemHMargin       = 3;    // menu item hor text margin
const int macRightBorder       = 12;   // right border on mac

/*****************************************************************************
  QMacCGStyle utility functions
 *****************************************************************************/

enum QAquaMetric {
    // Prepend kThemeMetric to get the HIToolBox constant.
    // Represents the values already used in QMacStyle.
    CheckBoxHeight = 0,
    CheckBoxWidth,
    EditTextFrameOutset,
    FocusRectOutset,
    HSliderHeight,
    HSliderTickHeight,
    LargeProgressBarThickness,
    ListHeaderHeight,
    MenuSeparatorHeight, // GetThemeMenuSeparatorHeight
    MiniCheckBoxHeight,
    MiniCheckBoxWidth,
    MiniHSliderHeight,
    MiniHSliderTickHeight,
    MiniPopupButtonHeight,
    MiniPushButtonHeight,
    MiniRadioButtonHeight,
    MiniRadioButtonWidth,
    MiniVSliderTickWidth,
    MiniVSliderWidth,
    NormalProgressBarThickness,
    PopupButtonHeight,
    ProgressBarShadowOutset,
    PushButtonHeight,
    RadioButtonHeight,
    RadioButtonWidth,
    SeparatorSize,
    SmallCheckBoxHeight,
    SmallCheckBoxWidth,
    SmallHSliderHeight,
    SmallHSliderTickHeight,
    SmallPopupButtonHeight,
    SmallProgressBarShadowOutset,
    SmallPushButtonHeight,
    SmallRadioButtonHeight,
    SmallRadioButtonWidth,
    SmallVSliderTickWidth,
    SmallVSliderWidth,
    VSliderTickWidth,
    VSliderWidth
};

static const int qt_mac_aqua_metrics[] = {
    // Values as of macOS 10.12.4 and Xcode 8.3.1
    18 /* CheckBoxHeight */,
    18 /* CheckBoxWidth */,
    1  /* EditTextFrameOutset */,
    4  /* FocusRectOutset */,
    22 /* HSliderHeight */,
    5  /* HSliderTickHeight */,
    16 /* LargeProgressBarThickness */,
    17 /* ListHeaderHeight */,
    12 /* MenuSeparatorHeight, aka GetThemeMenuSeparatorHeight */,
    11 /* MiniCheckBoxHeight */,
    10 /* MiniCheckBoxWidth */,
    12 /* MiniHSliderHeight */,
    4  /* MiniHSliderTickHeight */,
    15 /* MiniPopupButtonHeight */,
    16 /* MiniPushButtonHeight */,
    11 /* MiniRadioButtonHeight */,
    10 /* MiniRadioButtonWidth */,
    4  /* MiniVSliderTickWidth */,
    12 /* MiniVSliderWidth */,
    12 /* NormalProgressBarThickness */,
    20 /* PopupButtonHeight */,
    4  /* ProgressBarShadowOutset */,
    20 /* PushButtonHeight */,
    18 /* RadioButtonHeight */,
    18 /* RadioButtonWidth */,
    1  /* SeparatorSize */,
    16 /* SmallCheckBoxHeight */,
    14 /* SmallCheckBoxWidth */,
    15 /* SmallHSliderHeight */,
    4  /* SmallHSliderTickHeight */,
    17 /* SmallPopupButtonHeight */,
    2  /* SmallProgressBarShadowOutset */,
    17 /* SmallPushButtonHeight */,
    15 /* SmallRadioButtonHeight */,
    14 /* SmallRadioButtonWidth */,
    4  /* SmallVSliderTickWidth */,
    15 /* SmallVSliderWidth */,
    5  /* VSliderTickWidth */,
    22 /* VSliderWidth */
};

static inline int qt_mac_aqua_get_metric(QAquaMetric m)
{
    return qt_mac_aqua_metrics[m];
}

static QSize qt_aqua_get_known_size(QStyle::ContentsType ct, const QWidget *widg, QSize szHint,
                                    QStyleHelper::WidgetSizePolicy sz)
{
    QSize ret(-1, -1);
    if (sz != QStyleHelper::SizeSmall && sz != QStyleHelper::SizeLarge && sz != QStyleHelper::SizeMini) {
        qDebug("Not sure how to return this...");
        return ret;
    }
    if ((widg && widg->testAttribute(Qt::WA_SetFont)) || !QApplication::desktopSettingsAware()) {
        // If you're using a custom font and it's bigger than the default font,
        // then no constraints for you. If you are smaller, we can try to help you out
        QFont font = qt_app_fonts_hash()->value(widg->metaObject()->className(), QFont());
        if (widg->font().pointSize() > font.pointSize())
            return ret;
    }

    if (ct == QStyle::CT_CustomBase && widg) {
#if QT_CONFIG(pushbutton)
        if (qobject_cast<const QPushButton *>(widg))
            ct = QStyle::CT_PushButton;
#endif
        else if (qobject_cast<const QRadioButton *>(widg))
            ct = QStyle::CT_RadioButton;
#if QT_CONFIG(checkbox)
        else if (qobject_cast<const QCheckBox *>(widg))
            ct = QStyle::CT_CheckBox;
#endif
#if QT_CONFIG(combobox)
        else if (qobject_cast<const QComboBox *>(widg))
            ct = QStyle::CT_ComboBox;
#endif
#if QT_CONFIG(toolbutton)
        else if (qobject_cast<const QToolButton *>(widg))
            ct = QStyle::CT_ToolButton;
#endif
        else if (qobject_cast<const QSlider *>(widg))
            ct = QStyle::CT_Slider;
#if QT_CONFIG(progressbar)
        else if (qobject_cast<const QProgressBar *>(widg))
            ct = QStyle::CT_ProgressBar;
#endif
#if QT_CONFIG(lineedit)
        else if (qobject_cast<const QLineEdit *>(widg))
            ct = QStyle::CT_LineEdit;
#endif
#if QT_CONFIG(itemviews)
        else if (qobject_cast<const QHeaderView *>(widg))
            ct = QStyle::CT_HeaderSection;
#endif
#if QT_CONFIG(menubar)
        else if (qobject_cast<const QMenuBar *>(widg))
            ct = QStyle::CT_MenuBar;
#endif
#if QT_CONFIG(sizegrip)
        else if (qobject_cast<const QSizeGrip *>(widg))
            ct = QStyle::CT_SizeGrip;
#endif
        else
            return ret;
    }

    switch (ct) {
#if QT_CONFIG(pushbutton)
    case QStyle::CT_PushButton: {
        const QPushButton *psh = qobject_cast<const QPushButton *>(widg);
        // If this comparison is false, then the widget was not a push button.
        // This is bad and there's very little we can do since we were requested to find a
        // sensible size for a widget that pretends to be a QPushButton but is not.
        if(psh) {
            QString buttonText = qt_mac_removeMnemonics(psh->text());
            if (buttonText.contains(QLatin1Char('\n')))
                ret = QSize(-1, -1);
            else if (sz == QStyleHelper::SizeLarge)
                ret = QSize(-1, qt_mac_aqua_get_metric(PushButtonHeight));
            else if (sz == QStyleHelper::SizeSmall)
                ret = QSize(-1, qt_mac_aqua_get_metric(SmallPushButtonHeight));
            else if (sz == QStyleHelper::SizeMini)
                ret = QSize(-1, qt_mac_aqua_get_metric(MiniPushButtonHeight));

            if (!psh->icon().isNull()){
                // If the button got an icon, and the icon is larger than the
                // button, we can't decide on a default size
                ret.setWidth(-1);
                if (ret.height() < psh->iconSize().height())
                    ret.setHeight(-1);
            }
            else if (buttonText == QLatin1String("OK") || buttonText == QLatin1String("Cancel")){
                // Aqua Style guidelines restrict the size of OK and Cancel buttons to 68 pixels.
                // However, this doesn't work for German, therefore only do it for English,
                // I suppose it would be better to do some sort of lookups for languages
                // that like to have really long words.
                // FIXME This is not exactly true. Out of context, OK buttons have their
                // implicit size calculated the same way as any other button. Inside a
                // QDialogButtonBox, their size should be calculated such that the action
                // or accept button (i.e., rightmost) and cancel button have the same width.
                ret.setWidth(69);
            }
        } else {
            // The only sensible thing to do is to return whatever the style suggests...
            if (sz == QStyleHelper::SizeLarge)
                ret = QSize(-1, qt_mac_aqua_get_metric(PushButtonHeight));
            else if (sz == QStyleHelper::SizeSmall)
                ret = QSize(-1, qt_mac_aqua_get_metric(SmallPushButtonHeight));
            else if (sz == QStyleHelper::SizeMini)
                ret = QSize(-1, qt_mac_aqua_get_metric(MiniPushButtonHeight));
            else
                // Since there's no default size we return the large size...
                ret = QSize(-1, qt_mac_aqua_get_metric(PushButtonHeight));
         }
#endif
#if 0 //Not sure we are applying the rules correctly for RadioButtons/CheckBoxes --Sam
    } else if (ct == QStyle::CT_RadioButton) {
        QRadioButton *rdo = static_cast<QRadioButton *>(widg);
        // Exception for case where multiline radio button text requires no size constrainment
        if (rdo->text().find('\n') != -1)
            return ret;
        if (sz == QStyleHelper::SizeLarge)
            ret = QSize(-1, qt_mac_aqua_get_metric(RadioButtonHeight));
        else if (sz == QStyleHelper::SizeSmall)
            ret = QSize(-1, qt_mac_aqua_get_metric(SmallRadioButtonHeight));
        else if (sz == QStyleHelper::SizeMini)
            ret = QSize(-1, qt_mac_aqua_get_metric(MiniRadioButtonHeight));
    } else if (ct == QStyle::CT_CheckBox) {
        if (sz == QStyleHelper::SizeLarge)
            ret = QSize(-1, qt_mac_aqua_get_metric(CheckBoxHeight));
        else if (sz == QStyleHelper::SizeSmall)
            ret = QSize(-1, qt_mac_aqua_get_metric(SmallCheckBoxHeight));
        else if (sz == QStyleHelper::SizeMini)
            ret = QSize(-1, qt_mac_aqua_get_metric(MiniCheckBoxHeight));
#endif
        break;
    }
    case QStyle::CT_SizeGrip:
        // Not HIG kosher: mimic what we were doing earlier until we support 4-edge resizing in MDI subwindows
        if (sz == QStyleHelper::SizeLarge || sz == QStyleHelper::SizeSmall) {
            int s = sz == QStyleHelper::SizeSmall ? 16 : 22; // large: pixel measured from HITheme, small: from my hat
            int width = 0;
#if QT_CONFIG(mdiarea)
            if (widg && qobject_cast<QMdiSubWindow *>(widg->parentWidget()))
                width = s;
#endif
            ret = QSize(width, s);
        }
        break;
    case QStyle::CT_ComboBox:
        switch (sz) {
        case QStyleHelper::SizeLarge:
            ret = QSize(-1, qt_mac_aqua_get_metric(PopupButtonHeight));
            break;
        case QStyleHelper::SizeSmall:
            ret = QSize(-1, qt_mac_aqua_get_metric(SmallPopupButtonHeight));
            break;
        case QStyleHelper::SizeMini:
            ret = QSize(-1, qt_mac_aqua_get_metric(MiniPopupButtonHeight));
            break;
        default:
            break;
        }
        break;
    case QStyle::CT_ToolButton:
        if (sz == QStyleHelper::SizeSmall) {
            int width = 0, height = 0;
            if (szHint == QSize(-1, -1)) { //just 'guess'..
#if QT_CONFIG(toolbutton)
                const QToolButton *bt = qobject_cast<const QToolButton *>(widg);
                // If this conversion fails then the widget was not what it claimed to be.
                if(bt) {
                    if (!bt->icon().isNull()) {
                        QSize iconSize = bt->iconSize();
                        QSize pmSize = bt->icon().actualSize(QSize(32, 32), QIcon::Normal);
                        width = qMax(width, qMax(iconSize.width(), pmSize.width()));
                        height = qMax(height, qMax(iconSize.height(), pmSize.height()));
                    }
                    if (!bt->text().isNull() && bt->toolButtonStyle() != Qt::ToolButtonIconOnly) {
                        int text_width = bt->fontMetrics().horizontalAdvance(bt->text()),
                           text_height = bt->fontMetrics().height();
                        if (bt->toolButtonStyle() == Qt::ToolButtonTextUnderIcon) {
                            width = qMax(width, text_width);
                            height += text_height;
                        } else {
                            width += text_width;
                            width = qMax(height, text_height);
                        }
                    }
                } else
#endif
                {
                    // Let's return the size hint...
                    width = szHint.width();
                    height = szHint.height();
                }
            } else {
                width = szHint.width();
                height = szHint.height();
            }
            width =  qMax(20, width +  5); //border
            height = qMax(20, height + 5); //border
            ret = QSize(width, height);
        }
        break;
    case QStyle::CT_Slider: {
        int w = -1;
        const QSlider *sld = qobject_cast<const QSlider *>(widg);
        // If this conversion fails then the widget was not what it claimed to be.
        if(sld) {
            if (sz == QStyleHelper::SizeLarge) {
                if (sld->orientation() == Qt::Horizontal) {
                    w = qt_mac_aqua_get_metric(HSliderHeight);
                    if (sld->tickPosition() != QSlider::NoTicks)
                        w += qt_mac_aqua_get_metric(HSliderTickHeight);
                } else {
                    w = qt_mac_aqua_get_metric(VSliderWidth);
                    if (sld->tickPosition() != QSlider::NoTicks)
                        w += qt_mac_aqua_get_metric(VSliderTickWidth);
                }
            } else if (sz == QStyleHelper::SizeSmall) {
                if (sld->orientation() == Qt::Horizontal) {
                    w = qt_mac_aqua_get_metric(SmallHSliderHeight);
                    if (sld->tickPosition() != QSlider::NoTicks)
                        w += qt_mac_aqua_get_metric(SmallHSliderTickHeight);
                } else {
                    w = qt_mac_aqua_get_metric(SmallVSliderWidth);
                    if (sld->tickPosition() != QSlider::NoTicks)
                        w += qt_mac_aqua_get_metric(SmallVSliderTickWidth);
                }
            } else if (sz == QStyleHelper::SizeMini) {
                if (sld->orientation() == Qt::Horizontal) {
                    w = qt_mac_aqua_get_metric(MiniHSliderHeight);
                    if (sld->tickPosition() != QSlider::NoTicks)
                        w += qt_mac_aqua_get_metric(MiniHSliderTickHeight);
                } else {
                    w = qt_mac_aqua_get_metric(MiniVSliderWidth);
                    if (sld->tickPosition() != QSlider::NoTicks)
                        w += qt_mac_aqua_get_metric(MiniVSliderTickWidth);
                }
            }
        } else {
            // This is tricky, we were requested to find a size for a slider which is not
            // a slider. We don't know if this is vertical or horizontal or if we need to
            // have tick marks or not.
            // For this case we will return an horizontal slider without tick marks.
            w = qt_mac_aqua_get_metric(HSliderHeight);
            w += qt_mac_aqua_get_metric(HSliderTickHeight);
        }
        if (sld->orientation() == Qt::Horizontal)
            ret.setHeight(w);
        else
            ret.setWidth(w);
        break;
    }
#if QT_CONFIG(progressbar)
    case QStyle::CT_ProgressBar: {
        int finalValue = -1;
        Qt::Orientation orient = Qt::Horizontal;
        if (const QProgressBar *pb = qobject_cast<const QProgressBar *>(widg))
            orient = pb->orientation();

        if (sz == QStyleHelper::SizeLarge)
            finalValue = qt_mac_aqua_get_metric(LargeProgressBarThickness)
                            + qt_mac_aqua_get_metric(ProgressBarShadowOutset);
        else
            finalValue = qt_mac_aqua_get_metric(NormalProgressBarThickness)
                            + qt_mac_aqua_get_metric(SmallProgressBarShadowOutset);
        if (orient == Qt::Horizontal)
            ret.setHeight(finalValue);
        else
            ret.setWidth(finalValue);
        break;
    }
#endif
#if QT_CONFIG(combobox)
    case QStyle::CT_LineEdit:
        if (!widg || !qobject_cast<QComboBox *>(widg->parentWidget())) {
            //should I take into account the font dimentions of the lineedit? -Sam
            if (sz == QStyleHelper::SizeLarge)
                ret = QSize(-1, 21);
            else
                ret = QSize(-1, 19);
        }
        break;
#endif
    case QStyle::CT_HeaderSection:
#if QT_CONFIG(treeview)
        if (isTreeView(widg))
           ret = QSize(-1, qt_mac_aqua_get_metric(ListHeaderHeight));
#endif
        break;
    case QStyle::CT_MenuBar:
        if (sz == QStyleHelper::SizeLarge) {
            ret = QSize(-1, [[NSApp mainMenu] menuBarHeight]);
            // In the qt_mac_set_native_menubar(false) case,
            // we come it here with a zero-height main menu,
            // preventing the in-window menu from displaying.
            // Use 22 pixels for the height, by observation.
            if (ret.height() <= 0)
                ret.setHeight(22);
        }
        break;
    default:
        break;
    }
    return ret;
}


#if defined(QMAC_QAQUASTYLE_SIZE_CONSTRAIN) || defined(DEBUG_SIZE_CONSTRAINT)
static QStyleHelper::WidgetSizePolicy qt_aqua_guess_size(const QWidget *widg, QSize large, QSize small, QSize mini)
{
    Q_UNUSED(widg);

    if (large == QSize(-1, -1)) {
        if (small != QSize(-1, -1))
            return QStyleHelper::SizeSmall;
        if (mini != QSize(-1, -1))
            return QStyleHelper::SizeMini;
        return QStyleHelper::SizeDefault;
    } else if (small == QSize(-1, -1)) {
        if (mini != QSize(-1, -1))
            return QStyleHelper::SizeMini;
        return QStyleHelper::SizeLarge;
    } else if (mini == QSize(-1, -1)) {
        return QStyleHelper::SizeLarge;
    }

    if (qEnvironmentVariableIsSet("QWIDGET_ALL_SMALL"))
        return QStyleHelper::SizeSmall;
    else if (qEnvironmentVariableIsSet("QWIDGET_ALL_MINI"))
        return QStyleHelper::SizeMini;

    return QStyleHelper::SizeLarge;
}
#endif

void QMacStylePrivate::drawFocusRing(QPainter *p, const QRectF &targetRect, int hMargin, int vMargin, const CocoaControl &cw) const
{
    QPainterPath focusRingPath;
    focusRingPath.setFillRule(Qt::OddEvenFill);

    qreal hOffset = 0.0;
    qreal vOffset = 0.0;
    switch (cw.type) {
    case Box:
    case Button_SquareButton:
    case SegmentedControl_Middle:
    case TextField: {
        auto innerRect = targetRect;
        if (cw.type == TextField)
            innerRect = innerRect.adjusted(hMargin, vMargin, -hMargin, -vMargin).adjusted(0.5, 0.5, -0.5, -0.5);
        const auto outerRect = innerRect.adjusted(-focusRingWidth, -focusRingWidth, focusRingWidth, focusRingWidth);
        const auto outerRadius = focusRingWidth;
        focusRingPath.addRect(innerRect);
        focusRingPath.addRoundedRect(outerRect, outerRadius, outerRadius);
        break;
    }
    case Button_CheckBox: {
        const auto cbInnerRadius = (cw.size == QStyleHelper::SizeMini ? 2.0 : 3.0);
        const auto cbSize = cw.size == QStyleHelper::SizeLarge ? 13 :
                            cw.size == QStyleHelper::SizeSmall ? 11 : 9; // As measured
        hOffset = hMargin + (cw.size == QStyleHelper::SizeLarge ? 2.5 :
                             cw.size == QStyleHelper::SizeSmall ? 2.0 : 1.0); // As measured
        vOffset = 0.5 * qreal(targetRect.height() - cbSize);
        const auto cbInnerRect = QRectF(0, 0, cbSize, cbSize);
        const auto cbOuterRadius = cbInnerRadius + focusRingWidth;
        const auto cbOuterRect = cbInnerRect.adjusted(-focusRingWidth, -focusRingWidth, focusRingWidth, focusRingWidth);
        focusRingPath.addRoundedRect(cbOuterRect, cbOuterRadius, cbOuterRadius);
        focusRingPath.addRoundedRect(cbInnerRect, cbInnerRadius, cbInnerRadius);
        break;
    }
    case Button_RadioButton: {
        const auto rbSize = cw.size == QStyleHelper::SizeLarge ? 15 :
                            cw.size == QStyleHelper::SizeSmall ? 13 : 9; // As measured
        hOffset = hMargin + (cw.size == QStyleHelper::SizeLarge ? 1.5 :
                             cw.size == QStyleHelper::SizeSmall ? 1.0 : 1.0); // As measured
        vOffset = 0.5 * qreal(targetRect.height() - rbSize);
        const auto rbInnerRect = QRectF(0, 0, rbSize, rbSize);
        const auto rbOuterRect = rbInnerRect.adjusted(-focusRingWidth, -focusRingWidth, focusRingWidth, focusRingWidth);
        focusRingPath.addEllipse(rbInnerRect);
        focusRingPath.addEllipse(rbOuterRect);
        break;
    }
    case Button_PopupButton:
    case Button_PullDown:
    case Button_PushButton:
    case SegmentedControl_Single: {
        const qreal innerRadius = cw.type == Button_PushButton ? 3 : 4;
        const qreal outerRadius = innerRadius + focusRingWidth;
        hOffset = targetRect.left();
        vOffset = targetRect.top();
        const auto innerRect = targetRect.translated(-targetRect.topLeft());
        const auto outerRect = innerRect.adjusted(-hMargin, -vMargin, hMargin, vMargin);
        focusRingPath.addRoundedRect(innerRect, innerRadius, innerRadius);
        focusRingPath.addRoundedRect(outerRect, outerRadius, outerRadius);
        break;
    }
    case ComboBox:
    case SegmentedControl_First:
    case SegmentedControl_Last: {
        hOffset = targetRect.left();
        vOffset = targetRect.top();
        const qreal innerRadius = 8;
        const qreal outerRadius = innerRadius + focusRingWidth;
        const auto innerRect = targetRect.translated(-targetRect.topLeft());
        const auto outerRect = innerRect.adjusted(-hMargin, -vMargin, hMargin, vMargin);

        const auto cbFocusFramePath = [](const QRectF &rect, qreal tRadius, qreal bRadius) {
            QPainterPath path;

            if (tRadius > 0) {
                const auto topLeftCorner = QRectF(rect.topLeft(), QSizeF(tRadius, tRadius));
                path.arcMoveTo(topLeftCorner, 180);
                path.arcTo(topLeftCorner, 180, -90);
            } else {
                path.moveTo(rect.topLeft());
            }
            const auto rightEdge = rect.right() - bRadius;
            path.arcTo(rightEdge, rect.top(), bRadius, bRadius, 90, -90);
            path.arcTo(rightEdge, rect.bottom() - bRadius, bRadius, bRadius, 0, -90);
            if (tRadius > 0)
                path.arcTo(rect.left(), rect.bottom() - tRadius, tRadius, tRadius, 270, -90);
            else
                path.lineTo(rect.bottomLeft());
            path.closeSubpath();

            return path;
        };

        const auto innerPath = cbFocusFramePath(innerRect, 0, innerRadius);
        focusRingPath.addPath(innerPath);
        const auto outerPath = cbFocusFramePath(outerRect, 2 * focusRingWidth, outerRadius);
        focusRingPath.addPath(outerPath);
        break;
    }
    default:
        Q_UNREACHABLE();
    }

    auto focusRingColor = qt_mac_toQColor(NSColor.keyboardFocusIndicatorColor.CGColor);
    if (!qt_mac_applicationIsInDarkMode()) {
        // This color already has alpha ~ 0.25, this value is too small - the ring is
        // very pale and nothing like the native one. 0.39 makes it better (not ideal
        // anyway). The color seems to be correct in dark more without any modification.
        focusRingColor.setAlphaF(0.39);
    }

    p->save();
    p->setRenderHint(QPainter::Antialiasing);

    if (cw.type == SegmentedControl_First) {
        // TODO Flip left-right
    }
    p->translate(hOffset, vOffset);
    p->fillPath(focusRingPath, focusRingColor);
    p->restore();
}

QPainterPath QMacStylePrivate::windowPanelPath(const QRectF &r) const
{
    static const qreal CornerPointOffset = 5.5;
    static const qreal CornerControlOffset = 2.1;

    QPainterPath path;
    // Top-left corner
    path.moveTo(r.left(), r.top() + CornerPointOffset);
    path.cubicTo(r.left(), r.top() + CornerControlOffset,
                 r.left() + CornerControlOffset, r.top(),
                 r.left() + CornerPointOffset, r.top());
    // Top-right corner
    path.lineTo(r.right() - CornerPointOffset, r.top());
    path.cubicTo(r.right() - CornerControlOffset, r.top(),
                 r.right(), r.top() + CornerControlOffset,
                 r.right(), r.top() + CornerPointOffset);
    // Bottom-right corner
    path.lineTo(r.right(), r.bottom() - CornerPointOffset);
    path.cubicTo(r.right(), r.bottom() - CornerControlOffset,
                 r.right() - CornerControlOffset, r.bottom(),
                 r.right() - CornerPointOffset, r.bottom());
    // Bottom-right corner
    path.lineTo(r.left() + CornerPointOffset, r.bottom());
    path.cubicTo(r.left() + CornerControlOffset, r.bottom(),
                 r.left(), r.bottom() - CornerControlOffset,
                 r.left(), r.bottom() - CornerPointOffset);
    path.lineTo(r.left(), r.top() + CornerPointOffset);

    return path;
}

QMacStylePrivate::CocoaControlType QMacStylePrivate::windowButtonCocoaControl(QStyle::SubControl sc) const
{
    struct WindowButtons {
        QStyle::SubControl sc;
        QMacStylePrivate::CocoaControlType ct;
    };

    static const WindowButtons buttons[] = {
        { QStyle::SC_TitleBarCloseButton, QMacStylePrivate::Button_WindowClose },
        { QStyle::SC_TitleBarMinButton,   QMacStylePrivate::Button_WindowMiniaturize },
        { QStyle::SC_TitleBarMaxButton,   QMacStylePrivate::Button_WindowZoom }
    };

    for (const auto &wb : buttons)
        if (wb.sc == sc)
            return wb.ct;

    return NoControl;
}


#if QT_CONFIG(tabbar)
void QMacStylePrivate::tabLayout(const QStyleOptionTab *opt, const QWidget *widget, QRect *textRect, QRect *iconRect) const
{
    Q_ASSERT(textRect);
    Q_ASSERT(iconRect);
    QRect tr = opt->rect;
    const bool verticalTabs = opt->shape == QTabBar::RoundedEast
                              || opt->shape == QTabBar::RoundedWest
                              || opt->shape == QTabBar::TriangularEast
                              || opt->shape == QTabBar::TriangularWest;
    if (verticalTabs)
        tr.setRect(0, 0, tr.height(), tr.width()); // 0, 0 as we will have a translate transform

    int verticalShift = proxyStyle->pixelMetric(QStyle::PM_TabBarTabShiftVertical, opt, widget);
    int horizontalShift = proxyStyle->pixelMetric(QStyle::PM_TabBarTabShiftHorizontal, opt, widget);
    const int hpadding = 4;
    const int vpadding = proxyStyle->pixelMetric(QStyle::PM_TabBarTabVSpace, opt, widget) / 2;
    if (opt->shape == QTabBar::RoundedSouth || opt->shape == QTabBar::TriangularSouth)
        verticalShift = -verticalShift;
    tr.adjust(hpadding, verticalShift - vpadding, horizontalShift - hpadding, vpadding);

    // left widget
    if (!opt->leftButtonSize.isEmpty()) {
        const int buttonSize = verticalTabs ? opt->leftButtonSize.height() : opt->leftButtonSize.width();
        tr.setLeft(tr.left() + 4 + buttonSize);
        // make text aligned to center
        if (opt->rightButtonSize.isEmpty())
            tr.setRight(tr.right() - 4 - buttonSize);
    }
    // right widget
    if (!opt->rightButtonSize.isEmpty()) {
        const int buttonSize = verticalTabs ? opt->rightButtonSize.height() : opt->rightButtonSize.width();
        tr.setRight(tr.right() - 4 - buttonSize);
        // make text aligned to center
        if (opt->leftButtonSize.isEmpty())
            tr.setLeft(tr.left() + 4 + buttonSize);
    }

    // icon
    if (!opt->icon.isNull()) {
        QSize iconSize = opt->iconSize;
        if (!iconSize.isValid()) {
            int iconExtent = proxyStyle->pixelMetric(QStyle::PM_SmallIconSize);
            iconSize = QSize(iconExtent, iconExtent);
        }
        QSize tabIconSize = opt->icon.actualSize(iconSize,
                        (opt->state & QStyle::State_Enabled) ? QIcon::Normal : QIcon::Disabled,
                        (opt->state & QStyle::State_Selected) ? QIcon::On : QIcon::Off);
        // High-dpi icons do not need adjustment; make sure tabIconSize is not larger than iconSize
        tabIconSize = QSize(qMin(tabIconSize.width(), iconSize.width()), qMin(tabIconSize.height(), iconSize.height()));

        const int stylePadding = proxyStyle->pixelMetric(QStyle::PM_TabBarTabHSpace, opt, widget) / 2 - hpadding;

        if (opt->documentMode) {
            // documents show the icon as part of the the text
            const int textWidth =
                opt->fontMetrics.boundingRect(tr, Qt::AlignCenter | Qt::TextShowMnemonic, opt->text).width();
            *iconRect = QRect(tr.center().x() - textWidth / 2 - stylePadding - tabIconSize.width(),
                              tr.center().y() - tabIconSize.height() / 2,
                              tabIconSize.width(), tabIconSize.height());
        } else {
            *iconRect = QRect(tr.left() + stylePadding, tr.center().y() - tabIconSize.height() / 2,
                        tabIconSize.width(), tabIconSize.height());
        }
        if (!verticalTabs)
            *iconRect = proxyStyle->visualRect(opt->direction, opt->rect, *iconRect);

        tr.setLeft(tr.left() + stylePadding + tabIconSize.width() + 4);
        tr.setRight(tr.right() - stylePadding - tabIconSize.width() - 4);
    }

    if (!verticalTabs)
        tr = proxyStyle->visualRect(opt->direction, opt->rect, tr);

    *textRect = tr;
}

QMacStylePrivate::Direction QMacStylePrivate::tabDirection(QTabBar::Shape shape)
{
    switch (shape) {
    case QTabBar::RoundedSouth:
    case QTabBar::TriangularSouth:
        return South;
    case QTabBar::RoundedNorth:
    case QTabBar::TriangularNorth:
        return North;
    case QTabBar::RoundedWest:
    case QTabBar::TriangularWest:
        return West;
    case QTabBar::RoundedEast:
    case QTabBar::TriangularEast:
        return East;
    }
}

bool QMacStylePrivate::verticalTabs(QMacStylePrivate::Direction direction)
{
    return (direction == QMacStylePrivate::East
         || direction == QMacStylePrivate::West);
}

#endif // QT_CONFIG(tabbar)

QStyleHelper::WidgetSizePolicy QMacStylePrivate::effectiveAquaSizeConstrain(const QStyleOption *option,
                                                            const QWidget *widg,
                                                            QStyle::ContentsType ct,
                                                            QSize szHint, QSize *insz) const
{
    QStyleHelper::WidgetSizePolicy sz = aquaSizeConstrain(option, widg, ct, szHint, insz);
    if (sz == QStyleHelper::SizeDefault)
        return QStyleHelper::SizeLarge;
    return sz;
}

QStyleHelper::WidgetSizePolicy QMacStylePrivate::aquaSizeConstrain(const QStyleOption *option, const QWidget *widg,
                                       QStyle::ContentsType ct, QSize szHint, QSize *insz) const
{
#if defined(QMAC_QAQUASTYLE_SIZE_CONSTRAIN) || defined(DEBUG_SIZE_CONSTRAINT)
    if (option) {
        if (option->state & QStyle::State_Small)
            return QStyleHelper::SizeSmall;
        if (option->state & QStyle::State_Mini)
            return QStyleHelper::SizeMini;
    }

    if (!widg) {
        if (insz)
            *insz = QSize();
        if (qEnvironmentVariableIsSet("QWIDGET_ALL_SMALL"))
            return QStyleHelper::SizeSmall;
        if (qEnvironmentVariableIsSet("QWIDGET_ALL_MINI"))
            return QStyleHelper::SizeMini;
        return QStyleHelper::SizeDefault;
    }

    QSize large = qt_aqua_get_known_size(ct, widg, szHint, QStyleHelper::SizeLarge),
          small = qt_aqua_get_known_size(ct, widg, szHint, QStyleHelper::SizeSmall),
          mini  = qt_aqua_get_known_size(ct, widg, szHint, QStyleHelper::SizeMini);
    bool guess_size = false;
    QStyleHelper::WidgetSizePolicy ret = QStyleHelper::SizeDefault;
    QStyleHelper::WidgetSizePolicy wsp = QStyleHelper::widgetSizePolicy(widg);
    if (wsp == QStyleHelper::SizeDefault)
        guess_size = true;
    else if (wsp == QStyleHelper::SizeMini)
        ret = QStyleHelper::SizeMini;
    else if (wsp == QStyleHelper::SizeSmall)
        ret = QStyleHelper::SizeSmall;
    else if (wsp == QStyleHelper::SizeLarge)
        ret = QStyleHelper::SizeLarge;
    if (guess_size)
        ret = qt_aqua_guess_size(widg, large, small, mini);

    QSize *sz = 0;
    if (ret == QStyleHelper::SizeSmall)
        sz = &small;
    else if (ret == QStyleHelper::SizeLarge)
        sz = &large;
    else if (ret == QStyleHelper::SizeMini)
        sz = &mini;
    if (insz)
        *insz = sz ? *sz : QSize(-1, -1);
#ifdef DEBUG_SIZE_CONSTRAINT
    if (sz) {
        const char *size_desc = "Unknown";
        if (sz == &small)
            size_desc = "Small";
        else if (sz == &large)
            size_desc = "Large";
        else if (sz == &mini)
            size_desc = "Mini";
        qDebug("%s - %s: %s taken (%d, %d) [%d, %d]",
               widg ? widg->objectName().toLatin1().constData() : "*Unknown*",
               widg ? widg->metaObject()->className() : "*Unknown*", size_desc, widg->width(), widg->height(),
               sz->width(), sz->height());
    }
#endif
    return ret;
#else
    if (insz)
        *insz = QSize();
    Q_UNUSED(widg);
    Q_UNUSED(ct);
    Q_UNUSED(szHint);
    return QStyleHelper::SizeDefault;
#endif
}

uint qHash(const QMacStylePrivate::CocoaControl &cw, uint seed = 0)
{
    return ((cw.type << 2) | cw.size) ^ seed;
}

QMacStylePrivate::CocoaControl::CocoaControl()
  : type(NoControl), size(QStyleHelper::SizeDefault)
{
}

QMacStylePrivate::CocoaControl::CocoaControl(CocoaControlType t, QStyleHelper::WidgetSizePolicy s)
    : type(t), size(s)
{
}

bool QMacStylePrivate::CocoaControl::operator==(const CocoaControl &other) const
{
    return other.type == type && other.size == size;
}

QSizeF QMacStylePrivate::CocoaControl::defaultFrameSize() const
{
    // We need this because things like NSView.alignmentRectInsets
    // or -[NSCell titleRectForBounds:] won't work unless the control
    // has a reasonable frame set. IOW, it's a chicken and egg problem.
    // These values are as observed in Xcode 9's Interface Builder.

    if (type == Button_PushButton)
        return QSizeF(-1, pushButtonDefaultHeight[size]);

    if (type == Button_PopupButton
            || type == Button_PullDown)
        return QSizeF(-1, popupButtonDefaultHeight[size]);

    if (type == ComboBox)
        return QSizeF(-1, comboBoxDefaultHeight[size]);

    return QSizeF();
}

QRectF QMacStylePrivate::CocoaControl::adjustedControlFrame(const QRectF &rect) const
{
    QRectF frameRect;
    const auto frameSize = defaultFrameSize();
    if (type == QMacStylePrivate::Button_SquareButton) {
        frameRect = rect.adjusted(3, 1, -3, -1)
                        .adjusted(focusRingWidth, focusRingWidth, -focusRingWidth, -focusRingWidth);
    } else if (type == QMacStylePrivate::Button_PushButton) {
        // Start from the style option's top-left corner.
        frameRect = QRectF(rect.topLeft(),
                           QSizeF(rect.width(), frameSize.height()));
        if (size == QStyleHelper::SizeSmall)
            frameRect = frameRect.translated(0, 1.5);
        else if (size == QStyleHelper::SizeMini)
            frameRect = frameRect.adjusted(0, 0, -8, 0).translated(4, 4);
    } else {
        // Center in the style option's rect.
        frameRect = QRectF(QPointF(0, (rect.height() - frameSize.height()) / 2.0),
                           QSizeF(rect.width(), frameSize.height()));
        frameRect = frameRect.translated(rect.topLeft());
        if (type == QMacStylePrivate::Button_PullDown || type == QMacStylePrivate::Button_PopupButton) {
            if (size == QStyleHelper::SizeLarge)
                frameRect = frameRect.adjusted(0, 0, -6, 0).translated(3, 0);
            else if (size == QStyleHelper::SizeSmall)
                frameRect = frameRect.adjusted(0, 0, -4, 0).translated(2, 1);
            else if (size == QStyleHelper::SizeMini)
                frameRect = frameRect.adjusted(0, 0, -9, 0).translated(5, 0);
        } else if (type == QMacStylePrivate::ComboBox) {
            frameRect = frameRect.adjusted(0, 0, -6, 0).translated(4, 0);
        }
    }

    return frameRect;
}

QMarginsF QMacStylePrivate::CocoaControl::titleMargins() const
{
    if (type == QMacStylePrivate::Button_PushButton) {
        if (size == QStyleHelper::SizeLarge)
            return QMarginsF(12, 5, 12, 9);
        if (size == QStyleHelper::SizeSmall)
            return QMarginsF(12, 4, 12, 9);
        if (size == QStyleHelper::SizeMini)
            return QMarginsF(10, 1, 10, 2);
    }

    if (type == QMacStylePrivate::Button_PullDown) {
        if (size == QStyleHelper::SizeLarge)
            return QMarginsF(7.5, 2.5, 22.5, 5.5);
        if (size == QStyleHelper::SizeSmall)
            return QMarginsF(7.5, 2, 20.5, 4);
        if (size == QStyleHelper::SizeMini)
            return QMarginsF(4.5, 0, 16.5, 2);
    }

    if (type == QMacStylePrivate::Button_SquareButton)
        return QMarginsF(6, 1, 6, 2);

    return QMarginsF();
}

bool QMacStylePrivate::CocoaControl::getCocoaButtonTypeAndBezelStyle(NSButtonType *buttonType, NSBezelStyle *bezelStyle) const
{
    switch (type) {
    case Button_CheckBox:
        *buttonType = NSSwitchButton;
        *bezelStyle = NSRegularSquareBezelStyle;
        break;
    case Button_Disclosure:
        *buttonType = NSOnOffButton;
        *bezelStyle = NSDisclosureBezelStyle;
        break;
    case Button_RadioButton:
        *buttonType = NSRadioButton;
        *bezelStyle = NSRegularSquareBezelStyle;
        break;
    case Button_SquareButton:
        *buttonType = NSPushOnPushOffButton;
        *bezelStyle = NSShadowlessSquareBezelStyle;
        break;
    case Button_PushButton:
        *buttonType = NSPushOnPushOffButton;
        *bezelStyle = NSRoundedBezelStyle;
        break;
    default:
        return false;
    }

    return true;
}

QMacStylePrivate::CocoaControlType cocoaControlType(const QStyleOption *opt, const QWidget *w)
{
    if (const auto *btn = qstyleoption_cast<const QStyleOptionButton *>(opt)) {
        const bool hasMenu = btn->features & QStyleOptionButton::HasMenu;
        // When the contents won't fit in a large sized button,
        // and WA_MacNormalSize is not set, make the button square.
        // Threshold used to be at 34, not 32.
        const auto maxNonSquareHeight = pushButtonDefaultHeight[QStyleHelper::SizeLarge];
        const bool isSquare = (btn->features & QStyleOptionButton::Flat)
                || (btn->rect.height() > maxNonSquareHeight
                    && !(w && w->testAttribute(Qt::WA_MacNormalSize)));
        return (isSquare? QMacStylePrivate::Button_SquareButton :
                hasMenu ? QMacStylePrivate::Button_PullDown :
                QMacStylePrivate::Button_PushButton);
    }

    if (const auto *combo = qstyleoption_cast<const QStyleOptionComboBox *>(opt)) {
        if (combo->editable)
            return QMacStylePrivate::ComboBox;
        // TODO Me may support square, non-editable combo boxes, but not more than that
        return QMacStylePrivate::Button_PopupButton;
    }

    return QMacStylePrivate::NoControl;
}

/**
    Carbon draws comboboxes (and other views) outside the rect given as argument. Use this function to obtain
    the corresponding inner rect for drawing the same combobox so that it stays inside the given outerBounds.
*/
CGRect QMacStylePrivate::comboboxInnerBounds(const CGRect &outerBounds, const CocoaControl &cocoaWidget)
{
    CGRect innerBounds = outerBounds;
    // Carbon draw parts of the view outside the rect.
    // So make the rect a bit smaller to compensate
    // (I wish HIThemeGetButtonBackgroundBounds worked)
    if (cocoaWidget.type == Button_PopupButton) {
        switch (cocoaWidget.size) {
        case QStyleHelper::SizeSmall:
            innerBounds.origin.x += 3;
            innerBounds.origin.y += 3;
            innerBounds.size.width -= 6;
            innerBounds.size.height -= 7;
            break;
        case QStyleHelper::SizeMini:
            innerBounds.origin.x += 2;
            innerBounds.origin.y += 2;
            innerBounds.size.width -= 5;
            innerBounds.size.height -= 6;
            break;
        case QStyleHelper::SizeLarge:
        case QStyleHelper::SizeDefault:
            innerBounds.origin.x += 2;
            innerBounds.origin.y += 2;
            innerBounds.size.width -= 5;
            innerBounds.size.height -= 6;
        }
    } else if (cocoaWidget.type == ComboBox) {
        switch (cocoaWidget.size) {
        case QStyleHelper::SizeSmall:
            innerBounds.origin.x += 3;
            innerBounds.origin.y += 3;
            innerBounds.size.width -= 7;
            innerBounds.size.height -= 8;
            break;
        case QStyleHelper::SizeMini:
            innerBounds.origin.x += 3;
            innerBounds.origin.y += 3;
            innerBounds.size.width -= 4;
            innerBounds.size.height -= 8;
            break;
        case QStyleHelper::SizeLarge:
        case QStyleHelper::SizeDefault:
            innerBounds.origin.x += 3;
            innerBounds.origin.y += 2;
            innerBounds.size.width -= 6;
            innerBounds.size.height -= 8;
        }
    }

    return innerBounds;
}

/**
    Inside a combobox Qt places a line edit widget. The size of this widget should depend on the kind
    of combobox we choose to draw. This function calculates and returns this size.
*/
QRectF QMacStylePrivate::comboboxEditBounds(const QRectF &outerBounds, const CocoaControl &cw)
{
    QRectF ret = outerBounds;
    if (cw.type == ComboBox) {
        switch (cw.size) {
        case QStyleHelper::SizeLarge:
            ret = ret.adjusted(0, 0, -25, 0).translated(2, 4.5);
            ret.setHeight(16);
            break;
        case QStyleHelper::SizeSmall:
            ret = ret.adjusted(0, 0, -22, 0).translated(2, 3);
            ret.setHeight(14);
            break;
        case QStyleHelper::SizeMini:
            ret = ret.adjusted(0, 0, -19, 0).translated(2, 2.5);
            ret.setHeight(10.5);
            break;
        default:
            break;
        }
    } else if (cw.type == Button_PopupButton) {
        switch (cw.size) {
        case QStyleHelper::SizeLarge:
            ret.adjust(10, 1, -23, -4);
            break;
        case QStyleHelper::SizeSmall:
            ret.adjust(10, 4, -20, -3);
            break;
        case QStyleHelper::SizeMini:
            ret.adjust(9, 0, -19, 0);
            ret.setHeight(13);
            break;
        default:
            break;
        }
    }
    return ret;
}

QMacStylePrivate::QMacStylePrivate()
    : backingStoreNSView(nil)
{
    if (auto *ssf = QGuiApplicationPrivate::platformTheme()->font(QPlatformTheme::SmallFont))
        smallSystemFont = *ssf;
    if (auto *msf = QGuiApplicationPrivate::platformTheme()->font(QPlatformTheme::MiniFont))
        miniSystemFont = *msf;
}

QMacStylePrivate::~QMacStylePrivate()
{
    QMacAutoReleasePool pool;
    for (NSView *b : cocoaControls)
        [b release];
    for (NSCell *cell : cocoaCells)
        [cell release];
}

NSView *QMacStylePrivate::cocoaControl(CocoaControl widget) const
{
    if (widget.type == QMacStylePrivate::NoControl
        || widget.size == QStyleHelper::SizeDefault)
        return nil;

    if (widget.type == Box) {
        if (__builtin_available(macOS 10.14, *)) {
            if (qt_mac_applicationIsInDarkMode()) {
                // See render code in drawPrimitive(PE_FrameTabWidget)
                widget.type = Box_Dark;
            }
        }
    }

    NSView *bv = cocoaControls.value(widget, nil);
    if (!bv) {
        switch (widget.type) {
        case Box: {
            NSBox *box = [[NSBox alloc] init];
            bv = box;
            box.title = @"";
            box.titlePosition = NSNoTitle;
            break;
        }
        case Box_Dark:
            bv = [[QDarkNSBox alloc] init];
            break;
        case Button_CheckBox:
        case Button_Disclosure:
        case Button_PushButton:
        case Button_RadioButton:
        case Button_SquareButton: {
            NSButton *bc = [[NSButton alloc] init];
            bc.title = @"";
            // See below for style and bezel setting.
            bv = bc;
            break;
        }
        case Button_PopupButton:
        case Button_PullDown: {
            NSPopUpButton *bc = [[NSPopUpButton alloc] init];
            bc.title = @"";
            if (widget.type == Button_PullDown)
                bc.pullsDown = YES;
            bv = bc;
            break;
        }
        case Button_WindowClose:
        case Button_WindowMiniaturize:
        case Button_WindowZoom: {
            const NSWindowButton button = [=] {
                switch (widget.type) {
                case Button_WindowClose:
                    return NSWindowCloseButton;
                case Button_WindowMiniaturize:
                    return NSWindowMiniaturizeButton;
                case Button_WindowZoom:
                    return NSWindowZoomButton;
                default:
                    break;
                }
                Q_UNREACHABLE();
            } ();
            const auto styleMask = NSWindowStyleMaskTitled
                                 | NSWindowStyleMaskClosable
                                 | NSWindowStyleMaskMiniaturizable
                                 | NSWindowStyleMaskResizable;
            bv = [NSWindow standardWindowButton:button forStyleMask:styleMask];
            [bv retain];
            break;
        }
        case ComboBox:
            bv = [[NSComboBox alloc] init];
            break;
        case ProgressIndicator_Determinate:
            bv = [[NSProgressIndicator alloc] init];
            break;
        case ProgressIndicator_Indeterminate:
            bv = [[QIndeterminateProgressIndicator alloc] init];
            break;
        case Scroller_Horizontal:
            bv = [[NSScroller alloc] initWithFrame:NSMakeRect(0, 0, 200, 20)];
            break;
        case Scroller_Vertical:
            // Cocoa sets the orientation from the view's frame
            // at construction time, and it cannot be changed later.
            bv = [[NSScroller alloc] initWithFrame:NSMakeRect(0, 0, 20, 200)];
            break;
        case Slider_Horizontal:
            bv = [[NSSlider alloc] initWithFrame:NSMakeRect(0, 0, 200, 20)];
            break;
        case Slider_Vertical:
            // Cocoa sets the orientation from the view's frame
            // at construction time, and it cannot be changed later.
            bv = [[NSSlider alloc] initWithFrame:NSMakeRect(0, 0, 20, 200)];
            break;
        case SplitView_Horizontal:
            bv = [[NSSplitView alloc] init];
            break;
        case SplitView_Vertical:
            bv = [[QVerticalSplitView alloc] init];
            break;
        case TextField:
            bv = [[NSTextField alloc] init];
            break;
        default:
            break;
        }

        if ([bv isKindOfClass:[NSControl class]]) {
            auto *ctrl = static_cast<NSControl *>(bv);
            switch (widget.size) {
            case QStyleHelper::SizeSmall:
                ctrl.controlSize = NSControlSizeSmall;
                break;
            case QStyleHelper::SizeMini:
                ctrl.controlSize = NSControlSizeMini;
                break;
            default:
                break;
            }
        } else if (widget.type == ProgressIndicator_Determinate ||
                   widget.type == ProgressIndicator_Indeterminate) {
            auto *pi = static_cast<NSProgressIndicator *>(bv);
            pi.indeterminate = (widget.type == ProgressIndicator_Indeterminate);
            switch (widget.size) {
            case QStyleHelper::SizeSmall:
                pi.controlSize = NSControlSizeSmall;
                break;
            case QStyleHelper::SizeMini:
                pi.controlSize = NSControlSizeMini;
                break;
            default:
                break;
            }
        }

        cocoaControls.insert(widget, bv);
    }

    NSButtonType buttonType;
    NSBezelStyle bezelStyle;
    if (widget.getCocoaButtonTypeAndBezelStyle(&buttonType, &bezelStyle)) {
        // FIXME We need to reset the button's type and
        // bezel style properties, even when cached.
        auto *button = static_cast<NSButton *>(bv);
        button.buttonType = buttonType;
        button.bezelStyle = bezelStyle;
        if (widget.type == Button_CheckBox)
            button.allowsMixedState = YES;
    }

    return bv;
}

NSCell *QMacStylePrivate::cocoaCell(CocoaControl widget) const
{
    NSCell *cell = cocoaCells[widget];
    if (!cell) {
        switch (widget.type) {
        case Stepper:
            cell = [[NSStepperCell alloc] init];
            break;
        case Button_Disclosure: {
            NSButtonCell *bc = [[NSButtonCell alloc] init];
            bc.buttonType = NSOnOffButton;
            bc.bezelStyle = NSDisclosureBezelStyle;
            cell = bc;
            break;
        }
        default:
            break;
        }

        switch (widget.size) {
        case QStyleHelper::SizeSmall:
            cell.controlSize = NSControlSizeSmall;
            break;
        case QStyleHelper::SizeMini:
            cell.controlSize = NSControlSizeMini;
            break;
        default:
            break;
        }

        cocoaCells.insert(widget, cell);
    }

    return cell;
}

void QMacStylePrivate::drawNSViewInRect(NSView *view, const QRectF &rect, QPainter *p,
                                        __attribute__((noescape)) DrawRectBlock drawRectBlock) const
{
    QMacCGContext ctx(p);
    setupNSGraphicsContext(ctx, YES);

    // FIXME: The rect that we get in is relative to the widget that we're drawing
    // style on behalf of, and doesn't take into account the offset of that widget
    // to the widget that owns the backingstore, which we are placing the native
    // view into below. This means most of the views are placed in the upper left
    // corner of backingStoreNSView, which does not map to where the actual widget
    // is, and which may cause problems such as triggering a setNeedsDisplay of the
    // backingStoreNSView for the wrong rect. We work around this by making the view
    // layer-backed, which prevents triggering display of the backingStoreNSView, but
    // but there may be other issues lurking here due to the wrong position. QTBUG-68023
    view.wantsLayer = YES;

    // FIXME: We are also setting the frame of the incoming view a lot at the call
    // sites of this function, making it unclear who's actually responsible for
    // maintaining the size and position of the view. In theory the call sites
    // should ensure the _size_ of the view is correct, and then let this code
    // take care of _positioning_ the view at the right place inside backingStoreNSView.
    // For now we pass on the rect as is, to prevent any regressions until this
    // can be investigated properly.
    view.frame = rect.toCGRect();

    [backingStoreNSView addSubview:view];

    // FIXME: Based on the code below, this method isn't drawing an NSView into
    // a rect, it's drawing _part of the NSView_, defined by the incoming clip
    // or dirty rect, into the current graphics context. We're doing some manual
    // translations at the call sites that would indicate that this relationship
    // is a bit fuzzy.
    const CGRect dirtyRect = rect.toCGRect();

    if (drawRectBlock)
        drawRectBlock(ctx, dirtyRect);
    else
        [view drawRect:dirtyRect];

    [view removeFromSuperviewWithoutNeedingDisplay];

    restoreNSGraphicsContext(ctx);
}

void QMacStylePrivate::resolveCurrentNSView(QWindow *window) const
{
    backingStoreNSView = window ? (NSView *)window->winId() : nil;
}

QMacStyle::QMacStyle()
    : QCommonStyle(*new QMacStylePrivate)
{
    QMacAutoReleasePool pool;

    static QMacNotificationObserver scrollbarStyleObserver(nil,
        NSPreferredScrollerStyleDidChangeNotification, []() {
            // Purge destroyed scroll bars
            QMacStylePrivate::scrollBars.removeAll(QPointer<QObject>());

            QEvent event(QEvent::StyleChange);
            for (const auto &o : QMacStylePrivate::scrollBars)
                QCoreApplication::sendEvent(o, &event);
    });

#if QT_MACOS_PLATFORM_SDK_EQUAL_OR_ABOVE(__MAC_10_14)
    Q_D(QMacStyle);
    // FIXME: Tie this logic into theme change, or even polish/unpolish
    if (QOperatingSystemVersion::current() >= QOperatingSystemVersion::MacOSMojave) {
        d->appearanceObserver = QMacKeyValueObserver(NSApp, @"effectiveAppearance", [this] {
            Q_D(QMacStyle);
            for (NSView *b : d->cocoaControls)
                [b release];
            d->cocoaControls.clear();
        });
    }
#endif
}

QMacStyle::~QMacStyle()
{
}

void QMacStyle::polish(QPalette &)
{
}

void QMacStyle::polish(QApplication *)
{
}

void QMacStyle::unpolish(QApplication *)
{
}

void QMacStyle::polish(QWidget* w)
{
    if (false
#if QT_CONFIG(menu)
        || qobject_cast<QMenu*>(w)
#  if QT_CONFIG(combobox)
        || qobject_cast<QComboBoxPrivateContainer *>(w)
#  endif
#endif
#if QT_CONFIG(mdiarea)
        || qobject_cast<QMdiSubWindow *>(w)
#endif
        ) {
        w->setAttribute(Qt::WA_TranslucentBackground, true);
        w->setAutoFillBackground(false);
    }

#if QT_CONFIG(tabbar)
    if (QTabBar *tb = qobject_cast<QTabBar*>(w)) {
        if (tb->documentMode()) {
            w->setAttribute(Qt::WA_Hover);
            w->setFont(qt_app_fonts_hash()->value("QSmallFont", QFont()));
            QPalette p = w->palette();
            p.setColor(QPalette::WindowText, QColor(17, 17, 17));
            w->setPalette(p);
            w->setAttribute(Qt::WA_SetPalette, false);
            w->setAttribute(Qt::WA_SetFont, false);
        }
    }
#endif

    QCommonStyle::polish(w);

    if (QRubberBand *rubber = qobject_cast<QRubberBand*>(w)) {
        rubber->setWindowOpacity(0.25);
        rubber->setAttribute(Qt::WA_PaintOnScreen, false);
        rubber->setAttribute(Qt::WA_NoSystemBackground, false);
    }

    if (qobject_cast<QScrollBar*>(w)) {
        w->setAttribute(Qt::WA_OpaquePaintEvent, false);
        w->setAttribute(Qt::WA_Hover, true);
        w->setMouseTracking(true);
    }
}

void QMacStyle::unpolish(QWidget* w)
{
    if (
#if QT_CONFIG(menu)
        qobject_cast<QMenu*>(w) &&
#endif
        !w->testAttribute(Qt::WA_SetPalette))
    {
        w->setPalette(QPalette());
        w->setWindowOpacity(1.0);
    }

#if QT_CONFIG(combobox)
    if (QComboBox *combo = qobject_cast<QComboBox *>(w)) {
        if (!combo->isEditable()) {
            if (QWidget *widget = combo->findChild<QComboBoxPrivateContainer *>())
                widget->setWindowOpacity(1.0);
        }
    }
#endif

#if QT_CONFIG(tabbar)
    if (qobject_cast<QTabBar*>(w)) {
        if (!w->testAttribute(Qt::WA_SetFont))
            w->setFont(QFont());
        if (!w->testAttribute(Qt::WA_SetPalette))
            w->setPalette(QPalette());
    }
#endif

    if (QRubberBand *rubber = qobject_cast<QRubberBand*>(w)) {
        rubber->setWindowOpacity(1.0);
        rubber->setAttribute(Qt::WA_PaintOnScreen, true);
        rubber->setAttribute(Qt::WA_NoSystemBackground, true);
    }

    if (QFocusFrame *frame = qobject_cast<QFocusFrame *>(w))
        frame->setAttribute(Qt::WA_NoSystemBackground, true);

    QCommonStyle::unpolish(w);

    if (qobject_cast<QScrollBar*>(w)) {
        w->setAttribute(Qt::WA_OpaquePaintEvent, true);
        w->setAttribute(Qt::WA_Hover, false);
        w->setMouseTracking(false);
    }
}

int QMacStyle::pixelMetric(PixelMetric metric, const QStyleOption *opt, const QWidget *widget) const
{
    Q_D(const QMacStyle);
    const int controlSize = getControlSize(opt, widget);
    int ret = 0;

    switch (metric) {
#if QT_CONFIG(tabbar)
    case PM_TabCloseIndicatorWidth:
    case PM_TabCloseIndicatorHeight:
        ret = closeButtonSize;
        break;
#endif
    case PM_ToolBarIconSize:
        ret = proxy()->pixelMetric(PM_LargeIconSize);
        break;
    case PM_FocusFrameVMargin:
    case PM_FocusFrameHMargin:
        ret = qt_mac_aqua_get_metric(FocusRectOutset);
        break;
    case PM_DialogButtonsSeparator:
        ret = -5;
        break;
    case PM_DialogButtonsButtonHeight: {
        QSize sz;
        ret = d->aquaSizeConstrain(opt, 0, QStyle::CT_PushButton, QSize(-1, -1), &sz);
        if (sz == QSize(-1, -1))
            ret = 32;
        else
            ret = sz.height();
        break; }
    case PM_DialogButtonsButtonWidth: {
        QSize sz;
        ret = d->aquaSizeConstrain(opt, 0, QStyle::CT_PushButton, QSize(-1, -1), &sz);
        if (sz == QSize(-1, -1))
            ret = 70;
        else
            ret = sz.width();
        break; }

    case PM_MenuBarHMargin:
        ret = 8;
        break;

    case PM_MenuBarVMargin:
        ret = 0;
        break;

    case PM_MenuBarPanelWidth:
        ret = 0;
        break;

    case PM_MenuButtonIndicator:
        ret = toolButtonArrowSize;
        break;

    case QStyle::PM_MenuDesktopFrameWidth:
        ret = 5;
        break;

    case PM_CheckBoxLabelSpacing:
    case PM_RadioButtonLabelSpacing:
        ret = [=] {
            if (opt) {
                if (opt->state & State_Mini)
                    return 4;
                if (opt->state & State_Small)
                    return 3;
            }
            return 2;
        } ();
        break;
    case PM_MenuScrollerHeight:
        ret = 15; // I hate having magic numbers in here...
        break;
    case PM_DefaultFrameWidth:
#if QT_CONFIG(mainwindow)
        if (widget && (widget->isWindow() || !widget->parentWidget()
                || (qobject_cast<const QMainWindow*>(widget->parentWidget())
                   && static_cast<QMainWindow *>(widget->parentWidget())->centralWidget() == widget))
                && qobject_cast<const QAbstractScrollArea *>(widget))
            ret = 0;
        else
#endif
        // The combo box popup has no frame.
        if (qstyleoption_cast<const QStyleOptionComboBox *>(opt) != 0)
            ret = 0;
        else
            ret = 1;
        break;
    case PM_MaximumDragDistance:
        ret = -1;
        break;
    case PM_ScrollBarSliderMin:
        ret = 24;
        break;
    case PM_SpinBoxFrameWidth:
        ret = qt_mac_aqua_get_metric(EditTextFrameOutset);
        break;
    case PM_ButtonShiftHorizontal:
    case PM_ButtonShiftVertical:
        ret = 0;
        break;
    case PM_SliderLength:
        ret = 17;
        break;
        // Returns the number of pixels to use for the business part of the
        // slider (i.e., the non-tickmark portion). The remaining space is shared
        // equally between the tickmark regions.
    case PM_SliderControlThickness:
        if (const QStyleOptionSlider *sl = qstyleoption_cast<const QStyleOptionSlider *>(opt)) {
            int space = (sl->orientation == Qt::Horizontal) ? sl->rect.height() : sl->rect.width();
            int ticks = sl->tickPosition;
            int n = 0;
            if (ticks & QSlider::TicksAbove)
                ++n;
            if (ticks & QSlider::TicksBelow)
                ++n;
            if (!n) {
                ret = space;
                break;
            }

            int thick = 6;        // Magic constant to get 5 + 16 + 5
            if (ticks != QSlider::TicksBothSides && ticks != QSlider::NoTicks)
                thick += proxy()->pixelMetric(PM_SliderLength, sl, widget) / 4;

            space -= thick;
            if (space > 0)
                thick += (space * 2) / (n + 2);
            ret = thick;
        } else {
            ret = 0;
        }
        break;
    case PM_SmallIconSize:
        ret = int(QStyleHelper::dpiScaled(16., opt));
        break;

    case PM_LargeIconSize:
        ret = int(QStyleHelper::dpiScaled(32., opt));
        break;

    case PM_IconViewIconSize:
        ret = proxy()->pixelMetric(PM_LargeIconSize, opt, widget);
        break;

    case PM_ButtonDefaultIndicator:
        ret = 0;
        break;
    case PM_TitleBarHeight: {
        NSUInteger style = NSWindowStyleMaskTitled;
        if (widget && ((widget->windowFlags() & Qt::Tool) == Qt::Tool))
            style |= NSWindowStyleMaskUtilityWindow;
        ret = int([NSWindow frameRectForContentRect:NSZeroRect
                                          styleMask:style].size.height);
        break; }
    case QStyle::PM_TabBarTabHSpace:
        switch (d->aquaSizeConstrain(opt, widget)) {
        case QStyleHelper::SizeLarge:
            ret = QCommonStyle::pixelMetric(metric, opt, widget);
            break;
        case QStyleHelper::SizeSmall:
            ret = 20;
            break;
        case QStyleHelper::SizeMini:
            ret = 16;
            break;
        case QStyleHelper::SizeDefault:
#if QT_CONFIG(tabbar)
            const QStyleOptionTab *tb = qstyleoption_cast<const QStyleOptionTab *>(opt);
            if (tb && tb->documentMode)
                ret = 30;
            else
#endif
                ret = QCommonStyle::pixelMetric(metric, opt, widget);
            break;
        }
        break;
    case PM_TabBarTabVSpace:
        ret = 4;
        break;
    case PM_TabBarTabShiftHorizontal:
    case PM_TabBarTabShiftVertical:
        ret = 0;
        break;
    case PM_TabBarBaseHeight:
        ret = 0;
        break;
    case PM_TabBarTabOverlap:
        ret = 1;
        break;
    case PM_TabBarBaseOverlap:
        switch (d->aquaSizeConstrain(opt, widget)) {
        case QStyleHelper::SizeDefault:
        case QStyleHelper::SizeLarge:
            ret = 11;
            break;
        case QStyleHelper::SizeSmall:
            ret = 8;
            break;
        case QStyleHelper::SizeMini:
            ret = 7;
            break;
        }
        break;
    case PM_ScrollBarExtent: {
        const QStyleHelper::WidgetSizePolicy size = d->effectiveAquaSizeConstrain(opt, widget);
        ret = static_cast<int>([NSScroller
            scrollerWidthForControlSize:static_cast<NSControlSize>(size)
                          scrollerStyle:[NSScroller preferredScrollerStyle]]);
        break; }
    case PM_IndicatorHeight: {
        switch (d->aquaSizeConstrain(opt, widget)) {
        case QStyleHelper::SizeDefault:
        case QStyleHelper::SizeLarge:
            ret = qt_mac_aqua_get_metric(CheckBoxHeight);
            break;
        case QStyleHelper::SizeMini:
            ret = qt_mac_aqua_get_metric(MiniCheckBoxHeight);
            break;
        case QStyleHelper::SizeSmall:
            ret = qt_mac_aqua_get_metric(SmallCheckBoxHeight);
            break;
        }
        break; }
    case PM_IndicatorWidth: {
        switch (d->aquaSizeConstrain(opt, widget)) {
        case QStyleHelper::SizeDefault:
        case QStyleHelper::SizeLarge:
            ret = qt_mac_aqua_get_metric(CheckBoxWidth);
            break;
        case QStyleHelper::SizeMini:
            ret = qt_mac_aqua_get_metric(MiniCheckBoxWidth);
            break;
        case QStyleHelper::SizeSmall:
            ret = qt_mac_aqua_get_metric(SmallCheckBoxWidth);
            break;
        }
        ++ret;
        break; }
    case PM_ExclusiveIndicatorHeight: {
        switch (d->aquaSizeConstrain(opt, widget)) {
        case QStyleHelper::SizeDefault:
        case QStyleHelper::SizeLarge:
            ret = qt_mac_aqua_get_metric(RadioButtonHeight);
            break;
        case QStyleHelper::SizeMini:
            ret = qt_mac_aqua_get_metric(MiniRadioButtonHeight);
            break;
        case QStyleHelper::SizeSmall:
            ret = qt_mac_aqua_get_metric(SmallRadioButtonHeight);
            break;
        }
        break; }
    case PM_ExclusiveIndicatorWidth: {
        switch (d->aquaSizeConstrain(opt, widget)) {
        case QStyleHelper::SizeDefault:
        case QStyleHelper::SizeLarge:
            ret = qt_mac_aqua_get_metric(RadioButtonWidth);
            break;
        case QStyleHelper::SizeMini:
            ret = qt_mac_aqua_get_metric(MiniRadioButtonWidth);
            break;
        case QStyleHelper::SizeSmall:
            ret = qt_mac_aqua_get_metric(SmallRadioButtonWidth);
            break;
        }
        ++ret;
        break; }
    case PM_MenuVMargin:
        ret = 4;
        break;
    case PM_MenuPanelWidth:
        ret = 0;
        break;
    case PM_ToolTipLabelFrameWidth:
        ret = 0;
        break;
    case PM_SizeGripSize: {
        QStyleHelper::WidgetSizePolicy aSize;
        if (widget && widget->window()->windowType() == Qt::Tool)
            aSize = QStyleHelper::SizeSmall;
        else
            aSize = QStyleHelper::SizeLarge;
        const QSize size = qt_aqua_get_known_size(CT_SizeGrip, widget, QSize(), aSize);
        ret = size.width();
        break; }
    case PM_MdiSubWindowFrameWidth:
        ret = 1;
        break;
    case PM_DockWidgetFrameWidth:
        ret = 0;
        break;
    case PM_DockWidgetTitleMargin:
        ret = 0;
        break;
    case PM_DockWidgetSeparatorExtent:
        ret = 1;
        break;
    case PM_ToolBarHandleExtent:
        ret = 11;
        break;
    case PM_ToolBarItemMargin:
        ret = 0;
        break;
    case PM_ToolBarItemSpacing:
        ret = 4;
        break;
    case PM_SplitterWidth:
        ret = qMax(7, QApplication::globalStrut().width());
        break;
    case PM_LayoutLeftMargin:
    case PM_LayoutTopMargin:
    case PM_LayoutRightMargin:
    case PM_LayoutBottomMargin:
        {
            bool isWindow = false;
            if (opt) {
                isWindow = (opt->state & State_Window);
            } else if (widget) {
                isWindow = widget->isWindow();
            }

            if (isWindow) {
                /*
                    AHIG would have (20, 8, 10) here but that makes
                    no sense. It would also have 14 for the top margin
                    but this contradicts both Builder and most
                    applications.
                */
                return_SIZE(20, 10, 10);    // AHIG
            } else {
                // hack to detect QTabWidget
                if (widget && widget->parentWidget()
                        && widget->parentWidget()->sizePolicy().controlType() == QSizePolicy::TabWidget) {
                    if (metric == PM_LayoutTopMargin) {
                        /*
                            Builder would have 14 (= 20 - 6) instead of 12,
                            but that makes the tab look disproportionate.
                        */
                        return_SIZE(12, 6, 6);  // guess
                    } else {
                        return_SIZE(20 /* Builder */, 8 /* guess */, 8 /* guess */);
                    }
                } else {
                    /*
                        Child margins are highly inconsistent in AHIG and Builder.
                    */
                    return_SIZE(12, 8, 6);    // guess
                }
            }
        }
    case PM_LayoutHorizontalSpacing:
    case PM_LayoutVerticalSpacing:
        return -1;
    case PM_MenuHMargin:
        ret = 0;
        break;
    case PM_ToolBarExtensionExtent:
        ret = 21;
        break;
    case PM_ToolBarFrameWidth:
        ret = 1;
        break;
    case PM_ScrollView_ScrollBarOverlap:
        ret = [NSScroller preferredScrollerStyle] == NSScrollerStyleOverlay ?
               pixelMetric(PM_ScrollBarExtent, opt, widget) : 0;
        break;
    default:
        ret = QCommonStyle::pixelMetric(metric, opt, widget);
        break;
    }
    return ret;
}

QPalette QMacStyle::standardPalette() const
{
    auto platformTheme = QGuiApplicationPrivate::platformTheme();
    auto styleNames = platformTheme->themeHint(QPlatformTheme::StyleNames);
    if (styleNames.toStringList().contains("macintosh"))
        return QPalette(); // Inherit everything from theme
    else
        return QStyle::standardPalette();
}

int QMacStyle::styleHint(StyleHint sh, const QStyleOption *opt, const QWidget *w,
                         QStyleHintReturn *hret) const
{
    QMacAutoReleasePool pool;

    int ret = 0;
    switch (sh) {
    case SH_Slider_SnapToValue:
    case SH_PrintDialog_RightAlignButtons:
    case SH_FontDialog_SelectAssociatedText:
    case SH_MenuBar_MouseTracking:
    case SH_Menu_MouseTracking:
    case SH_ComboBox_ListMouseTracking:
    case SH_MainWindow_SpaceBelowMenuBar:
    case SH_ItemView_ChangeHighlightOnFocus:
        ret = 1;
        break;
    case SH_ToolBox_SelectedPageTitleBold:
        ret = 0;
        break;
    case SH_DialogButtonBox_ButtonsHaveIcons:
        ret = 0;
        break;
    case SH_Menu_SelectionWrap:
        ret = false;
        break;
    case SH_Menu_KeyboardSearch:
        ret = true;
        break;
    case SH_Menu_SpaceActivatesItem:
        ret = true;
        break;
    case SH_Slider_AbsoluteSetButtons:
        ret = Qt::LeftButton|Qt::MiddleButton;
        break;
    case SH_Slider_PageSetButtons:
        ret = 0;
        break;
    case SH_ScrollBar_ContextMenu:
        ret = false;
        break;
    case SH_TitleBar_AutoRaise:
        ret = true;
        break;
    case SH_Menu_AllowActiveAndDisabled:
        ret = false;
        break;
    case SH_Menu_SubMenuPopupDelay:
        ret = 100;
        break;
    case SH_Menu_SubMenuUniDirection:
        ret = true;
        break;
    case SH_Menu_SubMenuSloppySelectOtherActions:
        ret = false;
        break;
    case SH_Menu_SubMenuResetWhenReenteringParent:
        ret = true;
        break;
    case SH_Menu_SubMenuDontStartSloppyOnLeave:
        ret = true;
        break;

    case SH_ScrollBar_LeftClickAbsolutePosition: {
        NSUserDefaults* defaults = [NSUserDefaults standardUserDefaults];
        bool result = [defaults boolForKey:@"AppleScrollerPagingBehavior"];
        if(QApplication::keyboardModifiers() & Qt::AltModifier)
            ret = !result;
        else
            ret = result;
        break; }
    case SH_TabBar_PreferNoArrows:
        ret = true;
        break;
        /*
    case SH_DialogButtons_DefaultButton:
        ret = QDialogButtons::Reject;
        break;
        */
    case SH_GroupBox_TextLabelVerticalAlignment:
        ret = Qt::AlignTop;
        break;
    case SH_ScrollView_FrameOnlyAroundContents:
        ret = QCommonStyle::styleHint(sh, opt, w, hret);
        break;
    case SH_Menu_FillScreenWithScroll:
        ret = false;
        break;
    case SH_Menu_Scrollable:
        ret = true;
        break;
    case SH_RichText_FullWidthSelection:
        ret = true;
        break;
    case SH_BlinkCursorWhenTextSelected:
        ret = false;
        break;
    case SH_ScrollBar_StopMouseOverSlider:
        ret = true;
        break;
    case SH_ListViewExpand_SelectMouseType:
        ret = QEvent::MouseButtonRelease;
        break;
    case SH_TabBar_SelectMouseType:
#if QT_CONFIG(tabbar)
        if (const QStyleOptionTabBarBase *opt2 = qstyleoption_cast<const QStyleOptionTabBarBase *>(opt)) {
            ret = opt2->documentMode ? QEvent::MouseButtonPress : QEvent::MouseButtonRelease;
        } else
#endif
        {
            ret = QEvent::MouseButtonRelease;
        }
        break;
    case SH_ComboBox_Popup:
        if (const QStyleOptionComboBox *cmb = qstyleoption_cast<const QStyleOptionComboBox *>(opt))
            ret = !cmb->editable;
        else
            ret = 0;
        break;
    case SH_Workspace_FillSpaceOnMaximize:
        ret = true;
        break;
    case SH_Widget_ShareActivation:
        ret = true;
        break;
    case SH_Header_ArrowAlignment:
        ret = Qt::AlignRight;
        break;
    case SH_TabBar_Alignment: {
#if QT_CONFIG(tabwidget)
        if (const QTabWidget *tab = qobject_cast<const QTabWidget*>(w)) {
            if (tab->documentMode()) {
                ret = Qt::AlignLeft;
                break;
            }
        }
#endif
#if QT_CONFIG(tabbar)
        if (const QTabBar *tab = qobject_cast<const QTabBar*>(w)) {
            if (tab->documentMode()) {
                ret = Qt::AlignLeft;
                break;
            }
        }
#endif
        ret = Qt::AlignCenter;
        } break;
    case SH_UnderlineShortcut:
        ret = false;
        break;
    case SH_ToolTipLabel_Opacity:
        ret = 242; // About 95%
        break;
    case SH_Button_FocusPolicy:
        ret = Qt::TabFocus;
        break;
    case SH_EtchDisabledText:
        ret = false;
        break;
    case SH_FocusFrame_Mask: {
        ret = true;
        if(QStyleHintReturnMask *mask = qstyleoption_cast<QStyleHintReturnMask*>(hret)) {
            const uchar fillR = 192, fillG = 191, fillB = 190;
            QImage img;

            QSize pixmapSize = opt->rect.size();
            if (!pixmapSize.isEmpty()) {
                QPixmap pix(pixmapSize);
                pix.fill(QColor(fillR, fillG, fillB));
                QPainter pix_paint(&pix);
                proxy()->drawControl(CE_FocusFrame, opt, &pix_paint, w);
                pix_paint.end();
                img = pix.toImage();
            }

            const QRgb *sptr = (QRgb*)img.bits(), *srow;
            const int sbpl = img.bytesPerLine();
            const int w = sbpl/4, h = img.height();

            QImage img_mask(img.width(), img.height(), QImage::Format_ARGB32);
            QRgb *dptr = (QRgb*)img_mask.bits(), *drow;
            const int dbpl = img_mask.bytesPerLine();

            for (int y = 0; y < h; ++y) {
                srow = sptr+((y*sbpl)/4);
                drow = dptr+((y*dbpl)/4);
                for (int x = 0; x < w; ++x) {
                    const int redDiff = qRed(*srow) - fillR;
                    const int greenDiff = qGreen(*srow) - fillG;
                    const int blueDiff = qBlue(*srow) - fillB;
                    const int diff = (redDiff * redDiff) + (greenDiff * greenDiff) + (blueDiff * blueDiff);
                    (*drow++) = (diff < 10) ? 0xffffffff : 0xff000000;
                    ++srow;
                }
            }
            QBitmap qmask = QBitmap::fromImage(img_mask);
            mask->region = QRegion(qmask);
        }
        break; }
    case SH_TitleBar_NoBorder:
        ret = 1;
        break;
    case SH_RubberBand_Mask:
        ret = 0;
        break;
    case SH_ComboBox_LayoutDirection:
        ret = Qt::LeftToRight;
        break;
    case SH_ItemView_EllipsisLocation:
        ret = Qt::AlignHCenter;
        break;
    case SH_ItemView_ShowDecorationSelected:
        ret = true;
        break;
    case SH_TitleBar_ModifyNotification:
        ret = false;
        break;
    case SH_ScrollBar_RollBetweenButtons:
        ret = true;
        break;
    case SH_WindowFrame_Mask:
        ret = false;
        break;
    case SH_TabBar_ElideMode:
        ret = Qt::ElideRight;
        break;
#if QT_CONFIG(dialogbuttonbox)
    case SH_DialogButtonLayout:
        ret = QDialogButtonBox::MacLayout;
        break;
#endif
    case SH_FormLayoutWrapPolicy:
        ret = QFormLayout::DontWrapRows;
        break;
    case SH_FormLayoutFieldGrowthPolicy:
        ret = QFormLayout::FieldsStayAtSizeHint;
        break;
    case SH_FormLayoutFormAlignment:
        ret = Qt::AlignHCenter | Qt::AlignTop;
        break;
    case SH_FormLayoutLabelAlignment:
        ret = Qt::AlignRight;
        break;
    case SH_ComboBox_PopupFrameStyle:
        ret = QFrame::NoFrame;
        break;
    case SH_MessageBox_TextInteractionFlags:
        ret = Qt::TextSelectableByMouse | Qt::LinksAccessibleByMouse | Qt::TextSelectableByKeyboard;
        break;
    case SH_SpellCheckUnderlineStyle:
        ret = QTextCharFormat::DashUnderline;
        break;
    case SH_MessageBox_CenterButtons:
        ret = false;
        break;
    case SH_MenuBar_AltKeyNavigation:
        ret = false;
        break;
    case SH_ItemView_MovementWithoutUpdatingSelection:
        ret = false;
        break;
    case SH_FocusFrame_AboveWidget:
        ret = true;
        break;
#if QT_CONFIG(wizard)
    case SH_WizardStyle:
        ret = QWizard::MacStyle;
        break;
#endif
    case SH_ItemView_ArrowKeysNavigateIntoChildren:
        ret = false;
        break;
    case SH_Menu_FlashTriggeredItem:
        ret = true;
        break;
    case SH_Menu_FadeOutOnHide:
        ret = true;
        break;
    case SH_ItemView_PaintAlternatingRowColorsForEmptyArea:
        ret = true;
        break;
#if QT_CONFIG(tabbar)
    case SH_TabBar_CloseButtonPosition:
        ret = QTabBar::LeftSide;
        break;
#endif
    case SH_DockWidget_ButtonsHaveFrame:
        ret = false;
        break;
    case SH_ScrollBar_Transient:
        if ((qobject_cast<const QScrollBar *>(w) && w->parent() &&
                qobject_cast<QAbstractScrollArea*>(w->parent()->parent()))
#ifndef QT_NO_ACCESSIBILITY
                || (opt && QStyleHelper::hasAncestor(opt->styleObject, QAccessible::ScrollBar))
#endif
        ) {
            ret = [NSScroller preferredScrollerStyle] == NSScrollerStyleOverlay;
        }
        break;
#if QT_CONFIG(itemviews)
    case SH_ItemView_ScrollMode:
        ret = QAbstractItemView::ScrollPerPixel;
        break;
#endif
    case SH_TitleBar_ShowToolTipsOnButtons:
        // min/max/close buttons on windows don't show tool tips
        ret = false;
        break;
    case SH_ComboBox_AllowWheelScrolling:
        ret = false;
        break;
    case SH_SpinBox_ButtonsInsideFrame:
        ret = false;
        break;
    case SH_Table_GridLineColor:
        ret = int(qt_mac_toQColor(NSColor.gridColor).rgba());
        break;
    default:
        ret = QCommonStyle::styleHint(sh, opt, w, hret);
        break;
    }
    return ret;
}

QPixmap QMacStyle::generatedIconPixmap(QIcon::Mode iconMode, const QPixmap &pixmap,
                                       const QStyleOption *opt) const
{
    switch (iconMode) {
    case QIcon::Disabled: {
        QImage img = pixmap.toImage().convertToFormat(QImage::Format_ARGB32);
        int imgh = img.height();
        int imgw = img.width();
        QRgb pixel;
        for (int y = 0; y < imgh; ++y) {
            for (int x = 0; x < imgw; ++x) {
                pixel = img.pixel(x, y);
                img.setPixel(x, y, qRgba(qRed(pixel), qGreen(pixel), qBlue(pixel),
                                         qAlpha(pixel) / 2));
            }
        }
        return QPixmap::fromImage(img);
    }
    default:
        ;
    }
    return QCommonStyle::generatedIconPixmap(iconMode, pixmap, opt);
}


QPixmap QMacStyle::standardPixmap(StandardPixmap standardPixmap, const QStyleOption *opt,
                                  const QWidget *widget) const
{
    // The default implementation of QStyle::standardIconImplementation() is to call standardPixmap()
    // I don't want infinite recursion so if we do get in that situation, just return the Window's
    // standard pixmap instead (since there is no mac-specific icon then). This should be fine until
    // someone changes how Windows standard
    // pixmap works.
    static bool recursionGuard = false;

    if (recursionGuard)
        return QCommonStyle::standardPixmap(standardPixmap, opt, widget);

    recursionGuard = true;
    QIcon icon = proxy()->standardIcon(standardPixmap, opt, widget);
    recursionGuard = false;
    int size;
    switch (standardPixmap) {
        default:
            size = 32;
            break;
        case SP_MessageBoxCritical:
        case SP_MessageBoxQuestion:
        case SP_MessageBoxInformation:
        case SP_MessageBoxWarning:
            size = 64;
            break;
    }
    return icon.pixmap(qt_getWindow(widget), QSize(size, size));
}

void QMacStyle::drawPrimitive(PrimitiveElement pe, const QStyleOption *opt, QPainter *p,
                              const QWidget *w) const
{
    Q_D(const QMacStyle);
    const AppearanceSync appSync;
    QMacCGContext cg(p);
    QWindow *window = w && w->window() ? w->window()->windowHandle() : nullptr;
    d->resolveCurrentNSView(window);
    switch (pe) {
    case PE_IndicatorArrowUp:
    case PE_IndicatorArrowDown:
    case PE_IndicatorArrowRight:
    case PE_IndicatorArrowLeft: {
        p->save();
        p->setRenderHint(QPainter::Antialiasing);
        const int xOffset = 1; // FIXME: opt->direction == Qt::LeftToRight ? 2 : -1;
        qreal halfSize = 0.5 * qMin(opt->rect.width(), opt->rect.height());
        const qreal penWidth = qMax(halfSize / 3.0, 1.25);
#if QT_CONFIG(toolbutton)
        if (const QToolButton *tb = qobject_cast<const QToolButton *>(w)) {
            // When stroking the arrow, make sure it fits in the tool button
            if (tb->arrowType() != Qt::NoArrow
                    || tb->popupMode() == QToolButton::MenuButtonPopup)
                halfSize -= penWidth;
        }
#endif

        QTransform transform;
        transform.translate(opt->rect.center().x() + xOffset, opt->rect.center().y() + 2);
        QPainterPath path;
        switch(pe) {
        default:
        case PE_IndicatorArrowDown:
            break;
        case PE_IndicatorArrowUp:
            transform.rotate(180);
            break;
        case PE_IndicatorArrowLeft:
            transform.rotate(90);
            break;
        case PE_IndicatorArrowRight:
            transform.rotate(-90);
            break;
        }
        p->setTransform(transform);

        path.moveTo(-halfSize, -halfSize * 0.5);
        path.lineTo(0.0, halfSize * 0.5);
        path.lineTo(halfSize, -halfSize * 0.5);

        const QPen arrowPen(opt->palette.text(), penWidth,
                            Qt::SolidLine, Qt::RoundCap, Qt::RoundJoin);
        p->strokePath(path, arrowPen);
        p->restore();
        break; }
#if QT_CONFIG(tabbar)
    case PE_FrameTabBarBase:
        if (const QStyleOptionTabBarBase *tbb
                = qstyleoption_cast<const QStyleOptionTabBarBase *>(opt)) {
            if (tbb->documentMode) {
                p->save();
                drawTabBase(p, tbb, w);
                p->restore();
                return;
            }
#if QT_CONFIG(tabwidget)
            QRegion region(tbb->rect);
            region -= tbb->tabBarRect;
            p->save();
            p->setClipRegion(region);
            QStyleOptionTabWidgetFrame twf;
            twf.QStyleOption::operator=(*tbb);
            twf.shape  = tbb->shape;
            switch (QMacStylePrivate::tabDirection(twf.shape)) {
            case QMacStylePrivate::North:
                twf.rect = twf.rect.adjusted(0, 0, 0, 10);
                break;
            case QMacStylePrivate::South:
                twf.rect = twf.rect.adjusted(0, -10, 0, 0);
                break;
            case QMacStylePrivate::West:
                twf.rect = twf.rect.adjusted(0, 0, 10, 0);
                break;
            case QMacStylePrivate::East:
                twf.rect = twf.rect.adjusted(0, -10, 0, 0);
                break;
            }
            proxy()->drawPrimitive(PE_FrameTabWidget, &twf, p, w);
            p->restore();
#endif
        }
        break;
#endif
    case PE_PanelTipLabel:
        p->fillRect(opt->rect, opt->palette.brush(QPalette::ToolTipBase));
        break;
    case PE_FrameGroupBox:
        if (const auto *groupBox = qstyleoption_cast<const QStyleOptionFrame *>(opt))
            if (groupBox->features & QStyleOptionFrame::Flat) {
                QCommonStyle::drawPrimitive(pe, groupBox, p, w);
                break;
            }
#if QT_CONFIG(tabwidget)
        Q_FALLTHROUGH();
    case PE_FrameTabWidget:
#endif
    {
        const auto cw = QMacStylePrivate::CocoaControl(QMacStylePrivate::Box, QStyleHelper::SizeLarge);
        auto *box = static_cast<NSBox *>(d->cocoaControl(cw));
        // FIXME Since macOS 10.14, simply calling drawRect: won't display anything anymore.
        // The AppKit team is aware of this and has proposed a couple of solutions.
        // The first solution was to call displayRectIgnoringOpacity:inContext: instead.
        // However, it doesn't seem to work on 10.13. More importantly, dark mode on 10.14
        // is extremely slow. Light mode works fine.
        // The second solution is to subclass NSBox and reimplement a trivial drawRect: which
        // would only call super. This works without any issue on 10.13, but a double border
        // shows on 10.14 in both light and dark modes.
        // The code below picks what works on each version and mode. On 10.13 and earlier, we
        // simply call drawRect: on a regular NSBox. On 10.14, we call displayRectIgnoringOpacity:
        // inContext:, but only in light mode. In dark mode, we use a custom NSBox subclass,
        // QDarkNSBox, of type NSBoxCustom. Its appearance is close enough to the real thing so
        // we can use this for now.
        auto adjustedRect = opt->rect;
        bool needTranslation = false;
        if (QOperatingSystemVersion::current() >= QOperatingSystemVersion::MacOSMojave
            && !qt_mac_applicationIsInDarkMode()) {
            // In Aqua theme we have to use the 'default' NSBox (as opposite
            // to the 'custom' QDarkNSBox we use in dark theme). Since -drawRect:
            // does nothing in default NSBox, we call -displayRectIgnoringOpaticty:.
            // Unfortunately, the resulting box is smaller then the actual rect we
            // wanted. This can be seen, e.g. because tabs (buttons) are misaligned
            // vertically and even worse, if QTabWidget has autoFillBackground
            // set, this background overpaints NSBox making it to disappear.
            // We trick our NSBox to render in a larger rectangle, so that
            // the actuall result (which is again smaller than requested),
            // more or less is what we really want. We'll have to adjust CTM
            // and translate accordingly.
            adjustedRect.adjust(0, 0, 6, 6);
            needTranslation = true;
        }
        d->drawNSViewInRect(box, adjustedRect, p, ^(CGContextRef ctx, const CGRect &rect) {
#if QT_CONFIG(tabwidget)
            if (QTabWidget *tabWidget = qobject_cast<QTabWidget *>(opt->styleObject))
                clipTabBarFrame(opt, this, ctx);
#endif
            CGContextTranslateCTM(ctx, 0, rect.origin.y + rect.size.height);
            CGContextScaleCTM(ctx, 1, -1);
            if (QOperatingSystemVersion::current() < QOperatingSystemVersion::MacOSMojave
                || [box isMemberOfClass:QDarkNSBox.class]) {
                [box drawRect:rect];
            } else {
                if (needTranslation)
                    CGContextTranslateCTM(ctx, -3.0, 5.0);
                [box displayRectIgnoringOpacity:box.bounds inContext:NSGraphicsContext.currentContext];
            }
        });
        break;
    }
    case PE_IndicatorToolBarSeparator: {
            QPainterPath path;
            if (opt->state & State_Horizontal) {
                int xpoint = opt->rect.center().x();
                path.moveTo(xpoint + 0.5, opt->rect.top() + 1);
                path.lineTo(xpoint + 0.5, opt->rect.bottom());
            } else {
                int ypoint = opt->rect.center().y();
                path.moveTo(opt->rect.left() + 2 , ypoint + 0.5);
                path.lineTo(opt->rect.right() + 1, ypoint + 0.5);
            }
            QPainterPathStroker theStroker;
            theStroker.setCapStyle(Qt::FlatCap);
            theStroker.setDashPattern(QVector<qreal>() << 1 << 2);
            path = theStroker.createStroke(path);
            const auto dark = qt_mac_applicationIsInDarkMode() ? opt->palette.dark().color().darker()
                                                               : QColor(0, 0, 0, 119);
            p->fillPath(path, dark);
        }
        break;
    case PE_FrameWindow:
        if (const QStyleOptionFrame *frame = qstyleoption_cast<const QStyleOptionFrame *>(opt)) {
            if (w && w->inherits("QMdiSubWindow")) {
                p->save();
                p->setPen(QPen(frame->palette.dark().color(), frame->lineWidth));
                p->setBrush(frame->palette.window());
                p->drawRect(frame->rect);
                p->restore();
            }
        }
        break;
    case PE_IndicatorDockWidgetResizeHandle: {
            // The docwidget resize handle is drawn as a one-pixel wide line.
            p->save();
            if (opt->state & State_Horizontal) {
                p->setPen(QColor(160, 160, 160));
                p->drawLine(opt->rect.topLeft(), opt->rect.topRight());
            } else {
                p->setPen(QColor(145, 145, 145));
                p->drawLine(opt->rect.topRight(), opt->rect.bottomRight());
            }
            p->restore();
        } break;
    case PE_IndicatorToolBarHandle: {
            p->save();
            QPainterPath path;
            int x = opt->rect.x() + 6;
            int y = opt->rect.y() + 7;
            static const int RectHeight = 2;
            if (opt->state & State_Horizontal) {
                while (y < opt->rect.height() - RectHeight - 5) {
                    path.moveTo(x, y);
                    path.addEllipse(x, y, RectHeight, RectHeight);
                    y += 6;
                }
            } else {
                while (x < opt->rect.width() - RectHeight - 5) {
                    path.moveTo(x, y);
                    path.addEllipse(x, y, RectHeight, RectHeight);
                    x += 6;
                }
            }
            p->setPen(Qt::NoPen);
            QColor dark = opt->palette.dark().color().darker();
            dark.setAlphaF(0.50);
            p->fillPath(path, dark);
            p->restore();

            break;
        }
    case PE_IndicatorHeaderArrow:
        if (const QStyleOptionHeader *header = qstyleoption_cast<const QStyleOptionHeader *>(opt)) {
            // In HITheme, up is down, down is up and hamburgers eat people.
            if (header->sortIndicator != QStyleOptionHeader::None)
                proxy()->drawPrimitive(
                    (header->sortIndicator == QStyleOptionHeader::SortDown) ?
                    PE_IndicatorArrowUp : PE_IndicatorArrowDown, header, p, w);
        }
        break;
    case PE_IndicatorMenuCheckMark: {
        QColor pc;
        if (opt->state & State_On)
            pc = opt->palette.highlightedText().color();
        else
            pc = opt->palette.text().color();

        QCFType<CGColorRef> checkmarkColor = CGColorCreateGenericRGB(static_cast<CGFloat>(pc.redF()),
                                                                     static_cast<CGFloat>(pc.greenF()),
                                                                     static_cast<CGFloat>(pc.blueF()),
                                                                     static_cast<CGFloat>(pc.alphaF()));
        // kCTFontUIFontSystem and others give the same result
        // as kCTFontUIFontMenuItemMark. However, the latter is
        // more reminiscent to HITheme's kThemeMenuItemMarkFont.
        // See also the font for small- and mini-sized widgets,
        // where we end up using the generic system font type.
        const CTFontUIFontType fontType = (opt->state & State_Mini) ? kCTFontUIFontMiniSystem :
                                          (opt->state & State_Small) ? kCTFontUIFontSmallSystem :
                                          kCTFontUIFontMenuItemMark;
        // Similarly for the font size, where there is a small difference
        // between regular combobox and item view items, and and menu items.
        // However, we ignore any difference for small- and mini-sized widgets.
        const CGFloat fontSize = fontType == kCTFontUIFontMenuItemMark ? opt->fontMetrics.height() : 0.0;
        QCFType<CTFontRef> checkmarkFont = CTFontCreateUIFontForLanguage(fontType, fontSize, NULL);

        CGContextSaveGState(cg);
        CGContextSetShouldSmoothFonts(cg, NO); // Same as HITheme and Cocoa menu checkmarks

        // Baseline alignment tweaks for QComboBox and QMenu
        const CGFloat vOffset = (opt->state & State_Mini) ? 0.0 :
                                (opt->state & State_Small) ? 1.0 :
                                0.75;

        CGContextTranslateCTM(cg, 0, opt->rect.bottom());
        CGContextScaleCTM(cg, 1, -1);
        // Translate back to the original position and add rect origin and offset
        CGContextTranslateCTM(cg, opt->rect.x(), vOffset);

        // CTFont has severe difficulties finding the checkmark character among its
        // glyphs. Fortunately, CTLine knows its ways inside the Cocoa labyrinth.
        static const CFStringRef keys[] = { kCTFontAttributeName, kCTForegroundColorAttributeName };
        static const int numValues = sizeof(keys) / sizeof(keys[0]);
        const CFTypeRef values[] = { (CFTypeRef)checkmarkFont,  (CFTypeRef)checkmarkColor };
        Q_STATIC_ASSERT((sizeof(values) / sizeof(values[0])) == numValues);
        QCFType<CFDictionaryRef> attributes = CFDictionaryCreate(kCFAllocatorDefault, (const void **)keys, (const void **)values,
                                                                 numValues, NULL, NULL);
        // U+2713: CHECK MARK
        QCFType<CFAttributedStringRef> checkmarkString = CFAttributedStringCreate(kCFAllocatorDefault, (CFStringRef)@"\u2713", attributes);
        QCFType<CTLineRef> line = CTLineCreateWithAttributedString(checkmarkString);

        CTLineDraw((CTLineRef)line, cg);
        CGContextFlush(cg); // CTLineDraw's documentation says it doesn't flush

        CGContextRestoreGState(cg);
        break; }
    case PE_IndicatorViewItemCheck:
    case PE_IndicatorRadioButton:
    case PE_IndicatorCheckBox: {
        const bool isEnabled = opt->state & State_Enabled;
        const bool isPressed = opt->state & State_Sunken;
        const bool isRadioButton = (pe == PE_IndicatorRadioButton);
        const auto ct = isRadioButton ? QMacStylePrivate::Button_RadioButton : QMacStylePrivate::Button_CheckBox;
        const auto cs = d->effectiveAquaSizeConstrain(opt, w);
        const auto cw = QMacStylePrivate::CocoaControl(ct, cs);
        auto *tb = static_cast<NSButton *>(d->cocoaControl(cw));
        tb.enabled = isEnabled;
        tb.state = (opt->state & State_NoChange) ? NSMixedState :
                   (opt->state & State_On) ? NSOnState : NSOffState;
        [tb highlight:isPressed];
        const auto vOffset = [=] {
            // As measured
            if (cs == QStyleHelper::SizeMini)
                return ct == QMacStylePrivate::Button_CheckBox ? -0.5 : 0.5;

            return cs == QStyleHelper::SizeSmall ? 0.5 : 0.0;
        } ();
        d->drawNSViewInRect(tb, opt->rect, p, ^(CGContextRef ctx, const CGRect &rect) {
            CGContextTranslateCTM(ctx, 0, vOffset);
            [tb.cell drawInteriorWithFrame:rect inView:tb];
        });
        break; }
    case PE_FrameFocusRect:
        // Use the our own focus widget stuff.
        break;
    case PE_IndicatorBranch: {
        if (!(opt->state & State_Children))
            break;
        const auto cw = QMacStylePrivate::CocoaControl(QMacStylePrivate::Button_Disclosure, QStyleHelper::SizeLarge);
        NSButtonCell *triangleCell = static_cast<NSButtonCell *>(d->cocoaCell(cw));
        [triangleCell setState:(opt->state & State_Open) ? NSOnState : NSOffState];
        bool viewHasFocus = (w && w->hasFocus()) || (opt->state & State_HasFocus);
        [triangleCell setBackgroundStyle:((opt->state & State_Selected) && viewHasFocus) ? NSBackgroundStyleDark : NSBackgroundStyleLight];

        d->setupNSGraphicsContext(cg, NO);

        QRect qtRect = opt->rect.adjusted(DisclosureOffset, 0, -DisclosureOffset, 0);
        CGRect rect = CGRectMake(qtRect.x() + 1, qtRect.y(), qtRect.width(), qtRect.height());
        CGContextTranslateCTM(cg, rect.origin.x, rect.origin.y + rect.size.height);
        CGContextScaleCTM(cg, 1, -1);
        CGContextTranslateCTM(cg, -rect.origin.x, -rect.origin.y);

        [triangleCell drawBezelWithFrame:NSRectFromCGRect(rect) inView:[triangleCell controlView]];

        d->restoreNSGraphicsContext(cg);
        break; }

    case PE_Frame: {
        QPen oldPen = p->pen();
        p->setPen(opt->palette.base().color().darker(140));
        p->drawRect(opt->rect.adjusted(0, 0, -1, -1));
        p->setPen(opt->palette.base().color().darker(180));
        p->drawLine(opt->rect.topLeft(), opt->rect.topRight());
        p->setPen(oldPen);
        break; }

    case PE_FrameLineEdit:
        if (const QStyleOptionFrame *frame = qstyleoption_cast<const QStyleOptionFrame *>(opt)) {
            if (frame->state & State_Sunken) {
                const bool isEnabled = opt->state & State_Enabled;
                const bool isReadOnly = opt->state & State_ReadOnly;
                const bool isRounded = frame->features & QStyleOptionFrame::Rounded;
                const auto cs = d->effectiveAquaSizeConstrain(opt, w, CT_LineEdit);
                const auto cw = QMacStylePrivate::CocoaControl(QMacStylePrivate::TextField, cs);
                auto *tf = static_cast<NSTextField *>(d->cocoaControl(cw));
                tf.enabled = isEnabled;
                tf.editable = !isReadOnly;
                tf.bezeled = YES;
                static_cast<NSTextFieldCell *>(tf.cell).bezelStyle = isRounded ? NSTextFieldRoundedBezel : NSTextFieldSquareBezel;
                tf.frame = opt->rect.toCGRect();
                d->drawNSViewInRect(tf, opt->rect, p, ^(CGContextRef, const CGRect &rect) {
                    if (!qt_mac_applicationIsInDarkMode()) {
                        // In 'Dark' mode controls are transparent, so we do not
                        // over-paint the (potentially custom) color in the background.
                        // In 'Light' mode we have to care about the correct
                        // background color. See the comments below for PE_PanelLineEdit.
                        CGContextRef cgContext = NSGraphicsContext.currentContext.CGContext;
                        // See QMacCGContext, here we expect bitmap context created with
                        // color space 'kCGColorSpaceSRGB', if it's something else - we
                        // give up.
                        if (cgContext ? bool(CGBitmapContextGetColorSpace(cgContext)) : false) {
                            tf.drawsBackground = YES;
                            const QColor bgColor = frame->palette.brush(QPalette::Base).color();
                            tf.backgroundColor = [NSColor colorWithSRGBRed:bgColor.redF()
                                                                     green:bgColor.greenF()
                                                                      blue:bgColor.blueF()
                                                                     alpha:bgColor.alphaF()];
                            if (bgColor.alpha() != 255) {
                                // No way we can have it bezeled and transparent ...
                                tf.bordered = YES;
                            }
                        }
                    }

                    [tf.cell drawWithFrame:rect inView:tf];
                });
            } else {
                QCommonStyle::drawPrimitive(pe, opt, p, w);
            }
        }
        break;
    case PE_PanelLineEdit:
        {
            const QStyleOptionFrame *panel = qstyleoption_cast<const QStyleOptionFrame *>(opt);
            if (qt_mac_applicationIsInDarkMode() || (panel && panel->lineWidth <= 0)) {
                // QCommonStyle::drawPrimitive(PE_PanelLineEdit) fill the background with
                // a proper color, defined in opt->palette and then, if lineWidth > 0, it
                // calls QMacStyle::drawPrimitive(PE_FrameLineEdit). We use NSTextFieldCell
                // to handle PE_FrameLineEdit, which will use system-default background.
                // In 'Dark' mode it's transparent and thus it's not over-painted.
                QCommonStyle::drawPrimitive(pe, opt, p, w);
            } else {
                // In 'Light' mode, if panel->lineWidth > 0, we have to use the correct
                // background color when drawing PE_FrameLineEdit, so let's call it
                // directly and set the proper color there.
                drawPrimitive(PE_FrameLineEdit, opt, p, w);
            }

            // Draw the focus frame for widgets other than QLineEdit (e.g. for line edits in Webkit).
            // Focus frame is drawn outside the rectangle passed in the option-rect.
            if (panel) {
#if QT_CONFIG(lineedit)
                if ((opt->state & State_HasFocus) && !qobject_cast<const QLineEdit*>(w)) {
                    int vmargin = pixelMetric(QStyle::PM_FocusFrameVMargin);
                    int hmargin = pixelMetric(QStyle::PM_FocusFrameHMargin);
                    QStyleOptionFrame focusFrame = *panel;
                    focusFrame.rect = panel->rect.adjusted(-hmargin, -vmargin, hmargin, vmargin);
                    drawControl(CE_FocusFrame, &focusFrame, p, w);
                }
#endif
            }
        }
        break;
    case PE_PanelScrollAreaCorner: {
        const QBrush brush(opt->palette.brush(QPalette::Base));
        p->fillRect(opt->rect, brush);
        p->setPen(QPen(QColor(217, 217, 217)));
        p->drawLine(opt->rect.topLeft(), opt->rect.topRight());
        p->drawLine(opt->rect.topLeft(), opt->rect.bottomLeft());
        } break;
    case PE_FrameStatusBarItem:
        break;
#if QT_CONFIG(tabbar)
    case PE_IndicatorTabClose: {
        // Make close button visible only on the hovered tab.
        QTabBar *tabBar = qobject_cast<QTabBar*>(w->parentWidget());
        const QWidget *closeBtn = w;
        if (!tabBar) {
            // QStyleSheetStyle instead of CloseButton (which has
            // a QTabBar as a parent widget) uses the QTabBar itself:
            tabBar = qobject_cast<QTabBar *>(const_cast<QWidget*>(w));
            closeBtn = decltype(closeBtn)(property("_q_styleSheetRealCloseButton").value<void *>());
        }
        if (tabBar) {
            const bool documentMode = tabBar->documentMode();
            const QTabBarPrivate *tabBarPrivate = static_cast<QTabBarPrivate *>(QObjectPrivate::get(tabBar));
            const int hoveredTabIndex = tabBarPrivate->hoveredTabIndex();
            if (!documentMode ||
                (hoveredTabIndex != -1 && ((closeBtn == tabBar->tabButton(hoveredTabIndex, QTabBar::LeftSide)) ||
                                           (closeBtn == tabBar->tabButton(hoveredTabIndex, QTabBar::RightSide))))) {
                const bool hover = (opt->state & State_MouseOver);
                const bool selected = (opt->state & State_Selected);
                const bool pressed = (opt->state & State_Sunken);
                drawTabCloseButton(p, hover, selected, pressed, documentMode);
            }
        }
        } break;
#endif // QT_CONFIG(tabbar)
    case PE_PanelStatusBar: {
        // Fill the status bar with the titlebar gradient.
        QLinearGradient linearGrad;
        if (w ? qt_macWindowMainWindow(w->window()) : (opt->state & QStyle::State_Active)) {
            linearGrad = titlebarGradientActive();
        } else {
            linearGrad = titlebarGradientInactive();
        }

        linearGrad.setStart(0, opt->rect.top());
        linearGrad.setFinalStop(0, opt->rect.bottom());
        p->fillRect(opt->rect, linearGrad);

        // Draw the black separator line at the top of the status bar.
        if (w ? qt_macWindowMainWindow(w->window()) : (opt->state & QStyle::State_Active))
            p->setPen(titlebarSeparatorLineActive);
        else
            p->setPen(titlebarSeparatorLineInactive);
        p->drawLine(opt->rect.left(), opt->rect.top(), opt->rect.right(), opt->rect.top());

        break;
    }
    case PE_PanelMenu: {
        p->save();
        p->fillRect(opt->rect, Qt::transparent);
        p->setPen(Qt::transparent);
        p->setBrush(opt->palette.window());
        p->setRenderHint(QPainter::Antialiasing, true);
        const QPainterPath path = d->windowPanelPath(opt->rect);
        p->drawPath(path);
        p->restore();
        } break;

    default:
        QCommonStyle::drawPrimitive(pe, opt, p, w);
        break;
    }
}

static QPixmap darkenPixmap(const QPixmap &pixmap)
{
    QImage img = pixmap.toImage().convertToFormat(QImage::Format_ARGB32);
    int imgh = img.height();
    int imgw = img.width();
    int h, s, v, a;
    QRgb pixel;
    for (int y = 0; y < imgh; ++y) {
        for (int x = 0; x < imgw; ++x) {
            pixel = img.pixel(x, y);
            a = qAlpha(pixel);
            QColor hsvColor(pixel);
            hsvColor.getHsv(&h, &s, &v);
            s = qMin(100, s * 2);
            v = v / 2;
            hsvColor.setHsv(h, s, v);
            pixel = hsvColor.rgb();
            img.setPixel(x, y, qRgba(qRed(pixel), qGreen(pixel), qBlue(pixel), a));
        }
    }
    return QPixmap::fromImage(img);
}

void QMacStylePrivate::setupVerticalInvertedXform(CGContextRef cg, bool reverse, bool vertical, const CGRect &rect) const
{
    if (vertical) {
        CGContextTranslateCTM(cg, rect.size.height, 0);
        CGContextRotateCTM(cg, M_PI_2);
    }
    if (vertical != reverse) {
        CGContextTranslateCTM(cg, rect.size.width, 0);
        CGContextScaleCTM(cg, -1, 1);
    }
}

void QMacStyle::drawControl(ControlElement ce, const QStyleOption *opt, QPainter *p,
                            const QWidget *w) const
{
    Q_D(const QMacStyle);
    const AppearanceSync sync;
    const QMacAutoReleasePool pool;
    QMacCGContext cg(p);
    QWindow *window = w && w->window() ? w->window()->windowHandle() : nullptr;
    d->resolveCurrentNSView(window);
    switch (ce) {
    case CE_HeaderSection:
        if (const QStyleOptionHeader *header = qstyleoption_cast<const QStyleOptionHeader *>(opt)) {
            State flags = header->state;
            QRect ir = header->rect;


#if 0 // FIXME: What's this solving exactly?
            bool noVerticalHeader = true;
#if QT_CONFIG(tableview)
            if (w)
                if (const QTableView *table = qobject_cast<const QTableView *>(w->parentWidget()))
                    noVerticalHeader = !table->verticalHeader()->isVisible();
#endif

            const bool drawLeftBorder = header->orientation == Qt::Vertical
                    || header->position == QStyleOptionHeader::OnlyOneSection
                    || (header->position == QStyleOptionHeader::Beginning && noVerticalHeader);
#endif

            const bool pressed = (flags & State_Sunken) && !(flags & State_On);
            p->fillRect(ir, pressed ? header->palette.dark() : header->palette.button());
            p->setPen(QPen(header->palette.dark(), 1.0));
            if (header->orientation == Qt::Horizontal)
                p->drawLine(QLineF(ir.right() + 0.5, ir.top() + headerSectionSeparatorInset,
                                   ir.right() + 0.5, ir.bottom() - headerSectionSeparatorInset));
            else
                p->drawLine(QLineF(ir.left() + headerSectionSeparatorInset, ir.bottom(),
                                   ir.right() - headerSectionSeparatorInset, ir.bottom()));
        }

        break;
    case CE_HeaderLabel:
        if (const QStyleOptionHeader *header = qstyleoption_cast<const QStyleOptionHeader *>(opt)) {
            p->save();
            QRect textr = header->rect;
            if (!header->icon.isNull()) {
                QIcon::Mode mode = QIcon::Disabled;
                if (opt->state & State_Enabled)
                    mode = QIcon::Normal;
                int iconExtent = proxy()->pixelMetric(PM_SmallIconSize);
                QPixmap pixmap = header->icon.pixmap(window, QSize(iconExtent, iconExtent), mode);

                QRect pixr = header->rect;
                pixr.setY(header->rect.center().y() - (pixmap.height() / pixmap.devicePixelRatio() - 1) / 2);
                proxy()->drawItemPixmap(p, pixr, Qt::AlignVCenter, pixmap);
                textr.translate(pixmap.width() / pixmap.devicePixelRatio() + 2, 0);
            }

            proxy()->drawItemText(p, textr, header->textAlignment | Qt::AlignVCenter, header->palette,
                                       header->state & State_Enabled, header->text, QPalette::ButtonText);
            p->restore();
        }
        break;
    case CE_ToolButtonLabel:
        if (const QStyleOptionToolButton *tb = qstyleoption_cast<const QStyleOptionToolButton *>(opt)) {
            QStyleOptionToolButton myTb = *tb;
            myTb.state &= ~State_AutoRaise;
#ifndef QT_NO_ACCESSIBILITY
            if (QStyleHelper::hasAncestor(opt->styleObject, QAccessible::ToolBar)) {
                QRect cr = tb->rect;
                int shiftX = 0;
                int shiftY = 0;
                bool needText = false;
                int alignment = 0;
                bool down = tb->state & (State_Sunken | State_On);
                if (down) {
                    shiftX = proxy()->pixelMetric(PM_ButtonShiftHorizontal, tb, w);
                    shiftY = proxy()->pixelMetric(PM_ButtonShiftVertical, tb, w);
                }
                // The down state is special for QToolButtons in a toolbar on the Mac
                // The text is a bit bolder and gets a drop shadow and the icons are also darkened.
                // This doesn't really fit into any particular case in QIcon, so we
                // do the majority of the work ourselves.
                if (!(tb->features & QStyleOptionToolButton::Arrow)) {
                    Qt::ToolButtonStyle tbstyle = tb->toolButtonStyle;
                    if (tb->icon.isNull() && !tb->text.isEmpty())
                        tbstyle = Qt::ToolButtonTextOnly;

                    switch (tbstyle) {
                    case Qt::ToolButtonTextOnly: {
                        needText = true;
                        alignment = Qt::AlignCenter;
                        break; }
                    case Qt::ToolButtonIconOnly:
                    case Qt::ToolButtonTextBesideIcon:
                    case Qt::ToolButtonTextUnderIcon: {
                        QRect pr = cr;
                        QIcon::Mode iconMode = (tb->state & State_Enabled) ? QIcon::Normal
                                                                            : QIcon::Disabled;
                        QIcon::State iconState = (tb->state & State_On) ? QIcon::On
                                                                         : QIcon::Off;
                        QPixmap pixmap = tb->icon.pixmap(window,
                                                         tb->rect.size().boundedTo(tb->iconSize),
                                                         iconMode, iconState);

                        // Draw the text if it's needed.
                        if (tb->toolButtonStyle != Qt::ToolButtonIconOnly) {
                            needText = true;
                            if (tb->toolButtonStyle == Qt::ToolButtonTextUnderIcon) {
                                pr.setHeight(pixmap.size().height() / pixmap.devicePixelRatio() + 6);
                                cr.adjust(0, pr.bottom(), 0, -3);
                                alignment |= Qt::AlignCenter;
                            } else {
                                pr.setWidth(pixmap.width() / pixmap.devicePixelRatio() + 8);
                                cr.adjust(pr.right(), 0, 0, 0);
                                alignment |= Qt::AlignLeft | Qt::AlignVCenter;
                            }
                        }
                        if (opt->state & State_Sunken) {
                            pr.translate(shiftX, shiftY);
                            pixmap = darkenPixmap(pixmap);
                        }
                        proxy()->drawItemPixmap(p, pr, Qt::AlignCenter, pixmap);
                        break; }
                    default:
                        Q_ASSERT(false);
                        break;
                    }

                    if (needText) {
                        QPalette pal = tb->palette;
                        QPalette::ColorRole role = QPalette::NoRole;
                        if (!proxy()->styleHint(SH_UnderlineShortcut, tb, w))
                            alignment |= Qt::TextHideMnemonic;
                        if (down)
                            cr.translate(shiftX, shiftY);
                        if (tbstyle == Qt::ToolButtonTextOnly
                            || (tbstyle != Qt::ToolButtonTextOnly && !down)) {
                            QPen pen = p->pen();
                            QColor light = down || isDarkMode() ? Qt::black : Qt::white;
                            light.setAlphaF(0.375f);
                            p->setPen(light);
                            p->drawText(cr.adjusted(0, 1, 0, 1), alignment, tb->text);
                            p->setPen(pen);
                            if (down && tbstyle == Qt::ToolButtonTextOnly) {
                                pal = QApplication::palette("QMenu");
                                pal.setCurrentColorGroup(tb->palette.currentColorGroup());
                                role = QPalette::HighlightedText;
                            }
                        }
                        proxy()->drawItemText(p, cr, alignment, pal,
                                              tb->state & State_Enabled, tb->text, role);
                    }
                } else {
                    QCommonStyle::drawControl(ce, &myTb, p, w);
                }
            } else
#endif // QT_NO_ACCESSIBILITY
            {
                QCommonStyle::drawControl(ce, &myTb, p, w);
            }
        }
        break;
    case CE_ToolBoxTabShape:
        QCommonStyle::drawControl(ce, opt, p, w);
        break;
    case CE_PushButtonBevel:
        if (const QStyleOptionButton *btn = qstyleoption_cast<const QStyleOptionButton *>(opt)) {
            if (!(btn->state & (State_Raised | State_Sunken | State_On)))
                break;

            if (btn->features & QStyleOptionButton::CommandLinkButton) {
                QCommonStyle::drawControl(ce, opt, p, w);
                break;
            }

            const bool hasFocus = btn->state & State_HasFocus;
            const bool isActive = btn->state & State_Active;

            // a focused auto-default button within an active window
            // takes precedence over a normal default button
            if ((btn->features & QStyleOptionButton::AutoDefaultButton)
                && isActive && hasFocus)
                d->autoDefaultButton = btn->styleObject;
            else if (d->autoDefaultButton == btn->styleObject)
                d->autoDefaultButton = nullptr;

            const bool isEnabled = btn->state & State_Enabled;
            const bool isPressed = btn->state & State_Sunken;
            const bool isHighlighted = isActive &&
                    ((btn->state & State_On)
                     || (btn->features & QStyleOptionButton::DefaultButton)
                     || (btn->features & QStyleOptionButton::AutoDefaultButton
                         && d->autoDefaultButton == btn->styleObject));
            const bool hasMenu = btn->features & QStyleOptionButton::HasMenu;
            const auto ct = cocoaControlType(btn, w);
            const auto cs = d->effectiveAquaSizeConstrain(btn, w);
            const auto cw = QMacStylePrivate::CocoaControl(ct, cs);
            auto *pb = static_cast<NSButton *>(d->cocoaControl(cw));
            // Ensure same size and location as we used to have with HITheme.
            // This is more convoluted than we initialy thought. See for example
            // differences between plain and menu button frames.
            const QRectF frameRect = cw.adjustedControlFrame(btn->rect);
            pb.frame = frameRect.toCGRect();

            pb.enabled = isEnabled;
            [pb highlight:isPressed];
            pb.state = isHighlighted && !isPressed ? NSOnState : NSOffState;
            d->drawNSViewInRect(pb, frameRect, p, ^(CGContextRef, const CGRect &r) {
                [pb.cell drawBezelWithFrame:r inView:pb.superview];
            });
            [pb highlight:NO];

            if (hasMenu && cw.type == QMacStylePrivate::Button_SquareButton) {
                // Using -[NSPopuButtonCell drawWithFrame:inView:] above won't do
                // it right because we don't set the text in the native button.
                const int mbi = proxy()->pixelMetric(QStyle::PM_MenuButtonIndicator, btn, w);
                const auto ir = frameRect.toRect();
                int arrowYOffset = 0;
#if 0
                // FIXME What's this for again?
                if (!w) {
                    // adjustment for Qt Quick Controls
                    arrowYOffset -= ir.top();
                    if (cw.second == QStyleHelper::SizeSmall)
                        arrowYOffset += 1;
                }
#endif
                const auto ar = visualRect(btn->direction, ir, QRect(ir.right() - mbi - 6, ir.height() / 2 - arrowYOffset, mbi, mbi));

                QStyleOption arrowOpt = *opt;
                arrowOpt.rect = ar;
                proxy()->drawPrimitive(PE_IndicatorArrowDown, &arrowOpt, p, w);
            }


            if (btn->state & State_HasFocus) {
                // TODO Remove and use QFocusFrame instead.
                const int hMargin = proxy()->pixelMetric(QStyle::PM_FocusFrameHMargin, btn, w);
                const int vMargin = proxy()->pixelMetric(QStyle::PM_FocusFrameVMargin, btn, w);
                QRectF focusRect;
                if (cw.type == QMacStylePrivate::Button_SquareButton) {
                    focusRect = frameRect;
                } else {
                    focusRect = QRectF::fromCGRect([pb alignmentRectForFrame:pb.frame]);
                    if (cw.type == QMacStylePrivate::Button_PushButton)
                        focusRect -= pushButtonShadowMargins[cw.size];
                    else if (cw.type == QMacStylePrivate::Button_PullDown)
                        focusRect -= pullDownButtonShadowMargins[cw.size];
                }
                d->drawFocusRing(p, focusRect, hMargin, vMargin, cw);
            }
        }
        break;
    case CE_PushButtonLabel:
        if (const QStyleOptionButton *b = qstyleoption_cast<const QStyleOptionButton *>(opt)) {
            QStyleOptionButton btn(*b);
            // We really don't want the label to be drawn the same as on
            // windows style if it has an icon and text, then it should be more like a
            // tab. So, cheat a little here. However, if it *is* only an icon
            // the windows style works great, so just use that implementation.
            const bool isEnabled = btn.state & State_Enabled;
            const bool hasMenu = btn.features & QStyleOptionButton::HasMenu;
            const bool hasIcon = !btn.icon.isNull();
            const bool hasText = !btn.text.isEmpty();
            const bool isActive = btn.state & State_Active;
            const bool isPressed = btn.state & State_Sunken;

            const auto ct = cocoaControlType(&btn, w);

            if (!hasMenu && ct != QMacStylePrivate::Button_SquareButton) {
                if (isPressed
                    || (isActive && isEnabled
                        && ((btn.state & State_On)
                            || ((btn.features & QStyleOptionButton::DefaultButton) && !d->autoDefaultButton)
                            || d->autoDefaultButton == btn.styleObject)))
                btn.palette.setColor(QPalette::ButtonText, Qt::white);
            }

            if ((!hasIcon && !hasMenu) || (hasIcon && !hasText)) {
                QCommonStyle::drawControl(ce, &btn, p, w);
            } else {
                QRect freeContentRect = btn.rect;
                QRect textRect = itemTextRect(
                            btn.fontMetrics, freeContentRect, Qt::AlignCenter, isEnabled, btn.text);
                if (hasMenu) {
                    textRect.moveTo(w ? 15 : 11, textRect.top()); // Supports Qt Quick Controls
                }
                // Draw the icon:
                if (hasIcon) {
                    int contentW = textRect.width();
                    if (hasMenu)
                        contentW += proxy()->pixelMetric(PM_MenuButtonIndicator) + 4;
                    QIcon::Mode mode = isEnabled ? QIcon::Normal : QIcon::Disabled;
                    if (mode == QIcon::Normal && btn.state & State_HasFocus)
                        mode = QIcon::Active;
                    // Decide if the icon is should be on or off:
                    QIcon::State state = QIcon::Off;
                    if (btn.state & State_On)
                        state = QIcon::On;
                    QPixmap pixmap = btn.icon.pixmap(window, btn.iconSize, mode, state);
                    int pixmapWidth = pixmap.width() / pixmap.devicePixelRatio();
                    int pixmapHeight = pixmap.height() / pixmap.devicePixelRatio();
                    contentW += pixmapWidth + QMacStylePrivate::PushButtonContentPadding;
                    int iconLeftOffset = freeContentRect.x() + (freeContentRect.width() - contentW) / 2;
                    int iconTopOffset = freeContentRect.y() + (freeContentRect.height() - pixmapHeight) / 2;
                    QRect iconDestRect(iconLeftOffset, iconTopOffset, pixmapWidth, pixmapHeight);
                    QRect visualIconDestRect = visualRect(btn.direction, freeContentRect, iconDestRect);
                    proxy()->drawItemPixmap(p, visualIconDestRect, Qt::AlignLeft | Qt::AlignVCenter, pixmap);
                    int newOffset = iconDestRect.x() + iconDestRect.width()
                            + QMacStylePrivate::PushButtonContentPadding - textRect.x();
                    textRect.adjust(newOffset, 0, newOffset, 0);
                }
                // Draw the text:
                if (hasText) {
                    textRect = visualRect(btn.direction, freeContentRect, textRect);
                    proxy()->drawItemText(p, textRect, Qt::AlignLeft | Qt::AlignVCenter | Qt::TextShowMnemonic, btn.palette,
                                          isEnabled, btn.text, QPalette::ButtonText);
                }
            }
        }
        break;
#if QT_CONFIG(combobox)
    case CE_ComboBoxLabel:
        if (const auto *cb = qstyleoption_cast<const QStyleOptionComboBox *>(opt)) {
            auto comboCopy = *cb;
            comboCopy.direction = Qt::LeftToRight;
            // The rectangle will be adjusted to SC_ComboBoxEditField with comboboxEditBounds()
            QCommonStyle::drawControl(CE_ComboBoxLabel, &comboCopy, p, w);
        }
        break;
#endif // #if QT_CONFIG(combobox)
#if QT_CONFIG(tabbar)
    case CE_TabBarTabShape:
        if (const auto *tabOpt = qstyleoption_cast<const QStyleOptionTab *>(opt)) {
            if (tabOpt->documentMode) {
                p->save();
                bool isUnified = false;
                if (w) {
                    QRect tabRect = tabOpt->rect;
                    QPoint windowTabStart = w->mapTo(w->window(), tabRect.topLeft());
                    isUnified = isInMacUnifiedToolbarArea(w->window()->windowHandle(), windowTabStart.y());
                }

                const int tabOverlap = proxy()->pixelMetric(PM_TabBarTabOverlap, opt, w);
                drawTabShape(p, tabOpt, isUnified, tabOverlap);

                p->restore();
                return;
            }

            const bool isActive = tabOpt->state & State_Active;
            const bool isEnabled = tabOpt->state & State_Enabled;
            const bool isPressed = tabOpt->state & State_Sunken;
            const bool isSelected = tabOpt->state & State_Selected;
            const auto tabDirection = QMacStylePrivate::tabDirection(tabOpt->shape);
            const bool verticalTabs = tabDirection == QMacStylePrivate::East
                                   || tabDirection == QMacStylePrivate::West;

            QStyleOptionTab::TabPosition tp = tabOpt->position;
            QStyleOptionTab::SelectedPosition sp = tabOpt->selectedPosition;
            if (tabOpt->direction == Qt::RightToLeft && !verticalTabs) {
                if (tp == QStyleOptionTab::Beginning)
                    tp = QStyleOptionTab::End;
                else if (tp == QStyleOptionTab::End)
                    tp = QStyleOptionTab::Beginning;

                if (sp == QStyleOptionTab::NextIsSelected)
                    sp = QStyleOptionTab::PreviousIsSelected;
                else if (sp == QStyleOptionTab::PreviousIsSelected)
                    sp = QStyleOptionTab::NextIsSelected;
            }

            // Alas, NSSegmentedControl and NSSegmentedCell are letting us down.
            // We're not able to draw it at will, either calling -[drawSegment:
            // inFrame:withView:], -[drawRect:] or anything in between. Besides,
            // there's no public API do draw the pressed state, AFAICS. We'll use
            // a push NSButton instead and clip the CGContext.
            // NOTE/TODO: this is not true. On 10.13 NSSegmentedControl works with
            // some (black?) magic/magic dances, on 10.14 it simply works (was
            // it fixed in AppKit?). But, indeed, we cannot make a tab 'pressed'
            // with NSSegmentedControl (only selected), so we stay with buttons
            // (mixing buttons and NSSegmentedControl for such a simple thing
            // is too much work).

            const auto cs = d->effectiveAquaSizeConstrain(opt, w);
            // Extra hacks to get the proper pressed appreance when not selected or selected and inactive
            const bool needsInactiveHack = (!isActive && isSelected);
            const auto ct = !needsInactiveHack && (isSelected || tp == QStyleOptionTab::OnlyOneTab) ?
                    QMacStylePrivate::Button_PushButton :
                    QMacStylePrivate::Button_PopupButton;
            const bool isPopupButton = ct == QMacStylePrivate::Button_PopupButton;
            const auto cw = QMacStylePrivate::CocoaControl(ct, cs);
            auto *pb = static_cast<NSButton *>(d->cocoaControl(cw));

            auto vOffset = isPopupButton ? 1 : 2;
            if (tabDirection == QMacStylePrivate::East)
                vOffset -= 1;
            const auto outerAdjust = isPopupButton ? 1 : 4;
            const auto innerAdjust = isPopupButton ? 20 : 10;
            QRectF frameRect = tabOpt->rect;
            if (verticalTabs)
                frameRect = QRectF(frameRect.y(), frameRect.x(), frameRect.height(), frameRect.width());
            // Adjust before clipping
            frameRect = frameRect.translated(0, vOffset);
            switch (tp) {
            case QStyleOptionTab::Beginning:
                // Pressed state hack: tweak adjustments in preparation for flip below
                if (!isSelected && tabDirection == QMacStylePrivate::West)
                    frameRect = frameRect.adjusted(-innerAdjust, 0, outerAdjust, 0);
                else
                    frameRect = frameRect.adjusted(-outerAdjust, 0, innerAdjust, 0);
                break;
            case QStyleOptionTab::Middle:
                frameRect = frameRect.adjusted(-innerAdjust, 0, innerAdjust, 0);
                break;
            case QStyleOptionTab::End:
                // Pressed state hack: tweak adjustments in preparation for flip below
                if (isSelected || tabDirection == QMacStylePrivate::West)
                    frameRect = frameRect.adjusted(-innerAdjust, 0, outerAdjust, 0);
                else
                    frameRect = frameRect.adjusted(-outerAdjust, 0, innerAdjust, 0);
                break;
            case QStyleOptionTab::OnlyOneTab:
                frameRect = frameRect.adjusted(-outerAdjust, 0, outerAdjust, 0);
                break;
            }
            pb.frame = frameRect.toCGRect();

            pb.enabled = isEnabled;
            [pb highlight:isPressed];
            // Set off state when inactive. See needsInactiveHack for when it's selected
            pb.state = (isActive && isSelected && !isPressed) ? NSOnState : NSOffState;

            const auto drawBezelBlock = ^(CGContextRef ctx, const CGRect &r) {
                CGContextClipToRect(ctx, opt->rect.toCGRect());
                if (!isSelected || needsInactiveHack) {
                    // Final stage of the pressed state hack: flip NSPopupButton rendering
                    if (!verticalTabs && tp == QStyleOptionTab::End) {
                        CGContextTranslateCTM(ctx, opt->rect.right(), 0);
                        CGContextScaleCTM(ctx, -1, 1);
                        CGContextTranslateCTM(ctx, -frameRect.left(), 0);
                    } else if (tabDirection == QMacStylePrivate::West && tp == QStyleOptionTab::Beginning) {
                        CGContextTranslateCTM(ctx, 0, opt->rect.top());
                        CGContextScaleCTM(ctx, 1, -1);
                        CGContextTranslateCTM(ctx, 0, -frameRect.right());
                    } else if (tabDirection == QMacStylePrivate::East && tp == QStyleOptionTab::End) {
                        CGContextTranslateCTM(ctx, 0, opt->rect.bottom());
                        CGContextScaleCTM(ctx, 1, -1);
                        CGContextTranslateCTM(ctx, 0, -frameRect.left());
                    }
                }

                // Rotate and translate CTM when vertical
                // On macOS: positive angle is CW, negative is CCW
                if (tabDirection == QMacStylePrivate::West) {
                    CGContextTranslateCTM(ctx, 0, frameRect.right());
                    CGContextRotateCTM(ctx, -M_PI_2);
                    CGContextTranslateCTM(ctx, -frameRect.left(), 0);
                } else if (tabDirection == QMacStylePrivate::East) {
                    CGContextTranslateCTM(ctx, opt->rect.right(), 0);
                    CGContextRotateCTM(ctx, M_PI_2);
                }

                // Now, if it's a trick with a popup button, it has an arrow
                // which makes no sense on tabs.
                NSPopUpArrowPosition oldPosition = NSPopUpArrowAtCenter;
                NSPopUpButtonCell *pbCell = nil;
                auto rAdjusted = r;
                if (isPopupButton && tp == QStyleOptionTab::OnlyOneTab) {
                    pbCell = static_cast<NSPopUpButtonCell *>(pb.cell);
                    oldPosition = pbCell.arrowPosition;
                    pbCell.arrowPosition = NSPopUpNoArrow;
                    if (pb.state == NSControlStateValueOff) {
                        // NSPopUpButton in this state is smaller.
                        rAdjusted.origin.x -= 3;
                        rAdjusted.size.width += 6;
                    }
                }

                [pb.cell drawBezelWithFrame:rAdjusted inView:pb.superview];

                if (pbCell) // Restore, we may reuse it for a ComboBox.
                    pbCell.arrowPosition = oldPosition;
            };

            if (needsInactiveHack) {
                // First, render tab as non-selected tab on a pixamp
                const qreal pixelRatio = p->device()->devicePixelRatioF();
                QImage tabPixmap(opt->rect.size() * pixelRatio, QImage::Format_ARGB32_Premultiplied);
                tabPixmap.setDevicePixelRatio(pixelRatio);
                tabPixmap.fill(Qt::transparent);
                QPainter tabPainter(&tabPixmap);
                d->drawNSViewInRect(pb, frameRect, &tabPainter, ^(CGContextRef ctx, const CGRect &r) {
                    CGContextTranslateCTM(ctx, -opt->rect.left(), -opt->rect.top());
                    drawBezelBlock(ctx, r);
                });
                tabPainter.end();

                // Then, darken it with the proper shade of gray
                const qreal inactiveGray = 0.898; // As measured
                const int inactiveGray8 = qRound(inactiveGray * 255.0);
                const QRgb inactiveGrayRGB = qRgb(inactiveGray8, inactiveGray8, inactiveGray8);
                for (int l = 0; l < tabPixmap.height(); ++l) {
                    auto *line = reinterpret_cast<QRgb*>(tabPixmap.scanLine(l));
                    for (int i = 0; i < tabPixmap.width(); ++i) {
                        if (qAlpha(line[i]) == 255) {
                            line[i] = inactiveGrayRGB;
                        } else if (qAlpha(line[i]) > 128) {
                            const int g = qRound(inactiveGray * qRed(line[i]));
                            line[i] = qRgba(g, g, g, qAlpha(line[i]));
                        }
                    }
                }

                // Finally, draw the tab pixmap on the current painter
                p->drawImage(opt->rect, tabPixmap);
            } else {
                d->drawNSViewInRect(pb, frameRect, p, drawBezelBlock);
            }

            if (!isSelected && sp != QStyleOptionTab::NextIsSelected
                    && tp != QStyleOptionTab::End
                    && tp != QStyleOptionTab::OnlyOneTab) {
                static const QPen separatorPen(Qt::black, 1.0);
                p->save();
                p->setOpacity(isEnabled ? 0.105 : 0.06); // As measured
                p->setPen(separatorPen);
                if (tabDirection == QMacStylePrivate::West) {
                    p->drawLine(QLineF(opt->rect.left() + 1.5, opt->rect.bottom(),
                                       opt->rect.right() - 0.5, opt->rect.bottom()));
                } else if (tabDirection == QMacStylePrivate::East) {
                    p->drawLine(QLineF(opt->rect.left(), opt->rect.bottom(),
                                       opt->rect.right() - 0.5, opt->rect.bottom()));
                } else {
                    p->drawLine(QLineF(opt->rect.right(), opt->rect.top() + 1.0,
                                       opt->rect.right(), opt->rect.bottom() - 0.5));
                }
                p->restore();
            }

            // TODO Needs size adjustment to fit the focus ring
            if (tabOpt->state & State_HasFocus) {
                QMacStylePrivate::CocoaControlType focusRingType;
                switch (tp) {
                case QStyleOptionTab::Beginning:
                    focusRingType = verticalTabs ? QMacStylePrivate::SegmentedControl_Last
                                                 : QMacStylePrivate::SegmentedControl_First;
                    break;
                case QStyleOptionTab::Middle:
                    focusRingType = QMacStylePrivate::SegmentedControl_Middle;
                    break;
                case QStyleOptionTab::End:
                    focusRingType = verticalTabs ? QMacStylePrivate::SegmentedControl_First
                                                 : QMacStylePrivate::SegmentedControl_Last;
                    break;
                case QStyleOptionTab::OnlyOneTab:
                    focusRingType = QMacStylePrivate::SegmentedControl_Single;
                    break;
                }
            }
        }
        break;
    case CE_TabBarTabLabel:
        if (const auto *tab = qstyleoption_cast<const QStyleOptionTab *>(opt)) {
            QStyleOptionTab myTab = *tab;
            const auto tabDirection = QMacStylePrivate::tabDirection(tab->shape);
            const bool verticalTabs = tabDirection == QMacStylePrivate::East
                                   || tabDirection == QMacStylePrivate::West;

            // Check to see if we use have the same as the system font
            // (QComboMenuItem is internal and should never be seen by the
            // outside world, unless they read the source, in which case, it's
            // their own fault).
            const bool nonDefaultFont = p->font() != qt_app_fonts_hash()->value("QComboMenuItem");

            if (!myTab.documentMode && (myTab.state & State_Selected) && (myTab.state & State_Active))
                if (const auto *tabBar = qobject_cast<const QTabBar *>(w))
                    if (!tabBar->tabTextColor(tabBar->currentIndex()).isValid())
                        myTab.palette.setColor(QPalette::WindowText, Qt::white);

            if (myTab.documentMode && isDarkMode()) {
                bool active = (myTab.state & State_Selected) && (myTab.state & State_Active);
                myTab.palette.setColor(QPalette::WindowText, active ? Qt::white : Qt::gray);
            }

            int heightOffset = 0;
            if (verticalTabs) {
                heightOffset = -1;
            } else if (nonDefaultFont) {
                if (p->fontMetrics().height() == myTab.rect.height())
                    heightOffset = 2;
            }
            myTab.rect.setHeight(myTab.rect.height() + heightOffset);

            QCommonStyle::drawControl(ce, &myTab, p, w);
        }
        break;
#endif
#if QT_CONFIG(dockwidget)
    case CE_DockWidgetTitle:
        if (const auto *dwOpt = qstyleoption_cast<const QStyleOptionDockWidget *>(opt)) {
            const bool isVertical = dwOpt->verticalTitleBar;
            const auto effectiveRect = isVertical ? opt->rect.transposed() : opt->rect;
            p->save();
            if (isVertical) {
                p->translate(effectiveRect.left(), effectiveRect.top() + effectiveRect.width());
                p->rotate(-90);
                p->translate(-effectiveRect.left(), -effectiveRect.top());
            }

            // fill title bar background
            QLinearGradient linearGrad;
            linearGrad.setStart(QPointF(0, 0));
            linearGrad.setFinalStop(QPointF(0, 2 * effectiveRect.height()));
            linearGrad.setColorAt(0, opt->palette.button().color());
            linearGrad.setColorAt(1, opt->palette.dark().color());
            p->fillRect(effectiveRect, linearGrad);

            // draw horizontal line at bottom
            p->setPen(opt->palette.dark().color());
            p->drawLine(effectiveRect.bottomLeft(), effectiveRect.bottomRight());

            if (!dwOpt->title.isEmpty()) {
                auto titleRect = proxy()->subElementRect(SE_DockWidgetTitleBarText, opt, w);
                if (isVertical)
                    titleRect = QRect(effectiveRect.left() + opt->rect.bottom() - titleRect.bottom(),
                                      effectiveRect.top() + titleRect.left() - opt->rect.left(),
                                      titleRect.height(),
                                      titleRect.width());

                const auto text = p->fontMetrics().elidedText(dwOpt->title, Qt::ElideRight, titleRect.width());
                proxy()->drawItemText(p, titleRect, Qt::AlignCenter, dwOpt->palette,
                                      dwOpt->state & State_Enabled, text, QPalette::WindowText);
            }
            p->restore();
        }
        break;
#endif
    case CE_FocusFrame: {
        const auto *ff = qobject_cast<const QFocusFrame *>(w);
        const auto *ffw = ff ? ff->widget() : nullptr;
        const auto ct = [=] {
            if (ffw) {
                if (ffw->inherits("QCheckBox"))
                    return QMacStylePrivate::Button_CheckBox;
                if (ffw->inherits("QRadioButton"))
                    return QMacStylePrivate::Button_RadioButton;
                if (ffw->inherits("QLineEdit") || ffw->inherits("QTextEdit"))
                    return QMacStylePrivate::TextField;
            }

            return QMacStylePrivate::Box; // Not really, just make it the default
        } ();
        const auto cs = ffw ? (ffw->testAttribute(Qt::WA_MacMiniSize) ? QStyleHelper::SizeMini :
                               ffw->testAttribute(Qt::WA_MacSmallSize) ? QStyleHelper::SizeSmall :
                               QStyleHelper::SizeLarge) :
                        QStyleHelper::SizeLarge;
        const int hMargin = proxy()->pixelMetric(QStyle::PM_FocusFrameHMargin, opt, w);
        const int vMargin = proxy()->pixelMetric(QStyle::PM_FocusFrameVMargin, opt, w);
        d->drawFocusRing(p, opt->rect, hMargin, vMargin, QMacStylePrivate::CocoaControl(ct, cs));
        break; }
    case CE_MenuEmptyArea:
        // Skip: PE_PanelMenu fills in everything
        break;
    case CE_MenuItem:
    case CE_MenuHMargin:
    case CE_MenuVMargin:
    case CE_MenuTearoff:
    case CE_MenuScroller:
        if (const QStyleOptionMenuItem *mi = qstyleoption_cast<const QStyleOptionMenuItem *>(opt)) {
            const bool active = mi->state & State_Selected;
            if (active)
                p->fillRect(mi->rect, mi->palette.highlight());

            const QStyleHelper::WidgetSizePolicy widgetSize = d->aquaSizeConstrain(opt, w);

            if (ce == CE_MenuTearoff) {
                p->setPen(QPen(mi->palette.dark().color(), 1, Qt::DashLine));
                p->drawLine(mi->rect.x() + 2, mi->rect.y() + mi->rect.height() / 2 - 1,
                            mi->rect.x() + mi->rect.width() - 4,
                            mi->rect.y() + mi->rect.height() / 2 - 1);
                p->setPen(QPen(mi->palette.light().color(), 1, Qt::DashLine));
                p->drawLine(mi->rect.x() + 2, mi->rect.y() + mi->rect.height() / 2,
                            mi->rect.x() + mi->rect.width() - 4,
                            mi->rect.y() + mi->rect.height() / 2);
            } else if (ce == CE_MenuScroller) {
                const QSize scrollerSize = QSize(10, 8);
                const int scrollerVOffset = 5;
                const int left = mi->rect.x() + (mi->rect.width() - scrollerSize.width()) / 2;
                const int right = left + scrollerSize.width();
                int top;
                int bottom;
                if (opt->state & State_DownArrow) {
                    bottom = mi->rect.y() + scrollerVOffset;
                    top = bottom + scrollerSize.height();
                } else {
                    bottom = mi->rect.bottom() - scrollerVOffset;
                    top = bottom - scrollerSize.height();
                }
                p->save();
                p->setRenderHint(QPainter::Antialiasing);
                QPainterPath path;
                path.moveTo(left, bottom);
                path.lineTo(right, bottom);
                path.lineTo((left + right) / 2, top);
                p->fillPath(path, opt->palette.buttonText());
                p->restore();
            } else if (ce != CE_MenuItem) {
                break;
            }

            if (mi->menuItemType == QStyleOptionMenuItem::Separator) {
                CGColorRef separatorColor = [NSColor quaternaryLabelColor].CGColor;
                const QRect separatorRect = QRect(mi->rect.left(), mi->rect.center().y(), mi->rect.width(), 2);
                p->fillRect(separatorRect, qt_mac_toQColor(separatorColor));
                break;
            }

            const int maxpmw = mi->maxIconWidth;
            const bool enabled = mi->state & State_Enabled;

            int xpos = mi->rect.x() + 18;
            int checkcol = maxpmw;
            if (!enabled)
                p->setPen(mi->palette.text().color());
            else if (active)
                p->setPen(mi->palette.highlightedText().color());
            else
                p->setPen(mi->palette.buttonText().color());

            if (mi->checked) {
                QStyleOption checkmarkOpt;
                checkmarkOpt.initFrom(w);

                const int mw = checkcol + macItemFrame;
                const int mh = mi->rect.height() + macItemFrame;
                const int xp = mi->rect.x() + macItemFrame;
                checkmarkOpt.rect = QRect(xp, mi->rect.y() - checkmarkOpt.fontMetrics.descent(), mw, mh);

                checkmarkOpt.state.setFlag(State_On, active);
                checkmarkOpt.state.setFlag(State_Enabled, enabled);
                if (widgetSize == QStyleHelper::SizeMini)
                    checkmarkOpt.state |= State_Mini;
                else if (widgetSize == QStyleHelper::SizeSmall)
                    checkmarkOpt.state |= State_Small;

                // We let drawPrimitive(PE_IndicatorMenuCheckMark) pick the right color
                checkmarkOpt.palette.setColor(QPalette::HighlightedText, p->pen().color());
                checkmarkOpt.palette.setColor(QPalette::Text, p->pen().color());

                proxy()->drawPrimitive(PE_IndicatorMenuCheckMark, &checkmarkOpt, p, w);
            }
            if (!mi->icon.isNull()) {
                QIcon::Mode mode = (mi->state & State_Enabled) ? QIcon::Normal
                                                               : QIcon::Disabled;
                // Always be normal or disabled to follow the Mac style.
                int smallIconSize = proxy()->pixelMetric(PM_SmallIconSize);
                QSize iconSize(smallIconSize, smallIconSize);
#if QT_CONFIG(combobox)
                if (const QComboBox *comboBox = qobject_cast<const QComboBox *>(w)) {
                    iconSize = comboBox->iconSize();
                }
#endif
                QPixmap pixmap = mi->icon.pixmap(window, iconSize, mode);
                int pixw = pixmap.width() / pixmap.devicePixelRatio();
                int pixh = pixmap.height() / pixmap.devicePixelRatio();
                QRect cr(xpos, mi->rect.y(), checkcol, mi->rect.height());
                QRect pmr(0, 0, pixw, pixh);
                pmr.moveCenter(cr.center());
                p->drawPixmap(pmr.topLeft(), pixmap);
                xpos += pixw + 6;
            }

            QString s = mi->text;
            const auto text_flags = Qt::AlignVCenter | Qt::TextHideMnemonic
                                  | Qt::TextSingleLine | Qt::AlignAbsolute;
            int yPos = mi->rect.y();
            if (widgetSize == QStyleHelper::SizeMini)
                yPos += 1;

            const bool isSubMenu = mi->menuItemType == QStyleOptionMenuItem::SubMenu;
            const int tabwidth = isSubMenu ? 9 : mi->tabWidth;

            QString rightMarginText;
            if (isSubMenu)
                rightMarginText = QStringLiteral("\u25b6\ufe0e"); // U+25B6 U+FE0E: BLACK RIGHT-POINTING TRIANGLE

            // If present, save and remove embedded shorcut from text
            const int tabIndex = s.indexOf(QLatin1Char('\t'));
            if (tabIndex >= 0) {
                if (!isSubMenu) // ... but ignore it if it's a submenu.
                    rightMarginText = s.mid(tabIndex + 1);
                s = s.left(tabIndex);
            }

            p->save();
            if (!rightMarginText.isEmpty()) {
                p->setFont(qt_app_fonts_hash()->value("QMenuItem", p->font()));
                int xp = mi->rect.right() - tabwidth - macRightBorder + 2;
                if (!isSubMenu)
                    xp -= macItemHMargin + macItemFrame + 3; // Adjust for shortcut
                p->drawText(xp, yPos, tabwidth, mi->rect.height(), text_flags | Qt::AlignRight, rightMarginText);
            }

            if (!s.isEmpty()) {
                const int xm = macItemFrame + maxpmw + macItemHMargin;
                QFont myFont = mi->font;
                // myFont may not have any "hard" flags set. We override
                // the point size so that when it is resolved against the device, this font will win.
                // This is mainly to handle cases where someone sets the font on the window
                // and then the combo inherits it and passes it onward. At that point the resolve mask
                // is very, very weak. This makes it stonger.
                myFont.setPointSizeF(QFontInfo(mi->font).pointSizeF());

                // QTBUG-65653: Our own text rendering doesn't look good enough, especially on non-retina
                // displays. Worked around here while waiting for a proper fix in QCoreTextFontEngine.
                // Only if we're not using QCoreTextFontEngine we do fallback to our own text rendering.
                const auto *fontEngine = QFontPrivate::get(myFont)->engineForScript(QChar::Script_Common);
                Q_ASSERT(fontEngine);
                if (fontEngine->type() == QFontEngine::Multi) {
                    fontEngine = static_cast<const QFontEngineMulti *>(fontEngine)->engine(0);
                    Q_ASSERT(fontEngine);
                }
                if (fontEngine->type() == QFontEngine::Mac) {
                    NSFont *f = (NSFont *)(CTFontRef)fontEngine->handle();

                    // Respect the menu item palette as set in the style option.
                    const auto pc = p->pen().color();
                    NSColor *c = [NSColor colorWithSRGBRed:pc.redF()
                                                     green:pc.greenF()
                                                      blue:pc.blueF()
                                                     alpha:pc.alphaF()];

                    s = qt_mac_removeMnemonics(s);

                    QMacCGContext cgCtx(p);
                    d->setupNSGraphicsContext(cgCtx, YES);

                    // Draw at point instead of in rect, as the rect we've computed for the menu item
                    // is based on the font metrics we got from HarfBuzz, so we may risk having CoreText
                    // line-break the string if it doesn't fit the given rect. It's better to draw outside
                    // the rect and possibly overlap something than to have part of the text disappear.
                    [s.toNSString() drawAtPoint:CGPointMake(xpos, yPos)
                                withAttributes:@{ NSFontAttributeName:f, NSForegroundColorAttributeName:c,
                                                  NSObliquenessAttributeName: [NSNumber numberWithDouble: myFont.italic() ? 0.3 : 0.0]}];

                    d->restoreNSGraphicsContext(cgCtx);
                } else {
                    p->setFont(myFont);
                    p->drawText(xpos, yPos, mi->rect.width() - xm - tabwidth + 1,
                                mi->rect.height(), text_flags, s);
                }
            }
            p->restore();
        }
        break;
    case CE_MenuBarItem:
    case CE_MenuBarEmptyArea:
        if (const QStyleOptionMenuItem *mi = qstyleoption_cast<const QStyleOptionMenuItem *>(opt)) {
            const bool selected = (opt->state & State_Selected) && (opt->state & State_Enabled) && (opt->state & State_Sunken);
            const QBrush bg = selected ? mi->palette.highlight() : mi->palette.window();
            p->fillRect(mi->rect, bg);

            if (ce != CE_MenuBarItem)
                break;

            if (!mi->icon.isNull()) {
                int iconExtent = proxy()->pixelMetric(PM_SmallIconSize);
                drawItemPixmap(p, mi->rect,
                                  Qt::AlignCenter | Qt::TextHideMnemonic | Qt::TextDontClip
                                  | Qt::TextSingleLine,
                                  mi->icon.pixmap(window, QSize(iconExtent, iconExtent),
                          (mi->state & State_Enabled) ? QIcon::Normal : QIcon::Disabled));
            } else {
                drawItemText(p, mi->rect,
                                Qt::AlignCenter | Qt::TextHideMnemonic | Qt::TextDontClip
                                | Qt::TextSingleLine,
                                mi->palette, mi->state & State_Enabled,
                                mi->text, selected ? QPalette::HighlightedText : QPalette::ButtonText);
            }
        }
        break;
    case CE_ProgressBarLabel:
    case CE_ProgressBarGroove:
        // Do nothing. All done in CE_ProgressBarContents. Only keep these for proxy style overrides.
        break;
    case CE_ProgressBarContents:
        if (const QStyleOptionProgressBar *pb = qstyleoption_cast<const QStyleOptionProgressBar *>(opt)) {
            const bool isIndeterminate = (pb->minimum == 0 && pb->maximum == 0);
            const bool vertical = pb->orientation == Qt::Vertical;
            const bool inverted = pb->invertedAppearance;
            bool reverse = (!vertical && (pb->direction == Qt::RightToLeft));
            if (inverted)
                reverse = !reverse;

            QRect rect = pb->rect;
            if (vertical)
                rect = rect.transposed();
            const CGRect cgRect = rect.toCGRect();

            const auto aquaSize = d->effectiveAquaSizeConstrain(opt, w);
            const QProgressStyleAnimation *animation = qobject_cast<QProgressStyleAnimation*>(d->animation(opt->styleObject));
            QIndeterminateProgressIndicator *ipi = nil;
            if (isIndeterminate || animation)
                ipi = static_cast<QIndeterminateProgressIndicator *>(d->cocoaControl({ QMacStylePrivate::ProgressIndicator_Indeterminate, aquaSize }));
            if (isIndeterminate) {
                // QIndeterminateProgressIndicator derives from NSProgressIndicator. We use a single
                // instance that we start animating as soon as one of the progress bars is indeterminate.
                // Since they will be in sync (as it's the case in Cocoa), we just need to draw it with
                // the right geometry when the animation triggers an update. However, we can't hide it
                // entirely between frames since that would stop the animation, so we just set its alpha
                // value to 0. Same if we remove it from its superview. See QIndeterminateProgressIndicator
                // implementation for details.
                if (!animation && opt->styleObject) {
                    auto *animation = new QProgressStyleAnimation(d->animateSpeed(QMacStylePrivate::AquaProgressBar), opt->styleObject);
                    // NSProgressIndicator is heavier to draw than the HITheme API, so we reduce the frame rate a couple notches.
                    animation->setFrameRate(QStyleAnimation::FifteenFps);
                    d->startAnimation(animation);
                    [ipi startAnimation];
                }

                d->setupNSGraphicsContext(cg, NO);
                d->setupVerticalInvertedXform(cg, reverse, vertical, cgRect);
                [ipi drawWithFrame:cgRect inView:d->backingStoreNSView];
                d->restoreNSGraphicsContext(cg);
            } else {
                if (animation) {
                    d->stopAnimation(opt->styleObject);
                    [ipi stopAnimation];
                }

                const auto cw = QMacStylePrivate::CocoaControl(QMacStylePrivate::ProgressIndicator_Determinate, aquaSize);
                auto *pi = static_cast<NSProgressIndicator *>(d->cocoaControl(cw));
                d->drawNSViewInRect(pi, rect, p, ^(CGContextRef ctx, const CGRect &rect) {
                    d->setupVerticalInvertedXform(ctx, reverse, vertical, rect);
                    pi.minValue = pb->minimum;
                    pi.maxValue = pb->maximum;
                    pi.doubleValue = pb->progress;
                    [pi drawRect:rect];
                });
            }
        }
        break;
    case CE_SizeGrip: {
        // This is not HIG kosher: Fall back to the old stuff until we decide what to do.
#ifndef QT_NO_MDIAREA
        if (!w || !qobject_cast<QMdiSubWindow *>(w->parentWidget()))
#endif
            break;

        if (w->testAttribute(Qt::WA_MacOpaqueSizeGrip))
            p->fillRect(opt->rect, opt->palette.window());

        QPen lineColor = QColor(82, 82, 82, 192);
        lineColor.setWidth(1);
        p->save();
        p->setRenderHint(QPainter::Antialiasing);
        p->setPen(lineColor);
        const Qt::LayoutDirection layoutDirection = w ? w->layoutDirection() : qApp->layoutDirection();
        const int NumLines = 3;
        for (int l = 0; l < NumLines; ++l) {
            const int offset = (l * 4 + 3);
            QPoint start, end;
            if (layoutDirection == Qt::LeftToRight) {
                start = QPoint(opt->rect.width() - offset, opt->rect.height() - 1);
                end = QPoint(opt->rect.width() - 1, opt->rect.height() - offset);
            } else {
                start = QPoint(offset, opt->rect.height() - 1);
                end = QPoint(1, opt->rect.height() - offset);
            }
            p->drawLine(start, end);
        }
        p->restore();
        break;
        }
    case CE_Splitter:
        if (opt->rect.width() > 1 && opt->rect.height() > 1) {
            const bool isVertical = !(opt->state & QStyle::State_Horizontal);
            // Qt refers to the layout orientation, while Cocoa refers to the divider's.
            const auto ct = isVertical ? QMacStylePrivate::SplitView_Horizontal : QMacStylePrivate::SplitView_Vertical;
            const auto cw = QMacStylePrivate::CocoaControl(ct, QStyleHelper::SizeLarge);
            auto *sv = static_cast<NSSplitView *>(d->cocoaControl(cw));
            sv.frame = opt->rect.toCGRect();
            d->drawNSViewInRect(sv, opt->rect, p, ^(CGContextRef, const CGRect &rect) {
                [sv drawDividerInRect:rect];
            });
        } else {
            QPen oldPen = p->pen();
            p->setPen(opt->palette.dark().color());
            if (opt->state & QStyle::State_Horizontal)
                p->drawLine(opt->rect.topLeft(), opt->rect.bottomLeft());
            else
                p->drawLine(opt->rect.topLeft(), opt->rect.topRight());
            p->setPen(oldPen);
        }
        break;
    case CE_RubberBand:
        if (const QStyleOptionRubberBand *rubber = qstyleoption_cast<const QStyleOptionRubberBand *>(opt)) {
            QColor fillColor(opt->palette.color(QPalette::Disabled, QPalette::Highlight));
            if (!rubber->opaque) {
                QColor strokeColor;
                // I retrieved these colors from the Carbon-Dev mailing list
                strokeColor.setHsvF(0, 0, 0.86, 1.0);
                fillColor.setHsvF(0, 0, 0.53, 0.25);
                if (opt->rect.width() * opt->rect.height() <= 3) {
                    p->fillRect(opt->rect, strokeColor);
                } else {
                    QPen oldPen = p->pen();
                    QBrush oldBrush = p->brush();
                    QPen pen(strokeColor);
                    p->setPen(pen);
                    p->setBrush(fillColor);
                    QRect adjusted = opt->rect.adjusted(1, 1, -1, -1);
                    if (adjusted.isValid())
                        p->drawRect(adjusted);
                    p->setPen(oldPen);
                    p->setBrush(oldBrush);
                }
            } else {
                p->fillRect(opt->rect, fillColor);
            }
        }
        break;
#ifndef QT_NO_TOOLBAR
    case CE_ToolBar: {
        const QStyleOptionToolBar *toolBar = qstyleoption_cast<const QStyleOptionToolBar *>(opt);
        const bool isDarkMode = qt_mac_applicationIsInDarkMode();

        // Unified title and toolbar drawing. In this mode the cocoa platform plugin will
        // fill the top toolbar area part with a background gradient that "unifies" with
        // the title bar. The following code fills the toolBar area with transparent pixels
        // to make that gradient visible.
        if (w) {
#if QT_CONFIG(mainwindow)
            if (QMainWindow * mainWindow = qobject_cast<QMainWindow *>(w->window())) {
                if (toolBar && toolBar->toolBarArea == Qt::TopToolBarArea && mainWindow->unifiedTitleAndToolBarOnMac()) {
                    // fill with transparent pixels.
                    p->save();
                    p->setCompositionMode(QPainter::CompositionMode_Source);
                    p->fillRect(opt->rect, Qt::transparent);
                    p->restore();

                    // Draw a horizontal separator line at the toolBar bottom if the "unified" area ends here.
                    // There might be additional toolbars or other widgets such as tab bars in document
                    // mode below. Determine this by making a unified toolbar area test for the row below
                    // this toolbar.
                    const QPoint windowToolbarEnd = w->mapTo(w->window(), opt->rect.bottomLeft());
                    const bool isEndOfUnifiedArea = !isInMacUnifiedToolbarArea(w->window()->windowHandle(), windowToolbarEnd.y() + 1);
                    if (isEndOfUnifiedArea) {
                        const int margin = qt_mac_aqua_get_metric(SeparatorSize);
                        const auto separatorRect = QRect(opt->rect.left(), opt->rect.bottom(), opt->rect.width(), margin);
                        p->fillRect(separatorRect, isDarkMode ? darkModeSeparatorLine : opt->palette.dark().color());
                    }
                    break;
                }
            }
#endif
        }

        // draw background gradient
        QLinearGradient linearGrad;
        if (opt->state & State_Horizontal)
            linearGrad = QLinearGradient(0, opt->rect.top(), 0, opt->rect.bottom());
        else
            linearGrad = QLinearGradient(opt->rect.left(), 0,  opt->rect.right(), 0);

        QColor mainWindowGradientBegin = isDarkMode ? darkMainWindowGradientBegin : lightMainWindowGradientBegin;
        QColor mainWindowGradientEnd = isDarkMode ? darkMainWindowGradientEnd : lightMainWindowGradientEnd;

        linearGrad.setColorAt(0, mainWindowGradientBegin);
        linearGrad.setColorAt(1, mainWindowGradientEnd);
        p->fillRect(opt->rect, linearGrad);

        p->save();
        QRect toolbarRect = isDarkMode ? opt->rect.adjusted(0, 0, 0, 1) : opt->rect;
        if (opt->state & State_Horizontal) {
            p->setPen(isDarkMode ? darkModeSeparatorLine : mainWindowGradientBegin.lighter(114));
            p->drawLine(toolbarRect.topLeft(), toolbarRect.topRight());
            p->setPen(isDarkMode ? darkModeSeparatorLine :mainWindowGradientEnd.darker(114));
            p->drawLine(toolbarRect.bottomLeft(), toolbarRect.bottomRight());
        } else {
            p->setPen(isDarkMode ? darkModeSeparatorLine : mainWindowGradientBegin.lighter(114));
            p->drawLine(toolbarRect.topLeft(), toolbarRect.bottomLeft());
            p->setPen(isDarkMode ? darkModeSeparatorLine : mainWindowGradientEnd.darker(114));
            p->drawLine(toolbarRect.topRight(), toolbarRect.bottomRight());
        }
        p->restore();


        } break;
#endif
    default:
        QCommonStyle::drawControl(ce, opt, p, w);
        break;
    }
}

static void setLayoutItemMargins(int left, int top, int right, int bottom, QRect *rect, Qt::LayoutDirection dir)
{
    if (dir == Qt::RightToLeft) {
        rect->adjust(-right, top, -left, bottom);
    } else {
        rect->adjust(left, top, right, bottom);
    }
}

QRect QMacStyle::subElementRect(SubElement sr, const QStyleOption *opt,
                                const QWidget *widget) const
{
    Q_D(const QMacStyle);
    QRect rect;
    const int controlSize = getControlSize(opt, widget);

    switch (sr) {
#if QT_CONFIG(itemviews)
    case SE_ItemViewItemText:
        if (const QStyleOptionViewItem *vopt = qstyleoption_cast<const QStyleOptionViewItem *>(opt)) {
            int fw = proxy()->pixelMetric(PM_FocusFrameHMargin, opt, widget);
            // We add the focusframeargin between icon and text in commonstyle
            rect = QCommonStyle::subElementRect(sr, opt, widget);
            if (vopt->features & QStyleOptionViewItem::HasDecoration)
                rect.adjust(-fw, 0, 0, 0);
        }
        break;
#endif
    case SE_ToolBoxTabContents:
        rect = QCommonStyle::subElementRect(sr, opt, widget);
        break;
    case SE_PushButtonBevel:
    case SE_PushButtonContents:
        if (const QStyleOptionButton *btn = qstyleoption_cast<const QStyleOptionButton *>(opt)) {
            // Comment from the old HITheme days:
            //   "Unlike Carbon, we want the button to always be drawn inside its bounds.
            //    Therefore, the button is a bit smaller, so that even if it got focus,
            //    the focus 'shadow' will be inside. Adjust the content rect likewise."
            // In the future, we should consider using -[NSCell titleRectForBounds:].
            // Since it requires configuring the NSButton fully, i.e. frame, image,
            // title and font, we keep things more manual until we are more familiar
            // with side effects when changing NSButton state.
            const auto ct = cocoaControlType(btn, widget);
            const auto cs = d->effectiveAquaSizeConstrain(btn, widget);
            const auto cw = QMacStylePrivate::CocoaControl(ct, cs);
            auto frameRect = cw.adjustedControlFrame(btn->rect);
            if (sr == SE_PushButtonContents) {
                frameRect -= cw.titleMargins();
            } else if (cw.type != QMacStylePrivate::Button_SquareButton) {
                auto *pb = static_cast<NSButton *>(d->cocoaControl(cw));
                frameRect = QRectF::fromCGRect([pb alignmentRectForFrame:frameRect.toCGRect()]);
                if (cw.type == QMacStylePrivate::Button_PushButton)
                    frameRect -= pushButtonShadowMargins[cw.size];
                else if (cw.type == QMacStylePrivate::Button_PullDown)
                    frameRect -= pullDownButtonShadowMargins[cw.size];
            }
            rect = frameRect.toRect();
        }
        break;
    case SE_HeaderLabel: {
        int margin = proxy()->pixelMetric(QStyle::PM_HeaderMargin, opt, widget);
        rect.setRect(opt->rect.x() + margin, opt->rect.y(),
                  opt->rect.width() - margin * 2, opt->rect.height() - 2);
        if (const QStyleOptionHeader *header = qstyleoption_cast<const QStyleOptionHeader *>(opt)) {
            // Subtract width needed for arrow, if there is one
            if (header->sortIndicator != QStyleOptionHeader::None) {
                if (opt->state & State_Horizontal)
                    rect.setWidth(rect.width() - (headerSectionArrowHeight) - (margin * 2));
                else
                    rect.setHeight(rect.height() - (headerSectionArrowHeight) - (margin * 2));
            }
        }
        rect = visualRect(opt->direction, opt->rect, rect);
        break;
    }
    case SE_HeaderArrow: {
        int h = opt->rect.height();
        int w = opt->rect.width();
        int x = opt->rect.x();
        int y = opt->rect.y();
        int margin = proxy()->pixelMetric(QStyle::PM_HeaderMargin, opt, widget);

        if (opt->state & State_Horizontal) {
            rect.setRect(x + w - margin * 2 - headerSectionArrowHeight, y + 5,
                      headerSectionArrowHeight, h - margin * 2 - 5);
        } else {
            rect.setRect(x + 5, y + h - margin * 2 - headerSectionArrowHeight,
                      w - margin * 2 - 5, headerSectionArrowHeight);
        }
        rect = visualRect(opt->direction, opt->rect, rect);
        break;
    }
    case SE_ProgressBarGroove:
        // Wrong in the secondary dimension, but accurate enough in the main dimension.
        rect  = opt->rect;
        break;
    case SE_ProgressBarLabel:
        break;
    case SE_ProgressBarContents:
        rect = opt->rect;
        break;
    case SE_TreeViewDisclosureItem: {
        rect = opt->rect;
        // As previously returned by HIThemeGetButtonContentBounds
        rect.setLeft(rect.left() + 2 + DisclosureOffset);
        break;
    }
#if QT_CONFIG(tabwidget)
    case SE_TabWidgetLeftCorner:
        if (const QStyleOptionTabWidgetFrame *twf
                = qstyleoption_cast<const QStyleOptionTabWidgetFrame *>(opt)) {
            switch (twf->shape) {
            case QTabBar::RoundedNorth:
            case QTabBar::TriangularNorth:
                rect = QRect(QPoint(0, 0), twf->leftCornerWidgetSize);
                break;
            case QTabBar::RoundedSouth:
            case QTabBar::TriangularSouth:
                rect = QRect(QPoint(0, twf->rect.height() - twf->leftCornerWidgetSize.height()),
                          twf->leftCornerWidgetSize);
                break;
            default:
                break;
            }
            rect = visualRect(twf->direction, twf->rect, rect);
        }
        break;
    case SE_TabWidgetRightCorner:
        if (const QStyleOptionTabWidgetFrame *twf
                = qstyleoption_cast<const QStyleOptionTabWidgetFrame *>(opt)) {
            switch (twf->shape) {
            case QTabBar::RoundedNorth:
            case QTabBar::TriangularNorth:
                rect = QRect(QPoint(twf->rect.width() - twf->rightCornerWidgetSize.width(), 0),
                          twf->rightCornerWidgetSize);
                break;
            case QTabBar::RoundedSouth:
            case QTabBar::TriangularSouth:
                rect = QRect(QPoint(twf->rect.width() - twf->rightCornerWidgetSize.width(),
                                 twf->rect.height() - twf->rightCornerWidgetSize.height()),
                          twf->rightCornerWidgetSize);
                break;
            default:
                break;
            }
            rect = visualRect(twf->direction, twf->rect, rect);
        }
        break;
    case SE_TabWidgetTabContents:
        rect = QCommonStyle::subElementRect(sr, opt, widget);
        if (const auto *twf = qstyleoption_cast<const QStyleOptionTabWidgetFrame *>(opt)) {
            if (twf->lineWidth != 0) {
                switch (QMacStylePrivate::tabDirection(twf->shape)) {
                case QMacStylePrivate::North:
                    rect.adjust(+1, +14, -1, -1);
                    break;
                case QMacStylePrivate::South:
                    rect.adjust(+1, +1, -1, -14);
                    break;
                case QMacStylePrivate::West:
                    rect.adjust(+14, +1, -1, -1);
                    break;
                case QMacStylePrivate::East:
                    rect.adjust(+1, +1, -14, -1);
                }
            }
        }
        break;
    case SE_TabBarTabText:
        if (const QStyleOptionTab *tab = qstyleoption_cast<const QStyleOptionTab *>(opt)) {
            QRect dummyIconRect;
            d->tabLayout(tab, widget, &rect, &dummyIconRect);
        }
        break;
    case SE_TabBarTabLeftButton:
    case SE_TabBarTabRightButton:
        if (const QStyleOptionTab *tab = qstyleoption_cast<const QStyleOptionTab *>(opt)) {
            bool selected = tab->state & State_Selected;
            int verticalShift = proxy()->pixelMetric(QStyle::PM_TabBarTabShiftVertical, tab, widget);
            int horizontalShift = proxy()->pixelMetric(QStyle::PM_TabBarTabShiftHorizontal, tab, widget);
            int hpadding = 5;

            bool verticalTabs = tab->shape == QTabBar::RoundedEast
                    || tab->shape == QTabBar::RoundedWest
                    || tab->shape == QTabBar::TriangularEast
                    || tab->shape == QTabBar::TriangularWest;

            QRect tr = tab->rect;
            if (tab->shape == QTabBar::RoundedSouth || tab->shape == QTabBar::TriangularSouth)
                verticalShift = -verticalShift;
            if (verticalTabs) {
                qSwap(horizontalShift, verticalShift);
                horizontalShift *= -1;
                verticalShift *= -1;
            }
            if (tab->shape == QTabBar::RoundedWest || tab->shape == QTabBar::TriangularWest)
                horizontalShift = -horizontalShift;

            tr.adjust(0, 0, horizontalShift, verticalShift);
            if (selected)
            {
                tr.setBottom(tr.bottom() - verticalShift);
                tr.setRight(tr.right() - horizontalShift);
            }

            QSize size = (sr == SE_TabBarTabLeftButton) ? tab->leftButtonSize : tab->rightButtonSize;
            int w = size.width();
            int h = size.height();
            int midHeight = static_cast<int>(qCeil(float(tr.height() - h) / 2));
            int midWidth = ((tr.width() - w) / 2);

            bool atTheTop = true;
            switch (tab->shape) {
            case QTabBar::RoundedWest:
            case QTabBar::TriangularWest:
                atTheTop = (sr == SE_TabBarTabLeftButton);
                break;
            case QTabBar::RoundedEast:
            case QTabBar::TriangularEast:
                atTheTop = (sr == SE_TabBarTabRightButton);
                break;
            default:
                if (sr == SE_TabBarTabLeftButton)
                    rect = QRect(tab->rect.x() + hpadding, midHeight, w, h);
                else
                    rect = QRect(tab->rect.right() - w - hpadding, midHeight, w, h);
                rect = visualRect(tab->direction, tab->rect, rect);
            }
            if (verticalTabs) {
                if (atTheTop)
                    rect = QRect(midWidth, tr.y() + tab->rect.height() - hpadding - h, w, h);
                else
                    rect = QRect(midWidth, tr.y() + hpadding, w, h);
            }
        }
        break;
#endif
    case SE_LineEditContents:
        rect = QCommonStyle::subElementRect(sr, opt, widget);
#if QT_CONFIG(combobox)
        if (widget && qobject_cast<const QComboBox*>(widget->parentWidget()))
            rect.adjust(-1, -2, 0, 0);
        else
#endif
            rect.adjust(-1, -1, 0, +1);
        break;
    case SE_CheckBoxLayoutItem:
        rect = opt->rect;
        if (controlSize == QStyleHelper::SizeLarge) {
            setLayoutItemMargins(+2, +3, -9, -4, &rect, opt->direction);
        } else if (controlSize == QStyleHelper::SizeSmall) {
            setLayoutItemMargins(+1, +5, 0 /* fix */, -6, &rect, opt->direction);
        } else {
            setLayoutItemMargins(0, +7, 0 /* fix */, -6, &rect, opt->direction);
        }
        break;
    case SE_ComboBoxLayoutItem:
#ifndef QT_NO_TOOLBAR
        if (widget && qobject_cast<QToolBar *>(widget->parentWidget())) {
            // Do nothing, because QToolbar needs the entire widget rect.
            // Otherwise it will be clipped. Equivalent to
            // widget->setAttribute(Qt::WA_LayoutUsesWidgetRect), but without
            // all the hassle.
        } else
#endif
        {
            rect = opt->rect;
            if (controlSize == QStyleHelper::SizeLarge) {
                rect.adjust(+3, +2, -3, -4);
            } else if (controlSize == QStyleHelper::SizeSmall) {
                setLayoutItemMargins(+2, +1, -3, -4, &rect, opt->direction);
            } else {
                setLayoutItemMargins(+1, 0, -2, 0, &rect, opt->direction);
            }
        }
        break;
    case SE_LabelLayoutItem:
        rect = opt->rect;
        setLayoutItemMargins(+1, 0 /* SHOULD be -1, done for alignment */, 0, 0 /* SHOULD be -1, done for alignment */, &rect, opt->direction);
        break;
    case SE_ProgressBarLayoutItem: {
        rect = opt->rect;
        int bottom = SIZE(3, 8, 8);
        if (opt->state & State_Horizontal) {
            rect.adjust(0, +1, 0, -bottom);
        } else {
            setLayoutItemMargins(+1, 0, -bottom, 0, &rect, opt->direction);
        }
        break;
    }
    case SE_PushButtonLayoutItem:
        if (const QStyleOptionButton *buttonOpt
                = qstyleoption_cast<const QStyleOptionButton *>(opt)) {
            if ((buttonOpt->features & QStyleOptionButton::Flat))
                break;  // leave rect alone
        }
        rect = opt->rect;
        if (controlSize == QStyleHelper::SizeLarge) {
            rect.adjust(+6, +4, -6, -8);
        } else if (controlSize == QStyleHelper::SizeSmall) {
            rect.adjust(+5, +4, -5, -6);
        } else {
            rect.adjust(+1, 0, -1, -2);
        }
        break;
    case SE_RadioButtonLayoutItem:
        rect = opt->rect;
        if (controlSize == QStyleHelper::SizeLarge) {
            setLayoutItemMargins(+2, +2 /* SHOULD BE +3, done for alignment */,
                                 0, -4 /* SHOULD BE -3, done for alignment */, &rect, opt->direction);
        } else if (controlSize == QStyleHelper::SizeSmall) {
            rect.adjust(0, +6, 0 /* fix */, -5);
        } else {
            rect.adjust(0, +6, 0 /* fix */, -7);
        }
        break;
    case SE_SliderLayoutItem:
        if (const QStyleOptionSlider *sliderOpt
                = qstyleoption_cast<const QStyleOptionSlider *>(opt)) {
            rect = opt->rect;
            if (sliderOpt->tickPosition == QSlider::NoTicks) {
                int above = SIZE(3, 0, 2);
                int below = SIZE(4, 3, 0);
                if (sliderOpt->orientation == Qt::Horizontal) {
                    rect.adjust(0, +above, 0, -below);
                } else {
                    rect.adjust(+above, 0, -below, 0);  //### Seems that QSlider flip the position of the ticks in reverse mode.
                }
            } else if (sliderOpt->tickPosition == QSlider::TicksAbove) {
                int below = SIZE(3, 2, 0);
                if (sliderOpt->orientation == Qt::Horizontal) {
                    rect.setHeight(rect.height() - below);
                } else {
                    rect.setWidth(rect.width() - below);
                }
            } else if (sliderOpt->tickPosition == QSlider::TicksBelow) {
                int above = SIZE(3, 2, 0);
                if (sliderOpt->orientation == Qt::Horizontal) {
                    rect.setTop(rect.top() + above);
                } else {
                    rect.setLeft(rect.left() + above);
                }
            }
        }
        break;
    case SE_FrameLayoutItem:
        // hack because QStyleOptionFrame doesn't have a frameStyle member
        if (const QFrame *frame = qobject_cast<const QFrame *>(widget)) {
            rect = opt->rect;
            switch (frame->frameStyle() & QFrame::Shape_Mask) {
            case QFrame::HLine:
                rect.adjust(0, +1, 0, -1);
                break;
            case QFrame::VLine:
                rect.adjust(+1, 0, -1, 0);
                break;
            default:
                ;
            }
        }
        break;
    case SE_GroupBoxLayoutItem:
        rect = opt->rect;
        if (const QStyleOptionGroupBox *groupBoxOpt =
                qstyleoption_cast<const QStyleOptionGroupBox *>(opt)) {
            /*
                AHIG is very inconsistent when it comes to group boxes.
                Basically, we make sure that (non-checkable) group boxes
                and tab widgets look good when laid out side by side.
            */
            if (groupBoxOpt->subControls & (QStyle::SC_GroupBoxCheckBox
                                            | QStyle::SC_GroupBoxLabel)) {
                int delta;
                if (groupBoxOpt->subControls & QStyle::SC_GroupBoxCheckBox) {
                    delta = SIZE(8, 4, 4);       // guess
                } else {
                    delta = SIZE(15, 12, 12);    // guess
                }
                rect.setTop(rect.top() + delta);
            }
        }
        rect.setBottom(rect.bottom() - 1);
        break;
#if QT_CONFIG(tabwidget)
    case SE_TabWidgetLayoutItem:
        if (const QStyleOptionTabWidgetFrame *tabWidgetOpt =
                qstyleoption_cast<const QStyleOptionTabWidgetFrame *>(opt)) {
            /*
                AHIG specifies "12 or 14" as the distance from the window
                edge. We choose 14 and since the default top margin is 20,
                the overlap is 6.
            */
            rect = tabWidgetOpt->rect;
            if (tabWidgetOpt->shape == QTabBar::RoundedNorth)
                rect.setTop(rect.top() + SIZE(6 /* AHIG */, 3 /* guess */, 2 /* AHIG */));
        }
        break;
#endif
#if QT_CONFIG(dockwidget)
        case SE_DockWidgetCloseButton:
        case SE_DockWidgetFloatButton:
        case SE_DockWidgetTitleBarText:
        case SE_DockWidgetIcon: {
            int iconSize = proxy()->pixelMetric(PM_SmallIconSize, opt, widget);
            int buttonMargin = proxy()->pixelMetric(PM_DockWidgetTitleBarButtonMargin, opt, widget);
            QRect srect = opt->rect;

            const QStyleOptionDockWidget *dwOpt
                = qstyleoption_cast<const QStyleOptionDockWidget*>(opt);
            bool canClose = dwOpt == 0 ? true : dwOpt->closable;
            bool canFloat = dwOpt == 0 ? false : dwOpt->floatable;

            const bool verticalTitleBar = dwOpt->verticalTitleBar;

            // If this is a vertical titlebar, we transpose and work as if it was
            // horizontal, then transpose again.
            if (verticalTitleBar)
                srect = srect.transposed();

            do {
                int right = srect.right();
                int left = srect.left();

                QRect closeRect;
                if (canClose) {
                    QSize sz = proxy()->standardIcon(QStyle::SP_TitleBarCloseButton,
                                            opt, widget).actualSize(QSize(iconSize, iconSize));
                    sz += QSize(buttonMargin, buttonMargin);
                    if (verticalTitleBar)
                        sz = sz.transposed();
                    closeRect = QRect(left,
                                      srect.center().y() - sz.height()/2,
                                      sz.width(), sz.height());
                    left = closeRect.right() + 1;
                }
                if (sr == SE_DockWidgetCloseButton) {
                    rect = closeRect;
                    break;
                }

                QRect floatRect;
                if (canFloat) {
                    QSize sz = proxy()->standardIcon(QStyle::SP_TitleBarNormalButton,
                                            opt, widget).actualSize(QSize(iconSize, iconSize));
                    sz += QSize(buttonMargin, buttonMargin);
                    if (verticalTitleBar)
                        sz = sz.transposed();
                    floatRect = QRect(left,
                                      srect.center().y() - sz.height()/2,
                                      sz.width(), sz.height());
                    left = floatRect.right() + 1;
                }
                if (sr == SE_DockWidgetFloatButton) {
                    rect = floatRect;
                    break;
                }

                QRect iconRect;
                if (const QDockWidget *dw = qobject_cast<const QDockWidget*>(widget)) {
                    QIcon icon;
                    if (dw->isFloating())
                        icon = dw->windowIcon();
                    if (!icon.isNull()
                        && icon.cacheKey() != QApplication::windowIcon().cacheKey()) {
                        QSize sz = icon.actualSize(QSize(rect.height(), rect.height()));
                        if (verticalTitleBar)
                            sz = sz.transposed();
                        iconRect = QRect(right - sz.width(), srect.center().y() - sz.height()/2,
                                         sz.width(), sz.height());
                        right = iconRect.left() - 1;
                    }
                }
                if (sr == SE_DockWidgetIcon) {
                    rect = iconRect;
                    break;
                }

                QRect textRect = QRect(left, srect.top(),
                                       right - left, srect.height());
                if (sr == SE_DockWidgetTitleBarText) {
                    rect = textRect;
                    break;
                }
            } while (false);

            if (verticalTitleBar) {
                rect = QRect(srect.left() + rect.top() - srect.top(),
                          srect.top() + srect.right() - rect.right(),
                          rect.height(), rect.width());
            } else {
                rect = visualRect(opt->direction, srect, rect);
            }
            break;
        }
#endif
    default:
        rect = QCommonStyle::subElementRect(sr, opt, widget);
        break;
    }
    return rect;
}

void QMacStylePrivate::drawToolbarButtonArrow(const QStyleOption *opt, QPainter *p) const
{
    Q_Q(const QMacStyle);
    QStyleOption arrowOpt = *opt;
    arrowOpt.rect = QRect(opt->rect.right() - (toolButtonArrowSize + toolButtonArrowMargin),
                          opt->rect.bottom() - (toolButtonArrowSize + toolButtonArrowMargin),
                          toolButtonArrowSize,
                          toolButtonArrowSize);
    q->proxy()->drawPrimitive(QStyle::PE_IndicatorArrowDown, &arrowOpt, p);
}

void QMacStylePrivate::setupNSGraphicsContext(CGContextRef cg, bool flipped) const
{
    CGContextSaveGState(cg);
    [NSGraphicsContext saveGraphicsState];

    [NSGraphicsContext setCurrentContext:
        [NSGraphicsContext graphicsContextWithCGContext:cg flipped:flipped]];
}

void QMacStylePrivate::restoreNSGraphicsContext(CGContextRef cg) const
{
    [NSGraphicsContext restoreGraphicsState];
    CGContextRestoreGState(cg);
}

void QMacStyle::drawComplexControl(ComplexControl cc, const QStyleOptionComplex *opt, QPainter *p,
                                   const QWidget *widget) const
{
    Q_D(const QMacStyle);
    const AppearanceSync sync;
    QMacCGContext cg(p);
    QWindow *window = widget && widget->window() ? widget->window()->windowHandle() : nullptr;
    d->resolveCurrentNSView(window);
    switch (cc) {
    case CC_ScrollBar:
        if (const QStyleOptionSlider *sb = qstyleoption_cast<const QStyleOptionSlider *>(opt)) {

            const bool drawTrack = sb->subControls & SC_ScrollBarGroove;
            const bool drawKnob = sb->subControls & SC_ScrollBarSlider;
            if (!drawTrack && !drawKnob)
                break;

            const bool isHorizontal = sb->orientation == Qt::Horizontal;

            if (opt && opt->styleObject && !QMacStylePrivate::scrollBars.contains(opt->styleObject))
                QMacStylePrivate::scrollBars.append(QPointer<QObject>(opt->styleObject));

            static const CGFloat knobWidths[] = { 7.0, 5.0, 5.0 };
            static const CGFloat expandedKnobWidths[] = { 11.0, 9.0, 9.0 };
            const auto cocoaSize = d->effectiveAquaSizeConstrain(opt, widget);
            const CGFloat maxExpandScale = expandedKnobWidths[cocoaSize] / knobWidths[cocoaSize];

            const bool isTransient = proxy()->styleHint(SH_ScrollBar_Transient, opt, widget);
            if (!isTransient)
                d->stopAnimation(opt->styleObject);
            bool wasActive = false;
            CGFloat opacity = 0.0;
            CGFloat expandScale = 1.0;
            CGFloat expandOffset = 0.0;
            bool shouldExpand = false;

            if (QObject *styleObject = opt->styleObject) {
                const int oldPos = styleObject->property("_q_stylepos").toInt();
                const int oldMin = styleObject->property("_q_stylemin").toInt();
                const int oldMax = styleObject->property("_q_stylemax").toInt();
                const QRect oldRect = styleObject->property("_q_stylerect").toRect();
                const QStyle::State oldState = static_cast<QStyle::State>(styleObject->property("_q_stylestate").value<QStyle::State::Int>());
                const uint oldActiveControls = styleObject->property("_q_stylecontrols").toUInt();

                // a scrollbar is transient when the scrollbar itself and
                // its sibling are both inactive (ie. not pressed/hovered/moved)
                const bool transient = isTransient && !opt->activeSubControls && !(sb->state & State_On);

                if (!transient ||
                        oldPos != sb->sliderPosition ||
                        oldMin != sb->minimum ||
                        oldMax != sb->maximum ||
                        oldRect != sb->rect ||
                        oldState != sb->state ||
                        oldActiveControls != sb->activeSubControls) {

                    // if the scrollbar is transient or its attributes, geometry or
                    // state has changed, the opacity is reset back to 100% opaque
                    opacity = 1.0;

                    styleObject->setProperty("_q_stylepos", sb->sliderPosition);
                    styleObject->setProperty("_q_stylemin", sb->minimum);
                    styleObject->setProperty("_q_stylemax", sb->maximum);
                    styleObject->setProperty("_q_stylerect", sb->rect);
                    styleObject->setProperty("_q_stylestate", static_cast<QStyle::State::Int>(sb->state));
                    styleObject->setProperty("_q_stylecontrols", static_cast<uint>(sb->activeSubControls));

                    QScrollbarStyleAnimation *anim  = qobject_cast<QScrollbarStyleAnimation *>(d->animation(styleObject));
                    if (transient) {
                        if (!anim) {
                            anim = new QScrollbarStyleAnimation(QScrollbarStyleAnimation::Deactivating, styleObject);
                            d->startAnimation(anim);
                        } else if (anim->mode() == QScrollbarStyleAnimation::Deactivating) {
                            // the scrollbar was already fading out while the
                            // state changed -> restart the fade out animation
                            anim->setCurrentTime(0);
                        }
                    } else if (anim && anim->mode() == QScrollbarStyleAnimation::Deactivating) {
                        d->stopAnimation(styleObject);
                    }
                }

                QScrollbarStyleAnimation *anim = qobject_cast<QScrollbarStyleAnimation *>(d->animation(styleObject));
                if (anim && anim->mode() == QScrollbarStyleAnimation::Deactivating) {
                    // once a scrollbar was active (hovered/pressed), it retains
                    // the active look even if it's no longer active while fading out
                    if (oldActiveControls)
                        anim->setActive(true);

                    wasActive = anim->wasActive();
                    opacity = anim->currentValue();
                }

                shouldExpand = isTransient && (opt->activeSubControls || wasActive);
                if (shouldExpand) {
                    if (!anim && !oldActiveControls) {
                        // Start expand animation only once and when entering
                        anim = new QScrollbarStyleAnimation(QScrollbarStyleAnimation::Activating, styleObject);
                        d->startAnimation(anim);
                    }
                    if (anim && anim->mode() == QScrollbarStyleAnimation::Activating) {
                        expandScale = 1.0 + (maxExpandScale - 1.0) * anim->currentValue();
                        expandOffset = 5.5 * (1.0 - anim->currentValue());
                    } else {
                        // Keep expanded state after the animation ends, and when fading out
                        expandScale = maxExpandScale;
                        expandOffset = 0.0;
                    }
                }
            }

            d->setupNSGraphicsContext(cg, NO /* flipped */);

            const auto controlType = isHorizontal ? QMacStylePrivate::Scroller_Horizontal : QMacStylePrivate::Scroller_Vertical;
            const auto cw = QMacStylePrivate::CocoaControl(controlType, cocoaSize);
            NSScroller *scroller = static_cast<NSScroller *>(d->cocoaControl(cw));

            const QColor bgColor = QStyleHelper::backgroundColor(opt->palette, widget);
            const bool hasDarkBg = bgColor.red() < 128 && bgColor.green() < 128 && bgColor.blue() < 128;
            if (isTransient) {
                // macOS behavior: as soon as one color channel is >= 128,
                // the background is considered bright, scroller is dark.
                scroller.knobStyle = hasDarkBg? NSScrollerKnobStyleLight : NSScrollerKnobStyleDark;
            } else {
                scroller.knobStyle = NSScrollerKnobStyleDefault;
            }

            scroller.scrollerStyle = isTransient ? NSScrollerStyleOverlay : NSScrollerStyleLegacy;

            if (!setupScroller(scroller, sb))
                break;

            if (isTransient) {
                CGContextBeginTransparencyLayerWithRect(cg, scroller.frame, nullptr);
                CGContextSetAlpha(cg, opacity);
            }

            if (drawTrack) {
                // Draw the track when hovering. Expand by shifting the track rect.
                if (!isTransient || opt->activeSubControls || wasActive) {
                    CGRect trackRect = scroller.bounds;
                    if (isHorizontal)
                        trackRect.origin.y += expandOffset;
                    else
                        trackRect.origin.x += expandOffset;
                    [scroller drawKnobSlotInRect:trackRect highlight:NO];
                }
            }

            if (drawKnob) {
                if (shouldExpand) {
                    // -[NSScroller drawKnob] is not useful here because any scaling applied
                    // will only be used to draw the hi-DPI artwork. And even if did scale,
                    // the stretched knob would look wrong, actually. So we need to draw the
                    // scroller manually when it's being hovered.
                    const CGFloat scrollerWidth = [NSScroller scrollerWidthForControlSize:scroller.controlSize scrollerStyle:scroller.scrollerStyle];
                    const CGFloat knobWidth = knobWidths[cocoaSize] * expandScale;
                    // Cocoa can help get the exact knob length in the current orientation
                    const CGRect scrollerKnobRect = CGRectInset([scroller rectForPart:NSScrollerKnob], 1, 1);
                    const CGFloat knobLength = isHorizontal ? scrollerKnobRect.size.width : scrollerKnobRect.size.height;
                    const CGFloat knobPos = isHorizontal ? scrollerKnobRect.origin.x : scrollerKnobRect.origin.y;
                    const CGFloat knobOffset = qRound((scrollerWidth + expandOffset - knobWidth) / 2.0);
                    const CGFloat knobRadius = knobWidth / 2.0;
                    CGRect knobRect;
                    if (isHorizontal)
                        knobRect = CGRectMake(knobPos, knobOffset, knobLength, knobWidth);
                    else
                        knobRect = CGRectMake(knobOffset, knobPos, knobWidth, knobLength);
                    QCFType<CGPathRef> knobPath = CGPathCreateWithRoundedRect(knobRect, knobRadius, knobRadius, nullptr);
                    CGContextAddPath(cg, knobPath);
                    CGContextSetAlpha(cg, 0.5);
                    CGColorRef knobColor = hasDarkBg ? NSColor.whiteColor.CGColor : NSColor.blackColor.CGColor;
                    CGContextSetFillColorWithColor(cg, knobColor);
                    CGContextFillPath(cg);
                } else {
                    [scroller drawKnob];

                    if (!isTransient && opt->activeSubControls) {
                        // The knob should appear darker (going from 0.76 down to 0.49).
                        // But no blending mode can help darken enough in a single pass,
                        // so we resort to drawing the knob twice with a small help from
                        // blending. This brings the gray level to a close enough 0.53.
                        CGContextSetBlendMode(cg, kCGBlendModePlusDarker);
                        [scroller drawKnob];
                    }
                }
            }

            if (isTransient)
                CGContextEndTransparencyLayer(cg);

            d->restoreNSGraphicsContext(cg);
        }
        break;
    case CC_Slider:
        if (const QStyleOptionSlider *sl = qstyleoption_cast<const QStyleOptionSlider *>(opt)) {
            const bool isHorizontal = sl->orientation == Qt::Horizontal;
            const auto ct = isHorizontal ? QMacStylePrivate::Slider_Horizontal : QMacStylePrivate::Slider_Vertical;
            const auto cs = d->effectiveAquaSizeConstrain(opt, widget);
            const auto cw = QMacStylePrivate::CocoaControl(ct, cs);
            auto *slider = static_cast<NSSlider *>(d->cocoaControl(cw));
            if (!setupSlider(slider, sl))
                break;

            const bool hasTicks = sl->tickPosition != QSlider::NoTicks;
            const bool hasDoubleTicks = sl->tickPosition == QSlider::TicksBothSides;
            const bool drawKnob = sl->subControls & SC_SliderHandle;
            const bool drawBar = sl->subControls & SC_SliderGroove;
            const bool drawTicks = sl->subControls & SC_SliderTickmarks;
            const bool isPressed = sl->state & State_Sunken;

            CGPoint pressPoint;
            if (isPressed) {
                const CGRect knobRect = [slider.cell knobRectFlipped:slider.isFlipped];
                pressPoint.x = CGRectGetMidX(knobRect);
                pressPoint.y = CGRectGetMidY(knobRect);
                [slider.cell startTrackingAt:pressPoint inView:slider];
            }

            d->drawNSViewInRect(slider, opt->rect, p, ^(CGContextRef ctx, const CGRect &rect) {

                // Since the GC is flipped, upsideDown means *not* inverted when vertical.
                const bool verticalFlip = !isHorizontal && !sl->upsideDown; // FIXME: && !isSierraOrLater

                if (isHorizontal) {
                    if (sl->upsideDown) {
                        CGContextTranslateCTM(ctx, rect.size.width, rect.origin.y);
                        CGContextScaleCTM(ctx, -1, 1);
                    } else {
                        CGContextTranslateCTM(ctx, 0, rect.origin.y);
                    }
                } else if (verticalFlip) {
                    CGContextTranslateCTM(ctx, rect.origin.x, rect.size.height);
                    CGContextScaleCTM(ctx, 1, -1);
                }

                if (hasDoubleTicks) {
                    // This ain't HIG kosher: eye-proved constants
                    if (isHorizontal)
                        CGContextTranslateCTM(ctx, 0, 4);
                    else
                        CGContextTranslateCTM(ctx, 1, 0);
                }

#if 0
                // FIXME: Sadly, this part doesn't work. It seems to somehow polute the
                // NSSlider's internal state and, when we need to use the "else" part,
                // the slider's frame is not in sync with its cell dimensions.
                const bool drawAllParts = drawKnob && drawBar && (!hasTicks || drawTicks);
                if (drawAllParts && !hasDoubleTicks && (!verticalFlip || drawTicks)) {
                    // Draw eveything at once if we're going to, except for inverted vertical
                    // sliders which need to be drawn part by part because of the shadow below
                    // the knob. Same for two-sided tickmarks.
                    if (verticalFlip && drawTicks) {
                        // Since tickmarks are always rendered symmetrically, a vertically
                        // flipped slider with tickmarks only needs to get its value flipped.
                        slider.intValue = slider.maxValue - slider.intValue + slider.minValue;
                    }
                    [slider drawRect:CGRectZero];
                } else
#endif
                {
                    NSSliderCell *cell = slider.cell;

                    const int numberOfTickMarks = slider.numberOfTickMarks;
                    // This ain't HIG kosher: force tick-less bar position.
                    if (hasDoubleTicks)
                        slider.numberOfTickMarks = 0;

                    const CGRect barRect = [cell barRectFlipped:slider.isFlipped];
                    if (drawBar) {
                        if (!isHorizontal && !sl->upsideDown && (hasDoubleTicks || !hasTicks)) {
                            // The logic behind verticalFlip and upsideDown is the twisted one.
                            // Bar is the only part of the cell affected by this 'flipped'
                            // parameter in the call below, all other parts (knob, etc.) 'fixed'
                            // by scaling/translating. With ticks on one side it's not a problem
                            // at all - the bar is gray anyway. Without ticks or with ticks on
                            // the both sides, for inverted appearance and vertical orientation -
                            // we must flip so that knob and blue filling work in accordance.
                            [cell drawBarInside:barRect flipped:true];
                        } else {
                            [cell drawBarInside:barRect flipped:!verticalFlip];
                        }
                        // This ain't HIG kosher: force unfilled bar look.
                        if (hasDoubleTicks)
                            slider.numberOfTickMarks = numberOfTickMarks;
                    }

                    if (hasTicks && drawTicks) {
                        if (!drawBar && hasDoubleTicks)
                            slider.numberOfTickMarks = numberOfTickMarks;

                        [cell drawTickMarks];

                        if (hasDoubleTicks) {
                            // This ain't HIG kosher: just slap a set of tickmarks on each side, like we used to.
                            CGAffineTransform tickMarksFlip;
                            const CGRect tickMarkRect = [cell rectOfTickMarkAtIndex:0];
                            if (isHorizontal) {
                                tickMarksFlip = CGAffineTransformMakeTranslation(0, rect.size.height - tickMarkRect.size.height - 3);
                                tickMarksFlip = CGAffineTransformScale(tickMarksFlip, 1, -1);
                            } else {
                                tickMarksFlip = CGAffineTransformMakeTranslation(rect.size.width - tickMarkRect.size.width / 2, 0);
                                tickMarksFlip = CGAffineTransformScale(tickMarksFlip, -1, 1);
                            }
                            CGContextConcatCTM(ctx, tickMarksFlip);
                            [cell drawTickMarks];
                            CGContextConcatCTM(ctx, CGAffineTransformInvert(tickMarksFlip));
                        }
                    }

                    if (drawKnob) {
                        // This ain't HIG kosher: force round knob look.
                        if (hasDoubleTicks)
                            slider.numberOfTickMarks = 0;
                        [cell drawKnob];
                    }
                }
            });

            if (isPressed)
                [slider.cell stopTracking:pressPoint at:pressPoint inView:slider mouseIsUp:NO];
        }
        break;
#if QT_CONFIG(spinbox)
    case CC_SpinBox:
        if (const QStyleOptionSpinBox *sb = qstyleoption_cast<const QStyleOptionSpinBox *>(opt)) {
            if (sb->frame && (sb->subControls & SC_SpinBoxFrame)) {
                const auto lineEditRect = proxy()->subControlRect(CC_SpinBox, sb, SC_SpinBoxEditField, widget);
                QStyleOptionFrame frame;
                static_cast<QStyleOption &>(frame) = *opt;
                frame.rect = lineEditRect;
                frame.state |= State_Sunken;
                frame.lineWidth = 1;
                frame.midLineWidth = 0;
                frame.features = QStyleOptionFrame::None;
                frame.frameShape = QFrame::Box;
                drawPrimitive(PE_FrameLineEdit, &frame, p, widget);
            }
            if (sb->subControls & (SC_SpinBoxUp | SC_SpinBoxDown)) {
                const QRect updown = proxy()->subControlRect(CC_SpinBox, sb, SC_SpinBoxUp, widget)
                                   | proxy()->subControlRect(CC_SpinBox, sb, SC_SpinBoxDown, widget);

                d->setupNSGraphicsContext(cg, NO);

                const auto aquaSize = d->effectiveAquaSizeConstrain(opt, widget);
                const auto cw = QMacStylePrivate::CocoaControl(QMacStylePrivate::Stepper, aquaSize);
                NSStepperCell *cell = static_cast<NSStepperCell *>(d->cocoaCell(cw));
                cell.enabled = (sb->state & State_Enabled);

                const CGRect newRect = [cell drawingRectForBounds:updown.toCGRect()];

                const bool upPressed = sb->activeSubControls == SC_SpinBoxUp && (sb->state & State_Sunken);
                const bool downPressed = sb->activeSubControls == SC_SpinBoxDown && (sb->state & State_Sunken);
                const CGFloat x = CGRectGetMidX(newRect);
                const CGFloat y = upPressed ? -3 : 3; // Weird coordinate shift going on. Verified with Hopper
                const CGPoint pressPoint = CGPointMake(x, y);
                // Pretend we're pressing the mouse on the right button. Unfortunately, NSStepperCell has no
                // API to highlight a specific button. The highlighted property works only on the down button.
                if (upPressed || downPressed)
                    [cell startTrackingAt:pressPoint inView:d->backingStoreNSView];

                [cell drawWithFrame:newRect inView:d->backingStoreNSView];

                if (upPressed || downPressed)
                    [cell stopTracking:pressPoint at:pressPoint inView:d->backingStoreNSView mouseIsUp:NO];

                d->restoreNSGraphicsContext(cg);
            }
        }
        break;
#endif
#if QT_CONFIG(combobox)
    case CC_ComboBox:
        if (const auto *combo = qstyleoption_cast<const QStyleOptionComboBox *>(opt)) {
            const bool isEnabled = combo->state & State_Enabled;
            const bool isPressed = combo->state & State_Sunken;

            const auto ct = cocoaControlType(combo, widget);
            const auto cs = d->effectiveAquaSizeConstrain(combo, widget);
            const auto cw = QMacStylePrivate::CocoaControl(ct, cs);
            auto *cc = static_cast<NSControl *>(d->cocoaControl(cw));
            cc.enabled = isEnabled;
            QRectF frameRect = cw.adjustedControlFrame(combo->rect);;
            if (cw.type == QMacStylePrivate::Button_PopupButton) {
                // Non-editable QComboBox
                auto *pb = static_cast<NSPopUpButton *>(cc);
                // FIXME Old offsets. Try to move to adjustedControlFrame()
                if (cw.size == QStyleHelper::SizeSmall) {
                    frameRect = frameRect.translated(0, 1);
                } else if (cw.size == QStyleHelper::SizeMini) {
                    // Same 0.5 pt misalignment as AppKit and fit the focus ring
                    frameRect = frameRect.translated(2, -0.5);
                }
                pb.frame = frameRect.toCGRect();
                [pb highlight:isPressed];
                d->drawNSViewInRect(pb, frameRect, p, ^(CGContextRef, const CGRect &r) {
                    [pb.cell drawBezelWithFrame:r inView:pb.superview];
                });
            } else if (cw.type == QMacStylePrivate::ComboBox) {
                // Editable QComboBox
                auto *cb = static_cast<NSComboBox *>(cc);
                const auto frameRect = cw.adjustedControlFrame(combo->rect);
                cb.frame = frameRect.toCGRect();

                // This API was requested to Apple in rdar #36197888. We know it's safe to use up to macOS 10.13.3
                if (NSButtonCell *cell = static_cast<NSButtonCell *>([cc.cell qt_valueForPrivateKey:@"_buttonCell"])) {
                    cell.highlighted = isPressed;
                } else {
                    // TODO Render to pixmap and darken the button manually
                }

                d->drawNSViewInRect(cb, frameRect, p, ^(CGContextRef, const CGRect &r) {
                    // FIXME This is usually drawn in the control's superview, but we wouldn't get inactive look in this case
                    [cb.cell drawWithFrame:r inView:cb];
                });
            }

            if (combo->state & State_HasFocus) {
                // TODO Remove and use QFocusFrame instead.
                const int hMargin = proxy()->pixelMetric(QStyle::PM_FocusFrameHMargin, combo, widget);
                const int vMargin = proxy()->pixelMetric(QStyle::PM_FocusFrameVMargin, combo, widget);
                QRectF focusRect;
                if (cw.type == QMacStylePrivate::Button_PopupButton) {
                    focusRect = QRectF::fromCGRect([cc alignmentRectForFrame:cc.frame]);
                    focusRect -= pullDownButtonShadowMargins[cw.size];
                    if (cw.size == QStyleHelper::SizeSmall)
                        focusRect = focusRect.translated(0, 1);
                    else if (cw.size == QStyleHelper::SizeMini)
                        focusRect = focusRect.translated(2, -1);
                } else if (cw.type == QMacStylePrivate::ComboBox) {
                    focusRect = frameRect - comboBoxFocusRingMargins[cw.size];
                }
                d->drawFocusRing(p, focusRect, hMargin, vMargin, cw);
            }
        }
        break;
#endif // QT_CONFIG(combobox)
    case CC_TitleBar:
        if (const auto *titlebar = qstyleoption_cast<const QStyleOptionTitleBar *>(opt)) {
            const bool isActive = (titlebar->state & State_Active)
                               && (titlebar->titleBarState & State_Active);

            p->fillRect(opt->rect, Qt::transparent);
            p->setRenderHint(QPainter::Antialiasing);
            p->setClipRect(opt->rect, Qt::IntersectClip);

            // FIXME A single drawPath() with 0-sized pen
            // doesn't look as good as this double fillPath().
            const auto outerFrameRect = QRectF(opt->rect.adjusted(0, 0, 0, opt->rect.height()));
            QPainterPath outerFramePath = d->windowPanelPath(outerFrameRect);
            p->fillPath(outerFramePath, opt->palette.dark());

            const auto frameAdjust = 1.0 / p->device()->devicePixelRatioF();
            const auto innerFrameRect = outerFrameRect.adjusted(frameAdjust, frameAdjust, -frameAdjust, 0);
            QPainterPath innerFramePath = d->windowPanelPath(innerFrameRect);
            if (isActive) {
                QLinearGradient g;
                g.setStart(QPointF(0, 0));
                g.setFinalStop(QPointF(0, 2 * opt->rect.height()));
                g.setColorAt(0, opt->palette.button().color());
                g.setColorAt(1, opt->palette.dark().color());
                p->fillPath(innerFramePath, g);
            } else {
                p->fillPath(innerFramePath, opt->palette.button());
            }

            if (titlebar->subControls & (SC_TitleBarCloseButton
                                         | SC_TitleBarMaxButton
                                         | SC_TitleBarMinButton
                                         | SC_TitleBarNormalButton)) {
                const bool isHovered = (titlebar->state & State_MouseOver);
                static const SubControl buttons[] = {
                    SC_TitleBarCloseButton, SC_TitleBarMinButton, SC_TitleBarMaxButton
                };
                for (const auto sc : buttons) {
                    const auto ct = d->windowButtonCocoaControl(sc);
                    const auto cw = QMacStylePrivate::CocoaControl(ct, QStyleHelper::SizeLarge);
                    auto *wb = static_cast<NSButton *>(d->cocoaControl(cw));
                    wb.enabled = (sc & titlebar->subControls) && isActive;
                    [wb highlight:(titlebar->state & State_Sunken) && (sc & titlebar->activeSubControls)];
                    Q_UNUSED(isHovered); // FIXME No public API for this

                    const auto buttonRect = proxy()->subControlRect(CC_TitleBar, titlebar, sc, widget);
                    d->drawNSViewInRect(wb, buttonRect, p, ^(CGContextRef, const CGRect &rect) {
                        auto *wbCell = static_cast<NSButtonCell *>(wb.cell);
                        [wbCell drawWithFrame:rect inView:wb];
                    });
                }
            }

            if (titlebar->subControls & SC_TitleBarLabel) {
                const auto tr = proxy()->subControlRect(CC_TitleBar, titlebar, SC_TitleBarLabel, widget);
                if (!titlebar->icon.isNull()) {
                    const auto iconExtent = proxy()->pixelMetric(PM_SmallIconSize);
                    const auto iconSize = QSize(iconExtent, iconExtent);
                    const auto iconPos = tr.x() - titlebar->icon.actualSize(iconSize).width() - qRound(titleBarIconTitleSpacing);
                    // Only render the icon if it'll be fully visible
                    if (iconPos < tr.right() - titleBarIconTitleSpacing)
                        p->drawPixmap(iconPos, tr.y(), titlebar->icon.pixmap(window, iconSize, QIcon::Normal));
                }

                if (!titlebar->text.isEmpty())
                    drawItemText(p, tr, Qt::AlignCenter, opt->palette, isActive, titlebar->text, QPalette::Text);
            }
        }
        break;
    case CC_GroupBox:
        if (const QStyleOptionGroupBox *gb
                = qstyleoption_cast<const QStyleOptionGroupBox *>(opt)) {

            QStyleOptionGroupBox groupBox(*gb);
            const bool flat = groupBox.features & QStyleOptionFrame::Flat;
            if (!flat)
                groupBox.state |= QStyle::State_Mini; // Force mini-sized checkbox to go with small-sized label
            else
                groupBox.subControls = groupBox.subControls & ~SC_GroupBoxFrame; // We don't like frames and ugly lines

            const bool didSetFont = widget && widget->testAttribute(Qt::WA_SetFont);
            const bool didModifySubControls = !didSetFont && QApplication::desktopSettingsAware();
            if (didModifySubControls)
                groupBox.subControls = groupBox.subControls & ~SC_GroupBoxLabel;
            QCommonStyle::drawComplexControl(cc, &groupBox, p, widget);
            if (didModifySubControls) {
                const QRect rect = proxy()->subControlRect(CC_GroupBox, &groupBox, SC_GroupBoxLabel, widget);
                const bool rtl = groupBox.direction == Qt::RightToLeft;
                const int alignment = Qt::TextHideMnemonic | (rtl ? Qt::AlignRight : Qt::AlignLeft);
                const QFont savedFont = p->font();
                if (!flat)
                    p->setFont(d->smallSystemFont);
                proxy()->drawItemText(p, rect, alignment, groupBox.palette, groupBox.state & State_Enabled, groupBox.text, QPalette::WindowText);
                if (!flat)
                    p->setFont(savedFont);
            }
        }
        break;
    case CC_ToolButton:
        if (const QStyleOptionToolButton *tb
                = qstyleoption_cast<const QStyleOptionToolButton *>(opt)) {
#ifndef QT_NO_ACCESSIBILITY
            if (QStyleHelper::hasAncestor(opt->styleObject, QAccessible::ToolBar)) {
                if (tb->subControls & SC_ToolButtonMenu) {
                    QStyleOption arrowOpt = *tb;
                    arrowOpt.rect = proxy()->subControlRect(cc, tb, SC_ToolButtonMenu, widget);
                    arrowOpt.rect.setY(arrowOpt.rect.y() + arrowOpt.rect.height() / 2);
                    arrowOpt.rect.setHeight(arrowOpt.rect.height() / 2);
                    proxy()->drawPrimitive(PE_IndicatorArrowDown, &arrowOpt, p, widget);
                } else if ((tb->features & QStyleOptionToolButton::HasMenu)
                            && (tb->toolButtonStyle != Qt::ToolButtonTextOnly && !tb->icon.isNull())) {
                    d->drawToolbarButtonArrow(tb, p);
                }
                if (tb->state & State_On) {
                    NSView *view = window ? (NSView *)window->winId() : nil;
                    bool isKey = false;
                    if (view)
                        isKey = [view.window isKeyWindow];

                    QBrush brush(brushForToolButton(isKey));
                    QPainterPath path;
                    path.addRoundedRect(QRectF(tb->rect.x(), tb->rect.y(), tb->rect.width(), tb->rect.height() + 4), 4, 4);
                    p->setRenderHint(QPainter::Antialiasing);
                    p->fillPath(path, brush);
                }
                proxy()->drawControl(CE_ToolButtonLabel, opt, p, widget);
            } else
#endif // QT_NO_ACCESSIBILITY
            {
                auto bflags = tb->state;
                if (tb->subControls & SC_ToolButton)
                    bflags |= State_Sunken;
                auto mflags = tb->state;
                if (tb->subControls & SC_ToolButtonMenu)
                    mflags |= State_Sunken;

                if (tb->subControls & SC_ToolButton) {
                    if (bflags & (State_Sunken | State_On | State_Raised)) {
                        const bool isEnabled = tb->state & State_Enabled;
                        const bool isPressed = tb->state & State_Sunken;
                        const bool isHighlighted = (tb->state & State_Active) && (tb->state & State_On);
                        const auto ct = QMacStylePrivate::Button_PushButton;
                        const auto cs = d->effectiveAquaSizeConstrain(opt, widget);
                        const auto cw = QMacStylePrivate::CocoaControl(ct, cs);
                        auto *pb = static_cast<NSButton *>(d->cocoaControl(cw));
                        pb.bezelStyle = NSShadowlessSquareBezelStyle; // TODO Use NSTexturedRoundedBezelStyle in the future.
                        pb.frame = opt->rect.toCGRect();
                        pb.buttonType = NSPushOnPushOffButton;
                        pb.enabled = isEnabled;
                        [pb highlight:isPressed];
                        pb.state = isHighlighted && !isPressed ? NSOnState : NSOffState;
                        const auto buttonRect = proxy()->subControlRect(cc, tb, SC_ToolButton, widget);
                        d->drawNSViewInRect(pb, buttonRect, p, ^(CGContextRef, const CGRect &rect) {
                            [pb.cell drawBezelWithFrame:rect inView:pb];
                        });
                    }
                }

                if (tb->subControls & SC_ToolButtonMenu) {
                    const auto menuRect = proxy()->subControlRect(cc, tb, SC_ToolButtonMenu, widget);
                    QStyleOption arrowOpt = *tb;
                    arrowOpt.rect = QRect(menuRect.x() + ((menuRect.width() - toolButtonArrowSize) / 2),
                                          menuRect.height() - (toolButtonArrowSize + toolButtonArrowMargin),
                                          toolButtonArrowSize,
                                          toolButtonArrowSize);
                    proxy()->drawPrimitive(PE_IndicatorArrowDown, &arrowOpt, p, widget);
                } else if (tb->features & QStyleOptionToolButton::HasMenu) {
                    d->drawToolbarButtonArrow(tb, p);
                }
                QRect buttonRect = proxy()->subControlRect(CC_ToolButton, tb, SC_ToolButton, widget);
                int fw = proxy()->pixelMetric(PM_DefaultFrameWidth, opt, widget);
                QStyleOptionToolButton label = *tb;
                label.rect = buttonRect.adjusted(fw, fw, -fw, -fw);
                proxy()->drawControl(CE_ToolButtonLabel, &label, p, widget);
            }
        }
        break;
#if QT_CONFIG(dial)
    case CC_Dial:
        if (const QStyleOptionSlider *dial = qstyleoption_cast<const QStyleOptionSlider *>(opt))
            QStyleHelper::drawDial(dial, p);
        break;
#endif
    default:
        QCommonStyle::drawComplexControl(cc, opt, p, widget);
        break;
    }
}

QStyle::SubControl QMacStyle::hitTestComplexControl(ComplexControl cc,
                                                    const QStyleOptionComplex *opt,
                                                    const QPoint &pt, const QWidget *widget) const
{
    Q_D(const QMacStyle);
    SubControl sc = QStyle::SC_None;
    switch (cc) {
    case CC_ComboBox:
        if (const QStyleOptionComboBox *cmb = qstyleoption_cast<const QStyleOptionComboBox *>(opt)) {
            sc = QCommonStyle::hitTestComplexControl(cc, cmb, pt, widget);
            if (!cmb->editable && sc != QStyle::SC_None)
                sc = SC_ComboBoxArrow;  // A bit of a lie, but what we want
        }
        break;
    case CC_Slider:
        if (const QStyleOptionSlider *sl = qstyleoption_cast<const QStyleOptionSlider *>(opt)) {
            if (!sl->rect.contains(pt))
                break;

            const bool hasTicks = sl->tickPosition != QSlider::NoTicks;
            const bool isHorizontal = sl->orientation == Qt::Horizontal;
            const auto ct = isHorizontal ? QMacStylePrivate::Slider_Horizontal : QMacStylePrivate::Slider_Vertical;
            const auto cs = d->effectiveAquaSizeConstrain(opt, widget);
            const auto cw = QMacStylePrivate::CocoaControl(ct, cs);
            auto *slider = static_cast<NSSlider *>(d->cocoaControl(cw));
            if (!setupSlider(slider, sl))
                break;

            NSSliderCell *cell = slider.cell;
            const auto barRect = QRectF::fromCGRect([cell barRectFlipped:slider.isFlipped]);
            const auto knobRect = QRectF::fromCGRect([cell knobRectFlipped:slider.isFlipped]);
            if (knobRect.contains(pt)) {
                sc = SC_SliderHandle;
            } else if (barRect.contains(pt)) {
                sc = SC_SliderGroove;
            } else if (hasTicks) {
                sc = SC_SliderTickmarks;
            }
        }
        break;
    case CC_ScrollBar:
        if (const QStyleOptionSlider *sb = qstyleoption_cast<const QStyleOptionSlider *>(opt)) {
            if (!sb->rect.contains(pt)) {
                sc = SC_None;
                break;
            }

            const bool isHorizontal = sb->orientation == Qt::Horizontal;
            const auto ct = isHorizontal ? QMacStylePrivate::Scroller_Horizontal : QMacStylePrivate::Scroller_Vertical;
            const auto cs = d->effectiveAquaSizeConstrain(opt, widget);
            const auto cw = QMacStylePrivate::CocoaControl(ct, cs);
            auto *scroller = static_cast<NSScroller *>(d->cocoaControl(cw));
            if (!setupScroller(scroller, sb)) {
                sc = SC_None;
                break;
            }

            // Since -[NSScroller testPart:] doesn't want to cooperate, we do it the
            // straightforward way. In any case, macOS doesn't return line-sized changes
            // with NSScroller since 10.7, according to the aforementioned method's doc.
            const auto knobRect = QRectF::fromCGRect([scroller rectForPart:NSScrollerKnob]);
            if (isHorizontal) {
                const bool isReverse = sb->direction == Qt::RightToLeft;
                if (pt.x() < knobRect.left())
                    sc = isReverse ? SC_ScrollBarAddPage : SC_ScrollBarSubPage;
                else if (pt.x() > knobRect.right())
                    sc = isReverse ? SC_ScrollBarSubPage : SC_ScrollBarAddPage;
                else
                    sc = SC_ScrollBarSlider;
            } else {
                if (pt.y() < knobRect.top())
                    sc = SC_ScrollBarSubPage;
                else if (pt.y() > knobRect.bottom())
                    sc = SC_ScrollBarAddPage;
                else
                    sc = SC_ScrollBarSlider;
            }
        }
        break;
    default:
        sc = QCommonStyle::hitTestComplexControl(cc, opt, pt, widget);
        break;
    }
    return sc;
}

QRect QMacStyle::subControlRect(ComplexControl cc, const QStyleOptionComplex *opt, SubControl sc,
                                const QWidget *widget) const
{
    Q_D(const QMacStyle);
    QRect ret;
    switch (cc) {
    case CC_ScrollBar:
        if (const QStyleOptionSlider *sb = qstyleoption_cast<const QStyleOptionSlider *>(opt)) {
            const bool isHorizontal = sb->orientation == Qt::Horizontal;
            const bool isReverseHorizontal = isHorizontal && (sb->direction == Qt::RightToLeft);

            NSScrollerPart part = NSScrollerNoPart;
            if (sc == SC_ScrollBarSlider) {
                part = NSScrollerKnob;
            } else if (sc == SC_ScrollBarGroove) {
                part = NSScrollerKnobSlot;
            } else if (sc == SC_ScrollBarSubPage || sc == SC_ScrollBarAddPage) {
                if ((!isReverseHorizontal && sc == SC_ScrollBarSubPage)
                        || (isReverseHorizontal && sc == SC_ScrollBarAddPage))
                    part = NSScrollerDecrementPage;
                else
                    part = NSScrollerIncrementPage;
            }
            // And nothing else since 10.7

            if (part != NSScrollerNoPart) {
                const auto ct = isHorizontal ? QMacStylePrivate::Scroller_Horizontal : QMacStylePrivate::Scroller_Vertical;
                const auto cs = d->effectiveAquaSizeConstrain(opt, widget);
                const auto cw = QMacStylePrivate::CocoaControl(ct, cs);
                auto *scroller = static_cast<NSScroller *>(d->cocoaControl(cw));
                if (setupScroller(scroller, sb))
                    ret = QRectF::fromCGRect([scroller rectForPart:part]).toRect();
            }
        }
        break;
    case CC_Slider:
        if (const QStyleOptionSlider *sl = qstyleoption_cast<const QStyleOptionSlider *>(opt)) {
            const bool hasTicks = sl->tickPosition != QSlider::NoTicks;
            const bool isHorizontal = sl->orientation == Qt::Horizontal;
            const auto ct = isHorizontal ? QMacStylePrivate::Slider_Horizontal : QMacStylePrivate::Slider_Vertical;
            const auto cs = d->effectiveAquaSizeConstrain(opt, widget);
            const auto cw = QMacStylePrivate::CocoaControl(ct, cs);
            auto *slider = static_cast<NSSlider *>(d->cocoaControl(cw));
            if (!setupSlider(slider, sl))
                break;

            NSSliderCell *cell = slider.cell;
            if (sc == SC_SliderHandle) {
                ret = QRectF::fromCGRect([cell knobRectFlipped:slider.isFlipped]).toRect();
                if (isHorizontal) {
                    ret.setTop(sl->rect.top());
                    ret.setBottom(sl->rect.bottom());
                } else {
                    ret.setLeft(sl->rect.left());
                    ret.setRight(sl->rect.right());
                }
            } else if (sc == SC_SliderGroove) {
                ret = QRectF::fromCGRect([cell barRectFlipped:slider.isFlipped]).toRect();
            } else if (hasTicks && sc == SC_SliderTickmarks) {
                const auto tickMarkRect = QRectF::fromCGRect([cell rectOfTickMarkAtIndex:0]);
                if (isHorizontal)
                    ret = QRect(sl->rect.left(), tickMarkRect.top(), sl->rect.width(), tickMarkRect.height());
                else
                    ret = QRect(tickMarkRect.left(), sl->rect.top(), tickMarkRect.width(), sl->rect.height());
            }

            // Invert if needed and extend to the actual bounds of the slider
            if (isHorizontal) {
                if (sl->upsideDown) {
                    ret = QRect(sl->rect.right() - ret.right(), sl->rect.top(), ret.width(), sl->rect.height());
                } else {
                    ret.setTop(sl->rect.top());
                    ret.setBottom(sl->rect.bottom());
                }
            } else {
                if (!sl->upsideDown) {
                    ret = QRect(sl->rect.left(), sl->rect.bottom() - ret.bottom(), sl->rect.width(), ret.height());
                } else {
                    ret.setLeft(sl->rect.left());
                    ret.setRight(sl->rect.right());
                }
            }
        }
        break;
    case CC_TitleBar:
        if (const auto *titlebar = qstyleoption_cast<const QStyleOptionTitleBar *>(opt)) {
            // The title bar layout is as follows: close, min, zoom, icon, title
            //              [ x _ +    @ Window Title    ]
            // Center the icon and title until it starts to overlap with the buttons.
            // The icon doesn't count towards SC_TitleBarLabel, but it's still rendered
            // next to the title text. See drawComplexControl().
            if (sc == SC_TitleBarLabel) {
                qreal labelWidth = titlebar->fontMetrics.horizontalAdvance(titlebar->text) + 1; // FIXME Rounding error?
                qreal labelHeight = titlebar->fontMetrics.height();

                const auto lastButtonRect = proxy()->subControlRect(CC_TitleBar, titlebar, SC_TitleBarMaxButton, widget);
                qreal controlsSpacing = lastButtonRect.right() + titleBarButtonSpacing;
                if (!titlebar->icon.isNull()) {
                    const auto iconSize = proxy()->pixelMetric(PM_SmallIconSize);
                    const auto actualIconSize = titlebar->icon.actualSize(QSize(iconSize, iconSize)).width();;
                    controlsSpacing += actualIconSize + titleBarIconTitleSpacing;
                }

                const qreal labelPos = qMax(controlsSpacing, (opt->rect.width() - labelWidth) / 2.0);
                labelWidth = qMin(labelWidth, opt->rect.width() - (labelPos + titleBarTitleRightMargin));
                ret = QRect(labelPos, (opt->rect.height() - labelHeight) / 2,
                            labelWidth, labelHeight);
            } else {
                const auto currentButton = d->windowButtonCocoaControl(sc);
                if (currentButton == QMacStylePrivate::NoControl)
                    break;

                QPointF buttonPos = titlebar->rect.topLeft() + QPointF(titleBarButtonSpacing, 0);
                QSizeF buttonSize;
                for (int ct = QMacStylePrivate::Button_WindowClose; ct <= currentButton; ct++) {
                    const auto cw = QMacStylePrivate::CocoaControl(QMacStylePrivate::CocoaControlType(ct),
                                                                   QStyleHelper::SizeLarge);
                    auto *wb = static_cast<NSButton *>(d->cocoaControl(cw));
                    if (ct == currentButton)
                        buttonSize = QSizeF::fromCGSize(wb.frame.size);
                    else
                        buttonPos.rx() += wb.frame.size.width + titleBarButtonSpacing;
                }

                const auto vOffset = (opt->rect.height() - buttonSize.height()) / 2.0;
                ret = QRectF(buttonPos, buttonSize).translated(0, vOffset).toRect();
            }
        }
        break;
    case CC_ComboBox:
        if (const QStyleOptionComboBox *combo = qstyleoption_cast<const QStyleOptionComboBox *>(opt)) {
            const auto ct = cocoaControlType(combo, widget);
            const auto cs = d->effectiveAquaSizeConstrain(combo, widget);
            const auto cw = QMacStylePrivate::CocoaControl(ct, cs);
            const auto editRect = QMacStylePrivate::comboboxEditBounds(cw.adjustedControlFrame(combo->rect), cw);

            switch (sc) {
            case SC_ComboBoxEditField:{
                ret = editRect.toAlignedRect();
                break; }
            case SC_ComboBoxArrow:{
                ret = editRect.toAlignedRect();
                ret.setX(ret.x() + ret.width());
                ret.setWidth(combo->rect.right() - ret.right());
                break; }
            case SC_ComboBoxListBoxPopup:{
                if (combo->editable) {
                    const CGRect inner = QMacStylePrivate::comboboxInnerBounds(combo->rect.toCGRect(), cw);
                    const int comboTop = combo->rect.top();
                    ret = QRect(qRound(inner.origin.x),
                                comboTop,
                                qRound(inner.origin.x - combo->rect.left() + inner.size.width),
                                editRect.bottom() - comboTop + 2);
                } else {
                    ret = QRect(combo->rect.x() + 4 - 11,
                                combo->rect.y() + 1,
                                editRect.width() + 10 + 11,
                                1);
                 }
                break; }
            default:
                break;
            }
        }
        break;
    case CC_GroupBox:
        if (const QStyleOptionGroupBox *groupBox = qstyleoption_cast<const QStyleOptionGroupBox *>(opt)) {
            bool checkable = groupBox->subControls & SC_GroupBoxCheckBox;
            const bool flat = groupBox->features & QStyleOptionFrame::Flat;
            bool hasNoText = !checkable && groupBox->text.isEmpty();
            switch (sc) {
            case SC_GroupBoxLabel:
            case SC_GroupBoxCheckBox: {
                // Cheat and use the smaller font if we need to
                const bool checkable = groupBox->subControls & SC_GroupBoxCheckBox;
                const bool fontIsSet = (widget && widget->testAttribute(Qt::WA_SetFont))
                                       || !QApplication::desktopSettingsAware();
                const int margin =  flat || hasNoText ? 0 : 9;
                ret = groupBox->rect.adjusted(margin, 0, -margin, 0);

                const QFontMetricsF fm = flat || fontIsSet ? QFontMetricsF(groupBox->fontMetrics) : QFontMetricsF(d->smallSystemFont);
                const QSizeF s = fm.size(Qt::AlignHCenter | Qt::AlignVCenter, qt_mac_removeMnemonics(groupBox->text), 0, nullptr);
                const int tw = qCeil(s.width());
                const int h = qCeil(fm.height());
                ret.setHeight(h);

                QRect labelRect = alignedRect(groupBox->direction, groupBox->textAlignment,
                                              QSize(tw, h), ret);
                if (flat && checkable)
                    labelRect.moveLeft(labelRect.left() + 4);
                int indicatorWidth = proxy()->pixelMetric(PM_IndicatorWidth, opt, widget);
                bool rtl = groupBox->direction == Qt::RightToLeft;
                if (sc == SC_GroupBoxLabel) {
                    if (checkable) {
                        int newSum = indicatorWidth + 1;
                        int newLeft = labelRect.left() + (rtl ? -newSum : newSum);
                        labelRect.moveLeft(newLeft);
                        if (flat)
                            labelRect.moveTop(labelRect.top() + 3);
                        else
                            labelRect.moveTop(labelRect.top() + 4);
                    } else if (flat) {
                        int newLeft = labelRect.left() - (rtl ? 3 : -3);
                        labelRect.moveLeft(newLeft);
                        labelRect.moveTop(labelRect.top() + 3);
                    } else {
                        int newLeft = labelRect.left() - (rtl ? 3 : 2);
                        labelRect.moveLeft(newLeft);
                        labelRect.moveTop(labelRect.top() + 4);
                    }
                    ret = labelRect;
                }

                if (sc == SC_GroupBoxCheckBox) {
                    int left = rtl ? labelRect.right() - indicatorWidth : labelRect.left() - 1;
                    int top = flat ? ret.top() + 1 : ret.top() + 5;
                    ret.setRect(left, top,
                                indicatorWidth, proxy()->pixelMetric(PM_IndicatorHeight, opt, widget));
                }
                break;
            }
            case SC_GroupBoxContents:
            case SC_GroupBoxFrame: {
                QFontMetrics fm = groupBox->fontMetrics;
                int yOffset = 3;
                if (!flat) {
                    if (widget && !widget->testAttribute(Qt::WA_SetFont)
                            && QApplication::desktopSettingsAware())
                        fm = QFontMetrics(qt_app_fonts_hash()->value("QSmallFont", QFont()));
                    yOffset = 5;
                }

                if (hasNoText)
                    yOffset = -qCeil(QFontMetricsF(fm).height());
                ret = opt->rect.adjusted(0, qCeil(QFontMetricsF(fm).height()) + yOffset, 0, 0);
                if (sc == SC_GroupBoxContents) {
                    if (flat)
                        ret.adjust(3, -5, -3, -4);   // guess too
                    else
                        ret.adjust(3, 3, -3, -4);    // guess
                }
            }
                break;
            default:
                ret = QCommonStyle::subControlRect(cc, groupBox, sc, widget);
                break;
            }
        }
        break;
#if QT_CONFIG(spinbox)
    case CC_SpinBox:
        if (const QStyleOptionSpinBox *spin = qstyleoption_cast<const QStyleOptionSpinBox *>(opt)) {
            QStyleHelper::WidgetSizePolicy aquaSize = d->effectiveAquaSizeConstrain(spin, widget);
            const auto fw = proxy()->pixelMetric(PM_SpinBoxFrameWidth, spin, widget);
            int spinner_w;
            int spinBoxSep;
            switch (aquaSize) {
            case QStyleHelper::SizeLarge:
                spinner_w = 14;
                spinBoxSep = 2;
                break;
            case QStyleHelper::SizeSmall:
                spinner_w = 12;
                spinBoxSep = 2;
                break;
            case QStyleHelper::SizeMini:
                spinner_w = 10;
                spinBoxSep = 1;
                break;
            default:
                Q_UNREACHABLE();
            }

            switch (sc) {
            case SC_SpinBoxUp:
            case SC_SpinBoxDown: {
                if (spin->buttonSymbols == QAbstractSpinBox::NoButtons)
                    break;

                const int y = fw;
                const int x = spin->rect.width() - spinner_w;
                ret.setRect(x + spin->rect.x(), y + spin->rect.y(), spinner_w, spin->rect.height() - y * 2);
                int hackTranslateX;
                switch (aquaSize) {
                case QStyleHelper::SizeLarge:
                    hackTranslateX = 0;
                    break;
                case QStyleHelper::SizeSmall:
                    hackTranslateX = -2;
                    break;
                case QStyleHelper::SizeMini:
                    hackTranslateX = -1;
                    break;
                default:
                    Q_UNREACHABLE();
                }

                const auto cw = QMacStylePrivate::CocoaControl(QMacStylePrivate::Stepper, aquaSize);
                NSStepperCell *cell = static_cast<NSStepperCell *>(d->cocoaCell(cw));
                const CGRect outRect = [cell drawingRectForBounds:ret.toCGRect()];
                ret = QRectF::fromCGRect(outRect).toRect();

                switch (sc) {
                case SC_SpinBoxUp:
                    ret.setHeight(ret.height() / 2);
                    break;
                case SC_SpinBoxDown:
                    ret.setY(ret.y() + ret.height() / 2);
                    break;
                default:
                    Q_ASSERT(0);
                    break;
                }
                ret.translate(hackTranslateX, 0); // hack: position the buttons correctly (weird that we need this)
                ret = visualRect(spin->direction, spin->rect, ret);
                break;
            }
            case SC_SpinBoxEditField:
                ret = spin->rect.adjusted(fw, fw, -fw, -fw);
                if (spin->buttonSymbols != QAbstractSpinBox::NoButtons) {
                    ret.setWidth(spin->rect.width() - spinBoxSep - spinner_w);
                    ret = visualRect(spin->direction, spin->rect, ret);
                }
                break;
            default:
                ret = QCommonStyle::subControlRect(cc, spin, sc, widget);
                break;
            }
        }
        break;
#endif
    case CC_ToolButton:
        ret = QCommonStyle::subControlRect(cc, opt, sc, widget);
        if (sc == SC_ToolButtonMenu) {
#ifndef QT_NO_ACCESSIBILITY
            if (QStyleHelper::hasAncestor(opt->styleObject, QAccessible::ToolBar))
                ret.adjust(-toolButtonArrowMargin, 0, 0, 0);
#endif
            ret.adjust(-1, 0, 0, 0);
        }
        break;
    default:
        ret = QCommonStyle::subControlRect(cc, opt, sc, widget);
        break;
    }
    return ret;
}

QSize QMacStyle::sizeFromContents(ContentsType ct, const QStyleOption *opt,
                                  const QSize &csz, const QWidget *widget) const
{
    Q_D(const QMacStyle);
    QSize sz(csz);
    bool useAquaGuideline = true;

    switch (ct) {
#if QT_CONFIG(spinbox)
    case CT_SpinBox:
        if (const QStyleOptionSpinBox *vopt = qstyleoption_cast<const QStyleOptionSpinBox *>(opt)) {
            const bool hasButtons = (vopt->buttonSymbols != QAbstractSpinBox::NoButtons);
            const int buttonWidth = hasButtons ? proxy()->subControlRect(CC_SpinBox, vopt, SC_SpinBoxUp, widget).width() : 0;
            sz += QSize(buttonWidth, 0);
        }
        break;
#endif
    case QStyle::CT_TabWidget:
        // the size between the pane and the "contentsRect" (+4,+4)
        // (the "contentsRect" is on the inside of the pane)
        sz = QCommonStyle::sizeFromContents(ct, opt, csz, widget);
        /**
            This is supposed to show the relationship between the tabBar and
            the stack widget of a QTabWidget.
            Unfortunately ascii is not a good way of representing graphics.....
            PS: The '=' line is the painted frame.

               top    ---+
                         |
                         |
                         |
                         |                vvv just outside the painted frame is the "pane"
                      - -|- - - - - - - - - - <-+
            TAB BAR      +=====^============    | +2 pixels
                    - - -|- - -|- - - - - - - <-+
                         |     |      ^   ^^^ just inside the painted frame is the "contentsRect"
                         |     |      |
                         |   overlap  |
                         |     |      |
            bottom ------+   <-+     +14 pixels
                                      |
                                      v
                ------------------------------  <- top of stack widget


        To summarize:
             * 2 is the distance between the pane and the contentsRect
             * The 14 and the 1's are the distance from the contentsRect to the stack widget.
               (same value as used in SE_TabWidgetTabContents)
             * overlap is how much the pane should overlap the tab bar
        */
        // then add the size between the stackwidget and the "contentsRect"
#if QT_CONFIG(tabwidget)
        if (const QStyleOptionTabWidgetFrame *twf
                = qstyleoption_cast<const QStyleOptionTabWidgetFrame *>(opt)) {
            QSize extra(0,0);
            const int overlap = pixelMetric(PM_TabBarBaseOverlap, opt, widget);
            const int gapBetweenTabbarAndStackWidget = 2 + 14 - overlap;

            const auto tabDirection = QMacStylePrivate::tabDirection(twf->shape);
            if (tabDirection == QMacStylePrivate::North
                    || tabDirection == QMacStylePrivate::South) {
                extra = QSize(2, gapBetweenTabbarAndStackWidget + 1);
            } else {
                extra = QSize(gapBetweenTabbarAndStackWidget + 1, 2);
            }
            sz+= extra;
        }
#endif
        break;
#if QT_CONFIG(tabbar)
    case QStyle::CT_TabBarTab:
        if (const QStyleOptionTab *tab = qstyleoption_cast<const QStyleOptionTab *>(opt)) {
            const bool differentFont = (widget && widget->testAttribute(Qt::WA_SetFont))
                                       || !QApplication::desktopSettingsAware();
            const auto tabDirection = QMacStylePrivate::tabDirection(tab->shape);
            const bool verticalTabs = tabDirection == QMacStylePrivate::East
                                   || tabDirection == QMacStylePrivate::West;
            if (verticalTabs)
                sz = sz.transposed();

            int defaultTabHeight;
            const auto cs = d->effectiveAquaSizeConstrain(opt, widget);
            switch (cs) {
            case QStyleHelper::SizeLarge:
                if (tab->documentMode)
                    defaultTabHeight = 24;
                else
                    defaultTabHeight = 21;
                break;
            case QStyleHelper::SizeSmall:
                defaultTabHeight = 18;
                break;
            case QStyleHelper::SizeMini:
                defaultTabHeight = 16;
                break;
            default:
                break;
            }

            const bool widthSet = !differentFont && tab->icon.isNull();
            if (widthSet) {
                const auto textSize = opt->fontMetrics.size(Qt::TextShowMnemonic, tab->text);
                sz.rwidth() = textSize.width();
                sz.rheight() = qMax(defaultTabHeight, textSize.height());
            } else {
                sz.rheight() = qMax(defaultTabHeight, sz.height());
            }
            sz.rwidth() += proxy()->pixelMetric(PM_TabBarTabHSpace, tab, widget);

            if (verticalTabs)
                sz = sz.transposed();

            int maxWidgetHeight = qMax(tab->leftButtonSize.height(), tab->rightButtonSize.height());
            int maxWidgetWidth = qMax(tab->leftButtonSize.width(), tab->rightButtonSize.width());

            int widgetWidth = 0;
            int widgetHeight = 0;
            int padding = 0;
            if (tab->leftButtonSize.isValid()) {
                padding += 8;
                widgetWidth += tab->leftButtonSize.width();
                widgetHeight += tab->leftButtonSize.height();
            }
            if (tab->rightButtonSize.isValid()) {
                padding += 8;
                widgetWidth += tab->rightButtonSize.width();
                widgetHeight += tab->rightButtonSize.height();
            }

            if (verticalTabs) {
                sz.setWidth(qMax(sz.width(), maxWidgetWidth));
                sz.setHeight(sz.height() + widgetHeight + padding);
            } else {
                if (widthSet)
                    sz.setWidth(sz.width() + widgetWidth + padding);
                sz.setHeight(qMax(sz.height(), maxWidgetHeight));
            }
        }
        break;
#endif
    case QStyle::CT_PushButton: {
        if (const QStyleOptionButton *btn = qstyleoption_cast<const QStyleOptionButton *>(opt))
            if (btn->features & QStyleOptionButton::CommandLinkButton)
                return QCommonStyle::sizeFromContents(ct, opt, sz, widget);

        // By default, we fit the contents inside a normal rounded push button.
        // Do this by add enough space around the contents so that rounded
        // borders (including highlighting when active) will show.
        // TODO Use QFocusFrame and get rid of these horrors.
        QSize macsz;
        const auto controlSize = d->effectiveAquaSizeConstrain(opt, widget, CT_PushButton, sz, &macsz);
        // FIXME See comment in CT_PushButton case in qt_aqua_get_known_size().
        if (macsz.width() != -1)
            sz.setWidth(macsz.width());
        else
            sz.rwidth() += QMacStylePrivate::PushButtonLeftOffset + QMacStylePrivate::PushButtonRightOffset + 12;
        // All values as measured from HIThemeGetButtonBackgroundBounds()
        if (controlSize != QStyleHelper::SizeMini)
            sz.rwidth() += 12; // We like 12 over here.
        if (controlSize == QStyleHelper::SizeLarge && sz.height() > 16)
            sz.rheight() += pushButtonDefaultHeight[QStyleHelper::SizeLarge] - 16;
        else if (controlSize == QStyleHelper::SizeMini)
            sz.setHeight(24); // FIXME Our previous HITheme-based logic returned this.
        else
            sz.setHeight(pushButtonDefaultHeight[controlSize]);
        break;
    }
    case QStyle::CT_MenuItem:
        if (const QStyleOptionMenuItem *mi = qstyleoption_cast<const QStyleOptionMenuItem *>(opt)) {
            int maxpmw = mi->maxIconWidth;
#if QT_CONFIG(combobox)
            const QComboBox *comboBox = qobject_cast<const QComboBox *>(widget);
#endif
            int w = sz.width(),
                h = sz.height();
            if (mi->menuItemType == QStyleOptionMenuItem::Separator) {
                w = 10;
                h = qt_mac_aqua_get_metric(MenuSeparatorHeight);
            } else {
                h = mi->fontMetrics.height() + 2;
                if (!mi->icon.isNull()) {
#if QT_CONFIG(combobox)
                    if (comboBox) {
                        const QSize &iconSize = comboBox->iconSize();
                        h = qMax(h, iconSize.height() + 4);
                        maxpmw = qMax(maxpmw, iconSize.width());
                    } else
#endif
                    {
                        int iconExtent = proxy()->pixelMetric(PM_SmallIconSize);
                        h = qMax(h, mi->icon.actualSize(QSize(iconExtent, iconExtent)).height() + 4);
                    }
                }
            }
            if (mi->text.contains(QLatin1Char('\t')))
                w += 12;
            else if (mi->menuItemType == QStyleOptionMenuItem::SubMenu)
                w += 35; // Not quite exactly as it seems to depend on other factors
            if (maxpmw)
                w += maxpmw + 6;
            // add space for a check. All items have place for a check too.
            w += 20;
#if QT_CONFIG(combobox)
            if (comboBox && comboBox->isVisible()) {
                QStyleOptionComboBox cmb;
                cmb.initFrom(comboBox);
                cmb.editable = false;
                cmb.subControls = QStyle::SC_ComboBoxEditField;
                cmb.activeSubControls = QStyle::SC_None;
                w = qMax(w, subControlRect(QStyle::CC_ComboBox, &cmb,
                                                   QStyle::SC_ComboBoxEditField,
                                                   comboBox).width());
            } else
#endif
            {
                w += 12;
            }
            sz = QSize(w, h);
        }
        break;
    case CT_MenuBarItem:
        if (!sz.isEmpty())
            sz += QSize(12, 4); // Constants from QWindowsStyle
        break;
    case CT_ToolButton:
        sz.rwidth() += 10;
        sz.rheight() += 10;
        if (const auto *tb = qstyleoption_cast<const QStyleOptionToolButton *>(opt))
            if (tb->features & QStyleOptionToolButton::Menu)
                sz.rwidth() += toolButtonArrowMargin;
        return sz;
    case CT_ComboBox:
        if (const auto *cb = qstyleoption_cast<const QStyleOptionComboBox *>(opt)) {
            const auto controlSize = d->effectiveAquaSizeConstrain(opt, widget);
            if (!cb->editable) {
                // Same as CT_PushButton, because we have to fit the focus
                // ring and a non-editable combo box is a NSPopUpButton.
                sz.rwidth() += QMacStylePrivate::PushButtonLeftOffset + QMacStylePrivate::PushButtonRightOffset + 12;
                // All values as measured from HIThemeGetButtonBackgroundBounds()
                if (controlSize != QStyleHelper::SizeMini)
                    sz.rwidth() += 12; // We like 12 over here.
#if 0
                // TODO Maybe support square combo boxes
                if (controlSize == QStyleHelper::SizeLarge && sz.height() > 16)
                    sz.rheight() += popupButtonDefaultHeight[QStyleHelper::SizeLarge] - 16;
                else
#endif
            } else {
                sz.rwidth() += 50; // FIXME Double check this
            }

            // This should be enough to fit the focus ring
            if (controlSize == QStyleHelper::SizeMini)
                sz.setHeight(24); // FIXME Our previous HITheme-based logic returned this for CT_PushButton.
            else
                sz.setHeight(pushButtonDefaultHeight[controlSize]);

            return sz;
        }
        break;
    case CT_Menu: {
        if (proxy() == this) {
            sz = csz;
        } else {
            QStyleHintReturnMask menuMask;
            QStyleOption myOption = *opt;
            myOption.rect.setSize(sz);
            if (proxy()->styleHint(SH_Menu_Mask, &myOption, widget, &menuMask))
                sz = menuMask.region.boundingRect().size();
        }
        break; }
    case CT_HeaderSection:{
        const QStyleOptionHeader *header = qstyleoption_cast<const QStyleOptionHeader *>(opt);
        sz = QCommonStyle::sizeFromContents(ct, opt, csz, widget);
        if (header->text.contains(QLatin1Char('\n')))
            useAquaGuideline = false;
        break; }
    case CT_ScrollBar :
        // Make sure that the scroll bar is large enough to display the thumb indicator.
        if (const QStyleOptionSlider *slider = qstyleoption_cast<const QStyleOptionSlider *>(opt)) {
            const int minimumSize = 24; // Smallest knob size, but Cocoa doesn't seem to care
            if (slider->orientation == Qt::Horizontal)
                sz = sz.expandedTo(QSize(minimumSize, sz.height()));
            else
                sz = sz.expandedTo(QSize(sz.width(), minimumSize));
        }
        break;
#if QT_CONFIG(itemviews)
    case CT_ItemViewItem:
        if (const QStyleOptionViewItem *vopt = qstyleoption_cast<const QStyleOptionViewItem *>(opt)) {
            sz = QCommonStyle::sizeFromContents(ct, vopt, csz, widget);
            sz.setHeight(sz.height() + 2);
        }
        break;
#endif

    default:
        sz = QCommonStyle::sizeFromContents(ct, opt, csz, widget);
    }

    if (useAquaGuideline && ct != CT_PushButton) {
        // TODO Probably going away at some point
        QSize macsz;
        if (d->aquaSizeConstrain(opt, widget, ct, sz, &macsz) != QStyleHelper::SizeDefault) {
            if (macsz.width() != -1)
                sz.setWidth(macsz.width());
            if (macsz.height() != -1)
                sz.setHeight(macsz.height());
        }
    }

    // The sizes that Carbon and the guidelines gives us excludes the focus frame.
    // We compensate for this by adding some extra space here to make room for the frame when drawing:
    if (const QStyleOptionComboBox *combo = qstyleoption_cast<const QStyleOptionComboBox *>(opt)){
        if (combo->editable) {
            const auto widgetSize = d->aquaSizeConstrain(opt, widget);
            QMacStylePrivate::CocoaControl cw;
            cw.type = combo->editable ? QMacStylePrivate::ComboBox : QMacStylePrivate::Button_PopupButton;
            cw.size = widgetSize;
            const CGRect diffRect = QMacStylePrivate::comboboxInnerBounds(CGRectZero, cw);
            sz.rwidth() -= qRound(diffRect.size.width);
            sz.rheight() -= qRound(diffRect.size.height);
        }
    }
    return sz;
}

void QMacStyle::drawItemText(QPainter *p, const QRect &r, int flags, const QPalette &pal,
                             bool enabled, const QString &text, QPalette::ColorRole textRole) const
{
    if(flags & Qt::TextShowMnemonic)
        flags |= Qt::TextHideMnemonic;
    QCommonStyle::drawItemText(p, r, flags, pal, enabled, text, textRole);
}

bool QMacStyle::event(QEvent *e)
{
    Q_D(QMacStyle);
    if(e->type() == QEvent::FocusIn) {
        QWidget *f = 0;
        QWidget *focusWidget = QApplication::focusWidget();
#if QT_CONFIG(graphicsview)
        if (QGraphicsView *graphicsView = qobject_cast<QGraphicsView *>(focusWidget)) {
            QGraphicsItem *focusItem = graphicsView->scene() ? graphicsView->scene()->focusItem() : 0;
            if (focusItem && focusItem->type() == QGraphicsProxyWidget::Type) {
                QGraphicsProxyWidget *proxy = static_cast<QGraphicsProxyWidget *>(focusItem);
                if (proxy->widget())
                    focusWidget = proxy->widget()->focusWidget();
            }
        }
#endif

        if (focusWidget && focusWidget->testAttribute(Qt::WA_MacShowFocusRect)) {
#if QT_CONFIG(spinbox)
            if (const auto sb = qobject_cast<QAbstractSpinBox *>(focusWidget))
                f = sb->property("_q_spinbox_lineedit").value<QWidget *>();
            else
#endif
                f = focusWidget;
        }
        if (f) {
            if(!d->focusWidget)
                d->focusWidget = new QFocusFrame(f);
            d->focusWidget->setWidget(f);
        } else if(d->focusWidget) {
            d->focusWidget->setWidget(0);
        }
    } else if(e->type() == QEvent::FocusOut) {
        if(d->focusWidget)
            d->focusWidget->setWidget(0);
    }
    return false;
}

QIcon QMacStyle::standardIcon(StandardPixmap standardIcon, const QStyleOption *opt,
                              const QWidget *widget) const
{
    switch (standardIcon) {
    default:
        return QCommonStyle::standardIcon(standardIcon, opt, widget);
    case SP_ToolBarHorizontalExtensionButton:
    case SP_ToolBarVerticalExtensionButton: {
        QPixmap pixmap(QLatin1String(":/qt-project.org/styles/macstyle/images/toolbar-ext.png"));
        if (standardIcon == SP_ToolBarVerticalExtensionButton) {
            QPixmap pix2(pixmap.height(), pixmap.width());
            pix2.setDevicePixelRatio(pixmap.devicePixelRatio());
            pix2.fill(Qt::transparent);
            QPainter p(&pix2);
            p.translate(pix2.width(), 0);
            p.rotate(90);
            p.drawPixmap(0, 0, pixmap);
            return pix2;
        }
        return pixmap;
    }
    }
}

int QMacStyle::layoutSpacing(QSizePolicy::ControlType control1,
                             QSizePolicy::ControlType control2,
                             Qt::Orientation orientation,
                             const QStyleOption *option,
                             const QWidget *widget) const
{
    const int ButtonMask = QSizePolicy::ButtonBox | QSizePolicy::PushButton;
    const int controlSize = getControlSize(option, widget);

    if (control2 == QSizePolicy::ButtonBox) {
        /*
            AHIG seems to prefer a 12-pixel margin between group
            boxes and the row of buttons. The 20 pixel comes from
            Builder.
        */
        if (control1 & (QSizePolicy::Frame         // guess
                        | QSizePolicy::GroupBox    // (AHIG, guess, guess)
                        | QSizePolicy::TabWidget   // guess
                        | ButtonMask))    {        // AHIG
            return_SIZE(14, 8, 8);
        } else if (control1 == QSizePolicy::LineEdit) {
            return_SIZE(8, 8, 8); // Interface Builder
        } else {
            return_SIZE(20, 7, 7); // Interface Builder
        }
    }

    if ((control1 | control2) & ButtonMask) {
        if (control1 == QSizePolicy::LineEdit)
            return_SIZE(8, 8, 8); // Interface Builder
        else if (control2 == QSizePolicy::LineEdit) {
            if (orientation == Qt::Vertical)
                return_SIZE(20, 7, 7); // Interface Builder
            else
                return_SIZE(20, 8, 8);
        }
        return_SIZE(14, 8, 8);     // Interface Builder
    }

    switch (CT2(control1, control2)) {
    case CT1(QSizePolicy::Label):                             // guess
    case CT2(QSizePolicy::Label, QSizePolicy::DefaultType):   // guess
    case CT2(QSizePolicy::Label, QSizePolicy::CheckBox):      // AHIG
    case CT2(QSizePolicy::Label, QSizePolicy::ComboBox):      // AHIG
    case CT2(QSizePolicy::Label, QSizePolicy::LineEdit):      // guess
    case CT2(QSizePolicy::Label, QSizePolicy::RadioButton):   // AHIG
    case CT2(QSizePolicy::Label, QSizePolicy::Slider):        // guess
    case CT2(QSizePolicy::Label, QSizePolicy::SpinBox):       // guess
    case CT2(QSizePolicy::Label, QSizePolicy::ToolButton):    // guess
        return_SIZE(8, 6, 5);
    case CT1(QSizePolicy::ToolButton):
        return 8;   // AHIG
    case CT1(QSizePolicy::CheckBox):
    case CT2(QSizePolicy::CheckBox, QSizePolicy::RadioButton):
    case CT2(QSizePolicy::RadioButton, QSizePolicy::CheckBox):
        if (orientation == Qt::Vertical)
            return_SIZE(8, 8, 7);        // AHIG and Builder
        break;
    case CT1(QSizePolicy::RadioButton):
        if (orientation == Qt::Vertical)
            return 5;                   // (Builder, guess, AHIG)
    }

    if (orientation == Qt::Horizontal
            && (control2 & (QSizePolicy::CheckBox | QSizePolicy::RadioButton)))
        return_SIZE(12, 10, 8);        // guess

    if ((control1 | control2) & (QSizePolicy::Frame
                                 | QSizePolicy::GroupBox
                                 | QSizePolicy::TabWidget)) {
        /*
            These values were chosen so that nested container widgets
            look good side by side. Builder uses 8, which looks way
            too small, and AHIG doesn't say anything.
        */
        return_SIZE(16, 10, 10);    // guess
    }

    if ((control1 | control2) & (QSizePolicy::Line | QSizePolicy::Slider))
        return_SIZE(12, 10, 8);     // AHIG

    if ((control1 | control2) & QSizePolicy::LineEdit)
        return_SIZE(10, 8, 8);      // AHIG

    /*
        AHIG and Builder differ by up to 4 pixels for stacked editable
        comboboxes. We use some values that work fairly well in all
        cases.
    */
    if ((control1 | control2) & QSizePolicy::ComboBox)
        return_SIZE(10, 8, 7);      // guess

    /*
        Builder defaults to 8, 6, 5 in lots of cases, but most of the time the
        result looks too cramped.
    */
    return_SIZE(10, 8, 6);  // guess
}

QT_END_NAMESPACE
