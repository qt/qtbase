// Copyright (C) 2022 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#include "qwindows11style_p.h"
#include <qstylehints.h>
#include <private/qstyleanimation_p.h>
#include <private/qstyle_p.h>
#include <private/qstylehelper_p.h>
#include <private/qapplication_p.h>
#include <private/qcombobox_p.h>
#include <qstyleoption.h>
#include <qpainter.h>
#include <qpainterstateguard.h>
#include <QGraphicsDropShadowEffect>
#include <QLatin1StringView>
#include <QtWidgets/qcombobox.h>
#if QT_CONFIG(commandlinkbutton)
#include <QtWidgets/qcommandlinkbutton.h>
#endif
#include <QtWidgets/qgraphicsview.h>
#include <QtWidgets/qlistview.h>
#include <QtWidgets/qmenu.h>
#if QT_CONFIG(mdiarea)
#include <QtWidgets/qmdiarea.h>
#endif
#include <QtWidgets/qtextedit.h>
#include <QtWidgets/qtreeview.h>
#if QT_CONFIG(datetimeedit)
#  include <QtWidgets/qdatetimeedit.h>
#endif
#if QT_CONFIG(tabwidget)
#  include <QtWidgets/qtabwidget.h>
#endif
#include "qdrawutil.h"
#include <chrono>

QT_BEGIN_NAMESPACE

static constexpr int topLevelRoundingRadius    = 8; //Radius for toplevel items like popups for round corners
static constexpr int secondLevelRoundingRadius = 4; //Radius for second level items like hovered menu item round corners
template <typename R, typename P, typename B>
static inline void drawRoundedRect(QPainter *p, R &&rect, P &&pen, B &&brush)
{
    p->setPen(pen);
    p->setBrush(brush);
    p->drawRoundedRect(rect, secondLevelRoundingRadius, secondLevelRoundingRadius);
}

static const QColor WINUI3ColorsLight [] {
    QColor(0x00,0x00,0x00,0x09), //subtleHighlightColor
    QColor(0x00,0x00,0x00,0x06), //subtlePressedColor
    QColor(0x00,0x00,0x00,0x0F), //frameColorLight
    QColor(0x00,0x00,0x00,0x9c), //frameColorStrong
    QColor(0x00,0x00,0x00,0x72), //controlStrongFill
    QColor(0x00,0x00,0x00,0x29), //controlStrokeSecondary
    QColor(0x00,0x00,0x00,0x14), //controlStrokePrimary
    QColor(0xF9,0xF9,0xF9,0x00), //controlFillTertiary
    QColor(0xF9,0xF9,0xF9,0x80), //controlFillSecondary
    QColor(0xFF,0xFF,0xFF,0xFF), //menuPanelFill
    QColor(0xFF,0xFF,0xFF,0xFF), //textOnAccentPrimary
    QColor(0xFF,0xFF,0xFF,0x7F), //textOnAccentSecondary
    QColor(0x00,0x00,0x00,0x7F), //controlTextSecondary
    QColor(0x00,0x00,0x00,0x66), //controlStrokeOnAccentSecondary
    QColor(0xFF,0xFF,0xFF,0xFF), //controlFillSolid
    QColor(0x75,0x75,0x75,0x66), //surfaceStroke
    QColor(0x00,0x00,0x00,0x37), //controlAccentDisabled
    QColor(0xFF,0xFF,0xFF,0xFF), //textAccentDisabled
    QColor(0xFF,0xFF,0xFF,0xFF), //focusFrameInnerStroke
    QColor(0x00,0x00,0x00,0xFF), //focusFrameOuterStroke
};

static const QColor WINUI3ColorsDark[] {
    QColor(0xFF,0xFF,0xFF,0x0F), //subtleHighlightColor
    QColor(0xFF,0xFF,0xFF,0x0A), //subtlePressedColor
    QColor(0xFF,0xFF,0xFF,0x12), //frameColorLight
    QColor(0xFF,0xFF,0xFF,0x8B), //frameColorStrong
    QColor(0xFF,0xFF,0xFF,0x8B), //controlStrongFill
    QColor(0xFF,0xFF,0xFF,0x18), //controlStrokeSecondary
    QColor(0xFF,0xFF,0xFF,0x12), //controlStrokePrimary
    QColor(0xF9,0xF9,0xF9,0x00), //controlFillTertiary
    QColor(0xF9,0xF9,0xF9,0x80), //controlFillSecondary
    QColor(0x0F,0x0F,0x0F,0xFF), //menuPanelFill
    QColor(0x00,0x00,0x00,0xFF), //textOnAccentPrimary
    QColor(0x00,0x00,0x00,0x80), //textOnAccentSecondary
    QColor(0xFF,0xFF,0xFF,0x87), //controlTextSecondary
    QColor(0xFF,0xFF,0xFF,0x14), //controlStrokeOnAccentSecondary
    QColor(0x45,0x45,0x45,0xFF), //controlFillSolid
    QColor(0x75,0x75,0x75,0x66), //surfaceStroke
    QColor(0xFF,0xFF,0xFF,0x28), //controlAccentDisabled
    QColor(0xFF,0xFF,0xFF,0x87), //textAccentDisabled
    QColor(0x00,0x00,0x00,0xFF), //focusFrameInnerStroke
    QColor(0xFF,0xFF,0xFF,0xFF), //focusFrameOuterStroke
};

static const QColor* WINUI3Colors[] {
    WINUI3ColorsLight,
    WINUI3ColorsDark
};

static const QColor shellCloseButtonColor(0xC4,0x2B,0x1C,0xFF); //Color of close Button in Titlebar

#if QT_CONFIG(toolbutton)
static void drawArrow(const QStyle *style, const QStyleOptionToolButton *toolbutton,
                      const QRect &rect, QPainter *painter, const QWidget *widget = nullptr)
{
    QStyle::PrimitiveElement pe;
    switch (toolbutton->arrowType) {
    case Qt::LeftArrow:
        pe = QStyle::PE_IndicatorArrowLeft;
        break;
    case Qt::RightArrow:
        pe = QStyle::PE_IndicatorArrowRight;
        break;
    case Qt::UpArrow:
        pe = QStyle::PE_IndicatorArrowUp;
        break;
    case Qt::DownArrow:
        pe = QStyle::PE_IndicatorArrowDown;
        break;
    default:
        return;
    }
    QStyleOption arrowOpt = *toolbutton;
    arrowOpt.rect = rect;
    style->drawPrimitive(pe, &arrowOpt, painter, widget);
}
#endif // QT_CONFIG(toolbutton)
/*!
  \class QWindows11Style
  \brief The QWindows11Style class provides a look and feel suitable for applications on Microsoft Windows 11.
  \since 6.6
  \ingroup appearance
  \inmodule QtWidgets
  \internal

  \warning This style is only available on the Windows 11 platform and above.

  \sa QWindows11Style QWindowsVistaStyle, QMacStyle, QFusionStyle
*/

/*!
  Constructs a QWindows11Style object.
*/
QWindows11Style::QWindows11Style() : QWindows11Style(*new QWindows11StylePrivate)
{
}

/*!
  \internal
  Constructs a QWindows11Style object.
*/
QWindows11Style::QWindows11Style(QWindows11StylePrivate &dd) : QWindowsVistaStyle(dd)
{
    highContrastTheme = QGuiApplicationPrivate::styleHints->colorScheme() == Qt::ColorScheme::Unknown;
    colorSchemeIndex = QGuiApplicationPrivate::styleHints->colorScheme() == Qt::ColorScheme::Light ? 0 : 1;
}

/*!
  Destructor.
*/
QWindows11Style::~QWindows11Style() = default;

/*!
  \internal
  see drawPrimitive for comments on the animation support

 */
