// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#include "qfusionstyle_p.h"
#include "qfusionstyle_p_p.h"

#if QT_CONFIG(style_fusion) || defined(QT_PLUGIN)
#include "qcommonstyle_p.h"
#if QT_CONFIG(combobox)
#include <qcombobox.h>
#endif
#if QT_CONFIG(pushbutton)
#include <qpushbutton.h>
#endif
#if QT_CONFIG(abstractbutton)
#include <qabstractbutton.h>
#endif
#include <qpainter.h>
#include <qpainterpath.h>
#include <qpainterstateguard.h>
#include <qdir.h>
#include <qstyleoption.h>
#include <qapplication.h>
#if QT_CONFIG(mainwindow)
#include <qmainwindow.h>
#endif
#include <qfont.h>
#if QT_CONFIG(groupbox)
#include <qgroupbox.h>
#endif
#include <qpixmapcache.h>
#if QT_CONFIG(scrollbar)
#include <qscrollbar.h>
#endif
#if QT_CONFIG(spinbox)
#include <qspinbox.h>
#endif
#if QT_CONFIG(abstractslider)
#include <qabstractslider.h>
#endif
#if QT_CONFIG(slider)
#include <qslider.h>
#endif
#if QT_CONFIG(splitter)
#include <qsplitter.h>
#endif
#if QT_CONFIG(progressbar)
#include <qprogressbar.h>
#endif
#if QT_CONFIG(wizard)
#include <qwizard.h>
#endif
#include <qdrawutil.h>
#include <private/qstylehelper_p.h>
#include <private/qdrawhelper_p.h>
#include <private/qapplication_p.h>
#include <private/qwidget_p.h>

QT_BEGIN_NAMESPACE

using namespace Qt::StringLiterals;
using namespace QStyleHelper;

enum Direction {
    TopDown,
    FromLeft,
    BottomUp,
    FromRight
};

// from windows style
static const int windowsItemFrame        =  2; // menu item frame width
static const int windowsItemVMargin      =  8; // menu item ver text margin

static const int groupBoxBottomMargin    =  0;  // space below the groupbox
static const int groupBoxTopMargin       =  3;

#if QT_CONFIG(imageformat_xpm)
static const char * const qt_titlebar_context_help[] = {
    "10 10 3 1",
    "  c None",
    "# c #000000",
    "+ c #444444",
    "  +####+  ",
    " ###  ### ",
    " ##    ## ",
    "     +##+ ",
    "    +##   ",
    "    ##    ",
    "    ##    ",
    "          ",
    "    ##    ",
    "    ##    "};
#endif // QT_CONFIG(imageformat_xpm)

static QColor mergedColors(const QColor &colorA, const QColor &colorB, int factor = 50)
{
    const int maxFactor = 100;
    QColor tmp = colorA;
    tmp.setRed((tmp.red() * factor) / maxFactor + (colorB.red() * (maxFactor - factor)) / maxFactor);
    tmp.setGreen((tmp.green() * factor) / maxFactor + (colorB.green() * (maxFactor - factor)) / maxFactor);
    tmp.setBlue((tmp.blue() * factor) / maxFactor + (colorB.blue() * (maxFactor - factor)) / maxFactor);
    return tmp;
}

// The default button and handle gradient
static QLinearGradient qt_fusion_gradient(const QRect &rect, const QBrush &baseColor, Direction direction = TopDown)
{
    int x = rect.center().x();
    int y = rect.center().y();
    QLinearGradient gradient;
    switch (direction) {
    case FromLeft:
        gradient = QLinearGradient(rect.left(), y, rect.right(), y);
        break;
    case FromRight:
        gradient = QLinearGradient(rect.right(), y, rect.left(), y);
        break;
    case BottomUp:
        gradient = QLinearGradient(x, rect.bottom(), x, rect.top());
        break;
    case TopDown:
    default:
        gradient = QLinearGradient(x, rect.top(), x, rect.bottom());
        break;
    }
    if (baseColor.gradient())
        gradient.setStops(baseColor.gradient()->stops());
    else {
        QColor gradientStartColor = baseColor.color().lighter(124);
        QColor gradientStopColor = baseColor.color().lighter(102);
        gradient.setColorAt(0, gradientStartColor);
        gradient.setColorAt(1, gradientStopColor);
        //          Uncomment for adding shiny shading
        //            QColor midColor1 = mergedColors(gradientStartColor, gradientStopColor, 55);
        //            QColor midColor2 = mergedColors(gradientStartColor, gradientStopColor, 45);
        //            gradient.setColorAt(0.5, midColor1);
        //            gradient.setColorAt(0.501, midColor2);
    }
    return gradient;
}

static void qt_fusion_draw_arrow(Qt::ArrowType type, QPainter *painter, const QStyleOption *option, const QRect &rect, const QColor &color)
{
    if (rect.isEmpty())
        return;

    const qreal dpi = QStyleHelper::dpi(option);
    const int arrowWidth = int(QStyleHelper::dpiScaled(14, dpi));
    const int arrowHeight = int(QStyleHelper::dpiScaled(8, dpi));

    const int arrowMax = qMin(arrowHeight, arrowWidth);
    const int rectMax = qMin(rect.height(), rect.width());
    const int size = qMin(arrowMax, rectMax);

    const QString pixmapName = "fusion-arrow"_L1
                               % HexString<uint>(type)
                               % HexString<uint>(color.rgba());
    QCachedPainter cp(painter, pixmapName, option, rect.size(), rect);
    if (cp.needsPainting()) {
        QRectF arrowRect(0, 0, size, arrowHeight * size / arrowWidth);
        if (type == Qt::LeftArrow || type == Qt::RightArrow)
            arrowRect = arrowRect.transposed();
        arrowRect.moveTo((rect.width() - arrowRect.width()) / 2.0,
                         (rect.height() - arrowRect.height()) / 2.0);

        std::array<QPointF, 3> triangle;
        switch (type) {
        case Qt::DownArrow:
            triangle = {arrowRect.topLeft(), arrowRect.topRight(), QPointF(arrowRect.center().x(), arrowRect.bottom())};
            break;
        case Qt::RightArrow:
            triangle = {arrowRect.topLeft(), arrowRect.bottomLeft(), QPointF(arrowRect.right(), arrowRect.center().y())};
            break;
        case Qt::LeftArrow:
            triangle = {arrowRect.topRight(), arrowRect.bottomRight(), QPointF(arrowRect.left(), arrowRect.center().y())};
            break;
        default:
            triangle = {arrowRect.bottomLeft(), arrowRect.bottomRight(), QPointF(arrowRect.center().x(), arrowRect.top())};
            break;
        }

        cp->setPen(Qt::NoPen);
        cp->setBrush(color);
        cp->setRenderHint(QPainter::Antialiasing);
        cp->drawPolygon(triangle.data(), int(triangle.size()));
    }
}

static void qt_fusion_draw_mdibutton(QPainter *painter, const QStyleOptionTitleBar *option, const QRect &tmp, bool hover, bool sunken)
{
    bool active = (option->titleBarState & QStyle::State_Active);
    const QColor dark = QColor::fromHsv(option->palette.button().color().hue(),
                                        qMin(255, option->palette.button().color().saturation()),
                                        qMin(255, int(option->palette.button().color().value() * 0.7)));
    const QColor highlight = option->palette.highlight().color();
    const QColor titleBarHighlight(sunken ? highlight.darker(130) : QColor(255, 255, 255, 60));
    const QColor mdiButtonBorderColor(active ? highlight.darker(180): dark.darker(110));

    if (sunken)
        painter->fillRect(tmp.adjusted(1, 1, -1, -1), highlight.darker(120));
    else if (hover)
        painter->fillRect(tmp.adjusted(1, 1, -1, -1), QColor(255, 255, 255, 20));

    const QColor mdiButtonGradientStartColor(0, 0, 0, 40);
    const QColor mdiButtonGradientStopColor(255, 255, 255, 60);

    QLinearGradient gradient(tmp.center().x(), tmp.top(), tmp.center().x(), tmp.bottom());
    gradient.setColorAt(0, mdiButtonGradientStartColor);
    gradient.setColorAt(1, mdiButtonGradientStopColor);

    painter->setPen(mdiButtonBorderColor);
    const QLine lines[4] = {
        QLine(tmp.left() + 2, tmp.top(), tmp.right() - 2, tmp.top()),
        QLine(tmp.left() + 2, tmp.bottom(), tmp.right() - 2, tmp.bottom()),
        QLine(tmp.left(), tmp.top() + 2, tmp.left(), tmp.bottom() - 2),
        QLine(tmp.right(), tmp.top() + 2, tmp.right(), tmp.bottom() - 2)
    };
    painter->drawLines(lines, 4);
    const QPoint points[4] = {
        QPoint(tmp.left() + 1, tmp.top() + 1),
        QPoint(tmp.right() - 1, tmp.top() + 1),
        QPoint(tmp.left() + 1, tmp.bottom() - 1),
        QPoint(tmp.right() - 1, tmp.bottom() - 1)
    };
    painter->drawPoints(points, 4);

    painter->setPen(titleBarHighlight);
    painter->drawLine(tmp.left() + 2, tmp.top() + 1, tmp.right() - 2, tmp.top() + 1);
    painter->drawLine(tmp.left() + 1, tmp.top() + 2, tmp.left() + 1, tmp.bottom() - 2);

    painter->setPen(QPen(gradient, 1));
    painter->drawLine(tmp.right() + 1, tmp.top() + 2, tmp.right() + 1, tmp.bottom() - 2);
    painter->drawPoint(tmp.right() , tmp.top() + 1);

    painter->drawLine(tmp.left() + 2, tmp.bottom() + 1, tmp.right() - 2, tmp.bottom() + 1);
    painter->drawPoint(tmp.left() + 1, tmp.bottom());
    painter->drawPoint(tmp.right() - 1, tmp.bottom());
    painter->drawPoint(tmp.right() , tmp.bottom() - 1);
}

/*
    \internal
*/
QFusionStylePrivate::QFusionStylePrivate()
{
    animationFps = 60;
}

/*!
    \class QFusionStyle
    \brief The QFusionStyle class provides a custom widget style

    \inmodule QtWidgets
    \internal

    The Fusion style provides a custom look and feel that is not
    tied to a particular platform.
    //{Fusion Style Widget Gallery}
    \sa QWindowsStyle, QWindowsVistaStyle, QMacStyle, QCommonStyle
*/

/*!
    Constructs a QFusionStyle object.
*/
QFusionStyle::QFusionStyle() : QCommonStyle(*new QFusionStylePrivate)
{
    setObjectName("Fusion"_L1);
}

/*!
    \internal

    Constructs a QFusionStyle object.
*/
QFusionStyle::QFusionStyle(QFusionStylePrivate &dd) : QCommonStyle(dd)
{
}

/*!
    Destroys the QFusionStyle object.
*/
QFusionStyle::~QFusionStyle()
{
}

/*!
    \reimp
*/
void QFusionStyle::drawItemText(QPainter *painter, const QRect &rect, int alignment, const QPalette &pal,
                                bool enabled, const QString& text, QPalette::ColorRole textRole) const
{
    Q_UNUSED(enabled);
    if (text.isEmpty())
        return;

    QPen savedPen = painter->pen();
    if (textRole != QPalette::NoRole)
        painter->setPen(QPen(pal.brush(textRole), savedPen.widthF()));
    painter->drawText(rect, alignment, text);
    painter->setPen(savedPen);
}


/*!
    \reimp
*/
void QFusionStyle::drawPrimitive(PrimitiveElement elem,
                                 const QStyleOption *option,
                                 QPainter *painter, const QWidget *widget) const
{
    Q_ASSERT(option);
    Q_D (const QFusionStyle);

    const QColor outline = d->outline(option->palette);
    const QColor highlightedOutline = d->highlightedOutline(option->palette);

    switch (elem) {

#if QT_CONFIG(groupbox)
    // No frame drawn
    case PE_FrameGroupBox:
    {
        const auto strFrameGroupBox = QStringLiteral(u"fusion_groupbox");
        QPixmap pixmap;
        if (!QPixmapCache::find(strFrameGroupBox, &pixmap)) {
            pixmap.load(":/qt-project.org/styles/commonstyle/images/fusion_groupbox.png"_L1);
            QPixmapCache::insert(strFrameGroupBox, pixmap);
        }
        qDrawBorderPixmap(painter, option->rect, QMargins(6, 6, 6, 6), pixmap);
        break;
    }
#endif // QT_CONFIG(groupbox)
    case PE_IndicatorBranch: {
        if (!(option->state & State_Children))
            break;
        if (option->state & State_Open)
            drawPrimitive(PE_IndicatorArrowDown, option, painter, widget);
        else {
            const bool reverse = (option->direction == Qt::RightToLeft);
            drawPrimitive(reverse ? PE_IndicatorArrowLeft : PE_IndicatorArrowRight, option, painter, widget);
        }
        break;
    }
#if QT_CONFIG(tabbar)
    case PE_FrameTabBarBase:
        if (const QStyleOptionTabBarBase *tbb
                = qstyleoption_cast<const QStyleOptionTabBarBase *>(option)) {
            painter->save();
            painter->setPen(outline.lighter(110));
            switch (tbb->shape) {
            case QTabBar::RoundedNorth: {
                QRegion region(tbb->rect);
                region -= tbb->selectedTabRect;
                painter->drawLine(tbb->rect.topLeft(), tbb->rect.topRight());
                painter->setClipRegion(region);
                painter->setPen(option->palette.light().color());
                painter->drawLine(tbb->rect.topLeft() + QPoint(0, 1), tbb->rect.topRight() + QPoint(0, 1));
            }
                break;
            case QTabBar::RoundedWest:
                painter->drawLine(tbb->rect.left(), tbb->rect.top(), tbb->rect.left(), tbb->rect.bottom());
                break;
            case QTabBar::RoundedSouth:
                painter->drawLine(tbb->rect.left(), tbb->rect.bottom(),
                                  tbb->rect.right(), tbb->rect.bottom());
                break;
            case QTabBar::RoundedEast:
                painter->drawLine(tbb->rect.topRight(), tbb->rect.bottomRight());
                break;
            case QTabBar::TriangularNorth:
            case QTabBar::TriangularEast:
            case QTabBar::TriangularWest:
            case QTabBar::TriangularSouth:
                painter->restore();
                QCommonStyle::drawPrimitive(elem, option, painter, widget);
                return;
            }
            painter->restore();
        }
        return;
#endif // QT_CONFIG(tabbar)
    case PE_PanelScrollAreaCorner: {
        painter->save();
        QColor alphaOutline = outline;
        alphaOutline.setAlpha(180);
        painter->setPen(alphaOutline);
        painter->setBrush(option->palette.brush(QPalette::Window));
        painter->drawRect(option->rect);
        painter->restore();
    } break;
    case PE_IndicatorArrowUp:
    case PE_IndicatorArrowDown:
    case PE_IndicatorArrowRight:
    case PE_IndicatorArrowLeft:
    {
        if (option->rect.width() <= 1 || option->rect.height() <= 1)
            break;
        QColor arrowColor = option->palette.windowText().color();
        arrowColor.setAlpha(160);
        Qt::ArrowType arrow = Qt::UpArrow;
        switch (elem) {
        case PE_IndicatorArrowDown:
            arrow = Qt::DownArrow;
            break;
        case PE_IndicatorArrowRight:
            arrow = Qt::RightArrow;
            break;
        case PE_IndicatorArrowLeft:
            arrow = Qt::LeftArrow;
            break;
        default:
            break;
        }
        qt_fusion_draw_arrow(arrow, painter, option, option->rect, arrowColor);
    }
        break;
    case PE_IndicatorItemViewItemCheck:
    {
        QStyleOptionButton button;
        button.QStyleOption::operator=(*option);
        button.state &= ~State_MouseOver;
        proxy()->drawPrimitive(PE_IndicatorCheckBox, &button, painter, widget);
    }
        return;
    case PE_IndicatorHeaderArrow:
        if (const QStyleOptionHeader *header = qstyleoption_cast<const QStyleOptionHeader *>(option)) {
            const QRect r = header->rect.translated(0, -2);
            QColor arrowColor = header->palette.windowText().color();
            arrowColor.setAlpha(180);

            Qt::ArrowType arrowType = Qt::UpArrow;
#if defined(Q_OS_LINUX)
            if (header->sortIndicator & QStyleOptionHeader::SortDown)
                arrowType = Qt::DownArrow;
#else
            if (header->sortIndicator & QStyleOptionHeader::SortUp)
                arrowType = Qt::DownArrow;
#endif
            qt_fusion_draw_arrow(arrowType, painter, option, r, arrowColor);
        }
        break;
    case PE_IndicatorButtonDropDown:
        proxy()->drawPrimitive(PE_PanelButtonCommand, option, painter, widget);
        break;

    case PE_IndicatorToolBarSeparator:
    {
        const QRect &rect = option->rect;
        const int margin = 6;
        const QColor col = option->palette.window().color();
        painter->setPen(col.darker(110));
        if (option->state & State_Horizontal) {
            const int offset = rect.width() / 2;
            painter->drawLine(rect.left() + offset, rect.bottom() - margin,
                              rect.left() + offset, rect.top() + margin);
            painter->setPen(col.lighter(110));
            painter->drawLine(rect.left() + offset + 1, rect.bottom() - margin,
                              rect.left() + offset + 1, rect.top() + margin);
        } else { //Draw vertical separator
            const int offset = rect.height() / 2;
            painter->drawLine(rect.left() + margin, rect.top() + offset,
                              rect.right() - margin, rect.top() + offset);
            painter->setPen(col.lighter(110));
            painter->drawLine(rect.left() + margin, rect.top() + offset + 1,
                              rect.right() - margin, rect.top() + offset + 1);
        }
    }
        break;
    case PE_Frame: {
        if (widget && widget->inherits("QComboBoxPrivateContainer")){
            QStyleOption copy = *option;
            copy.state |= State_Raised;
            proxy()->drawPrimitive(PE_PanelMenu, &copy, painter, widget);
            break;
        }
        painter->save();
        painter->setPen(outline.lighter(108));
        painter->drawRect(option->rect.adjusted(0, 0, -1, -1));
        painter->restore(); }
        break;
    case PE_FrameMenu:
        painter->save();
    {
        painter->setPen(outline);
        painter->drawRect(option->rect.adjusted(0, 0, -1, -1));
        QColor frameLight = option->palette.window().color().lighter(160);
        QColor frameShadow = option->palette.window().color().darker(110);

        //paint beveleffect
        QRect frame = option->rect.adjusted(1, 1, -1, -1);
        painter->setPen(frameLight);
        painter->drawLine(frame.topLeft(), frame.bottomLeft());
        painter->drawLine(frame.topLeft(), frame.topRight());

        painter->setPen(frameShadow);
        painter->drawLine(frame.topRight(), frame.bottomRight());
        painter->drawLine(frame.bottomLeft(), frame.bottomRight());
    }
        painter->restore();
        break;
    case PE_PanelButtonTool:
        painter->save();
        if ((option->state & State_Enabled || option->state & State_On) || !(option->state & State_AutoRaise)) {
            if (widget && widget->inherits("QDockWidgetTitleButton")) {
                if (option->state & State_MouseOver)
                    proxy()->drawPrimitive(PE_PanelButtonCommand, option, painter, widget);
            } else {
                proxy()->drawPrimitive(PE_PanelButtonCommand, option, painter, widget);
            }
        }
        painter->restore();
        break;
    case PE_IndicatorDockWidgetResizeHandle:
    {
        QStyleOption dockWidgetHandle = *option;
        bool horizontal = option->state & State_Horizontal;
        dockWidgetHandle.state.setFlag(State_Horizontal, !horizontal);
        proxy()->drawControl(CE_Splitter, &dockWidgetHandle, painter, widget);
    }
        break;
    case PE_FrameDockWidget:
    case PE_FrameWindow:
    {
        painter->save();
        const QRect &rect = option->rect;
        const QColor col = (elem == PE_FrameWindow) ? outline.darker(150)
                                                    : option->palette.window().color().darker(120);
        painter->setPen(col);
        painter->drawRect(rect.adjusted(0, 0, -1, -1));
        painter->setPen(QPen(option->palette.light(), 1));
        painter->drawLine(rect.left() + 1, rect.top() + 1,
                          rect.left() + 1, rect.bottom() - 1);
        painter->setPen(option->palette.window().color().darker(120));
        const QLine lines[2] = {{rect.left() + 1, rect.bottom() - 1,
                                 rect.right() - 2, rect.bottom() - 1},
                                {rect.right() - 1, rect.top() + 1,
                                 rect.right() - 1, rect.bottom() - 1}};
        painter->drawLines(lines, 2);
        painter->restore();
        break;
    }
    case PE_FrameLineEdit:
    {
        const QRect &r = option->rect;
        bool hasFocus = option->state & State_HasFocus;

        painter->save();

        painter->setRenderHint(QPainter::Antialiasing, true);
        //  ### highdpi painter bug.
        painter->translate(0.5, 0.5);

        // Draw Outline
        painter->setPen(hasFocus ? highlightedOutline : outline);
        painter->drawRoundedRect(r.adjusted(0, 0, -1, -1), 2, 2);

        if (hasFocus) {
            QColor softHighlight = highlightedOutline;
            softHighlight.setAlpha(40);
            painter->setPen(softHighlight);
            painter->drawRoundedRect(r.adjusted(1, 1, -2, -2), 1.7, 1.7);
        }
        // Draw inner shadow
        painter->setPen(QFusionStylePrivate::topShadow);
        painter->drawLine(QPoint(r.left() + 2, r.top() + 1), QPoint(r.right() - 2, r.top() + 1));

        painter->restore();

    }
        break;
    case PE_IndicatorCheckBox:
        painter->save();
        if (const QStyleOptionButton *checkbox = qstyleoption_cast<const QStyleOptionButton*>(option)) {
            painter->setRenderHint(QPainter::Antialiasing, true);
            painter->translate(0.5, 0.5);
            const QRect rect = option->rect.adjusted(0, 0, -1, -1);

            QColor pressedColor = mergedColors(option->palette.base().color(), option->palette.windowText().color(), 85);
            painter->setBrush(Qt::NoBrush);

            const auto state = option->state;
            // Gradient fill
            QLinearGradient gradient(rect.topLeft(), rect.bottomLeft());
            gradient.setColorAt(0, (state & State_Sunken) ? pressedColor : option->palette.base().color().darker(115));
            gradient.setColorAt(0.15, (state & State_Sunken) ? pressedColor : option->palette.base().color());
            gradient.setColorAt(1, (state & State_Sunken) ? pressedColor : option->palette.base().color());

            painter->setBrush((state & State_Sunken) ? QBrush(pressedColor) : gradient);

            if (option->state & State_HasFocus && option->state & State_KeyboardFocusChange)
                painter->setPen(highlightedOutline);
            else
                painter->setPen(colorScheme() == Qt::ColorScheme::Dark ? outline.lighter(240)
                                                                       : outline.lighter(110));
            painter->drawRect(rect);

            QColor checkMarkColor = option->palette.text().color().darker(120);
            const qreal checkMarkPadding = 1 + rect.width() * 0.13; // at least one pixel padding

            if (checkbox->state & State_NoChange) {
                gradient = QLinearGradient(rect.topLeft(), rect.bottomLeft());
                checkMarkColor.setAlpha(80);
                gradient.setColorAt(0, checkMarkColor);
                checkMarkColor.setAlpha(140);
                gradient.setColorAt(1, checkMarkColor);
                checkMarkColor.setAlpha(180);
                painter->setPen(checkMarkColor);
                painter->setBrush(gradient);
                painter->drawRect(rect.adjusted(checkMarkPadding, checkMarkPadding, -checkMarkPadding, -checkMarkPadding));

            } else if (checkbox->state & State_On) {
                const qreal dpi = QStyleHelper::dpi(option);
                qreal penWidth = QStyleHelper::dpiScaled(1.5, dpi);
                penWidth = qMax<qreal>(penWidth, 0.13 * rect.height());
                penWidth = qMin<qreal>(penWidth, 0.20 * rect.height());
                QPen checkPen = QPen(checkMarkColor, penWidth);
                checkMarkColor.setAlpha(210);
                painter->translate(dpiScaled(-0.8, dpi), dpiScaled(0.5, dpi));
                painter->setPen(checkPen);
                painter->setBrush(Qt::NoBrush);

                // Draw checkmark
                QPainterPath path;
                const qreal rectHeight = rect.height(); // assuming height equals width
                path.moveTo(checkMarkPadding + rectHeight * 0.11, rectHeight * 0.47);
                path.lineTo(rectHeight * 0.5, rectHeight - checkMarkPadding);
                path.lineTo(rectHeight - checkMarkPadding, checkMarkPadding);
                painter->drawPath(path.translated(rect.topLeft()));
            }
        }
        painter->restore();
        break;
    case PE_IndicatorRadioButton:
        painter->save();
    {
        QColor pressedColor = mergedColors(option->palette.base().color(), option->palette.windowText().color(), 85);
        painter->setBrush((option->state & State_Sunken) ? pressedColor : option->palette.base().color());
        painter->setRenderHint(QPainter::Antialiasing, true);
        QPainterPath circle;
        const QRect &rect = option->rect;
        const QPointF circleCenter = rect.center() + QPoint(1, 1);
        const qreal outlineRadius = (rect.width() + (rect.width() + 1) % 2) / 2.0 - 1;
        circle.addEllipse(circleCenter, outlineRadius, outlineRadius);
        if (option->state & State_HasFocus && option->state & State_KeyboardFocusChange)
            painter->setPen(highlightedOutline);
        else if (isHighContrast())
            painter->setPen(outline);
        else
            painter->setPen(option->palette.window().color().darker(150));
        painter->drawPath(circle);

        if (option->state & State_On) {
            circle = QPainterPath();
            const qreal checkmarkRadius = outlineRadius / 2.32;
            circle.addEllipse(circleCenter, checkmarkRadius, checkmarkRadius);
            QColor checkMarkColor = option->palette.text().color().darker(120);
            checkMarkColor.setAlpha(200);
            painter->setPen(checkMarkColor);
            checkMarkColor.setAlpha(180);
            painter->setBrush(checkMarkColor);
            painter->drawPath(circle);
        }
    }
        painter->restore();
        break;
    case PE_IndicatorToolBarHandle:
    {
        const QPoint center = option->rect.center();
        //draw grips
        if (option->state & State_Horizontal) {
            for (int i = -3 ; i < 2 ; i += 3) {
                for (int j = -8 ; j < 10 ; j += 3) {
                    painter->fillRect(center.x() + i, center.y() + j, 2, 2, QFusionStylePrivate::lightShade);
                    painter->fillRect(center.x() + i, center.y() + j, 1, 1, QFusionStylePrivate::darkShade);
                }
            }
        } else { //vertical toolbar
            for (int i = -6 ; i < 12 ; i += 3) {
                for (int j = -3 ; j < 2 ; j += 3) {
                    painter->fillRect(center.x() + i, center.y() + j, 2, 2, QFusionStylePrivate::lightShade);
                    painter->fillRect(center.x() + i, center.y() + j, 1, 1, QFusionStylePrivate::darkShade);
                }
            }
        }
        break;
    }
    case PE_FrameDefaultButton:
        break;
    case PE_FrameFocusRect:
        if (const QStyleOptionFocusRect *fropt = qstyleoption_cast<const QStyleOptionFocusRect *>(option)) {
            //### check for d->alt_down
            if (!(fropt->state & State_KeyboardFocusChange))
                return;
            const QRect &rect = option->rect;

            painter->save();
            painter->setRenderHint(QPainter::Antialiasing, true);
            painter->translate(0.5, 0.5);
            QColor fillcolor = highlightedOutline;
            fillcolor.setAlpha(80);
            painter->setPen(fillcolor.darker(120));
            fillcolor.setAlpha(30);
            QLinearGradient gradient(rect.topLeft(), rect.bottomLeft());
            gradient.setColorAt(0, fillcolor.lighter(160));
            gradient.setColorAt(1, fillcolor);
            painter->setBrush(gradient);
            painter->drawRoundedRect(rect.adjusted(0, 0, -1, -1), 1, 1);
            painter->restore();
        }
        break;
    case PE_PanelButtonCommand:
    {
        bool isDefault = false;
        bool isFlat = false;
        bool isDown = (option->state & State_Sunken) || (option->state & State_On);

        if (const QStyleOptionButton *button = qstyleoption_cast<const QStyleOptionButton*>(option)) {
            isDefault = (button->features & QStyleOptionButton::DefaultButton) && (button->state & State_Enabled);
            isFlat = (button->features & QStyleOptionButton::Flat);
        }

        if (isFlat && !isDown) {
            if (isDefault) {
                const QRect r = option->rect.adjusted(0, 1, 0, -1);
                painter->setPen(Qt::black);
                const QLine lines[4] = {
                    QLine(QPoint(r.left() + 2, r.top()),
                    QPoint(r.right() - 2, r.top())),
                    QLine(QPoint(r.left(), r.top() + 2),
                    QPoint(r.left(), r.bottom() - 2)),
                    QLine(QPoint(r.right(), r.top() + 2),
                    QPoint(r.right(), r.bottom() - 2)),
                    QLine(QPoint(r.left() + 2, r.bottom()),
                    QPoint(r.right() - 2, r.bottom()))
                };
                painter->drawLines(lines, 4);
                const QPoint points[4] = {
                    QPoint(r.right() - 1, r.bottom() - 1),
                    QPoint(r.right() - 1, r.top() + 1),
                    QPoint(r.left() + 1, r.bottom() - 1),
                    QPoint(r.left() + 1, r.top() + 1)
                };
                painter->drawPoints(points, 4);
            }
            return;
        }


        bool isEnabled = option->state & State_Enabled;
        bool hasFocus = (option->state & State_HasFocus && option->state & State_KeyboardFocusChange);
        QColor buttonColor = d->buttonColor(option->palette);


        if (isDefault)
            buttonColor = mergedColors(buttonColor, highlightedOutline.lighter(130), 90);

        QCachedPainter p(painter, u"pushbutton-" + buttonColor.name(QColor::HexArgb), option);
        if (p.needsPainting()) {
            const QRect rect = p.pixmapRect();
            const QRect r = rect.adjusted(0, 1, -1, 0);
            const QColor &darkOutline = (hasFocus | isDefault) ? highlightedOutline : outline;

            p->setRenderHint(QPainter::Antialiasing, true);
            p->translate(0.5, -0.5);

            QLinearGradient gradient = qt_fusion_gradient(rect, (isEnabled && option->state & State_MouseOver ) ? buttonColor : buttonColor.darker(104));
            p->setPen(Qt::transparent);
            p->setBrush(isDown ? QBrush(buttonColor.darker(110)) : gradient);
            p->drawRoundedRect(r, 2.0, 2.0);
            p->setBrush(Qt::NoBrush);

            // Outline
            p->setPen(!isEnabled ? darkOutline.lighter(115) : darkOutline);
            p->drawRoundedRect(r, 2.0, 2.0);

            p->setPen(QFusionStylePrivate::innerContrastLine);
            p->drawRoundedRect(r.adjusted(1, 1, -1, -1), 2.0, 2.0);
        }
        }
        break;
    case PE_FrameTabWidget:
        painter->save();
        painter->fillRect(option->rect.adjusted(0, 0, -1, -1), d->tabFrameColor(option->palette));
#if QT_CONFIG(tabwidget)
        if (const QStyleOptionTabWidgetFrame *twf = qstyleoption_cast<const QStyleOptionTabWidgetFrame *>(option)) {
            QRect rect = option->rect.adjusted(0, 0, -1, -1);

            // Shadow outline
            if (twf->shape != QTabBar::RoundedSouth) {
                rect.adjust(0, 0, 0, -1);
                QColor borderColor = outline.lighter(110);
                QColor alphaShadow(Qt::black);
                alphaShadow.setAlpha(15);
                painter->setPen(alphaShadow);
                painter->drawLine(option->rect.bottomLeft(), option->rect.bottomRight());
                painter->setPen(borderColor);
            }

            // outline
            painter->setPen(outline);
            painter->drawRect(rect);

            // Inner frame highlight
            painter->setPen(QFusionStylePrivate::innerContrastLine);
            painter->drawRect(rect.adjusted(1, 1, -1, -1));

        }
#endif // QT_CONFIG(tabwidget)
        painter->restore();
        break ;

    case PE_FrameStatusBarItem:
        break;
    case PE_PanelMenu: {
        painter->save();
        const QBrush menuBackground = option->palette.base().color().lighter(108);
        QColor borderColor = option->palette.window().color().darker(160);
        qDrawPlainRect(painter, option->rect, borderColor, 1, &menuBackground);
        painter->restore();
    }
        break;

    default:
        QCommonStyle::drawPrimitive(elem, option, painter, widget);
        break;
    }
}