void QWindows11Style::drawComplexControl(ComplexControl control, const QStyleOptionComplex *option,
                                         QPainter *painter, const QWidget *widget) const
{
    QWindows11StylePrivate *d = const_cast<QWindows11StylePrivate*>(d_func());

    State state = option->state;
    SubControls sub = option->subControls;
    State flags = option->state;
    if (widget && widget->testAttribute(Qt::WA_UnderMouse) && widget->isActiveWindow())
        flags |= State_MouseOver;

    painter->save();
    painter->setRenderHint(QPainter::Antialiasing);
    if (d->transitionsEnabled()) {
        if (control == CC_Slider) {
            if (const auto *slider = qstyleoption_cast<const QStyleOptionSlider *>(option)) {
                QObject *styleObject = option->styleObject; // Can be widget or qquickitem

                QRectF thumbRect = proxy()->subControlRect(CC_Slider, option, SC_SliderHandle, widget);
                const qreal outerRadius = qMin(8.0, (slider->orientation == Qt::Horizontal ? thumbRect.height() / 2.0 : thumbRect.width() / 2.0) - 1);
                bool isInsideHandle = option->activeSubControls == SC_SliderHandle;

                bool oldIsInsideHandle = styleObject->property("_q_insidehandle").toBool();
                State oldState = State(styleObject->property("_q_stylestate").toInt());
                SubControls oldActiveControls = SubControls(styleObject->property("_q_stylecontrols").toInt());

                QRectF oldRect = styleObject->property("_q_stylerect").toRect();
                styleObject->setProperty("_q_insidehandle", isInsideHandle);
                styleObject->setProperty("_q_stylestate", int(state));
                styleObject->setProperty("_q_stylecontrols", int(option->activeSubControls));
                styleObject->setProperty("_q_stylerect", option->rect);
                if (option->styleObject->property("_q_end_radius").isNull())
                    option->styleObject->setProperty("_q_end_radius", outerRadius * 0.43);

                bool doTransition = (((state & State_Sunken) != (oldState & State_Sunken)
                                     || (oldIsInsideHandle != isInsideHandle)
                                     || (oldActiveControls != option->activeSubControls))
                                     && state & State_Enabled);

                if (oldRect != option->rect) {
                    doTransition = false;
                    d->stopAnimation(styleObject);
                    styleObject->setProperty("_q_inner_radius", outerRadius * 0.43);
                }

                if (doTransition) {
                    QNumberStyleAnimation *t = new QNumberStyleAnimation(styleObject);
                    t->setStartValue(styleObject->property("_q_inner_radius").toFloat());
                    if (state & State_Sunken)
                        t->setEndValue(outerRadius * 0.29);
                    else if (isInsideHandle)
                        t->setEndValue(outerRadius * 0.71);
                    else
                        t->setEndValue(outerRadius * 0.43);

                    styleObject->setProperty("_q_end_radius", t->endValue());

                    t->setStartTime(d->animationTime());
                    t->setDuration(150);
                    d->startAnimation(t);
                }
            }
        }
    }

    switch (control) {
#if QT_CONFIG(spinbox)
    case CC_SpinBox:
        if (const QStyleOptionSpinBox *sb = qstyleoption_cast<const QStyleOptionSpinBox *>(option)) {
            QCachedPainter cp(painter, QLatin1StringView("win11_spinbox") % HexString<uint8_t>(colorSchemeIndex),
                              sb, sb->rect.size());
            if (cp.needsPainting()) {
                const auto frameRect = QRectF(option->rect).marginsRemoved(QMarginsF(1.5, 1.5, 1.5, 1.5));
                drawRoundedRect(cp.painter(), frameRect, Qt::NoPen, option->palette.brush(QPalette::Base));

                if (sb->frame && (sub & SC_SpinBoxFrame))
                    drawLineEditFrame(cp.painter(), frameRect, option);

                const bool isMouseOver = state & State_MouseOver;
                const bool hasFocus = state & State_HasFocus;
                if (isMouseOver && !hasFocus && !highContrastTheme)
                    drawRoundedRect(cp.painter(), frameRect, Qt::NoPen, winUI3Color(subtleHighlightColor));

                const auto drawUpDown = [&](QStyle::SubControl sc) {
                    const bool isUp = sc == SC_SpinBoxUp;
                    const QRect rect = proxy()->subControlRect(CC_SpinBox, option, sc, widget);
                    if (sb->activeSubControls & sc)
                        drawRoundedRect(cp.painter(), rect.adjusted(1, 1, -1, -2), Qt::NoPen,
                                        winUI3Color(subtleHighlightColor));

                    cp->setFont(assetFont);
                    cp->setPen(sb->palette.buttonText().color());
                    cp->setBrush(Qt::NoBrush);
                    const auto str = isUp ? QStringLiteral(u"\uE70E") : QStringLiteral(u"\uE70D");
                    cp->drawText(rect, str, Qt::AlignVCenter | Qt::AlignHCenter);
                };
                if (sub & SC_SpinBoxUp) drawUpDown(SC_SpinBoxUp);
                if (sub & SC_SpinBoxDown) drawUpDown(SC_SpinBoxDown);
                if (state & State_KeyboardFocusChange && state & State_HasFocus) {
                    QStyleOptionFocusRect fropt;
                    fropt.QStyleOption::operator=(*option);
                    proxy()->drawPrimitive(PE_FrameFocusRect, &fropt, cp.painter(), widget);
                }
            }
        }
        break;
#endif // QT_CONFIG(spinbox)
#if QT_CONFIG(slider)
    case CC_Slider:
        if (const auto *slider = qstyleoption_cast<const QStyleOptionSlider *>(option)) {
            QRectF slrect = slider->rect;
            QRegion tickreg = slrect.toRect();

            if (sub & SC_SliderGroove) {
                QRectF rect = proxy()->subControlRect(CC_Slider, option, SC_SliderGroove, widget);
                QRectF handleRect = proxy()->subControlRect(CC_Slider, option, SC_SliderHandle, widget);
                QPointF handlePos = handleRect.center();
                QRectF leftRect;
                QRectF rightRect;

                if (slider->orientation == Qt::Horizontal) {
                    rect = QRect(slrect.left(), rect.center().y() - 2, slrect.width() - 5, 4);
                    leftRect = QRect(rect.left() + 1, rect.top(), (handlePos.x() - rect.left()), rect.height());
                    rightRect = QRect(handlePos.x(), rect.top(), (rect.width() - handlePos.x()), rect.height());
                } else {
                    rect = QRect(rect.center().x() - 2, slrect.top(), 4, slrect.height() - 5);
                    rightRect = QRect(rect.left(), rect.top() + 1, rect.width(), (handlePos.y() - rect.top()));
                    leftRect = QRect(rect.left(), handlePos.y(), rect.width(), (rect.height() - handlePos.y()));
                }

                painter->setPen(Qt::NoPen);
                painter->setBrush(option->palette.accent());
                painter->drawRoundedRect(leftRect,1,1);
                painter->setBrush(WINUI3Colors[colorSchemeIndex][controlStrongFill]);
                painter->drawRoundedRect(rightRect,1,1);

                painter->setPen(highContrastTheme == true ? slider->palette.buttonText().color()
                                                          : WINUI3Colors[colorSchemeIndex][frameColorLight]);
                painter->setBrush(Qt::NoBrush);
                painter->drawRoundedRect(leftRect,1.5,1.5);
                painter->drawRoundedRect(rightRect,1.5,1.5);

                tickreg -= rect.toRect();
            }
            if (sub & SC_SliderTickmarks) {
                int tickOffset = proxy()->pixelMetric(PM_SliderTickmarkOffset, slider, widget);
                int ticks = slider->tickPosition;
                int thickness = proxy()->pixelMetric(PM_SliderControlThickness, slider, widget);
                int len = proxy()->pixelMetric(PM_SliderLength, slider, widget);
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
                if (!interval)
                    interval = 1;
                int fudge = len / 2;
                int pos;
                int bothOffset = (ticks & QSlider::TicksAbove && ticks & QSlider::TicksBelow) ? 1 : 0;
                painter->setPen(slider->palette.text().color());
                QVarLengthArray<QLineF, 32> lines;
                int v = slider->minimum;
                while (v <= slider->maximum + 1) {
                    if (v == slider->maximum + 1 && interval == 1)
                        break;
                    const int v_ = qMin(v, slider->maximum);
                    int tickLength = (v_ == slider->minimum || v_ >= slider->maximum) ? 4 : 3;
                    pos = QStyle::sliderPositionFromValue(slider->minimum, slider->maximum,
                                                          v_, available) + fudge;
                    if (slider->orientation == Qt::Horizontal) {
                        if (ticks & QSlider::TicksAbove) {
                            lines.append(QLineF(pos, tickOffset - 1 - bothOffset + 0.5,
                                               pos, tickOffset - 1 - bothOffset - tickLength - 0.5));
                        }

                        if (ticks & QSlider::TicksBelow) {
                            lines.append(QLineF(pos, tickOffset + thickness + bothOffset - 0.5,
                                               pos, tickOffset + thickness + bothOffset + tickLength + 0.5));
                        }
                    } else {
                        if (ticks & QSlider::TicksAbove) {
                            lines.append(QLineF(tickOffset - 1 - bothOffset + 0.5, pos,
                                               tickOffset - 1 - bothOffset - tickLength - 0.5, pos));
                        }

                        if (ticks & QSlider::TicksBelow) {
                            lines.append(QLineF(tickOffset + thickness + bothOffset - 0.5, pos,
                                               tickOffset + thickness + bothOffset + tickLength + 0.5, pos));
                        }
                    }
                    // in the case where maximum is max int
                    int nextInterval = v + interval;
                    if (nextInterval < v)
                        break;
                    v = nextInterval;
                }
                if (!lines.isEmpty()) {
                    painter->save();
                    painter->translate(slrect.topLeft());
                    painter->drawLines(lines.constData(), lines.size());
                    painter->restore();
                }
            }
            if (sub & SC_SliderHandle) {
                const QRectF rect = proxy()->subControlRect(CC_Slider, option, SC_SliderHandle, widget);
                const QPointF center = rect.center();

                const QNumberStyleAnimation* animation = qobject_cast<QNumberStyleAnimation*>(d->animation(option->styleObject));

                float innerRadius = 0;
                if (animation != nullptr)
                    innerRadius = animation->currentValue();
                else
                    innerRadius = option->styleObject->property("_q_end_radius").toFloat();
                option->styleObject->setProperty("_q_inner_radius", innerRadius);
                const qreal outerRadius = qMin(8.0,(slider->orientation == Qt::Horizontal ? rect.height() / 2.0 : rect.width() / 2.0) - 1);

                painter->setPen(Qt::NoPen);
                painter->setBrush(winUI3Color(controlFillSolid));
                painter->drawEllipse(center, outerRadius, outerRadius);
                painter->setBrush(option->palette.accent());
                painter->drawEllipse(center, innerRadius, innerRadius);

                painter->setPen(winUI3Color(controlStrokeSecondary));
                painter->setBrush(Qt::NoBrush);
                painter->drawEllipse(center, outerRadius + 0.5, outerRadius + 0.5);
                painter->drawEllipse(center, innerRadius + 0.5, innerRadius + 0.5);
            }
            if (slider->state & State_HasFocus) {
                QStyleOptionFocusRect fropt;
                fropt.QStyleOption::operator=(*slider);
                fropt.rect = subElementRect(SE_SliderFocusRect, slider, widget);
                proxy()->drawPrimitive(PE_FrameFocusRect, &fropt, painter, widget);
            }
        }
        break;
#endif
#if QT_CONFIG(combobox)
    case CC_ComboBox:
        if (const QStyleOptionComboBox *combobox = qstyleoption_cast<const QStyleOptionComboBox *>(option)) {
            const auto frameRect = QRectF(option->rect).marginsRemoved(QMarginsF(1.5, 1.5, 1.5, 1.5));
            drawRoundedRect(painter, frameRect, Qt::NoPen, option->palette.brush(QPalette::Base));

            if (combobox->frame)
                drawLineEditFrame(painter, frameRect, option);

            const bool isMouseOver = state & State_MouseOver;
            const bool hasFocus = state & State_HasFocus;
            if (isMouseOver && !hasFocus && !highContrastTheme)
                drawRoundedRect(painter, frameRect, Qt::NoPen, winUI3Color(subtleHighlightColor));

            if (sub & SC_ComboBoxArrow) {
                QRectF rect = proxy()->subControlRect(CC_ComboBox, option, SC_ComboBoxArrow, widget).adjusted(4, 0, -4, 1);
                painter->setFont(assetFont);
                painter->setPen(combobox->palette.text().color());
                painter->drawText(rect, QStringLiteral(u"\uE70D"), Qt::AlignVCenter | Qt::AlignHCenter);
            }
            if (state & State_HasFocus) {
                drawPrimitive(PE_FrameFocusRect, option, painter, widget);
            }
            if (state & State_KeyboardFocusChange && state & State_HasFocus) {
                QStyleOptionFocusRect fropt;
                fropt.QStyleOption::operator=(*option);
                proxy()->drawPrimitive(PE_FrameFocusRect, &fropt, painter, widget);
            }
        }
        break;
#endif // QT_CONFIG(combobox)
    case CC_ScrollBar:
        if (const QStyleOptionSlider *scrollbar = qstyleoption_cast<const QStyleOptionSlider *>(option)) {
            QCachedPainter cp(painter, QLatin1StringView("win11_scrollbar")
                                       % HexString<uint8_t>(colorSchemeIndex)
                                       % HexString<int>(scrollbar->minimum)
                                       % HexString<int>(scrollbar->maximum)
                                       % HexString<int>(scrollbar->sliderPosition),
                              scrollbar, scrollbar->rect.size());
            if (cp.needsPainting()) {
                const bool vertical = scrollbar->orientation == Qt::Vertical;
                const bool horizontal = scrollbar->orientation == Qt::Horizontal;
                const bool isMouseOver = state & State_MouseOver;
                const bool isRtl = option->direction == Qt::RightToLeft;

                if (isMouseOver) {
                    QRectF rect = scrollbar->rect;
                    const QPointF center = rect.center();
                    if (vertical && rect.width() > 24) {
                        rect.marginsRemoved(QMargins(0, 2, 2, 2));
                        rect.setWidth(rect.width() / 2);
                    } else if (horizontal && rect.height() > 24) {
                        rect.marginsRemoved(QMargins(2, 0, 2, 2));
                        rect.setHeight(rect.height() / 2);
                    }
                    rect.moveCenter(center);
                    cp->setBrush(scrollbar->palette.base());
                    cp->setPen(Qt::NoPen);
                    cp->drawRoundedRect(rect, topLevelRoundingRadius, topLevelRoundingRadius);
                    rect = rect.marginsRemoved(QMarginsF(0.5, 0.5, 0.5, 0.5));
                    cp->setBrush(Qt::NoBrush);
                    cp->setPen(WINUI3Colors[colorSchemeIndex][frameColorLight]);
                    cp->drawRoundedRect(rect, topLevelRoundingRadius + 0.5, topLevelRoundingRadius + 0.5);
                }
                if (sub & SC_ScrollBarSlider) {
                    QRectF rect = proxy()->subControlRect(CC_ScrollBar, option, SC_ScrollBarSlider, widget);
                    const QPointF center = rect.center();
                    if (vertical)
                        rect.setWidth(isMouseOver ? rect.width() / 2 : 1);
                    else
                        rect.setHeight(isMouseOver ? rect.height() / 2 : 1);
                    rect.moveCenter(center);
                    cp->setBrush(Qt::gray);
                    cp->setPen(Qt::NoPen);
                    cp->drawRoundedRect(rect, secondLevelRoundingRadius, secondLevelRoundingRadius);
                }
                if (sub & SC_ScrollBarAddLine) {
                    if (isMouseOver) {
                        const QRectF rect = proxy()->subControlRect(CC_ScrollBar, option, SC_ScrollBarAddLine, widget);
                        QFont f = QFont(assetFont);
                        f.setPointSize(6);
                        cp->setFont(f);
                        cp->setPen(Qt::gray);
                        const auto str = vertical ? QStringLiteral(u"\uEDDC")
                                                  : (isRtl ? QStringLiteral(u"\uEDD9") : QStringLiteral(u"\uEDDA"));
                        cp->drawText(rect, str, Qt::AlignVCenter | Qt::AlignHCenter);
                    }
                }
                if (sub & SC_ScrollBarSubLine) {
                    if (isMouseOver) {
                        const QRectF rect = proxy()->subControlRect(CC_ScrollBar, option, SC_ScrollBarSubLine, widget);
                        QFont f = QFont(assetFont);
                        f.setPointSize(6);
                        cp->setFont(f);
                        cp->setPen(Qt::gray);
                        const auto str = vertical ? QStringLiteral(u"\uEDDB")
                                                  : (isRtl ? QStringLiteral(u"\uEDDA") : QStringLiteral(u"\uEDD9"));
                        cp->drawText(rect, str, Qt::AlignVCenter | Qt::AlignHCenter);
                    }
                }
            }
        }
        break;
    case CC_MdiControls:{
            QFont buttonFont = QFont(assetFont);
            buttonFont.setPointSize(8);
            QPoint mousePos = widget->mapFromGlobal(QCursor::pos());
            if (option->subControls.testFlag(SC_MdiCloseButton)) {
                const QRect closeButtonRect = proxy()->subControlRect(QStyle::CC_MdiControls, option, SC_MdiCloseButton, widget);;
                if (closeButtonRect.isValid()) {
                    bool hover = closeButtonRect.contains(mousePos);
                    if (hover)
                        painter->fillRect(closeButtonRect,shellCloseButtonColor);
                    const QString textToDraw(QStringLiteral(u"\uE8BB"));
                    painter->setPen(hover ? option->palette.highlightedText().color() : option->palette.text().color());
                    painter->setFont(buttonFont);
                    painter->drawText(closeButtonRect, Qt::AlignVCenter | Qt::AlignHCenter, textToDraw);
                }
            }
            if (option->subControls.testFlag(SC_MdiNormalButton)) {
                const QRect normalButtonRect = proxy()->subControlRect(QStyle::CC_MdiControls, option, SC_MdiNormalButton, widget);;
                if (normalButtonRect.isValid()) {
                    bool hover = normalButtonRect.contains(mousePos);
                    if (hover)
                        painter->fillRect(normalButtonRect,WINUI3Colors[colorSchemeIndex][subtleHighlightColor]);
                    const QString textToDraw(QStringLiteral(u"\uE923"));
                    painter->setPen(option->palette.text().color());
                    painter->setFont(buttonFont);
                    painter->drawText(normalButtonRect, Qt::AlignVCenter | Qt::AlignHCenter, textToDraw);
                }
            }
            if (option->subControls.testFlag(QStyle::SC_MdiMinButton)) {
                const QRect minButtonRect = proxy()->subControlRect(QStyle::CC_MdiControls, option, SC_MdiMinButton, widget);
                if (minButtonRect.isValid()) {
                    bool hover = minButtonRect.contains(mousePos);
                    if (hover)
                        painter->fillRect(minButtonRect,WINUI3Colors[colorSchemeIndex][subtleHighlightColor]);
                    const QString textToDraw(QStringLiteral(u"\uE921"));
                    painter->setPen(option->palette.text().color());
                    painter->setFont(buttonFont);
                    painter->drawText(minButtonRect, Qt::AlignVCenter | Qt::AlignHCenter, textToDraw);
                }
            }
        }
        break;
    case CC_TitleBar:
        if (const auto* titlebar = qstyleoption_cast<const QStyleOptionTitleBar*>(option)) {
            painter->setPen(Qt::NoPen);
            painter->setPen(WINUI3Colors[colorSchemeIndex][surfaceStroke]);
            painter->setBrush(titlebar->palette.button());
            painter->drawRect(titlebar->rect);

            // draw title
            QRect textRect = proxy()->subControlRect(CC_TitleBar, titlebar, SC_TitleBarLabel, widget);
            QColor textColor = titlebar->palette.color(titlebar->titleBarState & Qt::WindowActive ? QPalette::Active : QPalette::Disabled,QPalette::WindowText);
            painter->setPen(textColor);
            // Note workspace also does elliding but it does not use the correct font
            QString title = painter->fontMetrics().elidedText(titlebar->text, Qt::ElideRight, textRect.width() - 14);
            painter->drawText(textRect.adjusted(1, 1, -1, -1), title, QTextOption(Qt::AlignHCenter | Qt::AlignVCenter));

            QFont buttonFont = QFont(assetFont);
            buttonFont.setPointSize(8);
            auto drawButton = [&](SubControl sc, const QString &str, QColor col = {}) {
                const QRect buttonRect = proxy()->subControlRect(CC_TitleBar, option, sc, widget);
                if (buttonRect.isValid()) {
                    const bool hover = (option->activeSubControls & sc) &&
                                       (option->state & State_MouseOver);
                    if (hover) {
                        if (!col.isValid())
                            col = WINUI3Colors[colorSchemeIndex][subtleHighlightColor];
                        painter->fillRect(buttonRect, col);
                    }
                    painter->setPen(hover ? option->palette.color(QPalette::Active, QPalette::WindowText)
                                          : textColor);
                    painter->setFont(buttonFont);
                    painter->drawText(buttonRect, Qt::AlignVCenter | Qt::AlignHCenter, str);
                }
            };
            auto shouldDrawButton = [titlebar](SubControl sc, Qt::WindowType flag) {
                return (titlebar->subControls & sc) && (titlebar->titleBarFlags & flag);
            };

            // min button
            if (shouldDrawButton(SC_TitleBarMinButton, Qt::WindowMinimizeButtonHint) &&
                !(titlebar->titleBarState & Qt::WindowMinimized)) {
                drawButton(SC_TitleBarMinButton, QStringLiteral(u"\uE921"));
            }

            // max button
            if (shouldDrawButton(SC_TitleBarMaxButton, Qt::WindowMaximizeButtonHint) &&
                !(titlebar->titleBarState & Qt::WindowMaximized)) {
                drawButton(SC_TitleBarMaxButton, QStringLiteral(u"\uE922"));
            }

            // close button
            if (shouldDrawButton(SC_TitleBarCloseButton, Qt::WindowSystemMenuHint))
                drawButton(SC_TitleBarCloseButton, QStringLiteral(u"\uE8BB"), shellCloseButtonColor);

            // normalize button
            if ((titlebar->subControls & SC_TitleBarNormalButton) &&
                (((titlebar->titleBarFlags & Qt::WindowMinimizeButtonHint) &&
                  (titlebar->titleBarState & Qt::WindowMinimized)) ||
                 ((titlebar->titleBarFlags & Qt::WindowMaximizeButtonHint) &&
                  (titlebar->titleBarState & Qt::WindowMaximized)))) {
                drawButton(SC_TitleBarNormalButton, QStringLiteral(u"\uE923"));
            }

            // context help button
            if (shouldDrawButton(SC_TitleBarContextHelpButton, Qt::WindowContextHelpButtonHint))
                drawButton(SC_TitleBarContextHelpButton, QStringLiteral(u"\uE897"));

            // shade button
            if (shouldDrawButton(SC_TitleBarShadeButton, Qt::WindowShadeButtonHint))
                drawButton(SC_TitleBarShadeButton, QStringLiteral(u"\uE96D"));

             // unshade button
            if (shouldDrawButton(SC_TitleBarUnshadeButton, Qt::WindowShadeButtonHint))
                drawButton(SC_TitleBarUnshadeButton, QStringLiteral(u"\uE96E"));

            // window icon for system menu
            if (shouldDrawButton(SC_TitleBarSysMenu, Qt::WindowSystemMenuHint)) {
                const QRect iconRect = proxy()->subControlRect(CC_TitleBar, titlebar, SC_TitleBarSysMenu, widget);
                if (iconRect.isValid()) {
                    if (!titlebar->icon.isNull()) {
                        titlebar->icon.paint(painter, iconRect);
                    } else {
                        QStyleOption tool = *titlebar;
                        const auto extent = proxy()->pixelMetric(PM_SmallIconSize, &tool, widget);
                        const auto dpr = QStyleHelper::getDpr(widget);
                        const auto icon = proxy()->standardIcon(SP_TitleBarMenuButton, &tool, widget);
                        const auto pm = icon.pixmap(QSize(extent, extent), dpr);
                        proxy()->drawItemPixmap(painter, iconRect, Qt::AlignCenter, pm);
                    }
                }
            }
        }
        break;
    default:
        QWindowsVistaStyle::drawComplexControl(control, option, painter, widget);
    }
    painter->restore();
}