/*!
  \reimp
*/
void QFusionStyle::drawControl(ControlElement element, const QStyleOption *option, QPainter *painter,
                               const QWidget *widget) const
{
    Q_D (const QFusionStyle);

    switch (element) {
    case CE_ComboBoxLabel:
        if (const QStyleOptionComboBox *cb = qstyleoption_cast<const QStyleOptionComboBox *>(option)) {
            QRect editRect = proxy()->subControlRect(CC_ComboBox, cb, SC_ComboBoxEditField, widget);
            painter->save();
            painter->setClipRect(editRect);
            if (!cb->currentIcon.isNull()) {
                QIcon::Mode mode = cb->state & State_Enabled ? QIcon::Normal
                                                             : QIcon::Disabled;
                QPixmap pixmap = cb->currentIcon.pixmap(cb->iconSize, QStyleHelper::getDpr(painter), mode);
                QRect iconRect(editRect);
                iconRect.setWidth(cb->iconSize.width() + 4);
                iconRect = alignedRect(cb->direction,
                                       Qt::AlignLeft | Qt::AlignVCenter,
                                       iconRect.size(), editRect);
                if (cb->editable)
                    painter->fillRect(iconRect, cb->palette.brush(QPalette::Base));
                proxy()->drawItemPixmap(painter, iconRect, Qt::AlignCenter, pixmap);

                if (cb->direction == Qt::RightToLeft)
                    editRect.translate(-4 - cb->iconSize.width(), 0);
                else
                    editRect.translate(cb->iconSize.width() + 4, 0);
            }
            if (!cb->currentText.isEmpty() && !cb->editable) {
                proxy()->drawItemText(painter, editRect.adjusted(1, 0, -1, 0),
                                      visualAlignment(cb->direction, cb->textAlignment),
                                      cb->palette, cb->state & State_Enabled, cb->currentText,
                                      cb->editable ? QPalette::Text : QPalette::ButtonText);
            }
            painter->restore();
        }
        break;
    case CE_Splitter:
    {
        // Don't draw handle for single pixel splitters
        if (option->rect.width() > 1 && option->rect.height() > 1) {
            const QPoint center = option->rect.center();
            //draw grips
            if (option->state & State_Horizontal) {
                for (int j = -6 ; j< 12 ; j += 3) {
                    painter->fillRect(center.x() + 1, center.y() + j, 2, 2, QFusionStylePrivate::lightShade);
                    painter->fillRect(center.x() + 1, center.y() + j, 1, 1, QFusionStylePrivate::darkShade);
                }
            } else {
                for (int i = -6; i< 12 ; i += 3) {
                    painter->fillRect(center.x() + i, center.y(), 2, 2, QFusionStylePrivate::lightShade);
                    painter->fillRect(center.x() + i, center.y(), 1, 1, QFusionStylePrivate::darkShade);
                }
            }
        }
        break;
    }
#if QT_CONFIG(rubberband)
    case CE_RubberBand:
        if (qstyleoption_cast<const QStyleOptionRubberBand *>(option)) {
            const QRect &rect = option->rect;
            QColor highlight = option->palette.color(QPalette::Active, QPalette::Highlight);
            painter->save();
            QColor penColor = highlight.darker(120);
            penColor.setAlpha(180);
            painter->setPen(penColor);
            QColor dimHighlight(qMin(highlight.red()/2 + 110, 255),
                                qMin(highlight.green()/2 + 110, 255),
                                qMin(highlight.blue()/2 + 110, 255));
            dimHighlight.setAlpha(widget && widget->isWindow() ? 255 : 80);
            QLinearGradient gradient(rect.topLeft(), rect.bottomLeft());
            gradient.setColorAt(0, dimHighlight.lighter(120));
            gradient.setColorAt(1, dimHighlight);
            painter->setRenderHint(QPainter::Antialiasing, true);
            painter->translate(0.5, 0.5);
            painter->setBrush(dimHighlight);
            painter->drawRoundedRect(rect.adjusted(0, 0, -1, -1), 1, 1);
            //when the rectangle we get is large enough, draw the inner rectangle.
            if (rect.width() > 2 && rect.height() > 2) {
                QColor innerLine = Qt::white;
                innerLine.setAlpha(40);
                painter->setPen(innerLine);
                painter->drawRoundedRect(rect.adjusted(1, 1, -2, -2), 1, 1);
            }
            painter->restore();
        }
        break;
#endif //QT_CONFIG(rubberband)
    case CE_SizeGrip:
        painter->save();
    {
        const QPoint center = option->rect.center();
        //draw grips
        for (int i = -6; i< 12 ; i += 3) {
            for (int j = -6 ; j< 12 ; j += 3) {
                if ((option->direction == Qt::LeftToRight && i > -j) || (option->direction == Qt::RightToLeft && j > i) ) {
                    painter->fillRect(center.x() + i, center.y() + j, 2, 2, QFusionStylePrivate::lightShade);
                    painter->fillRect(center.x() + i, center.y() + j, 1, 1, QFusionStylePrivate::darkShade);
                }
            }
        }
    }
        painter->restore();
        break;
#if QT_CONFIG(toolbar)
    case CE_ToolBar:
        if (const QStyleOptionToolBar *toolBar = qstyleoption_cast<const QStyleOptionToolBar *>(option)) {
            // Reserve the beveled appearance only for mainwindow toolbars
            if (widget && !(qobject_cast<const QMainWindow*> (widget->parentWidget())))
                break;

            const QRect &rect = option->rect;
            // Draws the light line above and the dark line below menu bars and
            // tool bars.
            QLinearGradient gradient(rect.topLeft(), rect.bottomLeft());
            if (!(option->state & State_Horizontal))
                gradient = QLinearGradient(rect.left(), rect.center().y(),
                                           rect.right(), rect.center().y());
            gradient.setColorAt(0, option->palette.window().color().lighter(104));
            gradient.setColorAt(1, option->palette.window().color());
            painter->fillRect(rect, gradient);

            constexpr QColor light = QFusionStylePrivate::lightShade;
            constexpr QColor shadow = QFusionStylePrivate::darkShade;

            QPen oldPen = painter->pen();
            if (toolBar->toolBarArea == Qt::TopToolBarArea) {
                if (toolBar->positionOfLine == QStyleOptionToolBar::End
                        || toolBar->positionOfLine == QStyleOptionToolBar::OnlyOne) {
                    // The end and onlyone top toolbar lines draw a double
                    // line at the bottom to blend with the central
                    // widget.
                    painter->setPen(light);
                    painter->drawLine(rect.bottomLeft(), rect.bottomRight());
                    painter->setPen(shadow);
                    painter->drawLine(rect.left(), rect.bottom() - 1,
                                      rect.right(), rect.bottom() - 1);
                } else {
                    // All others draw a single dark line at the bottom.
                    painter->setPen(shadow);
                    painter->drawLine(rect.bottomLeft(), rect.bottomRight());
                }
                // All top toolbar lines draw a light line at the top.
                painter->setPen(light);
                painter->drawLine(rect.topLeft(), rect.topRight());
            } else if (toolBar->toolBarArea == Qt::BottomToolBarArea) {
                if (toolBar->positionOfLine == QStyleOptionToolBar::End
                        || toolBar->positionOfLine == QStyleOptionToolBar::Middle) {
                    // The end and middle bottom tool bar lines draw a dark
                    // line at the bottom.
                    painter->setPen(shadow);
                    painter->drawLine(rect.bottomLeft(), rect.bottomRight());
                }
                if (toolBar->positionOfLine == QStyleOptionToolBar::Beginning
                        || toolBar->positionOfLine == QStyleOptionToolBar::OnlyOne) {
                    // The beginning and only one tool bar lines draw a
                    // double line at the bottom to blend with the
                    // status bar.
                    // ### The styleoption could contain whether the
                    // main window has a menu bar and a status bar, and
                    // possibly dock widgets.
                    painter->setPen(shadow);
                    painter->drawLine(rect.left(), rect.bottom() - 1,
                                      rect.right(), rect.bottom() - 1);
                    painter->setPen(light);
                    painter->drawLine(rect.bottomLeft(), rect.bottomRight());
                }
                if (toolBar->positionOfLine == QStyleOptionToolBar::End) {
                    painter->setPen(shadow);
                    painter->drawLine(rect.topLeft(), rect.topRight());
                    painter->setPen(light);
                    painter->drawLine(rect.left(), rect.top() + 1,
                                      rect.right(), rect.top() + 1);

                } else {
                    // All other bottom toolbars draw a light line at the top.
                    painter->setPen(light);
                    painter->drawLine(rect.topLeft(), rect.topRight());
                }
            }
            if (toolBar->toolBarArea == Qt::LeftToolBarArea) {
                if (toolBar->positionOfLine == QStyleOptionToolBar::Middle
                        || toolBar->positionOfLine == QStyleOptionToolBar::End) {
                    // The middle and left end toolbar lines draw a light
                    // line to the left.
                    painter->setPen(light);
                    painter->drawLine(rect.topLeft(), rect.bottomLeft());
                }
                if (toolBar->positionOfLine == QStyleOptionToolBar::End) {
                    // All other left toolbar lines draw a dark line to the right
                    painter->setPen(shadow);
                    painter->drawLine(rect.right() - 1, rect.top(),
                                      rect.right() - 1, rect.bottom());
                    painter->setPen(light);
                    painter->drawLine(rect.topRight(), rect.bottomRight());
                } else {
                    // All other left toolbar lines draw a dark line to the right
                    painter->setPen(shadow);
                    painter->drawLine(rect.topRight(), rect.bottomRight());
                }
            } else if (toolBar->toolBarArea == Qt::RightToolBarArea) {
                if (toolBar->positionOfLine == QStyleOptionToolBar::Middle
                        || toolBar->positionOfLine == QStyleOptionToolBar::End) {
                    // Right middle and end toolbar lines draw the dark right line
                    painter->setPen(shadow);
                    painter->drawLine(rect.topRight(), rect.bottomRight());
                }
                if (toolBar->positionOfLine == QStyleOptionToolBar::End
                        || toolBar->positionOfLine == QStyleOptionToolBar::OnlyOne) {
                    // The right end and single toolbar draws the dark
                    // line on its left edge
                    painter->setPen(shadow);
                    painter->drawLine(rect.topLeft(), rect.bottomLeft());
                    // And a light line next to it
                    painter->setPen(light);
                    painter->drawLine(rect.left() + 1, rect.top(),
                                      rect.left() + 1, rect.bottom());
                } else {
                    // Other right toolbars draw a light line on its left edge
                    painter->setPen(light);
                    painter->drawLine(rect.topLeft(), rect.bottomLeft());
                }
            }
            painter->setPen(oldPen);
        }
        break;
#endif // QT_CONFIG(toolbar)
    case CE_DockWidgetTitle:
        painter->save();
        if (const QStyleOptionDockWidget *dwOpt = qstyleoption_cast<const QStyleOptionDockWidget *>(option)) {
            bool verticalTitleBar = dwOpt->verticalTitleBar;

            QRect titleRect = subElementRect(SE_DockWidgetTitleBarText, option, widget);
            if (verticalTitleBar) {
                const QRect &rect = dwOpt->rect;
                const QRect r = rect.transposed();
                titleRect = QRect(r.left() + rect.bottom() - titleRect.bottom(),
                                  r.top() + titleRect.left() - rect.left(),
                                  titleRect.height(), titleRect.width());

                painter->translate(r.left(), r.top() + r.width());
                painter->rotate(-90);
                painter->translate(-r.left(), -r.top());
            }

            if (!dwOpt->title.isEmpty()) {
                QString titleText
                        = painter->fontMetrics().elidedText(dwOpt->title,
                                                            Qt::ElideRight, titleRect.width());
                proxy()->drawItemText(painter,
                                      titleRect,
                                      Qt::AlignLeft | Qt::AlignVCenter | Qt::TextHideMnemonic, dwOpt->palette,
                                      dwOpt->state & State_Enabled, titleText,
                                      QPalette::WindowText);
            }
        }
        painter->restore();
        break;
    case CE_HeaderSection:
        painter->save();
        // Draws the header in tables.
        if (const QStyleOptionHeader *header = qstyleoption_cast<const QStyleOptionHeader *>(option)) {
            const QStyleOptionHeaderV2 *headerV2 = qstyleoption_cast<const QStyleOptionHeaderV2 *>(option);
            const bool isSectionDragTarget = headerV2 ? headerV2->isSectionDragTarget : false;

            const QString pixmapName = "headersection-"_L1
                                       % HexString(header->position)
                                       % HexString(header->orientation)
                                       % QLatin1Char(isSectionDragTarget ? '1' : '0');
            QCachedPainter cp(painter, pixmapName, option);
            if (cp.needsPainting()) {
                const QRect pixmapRect = cp.pixmapRect();
                QColor buttonColor = d->buttonColor(option->palette);
                QColor gradientStartColor = buttonColor.lighter(104);
                QColor gradientStopColor = buttonColor.darker(102);
                if (isSectionDragTarget) {
                    gradientStopColor = gradientStartColor.darker(130);
                    gradientStartColor = gradientStartColor.darker(130);
                }
                QLinearGradient gradient(pixmapRect.topLeft(), pixmapRect.bottomLeft());

                if (option->palette.window().gradient()) {
                    gradient.setStops(option->palette.window().gradient()->stops());
                } else {
                    QColor midColor1 = mergedColors(gradientStartColor, gradientStopColor, 60);
                    QColor midColor2 = mergedColors(gradientStartColor, gradientStopColor, 40);
                    gradient.setColorAt(0, gradientStartColor);
                    gradient.setColorAt(0.5, midColor1);
                    gradient.setColorAt(0.501, midColor2);
                    gradient.setColorAt(0.92, gradientStopColor);
                    gradient.setColorAt(1, gradientStopColor.darker(104));
                }
                cp->fillRect(pixmapRect, gradient);
                cp->setPen(QFusionStylePrivate::innerContrastLine);
                cp->setBrush(Qt::NoBrush);
                cp->drawLine(pixmapRect.topLeft(), pixmapRect.topRight());
                cp->setPen(d->outline(option->palette));
                cp->drawLine(pixmapRect.bottomLeft(), pixmapRect.bottomRight());

                if (header->orientation == Qt::Horizontal &&
                        header->position != QStyleOptionHeader::End &&
                        header->position != QStyleOptionHeader::OnlyOneSection) {
                    cp->setPen(QColor(0, 0, 0, 40));
                    cp->drawLine(pixmapRect.topRight(), pixmapRect.bottomRight() + QPoint(0, -1));
                    cp->setPen(QFusionStylePrivate::innerContrastLine);
                    cp->drawLine(pixmapRect.topRight() + QPoint(-1, 0), pixmapRect.bottomRight() + QPoint(-1, -1));
                } else if (header->orientation == Qt::Vertical) {
                    cp->setPen(d->outline(option->palette));
                    cp->drawLine(pixmapRect.topRight(), pixmapRect.bottomRight());
                }
            }
        }
        painter->restore();
        break;
    case CE_ProgressBarGroove:
        painter->save();
    {
        const QRect &rect = option->rect;
        painter->setRenderHint(QPainter::Antialiasing, true);
        painter->translate(0.5, 0.5);

        QColor shadowAlpha = Qt::black;
        shadowAlpha.setAlpha(16);
        painter->setPen(shadowAlpha);
        painter->drawLine(rect.topLeft() - QPoint(0, 1), rect.topRight() - QPoint(0, 1));

        painter->setBrush(option->palette.base());
        painter->setPen(d->outline(option->palette));
        painter->drawRoundedRect(rect.adjusted(0, 0, -1, -1), 2, 2);

        // Inner shadow
        painter->setPen(QFusionStylePrivate::topShadow);
        painter->drawLine(QPoint(rect.left() + 1, rect.top() + 1),
                          QPoint(rect.right() - 1, rect.top() + 1));
    }
        painter->restore();
        break;
    case CE_ProgressBarContents:
        painter->save();
        painter->setRenderHint(QPainter::Antialiasing, true);
        if (const QStyleOptionProgressBar *bar = qstyleoption_cast<const QStyleOptionProgressBar *>(option)) {
            QRect rect = option->rect;
            painter->translate(rect.topLeft());
            rect.translate(-rect.topLeft());
            const auto indeterminate = (bar->minimum == 0 && bar->maximum == 0);
            const auto complete = bar->progress == bar->maximum;
            const auto vertical = !(bar->state & QStyle::State_Horizontal);
            const auto inverted = bar->invertedAppearance;
            const auto reverse = (bar->direction == Qt::RightToLeft) ^ inverted;

            // If the orientation is vertical, we use a transform to rotate
            // the progress bar 90 degrees (counter)clockwise. This way we can use the
            // same rendering code for both orientations.
            if (vertical) {
                rect = QRect(rect.left(), rect.top(), rect.height(), rect.width()); // flip width and height
                QTransform m;
                if (inverted) {
                    m.rotate(90);
                    m.translate(0, -rect.height());
                } else {
                    m.rotate(-90);
                    m.translate(-rect.width(), 0);
                }
                painter->setTransform(m, true);
            } else if (reverse) {
                QTransform m = QTransform::fromScale(-1, 1);
                m.translate(-rect.width(), 0);
                painter->setTransform(m, true);
            }
            painter->translate(0.5, 0.5);

            const auto progress = qMax(bar->progress, bar->minimum); // workaround for bug in QProgressBar
            const auto totalSteps = qMax(Q_INT64_C(1), qint64(bar->maximum) - bar->minimum);
            const auto progressSteps = qint64(progress) - bar->minimum;
            const auto progressBarWidth = progressSteps * rect.width() / totalSteps;
            int width = indeterminate ? rect.width() : progressBarWidth;

            int step = 0;
            QRect progressBar;
            QColor highlight = d->highlight(option->palette);
            QColor highlightedoutline = highlight.darker(140);
            QColor outline = d->outline(option->palette);
            if (qGray(outline.rgb()) > qGray(highlightedoutline.rgb()))
                outline = highlightedoutline;

            if (!indeterminate)
                progressBar.setRect(rect.left(), rect.top(), width - 1, rect.height() - 1);
            else
                progressBar.setRect(rect.left(), rect.top(), rect.width() - 1, rect.height() - 1);

            if (indeterminate || bar->progress > bar->minimum) {

                painter->setPen(outline);

                QColor highlightedGradientStartColor = highlight.lighter(120);
                QColor highlightedGradientStopColor  = highlight;
                QLinearGradient gradient(rect.topLeft(), QPoint(rect.bottomLeft().x(), rect.bottomLeft().y()));
                gradient.setColorAt(0, highlightedGradientStartColor);
                gradient.setColorAt(1, highlightedGradientStopColor);

                painter->setBrush(gradient);

                painter->save();
                // 0.5 - half the width of a cosmetic pen (for vertical line below)
                if (!complete && !indeterminate)
                    painter->setClipRect(QRectF(progressBar).adjusted(-1, -1, 0.5, 1));

                QRect fillRect = progressBar;
                if (!indeterminate && !complete)
                    fillRect.setWidth(std::min(fillRect.width() + 2, rect.width() - 1));  // avoid round borders at the right end
                painter->drawRoundedRect(fillRect, 2, 2);
                painter->restore();

                painter->setBrush(Qt::NoBrush);
                painter->setPen(QColor(255, 255, 255, 50));
                painter->drawRoundedRect(progressBar.adjusted(1, 1, -1, -1), 1, 1);

                if (!indeterminate) {
#if QT_CONFIG(animation)
                    (const_cast<QFusionStylePrivate*>(d))->stopAnimation(option->styleObject);
#endif
                } else {
                    highlightedGradientStartColor.setAlpha(120);
                    painter->setPen(QPen(highlightedGradientStartColor, 9.0));
                    painter->setClipRect(progressBar.adjusted(1, 1, -1, -1));
#if QT_CONFIG(animation)
                    if (QProgressStyleAnimation *animation =
                        qobject_cast<QProgressStyleAnimation*>(d->animation(option->styleObject))) {
                        step = animation->animationStep() % 22;
                    } else {
                        (const_cast<QFusionStylePrivate*>(d))->startAnimation(
                                new QProgressStyleAnimation(d->animationFps, option->styleObject)
                        );
                    }
#endif
                    QVarLengthArray<QLine, 40> lines;
                    for (int x = progressBar.left() - rect.height(); x < rect.right() ; x += 22) {
                        lines.emplace_back(x + step, progressBar.bottom() + 1,
                                           x + rect.height() + step, progressBar.top() - 2);
                    }
                    painter->drawLines(lines.data(), lines.count());
                }
            }
            if (!indeterminate && !complete) {
                QColor innerShadow(Qt::black);
                innerShadow.setAlpha(35);
                painter->setPen(innerShadow);
                painter->drawLine(progressBar.topRight() + QPoint(2, 1), progressBar.bottomRight() + QPoint(2, 0));
                painter->setPen(highlight.darker(140));
                painter->drawLine(progressBar.topRight() + QPoint(1, 1), progressBar.bottomRight() + QPoint(1, 0));
            }
        }
        painter->restore();
        break;
    case CE_ProgressBarLabel:
        if (const QStyleOptionProgressBar *bar = qstyleoption_cast<const QStyleOptionProgressBar *>(option)) {
            const QRect &rect = bar->rect;
            QRect leftRect = rect;
            QRect rightRect = rect;
            QColor textColor = option->palette.text().color();
            QColor alternateTextColor = d->highlightedText(option->palette);

            painter->save();
            const auto vertical = !(bar->state & QStyle::State_Horizontal);
            const auto inverted = bar->invertedAppearance;
            const auto reverse = (bar->direction == Qt::RightToLeft) ^ inverted;
            const auto totalSteps = qMax(Q_INT64_C(1), qint64(bar->maximum) - bar->minimum);
            const auto progressSteps = qint64(bar->progress) - bar->minimum;
            const auto progressIndicatorPos = progressSteps * (vertical ? rect.height() : rect.width()) / totalSteps;

            if (vertical) {
                if (progressIndicatorPos >= 0 && progressIndicatorPos <= rect.height()) {
                    if (inverted) {
                        leftRect.setHeight(progressIndicatorPos);
                        rightRect.setY(progressIndicatorPos);
                    } else {
                        leftRect.setHeight(rect.height() - progressIndicatorPos);
                        rightRect.setY(rect.height() - progressIndicatorPos);
                    }
                }
            } else {
                if (progressIndicatorPos >= 0 && progressIndicatorPos <= rect.width()) {
                    if (reverse) {
                        leftRect.setWidth(rect.width() - progressIndicatorPos);
                        rightRect.setX(rect.width() - progressIndicatorPos);
                    } else {
                        leftRect.setWidth(progressIndicatorPos);
                        rightRect.setX(progressIndicatorPos);
                    }
                }
            }

            const auto firstIsAlternateColor = (vertical && !inverted) || (!vertical && reverse);
            painter->setClipRect(rightRect);
            painter->setPen(firstIsAlternateColor ? alternateTextColor : textColor);
            painter->drawText(rect, bar->text, QTextOption(Qt::AlignAbsolute | Qt::AlignHCenter | Qt::AlignVCenter));
            painter->setPen(firstIsAlternateColor ? textColor : alternateTextColor);
            painter->setClipRect(leftRect);
            painter->drawText(rect, bar->text, QTextOption(Qt::AlignAbsolute | Qt::AlignHCenter | Qt::AlignVCenter));

            painter->restore();
        }
        break;
    case CE_MenuBarItem:
        painter->save();
        if (const QStyleOptionMenuItem *mbi = qstyleoption_cast<const QStyleOptionMenuItem *>(option))
        {
            const QRect &rect = option->rect;
            QStyleOptionMenuItem item = *mbi;
            item.rect = mbi->rect.adjusted(0, 1, 0, -3);
            QColor highlightOutline = option->palette.highlight().color().darker(125);
            painter->fillRect(rect, option->palette.window());

            QCommonStyle::drawControl(element, &item, painter, widget);

            bool act = mbi->state & State_Selected && mbi->state & State_Sunken;
            bool dis = !(mbi->state & State_Enabled);

            if (act) {
                painter->setBrush(option->palette.highlight());
                painter->setPen(highlightOutline);
                painter->drawRect(rect.adjusted(0, 0, -1, -1));

                //                painter->drawRoundedRect(r.adjusted(1, 1, -1, -1), 2, 2);

                //draw text
                QPalette::ColorRole textRole = dis ? QPalette::Text : QPalette::HighlightedText;
                uint alignment = Qt::AlignCenter | Qt::TextShowMnemonic | Qt::TextDontClip | Qt::TextSingleLine;
                if (!proxy()->styleHint(SH_UnderlineShortcut, mbi, widget))
                    alignment |= Qt::TextHideMnemonic;
                proxy()->drawItemText(painter, item.rect, alignment, mbi->palette, mbi->state & State_Enabled, mbi->text, textRole);
            } else {
                QColor shadow = mergedColors(option->palette.window().color().darker(120),
                                             d->outline(option->palette).lighter(140), 60);
                painter->setPen(shadow);
                painter->drawLine(rect.bottomLeft(), rect.bottomRight());
            }
        }
        painter->restore();
        break;
    case CE_MenuItem:
        painter->save();
        // Draws one item in a popup menu.
        if (const QStyleOptionMenuItem *menuItem = qstyleoption_cast<const QStyleOptionMenuItem *>(option)) {
            if (menuItem->menuItemType == QStyleOptionMenuItem::Separator) {
                int w = 0;
                const int margin = int(QStyleHelper::dpiScaled(5, option));
                if (!menuItem->text.isEmpty()) {
                    painter->setFont(menuItem->font);
                    proxy()->drawItemText(painter, menuItem->rect.adjusted(margin, 0, -margin, 0),
                                          Qt::AlignLeft | Qt::AlignVCenter,
                                          menuItem->palette, menuItem->state & State_Enabled, menuItem->text,
                                          QPalette::Text);
                    w = menuItem->fontMetrics.horizontalAdvance(menuItem->text) + margin;
                }
                painter->setPen(QFusionStylePrivate::darkShade.lighter(106));
                const bool reverse = menuItem->direction == Qt::RightToLeft;
                qreal y = menuItem->rect.center().y() + 0.5f;
                painter->drawLine(QPointF(menuItem->rect.left() + margin + (reverse ? 0 : w), y),
                                  QPointF(menuItem->rect.right() - margin - (reverse ? w : 0), y));
                painter->restore();
                break;
            }
            const bool selected = menuItem->state & State_Selected && menuItem->state & State_Enabled;
            if (selected) {
                const QColor highlightOutline = d->highlightedOutline(option->palette);
                const QColor highlight = option->palette.highlight().color();
                const QRect &r = option->rect;
                painter->fillRect(r, highlight);
                painter->setPen(highlightOutline);
                painter->drawRect(QRectF(r).adjusted(0.5, 0.5, -0.5, -0.5));
            }
            const bool checkable = menuItem->checkType != QStyleOptionMenuItem::NotCheckable;
            const bool checked = menuItem->checked;
            const bool sunken = menuItem->state & State_Sunken;
            const bool enabled = menuItem->state & State_Enabled;

            const int checkColHOffset = QFusionStylePrivate::menuItemHMargin + windowsItemFrame - 1;
            // icon checkbox's highlight column width
            int checkcol = qMax<int>(menuItem->rect.height() * 0.79,
                                     qMax<int>(menuItem->maxIconWidth, dpiScaled(21, option)));
            bool ignoreCheckMark = false;
#if QT_CONFIG(combobox)
            if (qobject_cast<const QComboBox*>(widget))
                ignoreCheckMark = true; //ignore the checkmarks provided by the QComboMenuDelegate
#endif
            if (!ignoreCheckMark || menuItem->state & (State_On | State_Off)) {
                // Check, using qreal and QRectF to avoid error accumulation
                const qreal boxMargin = dpiScaled(3.5, option);
                const qreal boxWidth = checkcol - 2 * boxMargin;
                QRect checkRect = QRectF(option->rect.left() + boxMargin + checkColHOffset,
                                         option->rect.center().y() - boxWidth/2 + 1, boxWidth,
                                         boxWidth).toRect();
                checkRect.setWidth(checkRect.height()); // avoid .toRect() round error results in non-perfect square
                checkRect = visualRect(menuItem->direction, menuItem->rect, checkRect);
                if (checkable) {
                    if (menuItem->checkType & QStyleOptionMenuItem::Exclusive) {
                        // Radio button
                        if (menuItem->state & State_On || checked || sunken) {
                            painter->setRenderHint(QPainter::Antialiasing);
                            painter->setPen(Qt::NoPen);

                            QPalette::ColorRole textRole = !enabled
                                                         ? QPalette::Text :
                                                           selected ? QPalette::HighlightedText
                                                                    : QPalette::ButtonText;
                            painter->setBrush(option->palette.brush( option->palette.currentColorGroup(), textRole));
                            const int adjustment = checkRect.height() * 0.3;
                            painter->drawEllipse(checkRect.adjusted(adjustment, adjustment, -adjustment, -adjustment));
                        }
                    } else {
                        // Check box
                        if (menuItem->icon.isNull()) {
                            QStyleOptionButton box;
                            box.QStyleOption::operator=(*option);
                            box.rect = checkRect;
                            if (checked || menuItem->state & State_On)
                                box.state |= State_On;
                            else
                                box.state |= State_Off;
                            proxy()->drawPrimitive(PE_IndicatorCheckBox, &box, painter, widget);
                        }
                    }
                }
            } else { //ignore checkmark
                if (menuItem->icon.isNull())
                    checkcol = 0;
                else
                    checkcol = menuItem->maxIconWidth;
            }

            // Text and icon, ripped from windows style
            const bool dis = !(menuItem->state & State_Enabled);
            const bool act = menuItem->state & State_Selected;
            const QStyleOption *opt = option;
            const QStyleOptionMenuItem *menuitem = menuItem;

            QPainter *p = painter;
            QRect vCheckRect = visualRect(opt->direction, menuitem->rect,
                                          QRect(menuitem->rect.x() + checkColHOffset, menuitem->rect.y(),
                                                checkcol, menuitem->rect.height()));
            if (!menuItem->icon.isNull()) {
                QIcon::Mode mode = dis ? QIcon::Disabled : QIcon::Normal;
                if (act && !dis)
                    mode = QIcon::Active;
                QPixmap pixmap;

                int smallIconSize = proxy()->pixelMetric(PM_SmallIconSize, option, widget);
                QSize iconSize(smallIconSize, smallIconSize);
#if QT_CONFIG(combobox)
                if (const QComboBox *combo = qobject_cast<const QComboBox*>(widget))
                    iconSize = combo->iconSize();
#endif
                if (checked)
                    pixmap = menuItem->icon.pixmap(iconSize, QStyleHelper::getDpr(painter), mode, QIcon::On);
                else
                    pixmap = menuItem->icon.pixmap(iconSize, QStyleHelper::getDpr(painter), mode);

                QRect pmr(QPoint(0, 0), pixmap.deviceIndependentSize().toSize());
                pmr.moveCenter(vCheckRect.center());
                painter->setPen(menuItem->palette.text().color());
                if (!ignoreCheckMark && checkable && checked) {
                    QStyleOption opt = *option;
                    if (act) {
                        QColor activeColor = mergedColors(option->palette.window().color(),
                                                          option->palette.highlight().color());
                        opt.palette.setBrush(QPalette::Button, activeColor);
                    }
                    opt.state |= State_Sunken;
                    opt.rect = vCheckRect;
                    proxy()->drawPrimitive(PE_PanelButtonCommand, &opt, painter, widget);
                }
                painter->drawPixmap(pmr.topLeft(), pixmap);
            }
            if (selected) {
                painter->setPen(menuItem->palette.highlightedText().color());
            } else {
                painter->setPen(menuItem->palette.text().color());
            }
            int x, y, w, h;
            menuitem->rect.getRect(&x, &y, &w, &h);
            QColor discol;
            if (dis) {
                discol = menuitem->palette.text().color();
                p->setPen(discol);
            }
            const int xm = checkColHOffset + checkcol + QFusionStylePrivate::menuItemHMargin;
            const int xpos = menuitem->rect.x() + xm;

            const QRect textRect(xpos, y + windowsItemVMargin,
                                 w - xm - QFusionStylePrivate::menuRightBorder - menuitem->reservedShortcutWidth + 2,
                                 h - 2 * windowsItemVMargin);
            const QRect vTextRect = visualRect(opt->direction, menuitem->rect, textRect);
            QStringView s(menuitem->text);
            if (!s.isEmpty()) {                     // draw text
                p->save();
                const qsizetype tabIndex = s.indexOf(u'\t');
                int text_flags = Qt::AlignVCenter | Qt::TextShowMnemonic | Qt::TextDontClip | Qt::TextSingleLine;
                if (!proxy()->styleHint(SH_UnderlineShortcut, menuitem, widget))
                    text_flags |= Qt::TextHideMnemonic;
                text_flags |= Qt::AlignLeft;
                if (tabIndex >= 0) {
                    QRect vShortcutRect = visualRect(opt->direction, menuitem->rect,
                                                     QRect(textRect.topRight(),
                                                     QPoint(menuitem->rect.right(), textRect.bottom())));
                    const QString textToDraw = s.mid(tabIndex + 1).toString();
                    if (dis && !act && proxy()->styleHint(SH_EtchDisabledText, option, widget)) {
                        p->setPen(menuitem->palette.light().color());
                        p->drawText(vShortcutRect.adjusted(1, 1, 1, 1), text_flags, textToDraw);
                        p->setPen(discol);
                    }
                    p->drawText(vShortcutRect, text_flags, textToDraw);
                    s = s.left(tabIndex);
                }
                QFont font = menuitem->font;
                // font may not have any "hard" flags set. We override
                // the point size so that when it is resolved against the device, this font will win.
                // This is mainly to handle cases where someone sets the font on the window
                // and then the combo inherits it and passes it onward. At that point the resolve mask
                // is very, very weak. This makes it stonger.
                font.setPointSizeF(QFontInfo(menuItem->font).pointSizeF());

                if (menuitem->menuItemType == QStyleOptionMenuItem::DefaultItem)
                    font.setBold(true);

                p->setFont(font);
                const QFontMetricsF fontMetrics(font);
                const QString textToDraw = fontMetrics.elidedText(s.left(tabIndex).toString(),
                                                                  Qt::ElideMiddle, vTextRect.width() + 0.5f,
                                                                  text_flags);
                if (dis && !act && proxy()->styleHint(SH_EtchDisabledText, option, widget)) {
                    p->setPen(menuitem->palette.light().color());
                    p->drawText(vTextRect.adjusted(1, 1, 1, 1), text_flags, textToDraw);
                    p->setPen(discol);
                }
                p->drawText(vTextRect, text_flags, textToDraw);
                p->restore();
            }

            // Arrow
            if (menuItem->menuItemType == QStyleOptionMenuItem::SubMenu) {// draw sub menu arrow
                const int dim = (menuItem->rect.height() - 4) / 2;
                PrimitiveElement arrow;
                arrow = option->direction == Qt::RightToLeft ? PE_IndicatorArrowLeft : PE_IndicatorArrowRight;
                const int xpos = menuItem->rect.left() + menuItem->rect.width() - 3 - dim;
                QRect  vSubMenuRect = visualRect(option->direction, menuItem->rect,
                                                 QRect(xpos, menuItem->rect.top() + menuItem->rect.height() / 2 - dim / 2, dim, dim));
                QStyleOptionMenuItem newMI = *menuItem;
                newMI.rect = vSubMenuRect;
                newMI.state = !enabled ? State_None : State_Enabled;
                if (selected)
                    newMI.palette.setColor(QPalette::WindowText,
                                           newMI.palette.highlightedText().color());
                proxy()->drawPrimitive(arrow, &newMI, painter, widget);
            }
        }
        painter->restore();
        break;
    case CE_MenuHMargin:
    case CE_MenuVMargin:
        break;
    case CE_MenuEmptyArea:
        break;
    case CE_PushButton:
        if (const QStyleOptionButton *btn = qstyleoption_cast<const QStyleOptionButton *>(option)) {
            proxy()->drawControl(CE_PushButtonBevel, btn, painter, widget);
            QStyleOptionButton subopt = *btn;
            subopt.rect = subElementRect(SE_PushButtonContents, btn, widget);
            proxy()->drawControl(CE_PushButtonLabel, &subopt, painter, widget);
        }
        break;
    case CE_MenuBarEmptyArea:
        painter->save();
    {
        const QRect &rect = option->rect;
        painter->fillRect(rect, option->palette.window());
        QColor shadow = mergedColors(option->palette.window().color().darker(120),
                                     d->outline(option->palette).lighter(140), 60);
        painter->setPen(shadow);
        painter->drawLine(rect.bottomLeft(), rect.bottomRight());
    }
        painter->restore();
        break;
#if QT_CONFIG(tabbar)
    case CE_TabBarTabShape:
        painter->save();
        if (const QStyleOptionTab *tab = qstyleoption_cast<const QStyleOptionTab *>(option)) {

            bool rtlHorTabs = (tab->direction == Qt::RightToLeft
                               && (tab->shape == QTabBar::RoundedNorth
                                   || tab->shape == QTabBar::RoundedSouth));
            bool selected = tab->state & State_Selected;
            bool lastTab = ((!rtlHorTabs && tab->position == QStyleOptionTab::End)
                            || (rtlHorTabs
                                && tab->position == QStyleOptionTab::Beginning));
            bool onlyOne = tab->position == QStyleOptionTab::OnlyOneTab;
            int tabOverlap = pixelMetric(PM_TabBarTabOverlap, option, widget);
            QRect rect = option->rect.adjusted(0, 0, (onlyOne || lastTab) ? 0 : tabOverlap, 0);

            QTransform rotMatrix;
            bool flip = false;
            painter->setPen(QFusionStylePrivate::darkShade);

            switch (tab->shape) {
            case QTabBar::RoundedNorth:
                break;
            case QTabBar::RoundedSouth:
                rotMatrix.rotate(180);
                rotMatrix.translate(0, -rect.height() + 1);
                rotMatrix.scale(-1, 1);
                painter->setTransform(rotMatrix, true);
                break;
            case QTabBar::RoundedWest:
                rotMatrix.rotate(180 + 90);
                rotMatrix.scale(-1, 1);
                flip = true;
                painter->setTransform(rotMatrix, true);
                break;
            case QTabBar::RoundedEast:
                rotMatrix.rotate(90);
                rotMatrix.translate(0, - rect.width() + 1);
                flip = true;
                painter->setTransform(rotMatrix, true);
                break;
            default:
                painter->restore();
                QCommonStyle::drawControl(element, tab, painter, widget);
                return;
            }

            if (flip)
                rect = QRect(rect.y(), rect.x(), rect.height(), rect.width());

            painter->setRenderHint(QPainter::Antialiasing, true);
            painter->translate(0.5, 0.5);

            QColor tabFrameColor = tab->features & QStyleOptionTab::HasFrame ?
                        d->tabFrameColor(option->palette) :
                        option->palette.window().color();

            QLinearGradient fillGradient(rect.topLeft(), rect.bottomLeft());
            QLinearGradient outlineGradient(rect.topLeft(), rect.bottomLeft());
            const QColor outline = d->outline(option->palette);
            if (selected) {
                fillGradient.setColorAt(0, tabFrameColor.lighter(104));
                fillGradient.setColorAt(1, tabFrameColor);
                outlineGradient.setColorAt(1, outline);
                painter->setPen(QPen(outlineGradient, 1));
            } else {
                fillGradient.setColorAt(0, tabFrameColor.darker(108));
                fillGradient.setColorAt(0.85, tabFrameColor.darker(108));
                fillGradient.setColorAt(1, tabFrameColor.darker(116));
                painter->setPen(outline.lighter(110));
            }

            QRect drawRect = rect.adjusted(0, selected ? 0 : 2, 0, 3);
            painter->save();
            painter->setClipRect(rect.adjusted(-1, -1, 1, selected ? -2 : -3));
            painter->setBrush(fillGradient);
            painter->drawRoundedRect(drawRect.adjusted(0, 0, -1, -1), 2.0, 2.0);
            painter->setBrush(Qt::NoBrush);
            painter->setPen(QFusionStylePrivate::innerContrastLine);
            painter->drawRoundedRect(drawRect.adjusted(1, 1, -2, -1), 2.0, 2.0);
            painter->restore();

            if (selected) {
                painter->fillRect(rect.left() + 1, rect.bottom() - 1, rect.width() - 2, rect.bottom() - 1, tabFrameColor);
                painter->fillRect(QRect(rect.bottomRight() + QPoint(-2, -1), QSize(1, 1)), QFusionStylePrivate::innerContrastLine);
                painter->fillRect(QRect(rect.bottomLeft() + QPoint(0, -1), QSize(1, 1)), QFusionStylePrivate::innerContrastLine);
                painter->fillRect(QRect(rect.bottomRight() + QPoint(-1, -1), QSize(1, 1)), QFusionStylePrivate::innerContrastLine);
            }
        }
        painter->restore();
        break;
#endif //QT_CONFIG(tabbar)
    default:
        QCommonStyle::drawControl(element,option,painter,widget);
        break;
    }
}

extern QPalette qt_fusionPalette();

/*!
  \reimp
*/
QPalette QFusionStyle::standardPalette () const
{
    return qt_fusionPalette();
}

/*!
  \reimp
*/
void QFusionStyle::drawComplexControl(ComplexControl control, const QStyleOptionComplex *option,
                                      QPainter *painter, const QWidget *widget) const
{

    Q_D (const QFusionStyle);
    const QColor outline = d->outline(option->palette);
    switch (control) {
    case CC_GroupBox:
        painter->save();
        if (const QStyleOptionGroupBox *groupBox = qstyleoption_cast<const QStyleOptionGroupBox *>(option)) {
            // Draw frame
            QRect textRect = proxy()->subControlRect(CC_GroupBox, option, SC_GroupBoxLabel, widget);
            QRect checkBoxRect = proxy()->subControlRect(CC_GroupBox, option, SC_GroupBoxCheckBox, widget);

            if (groupBox->subControls & QStyle::SC_GroupBoxFrame) {
                QStyleOptionFrame frame;
                frame.QStyleOption::operator=(*groupBox);
                frame.features = groupBox->features;
                frame.lineWidth = groupBox->lineWidth;
                frame.midLineWidth = groupBox->midLineWidth;
                frame.rect = proxy()->subControlRect(CC_GroupBox, option, SC_GroupBoxFrame, widget);
                painter->save();
                QRegion region(groupBox->rect);
                if (!groupBox->text.isEmpty()) {
                    bool ltr = groupBox->direction == Qt::LeftToRight;
                    QRect finalRect;
                    if (groupBox->subControls & QStyle::SC_GroupBoxCheckBox) {
                        finalRect = checkBoxRect.united(textRect);
                        finalRect.adjust(ltr ? -4 : -2, 0, ltr ? 2 : 4, 0);
                    } else {
                        finalRect = textRect;
                        finalRect.adjust(-2, 0, 2, 0);
                    }
                    region -= finalRect.adjusted(0, 0, 0, 3 - textRect.height() / 2);
                }
                painter->setClipRegion(region);
                if (isHighContrast()) {
                    painter->setPen(outline);
                    QMargins margins(3, 3, 3, 3);
                    painter->drawRoundedRect(frame.rect.marginsRemoved(margins), 2, 2);
                } else {
                    proxy()->drawPrimitive(PE_FrameGroupBox, &frame, painter, widget);
                }
                painter->restore();
            }

            // Draw title
            if ((groupBox->subControls & QStyle::SC_GroupBoxLabel) && !groupBox->text.isEmpty()) {
                // groupBox->textColor gets the incorrect palette here
                painter->setPen(QPen(option->palette.windowText(), 1));
                int alignment = int(groupBox->textAlignment);
                if (!proxy()->styleHint(QStyle::SH_UnderlineShortcut, option, widget))
                    alignment |= Qt::TextHideMnemonic;

                proxy()->drawItemText(painter, textRect,  Qt::TextShowMnemonic | Qt::AlignLeft | alignment,
                                      groupBox->palette, groupBox->state & State_Enabled, groupBox->text, QPalette::NoRole);

                if (groupBox->state & State_HasFocus) {
                    QStyleOptionFocusRect fropt;
                    fropt.QStyleOption::operator=(*groupBox);
                    fropt.rect = textRect.adjusted(-2, -1, 2, 1);
                    proxy()->drawPrimitive(PE_FrameFocusRect, &fropt, painter, widget);
                }
            }

            // Draw checkbox
            if (groupBox->subControls & SC_GroupBoxCheckBox) {
                QStyleOptionButton box;
                box.QStyleOption::operator=(*groupBox);
                box.rect = checkBoxRect;
                proxy()->drawPrimitive(PE_IndicatorCheckBox, &box, painter, widget);
            }
        }
        painter->restore();
        break;
#if QT_CONFIG(spinbox)
    case CC_SpinBox:
        if (const QStyleOptionSpinBox *spinBox = qstyleoption_cast<const QStyleOptionSpinBox *>(option)) {
            QCachedPainter cp(painter, "spinbox"_L1, option);
            if (cp.needsPainting()) {
                const QRect pixmapRect = cp.pixmapRect();
                const QRect r = pixmapRect.adjusted(0, 1, 0, -1);
                const QColor buttonColor = d->buttonColor(option->palette);
                const QColor &gradientStopColor = buttonColor;
                QColor arrowColor = spinBox->palette.windowText().color();
                arrowColor.setAlpha(160);

                bool isEnabled = (spinBox->state & State_Enabled);
                bool hover = isEnabled && (spinBox->state & State_MouseOver);
                bool sunken = (spinBox->state & State_Sunken);
                bool upIsActive = (spinBox->activeSubControls == SC_SpinBoxUp);
                bool downIsActive = (spinBox->activeSubControls == SC_SpinBoxDown);
                bool hasFocus = (option->state & State_HasFocus);

                QStyleOptionSpinBox spinBoxCopy = *spinBox;
                spinBoxCopy.rect = pixmapRect;
                QRect upRect = proxy()->subControlRect(CC_SpinBox, &spinBoxCopy, SC_SpinBoxUp, widget);
                QRect downRect = proxy()->subControlRect(CC_SpinBox, &spinBoxCopy, SC_SpinBoxDown, widget);

                if (spinBox->frame) {
                    cp->save();
                    cp->setRenderHint(QPainter::Antialiasing, true);
                    cp->translate(0.5, 0.5);

                    // Fill background
                    cp->setPen(Qt::NoPen);
                    cp->setBrush(option->palette.base());
                    cp->drawRoundedRect(r.adjusted(0, 0, -1, -1), 2, 2);

                    // Draw inner shadow
                    cp->setPen(QFusionStylePrivate::topShadow);
                    cp->drawLine(QPoint(r.left() + 2, r.top() + 1), QPoint(r.right() - 2, r.top() + 1));

                    if (!upRect.isNull()) {
                        // Draw button gradient
                        const QRect updownRect = upRect.adjusted(0, -2, 0, downRect.height() + 2);
                        const QLinearGradient gradient = qt_fusion_gradient(updownRect, (isEnabled && option->state & State_MouseOver )
                                                       ? buttonColor : buttonColor.darker(104));

                        cp->setPen(Qt::NoPen);
                        cp->setBrush(gradient);

                        cp->save();
                        cp->setClipRect(updownRect);
                        cp->drawRoundedRect(r.adjusted(0, 0, -1, -1), 2, 2);
                        cp->setPen(QFusionStylePrivate::innerContrastLine);
                        cp->setBrush(Qt::NoBrush);
                        cp->drawRoundedRect(r.adjusted(1, 1, -2, -2), 2, 2);
                        cp->restore();
                    }

                    if ((spinBox->stepEnabled & QAbstractSpinBox::StepUpEnabled) && upIsActive) {
                        if (sunken)
                            cp->fillRect(upRect.adjusted(0, -1, 0, 0), gradientStopColor.darker(110));
                        else if (hover)
                            cp->fillRect(upRect.adjusted(0, -1, 0, 0), QFusionStylePrivate::innerContrastLine);
                    }

                    if ((spinBox->stepEnabled & QAbstractSpinBox::StepDownEnabled) && downIsActive) {
                        if (sunken)
                            cp->fillRect(downRect.adjusted(0, 0, 0, 1), gradientStopColor.darker(110));
                        else if (hover)
                            cp->fillRect(downRect.adjusted(0, 0, 0, 1), QFusionStylePrivate::innerContrastLine);
                    }

                    cp->setPen(hasFocus ? d->highlightedOutline(option->palette) : d->outline(option->palette));
                    cp->setBrush(Qt::NoBrush);
                    cp->drawRoundedRect(r.adjusted(0, 0, -1, -1), 2, 2);
                    if (hasFocus) {
                        QColor softHighlight = option->palette.highlight().color();
                        softHighlight.setAlpha(40);
                        cp->setPen(softHighlight);
                        cp->drawRoundedRect(r.adjusted(1, 1, -2, -2), 1.7, 1.7);
                    }
                    cp->restore();
                }

                if (spinBox->buttonSymbols != QAbstractSpinBox::NoButtons) {
                    // buttonSymbols == NoButtons results in 'null' rects
                    // and a tiny rect painted in the corner.
                    cp->setPen(d->outline(option->palette));
                    if (spinBox->direction == Qt::RightToLeft)
                        cp->drawLine(QLineF(upRect.right(), upRect.top() - 0.5, upRect.right(), downRect.bottom() + 1.5));
                    else
                        cp->drawLine(QLineF(upRect.left(), upRect.top() - 0.5, upRect.left(), downRect.bottom() + 1.5));
                }

                if (upIsActive && sunken) {
                    cp->setPen(gradientStopColor.darker(130));
                    cp->drawLine(downRect.left() + 1, downRect.top(), downRect.right(), downRect.top());
                    cp->drawLine(upRect.left() + 1, upRect.top(), upRect.left() + 1, upRect.bottom());
                    cp->drawLine(upRect.left() + 1, upRect.top() - 1, upRect.right(), upRect.top() - 1);
                }

                if (downIsActive && sunken) {
                    cp->setPen(gradientStopColor.darker(130));
                    cp->drawLine(downRect.left() + 1, downRect.top(), downRect.left() + 1, downRect.bottom() + 1);
                    cp->drawLine(downRect.left() + 1, downRect.top(), downRect.right(), downRect.top());
                    cp->setPen(gradientStopColor.darker(110));
                    cp->drawLine(downRect.left() + 1, downRect.bottom() + 1, downRect.right(), downRect.bottom() + 1);
                }

                QColor disabledColor = mergedColors(arrowColor, option->palette.button().color());
                if (spinBox->buttonSymbols == QAbstractSpinBox::PlusMinus) {
                    int centerX = upRect.center().x();
                    int centerY = upRect.center().y();

                    // plus/minus
                    cp->setPen((spinBox->stepEnabled & QAbstractSpinBox::StepUpEnabled) ? arrowColor : disabledColor);
                    cp->drawLine(centerX - 1, centerY, centerX + 3, centerY);
                    cp->drawLine(centerX + 1, centerY - 2, centerX + 1, centerY + 2);

                    centerX = downRect.center().x();
                    centerY = downRect.center().y();
                    cp->setPen((spinBox->stepEnabled & QAbstractSpinBox::StepDownEnabled) ? arrowColor : disabledColor);
                    cp->drawLine(centerX - 1, centerY, centerX + 3, centerY);

                } else if (spinBox->buttonSymbols == QAbstractSpinBox::UpDownArrows){
                    // arrows
                    qt_fusion_draw_arrow(Qt::UpArrow, cp.painter(), option, upRect.adjusted(0, 0, 0, 1),
                                         (spinBox->stepEnabled & QAbstractSpinBox::StepUpEnabled) ? arrowColor : disabledColor);
                    qt_fusion_draw_arrow(Qt::DownArrow, cp.painter(), option, downRect,
                                         (spinBox->stepEnabled & QAbstractSpinBox::StepDownEnabled) ? arrowColor : disabledColor);
                }
            }
        }
        break;
#endif // QT_CONFIG(spinbox)
    case CC_TitleBar:
        painter->save();
        if (const QStyleOptionTitleBar *titleBar = qstyleoption_cast<const QStyleOptionTitleBar *>(option)) {
            constexpr auto buttonMargins = QMargins(5, 5, 5, 5);
            const bool active = (titleBar->titleBarState & State_Active);
            const QRect &fullRect = titleBar->rect;
            const QPalette &palette = option->palette;
            const QColor highlight = palette.highlight().color();
            const QColor outline = d->outline(palette);
            const QColor buttonPaintingsColor(active ? 0xffffff : 0xff000000);

            {
                // Fill title bar gradient
                const QColor titlebarColor = active ? highlight : palette.window().color();
                QLinearGradient gradient(option->rect.center().x(), option->rect.top(),
                                         option->rect.center().x(), option->rect.bottom());

                gradient.setColorAt(0, titlebarColor.lighter(114));
                gradient.setColorAt(0.5, titlebarColor.lighter(102));
                gradient.setColorAt(0.51, titlebarColor.darker(104));
                gradient.setColorAt(1, titlebarColor);
                painter->fillRect(option->rect.adjusted(1, 1, -1, 0), gradient);

                // Frame and rounded corners
                painter->setPen(active ? highlight.darker(180) : outline.darker(110));

                // top outline
                const QLine lines[2] = {{fullRect.left() + 5, fullRect.top(), fullRect.right() - 5, fullRect.top()},
                                        {fullRect.left(), fullRect.top() + 4, fullRect.left(), fullRect.bottom()}};
                const QPoint points[5] = {
                    QPoint(fullRect.left() + 4, fullRect.top() + 1),
                    QPoint(fullRect.left() + 3, fullRect.top() + 1),
                    QPoint(fullRect.left() + 2, fullRect.top() + 2),
                    QPoint(fullRect.left() + 1, fullRect.top() + 3),
                    QPoint(fullRect.left() + 1, fullRect.top() + 4)
                };
                painter->drawLines(lines, 2);
                painter->drawPoints(points, 5);

                painter->drawLine(fullRect.right(), fullRect.top() + 4, fullRect.right(), fullRect.bottom());
                const QPoint points2[5] = {
                    QPoint(fullRect.right() - 3, fullRect.top() + 1),
                    QPoint(fullRect.right() - 4, fullRect.top() + 1),
                    QPoint(fullRect.right() - 2, fullRect.top() + 2),
                    QPoint(fullRect.right() - 1, fullRect.top() + 3),
                    QPoint(fullRect.right() - 1, fullRect.top() + 4)
                };
                painter->drawPoints(points2, 5);

                // draw bottomline
                painter->drawLine(fullRect.right(), fullRect.bottom(), fullRect.left(), fullRect.bottom());

                // top highlight
                painter->setPen(active ? highlight.lighter(120): palette.window().color().lighter(120));
                painter->drawLine(fullRect.left() + 6, fullRect.top() + 1, fullRect.right() - 6, fullRect.top() + 1);
            }
            // draw title
            const QRect textRect = proxy()->subControlRect(CC_TitleBar, titleBar, SC_TitleBarLabel, widget);
            painter->setPen(active ? palette.text().color().lighter(120) : palette.text().color());
            // Note workspace also does elliding but it does not use the correct font
            const QString title =
                painter->fontMetrics().elidedText(titleBar->text, Qt::ElideRight, textRect.width() - 14);
            painter->drawText(textRect.adjusted(1, 1, 1, 1), title, QTextOption(Qt::AlignHCenter | Qt::AlignVCenter));
            painter->setPen(Qt::white);
            if (active)
                painter->drawText(textRect, title, QTextOption(Qt::AlignHCenter | Qt::AlignVCenter));

            const auto isHover = [option](SubControl sc)
                { return (option->activeSubControls & sc) && (option->state & State_MouseOver); };
            const auto isSunken = [option](SubControl sc)
                { return (option->activeSubControls & sc) && (option->state & State_Sunken); };
            // min button
            if ((titleBar->subControls & SC_TitleBarMinButton) && (titleBar->titleBarFlags & Qt::WindowMinimizeButtonHint) &&
                    !(titleBar->titleBarState& Qt::WindowMinimized)) {
                const auto sc = SC_TitleBarMinButton;
                const auto buttonRect = proxy()->subControlRect(CC_TitleBar, titleBar, sc, widget);
                if (buttonRect.isValid()) {
                    qt_fusion_draw_mdibutton(painter, titleBar, buttonRect, isHover(sc), isSunken(sc));

                    const QRect rect = buttonRect.marginsRemoved(buttonMargins);
                    const QLine lines[2] = {{rect.left(), rect.bottom(), rect.right() - 1, rect.bottom()},
                                            {rect.left(), rect.bottom() - 1, rect.right() - 1, rect.bottom() - 1}};
                    painter->setPen(buttonPaintingsColor);
                    painter->drawLines(lines, 2);
                }
            }
            // max button
            if ((titleBar->subControls & SC_TitleBarMaxButton) && (titleBar->titleBarFlags & Qt::WindowMaximizeButtonHint) &&
                    !(titleBar->titleBarState & Qt::WindowMaximized)) {
                const auto sc = SC_TitleBarMaxButton;
                const auto buttonRect = proxy()->subControlRect(CC_TitleBar, titleBar, sc, widget);
                if (buttonRect.isValid()) {
                    qt_fusion_draw_mdibutton(painter, titleBar, buttonRect, isHover(sc), isSunken(sc));

                    const QRect rect = buttonRect.marginsRemoved(buttonMargins);
                    const QLine lines[5] = {{rect.left(), rect.top(), rect.right(), rect.top()},
                                            {rect.left(), rect.top() + 1, rect.right(), rect.top() + 1},
                                            {rect.left(), rect.bottom(), rect.right(), rect.bottom()},
                                            {rect.left(), rect.top(), rect.left(), rect.bottom()},
                                            {rect.right(), rect.top(), rect.right(), rect.bottom()}};
                    painter->setPen(buttonPaintingsColor);
                    painter->drawLines(lines, 5);
                }
            }

            // close button
            if ((titleBar->subControls & SC_TitleBarCloseButton) && (titleBar->titleBarFlags & Qt::WindowSystemMenuHint)) {
                const auto sc = SC_TitleBarCloseButton;
                const auto buttonRect = proxy()->subControlRect(CC_TitleBar, titleBar, sc, widget);
                if (buttonRect.isValid()) {
                    qt_fusion_draw_mdibutton(painter, titleBar, buttonRect, isHover(sc), isSunken(sc));
                    QRect rect = buttonRect.marginsRemoved(buttonMargins);
                    rect.setWidth((rect.width() / 2) * 2 + 1);
                    rect.setHeight((rect.height() / 2) * 2 + 1);
                    const QLine lines[2] = { { rect.topLeft(), rect.bottomRight() },
                                             { rect.topRight(), rect.bottomLeft() }, };
                    const auto pen = QPen(buttonPaintingsColor, 2);
                    painter->setPen(pen);
                    painter->drawLines(lines, 2);
                }
            }

            // normalize button
            if ((titleBar->subControls & SC_TitleBarNormalButton) &&
                    (((titleBar->titleBarFlags & Qt::WindowMinimizeButtonHint) &&
                      (titleBar->titleBarState & Qt::WindowMinimized)) ||
                     ((titleBar->titleBarFlags & Qt::WindowMaximizeButtonHint) &&
                      (titleBar->titleBarState & Qt::WindowMaximized)))) {
                const auto sc = SC_TitleBarNormalButton;
                const auto buttonRect = proxy()->subControlRect(CC_TitleBar, titleBar, sc, widget);
                if (buttonRect.isValid()) {
                    qt_fusion_draw_mdibutton(painter, titleBar, buttonRect, isHover(sc), isSunken(sc));

                    QRect normalButtonIconRect = buttonRect.marginsRemoved(buttonMargins);
                    painter->setPen(buttonPaintingsColor);
                    {
                        const QRect rect = normalButtonIconRect.adjusted(0, 3, -3, 0);
                        const QLine lines[5] = {{rect.left(), rect.top(), rect.right(), rect.top()},
                                                {rect.left(), rect.top() + 1, rect.right(), rect.top() + 1},
                                                {rect.left(), rect.bottom(), rect.right(), rect.bottom()},
                                                {rect.left(), rect.top(), rect.left(), rect.bottom()},
                                                {rect.right(), rect.top(), rect.right(), rect.bottom()}};
                        painter->drawLines(lines, 5);
                    }
                    {
                        const QRect rect = normalButtonIconRect.adjusted(3, 0, 0, -3);
                        const QLine lines[5] = {{rect.left(), rect.top(), rect.right(), rect.top()},
                                                {rect.left(), rect.top() + 1, rect.right(), rect.top() + 1},
                                                {rect.left(), rect.bottom(), rect.right(), rect.bottom()},
                                                {rect.left(), rect.top(), rect.left(), rect.bottom()},
                                                {rect.right(), rect.top(), rect.right(), rect.bottom()}};
                        painter->drawLines(lines, 5);
                    }
                }
            }

            // context help button
            if (titleBar->subControls & SC_TitleBarContextHelpButton
                    && (titleBar->titleBarFlags & Qt::WindowContextHelpButtonHint)) {
                const auto sc = SC_TitleBarContextHelpButton;
                const auto buttonRect = proxy()->subControlRect(CC_TitleBar, titleBar, sc, widget);
                if (buttonRect.isValid()) {
                    qt_fusion_draw_mdibutton(painter, titleBar, buttonRect, isHover(sc), isSunken(sc));
#if QT_CONFIG(imageformat_xpm)
                    QImage image(qt_titlebar_context_help);
                    QColor alpha = buttonPaintingsColor;
                    alpha.setAlpha(128);
                    image.setColor(1, buttonPaintingsColor.rgba());
                    image.setColor(2, alpha.rgba());
                    painter->setRenderHint(QPainter::SmoothPixmapTransform);
                    painter->drawImage(buttonRect.adjusted(4, 4, -4, -4), image);
#endif
                }
            }

            // shade button
            if (titleBar->subControls & SC_TitleBarShadeButton && (titleBar->titleBarFlags & Qt::WindowShadeButtonHint)) {
                const auto sc = SC_TitleBarShadeButton;
                const auto buttonRect = proxy()->subControlRect(CC_TitleBar, titleBar, sc, widget);
                if (buttonRect.isValid()) {
                    qt_fusion_draw_mdibutton(painter, titleBar, buttonRect, isHover(sc), isSunken(sc));
                    qt_fusion_draw_arrow(Qt::UpArrow, painter, option, buttonRect.adjusted(5, 7, -5, -7), buttonPaintingsColor);
                }
            }

            // unshade button
            if (titleBar->subControls & SC_TitleBarUnshadeButton && (titleBar->titleBarFlags & Qt::WindowShadeButtonHint)) {
                const auto sc = SC_TitleBarUnshadeButton;
                const auto buttonRect = proxy()->subControlRect(CC_TitleBar, titleBar, sc, widget);
                if (buttonRect.isValid()) {
                    qt_fusion_draw_mdibutton(painter, titleBar, buttonRect, isHover(sc), isSunken(sc));
                    qt_fusion_draw_arrow(Qt::DownArrow, painter, option, buttonRect.adjusted(5, 7, -5, -7), buttonPaintingsColor);
                }
            }

            if ((titleBar->subControls & SC_TitleBarSysMenu) && (titleBar->titleBarFlags & Qt::WindowSystemMenuHint)) {
                QRect iconRect = proxy()->subControlRect(CC_TitleBar, titleBar, SC_TitleBarSysMenu, widget);
                if (iconRect.isValid()) {
                    if (!titleBar->icon.isNull()) {
                        titleBar->icon.paint(painter, iconRect);
                    } else {
                        QStyleOption tool = *titleBar;
                        QPixmap pm = proxy()->standardIcon(SP_TitleBarMenuButton, &tool, widget)
                                        .pixmap(QSize(16, 16), QStyleHelper::getDpr(painter));
                        tool.rect = iconRect;
                        painter->save();
                        proxy()->drawItemPixmap(painter, iconRect, Qt::AlignCenter, pm);
                        painter->restore();
                    }
                }
            }
        }
        painter->restore();
        break;
#if QT_CONFIG(slider)
    case CC_ScrollBar:
        painter->save();
        if (const QStyleOptionSlider *scrollBar = qstyleoption_cast<const QStyleOptionSlider *>(option)) {
            bool wasActive = false;
            qreal expandScale = 1.0;
            qreal expandOffset = -1.0;
            QObject *styleObject = option->styleObject;
            if (styleObject && proxy()->styleHint(SH_ScrollBar_Transient, option, widget)) {
#if QT_CONFIG(animation)
                qreal opacity = 0.0;
                bool shouldExpand = false;
                const qreal maxExpandScale = 13.0 / 9.0;
#endif

                int oldPos = styleObject->property("_q_stylepos").toInt();
                int oldMin = styleObject->property("_q_stylemin").toInt();
                int oldMax = styleObject->property("_q_stylemax").toInt();
                QRect oldRect = styleObject->property("_q_stylerect").toRect();
                QStyle::State oldState = static_cast<QStyle::State>(qvariant_cast<QStyle::State::Int>(styleObject->property("_q_stylestate")));
                uint oldActiveControls = styleObject->property("_q_stylecontrols").toUInt();

                // a scrollbar is transient when the scrollbar itself and
                // its sibling are both inactive (ie. not pressed/hovered/moved)
                bool transient = !option->activeSubControls && !(option->state & State_On);

                if (!transient ||
                        oldPos != scrollBar->sliderPosition ||
                        oldMin != scrollBar->minimum ||
                        oldMax != scrollBar->maximum ||
                        oldRect != scrollBar->rect ||
                        oldState != scrollBar->state ||
                        oldActiveControls != scrollBar->activeSubControls) {

                    styleObject->setProperty("_q_stylepos", scrollBar->sliderPosition);
                    styleObject->setProperty("_q_stylemin", scrollBar->minimum);
                    styleObject->setProperty("_q_stylemax", scrollBar->maximum);
                    styleObject->setProperty("_q_stylerect", scrollBar->rect);
                    styleObject->setProperty("_q_stylestate", static_cast<QStyle::State::Int>(scrollBar->state));
                    styleObject->setProperty("_q_stylecontrols", static_cast<uint>(scrollBar->activeSubControls));

#if QT_CONFIG(animation)
                    // if the scrollbar is transient or its attributes, geometry or
                    // state has changed, the opacity is reset back to 100% opaque
                    opacity = 1.0;

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
#endif // animation
                }

#if QT_CONFIG(animation)
                QScrollbarStyleAnimation *anim = qobject_cast<QScrollbarStyleAnimation *>(d->animation(styleObject));
                if (anim && anim->mode() == QScrollbarStyleAnimation::Deactivating) {
                    // once a scrollbar was active (hovered/pressed), it retains
                    // the active look even if it's no longer active while fading out
                    if (oldActiveControls)
                        anim->setActive(true);

                    wasActive = anim->wasActive();
                    opacity = anim->currentValue();
                }

                shouldExpand = (option->activeSubControls || wasActive);
                if (shouldExpand) {
                    if (!anim && !oldActiveControls) {
                        // Start expand animation only once and when entering
                        anim = new QScrollbarStyleAnimation(QScrollbarStyleAnimation::Activating, styleObject);
                        d->startAnimation(anim);
                    }
                    if (anim && anim->mode() == QScrollbarStyleAnimation::Activating) {
                        expandScale = 1.0 + (maxExpandScale - 1.0) * anim->currentValue();
                        expandOffset = 5.5 * anim->currentValue() - 1;
                    } else {
                        // Keep expanded state after the animation ends, and when fading out
                        expandScale = maxExpandScale;
                        expandOffset = 4.5;
                    }
                }
                painter->setOpacity(opacity);
#endif // animation
            }

            bool transient = proxy()->styleHint(SH_ScrollBar_Transient, option, widget);
            bool horizontal = scrollBar->orientation == Qt::Horizontal;
            bool sunken = scrollBar->state & State_Sunken;

            QRect scrollBarSubLine = proxy()->subControlRect(control, scrollBar, SC_ScrollBarSubLine, widget);
            QRect scrollBarAddLine = proxy()->subControlRect(control, scrollBar, SC_ScrollBarAddLine, widget);
            QRect scrollBarSlider = proxy()->subControlRect(control, scrollBar, SC_ScrollBarSlider, widget);
            QRect scrollBarGroove = proxy()->subControlRect(control, scrollBar, SC_ScrollBarGroove, widget);

            QRect rect = option->rect;
            QColor alphaOutline = d->outline(option->palette);
            alphaOutline.setAlpha(180);

            QColor arrowColor = option->palette.windowText().color();
            arrowColor.setAlpha(160);

            const QColor bgColor = QStyleHelper::backgroundColor(option->palette, widget);
            const bool isDarkBg = bgColor.red() < 128 && bgColor.green() < 128 && bgColor.blue() < 128;

            if (transient) {
                if (horizontal) {
                    rect.setY(rect.y() + 4.5 - expandOffset);
                    scrollBarSlider.setY(scrollBarSlider.y() + 4.5 - expandOffset);
                    scrollBarGroove.setY(scrollBarGroove.y() + 4.5 - expandOffset);

                    rect.setHeight(rect.height() * expandScale);
                    scrollBarGroove.setHeight(scrollBarGroove.height() * expandScale);
                } else {
                    rect.setX(rect.x() + 4.5 - expandOffset);
                    scrollBarSlider.setX(scrollBarSlider.x() + 4.5 - expandOffset);
                    scrollBarGroove.setX(scrollBarGroove.x() + 4.5 - expandOffset);

                    rect.setWidth(rect.width() * expandScale);
                    scrollBarGroove.setWidth(scrollBarGroove.width() * expandScale);
                }
            }

            // Paint groove
            if ((!transient || scrollBar->activeSubControls || wasActive) && scrollBar->subControls & SC_ScrollBarGroove) {
                QLinearGradient gradient(rect.center().x(), rect.top(),
                                         rect.center().x(), rect.bottom());
                if (!horizontal)
                    gradient = QLinearGradient(rect.left(), rect.center().y(),
                                               rect.right(), rect.center().y());
                if (!transient || !isDarkBg) {
                    QColor buttonColor = d->buttonColor(option->palette);
                    gradient.setColorAt(0, buttonColor.darker(107));
                    gradient.setColorAt(0.1, buttonColor.darker(105));
                    gradient.setColorAt(0.9, buttonColor.darker(105));
                    gradient.setColorAt(1, buttonColor.darker(107));
                } else {
                    gradient.setColorAt(0, bgColor.lighter(157));
                    gradient.setColorAt(0.1, bgColor.lighter(155));
                    gradient.setColorAt(0.9, bgColor.lighter(155));
                    gradient.setColorAt(1, bgColor.lighter(157));
                }

                painter->save();
                if (transient)
                    painter->setOpacity(0.8);
                painter->fillRect(rect, gradient);
                painter->setPen(Qt::NoPen);
                if (transient)
                    painter->setOpacity(0.4);
                painter->setPen(alphaOutline);
                if (horizontal)
                    painter->drawLine(rect.topLeft(), rect.topRight());
                else
                    painter->drawLine(rect.topLeft(), rect.bottomLeft());

                QColor subtleEdge = alphaOutline;
                subtleEdge.setAlpha(40);
                painter->setPen(subtleEdge);
                painter->setBrush(Qt::NoBrush);
                painter->drawRect(scrollBarGroove.adjusted(horizontal ? 0 : 1, horizontal ? 1 : 0, -1, -1));
                painter->restore();
            }

            QRect pixmapRect = scrollBarSlider;
            QLinearGradient gradient(pixmapRect.center().x(), pixmapRect.top(),
                                     pixmapRect.center().x(), pixmapRect.bottom());
            if (!horizontal)
                gradient = QLinearGradient(pixmapRect.left(), pixmapRect.center().y(),
                                           pixmapRect.right(), pixmapRect.center().y());

            QLinearGradient highlightedGradient = gradient;

            const QColor buttonColor = d->buttonColor(option->palette);
            const QColor gradientStartColor = buttonColor.lighter(118);
            const QColor &gradientStopColor = buttonColor;
            const QColor midColor2 = mergedColors(gradientStartColor, gradientStopColor, 40);
            gradient.setColorAt(0, buttonColor.lighter(108));
            gradient.setColorAt(1, buttonColor);

            highlightedGradient.setColorAt(0, gradientStartColor.darker(102));
            highlightedGradient.setColorAt(1, gradientStopColor.lighter(102));

            // Paint slider
            if (scrollBar->subControls & SC_ScrollBarSlider) {
                if (transient) {
                    QRect rect = scrollBarSlider.adjusted(horizontal ? 1 : 2, horizontal ? 2 : 1, -1, -1);
                    painter->setPen(Qt::NoPen);
                    painter->setBrush(isDarkBg ? QFusionStylePrivate::lightShade : QFusionStylePrivate::darkShade);
                    int r = qMin(rect.width(), rect.height()) / 2;

                    painter->save();
                    painter->setRenderHint(QPainter::Antialiasing, true);
                    painter->drawRoundedRect(rect, r, r);
                    painter->restore();
                } else {
                    QRect pixmapRect = scrollBarSlider;
                    painter->setPen(alphaOutline);
                    if (option->state & State_Sunken && scrollBar->activeSubControls & SC_ScrollBarSlider)
                        painter->setBrush(midColor2);
                    else if (option->state & State_MouseOver && scrollBar->activeSubControls & SC_ScrollBarSlider)
                        painter->setBrush(highlightedGradient);
                    else if (!isDarkBg)
                        painter->setBrush(gradient);
                    else
                        painter->setBrush(midColor2);

                    painter->drawRect(pixmapRect.adjusted(horizontal ? -1 : 0, horizontal ? 0 : -1, horizontal ? 0 : -1, horizontal ? -1 : 0));

                    painter->setPen(QFusionStylePrivate::innerContrastLine);
                    painter->drawRect(scrollBarSlider.adjusted(horizontal ? 0 : 1, horizontal ? 1 : 0, -1, -1));

                    // Outer shadow
                    //                  painter->setPen(subtleEdge);
                    //                  if (horizontal) {
                    ////                    painter->drawLine(scrollBarSlider.topLeft() + QPoint(-2, 0), scrollBarSlider.bottomLeft() + QPoint(2, 0));
                    ////                    painter->drawLine(scrollBarSlider.topRight() + QPoint(-2, 0), scrollBarSlider.bottomRight() + QPoint(2, 0));
                    //                  } else {
                    ////                    painter->drawLine(pixmapRect.topLeft() + QPoint(0, -2), pixmapRect.bottomLeft() + QPoint(0, -2));
                    ////                    painter->drawLine(pixmapRect.topRight() + QPoint(0, 2), pixmapRect.bottomRight() + QPoint(0, 2));
                    //                  }
                }
            }

            // The SubLine (up/left) buttons
            if (!transient && scrollBar->subControls & SC_ScrollBarSubLine) {
                if ((scrollBar->activeSubControls & SC_ScrollBarSubLine) && sunken)
                    painter->setBrush(gradientStopColor);
                else if ((scrollBar->activeSubControls & SC_ScrollBarSubLine))
                    painter->setBrush(highlightedGradient);
                else
                    painter->setBrush(gradient);

                const QRect upRect = scrollBarSubLine.adjusted(0, 0, -1, -1);
                painter->setPen(Qt::NoPen);
                painter->drawRect(upRect.adjusted(horizontal ? 0 : 1, horizontal ? 1 : 0, 0, 0));
                painter->setPen(alphaOutline);
                painter->drawRect(upRect);

                painter->setBrush(Qt::NoBrush);
                painter->setPen(QFusionStylePrivate::innerContrastLine);
                painter->drawRect(upRect.adjusted(1, 1, -1, -1));

                // Arrows
                Qt::ArrowType arrowType = Qt::UpArrow;
                if (option->state & State_Horizontal)
                    arrowType = option->direction == Qt::LeftToRight ? Qt::LeftArrow : Qt::RightArrow;
                qt_fusion_draw_arrow(arrowType, painter, option, upRect.adjusted(1, 1, 0, 0), arrowColor);
            }

            // The AddLine (down/right) button
            if (!transient && scrollBar->subControls & SC_ScrollBarAddLine) {
                if ((scrollBar->activeSubControls & SC_ScrollBarAddLine) && sunken)
                    painter->setBrush(gradientStopColor);
                else if ((scrollBar->activeSubControls & SC_ScrollBarAddLine))
                    painter->setBrush(midColor2);
                else
                    painter->setBrush(gradient);

                const QRect downRect = scrollBarAddLine.adjusted(0, 0, -1, -1);
                painter->setPen(Qt::NoPen);
                painter->drawRect(downRect.adjusted(horizontal ? 0 : 1, horizontal ? 1 : 0, 0, 0));
                painter->setPen(alphaOutline);
                painter->drawRect(downRect);

                painter->setBrush(Qt::NoBrush);
                painter->setPen(QFusionStylePrivate::innerContrastLine);
                painter->drawRect(downRect.adjusted(1, 1, -1, -1));

                Qt::ArrowType arrowType = Qt::DownArrow;
                if (option->state & State_Horizontal)
                    arrowType = option->direction == Qt::LeftToRight ? Qt::RightArrow : Qt::LeftArrow;
                qt_fusion_draw_arrow(arrowType, painter, option, downRect.adjusted(1, 1, 0, 0), arrowColor);
            }

        }
        painter->restore();
        break;
#endif // QT_CONFIG(slider)
    case CC_ComboBox:
        painter->save();
        if (const QStyleOptionComboBox *comboBox = qstyleoption_cast<const QStyleOptionComboBox *>(option)) {
            bool hasFocus = option->state & State_HasFocus && option->state & State_KeyboardFocusChange;
            bool sunken = comboBox->state & State_On; // play dead, if combobox has no items
            bool isEnabled = (comboBox->state & State_Enabled);
            const QString pixmapName = "combobox"_L1
                                       % QLatin1StringView(sunken ? "-sunken" : "")
                                       % QLatin1StringView(comboBox->editable ? "-editable" : "")
                                       % QLatin1StringView(isEnabled ? "-enabled" : "")
                                       % QLatin1StringView(!comboBox->frame ? "-frameless" : "");
            QCachedPainter cp(painter, pixmapName, option);
            if (cp.needsPainting()) {
                const QRect pixmapRect = cp.pixmapRect();
                QStyleOptionComboBox comboBoxCopy = *comboBox;
                comboBoxCopy.rect = pixmapRect;

                QRect rect = pixmapRect;
                QRect downArrowRect = proxy()->subControlRect(CC_ComboBox, &comboBoxCopy,
                                                              SC_ComboBoxArrow, widget);
                // Draw a line edit
                if (comboBox->editable) {
                    QStyleOptionFrame  buttonOption;
                    buttonOption.QStyleOption::operator=(*comboBox);
                    buttonOption.rect = rect;
                    buttonOption.state = (comboBox->state & (State_Enabled | State_MouseOver | State_HasFocus))
                            | State_KeyboardFocusChange; // Always show hig

                    if (sunken) {
                        buttonOption.state |= State_Sunken;
                        buttonOption.state &= ~State_MouseOver;
                    }

                    if (comboBox->frame) {
                        cp->save();
                        cp->setRenderHint(QPainter::Antialiasing, true);
                        cp->translate(0.5, 0.5);
                        cp->setPen(Qt::NoPen);
                        cp->setBrush(buttonOption.palette.base());
                        cp->drawRoundedRect(rect.adjusted(0, 0, -1, -1), 2, 2);
                        cp->restore();
                        proxy()->drawPrimitive(PE_FrameLineEdit, &buttonOption, cp.painter(), widget);
                    }

                    // Draw button clipped
                    cp->save();
                    cp->setClipRect(downArrowRect.adjusted(0, 0, 1, 0));
                    buttonOption.rect.setLeft(comboBox->direction == Qt::LeftToRight ?
                                                  downArrowRect.left() - 6: downArrowRect.right() + 6);
                    proxy()->drawPrimitive(PE_PanelButtonCommand, &buttonOption, cp.painter(), widget);
                    cp->restore();
                    cp->setPen(QPen(hasFocus ? option->palette.highlight() : d->outline(option->palette).lighter(110), 1));

                    if (!sunken) {
                        int borderSize = 1;
                        if (comboBox->direction == Qt::RightToLeft) {
                            cp->drawLine(QPoint(downArrowRect.right() - 1, downArrowRect.top() + borderSize ),
                                         QPoint(downArrowRect.right() - 1, downArrowRect.bottom() - borderSize));
                        } else {
                            cp->drawLine(QPoint(downArrowRect.left() , downArrowRect.top() + borderSize),
                                         QPoint(downArrowRect.left() , downArrowRect.bottom() - borderSize));
                        }
                    } else {
                        if (comboBox->direction == Qt::RightToLeft) {
                            cp->drawLine(QPoint(downArrowRect.right(), downArrowRect.top() + 2),
                                         QPoint(downArrowRect.right(), downArrowRect.bottom() - 2));

                        } else {
                            cp->drawLine(QPoint(downArrowRect.left(), downArrowRect.top() + 2),
                                         QPoint(downArrowRect.left(), downArrowRect.bottom() - 2));
                        }
                    }
                } else {
                    QStyleOptionButton buttonOption;
                    buttonOption.QStyleOption::operator=(*comboBox);
                    buttonOption.rect = rect;
                    buttonOption.state = comboBox->state & (State_Enabled | State_MouseOver | State_HasFocus | State_KeyboardFocusChange);
                    if (sunken) {
                        buttonOption.state |= State_Sunken;
                        buttonOption.state &= ~State_MouseOver;
                    }
                    proxy()->drawPrimitive(PE_PanelButtonCommand, &buttonOption, cp.painter(), widget);
                }
                if (comboBox->subControls & SC_ComboBoxArrow) {
                    // Draw the up/down arrow
                    QColor arrowColor = option->palette.buttonText().color();
                    arrowColor.setAlpha(160);
                    qt_fusion_draw_arrow(Qt::DownArrow, cp.painter(), option, downArrowRect, arrowColor);
                }
            }
        }
        painter->restore();
        break;
#if QT_CONFIG(slider)
    case CC_Slider:
        if (const QStyleOptionSlider *slider = qstyleoption_cast<const QStyleOptionSlider *>(option)) {
            QPainterStateGuard psg(painter);
            QRect groove = proxy()->subControlRect(CC_Slider, option, SC_SliderGroove, widget);
            QRect handle = proxy()->subControlRect(CC_Slider, option, SC_SliderHandle, widget);

            bool horizontal = slider->orientation == Qt::Horizontal;
            bool ticksAbove = slider->tickPosition & QSlider::TicksAbove;
            bool ticksBelow = slider->tickPosition & QSlider::TicksBelow;
            const QColor activeHighlight = d->highlight(option->palette);
            const QColor shadowAlpha(0, 0, 0, 10); // Qt::black with 4% opacity
            const QColor outline = (option->state & State_HasFocus
                                    && option->state & State_KeyboardFocusChange)
                                 ? d->highlightedOutline(option->palette)
                                 : d->outline(option->palette);

            painter->setRenderHint(QPainter::Antialiasing, true);
            if ((option->subControls & SC_SliderGroove) && groove.isValid()) {
                // draw background groove
                QCachedPainter cp(painter, "slider_groove"_L1, option, groove.size(), groove);
                if (cp.needsPainting()) {
                    const QRect pixmapRect = cp.pixmapRect();
                    const QColor buttonColor = d->buttonColor(option->palette);
                    const auto grooveColor =
                        QColor::fromHsv(buttonColor.hue(),
                                        qMin(255, (int)(buttonColor.saturation())),
                                        qMin(255, (int)(buttonColor.value()*0.9)));
                    cp->translate(0.5, 0.5);
                    QLinearGradient gradient;
                    if (horizontal) {
                        gradient.setStart(pixmapRect.center().x(), pixmapRect.top());
                        gradient.setFinalStop(pixmapRect.center().x(), pixmapRect.bottom());
                    }
                    else {
                        gradient.setStart(pixmapRect.left(), pixmapRect.center().y());
                        gradient.setFinalStop(pixmapRect.right(), pixmapRect.center().y());
                    }
                    cp->setPen(outline);
                    gradient.setColorAt(0, grooveColor.darker(110));
                    gradient.setColorAt(1, grooveColor.lighter(110));//palette.button().color().darker(115));
                    cp->setBrush(gradient);
                    cp->drawRoundedRect(pixmapRect.adjusted(1, 1, -2, -2), 1, 1);
                }
                cp.finish();

                // draw blue groove highlight
                QRect clipRect;
                if (horizontal) {
                    if (slider->upsideDown)
                        clipRect = QRect(handle.right(), groove.top(), groove.right() - handle.right(), groove.height());
                    else
                        clipRect = QRect(groove.left(), groove.top(),
                                         handle.left() - slider->rect.left(), groove.height());
                } else {
                    if (slider->upsideDown)
                        clipRect = QRect(groove.left(), handle.bottom(), groove.width(), groove.height() - (handle.bottom() - slider->rect.top()));
                    else
                        clipRect = QRect(groove.left(), groove.top(), groove.width(), handle.top() - groove.top());
                }
                painter->save();
                painter->setClipRect(clipRect.adjusted(0, 0, 1, 1), Qt::IntersectClip);

                QCachedPainter cpBlue(painter, "slider_groove_blue"_L1, option, groove.size(), groove);
                if (cpBlue.needsPainting()) {
                    const QRect pixmapRect = cp.pixmapRect();
                    QLinearGradient gradient;
                    if (horizontal) {
                        gradient.setStart(pixmapRect.center().x(), pixmapRect.top());
                        gradient.setFinalStop(pixmapRect.center().x(), pixmapRect.bottom());
                    }
                    else {
                        gradient.setStart(pixmapRect.left(), pixmapRect.center().y());
                        gradient.setFinalStop(pixmapRect.right(), pixmapRect.center().y());
                    }
                    const QColor highlightedoutline = activeHighlight.darker(140);
                    QColor grooveOutline = outline;
                    if (qGray(grooveOutline.rgb()) > qGray(highlightedoutline.rgb()))
                        grooveOutline = highlightedoutline;

                    cpBlue->translate(0.5, 0.5);
                    cpBlue->setPen(grooveOutline);
                    gradient.setColorAt(0, activeHighlight);
                    gradient.setColorAt(1, activeHighlight.lighter(130));
                    cpBlue->setBrush(gradient);
                    cpBlue->drawRoundedRect(pixmapRect.adjusted(1, 1, -2, -2), 1, 1);
                    cpBlue->setPen(QFusionStylePrivate::innerContrastLine);
                    cpBlue->setBrush(Qt::NoBrush);
                    cpBlue->drawRoundedRect(pixmapRect.adjusted(2, 2, -3, -3), 1, 1);
                }
                cpBlue.finish();
                painter->restore();
            }

            if (option->subControls & SC_SliderTickmarks) {
                painter->save();
                painter->translate(slider->rect.x(), slider->rect.y());
                painter->setRenderHint(QPainter::Antialiasing, false);
                painter->setPen(outline);
                int tickSize = proxy()->pixelMetric(PM_SliderTickmarkOffset, option, widget);
                int available = proxy()->pixelMetric(PM_SliderSpaceAvailable, slider, widget);
                int interval = slider->tickInterval;
                if (interval <= 0) {
                    interval = slider->singleStep;
                    if (QStyle::sliderPositionFromValue(slider->minimum, slider->maximum, interval,
                                                        available)
                            - QStyle::sliderPositionFromValue(slider->minimum, slider->maximum,
                                                              0, available) < 3)
                        interval = slider->pageStep;
                }
                if (interval <= 0)
                    interval = 1;

                int v = slider->minimum;
                int len = proxy()->pixelMetric(PM_SliderLength, slider, widget);
                QVector<QLine> lines;
                while (v <= slider->maximum + 1) {
                    if (v == slider->maximum + 1 && interval == 1)
                        break;
                    const int v_ = qMin(v, slider->maximum);
                    int pos = sliderPositionFromValue(slider->minimum, slider->maximum,
                                                      v_, (horizontal
                                                           ? slider->rect.width()
                                                           : slider->rect.height()) - len,
                                                      slider->upsideDown) + len / 2;
                    int extra = 2 - ((v_ == slider->minimum || v_ == slider->maximum) ? 1 : 0);

                    if (horizontal) {
                        if (ticksAbove) {
                            lines += QLine(pos, slider->rect.top() + extra,
                                           pos, slider->rect.top() + tickSize);
                        }
                        if (ticksBelow) {
                            lines += QLine(pos, slider->rect.bottom() - extra,
                                           pos, slider->rect.bottom() - tickSize);
                        }
                    } else {
                        if (ticksAbove) {
                            lines += QLine(slider->rect.left() + extra, pos,
                                           slider->rect.left() + tickSize, pos);
                        }
                        if (ticksBelow) {
                            lines += QLine(slider->rect.right() - extra, pos,
                                           slider->rect.right() - tickSize, pos);
                        }
                    }
                    // in the case where maximum is max int
                    int nextInterval = v + interval;
                    if (nextInterval < v)
                        break;
                    v = nextInterval;
                }
                painter->drawLines(lines);
                painter->restore();
            }
            // draw handle
            if ((option->subControls & SC_SliderHandle) ) {
                QCachedPainter cp(painter, "slider_handle"_L1, option, handle.size(), handle);
                if (cp.needsPainting()) {
                    const QRect pixmapRect = cp.pixmapRect();
                    QRect gradRect = pixmapRect.adjusted(2, 2, -2, -2);

                    // gradient fill
                    QRect r = pixmapRect.adjusted(1, 1, -2, -2);
                    QLinearGradient gradient = qt_fusion_gradient(gradRect, d->buttonColor(option->palette),horizontal ? TopDown : FromLeft);

                    cp->translate(0.5, 0.5);

                    cp->setPen(Qt::NoPen);
                    cp->setBrush(QColor(0, 0, 0, 40));
                    cp->drawRect(horizontal ? r.adjusted(-1, 2, 1, -2) : r.adjusted(2, -1, -2, 1));

                    cp->setPen(outline);
                    cp->setBrush(gradient);
                    cp->drawRoundedRect(r, 2, 2);
                    cp->setBrush(Qt::NoBrush);
                    cp->setPen(QFusionStylePrivate::innerContrastLine);
                    cp->drawRoundedRect(r.adjusted(1, 1, -1, -1), 2, 2);

                    QColor cornerAlpha = outline.darker(120);
                    cornerAlpha.setAlpha(80);

                    //handle shadow
                    cp->setPen(shadowAlpha);
                    cp->drawLine(QPoint(r.left() + 2, r.bottom() + 1), QPoint(r.right() - 2, r.bottom() + 1));
                    cp->drawLine(QPoint(r.right() + 1, r.bottom() - 3), QPoint(r.right() + 1, r.top() + 4));
                    cp->drawLine(QPoint(r.right() - 1, r.bottom()), QPoint(r.right() + 1, r.bottom() - 2));
                }
            }
        }
        break;
#endif // QT_CONFIG(slider)
#if QT_CONFIG(dial)
    case CC_Dial:
        if (const QStyleOptionSlider *dial = qstyleoption_cast<const QStyleOptionSlider *>(option))
            QStyleHelper::drawDial(dial, painter);
        break;
#endif
    default:
        QCommonStyle::drawComplexControl(control, option, painter, widget);
        break;
    }
}