void QWindows11Style::drawPrimitive(PrimitiveElement element, const QStyleOption *option,
                                    QPainter *painter,
                                    const QWidget *widget) const {
    QWindows11StylePrivate *d = const_cast<QWindows11StylePrivate*>(d_func());

    int state = option->state;
    painter->save();
    painter->setRenderHint(QPainter::Antialiasing);
    if (d->transitionsEnabled() && (element == PE_IndicatorCheckBox || element == PE_IndicatorRadioButton)) {
        QObject *styleObject = option->styleObject; // Can be widget or qquickitem
        if (styleObject) {
            int oldState = styleObject->property("_q_stylestate").toInt();
            styleObject->setProperty("_q_stylestate", int(option->state));
            styleObject->setProperty("_q_stylerect", option->rect);
            bool doTransition = (((state & State_Sunken) != (oldState & State_Sunken)
                                 || ((state & State_MouseOver) != (oldState & State_MouseOver))
                                 || (state & State_On) != (oldState & State_On))
                                 && state & State_Enabled);
            if (doTransition) {
                if (element == PE_IndicatorRadioButton) {
                    QNumberStyleAnimation *t = new QNumberStyleAnimation(styleObject);
                    t->setStartValue(styleObject->property("_q_inner_radius").toFloat());
                    t->setEndValue(7.0f);
                    if (option->state & State_Sunken)
                        t->setEndValue(4.0f);
                    else if (option->state & State_MouseOver && !(option->state & State_On))
                        t->setEndValue(7.0f);
                    else if (option->state & State_MouseOver && (option->state & State_On))
                        t->setEndValue(5.0f);
                    else if (option->state & State_On)
                        t->setEndValue(4.0f);
                    styleObject->setProperty("_q_end_radius", t->endValue());
                    t->setStartTime(d->animationTime());
                    t->setDuration(150);
                    d->startAnimation(t);
                }
                else if (element == PE_IndicatorCheckBox) {
                    if ((oldState & State_Off && state & State_On) || (oldState & State_NoChange && state & State_On)) {
                        QNumberStyleAnimation *t = new QNumberStyleAnimation(styleObject);
                        t->setStartValue(0.0f);
                        t->setEndValue(1.0f);
                        t->setStartTime(d->animationTime());
                        t->setDuration(150);
                        d->startAnimation(t);
                    }
                }
            }
        }
    } else if (!d->transitionsEnabled() && element == PE_IndicatorRadioButton) {
        QObject *styleObject = option->styleObject; // Can be widget or qquickitem
        if (styleObject) {
            styleObject->setProperty("_q_end_radius",7.0);
            if (option->state & State_Sunken)
                styleObject->setProperty("_q_end_radius",2.0);
            else if (option->state & State_MouseOver && !(option->state & State_On))
                styleObject->setProperty("_q_end_radius",7.0);
            else if (option->state & State_MouseOver && (option->state & State_On))
                styleObject->setProperty("_q_end_radius",5.0);
            else if (option->state & State_On)
                styleObject->setProperty("_q_end_radius",4.0);
        }
    }

    switch (element) {
    case PE_FrameFocusRect: {
        if (const QStyleOptionFocusRect *fropt = qstyleoption_cast<const QStyleOptionFocusRect *>(option)) {
            if (!(fropt->state & State_KeyboardFocusChange))
                break;
            QRectF focusRect = option->rect;
            focusRect = focusRect.marginsRemoved(QMarginsF(1.5,1.5,1.5,1.5));
            painter->setPen(winUI3Color(focusFrameInnerStroke));
            painter->drawRoundedRect(focusRect,4,4);

            focusRect = focusRect.marginsAdded(QMarginsF(1.0,1.0,1.0,1.0));
            painter->setPen(QPen(winUI3Color(focusFrameOuterStroke),1));
            painter->drawRoundedRect(focusRect,4,4);
        }
        break;
    }
    case PE_PanelTipLabel: {
        const auto rect = QRectF(option->rect).marginsRemoved(QMarginsF(0.5, 0.5, 0.5, 0.5));
        const auto pen = highContrastTheme ? option->palette.buttonText().color()
                                           : winUI3Color(frameColorLight);
        drawRoundedRect(painter, rect, pen, option->palette.toolTipBase());
        break;
    }
    case PE_FrameTabWidget:
#if QT_CONFIG(tabwidget)
        if (const QStyleOptionTabWidgetFrame *frame = qstyleoption_cast<const QStyleOptionTabWidgetFrame *>(option)) {
            const auto rect = QRectF(option->rect).marginsRemoved(QMarginsF(0.5, 0.5, 0.5, 0.5));
            const auto pen = highContrastTheme ? frame->palette.buttonText().color()
                                               : winUI3Color(frameColorLight);
            drawRoundedRect(painter, rect, pen, frame->palette.base());
        }
#endif  // QT_CONFIG(tabwidget)
        break;
    case PE_FrameGroupBox:
        if (const QStyleOptionFrame *frame = qstyleoption_cast<const QStyleOptionFrame *>(option)) {
            const auto pen = highContrastTheme ? frame->palette.buttonText().color()
                                               : winUI3Color(frameColorStrong);
            if (frame->features & QStyleOptionFrame::Flat) {
                painter->setBrush(Qt::NoBrush);
                painter->setPen(pen);
                const QRect &fr = frame->rect;
                QPoint p1(fr.x(), fr.y() + 1);
                QPoint p2(fr.x() + fr.width(), p1.y());
                painter->drawLine(p1, p2);
            } else {
                const auto frameRect = QRectF(frame->rect).marginsRemoved(QMarginsF(1.5, 1.5, 1.5, 1.5));
                drawRoundedRect(painter, frameRect, pen, Qt::NoBrush);
            }
        }
        break;
    case PE_IndicatorHeaderArrow:
        if (const QStyleOptionHeader *header = qstyleoption_cast<const QStyleOptionHeader *>(option)) {
            QFont f(assetFont);
            f.setPointSize(6);
            painter->setFont(f);
            painter->setPen(header->palette.text().color());
            QRectF rect = option->rect;
            if (header->sortIndicator & QStyleOptionHeader::SortUp) {
                painter->drawText(rect, Qt::AlignCenter, QStringLiteral(u"\uE96D"));
            } else if (header->sortIndicator & QStyleOptionHeader::SortDown) {
                painter->drawText(rect, Qt::AlignCenter, QStringLiteral(u"\uE96E"));
            }
        }
        break;
    case PE_IndicatorCheckBox: {
            const bool isRtl = option->direction == Qt::RightToLeft;
            const bool isOn = option->state & State_On;
            const bool isPartial = option->state & State_NoChange;

            QRectF rect = isRtl ? option->rect.adjusted(0, 0, -2, 0) : option->rect.adjusted(2, 0, 0, 0);
            const QPointF center = rect.center();
            rect.setWidth(15);
            rect.setHeight(15);
            rect.moveCenter(center);

            QPen borderPen(Qt::NoPen);
            if (!isOn && !isPartial) {
                borderPen = highContrastTheme ? option->palette.buttonText().color()
                                              : winUI3Color(frameColorStrong);
            }
            drawRoundedRect(painter, rect, borderPen, buttonFillBrush(option));

            if (isOn) {
                painter->setFont(assetFont);
                painter->setPen(option->palette.color(QPalette::Window));
                QNumberStyleAnimation *animation = qobject_cast<QNumberStyleAnimation *>(
                        d->animation(option->styleObject));
                QFontMetrics fm(assetFont);
                float clipWidth = animation != nullptr ? animation->currentValue() : 1.0f;
                QRectF clipRect = fm.boundingRect(QStringLiteral(u"\uE73E"));
                clipRect.moveCenter(center);
                clipRect.setLeft(rect.x() + (rect.width() - clipRect.width()) / 2.0);
                clipRect.setWidth(clipWidth * clipRect.width());
                painter->drawText(clipRect, Qt::AlignVCenter | Qt::AlignLeft, QStringLiteral(u"\uE73E"));
            } else if (isPartial) {
                painter->setFont(assetFont);
                painter->setPen(option->palette.color(QPalette::Window));
                painter->drawText(rect, Qt::AlignCenter, QStringLiteral(u"\uE73C"));
            }
        }
        break;
    case PE_IndicatorBranch: {
            if (option->state & State_Children) {
                const bool isReverse = option->direction == Qt::RightToLeft;
                const bool isOpen = option->state & QStyle::State_Open;
                QFont f(assetFont);
                f.setPointSize(6);
                painter->setFont(f);
                painter->setPen(option->palette.color(isOpen ? QPalette::Active : QPalette::Disabled,
                                                      QPalette::WindowText));
                const auto str = isOpen ? QStringLiteral(u"\uE96E") :
                                          (isReverse ? QStringLiteral(u"\uE96F") : QStringLiteral(u"\uE970"));
                painter->drawText(option->rect, Qt::AlignCenter, str);
            }
        }
        break;
    case PE_IndicatorRadioButton: {
            const bool isRtl = option->direction == Qt::RightToLeft;
            const bool isOn = option->state & State_On;
            qreal innerRadius = option->state & State_On ? 4.0f :7.0f;
            if (option->styleObject) {
                if (option->styleObject->property("_q_end_radius").isNull())
                    option->styleObject->setProperty("_q_end_radius", innerRadius);
                QNumberStyleAnimation *animation = qobject_cast<QNumberStyleAnimation *>(d->animation(option->styleObject));
                innerRadius = animation ? animation->currentValue() : option->styleObject->property("_q_end_radius").toFloat();
                option->styleObject->setProperty("_q_inner_radius", innerRadius);
            }

            QRectF rect = isRtl ? option->rect.adjusted(0, 0, -2, 0) : option->rect.adjusted(2, 0, 0, 0);
            const QPointF center = rect.center();

            if (isOn) {
                painter->setPen(Qt::NoPen);
                painter->setBrush(buttonFillBrush(option));
                QPainterPath path;
                path.addEllipse(center, 7.5, 7.5);
                path.addEllipse(center, innerRadius, innerRadius);
                painter->drawPath(path);
                QColor fillColor = option->palette.window().color();
                if (option->state & State_MouseOver && option->state & State_Enabled)
                    fillColor = fillColor.darker(107);
                painter->setBrush(fillColor);
                painter->drawEllipse(center, innerRadius, innerRadius);
            } else {
                painter->setPen(winUI3Color(frameColorStrong));
                painter->setBrush(buttonFillBrush(option));
                painter->drawEllipse(center, 7.5, 7.5);
            }
        }
        break;
    case PE_PanelButtonTool:
    case PE_PanelButtonBevel:{
            const bool isEnabled = state & QStyle::State_Enabled;
            const bool isMouseOver = state & QStyle::State_MouseOver;
            const bool isRaised = state & QStyle::State_Raised;
            const QRectF rect = option->rect.marginsRemoved(QMargins(2,2,2,2));
            if (element == PE_PanelButtonTool && ((!isMouseOver && !isRaised) || !isEnabled))
                painter->setPen(Qt::NoPen);
            else
                painter->setPen(WINUI3Colors[colorSchemeIndex][controlStrokePrimary]);
            painter->setBrush(buttonFillBrush(option));
            painter->drawRoundedRect(rect,
                                     secondLevelRoundingRadius, secondLevelRoundingRadius);

            if (isRaised) {
                const qreal sublineOffset = secondLevelRoundingRadius - 0.5;
                painter->setPen(WINUI3Colors[colorSchemeIndex][controlStrokeSecondary]);
                painter->drawLine(rect.bottomLeft() + QPointF(sublineOffset, 0.5), rect.bottomRight() + QPointF(-sublineOffset, 0.5));
            }
        }
        break;
    case PE_FrameDefaultButton:
        painter->setPen(option->palette.accent().color());
        painter->setBrush(Qt::NoBrush);
        painter->drawRoundedRect(option->rect, secondLevelRoundingRadius, secondLevelRoundingRadius);
        break;
    case PE_FrameMenu:
        break;
    case PE_PanelMenu: {
        const QRect rect = option->rect.marginsRemoved(QMargins(2, 2, 2, 2));
        painter->setPen(highContrastTheme ? QPen(option->palette.windowText().color(), 2)
                                          : winUI3Color(frameColorLight));
        painter->setBrush(winUI3Color(menuPanelFill));
        painter->drawRoundedRect(rect, topLevelRoundingRadius, topLevelRoundingRadius);
        break;
    }
    case PE_PanelLineEdit:
        if (const auto *panel = qstyleoption_cast<const QStyleOptionFrame *>(option)) {
            const auto frameRect = QRectF(option->rect).marginsRemoved(QMarginsF(1.5, 1.5, 1.5, 1.5));
            drawRoundedRect(painter, frameRect, Qt::NoPen, option->palette.brush(QPalette::Base));

            if (panel->lineWidth > 0)
                proxy()->drawPrimitive(PE_FrameLineEdit, panel, painter, widget);

            const bool isMouseOver = state & State_MouseOver;
            const bool hasFocus = state & State_HasFocus;
            if (isMouseOver && !hasFocus && !highContrastTheme)
                drawRoundedRect(painter, frameRect, Qt::NoPen, winUI3Color(subtleHighlightColor));
        }
        break;
    case PE_FrameLineEdit: {
        const auto frameRect = QRectF(option->rect).marginsRemoved(QMarginsF(1.5, 1.5, 1.5, 1.5));
        drawLineEditFrame(painter, frameRect, option);
        if (state & State_KeyboardFocusChange && state & State_HasFocus) {
            QStyleOptionFocusRect fropt;
            fropt.QStyleOption::operator=(*option);
            proxy()->drawPrimitive(PE_FrameFocusRect, &fropt, painter, widget);
        }
        break;
    }
    case PE_Frame: {
        if (const auto *frame = qstyleoption_cast<const QStyleOptionFrame *>(option)) {
            const auto rect = QRectF(option->rect).marginsRemoved(QMarginsF(1.5, 1.5, 1.5, 1.5));
            if (qobject_cast<const QComboBoxPrivateContainer *>(widget)) {
                QPen pen;
                if (highContrastTheme)
                    pen = QPen(option->palette.windowText().color(), 2);
                else
                    pen = Qt::NoPen;
                drawRoundedRect(painter, rect, pen, WINUI3Colors[colorSchemeIndex][menuPanelFill]);
            } else
                drawRoundedRect(painter, rect, Qt::NoPen, option->palette.brush(QPalette::Base));

            if (frame->frameShape == QFrame::NoFrame)
                break;

            drawLineEditFrame(painter, rect, option, qobject_cast<const QTextEdit *>(widget) != nullptr);
        }
        break;
    }
    case PE_PanelItemViewItem:
        if (const QStyleOptionViewItem *vopt = qstyleoption_cast<const QStyleOptionViewItem *>(option)) {
            if (vopt->backgroundBrush.style() != Qt::NoBrush) {
                QPainterStateGuard psg(painter);
                painter->setBrushOrigin(vopt->rect.topLeft());
                painter->fillRect(vopt->rect, vopt->backgroundBrush);
            }
        }
        break;
    case PE_PanelItemViewRow:
        if (const QStyleOptionViewItem *vopt = qstyleoption_cast<const QStyleOptionViewItem *>(option)) {
            // this is only called from a QTreeView to paint
            //  - the tree branch decoration (incl. selected/hovered or not)
            //  - the (alternate) background of the item in always unselected state
            const QRect &rect = vopt->rect;
            const bool isRtl = option->direction == Qt::RightToLeft;
            if (rect.width() <= 0)
                break;

            painter->setPen(Qt::NoPen);
            if (vopt->features & QStyleOptionViewItem::Alternate)
                painter->setBrush(vopt->palette.alternateBase());
            else
                painter->setBrush(vopt->palette.base());
            painter->drawRect(rect);

            const bool isTreeDecoration = vopt->features.testFlag(
                    QStyleOptionViewItem::IsDecorationForRootColumn);
            if (isTreeDecoration && vopt->state.testAnyFlags(State_Selected | State_MouseOver) &&
                vopt->showDecorationSelected) {
                const bool onlyOne = vopt->viewItemPosition == QStyleOptionViewItem::OnlyOne ||
                                     vopt->viewItemPosition == QStyleOptionViewItem::Invalid;
                bool isFirst = vopt->viewItemPosition == QStyleOptionViewItem::Beginning;
                bool isLast = vopt->viewItemPosition == QStyleOptionViewItem::End;

                if (onlyOne)
                    isFirst = true;

                if (isRtl) {
                    isFirst = !isFirst;
                    isLast = !isLast;
                }

                const QAbstractItemView *view = qobject_cast<const QAbstractItemView *>(widget);
                painter->setBrush(view->alternatingRowColors() ? vopt->palette.highlight() : WINUI3Colors[colorSchemeIndex][subtleHighlightColor]);
                painter->setPen(Qt::NoPen);
                if (isFirst) {
                    painter->save();
                    painter->setClipRect(rect);
                    painter->drawRoundedRect(rect.marginsRemoved(QMargins(2, 2, -secondLevelRoundingRadius, 2)),
                                             secondLevelRoundingRadius, secondLevelRoundingRadius);
                    painter->restore();
                } else if (isLast) {
                    painter->save();
                    painter->setClipRect(rect);
                    painter->drawRoundedRect(rect.marginsRemoved(QMargins(-secondLevelRoundingRadius, 2, 2, 2)),
                                             secondLevelRoundingRadius, secondLevelRoundingRadius);
                    painter->restore();
                } else {
                    painter->drawRect(vopt->rect.marginsRemoved(QMargins(0, 2, 0, 2)));
                }
            }
        }
        break;
    case QStyle::PE_Widget: {
        if (widget && widget->palette().isBrushSet(QPalette::Active, widget->backgroundRole())) {
            const QBrush bg = widget->palette().brush(widget->backgroundRole());
            auto wp = QWidgetPrivate::get(widget);
            QPainterStateGuard psg(painter);
            wp->updateBrushOrigin(painter, bg);
            painter->fillRect(option->rect, bg);
        }
        break;
    }
    case QStyle::PE_FrameWindow:
        if (const auto *frm = qstyleoption_cast<const QStyleOptionFrame *>(option)) {

            QRectF rect= option->rect;
            int fwidth = int((frm->lineWidth + frm->midLineWidth) / QWindowsStylePrivate::nativeMetricScaleFactor(widget));

            QRectF bottomLeftCorner = QRectF(rect.left() + 1.0,
                                             rect.bottom() - 1.0 - secondLevelRoundingRadius,
                                             secondLevelRoundingRadius,
                                             secondLevelRoundingRadius);
            QRectF bottomRightCorner = QRectF(rect.right() - 1.0  - secondLevelRoundingRadius,
                                              rect.bottom() - 1.0  - secondLevelRoundingRadius,
                                              secondLevelRoundingRadius,
                                              secondLevelRoundingRadius);

            //Draw Mask
            if (widget != nullptr) {
                QBitmap mask(widget->width(), widget->height());
                mask.clear();

                QPainter maskPainter(&mask);
                maskPainter.setRenderHint(QPainter::Antialiasing);
                maskPainter.setBrush(Qt::color1);
                maskPainter.setPen(Qt::NoPen);
                maskPainter.drawRoundedRect(option->rect,secondLevelRoundingRadius,secondLevelRoundingRadius);
                const_cast<QWidget*>(widget)->setMask(mask);
            }

            //Draw Window
            painter->setPen(QPen(frm->palette.base(), fwidth));
            painter->drawLine(QPointF(rect.left(), rect.top()),
                              QPointF(rect.left(), rect.bottom() - fwidth));
            painter->drawLine(QPointF(rect.left() + fwidth, rect.bottom()),
                              QPointF(rect.right() - fwidth, rect.bottom()));
            painter->drawLine(QPointF(rect.right(), rect.top()),
                              QPointF(rect.right(), rect.bottom() - fwidth));

            painter->setPen(WINUI3Colors[colorSchemeIndex][surfaceStroke]);
            painter->drawLine(QPointF(rect.left() + 0.5, rect.top() + 0.5),
                              QPointF(rect.left() + 0.5, rect.bottom() - 0.5 - secondLevelRoundingRadius));
            painter->drawLine(QPointF(rect.left() + 0.5 + secondLevelRoundingRadius, rect.bottom() - 0.5),
                              QPointF(rect.right() - 0.5 - secondLevelRoundingRadius, rect.bottom() - 0.5));
            painter->drawLine(QPointF(rect.right() - 0.5, rect.top() + 1.5),
                              QPointF(rect.right() - 0.5, rect.bottom() - 0.5 - secondLevelRoundingRadius));

            painter->setPen(Qt::NoPen);
            painter->setBrush(frm->palette.base());
            painter->drawPie(bottomRightCorner.marginsAdded(QMarginsF(2.5,2.5,0.0,0.0)),
                             270 * 16,90 * 16);
            painter->drawPie(bottomLeftCorner.marginsAdded(QMarginsF(0.0,2.5,2.5,0.0)),
                             -90 * 16,-90 * 16);

            painter->setPen(WINUI3Colors[colorSchemeIndex][surfaceStroke]);
            painter->setBrush(Qt::NoBrush);
            painter->drawArc(bottomRightCorner,
                             0 * 16,-90 * 16);
            painter->drawArc(bottomLeftCorner,
                             -90 * 16,-90 * 16);
        }
        break;
    default:
        QWindowsVistaStyle::drawPrimitive(element, option, painter, widget);
    }
    painter->restore();
}