/*!
  \reimp
*/
int QFusionStyle::pixelMetric(PixelMetric metric, const QStyleOption *option, const QWidget *widget) const
{
    int val = -1;
    switch (metric) {
    case PM_SliderTickmarkOffset:
        val = 4;
        break;
    case PM_HeaderMargin:
    case PM_ToolTipLabelFrameWidth:
        val = 2;
        break;
    case PM_ButtonDefaultIndicator:
    case PM_ButtonShiftHorizontal:
    case PM_ButtonShiftVertical:
        val = 0;
        break;
    case PM_MessageBoxIconSize:
        val = 48;
        break;
    case PM_ListViewIconSize:
        val = 24;
        break;
    case PM_ScrollBarSliderMin:
        val = 26;
        break;
    case PM_TitleBarHeight:
        val = 24;
        break;
    case PM_ScrollBarExtent:
        val = 14;
        break;
    case PM_SliderThickness:
    case PM_SliderLength:
        val = 15;
        break;
    case PM_DockWidgetTitleMargin:
        val = 1;
        break;
    case PM_SpinBoxFrameWidth:
        val = 3;
        break;
    case PM_MenuVMargin:
    case PM_MenuHMargin:
    case PM_MenuPanelWidth:
        val = 0;
        break;
    case PM_MenuBarItemSpacing:
        val = 6;
        break;
    case PM_MenuBarVMargin:
    case PM_MenuBarHMargin:
    case PM_MenuBarPanelWidth:
        val = 0;
        break;
    case PM_ToolBarHandleExtent:
        val = 9;
        break;
    case PM_ToolBarItemSpacing:
        val = 1;
        break;
    case PM_ToolBarFrameWidth:
    case PM_ToolBarItemMargin:
        val = 2;
        break;
    case PM_SmallIconSize:
    case PM_ButtonIconSize:
        val = 16;
        break;
    case PM_DockWidgetTitleBarButtonMargin:
        val = 2;
        break;
    case PM_ButtonMargin:
        val = 6;
        break;
    case PM_TitleBarButtonSize:
        val = 19;
        break;
    case PM_MaximumDragDistance:
        return -1; // Do not dpi-scale because the value is magic
    case PM_TabCloseIndicatorWidth:
    case PM_TabCloseIndicatorHeight:
        val = 20;
        break;
    case PM_TabBarTabVSpace:
        val = 12;
        break;
    case PM_TabBarTabOverlap:
        val = 1;
        break;
    case PM_TabBarBaseOverlap:
        val = 2;
        break;
    case PM_SubMenuOverlap:
        val = -1;
        break;
    case PM_DockWidgetHandleExtent:
    case PM_SplitterWidth:
        val = 4;
        break;
    case PM_IndicatorHeight:
    case PM_IndicatorWidth:
    case PM_ExclusiveIndicatorHeight:
    case PM_ExclusiveIndicatorWidth:
        val = 14;
        break;
    case PM_ScrollView_ScrollBarSpacing:
        val = 0;
        break;
    case PM_ScrollView_ScrollBarOverlap:
        if (proxy()->styleHint(SH_ScrollBar_Transient, option, widget))
            return proxy()->pixelMetric(PM_ScrollBarExtent, option, widget);
        val = 0;
        break;
    case PM_DefaultFrameWidth:
        return 1; // Do not dpi-scale because the drawn frame is always exactly 1 pixel thick
    default:
        return QCommonStyle::pixelMetric(metric, option, widget);
    }
    return QStyleHelper::dpiScaled(val, option);
}