/*!
    \internal
*/
void QWindows11Style::drawControl(ControlElement element, const QStyleOption *option,
                                  QPainter *painter, const QWidget *widget) const
{
    Q_D(const QWindows11Style);
    State flags = option->state;

    painter->save();
    painter->setRenderHint(QPainter::Antialiasing);
    switch (element) {
    case QStyle::CE_ComboBoxLabel:
        if (const QStyleOptionComboBox *cb = qstyleoption_cast<const QStyleOptionComboBox *>(option)) {
            QStyleOptionComboBox newOption = *cb;
            newOption.rect.adjust(4,0,-4,0);
            QCommonStyle::drawControl(element, &newOption, painter, widget);
        }
        break;
    case QStyle::CE_TabBarTabShape:
#if QT_CONFIG(tabbar)
        if (const QStyleOptionTab *tab = qstyleoption_cast<const QStyleOptionTab *>(option)) {
            QRectF tabRect = tab->rect.marginsRemoved(QMargins(2,2,0,0));
            painter->setPen(Qt::NoPen);
            painter->setBrush(tab->palette.base());
            if (tab->state & State_MouseOver){
                painter->setBrush(WINUI3Colors[colorSchemeIndex][subtleHighlightColor]);
            } else if (tab->state & State_Selected) {
                painter->setBrush(tab->palette.base());
            } else {
                painter->setBrush(tab->palette.window());
            }
            painter->drawRoundedRect(tabRect,2,2);

            painter->setBrush(Qt::NoBrush);
            painter->setPen(highContrastTheme == true ? tab->palette.buttonText().color() : WINUI3Colors[colorSchemeIndex][frameColorLight]);
            painter->drawRoundedRect(tabRect.adjusted(0.5,0.5,-0.5,-0.5),2,2);

        }
#endif  // QT_CONFIG(tabbar)
        break;
    case CE_ToolButtonLabel:
#if QT_CONFIG(toolbutton)
        if (const QStyleOptionToolButton *toolbutton
            = qstyleoption_cast<const QStyleOptionToolButton *>(option)) {
            QRect rect = toolbutton->rect;
            int shiftX = 0;
            int shiftY = 0;
            if (toolbutton->state & (State_Sunken | State_On)) {
                shiftX = proxy()->pixelMetric(PM_ButtonShiftHorizontal, toolbutton, widget);
                shiftY = proxy()->pixelMetric(PM_ButtonShiftVertical, toolbutton, widget);
            }
            // Arrow type always overrules and is always shown
            bool hasArrow = toolbutton->features & QStyleOptionToolButton::Arrow;
            if (((!hasArrow && toolbutton->icon.isNull()) && !toolbutton->text.isEmpty())
                || toolbutton->toolButtonStyle == Qt::ToolButtonTextOnly) {
                int alignment = Qt::AlignCenter | Qt::TextShowMnemonic;
                if (!proxy()->styleHint(SH_UnderlineShortcut, toolbutton, widget))
                    alignment |= Qt::TextHideMnemonic;
                rect.translate(shiftX, shiftY);
                painter->setFont(toolbutton->font);
                const QString text = d->toolButtonElideText(toolbutton, rect, alignment);
                painter->setPen(buttonLabelColor(option));
                proxy()->drawItemText(painter, rect, alignment, toolbutton->palette,
                                      toolbutton->state & State_Enabled, text);
            } else {
                QPixmap pm;
                QSize pmSize = toolbutton->iconSize;
                if (!toolbutton->icon.isNull()) {
                    QIcon::State state = toolbutton->state & State_On ? QIcon::On : QIcon::Off;
                    QIcon::Mode mode;
                    if (!(toolbutton->state & State_Enabled))
                        mode = QIcon::Disabled;
                    else if ((toolbutton->state & State_MouseOver) && (toolbutton->state & State_AutoRaise))
                        mode = QIcon::Active;
                    else
                        mode = QIcon::Normal;
                    pm = toolbutton->icon.pixmap(toolbutton->rect.size().boundedTo(toolbutton->iconSize), painter->device()->devicePixelRatio(),
                                                 mode, state);
                    pmSize = pm.size() / pm.devicePixelRatio();
                }

                if (toolbutton->toolButtonStyle != Qt::ToolButtonIconOnly) {
                    painter->setFont(toolbutton->font);
                    QRect pr = rect,
                            tr = rect;
                    int alignment = Qt::TextShowMnemonic;
                    if (!proxy()->styleHint(SH_UnderlineShortcut, toolbutton, widget))
                        alignment |= Qt::TextHideMnemonic;

                    if (toolbutton->toolButtonStyle == Qt::ToolButtonTextUnderIcon) {
                        pr.setHeight(pmSize.height() + 4); //### 4 is currently hardcoded in QToolButton::sizeHint()
                        tr.adjust(0, pr.height() - 1, 0, -1);
                        pr.translate(shiftX, shiftY);
                        if (!hasArrow) {
                            proxy()->drawItemPixmap(painter, pr, Qt::AlignCenter, pm);
                        } else {
                            drawArrow(proxy(), toolbutton, pr, painter, widget);
                        }
                        alignment |= Qt::AlignCenter;
                    } else {
                        pr.setWidth(pmSize.width() + 4); //### 4 is currently hardcoded in QToolButton::sizeHint()
                        tr.adjust(pr.width(), 0, 0, 0);
                        pr.translate(shiftX, shiftY);
                        if (!hasArrow) {
                            proxy()->drawItemPixmap(painter, QStyle::visualRect(toolbutton->direction, rect, pr), Qt::AlignCenter, pm);
                        } else {
                            drawArrow(proxy(), toolbutton, pr, painter, widget);
                        }
                        alignment |= Qt::AlignLeft | Qt::AlignVCenter;
                    }
                    tr.translate(shiftX, shiftY);
                    const QString text = d->toolButtonElideText(toolbutton, tr, alignment);
                    painter->setPen(buttonLabelColor(option));
                    proxy()->drawItemText(painter, QStyle::visualRect(toolbutton->direction, rect, tr), alignment, toolbutton->palette,
                                          toolbutton->state & State_Enabled, text);
                } else {
                    rect.translate(shiftX, shiftY);
                    if (hasArrow) {
                        drawArrow(proxy(), toolbutton, rect, painter, widget);
                    } else {
                        proxy()->drawItemPixmap(painter, rect, Qt::AlignCenter, pm);
                    }
                }
            }
        }
#endif  // QT_CONFIG(toolbutton)
        break;
    case QStyle::CE_ShapedFrame:
        if (const QStyleOptionFrame *f = qstyleoption_cast<const QStyleOptionFrame *>(option)) {
            int frameShape  = f->frameShape;
            int frameShadow = QFrame::Plain;
            if (f->state & QStyle::State_Sunken)
                frameShadow = QFrame::Sunken;
            else if (f->state & QStyle::State_Raised)
                frameShadow = QFrame::Raised;

            int lw = f->lineWidth;
            int mlw = f->midLineWidth;

            switch (frameShape) {
            case QFrame::Box:
                if (frameShadow == QFrame::Plain)
                    qDrawPlainRoundedRect(painter, f->rect, secondLevelRoundingRadius, secondLevelRoundingRadius, highContrastTheme == true ? f->palette.buttonText().color() : WINUI3Colors[colorSchemeIndex][frameColorStrong], lw);
                else
                    qDrawShadeRect(painter, f->rect, f->palette, frameShadow == QFrame::Sunken, lw, mlw);
                break;
            case QFrame::Panel:
                if (frameShadow == QFrame::Plain)
                    qDrawPlainRoundedRect(painter, f->rect, secondLevelRoundingRadius, secondLevelRoundingRadius, highContrastTheme == true ? f->palette.buttonText().color() : WINUI3Colors[colorSchemeIndex][frameColorStrong], lw);
                else
                    qDrawShadePanel(painter, f->rect, f->palette, frameShadow == QFrame::Sunken, lw);
                break;
            default:
                QWindowsVistaStyle::drawControl(element, option, painter, widget);
            }
        }
        break;
    case QStyle::CE_ProgressBarGroove:{
        if (const QStyleOptionProgressBar* progbaropt = qstyleoption_cast<const QStyleOptionProgressBar*>(option)) {
            QRect rect = subElementRect(SE_ProgressBarContents, progbaropt, widget);
            QPointF center = rect.center();
            if (progbaropt->state & QStyle::State_Horizontal) {
                rect.setHeight(1);
                rect.moveTop(center.y());
            } else {
                rect.setWidth(1);
                rect.moveLeft(center.x());
            }
            painter->setPen(Qt::NoPen);
            painter->setBrush(Qt::gray);
            painter->drawRect(rect);
        }
        break;
    }
    case QStyle::CE_ProgressBarContents:
        if (const QStyleOptionProgressBar* progbaropt = qstyleoption_cast<const QStyleOptionProgressBar*>(option)) {
            const qreal progressBarThickness = 3;
            const qreal progressBarHalfThickness = progressBarThickness / 2.0;
            QRectF rect = subElementRect(SE_ProgressBarContents, progbaropt, widget);
            painter->translate(rect.topLeft());
            rect.moveTo(QPoint(0, 0));
            QRectF originalRect = rect;
            QPointF center = rect.center();
            bool isIndeterminate = progbaropt->maximum == 0 && progbaropt->minimum == 0;
            float fillPercentage = 0;
            const Qt::Orientation orientation = (progbaropt->state & QStyle::State_Horizontal) ? Qt::Horizontal : Qt::Vertical;
            const qreal offset = (orientation == Qt::Horizontal && int(rect.height()) % 2 == 0)
                            || (orientation == Qt::Vertical && int(rect.width()) % 2 == 0) ? 0.5 : 0.0;

            if (isIndeterminate) {
                if (!d->animation(option->styleObject))
                    d->startAnimation(new QProgressStyleAnimation(d->animationFps, option->styleObject));
            } else {
                d->stopAnimation(option->styleObject);
            }

            if (!isIndeterminate) {
                fillPercentage = ((float(progbaropt->progress) - float(progbaropt->minimum)) / (float(progbaropt->maximum) - float(progbaropt->minimum)));
                if (orientation == Qt::Horizontal) {
                    rect.setHeight(progressBarThickness);
                    rect.moveTop(center.y() - progressBarHalfThickness - offset);
                    rect.setWidth(rect.width() * fillPercentage);
                } else {
                    float oldHeight = rect.height();
                    rect.setWidth(progressBarThickness);
                    rect.moveLeft(center.x() - progressBarHalfThickness - offset);
                    rect.moveTop(oldHeight * (1.0f - fillPercentage));
                    rect.setHeight(oldHeight * fillPercentage);
                }
            } else {
                if (qobject_cast<QProgressStyleAnimation *>(d->animation(option->styleObject))) {
                    auto elapsedTime = std::chrono::time_point_cast<std::chrono::milliseconds>(std::chrono::system_clock::now());
                    fillPercentage = (elapsedTime.time_since_epoch().count() % 5000)/(5000.0f*0.75);
                    if (orientation == Qt::Horizontal) {
                        float barBegin = qMin(qMax(fillPercentage-0.25,0.0) * rect.width(), float(rect.width()));
                        float barEnd = qMin(fillPercentage * rect.width(), float(rect.width()));
                        rect = QRect(QPoint(rect.left() + barBegin, rect.top()), QPoint(rect.left() + barEnd, rect.bottom()));
                        rect.setHeight(progressBarThickness);
                        rect.moveTop(center.y() - progressBarHalfThickness - offset);
                    } else {
                        float barBegin = qMin(qMax(fillPercentage-0.25,0.0) * rect.height(), float(rect.height()));
                        float barEnd = qMin(fillPercentage * rect.height(), float(rect.height()));
                        rect = QRect(QPoint(rect.left(), rect.bottom() - barEnd), QPoint(rect.right(), rect.bottom() - barBegin));
                        rect.setWidth(progressBarThickness);
                        rect.moveLeft(center.x() - progressBarHalfThickness - offset);
                    }
                }
            }
            if (progbaropt->invertedAppearance && orientation == Qt::Horizontal)
                rect.moveLeft(originalRect.width() * (1.0 - fillPercentage));
            else if (progbaropt->invertedAppearance && orientation == Qt::Vertical)
                rect.moveBottom(originalRect.height() * fillPercentage);
            painter->setPen(Qt::NoPen);
            painter->setBrush(progbaropt->palette.accent());
            painter->drawRoundedRect(rect, secondLevelRoundingRadius, secondLevelRoundingRadius);
        }
        break;
    case QStyle::CE_ProgressBarLabel:
        if (const QStyleOptionProgressBar* progbaropt = qstyleoption_cast<const QStyleOptionProgressBar*>(option)) {
            const bool vertical = !(progbaropt->state & QStyle::State_Horizontal);
            if (!vertical) {
                QRect rect = subElementRect(SE_ProgressBarLabel, progbaropt, widget);
                painter->setPen(progbaropt->palette.text().color());
                painter->drawText(rect, progbaropt->text, progbaropt->textAlignment);
            }
        }
        break;
    case CE_PushButtonLabel:
        if (const QStyleOptionButton *btn = qstyleoption_cast<const QStyleOptionButton *>(option))  {
            QRect textRect = btn->rect;

            int tf = Qt::AlignVCenter|Qt::TextShowMnemonic;
            if (!proxy()->styleHint(SH_UnderlineShortcut, btn, widget))
                tf |= Qt::TextHideMnemonic;

            if (btn->features & QStyleOptionButton::HasMenu) {
                int indicatorSize = proxy()->pixelMetric(PM_MenuButtonIndicator, btn, widget);
                QLineF menuSplitter;
                QRectF indicatorRect;
                painter->save();
                painter->setFont(assetFont);

                if (btn->direction == Qt::LeftToRight) {
                    indicatorRect = QRect(textRect.x() + textRect.width() - indicatorSize - 4, textRect.y(),2 * 4 + indicatorSize, textRect.height());
                    indicatorRect.adjust(0.5,-0.5,0.5,0.5);
                    menuSplitter = QLineF(indicatorRect.topLeft(),indicatorRect.bottomLeft());
                    textRect = textRect.adjusted(0, 0, -indicatorSize, 0);
                } else {
                    indicatorRect = QRect(textRect.x(), textRect.y(), textRect.x() + indicatorSize + 4, textRect.height());
                    indicatorRect.adjust(-0.5,-0.5,-0.5,0.5);
                    menuSplitter = QLineF(indicatorRect.topRight(),indicatorRect.bottomRight());
                    textRect = textRect.adjusted(indicatorSize, 0, 0, 0);
                }
                painter->drawText(indicatorRect, QStringLiteral(u"\uE70D"), Qt::AlignVCenter | Qt::AlignHCenter);
                painter->setPen(WINUI3Colors[colorSchemeIndex][controlStrokePrimary]);
                painter->drawLine(menuSplitter);
                painter->restore();
            }
            if (!btn->icon.isNull()) {
                //Center both icon and text
                QIcon::Mode mode = btn->state & State_Enabled ? QIcon::Normal : QIcon::Disabled;
                if (mode == QIcon::Normal && btn->state & State_HasFocus)
                    mode = QIcon::Active;
                QIcon::State state = QIcon::Off;
                if (btn->state & State_On)
                    state = QIcon::On;

                QPixmap pixmap = btn->icon.pixmap(btn->iconSize, painter->device()->devicePixelRatio(), mode, state);
                int pixmapWidth = pixmap.width() / pixmap.devicePixelRatio();
                int pixmapHeight = pixmap.height() / pixmap.devicePixelRatio();
                int labelWidth = pixmapWidth;
                int labelHeight = pixmapHeight;
                int iconSpacing = 4;//### 4 is currently hardcoded in QPushButton::sizeHint()
                if (!btn->text.isEmpty()) {
                    int textWidth = btn->fontMetrics.boundingRect(option->rect, tf, btn->text).width();
                    labelWidth += (textWidth + iconSpacing);
                }

                QRect iconRect = QRect(textRect.x() + (textRect.width() - labelWidth) / 2,
                                       textRect.y() + (textRect.height() - labelHeight) / 2,
                                       pixmapWidth, pixmapHeight);

                iconRect = visualRect(btn->direction, textRect, iconRect);

                if (btn->direction == Qt::RightToLeft) {
                    tf |= Qt::AlignRight;
                    textRect.setRight(iconRect.left() - iconSpacing / 2);
                } else {
                    tf |= Qt::AlignLeft; //left align, we adjust the text-rect instead
                    textRect.setLeft(iconRect.left() + iconRect.width() + iconSpacing / 2);
                }

                if (btn->state & (State_On | State_Sunken))
                    iconRect.translate(proxy()->pixelMetric(PM_ButtonShiftHorizontal, option, widget),
                                       proxy()->pixelMetric(PM_ButtonShiftVertical, option, widget));
                painter->drawPixmap(iconRect, pixmap);
            } else {
                tf |= Qt::AlignHCenter;
            }

            painter->setPen(buttonLabelColor(option));
            proxy()->drawItemText(painter, textRect, tf, option->palette,btn->state & State_Enabled, btn->text);
        }
        break;
    case CE_PushButtonBevel:
        if (const QStyleOptionButton *btn = qstyleoption_cast<const QStyleOptionButton *>(option))  {
            QRectF rect = btn->rect.marginsRemoved(QMargins(2, 2, 2, 2));
            painter->setPen(Qt::NoPen);
            if (btn->features.testFlag(QStyleOptionButton::Flat)) {
                painter->setBrush(btn->palette.button());
                painter->drawRoundedRect(rect, secondLevelRoundingRadius, secondLevelRoundingRadius);
                if (flags & (State_Sunken | State_On)) {
                    painter->setBrush(WINUI3Colors[colorSchemeIndex][subtlePressedColor]);
                }
                else if (flags & State_MouseOver) {
                    painter->setBrush(WINUI3Colors[colorSchemeIndex][subtleHighlightColor]);
                }
                painter->drawRoundedRect(rect, secondLevelRoundingRadius, secondLevelRoundingRadius);
            } else {
                if (option->palette.isBrushSet(QPalette::Current, QPalette::Button))
                    painter->setBrush(option->palette.button());
                else if (flags & (State_Sunken))
                    painter->setBrush(flags & State_On ? option->palette.accent().color().lighter(120) : WINUI3Colors[colorSchemeIndex][controlFillTertiary]);
                else if (flags & State_MouseOver)
                    painter->setBrush(flags & State_On ? option->palette.accent().color().lighter(110) : WINUI3Colors[colorSchemeIndex][controlFillSecondary]);
                else if (!(flags & State_Enabled))
                    painter->setBrush(flags & State_On ? WINUI3Colors[colorSchemeIndex][controlAccentDisabled] : option->palette.button());
                else
                    painter->setBrush(flags & State_On ? option->palette.accent() : option->palette.button());
                painter->drawRoundedRect(rect, secondLevelRoundingRadius, secondLevelRoundingRadius);

                rect.adjust(0.5,0.5,-0.5,-0.5);
                const bool defaultButton = btn->features.testFlag(QStyleOptionButton::DefaultButton);
                painter->setBrush(Qt::NoBrush);
                painter->setPen(defaultButton ? option->palette.accent().color()
                                              : WINUI3Colors[colorSchemeIndex][controlStrokePrimary]);
                painter->drawRoundedRect(rect, secondLevelRoundingRadius, secondLevelRoundingRadius);

                painter->setPen(defaultButton ? WINUI3Colors[colorSchemeIndex][controlStrokeOnAccentSecondary]
                                              : WINUI3Colors[colorSchemeIndex][controlStrokeSecondary]);
                if (flags & State_Raised)
                    painter->drawLine(rect.bottomLeft() + QPointF(4.0,0.0), rect.bottomRight() + QPointF(-4,0.0));
            }
        }
        break;
    case CE_MenuBarItem:
        if (const auto *mbi = qstyleoption_cast<const QStyleOptionMenuItem *>(option))  {
            constexpr int hPadding = 11;
            constexpr int topPadding = 4;
            constexpr int bottomPadding = 6;
            bool active = mbi->state & State_Selected;
            bool hasFocus = mbi->state & State_HasFocus;
            bool down = mbi->state & State_Sunken;
            bool enabled = mbi->state & State_Enabled;
            QStyleOptionMenuItem newMbi = *mbi;
            newMbi.font.setPointSize(10);
            if (enabled && active) {
                if (down)
                    painter->setBrushOrigin(painter->brushOrigin() + QPoint(1, 1));
                if (hasFocus) {
                    if (highContrastTheme)
                        painter->setPen(QPen(newMbi.palette.highlight().color(), 2));
                    else
                        painter->setPen(Qt::NoPen);
                    painter->setBrush(highContrastTheme ? newMbi.palette.window().color() : WINUI3Colors[colorSchemeIndex][subtleHighlightColor]);
                    QRect rect = mbi->rect.marginsRemoved(QMargins(5,0,5,0));
                    painter->drawRoundedRect(rect,secondLevelRoundingRadius,secondLevelRoundingRadius, Qt::AbsoluteSize);
                }
            } else if (enabled && highContrastTheme) {
                painter->setPen(QPen(newMbi.palette.windowText().color(), 2));
                painter->setBrush(newMbi.palette.window().color());
                QRect rect = mbi->rect.marginsRemoved(QMargins(5,0,5,0));
                painter->drawRoundedRect(rect,secondLevelRoundingRadius,secondLevelRoundingRadius, Qt::AbsoluteSize);
            }
            newMbi.rect.adjust(hPadding,topPadding,-hPadding,-bottomPadding);
            painter->setFont(newMbi.font);
            QCommonStyle::drawControl(element, &newMbi, painter, widget);
        }
        break;

#if QT_CONFIG(menu)
    case CE_MenuEmptyArea:
        break;

    case CE_MenuItem:
        if (const auto *menuitem = qstyleoption_cast<const QStyleOptionMenuItem *>(option)) {
            int x, y, w, h;
            menuitem->rect.getRect(&x, &y, &w, &h);
            int tab = menuitem->reservedShortcutWidth;
            bool dis = !(menuitem->state & State_Enabled);
            bool checked = menuitem->checkType != QStyleOptionMenuItem::NotCheckable
                    ? menuitem->checked : false;
            bool act = menuitem->state & State_Selected;

            // windows always has a check column, regardless whether we have an icon or not
            int checkcol = qMax<int>(menuitem->maxIconWidth, 32);

            QBrush fill = (act == true && dis == false) ? (highContrastTheme ? menuitem->palette.brush(QPalette::Highlight) : QBrush(WINUI3Colors[colorSchemeIndex][subtleHighlightColor])) : menuitem->palette.brush(QPalette::Button);
            painter->setBrush(fill);
            painter->setPen(Qt::NoPen);
            const QRect rect = menuitem->rect.marginsRemoved(QMargins(2,2,2,2));
            if (act && dis == false)
                painter->drawRoundedRect(rect, secondLevelRoundingRadius, secondLevelRoundingRadius, Qt::AbsoluteSize);

            if (menuitem->menuItemType == QStyleOptionMenuItem::Separator){
                int yoff = 4;
                painter->setPen(highContrastTheme == true ? menuitem->palette.buttonText().color() : WINUI3Colors[colorSchemeIndex][frameColorLight]);
                painter->drawLine(x, y + yoff, x + w, y + yoff  );
                break;
            }

            QRect vCheckRect = visualRect(option->direction, menuitem->rect, QRect(menuitem->rect.x(), menuitem->rect.y(), checkcol, menuitem->rect.height()));
            if (!menuitem->icon.isNull() && checked) {
                if (act) {
                    qDrawShadePanel(painter, vCheckRect,
                                    menuitem->palette, true, 1,
                                    &menuitem->palette.brush(QPalette::Button));
                } else {
                    QBrush fill(menuitem->palette.light().color(), Qt::Dense4Pattern);
                    qDrawShadePanel(painter, vCheckRect, menuitem->palette, true, 1, &fill);
                }
            }
            // On Windows Style, if we have a checkable item and an icon we
            // draw the icon recessed to indicate an item is checked. If we
            // have no icon, we draw a checkmark instead.
            if (!menuitem->icon.isNull()) {
                QIcon::Mode mode = dis ? QIcon::Disabled : QIcon::Normal;
                if (act && !dis)
                    mode = QIcon::Active;
                const auto size = proxy()->pixelMetric(PM_SmallIconSize, option, widget);
                QRect pmr(QPoint(0, 0), QSize(size, size));
                pmr.moveCenter(vCheckRect.center());
                menuitem->icon.paint(painter, pmr, Qt::AlignCenter, mode,
                                     checked ? QIcon::On : QIcon::Off);
            } else if (checked) {
                painter->save();
                if (dis)
                    painter->setPen(menuitem->palette.text().color());
                painter->setFont(assetFont);
                const int text_flags = Qt::AlignVCenter | Qt::AlignHCenter | Qt::TextDontClip | Qt::TextSingleLine;
                const auto textToDraw = QStringLiteral(u"\uE73E");
                painter->setPen(option->palette.text().color());
                painter->drawText(vCheckRect, text_flags, textToDraw);
                painter->restore();
            }
            painter->setPen(act ? menuitem->palette.highlightedText().color() : menuitem->palette.buttonText().color());

            QColor discol = menuitem->palette.text().color();
            if (dis)
                discol = menuitem->palette.color(QPalette::Disabled, QPalette::WindowText);

            QStringView s(menuitem->text);
            if (!s.isEmpty()) {                     // draw text
                int xm = QWindowsStylePrivate::windowsItemFrame + checkcol + QWindowsStylePrivate::windowsItemHMargin;
                int xpos = menuitem->rect.x() + xm;
                QRect textRect(xpos, y + QWindowsStylePrivate::windowsItemVMargin,
                               w - xm - QWindowsStylePrivate::windowsRightBorder - tab + 1, h - 2 * QWindowsStylePrivate::windowsItemVMargin);
                QRect vTextRect = visualRect(option->direction, menuitem->rect, textRect);

                painter->save();
                qsizetype t = s.indexOf(u'\t');
                int text_flags = Qt::AlignVCenter | Qt::TextShowMnemonic | Qt::TextDontClip | Qt::TextSingleLine;
                if (!proxy()->styleHint(SH_UnderlineShortcut, menuitem, widget))
                    text_flags |= Qt::TextHideMnemonic;
                text_flags |= Qt::AlignLeft;
                if (t >= 0) {
                    QRect vShortcutRect = visualRect(option->direction, menuitem->rect,
                                                     QRect(textRect.topRight(), QPoint(menuitem->rect.right(), textRect.bottom())));
                    const QString textToDraw = s.mid(t + 1).toString();
                    if (dis && !act && proxy()->styleHint(SH_EtchDisabledText, option, widget)) {
                        painter->setPen(menuitem->palette.light().color());
                        painter->drawText(vShortcutRect.adjusted(1, 1, 1, 1), text_flags, textToDraw);
                    }
                    if (highContrastTheme)
                        painter->setPen(act ? menuitem->palette.highlightedText().color() : menuitem->palette.buttonText().color());
                    else
                        painter->setPen(menuitem->palette.color(QPalette::Disabled, QPalette::Text));
                    painter->drawText(vShortcutRect, text_flags, textToDraw);
                    s = s.left(t);
                }
                QFont font = menuitem->font;
                if (menuitem->menuItemType == QStyleOptionMenuItem::DefaultItem)
                    font.setBold(true);
                painter->setFont(font);
                const QString textToDraw = s.left(t).toString();
                painter->setPen(highContrastTheme && act ? menuitem->palette.highlightedText().color() : discol);
                painter->drawText(vTextRect, text_flags, textToDraw);
                painter->restore();
            }
            if (menuitem->menuItemType == QStyleOptionMenuItem::SubMenu) {// draw sub menu arrow
                int dim = (h - 2 * QWindowsStylePrivate::windowsItemFrame) / 2;
                int xpos = x + w - QWindowsStylePrivate::windowsArrowHMargin - QWindowsStylePrivate::windowsItemFrame - dim;
                QRect  vSubMenuRect = visualRect(option->direction, menuitem->rect, QRect(xpos, y + h / 2 - dim / 2, dim, dim));
                QStyleOptionMenuItem newMI = *menuitem;
                newMI.rect = vSubMenuRect;
                newMI.state = dis ? State_None : State_Enabled;
                if (act)
                    newMI.palette.setColor(QPalette::ButtonText,
                                           newMI.palette.highlightedText().color());
                painter->save();
                painter->setFont(assetFont);
                int text_flags = Qt::AlignVCenter | Qt::TextShowMnemonic | Qt::TextDontClip | Qt::TextSingleLine;
                if (!proxy()->styleHint(SH_UnderlineShortcut, menuitem, widget))
                    text_flags |= Qt::TextHideMnemonic;
                text_flags |= Qt::AlignLeft;
                painter->setPen(option->palette.text().color());
                const bool isReverse = option->direction == Qt::RightToLeft;
                const auto str = isReverse ? QStringLiteral(u"\uE973") : QStringLiteral(u"\uE974");
                painter->drawText(vSubMenuRect, text_flags, str);
                painter->restore();
            }
        }
        break;
#endif // QT_CONFIG(menu)
    case CE_MenuBarEmptyArea: {
        break;
    }
    case CE_HeaderEmptyArea:
        break;
    case CE_HeaderSection: {
        if (const QStyleOptionHeader *header = qstyleoption_cast<const QStyleOptionHeader *>(option)) {
            painter->setPen(Qt::NoPen);
            painter->setBrush(header->palette.button());
            painter->drawRect(header->rect);

            painter->setPen(highContrastTheme == true ? header->palette.buttonText().color() : WINUI3Colors[colorSchemeIndex][frameColorLight]);
            painter->setBrush(Qt::NoBrush);

            if (header->position == QStyleOptionHeader::OnlyOneSection) {
                break;
            }
            else if (header->position == QStyleOptionHeader::Beginning) {
                painter->drawLine(QPointF(option->rect.topRight()) + QPointF(0.5,0.0),
                                  QPointF(option->rect.bottomRight()) + QPointF(0.5,0.0));
            }
            else if (header->position == QStyleOptionHeader::End) {
                painter->drawLine(QPointF(option->rect.topLeft()) - QPointF(0.5,0.0),
                                  QPointF(option->rect.bottomLeft()) - QPointF(0.5,0.0));
            } else {
                painter->drawLine(QPointF(option->rect.topRight()) + QPointF(0.5,0.0),
                                  QPointF(option->rect.bottomRight()) + QPointF(0.5,0.0));
                painter->drawLine(QPointF(option->rect.topLeft()) - QPointF(0.5,0.0),
                                  QPointF(option->rect.bottomLeft()) - QPointF(0.5,0.0));
            }
            painter->drawLine(QPointF(option->rect.bottomLeft()) + QPointF(0.0,0.5),
                              QPointF(option->rect.bottomRight()) + QPointF(0.0,0.5));
        }
        break;
    }
    case CE_ItemViewItem: {
        if (const QStyleOptionViewItem *vopt = qstyleoption_cast<const QStyleOptionViewItem *>(option)) {
            QRect checkRect = proxy()->subElementRect(SE_ItemViewItemCheckIndicator, vopt, widget);
            QRect iconRect = proxy()->subElementRect(SE_ItemViewItemDecoration, vopt, widget);
            QRect textRect = proxy()->subElementRect(SE_ItemViewItemText, vopt, widget);

            // draw the background
            proxy()->drawPrimitive(PE_PanelItemViewItem, option, painter, widget);

            const QRect &rect = vopt->rect;
            const bool isRtl = option->direction == Qt::RightToLeft;
            bool onlyOne = vopt->viewItemPosition == QStyleOptionViewItem::OnlyOne ||
                           vopt->viewItemPosition == QStyleOptionViewItem::Invalid;
            bool isFirst = vopt->viewItemPosition == QStyleOptionViewItem::Beginning;
            bool isLast = vopt->viewItemPosition == QStyleOptionViewItem::End;

            // the tree decoration already painted the left side of the rounded rect
            if (vopt->features.testFlag(QStyleOptionViewItem::IsDecoratedRootColumn) &&
                vopt->showDecorationSelected) {
                isFirst = false;
                if (onlyOne) {
                    onlyOne = false;
                    isLast = true;
                }
            }

            if (isRtl) {
                if (isFirst) {
                    isFirst = false;
                    isLast = true;
                } else if (isLast) {
                    isFirst = true;
                    isLast = false;
                }
            }
            const bool highlightCurrent = vopt->state.testAnyFlags(State_Selected | State_MouseOver);
            if (highlightCurrent) {
                if (highContrastTheme) {
                    painter->setBrush(vopt->palette.highlight());
                } else {
                    const QAbstractItemView *view = qobject_cast<const QAbstractItemView *>(widget);
                    painter->setBrush(view && view->alternatingRowColors()
                                              ? vopt->palette.highlight()
                                              : winUI3Color(subtleHighlightColor));
                }
            } else {
                painter->setBrush(vopt->backgroundBrush);
            }
            painter->setPen(Qt::NoPen);

            if (onlyOne) {
                painter->drawRoundedRect(rect.marginsRemoved(QMargins(2, 2, 2, 2)),
                                         secondLevelRoundingRadius, secondLevelRoundingRadius);
            } else if (isFirst) {
                painter->save();
                painter->setClipRect(rect);
                painter->drawRoundedRect(rect.marginsRemoved(QMargins(2, 2, -secondLevelRoundingRadius, 2)),
                                         secondLevelRoundingRadius, secondLevelRoundingRadius);
                painter->restore();
            } else if (isLast) {
                painter->save();
                painter->setClipRect(rect);
                painter->drawRoundedRect(rect.marginsRemoved(QMargins(-secondLevelRoundingRadius, 2, 2, 2)),
                                         secondLevelRoundingRadius, secondLevelRoundingRadius);
                painter->restore();
            } else {
                painter->drawRect(rect.marginsRemoved(QMargins(0, 2, 0, 2)));
            }

            // draw the check mark
            if (vopt->features & QStyleOptionViewItem::HasCheckIndicator) {
                QStyleOptionViewItem option(*vopt);
                option.rect = checkRect;
                option.state = option.state & ~QStyle::State_HasFocus;

                switch (vopt->checkState) {
                case Qt::Unchecked:
                    option.state |= QStyle::State_Off;
                    break;
                case Qt::PartiallyChecked:
                    option.state |= QStyle::State_NoChange;
                    break;
                case Qt::Checked:
                    option.state |= QStyle::State_On;
                    break;
                }
                proxy()->drawPrimitive(QStyle::PE_IndicatorItemViewItemCheck, &option, painter, widget);
            }

            // draw the icon
            QIcon::Mode mode = QIcon::Normal;
            if (!(vopt->state & QStyle::State_Enabled))
                mode = QIcon::Disabled;
            else if (vopt->state & QStyle::State_Selected)
                mode = QIcon::Selected;
            QIcon::State state = vopt->state & QStyle::State_Open ? QIcon::On : QIcon::Off;
            vopt->icon.paint(painter, iconRect, vopt->decorationAlignment, mode, state);

            painter->setPen(highlightCurrent && highContrastTheme ? vopt->palette.base().color()
                                                                  : vopt->palette.text().color());
            d->viewItemDrawText(painter, vopt, textRect);

            // paint a vertical marker for QListView
            if (vopt->state & State_Selected) {
                if (const QListView *lv = qobject_cast<const QListView *>(widget);
                    lv && lv->viewMode() != QListView::IconMode && !highContrastTheme) {
                    painter->setPen(vopt->palette.accent().color());
                    const auto xPos = isRtl ? rect.right() - 1 : rect.left();
                    const QLineF lines[2] = {
                        QLineF(xPos, rect.y() + 2, xPos, rect.y() + rect.height() - 2),
                        QLineF(xPos + 1, rect.y() + 2, xPos + 1, rect.y() + rect.height() - 2),
                    };
                    painter->drawLines(lines, 2);
                }
            }
        }
        break;
    }
    default:
        QWindowsVistaStyle::drawControl(element, option, painter, widget);
    }
    painter->restore();
}