/*!
  \reimp
*/
QSize QFusionStyle::sizeFromContents(ContentsType type, const QStyleOption *option,
                                     const QSize &size, const QWidget *widget) const
{
    QSize newSize = QCommonStyle::sizeFromContents(type, option, size, widget);
    switch (type) {
    case CT_PushButton:
        if (const QStyleOptionButton *btn = qstyleoption_cast<const QStyleOptionButton *>(option)) {
            const int horizontalMargin = pixelMetric(PM_ButtonMargin, btn);
            newSize += QSize(horizontalMargin, 0);
            if (!btn->text.isEmpty() && newSize.width() < 80)
                newSize.setWidth(80);
            if (!btn->icon.isNull() && btn->iconSize.height() > 16)
                newSize -= QSize(0, 2);
        }
        break;
    case CT_GroupBox:
        if (option) {
            int topMargin = qMax(pixelMetric(PM_IndicatorHeight, option, widget), option->fontMetrics.height()) + groupBoxTopMargin;
            newSize += QSize(10, topMargin); // Add some space below the groupbox
        }
        break;
    case CT_RadioButton:
    case CT_CheckBox:
        newSize += QSize(0, 1);
        break;
    case CT_ToolButton:
        newSize += QSize(2, 2);
        break;
    case CT_SpinBox:
        newSize += QSize(0, -3);
        break;
    case CT_ComboBox:
        newSize += QSize(2, 4);
        break;
    case CT_LineEdit:
        newSize += QSize(0, 4);
        break;
    case CT_MenuBarItem:
        newSize += QSize(8, 5);
        break;
    case CT_MenuItem:
        if (const QStyleOptionMenuItem *menuItem = qstyleoption_cast<const QStyleOptionMenuItem *>(option)) {
            int w = size.width(); // Don't rely of QCommonStyle's width calculation here
            if (menuItem->text.contains(u'\t'))
                w += menuItem->reservedShortcutWidth;
            else if (menuItem->menuItemType == QStyleOptionMenuItem::SubMenu)
                w += 2 * QStyleHelper::dpiScaled(QFusionStylePrivate::menuArrowHMargin, option);
            else if (menuItem->menuItemType == QStyleOptionMenuItem::DefaultItem) {
                const QFontMetricsF fm(menuItem->font);
                QFont fontBold = menuItem->font;
                fontBold.setBold(true);
                const QFontMetricsF fmBold(fontBold);
                w += qCeil(fmBold.horizontalAdvance(menuItem->text) - fm.horizontalAdvance(menuItem->text));
            }
            const qreal dpi = QStyleHelper::dpi(option);
             // Windows always shows a check column
            const int checkcol = qMax<int>(menuItem->maxIconWidth,
                                           QStyleHelper::dpiScaled(QFusionStylePrivate::menuCheckMarkWidth, dpi));
            w += checkcol + windowsItemFrame;
            w += QStyleHelper::dpiScaled(int(QFusionStylePrivate::menuRightBorder) + 10, dpi);
            newSize.setWidth(w);
            if (menuItem->menuItemType == QStyleOptionMenuItem::Separator) {
                if (!menuItem->text.isEmpty()) {
                    newSize.setHeight(menuItem->fontMetrics.height());
                }
            }
            else if (!menuItem->icon.isNull()) {
#if QT_CONFIG(combobox)
                if (const QComboBox *combo = qobject_cast<const QComboBox*>(widget)) {
                    newSize.setHeight(qMax(combo->iconSize().height() + 2, newSize.height()));
                }
#endif
            }
            newSize.setWidth(newSize.width() + int(QStyleHelper::dpiScaled(12, dpi)));
            newSize.setWidth(qMax<int>(newSize.width(), int(QStyleHelper::dpiScaled(120, dpi))));
        }
        break;
    case CT_SizeGrip:
        newSize += QSize(4, 4);
        break;
    case CT_MdiControls:
        newSize -= QSize(1, 0);
        break;
    default:
        break;
    }
    return newSize;
}

/*!
  \reimp
*/
void QFusionStyle::polish(QApplication *app)
{
    QCommonStyle::polish(app);
}

/*!
  \reimp
*/
void QFusionStyle::polish(QWidget *widget)
{
    QCommonStyle::polish(widget);
    if (false
#if QT_CONFIG(abstractbutton)
            || qobject_cast<QAbstractButton*>(widget)
#endif
#if QT_CONFIG(combobox)
            || qobject_cast<QComboBox *>(widget)
#endif
#if QT_CONFIG(progressbar)
            || qobject_cast<QProgressBar *>(widget)
#endif
#if QT_CONFIG(scrollbar)
            || qobject_cast<QScrollBar *>(widget)
#endif
#if QT_CONFIG(splitter)
            || qobject_cast<QSplitterHandle *>(widget)
#endif
#if QT_CONFIG(abstractslider)
            || qobject_cast<QAbstractSlider *>(widget)
#endif
#if QT_CONFIG(spinbox)
            || qobject_cast<QAbstractSpinBox *>(widget)
#endif
            || (widget->inherits("QDockSeparator"))
            || (widget->inherits("QDockWidgetSeparator"))
            ) {
        widget->setAttribute(Qt::WA_Hover, true);
        widget->setAttribute(Qt::WA_OpaquePaintEvent, false);
    }
}

/*!
  \reimp
*/
void QFusionStyle::polish(QPalette &pal)
{
    QCommonStyle::polish(pal);
}

/*!
  \reimp
*/
void QFusionStyle::unpolish(QWidget *widget)
{
    QCommonStyle::unpolish(widget);
    if (false
#if QT_CONFIG(abstractbutton)
            || qobject_cast<QAbstractButton*>(widget)
#endif
#if QT_CONFIG(combobox)
            || qobject_cast<QComboBox *>(widget)
#endif
#if QT_CONFIG(progressbar)
            || qobject_cast<QProgressBar *>(widget)
#endif
#if QT_CONFIG(scrollbar)
            || qobject_cast<QScrollBar *>(widget)
#endif
#if QT_CONFIG(splitter)
            || qobject_cast<QSplitterHandle *>(widget)
#endif
#if QT_CONFIG(abstractslider)
            || qobject_cast<QAbstractSlider *>(widget)
#endif
#if QT_CONFIG(spinbox)
            || qobject_cast<QAbstractSpinBox *>(widget)
#endif
            || (widget->inherits("QDockSeparator"))
            || (widget->inherits("QDockWidgetSeparator"))
            ) {
        widget->setAttribute(Qt::WA_Hover, false);
    }
}

/*!
  \reimp
*/
void QFusionStyle::unpolish(QApplication *app)
{
    QCommonStyle::unpolish(app);
}