int QWindows11Style::styleHint(StyleHint hint, const QStyleOption *opt,
              const QWidget *widget, QStyleHintReturn *returnData) const {
    switch (hint) {
    case QStyle::SH_Menu_AllowActiveAndDisabled:
        return 0;
    case SH_GroupBox_TextLabelColor:
        if (opt!=nullptr && widget!=nullptr)
            return opt->palette.text().color().rgba();
        return 0;
    case QStyle::SH_ItemView_ShowDecorationSelected:
        return 1;
    case QStyle::SH_Slider_AbsoluteSetButtons:
        return Qt::LeftButton;
    case QStyle::SH_Slider_PageSetButtons:
        return 0;
    default:
        return QWindowsVistaStyle::styleHint(hint, opt, widget, returnData);
    }
}

QRect QWindows11Style::subElementRect(QStyle::SubElement element, const QStyleOption *option,
                     const QWidget *widget) const
{
    QRect ret;
    switch (element) {
    case QStyle::SE_RadioButtonIndicator:
    case QStyle::SE_CheckBoxIndicator:
        ret = QWindowsVistaStyle::subElementRect(element, option, widget);
        ret = ret.marginsRemoved(QMargins(4,0,0,0));
        break;
    case QStyle::SE_ComboBoxFocusRect:
    case QStyle::SE_CheckBoxFocusRect:
    case QStyle::SE_RadioButtonFocusRect:
    case QStyle::SE_PushButtonFocusRect:
        ret = option->rect;
        break;
    case QStyle::SE_LineEditContents:
        ret = option->rect.adjusted(4,0,-4,0);
        break;
    case QStyle::SE_ItemViewItemText:
        if (const auto *item = qstyleoption_cast<const QStyleOptionViewItem *>(option)) {
            const int decorationOffset = item->features.testFlag(QStyleOptionViewItem::HasDecoration) ? item->decorationSize.width() : 0;
            const int checkboxOffset = item->features.testFlag(QStyleOptionViewItem::HasCheckIndicator) ? 16 : 0;
            if (widget && qobject_cast<QComboBoxPrivateContainer *>(widget->parentWidget())) {
                if (option->direction == Qt::LeftToRight)
                    ret = option->rect.adjusted(decorationOffset + checkboxOffset + 5, 0, -5, 0);
                else
                    ret = option->rect.adjusted(5, 0, decorationOffset - checkboxOffset - 5, 0);
            } else {
                ret = QWindowsVistaStyle::subElementRect(element, option, widget);
            }
        } else {
            ret = QWindowsVistaStyle::subElementRect(element, option, widget);
        }
        break;
    case QStyle::SE_ProgressBarLabel:
        if (const QStyleOptionProgressBar *pb = qstyleoption_cast<const QStyleOptionProgressBar *>(option)) {
            if (pb->textAlignment.testFlags(Qt::AlignVCenter)) {
                ret = option->rect.adjusted(0, 6, 0, 0);
            } else {
                ret = QWindowsVistaStyle::subElementRect(element, option, widget);
            }
        }
        break;
    case QStyle::SE_HeaderLabel:
    case QStyle::SE_HeaderArrow:
        ret = QCommonStyle::subElementRect(element, option, widget);
        break;
    default:
        ret = QWindowsVistaStyle::subElementRect(element, option, widget);
    }
    return ret;
}