/*!
  \reimp
*/
QRect QFusionStyle::subControlRect(ComplexControl control, const QStyleOptionComplex *option,
                                   SubControl subControl, const QWidget *widget) const
{
    QRect rect = QCommonStyle::subControlRect(control, option, subControl, widget);

    switch (control) {
#if QT_CONFIG(slider)
    case CC_Slider:
        if (const QStyleOptionSlider *slider = qstyleoption_cast<const QStyleOptionSlider *>(option)) {
            int tickSize = proxy()->pixelMetric(PM_SliderTickmarkOffset, option, widget);
            switch (subControl) {
            case SC_SliderHandle: {
                const bool bothTicks = (slider->tickPosition & QSlider::TicksBothSides) == QSlider::TicksBothSides;
                if (slider->orientation == Qt::Horizontal) {
                    rect.setHeight(proxy()->pixelMetric(PM_SliderThickness, option, widget));
                    rect.setWidth(proxy()->pixelMetric(PM_SliderLength, option, widget));
                    int centerY = slider->rect.center().y() - rect.height() / 2;
                    if (!bothTicks) {
                        if (slider->tickPosition & QSlider::TicksAbove)
                            centerY += tickSize;
                        if (slider->tickPosition & QSlider::TicksBelow)
                            centerY -= tickSize - 1;
                    }
                    rect.moveTop(centerY);
                } else {
                    rect.setWidth(proxy()->pixelMetric(PM_SliderThickness, option, widget));
                    rect.setHeight(proxy()->pixelMetric(PM_SliderLength, option, widget));
                    int centerX = slider->rect.center().x() - rect.width() / 2;
                    if (!bothTicks) {
                        if (slider->tickPosition & QSlider::TicksAbove)
                            centerX += tickSize;
                        if (slider->tickPosition & QSlider::TicksBelow)
                            centerX -= tickSize - 1;
                    }
                    rect.moveLeft(centerX);
                }
            }
                break;
            case SC_SliderGroove: {
                QPoint grooveCenter = slider->rect.center();
                const int grooveThickness = QStyleHelper::dpiScaled(7, option);
                const bool bothTicks = (slider->tickPosition & QSlider::TicksBothSides) == QSlider::TicksBothSides;
                if (slider->orientation == Qt::Horizontal) {
                    rect.setHeight(grooveThickness);
                    if (!bothTicks) {
                        if (slider->tickPosition & QSlider::TicksAbove)
                            grooveCenter.ry() += tickSize;
                        if (slider->tickPosition & QSlider::TicksBelow)
                            grooveCenter.ry() -= tickSize - 1;
                    }
                } else {
                    rect.setWidth(grooveThickness);
                    if (!bothTicks) {
                        if (slider->tickPosition & QSlider::TicksAbove)
                            grooveCenter.rx() += tickSize;
                        if (slider->tickPosition & QSlider::TicksBelow)
                            grooveCenter.rx() -= tickSize - 1;
                    }
                }
                rect.moveCenter(grooveCenter);
                break;
            }
            default:
                break;
            }
        }
        break;
#endif // QT_CONFIG(slider)
#if QT_CONFIG(spinbox)
    case CC_SpinBox:
        if (const QStyleOptionSpinBox *spinbox = qstyleoption_cast<const QStyleOptionSpinBox *>(option)) {
            int center = spinbox->rect.height() / 2;
            int fw = spinbox->frame ? 3 : 0; // Is drawn with 3 pixels width in drawComplexControl, independently from PM_SpinBoxFrameWidth
            int y = fw;
            const int buttonWidth = QStyleHelper::dpiScaled(14, option);
            int x, lx, rx;
            x = spinbox->rect.width() - y - buttonWidth + 2;
            lx = fw;
            rx = x - fw;
            switch (subControl) {
            case SC_SpinBoxUp:
                if (spinbox->buttonSymbols == QAbstractSpinBox::NoButtons)
                    return QRect();
                rect = QRect(x, fw, buttonWidth, center - fw);
                break;
            case SC_SpinBoxDown:
                if (spinbox->buttonSymbols == QAbstractSpinBox::NoButtons)
                    return QRect();

                rect = QRect(x, center, buttonWidth, spinbox->rect.bottom() - center - fw + 1);
                break;
            case SC_SpinBoxEditField:
                if (spinbox->buttonSymbols == QAbstractSpinBox::NoButtons) {
                    rect = QRect(lx, fw, spinbox->rect.width() - 2*fw, spinbox->rect.height() - 2*fw);
                } else {
                    rect = QRect(lx, fw, rx - qMax(fw - 1, 0), spinbox->rect.height() - 2*fw);
                }
                break;
            case SC_SpinBoxFrame:
                rect = spinbox->rect;
                break;
            default:
                break;
            }
            rect = visualRect(spinbox->direction, spinbox->rect, rect);
        }
        break;
#endif // QT_CONFIG(spinbox)
    case CC_GroupBox:
        if (const QStyleOptionGroupBox *groupBox = qstyleoption_cast<const QStyleOptionGroupBox *>(option)) {
            const int groupBoxTextAlignment = groupBox->textAlignment;
            const bool hasVerticalAlignment = (groupBoxTextAlignment & Qt::AlignVertical_Mask) == Qt::AlignVCenter;
            const int fontMetricsHeight = groupBox->text.isEmpty() ? 0 : groupBox->fontMetrics.height();

            if (subControl == SC_GroupBoxFrame)
                return rect;
            else if (subControl == SC_GroupBoxContents) {
                QRect frameRect = option->rect.adjusted(0, 0, 0, -groupBoxBottomMargin);
                int margin = 3;
                int leftMarginExtension = 0;
                const int indicatorHeight = option->subControls.testFlag(SC_GroupBoxCheckBox) ?
                                                        pixelMetric(PM_IndicatorHeight, option, widget) : 0;
                const int topMargin = qMax(indicatorHeight, fontMetricsHeight) +
                                        groupBoxTopMargin;
                return frameRect.adjusted(leftMarginExtension + margin, margin + topMargin, -margin, -margin - groupBoxBottomMargin);
            }

            QSize textSize = option->fontMetrics.boundingRect(groupBox->text).size() + QSize(2, 2);
            int indicatorWidth = proxy()->pixelMetric(PM_IndicatorWidth, option, widget);
            int indicatorHeight = proxy()->pixelMetric(PM_IndicatorHeight, option, widget);

            const int width = textSize.width()
                + (option->subControls & QStyle::SC_GroupBoxCheckBox ? indicatorWidth + 5 : 0);

            rect = QRect();

            if (option->rect.width() > width) {
                switch (groupBoxTextAlignment & Qt::AlignHorizontal_Mask) {
                case Qt::AlignHCenter:
                    rect.moveLeft((option->rect.width() - width) / 2);
                    break;
                case Qt::AlignRight:
                    rect.moveLeft(option->rect.width() - width
                                  - (hasVerticalAlignment ? proxy()->pixelMetric(PM_LayoutRightMargin, groupBox, widget) : 0));
                    break;
                case Qt::AlignLeft:
                    if (hasVerticalAlignment)
                        rect.moveLeft(proxy()->pixelMetric(PM_LayoutLeftMargin, option, widget));
                    break;
                }
            }

            if (subControl == SC_GroupBoxCheckBox) {
                rect.setWidth(indicatorWidth);
                rect.setHeight(indicatorHeight);
                rect.moveTop(textSize.height() > indicatorHeight ? (textSize.height() - indicatorHeight) / 2 : 0);
                rect.translate(1, 0);
            } else if (subControl == SC_GroupBoxLabel) {
                rect.setSize(textSize);
                rect.moveTop(1);
                if (option->subControls & QStyle::SC_GroupBoxCheckBox)
                    rect.translate(indicatorWidth + 5, 0);
            }
            return visualRect(option->direction, option->rect, rect);
        }

        return rect;

    case CC_ComboBox:
        switch (subControl) {
        case SC_ComboBoxArrow: {
            const qreal dpi = QStyleHelper::dpi(option);
            rect = visualRect(option->direction, option->rect, rect);
            rect.setRect(rect.right() - int(QStyleHelper::dpiScaled(18, dpi)), rect.top() - 2,
                         int(QStyleHelper::dpiScaled(19, dpi)), rect.height() + 4);
            rect = visualRect(option->direction, option->rect, rect);
        }
            break;
        case SC_ComboBoxEditField: {
            int frameWidth = 2;
            rect = visualRect(option->direction, option->rect, rect);
            rect.setRect(option->rect.left() + frameWidth, option->rect.top() + frameWidth,
                         option->rect.width() - int(QStyleHelper::dpiScaled(19, option)) - 2 * frameWidth,
                         option->rect.height() - 2 * frameWidth);
            if (const QStyleOptionComboBox *box = qstyleoption_cast<const QStyleOptionComboBox *>(option)) {
                if (!box->editable) {
                    rect.adjust(2, 0, 0, 0);
                    if (box->state & (State_Sunken | State_On))
                        rect.translate(1, 1);
                }
            }
            rect = visualRect(option->direction, option->rect, rect);
            break;
        }
        default:
            break;
        }
        break;
    case CC_TitleBar:
        if (const QStyleOptionTitleBar *tb = qstyleoption_cast<const QStyleOptionTitleBar *>(option)) {
            SubControl sc = subControl;
            QRect &ret = rect;
            const int indent = 3;
            const int controlTopMargin = 3;
            const int controlBottomMargin = 3;
            const int controlWidthMargin = 2;
            const int controlHeight = tb->rect.height() - controlTopMargin - controlBottomMargin ;
            const int delta = controlHeight + controlWidthMargin;
            int offset = 0;

            bool isMinimized = tb->titleBarState & Qt::WindowMinimized;
            bool isMaximized = tb->titleBarState & Qt::WindowMaximized;

            switch (sc) {
            case SC_TitleBarLabel:
                if (tb->titleBarFlags & (Qt::WindowTitleHint | Qt::WindowSystemMenuHint)) {
                    ret = tb->rect;
                    if (tb->titleBarFlags & Qt::WindowSystemMenuHint)
                        ret.adjust(delta, 0, -delta, 0);
                    if (tb->titleBarFlags & Qt::WindowMinimizeButtonHint)
                        ret.adjust(0, 0, -delta, 0);
                    if (tb->titleBarFlags & Qt::WindowMaximizeButtonHint)
                        ret.adjust(0, 0, -delta, 0);
                    if (tb->titleBarFlags & Qt::WindowShadeButtonHint)
                        ret.adjust(0, 0, -delta, 0);
                    if (tb->titleBarFlags & Qt::WindowContextHelpButtonHint)
                        ret.adjust(0, 0, -delta, 0);
                }
                break;
            case SC_TitleBarContextHelpButton:
                if (tb->titleBarFlags & Qt::WindowContextHelpButtonHint)
                    offset += delta;
                Q_FALLTHROUGH();
            case SC_TitleBarMinButton:
                if (!isMinimized && (tb->titleBarFlags & Qt::WindowMinimizeButtonHint))
                    offset += delta;
                else if (sc == SC_TitleBarMinButton)
                    break;
                Q_FALLTHROUGH();
            case SC_TitleBarNormalButton:
                if (isMinimized && (tb->titleBarFlags & Qt::WindowMinimizeButtonHint))
                    offset += delta;
                else if (isMaximized && (tb->titleBarFlags & Qt::WindowMaximizeButtonHint))
                    offset += delta;
                else if (sc == SC_TitleBarNormalButton)
                    break;
                Q_FALLTHROUGH();
            case SC_TitleBarMaxButton:
                if (!isMaximized && (tb->titleBarFlags & Qt::WindowMaximizeButtonHint))
                    offset += delta;
                else if (sc == SC_TitleBarMaxButton)
                    break;
                Q_FALLTHROUGH();
            case SC_TitleBarShadeButton:
                if (!isMinimized && (tb->titleBarFlags & Qt::WindowShadeButtonHint))
                    offset += delta;
                else if (sc == SC_TitleBarShadeButton)
                    break;
                Q_FALLTHROUGH();
            case SC_TitleBarUnshadeButton:
                if (isMinimized && (tb->titleBarFlags & Qt::WindowShadeButtonHint))
                    offset += delta;
                else if (sc == SC_TitleBarUnshadeButton)
                    break;
                Q_FALLTHROUGH();
            case SC_TitleBarCloseButton:
                if (tb->titleBarFlags & Qt::WindowSystemMenuHint)
                    offset += delta;
                else if (sc == SC_TitleBarCloseButton)
                    break;
                ret.setRect(tb->rect.right() - indent - offset, tb->rect.top() + controlTopMargin,
                            controlHeight, controlHeight);
                break;
            case SC_TitleBarSysMenu:
                if (tb->titleBarFlags & Qt::WindowSystemMenuHint) {
                    ret.setRect(tb->rect.left() + controlWidthMargin + indent, tb->rect.top() + controlTopMargin,
                                controlHeight, controlHeight);
                }
                break;
            default:
                break;
            }
            ret = visualRect(tb->direction, tb->rect, ret);
        }
        break;
    default:
        break;
    }

    return rect;
}


/*!
  \reimp
*/
QRect QFusionStyle::itemPixmapRect(const QRect &r, int flags, const QPixmap &pixmap) const
{
    return QCommonStyle::itemPixmapRect(r, flags, pixmap);
}

/*!
  \reimp
*/
void QFusionStyle::drawItemPixmap(QPainter *painter, const QRect &rect,
                                  int alignment, const QPixmap &pixmap) const
{
    QCommonStyle::drawItemPixmap(painter, rect, alignment, pixmap);
}

/*!
  \reimp
*/
QStyle::SubControl QFusionStyle::hitTestComplexControl(ComplexControl cc, const QStyleOptionComplex *opt,
                                                       const QPoint &pt, const QWidget *w) const
{
    return QCommonStyle::hitTestComplexControl(cc, opt, pt, w);
}

/*!
  \reimp
*/
QPixmap QFusionStyle::generatedIconPixmap(QIcon::Mode iconMode, const QPixmap &pixmap,
                                          const QStyleOption *opt) const
{
    return QCommonStyle::generatedIconPixmap(iconMode, pixmap, opt);
}

/*!
  \reimp
*/
int QFusionStyle::styleHint(StyleHint hint, const QStyleOption *option, const QWidget *widget,
                            QStyleHintReturn *returnData) const
{
    switch (hint) {
    case SH_Slider_SnapToValue:
    case SH_PrintDialog_RightAlignButtons:
    case SH_FontDialog_SelectAssociatedText:
    case SH_MenuBar_AltKeyNavigation:
    case SH_ComboBox_ListMouseTracking:
    case SH_Slider_StopMouseOverSlider:
    case SH_ScrollBar_MiddleClickAbsolutePosition:
    case SH_TitleBar_AutoRaise:
    case SH_TitleBar_NoBorder:
    case SH_ItemView_ShowDecorationSelected:
    case SH_ItemView_ArrowKeysNavigateIntoChildren:
    case SH_ItemView_ChangeHighlightOnFocus:
    case SH_MenuBar_MouseTracking:
    case SH_Menu_MouseTracking:
    case SH_Menu_SupportsSections:
        return 1;

#if defined(QT_PLATFORM_UIKIT)
    case SH_ComboBox_UseNativePopup:
        return 1;
#endif

    case SH_EtchDisabledText:
    case SH_ToolBox_SelectedPageTitleBold:
    case SH_ScrollView_FrameOnlyAroundContents:
    case SH_Menu_AllowActiveAndDisabled:
    case SH_MainWindow_SpaceBelowMenuBar:
    case SH_MessageBox_CenterButtons:
    case SH_RubberBand_Mask:
        return 0;

    case SH_ComboBox_Popup:
        if (const QStyleOptionComboBox *cmb = qstyleoption_cast<const QStyleOptionComboBox *>(option))
            return !cmb->editable;
        return 0;

    case SH_Table_GridLineColor:
        return option ? option->palette.window().color().darker(120).rgba() : 0;

    case SH_MessageBox_TextInteractionFlags:
        return Qt::TextSelectableByMouse | Qt::LinksAccessibleByMouse;
#if QT_CONFIG(wizard)
    case SH_WizardStyle:
        return QWizard::ClassicStyle;
#endif
    case SH_Menu_SubMenuPopupDelay:
        return 225; // default from GtkMenu

    case SH_WindowFrame_Mask:
        if (QStyleHintReturnMask *mask = qstyleoption_cast<QStyleHintReturnMask *>(returnData)) {
            //left rounded corner
            mask->region = option->rect;
            mask->region -= QRect(option->rect.left(), option->rect.top(), 5, 1);
            mask->region -= QRect(option->rect.left(), option->rect.top() + 1, 3, 1);
            mask->region -= QRect(option->rect.left(), option->rect.top() + 2, 2, 1);
            mask->region -= QRect(option->rect.left(), option->rect.top() + 3, 1, 2);

            //right rounded corner
            mask->region -= QRect(option->rect.right() - 4, option->rect.top(), 5, 1);
            mask->region -= QRect(option->rect.right() - 2, option->rect.top() + 1, 3, 1);
            mask->region -= QRect(option->rect.right() - 1, option->rect.top() + 2, 2, 1);
            mask->region -= QRect(option->rect.right() , option->rect.top() + 3, 1, 2);
            return 1;
        }
        break;
    case SH_GroupBox_TextLabelVerticalAlignment: {
        if (const auto *groupBox = qstyleoption_cast<const QStyleOptionGroupBox *>(option)) {
            if (groupBox) {
                const auto vAlign = groupBox->textAlignment & Qt::AlignVertical_Mask;
                // default fusion style is AlignTop
                return vAlign == 0 ? Qt::AlignTop : vAlign;
            }
        }
        break;
    }
    default:
        break;
    }
    return QCommonStyle::styleHint(hint, option, widget, returnData);
}

/*! \reimp */
QRect QFusionStyle::subElementRect(SubElement sr, const QStyleOption *opt, const QWidget *w) const
{
    QRect r = QCommonStyle::subElementRect(sr, opt, w);
    switch (sr) {
    case SE_ProgressBarLabel:
    case SE_ProgressBarContents:
    case SE_ProgressBarGroove:
        return opt->rect;
    case SE_PushButtonFocusRect:
        r.adjust(0, 1, 0, -1);
        break;
    case SE_DockWidgetTitleBarText: {
        if (const QStyleOptionDockWidget *titlebar = qstyleoption_cast<const QStyleOptionDockWidget*>(opt)) {
            bool verticalTitleBar = titlebar->verticalTitleBar;
            if (verticalTitleBar) {
                r.adjust(0, 0, 0, -4);
            } else {
                if (opt->direction == Qt::LeftToRight)
                    r.adjust(4, 0, 0, 0);
                else
                    r.adjust(0, 0, -4, 0);
            }
        }

        break;
    }
    default:
        break;
    }
    return r;
}

/*!
    \internal
*/
QIcon QFusionStyle::iconFromTheme(StandardPixmap standardIcon) const
{
    QIcon icon;
#if QT_CONFIG(imageformat_png)
    auto addIconFiles = [](QStringView prefix, QIcon &icon)
    {
        const auto fullPrefix = QStringLiteral(":/qt-project.org/styles/fusionstyle/images/") + prefix;
        static constexpr auto dockTitleIconSizes = {10, 16, 20, 32, 48, 64};
        for (int size : dockTitleIconSizes)
            icon.addFile(fullPrefix + QString::number(size) + QStringLiteral(".png"),
                         QSize(size, size));
    };
    switch (standardIcon) {
    case SP_TitleBarNormalButton:
        addIconFiles(u"fusion_normalizedockup-", icon);
      break;
    case SP_TitleBarMinButton:
        addIconFiles(u"fusion_titlebar-min-", icon);
        break;
    case SP_TitleBarCloseButton:
    case SP_DockWidgetCloseButton:
        addIconFiles(u"fusion_closedock-", icon);
        break;
    default:
        break;
    }
#else  // imageformat_png
    Q_UNUSED(standardIcon);
#endif // imageformat_png
    return icon;
}

/*!
    \reimp
*/
QIcon QFusionStyle::standardIcon(StandardPixmap standardIcon, const QStyleOption *option,
                                 const QWidget *widget) const
{
    const auto icon = iconFromTheme(standardIcon);
    if (!icon.availableSizes().isEmpty())
        return icon;
    return QCommonStyle::standardIcon(standardIcon, option, widget);
}

/*!
 \reimp
 */
QPixmap QFusionStyle::standardPixmap(StandardPixmap standardPixmap, const QStyleOption *opt,
                                     const QWidget *widget) const
{
    const auto icon = iconFromTheme(standardPixmap);
    if (!icon.availableSizes().isEmpty())
        return icon.pixmap(QSize(16, 16), QStyleHelper::getDpr(widget));
    return QCommonStyle::standardPixmap(standardPixmap, opt, widget);
}

bool QFusionStyle::isHighContrast() const
{
    return QGuiApplicationPrivate::platformTheme()->contrastPreference()
            == Qt::ContrastPreference::HighContrast;
}

Qt::ColorScheme QFusionStyle::colorScheme() const
{
    return QGuiApplicationPrivate::platformTheme()->colorScheme();
}

QT_END_NAMESPACE

#include "moc_qfusionstyle_p.cpp"

#endif // style_fusion|| QT_PLUGIN