/*!
 \internal
 */
QRect QWindows11Style::subControlRect(ComplexControl control, const QStyleOptionComplex *option,
                                         SubControl subControl, const QWidget *widget) const
{
    QRect ret;

    switch (control) {
#if QT_CONFIG(spinbox)
    case CC_SpinBox:
        if (const QStyleOptionSpinBox *spinbox = qstyleoption_cast<const QStyleOptionSpinBox *>(option)) {
            QSize bs;
            int fw = spinbox->frame ? proxy()->pixelMetric(PM_SpinBoxFrameWidth, spinbox, widget) : 0;
            bs.setHeight(qMax(8, spinbox->rect.height() - fw));
            bs.setWidth(16);
            int y = fw + spinbox->rect.y();
            int x, lx, rx;
            x = spinbox->rect.x() + spinbox->rect.width() - fw - 2 * bs.width();
            lx = fw;
            rx = x - fw;
            switch (subControl) {
            case SC_SpinBoxUp:
                if (spinbox->buttonSymbols == QAbstractSpinBox::NoButtons)
                    return QRect();
                ret = QRect(x, y, bs.width(), bs.height());
                break;
            case SC_SpinBoxDown:
                if (spinbox->buttonSymbols == QAbstractSpinBox::NoButtons)
                    return QRect();
                ret = QRect(x + bs.width(), y, bs.width(), bs.height());
                break;
            case SC_SpinBoxEditField:
                if (spinbox->buttonSymbols == QAbstractSpinBox::NoButtons) {
                    ret = QRect(lx, fw, spinbox->rect.width() - 2*fw, spinbox->rect.height() - 2*fw);
                } else {
                    ret = QRect(lx, fw, rx, spinbox->rect.height() - 2*fw);
                }
                break;
            case SC_SpinBoxFrame:
                ret = spinbox->rect;
            default:
                break;
            }
            ret = visualRect(spinbox->direction, spinbox->rect, ret);
        }
        break;
    case CC_TitleBar:
        if (const QStyleOptionTitleBar *titlebar = qstyleoption_cast<const QStyleOptionTitleBar *>(option)) {
            SubControl sc = subControl;
            ret = QCommonStyle::subControlRect(control, option, subControl, widget);
            static constexpr int indent = 3;
            static constexpr int controlWidthMargin = 2;
            const int controlHeight = titlebar->rect.height();
            const int controlWidth = 46;
            const int iconSize = proxy()->pixelMetric(QStyle::PM_TitleBarButtonIconSize, option, widget);
            int offset = -(controlWidthMargin + indent);

            bool isMinimized = titlebar->titleBarState & Qt::WindowMinimized;
            bool isMaximized = titlebar->titleBarState & Qt::WindowMaximized;

            switch (sc) {
            case SC_TitleBarLabel:
                if (titlebar->titleBarFlags & (Qt::WindowTitleHint | Qt::WindowSystemMenuHint)) {
                    ret = titlebar->rect;
                    if (titlebar->titleBarFlags & Qt::WindowSystemMenuHint)
                        ret.adjust(iconSize + controlWidthMargin + indent, 0, -controlWidth, 0);
                    if (titlebar->titleBarFlags & Qt::WindowMinimizeButtonHint)
                        ret.adjust(0, 0, -controlWidth, 0);
                    if (titlebar->titleBarFlags & Qt::WindowMaximizeButtonHint)
                        ret.adjust(0, 0, -controlWidth, 0);
                    if (titlebar->titleBarFlags & Qt::WindowShadeButtonHint)
                        ret.adjust(0, 0, -controlWidth, 0);
                    if (titlebar->titleBarFlags & Qt::WindowContextHelpButtonHint)
                        ret.adjust(0, 0, -controlWidth, 0);
                }
                break;
            case SC_TitleBarContextHelpButton:
                if (titlebar->titleBarFlags & Qt::WindowContextHelpButtonHint)
                    offset += controlWidth;
                Q_FALLTHROUGH();
            case SC_TitleBarMinButton:
                if (!isMinimized && (titlebar->titleBarFlags & Qt::WindowMinimizeButtonHint))
                    offset += controlWidth;
                else if (sc == SC_TitleBarMinButton)
                    break;
                Q_FALLTHROUGH();
            case SC_TitleBarNormalButton:
                if (isMinimized && (titlebar->titleBarFlags & Qt::WindowMinimizeButtonHint))
                    offset += controlWidth;
                else if (isMaximized && (titlebar->titleBarFlags & Qt::WindowMaximizeButtonHint))
                    offset += controlWidth;
                else if (sc == SC_TitleBarNormalButton)
                    break;
                Q_FALLTHROUGH();
            case SC_TitleBarMaxButton:
                if (!isMaximized && (titlebar->titleBarFlags & Qt::WindowMaximizeButtonHint))
                    offset += controlWidth;
                else if (sc == SC_TitleBarMaxButton)
                    break;
                Q_FALLTHROUGH();
            case SC_TitleBarShadeButton:
                if (!isMinimized && (titlebar->titleBarFlags & Qt::WindowShadeButtonHint))
                    offset += controlWidth;
                else if (sc == SC_TitleBarShadeButton)
                    break;
                Q_FALLTHROUGH();
            case SC_TitleBarUnshadeButton:
                if (isMinimized && (titlebar->titleBarFlags & Qt::WindowShadeButtonHint))
                    offset += controlWidth;
                else if (sc == SC_TitleBarUnshadeButton)
                    break;
                Q_FALLTHROUGH();
            case SC_TitleBarCloseButton:
                if (titlebar->titleBarFlags & Qt::WindowSystemMenuHint)
                    offset += controlWidth;
                else if (sc == SC_TitleBarCloseButton)
                    break;
                ret.setRect(titlebar->rect.right() - offset, titlebar->rect.top(),
                            controlWidth, controlHeight);
                break;
            case SC_TitleBarSysMenu:
                if (titlebar->titleBarFlags & Qt::WindowSystemMenuHint) {
                    const auto yOfs = titlebar->rect.top() + (titlebar->rect.height() - iconSize) / 2;
                    ret.setRect(titlebar->rect.left() + controlWidthMargin + indent, yOfs, iconSize,
                                iconSize);
                }
                break;
            default:
                break;
            }
            if (widget && isMinimized && titlebar->rect.width() < offset)
                const_cast<QWidget*>(widget)->resize(controlWidthMargin + indent + offset + iconSize + controlWidthMargin, controlWidth);
            ret = visualRect(titlebar->direction, titlebar->rect, ret);
        }
        break;
#endif // Qt_NO_SPINBOX
    case CC_ScrollBar:
    {
        ret = QCommonStyle::subControlRect(control, option, subControl, widget);

        if (subControl == SC_ScrollBarAddLine || subControl == SC_ScrollBarSubLine) {
            if (const QStyleOptionSlider *scrollbar = qstyleoption_cast<const QStyleOptionSlider *>(option)) {
                if (scrollbar->orientation == Qt::Vertical)
                    ret = ret.adjusted(2,2,-2,-3);
                else
                    ret = ret.adjusted(3,2,-2,-2);
            }
        }
        break;
    }
    default:
        ret = QWindowsVistaStyle::subControlRect(control, option, subControl, widget);
    }
    return ret;
}

/*!
 \internal
 */
QSize QWindows11Style::sizeFromContents(ContentsType type, const QStyleOption *option,
                                           const QSize &size, const QWidget *widget) const
{
    QSize contentSize(size);

    switch (type) {

#if QT_CONFIG(menubar)
    case CT_MenuBarItem:
        if (!contentSize.isEmpty()) {
            constexpr int hMargin = 2 * 6;
            constexpr int hPadding = 2 * 11;
            constexpr int itemHeight = 32;
            contentSize.setWidth(contentSize.width() + hMargin + hPadding);
#if QT_CONFIG(tabwidget)
            if (widget->parent() && !qobject_cast<const QTabWidget *>(widget->parent()))
#endif
                contentSize.setHeight(itemHeight);
        }
        break;
#endif
#if QT_CONFIG(menu)
    case CT_MenuItem:
        if (const auto *menuItem = qstyleoption_cast<const QStyleOptionMenuItem *>(option)) {
            const int checkcol = qMax<int>(menuItem->maxIconWidth, 32);
            int width = size.width();
            int height;
            if (menuItem->menuItemType == QStyleOptionMenuItem::Separator) {
                width = 10;
                height = 6;
            } else {
                height = menuItem->fontMetrics.height() + 8;
                if (!menuItem->icon.isNull()) {
                    int iconExtent = proxy()->pixelMetric(PM_SmallIconSize, option, widget);
                    height = qMax(height,
                                  menuItem->icon.actualSize(QSize(iconExtent, iconExtent)).height() + 4);
                }
            }
            if (menuItem->text.contains(u'\t'))
                width += menuItem->reservedShortcutWidth;
            else if (menuItem->menuItemType == QStyleOptionMenuItem::SubMenu)
                width += 2 * QWindowsStylePrivate::windowsArrowHMargin;
            else if (menuItem->menuItemType == QStyleOptionMenuItem::DefaultItem) {
                const QFontMetrics fm(menuItem->font);
                QFont fontBold = menuItem->font;
                fontBold.setBold(true);
                const QFontMetrics fmBold(fontBold);
                width += fmBold.horizontalAdvance(menuItem->text) - fm.horizontalAdvance(menuItem->text);
            }
            width += checkcol;
            width += 2 * QWindowsStylePrivate::windowsItemFrame;
             if (!menuItem->text.isEmpty()) {
                width += QWindowsStylePrivate::windowsItemHMargin;
                width += QWindowsStylePrivate::windowsRightBorder;
            }
            contentSize = QSize(width, height);
        }
        break;
#endif // QT_CONFIG(menu)
#if QT_CONFIG(spinbox)
    case QStyle::CT_SpinBox: {
        if (const auto *spinBoxOpt = qstyleoption_cast<const QStyleOptionSpinBox *>(option)) {
            // Add button + frame widths
            const qreal dpi = QStyleHelper::dpi(option);
            const bool hasButtons = (spinBoxOpt->buttonSymbols != QAbstractSpinBox::NoButtons);
            const int margins = 8;
            const int buttonWidth = hasButtons ? qRound(QStyleHelper::dpiScaled(16, dpi)) : 0;
            const int frameWidth = spinBoxOpt->frame ? proxy()->pixelMetric(PM_SpinBoxFrameWidth,
                                                                            spinBoxOpt, widget) : 0;

            contentSize += QSize(2 * buttonWidth + 2 * frameWidth + 2 * margins, 2 * frameWidth);
        }
        break;
    }
#endif
    case CT_ComboBox:
        if (const auto *comboBoxOpt = qstyleoption_cast<const QStyleOptionComboBox *>(option)) {
            contentSize = QWindowsStyle::sizeFromContents(type, option, size, widget);  // don't rely on QWindowsThemeData
            contentSize += QSize(4, 4);     // default win11 style margins
            if (comboBoxOpt->subControls & SC_ComboBoxArrow)
                contentSize += QSize(8, 0); // arrow margins
        }
        break;
    case CT_HeaderSection:
        // windows vista does not honor the indicator (as it was drawn above the text, not on the
        // side) so call QWindowsStyle::styleHint directly to get the correct size hint
        contentSize = QWindowsStyle::sizeFromContents(type, option, size, widget);
        break;
    case CT_RadioButton:
    case CT_CheckBox:
        // the indicator needs 2px more in width when there is no text, not needed when
        // the style draws the text
        contentSize = QWindowsVistaStyle::sizeFromContents(type, option, size, widget);
        if (size.width() == 0)
            contentSize.rwidth() += 2;
        break;
    case CT_PushButton:
        contentSize = QWindowsVistaStyle::sizeFromContents(type, option, size, widget);
        contentSize.rwidth() += 2 * 2; // the CE_PushButtonBevel draws a rounded rect with
                                       // QMargins(2, 2, 2, 2) removed
        break;
    default:
        contentSize = QWindowsVistaStyle::sizeFromContents(type, option, size, widget);
        break;
    }

    return contentSize;
}


/*!
 \internal
 */
int QWindows11Style::pixelMetric(PixelMetric metric, const QStyleOption *option, const QWidget *widget) const
{
    int res = 0;

    switch (metric) {
    case QStyle::PM_IndicatorWidth:
    case QStyle::PM_IndicatorHeight:
    case QStyle::PM_ExclusiveIndicatorWidth:
    case QStyle::PM_ExclusiveIndicatorHeight:
        res = 16;
        break;
    case QStyle::PM_SliderLength:
        res = int(QStyleHelper::dpiScaled(16, option));
        break;
    case QStyle::PM_TitleBarButtonIconSize:
        res = 16;
        break;
    case QStyle::PM_TitleBarButtonSize:
        res = 32;
        break;
    case QStyle::PM_ScrollBarExtent:
        res = 12;
        break;
    case QStyle::PM_SubMenuOverlap:
        res = -1;
        break;
    default:
        res = QWindowsVistaStyle::pixelMetric(metric, option, widget);
    }

    return res;
}

void QWindows11Style::polish(QWidget* widget)
{
#if QT_CONFIG(commandlinkbutton)
    if (!qobject_cast<QCommandLinkButton *>(widget))
#endif // QT_CONFIG(commandlinkbutton)
        QWindowsVistaStyle::polish(widget);

    const bool isScrollBar = qobject_cast<QScrollBar *>(widget);
    const auto comboBoxContainer = qobject_cast<const QComboBoxPrivateContainer *>(widget);
    if (isScrollBar || qobject_cast<QMenu *>(widget) || comboBoxContainer) {
        bool wasCreated = widget->testAttribute(Qt::WA_WState_Created);
        bool layoutDirection = widget->testAttribute(Qt::WA_RightToLeft);
        widget->setAttribute(Qt::WA_OpaquePaintEvent,false);
        widget->setAttribute(Qt::WA_TranslucentBackground);
        if (!isScrollBar)
            widget->setWindowFlag(Qt::FramelessWindowHint);
        widget->setWindowFlag(Qt::NoDropShadowWindowHint);
        widget->setAttribute(Qt::WA_RightToLeft, layoutDirection);
        widget->setAttribute(Qt::WA_WState_Created, wasCreated);
        if (!isScrollBar) {
            bool inGraphicsView = widget->graphicsProxyWidget() != nullptr;
            if (!inGraphicsView && comboBoxContainer && comboBoxContainer->parentWidget())
                inGraphicsView = comboBoxContainer->parentWidget()->graphicsProxyWidget() != nullptr;
            if (!inGraphicsView) { // for menus and combobox containers...
                QGraphicsDropShadowEffect *dropshadow = new QGraphicsDropShadowEffect(widget);
                dropshadow->setBlurRadius(3);
                dropshadow->setXOffset(3);
                dropshadow->setYOffset(3);
                widget->setGraphicsEffect(dropshadow);
            }
        }
    } else if (QComboBox* cb = qobject_cast<QComboBox*>(widget)) {
        if (cb->isEditable()) {
            QLineEdit *le = cb->lineEdit();
            le->setFrame(false);
        }
    } else if (qobject_cast<QGraphicsView *>(widget) && !qobject_cast<QTextEdit *>(widget)) {
        QPalette pal = widget->palette();
        pal.setColor(QPalette::Base, pal.window().color());
        widget->setPalette(pal);
    } else if (const auto *scrollarea = qobject_cast<QAbstractScrollArea *>(widget);
               scrollarea
#if QT_CONFIG(mdiarea)
               && !qobject_cast<QMdiArea *>(widget)
#endif
        ) {
        if (scrollarea->frameShape() == QFrame::StyledPanel) {
            const auto vp = scrollarea->viewport();
            const bool isAutoFillBackground = vp->autoFillBackground();
            const bool isStyledBackground = vp->testAttribute(Qt::WA_StyledBackground);
            vp->setProperty("_q_original_autofill_background", isAutoFillBackground);
            vp->setProperty("_q_original_styled_background", isStyledBackground);
            vp->setAutoFillBackground(false);
            vp->setAttribute(Qt::WA_StyledBackground, true);
        }
        // QTreeView & QListView are already set in the base windowsvista style
        if (auto table = qobject_cast<QTableView *>(widget))
            table->viewport()->setAttribute(Qt::WA_Hover, true);
    }
}

void QWindows11Style::unpolish(QWidget *widget)
{
#if QT_CONFIG(commandlinkbutton)
    if (!qobject_cast<QCommandLinkButton *>(widget))
#endif // QT_CONFIG(commandlinkbutton)
        QWindowsVistaStyle::unpolish(widget);

    if (const auto *scrollarea = qobject_cast<QAbstractScrollArea *>(widget);
        scrollarea
#if QT_CONFIG(mdiarea)
        && !qobject_cast<QMdiArea *>(widget)
#endif
        ) {
        const auto vp = scrollarea->viewport();
        const auto wasAutoFillBackground = vp->property("_q_original_autofill_background").toBool();
        vp->setAutoFillBackground(wasAutoFillBackground);
        vp->setProperty("_q_original_autofill_background", QVariant());
        const auto origStyledBackground = vp->property("_q_original_styled_background").toBool();
        vp->setAttribute(Qt::WA_StyledBackground, origStyledBackground);
        vp->setProperty("_q_original_styled_background", QVariant());
    }
}

/*
The colors for Windows 11 are taken from the official WinUI3 Figma style at
https://www.figma.com/community/file/1159947337437047524
*/
#define SET_IF_UNRESOLVED(GROUP, ROLE, VALUE) \
    if (!result.isBrushSet(QPalette::Inactive, ROLE) || styleSheetChanged) \
        result.setColor(GROUP, ROLE, VALUE)

static void populateLightSystemBasePalette(QPalette &result)
{
    static QString oldStyleSheet;
    const bool styleSheetChanged = oldStyleSheet != qApp->styleSheet();

    constexpr QColor textColor = QColor(0x00,0x00,0x00,0xE4);
    constexpr QColor textDisabled = QColor(0x00, 0x00, 0x00, 0x5C);
    constexpr QColor btnFace = QColor(0xFF, 0xFF, 0xFF, 0xB3);
    constexpr QColor base = QColor(0xFF, 0xFF, 0xFF, 0xFF);
    constexpr QColor alternateBase = QColor(0x00, 0x00, 0x00, 0x09);
    const QColor btnHighlight = result.accent().color();
    const QColor btnColor = result.button().color();

    SET_IF_UNRESOLVED(QPalette::Active, QPalette::Highlight, btnHighlight);
    SET_IF_UNRESOLVED(QPalette::Active, QPalette::WindowText, textColor);
    SET_IF_UNRESOLVED(QPalette::Active, QPalette::Button, btnFace);
    SET_IF_UNRESOLVED(QPalette::Active, QPalette::Light, btnColor.lighter(150));
    SET_IF_UNRESOLVED(QPalette::Active, QPalette::Dark, btnColor.darker(200));
    SET_IF_UNRESOLVED(QPalette::Active, QPalette::Mid, btnColor.darker(150));
    SET_IF_UNRESOLVED(QPalette::Active, QPalette::Text, textColor);
    SET_IF_UNRESOLVED(QPalette::Active, QPalette::BrightText, btnHighlight);
    SET_IF_UNRESOLVED(QPalette::Active, QPalette::Base, base);
    SET_IF_UNRESOLVED(QPalette::Active, QPalette::Window, QColor(0xF3,0xF3,0xF3,0xFF));
    SET_IF_UNRESOLVED(QPalette::Active, QPalette::ButtonText, textColor);
    SET_IF_UNRESOLVED(QPalette::Active, QPalette::Midlight, btnColor.lighter(125));
    SET_IF_UNRESOLVED(QPalette::Active, QPalette::Shadow, Qt::black);
    SET_IF_UNRESOLVED(QPalette::Active, QPalette::ToolTipBase, result.window().color());
    SET_IF_UNRESOLVED(QPalette::Active, QPalette::ToolTipText, result.windowText().color());
    SET_IF_UNRESOLVED(QPalette::Active, QPalette::AlternateBase, alternateBase);

    SET_IF_UNRESOLVED(QPalette::Inactive, QPalette::Highlight, btnHighlight);
    SET_IF_UNRESOLVED(QPalette::Inactive, QPalette::WindowText, textColor);
    SET_IF_UNRESOLVED(QPalette::Inactive, QPalette::Button, btnFace);
    SET_IF_UNRESOLVED(QPalette::Inactive, QPalette::Light, btnColor.lighter(150));
    SET_IF_UNRESOLVED(QPalette::Inactive, QPalette::Dark, btnColor.darker(200));
    SET_IF_UNRESOLVED(QPalette::Inactive, QPalette::Mid, btnColor.darker(150));
    SET_IF_UNRESOLVED(QPalette::Inactive, QPalette::Text, textColor);
    SET_IF_UNRESOLVED(QPalette::Inactive, QPalette::BrightText, btnHighlight);
    SET_IF_UNRESOLVED(QPalette::Inactive, QPalette::Base, base);
    SET_IF_UNRESOLVED(QPalette::Inactive, QPalette::Window, QColor(0xF3,0xF3,0xF3,0xFF));
    SET_IF_UNRESOLVED(QPalette::Inactive, QPalette::ButtonText, textColor);
    SET_IF_UNRESOLVED(QPalette::Inactive, QPalette::Midlight, btnColor.lighter(125));
    SET_IF_UNRESOLVED(QPalette::Inactive, QPalette::Shadow, Qt::black);
    SET_IF_UNRESOLVED(QPalette::Inactive, QPalette::ToolTipBase, result.window().color());
    SET_IF_UNRESOLVED(QPalette::Inactive, QPalette::ToolTipText, result.windowText().color());
    SET_IF_UNRESOLVED(QPalette::Inactive, QPalette::AlternateBase, alternateBase);

    result.setColor(QPalette::Disabled, QPalette::WindowText, textDisabled);

    if (result.midlight() == result.button())
        result.setColor(QPalette::Midlight, btnColor.lighter(110));
    oldStyleSheet = qApp->styleSheet();
}

static void populateDarkSystemBasePalette(QPalette &result)
{
    static QString oldStyleSheet;
    const bool styleSheetChanged = oldStyleSheet != qApp->styleSheet();

    constexpr QColor alternateBase = QColor(0xFF, 0xFF, 0xFF, 0x0F);

    SET_IF_UNRESOLVED(QPalette::Active, QPalette::AlternateBase, alternateBase);

    SET_IF_UNRESOLVED(QPalette::Inactive, QPalette::AlternateBase, alternateBase);

    oldStyleSheet = qApp->styleSheet();
}

/*!
 \internal
 */
void QWindows11Style::polish(QPalette& result)
{
    highContrastTheme = QGuiApplication::styleHints()->colorScheme() == Qt::ColorScheme::Unknown;
    colorSchemeIndex = QGuiApplication::styleHints()->colorScheme() == Qt::ColorScheme::Light ? 0 : 1;

    if (!highContrastTheme && colorSchemeIndex == 0)
        populateLightSystemBasePalette(result);
    else if (!highContrastTheme && colorSchemeIndex == 1)
        populateDarkSystemBasePalette(result);

    const bool styleSheetChanged = false; // so the macro works

    SET_IF_UNRESOLVED(QPalette::Inactive, QPalette::Button, result.button().color());
    SET_IF_UNRESOLVED(QPalette::Inactive, QPalette::Window, result.window().color());
    SET_IF_UNRESOLVED(QPalette::Inactive, QPalette::Light, result.light().color());
    SET_IF_UNRESOLVED(QPalette::Inactive, QPalette::Dark, result.dark().color());
    SET_IF_UNRESOLVED(QPalette::Inactive, QPalette::Accent, result.accent().color());
    SET_IF_UNRESOLVED(QPalette::Inactive, QPalette::Highlight, result.highlight().color());
    SET_IF_UNRESOLVED(QPalette::Inactive, QPalette::HighlightedText, result.highlightedText().color());
    SET_IF_UNRESOLVED(QPalette::Inactive, QPalette::Text, result.text().color());
    SET_IF_UNRESOLVED(QPalette::Inactive, QPalette::WindowText, result.windowText().color());

    if (highContrastTheme)
        result.setColor(QPalette::Active, QPalette::HighlightedText, result.windowText().color());
}

QBrush QWindows11Style::buttonFillBrush(const QStyleOption *option)
{
    if (option->palette.isBrushSet(QPalette::Current, QPalette::Button))
        return option->palette.button();

    const bool isOn = (option->state & QStyle::State_On || option->state & QStyle::State_NoChange);
    QBrush brush = isOn ? option->palette.accent() : option->palette.window();
    if (!isOn && option->state & QStyle::State_AutoRaise)
        return Qt::NoBrush;
    if (option->state & QStyle::State_MouseOver)
        brush.setColor(isOn ? brush.color().lighter(115) : brush.color().darker(107));
    return brush;
}

QColor QWindows11Style::buttonLabelColor(const QStyleOption *option) const
{
    if (option->palette.isBrushSet(QPalette::Current, QPalette::ButtonText))
        return option->palette.buttonText().color();

    const bool isOn = option->state & QStyle::State_On;
    if (option->state & QStyle::State_Sunken)
        return isOn ? winUI3Color(textOnAccentSecondary)
                    : winUI3Color(controlTextSecondary);
    if (!(option->state & QStyle::State_Enabled))
        return isOn ? winUI3Color(textAccentDisabled)
                    : option->palette.buttonText().color();
    return isOn ? winUI3Color(textOnAccentPrimary)
                : option->palette.buttonText().color();
}

void QWindows11Style::drawLineEditFrame(QPainter *p, const QRectF &rect, const QStyleOption *o, bool isEditable) const
{
    const bool isHovered = o->state & State_MouseOver;
    const auto frameCol = highContrastTheme
            ? o->palette.color(isHovered ? QPalette::Accent
                                         : QPalette::ButtonText)
            : winUI3Color(frameColorLight);
    drawRoundedRect(p, rect, frameCol, Qt::NoBrush);

    if (!isEditable)
        return;

    QPainterStateGuard psg(p);
    p->setClipRect(rect.marginsRemoved(QMarginsF(0, rect.height() - 0.5, 0, -1)));
    const bool hasFocus = o->state & State_HasFocus;
    const auto underlineCol = hasFocus
            ? o->palette.color(QPalette::Accent)
            : colorSchemeIndex == 0 ? QColor(0x80, 0x80, 0x80)
                                    : QColor(0xa0, 0xa0, 0xa0);
    const auto penUnderline = QPen(underlineCol, hasFocus ? 2 : 1);
    drawRoundedRect(p, rect, penUnderline, Qt::NoBrush);
}

QColor QWindows11Style::winUI3Color(enum WINUI3Color col) const
{
    return WINUI3Colors[colorSchemeIndex][col];
}

#undef SET_IF_UNRESOLVED

QT_END_NAMESPACE
