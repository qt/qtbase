// Copyright (C) 2022 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#include "qwindows11style_p.h"
#include <qstylehints.h>
#include <private/qstyleanimation_p.h>
#include <private/qstylehelper_p.h>
#include <qstyleoption.h>
#include <qpainter.h>
#include <QGraphicsDropShadowEffect>

#include "qdrawutil.h"
#include <chrono>

QT_BEGIN_NAMESPACE

const static int topLevelRoundingRadius    = 8; //Radius for toplevel items like popups for round corners
const static int secondLevelRoundingRadius = 4; //Radius for second level items like hovered menu item round corners

const static QColor subtleHighlightColor(0x00,0x00,0x00,0x09);                    //Subtle highlight based on alpha used for hovered elements
const static QColor subtlePressedColor(0x00,0x00,0x00,0x06);                      //Subtle highlight based on alpha used for pressed elements
const static QColor frameColorLight(0x00,0x00,0x00,0x0F);                         //Color of frame around flyouts and controls except for Checkbox and Radiobutton
const static QColor frameColorStrong(0x00,0x00,0x00,0x9c);                        //Color of frame around Checkbox and Radiobuttons
const static QColor controlStrongFill = QColor(0x00,0x00,0x00,0x72);              //Color of controls with strong filling such as the right side of a slider
const static QColor controlStrokeSecondary = QColor(0x00,0x00,0x00,0x29);
const static QColor controlStrokePrimary = QColor(0x00,0x00,0x00,0x14);
const static QColor controlFillTertiary = QColor(0xF9,0xF9,0xF9,0x00);            //Color of filled sunken controls
const static QColor controlFillSecondary= QColor(0xF9,0xF9,0xF9,0x80);            //Color of filled hovered controls
const static QColor menuPanelFill = QColor(0xFF,0xFF,0xFF,0xFF);                  //Color of menu panel
const static QColor textOnAccentPrimary = QColor(0xFF,0xFF,0xFF,0xFF);            //Color of text on controls filled in accent color
const static QColor textOnAccentSecondary = QColor(0xFF,0xFF,0xFF,0x7F);          //Color of text of sunken controls in accent color
const static QColor controlTextSecondary = QColor(0x00,0x00,0x00,0x7F);           //Color of text of sunken controls
const static QColor controlStrokeOnAccentSecondary = QColor(0x00,0x00,0x00,0x66); //Color of frame around Buttons in accent color

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
QWindows11Style::QWindows11Style() : QWindowsVistaStyle(*new QWindows11StylePrivate)
{
}

/*!
  \internal
  Constructs a QWindows11Style object.
*/
QWindows11Style::QWindows11Style(QWindows11StylePrivate &dd) : QWindowsVistaStyle(dd)
{
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
                auto center = thumbRect.center();
                const qreal outerRadius = qMin(8.0, (slider->orientation == Qt::Horizontal ? thumbRect.height() / 2.0 : thumbRect.width() / 2.0) - 1);

                thumbRect.setWidth(outerRadius);
                thumbRect.setHeight(outerRadius);
                thumbRect.moveCenter(center);
                QPointF cursorPos = widget ? widget->mapFromGlobal(QCursor::pos()) : QPointF();
                bool isInsideHandle = thumbRect.contains(cursorPos);

                bool oldIsInsideHandle = styleObject->property("_q_insidehandle").toBool();
                int oldState = styleObject->property("_q_stylestate").toInt();
                int oldActiveControls = styleObject->property("_q_stylecontrols").toInt();

                QRectF oldRect = styleObject->property("_q_stylerect").toRect();
                styleObject->setProperty("_q_insidehandle", isInsideHandle);
                styleObject->setProperty("_q_stylestate", int(option->state));
                styleObject->setProperty("_q_stylecontrols", int(option->activeSubControls));
                styleObject->setProperty("_q_stylerect", option->rect);
                if (option->styleObject->property("_q_end_radius").isNull())
                    option->styleObject->setProperty("_q_end_radius", outerRadius * 0.43);

                bool doTransition = ((state & State_Sunken) != (oldState & State_Sunken)
                                     || ((oldIsInsideHandle) != (isInsideHandle))
                                     || oldActiveControls != int(option->activeSubControls));

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
                    leftRect = QRect(rect.left(), rect.top(), (handlePos.x() - rect.left()), rect.height());
                    rightRect = QRect(handlePos.x(), rect.top(), (rect.width() - handlePos.x()), rect.height());
                } else {
                    rect = QRect(rect.center().x() - 2, slrect.top(), 4, slrect.height() - 5);
                    rightRect = QRect(rect.left(), rect.top(), rect.width(), (handlePos.y() - rect.top()));
                    leftRect = QRect(rect.left(), handlePos.y(), rect.width(), (rect.height() - handlePos.y()));
                }

                painter->setPen(QPen(frameColorLight));
                painter->setBrush(option->palette.accent());
                painter->drawRoundedRect(leftRect,1,1);
                painter->setBrush(QBrush(controlStrongFill));
                painter->drawRoundedRect(rightRect,1,1);

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
                QVarLengthArray<QLine, 32> lines;
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
                            lines.append(QLine(pos, tickOffset - 1 - bothOffset,
                                               pos, tickOffset - 1 - bothOffset - tickLength));
                        }

                        if (ticks & QSlider::TicksBelow) {
                            lines.append(QLine(pos, tickOffset + thickness + bothOffset,
                                               pos, tickOffset + thickness + bothOffset + tickLength));
                        }
                    } else {
                        if (ticks & QSlider::TicksAbove) {
                            lines.append(QLine(tickOffset - 1 - bothOffset, pos,
                                               tickOffset - 1 - bothOffset - tickLength, pos));
                        }

                        if (ticks & QSlider::TicksBelow) {
                            lines.append(QLine(tickOffset + thickness + bothOffset, pos,
                                               tickOffset + thickness + bothOffset + tickLength, pos));
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
                if (const auto *slider = qstyleoption_cast<const QStyleOptionSlider *>(option)) {
                    const QRectF rect = proxy()->subControlRect(CC_Slider, option, SC_SliderHandle, widget);
                    const QPointF center = rect.center();

                    const QNumberStyleAnimation* animation = qobject_cast<QNumberStyleAnimation*>(d->animation(option->styleObject));

                    if (animation != nullptr)
                        option->styleObject->setProperty("_q_inner_radius", animation->currentValue());
                    else
                        option->styleObject->setProperty("_q_inner_radius", option->styleObject->property("_q_end_radius"));

                    const qreal outerRadius = qMin(8.0,(slider->orientation == Qt::Horizontal ? rect.height() / 2.0 : rect.width() / 2.0) - 1);
                    const float innerRadius = option->styleObject->property("_q_inner_radius").toFloat();
                    painter->setRenderHint(QPainter::Antialiasing, true);
                    painter->setPen(QPen(QBrush(controlStrokeSecondary),1));
                    painter->setBrush(QBrush(Qt::white));
                    painter->drawEllipse(center, outerRadius, outerRadius);
                    painter->setBrush(option->palette.accent());
                    painter->drawEllipse(center, innerRadius, innerRadius);
                }
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
        if (const QStyleOptionComboBox *sb = qstyleoption_cast<const QStyleOptionComboBox *>(option)) {
            QBrush fillColor = state & State_MouseOver && !(state & State_HasFocus) ? QBrush(subtleHighlightColor) : option->palette.brush(QPalette::Base);
            QRectF rect = option->rect.adjusted(2,2,-2,-2);
            painter->setBrush(fillColor);
            painter->setPen(frameColorLight);
            painter->drawRoundedRect(rect, secondLevelRoundingRadius, secondLevelRoundingRadius);
            if (sub & SC_ComboBoxArrow) {
                QRectF rect = proxy()->subControlRect(CC_ComboBox, option, SC_ComboBoxArrow, widget).adjusted(0, 0, 0, 1);
                painter->setFont(assetFont);
                painter->setPen(sb->palette.text().color());
                painter->drawText(rect,"\uE019", Qt::AlignVCenter | Qt::AlignHCenter);
            }
            if (sb->editable) {
                QColor lineColor = state & State_HasFocus ? option->palette.accent().color() : QColor(0,0,0);
                painter->setPen(QPen(lineColor));
                painter->drawLine(rect.bottomLeft() + QPoint(2,1), rect.bottomRight() + QPoint(-2,1));
                if (state & State_HasFocus)
                    painter->drawLine(rect.bottomLeft() + QPoint(3,2), rect.bottomRight() + QPoint(-3,2));
            }
        }
        break;
#endif // QT_CONFIG(combobox)
    case QStyle::CC_ScrollBar:
        if (const QStyleOptionSlider *scrollbar = qstyleoption_cast<const QStyleOptionSlider *>(option)) {
            QRectF rect = scrollbar->rect;
            QPointF center = rect.center();

            if (scrollbar->orientation == Qt::Vertical && rect.width()>24)
                rect.marginsRemoved(QMargins(0,2,2,2));
            else if (scrollbar->orientation == Qt::Horizontal && rect.height()>24)
                rect.marginsRemoved(QMargins(2,0,2,2));

            if (state & State_MouseOver) {
                if (scrollbar->orientation == Qt::Vertical && rect.width()>24)
                    rect.setWidth(rect.width()/2);
                else if (scrollbar->orientation == Qt::Horizontal && rect.height()>24)
                    rect.setHeight(rect.height()/2);
                rect.moveCenter(center);
                painter->setBrush(scrollbar->palette.base());
                painter->setPen(frameColorLight);
                painter->drawRoundedRect(rect, topLevelRoundingRadius, topLevelRoundingRadius);
            }
            if (sub & SC_ScrollBarSlider) {
                QRectF rect = proxy()->subControlRect(CC_ScrollBar, option, SC_ScrollBarSlider, widget);
                QPointF center = rect.center();
                if (flags & State_MouseOver) {
                    if (scrollbar->orientation == Qt::Vertical)
                        rect.setWidth(rect.width()/2);
                    else
                        rect.setHeight(rect.height()/2);
                }
                else {
                    if (scrollbar->orientation == Qt::Vertical)
                        rect.setWidth(rect.width()/4);
                    else
                        rect.setHeight(rect.height()/4);

                }
                rect.moveCenter(center);
                painter->setBrush(Qt::gray);
                painter->setPen(Qt::NoPen);
                painter->drawRoundedRect(rect, secondLevelRoundingRadius, secondLevelRoundingRadius);
            }
            if (sub & SC_ScrollBarAddLine) {
                QRectF rect = proxy()->subControlRect(CC_ScrollBar, option, SC_ScrollBarAddLine, widget);
                if (flags & State_MouseOver) {
                    painter->setFont(QFont("Segoe Fluent Icons"));
                    painter->setPen(Qt::gray);
                    if (scrollbar->orientation == Qt::Vertical)
                        painter->drawText(rect,"\uEDDC", Qt::AlignVCenter | Qt::AlignHCenter);
                    else
                        painter->drawText(rect,"\uEDDA", Qt::AlignVCenter | Qt::AlignHCenter);
                }
            }
            if (sub & SC_ScrollBarSubLine) {
                QRectF rect = proxy()->subControlRect(CC_ScrollBar, option, SC_ScrollBarSubLine, widget);
                if (flags & State_MouseOver) {
                    painter->setPen(Qt::gray);
                    if (scrollbar->orientation == Qt::Vertical)
                        painter->drawText(rect,"\uEDDB", Qt::AlignVCenter | Qt::AlignHCenter);
                    else
                        painter->drawText(rect,"\uEDD9", Qt::AlignVCenter | Qt::AlignHCenter);
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
            bool doTransition = ((state & State_Sunken) != (oldState & State_Sunken)
                                 || ((state & State_MouseOver) != (oldState & State_MouseOver))
                                 || (state & State_On) != (oldState & State_On));
            if (doTransition) {
                if (element == PE_IndicatorRadioButton) {
                    QNumberStyleAnimation *t = new QNumberStyleAnimation(styleObject);
                    t->setStartValue(styleObject->property("_q_inner_radius").toFloat());
                    t->setEndValue(7.0f);
                    if (option->state & State_Sunken)
                        t->setEndValue(2.0f);
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
    }

    switch (element) {
    case PE_IndicatorCheckBox:
        {
            QNumberStyleAnimation* animation = qobject_cast<QNumberStyleAnimation*>(d->animation(option->styleObject));
            QFontMetrics fm(assetFont);

            QRectF rect = option->rect;
            auto center = QPoint(rect.x() + rect.width() / 2, rect.y() + rect.height() / 2);
            rect.setWidth(15);
            rect.setHeight(15);
            rect.moveCenter(center);

            float clipWidth = animation != nullptr ? animation->currentValue() : 1.0f;
            QRectF clipRect = fm.boundingRect("\uE001");
            clipRect.moveCenter(center);
            clipRect.setLeft(rect.x() + (rect.width() - clipRect.width()) / 2.0);
            clipRect.setWidth(clipWidth * clipRect.width());


            QBrush fillBrush = (option->state & State_On || option->state & State_NoChange) ? option->palette.accent() : option->palette.window();
            if (state & State_MouseOver && (option->state & State_On || option->state & State_NoChange))
                fillBrush.setColor(fillBrush.color().lighter(107));
            else if (state & State_MouseOver && !(option->state & State_On || option->state & State_NoChange))
                fillBrush.setColor(fillBrush.color().darker(107));
            painter->setPen(QPen(frameColorStrong));
            painter->setBrush(fillBrush);
            painter->drawRoundedRect(rect, secondLevelRoundingRadius, secondLevelRoundingRadius, Qt::AbsoluteSize);

            painter->setFont(assetFont);
            painter->setPen(option->palette.highlightedText().color());
            painter->setBrush(option->palette.highlightedText().color());
            if (option->state & State_On)
                painter->drawText(clipRect, Qt::AlignVCenter | Qt::AlignLeft,"\uE001");
            else if (option->state & State_NoChange)
                painter->drawText(rect, Qt::AlignVCenter | Qt::AlignHCenter,"\uE108");
        }
        break;

    case PE_IndicatorRadioButton:
        {
            if (option->styleObject->property("_q_end_radius").isNull())
                option->styleObject->setProperty("_q_end_radius", option->state & State_On ? 4.0f :7.0f);
            QNumberStyleAnimation* animation = qobject_cast<QNumberStyleAnimation*>(d->animation(option->styleObject));
            if (animation != nullptr)
                option->styleObject->setProperty("_q_inner_radius", animation->currentValue());
            else
                option->styleObject->setProperty("_q_inner_radius", option->styleObject->property("_q_end_radius"));
            int innerRadius = option->styleObject->property("_q_inner_radius").toFloat();

            QRect rect = option->rect;
            auto center = QPoint(rect.x() + rect.width() / 2, rect.y() + rect.height() / 2);
            rect.setWidth(15);
            rect.setHeight(15);
            rect.moveCenter(center);
            QRect innerRect = rect;
            innerRect.setWidth(8);
            innerRect.setHeight(8);
            innerRect.moveCenter(center);

            painter->setPen(QPen(frameColorStrong));
            painter->setBrush(option->palette.accent());
            if (option->state & State_MouseOver)
                painter->setBrush(QBrush(option->palette.accent().color().lighter(107)));
            painter->drawEllipse(center, 7, 7);

            painter->setBrush(QBrush(option->palette.window()));
            painter->drawEllipse(center,innerRadius, innerRadius);

            painter->setPen(Qt::NoPen);
            painter->setBrush(QBrush(option->palette.window()));
            if (option->state & State_MouseOver)
                painter->setBrush(QBrush(option->palette.window().color().darker(107)));
            painter->drawEllipse(center,innerRadius, innerRadius);
        }
        break;
    case PE_PanelButtonBevel:{
            QRect rect = option->rect.marginsRemoved(QMargins(2,2,2,2));
            painter->setPen(QPen(controlStrokePrimary));
            if (!(state & (State_Raised)))
                painter->setBrush(controlFillTertiary);
            else if (state & State_MouseOver)
                painter->setBrush(controlFillSecondary);
            else
                painter->setBrush(option->palette.button());
            painter->drawRoundedRect(rect, secondLevelRoundingRadius, secondLevelRoundingRadius);
            painter->setPen(QPen(controlStrokeSecondary));
            if (state & State_Raised)
                painter->drawLine(rect.bottomLeft() + QPoint(2,1), rect.bottomRight() + QPoint(-2,1));
        }
        break;
    case PE_FrameDefaultButton:
        painter->setPen(option->palette.accent().color());
        painter->setBrush(Qt::NoBrush);
        painter->drawRoundedRect(option->rect, secondLevelRoundingRadius, secondLevelRoundingRadius);
        break;
    case QStyle::PE_FrameMenu:
        break;
    case QStyle::PE_PanelMenu: {
        QRect rect = option->rect;
        QPen pen(frameColorLight);
        painter->save();
        painter->setPen(pen);
        painter->setBrush(QBrush(menuPanelFill));
        painter->setRenderHint(QPainter::Antialiasing);
        painter->drawRoundedRect(rect.marginsRemoved(QMargins(2,2,12,2)), topLevelRoundingRadius, topLevelRoundingRadius);
        painter->restore();
        break;
    }
    case PE_PanelLineEdit:
        if (const auto *panel = qstyleoption_cast<const QStyleOptionFrame *>(option)) {
            QBrush fillColor = state & State_MouseOver && !(state & State_HasFocus) ? QBrush(subtleHighlightColor) : option->palette.brush(QPalette::Base);
            painter->setBrush(fillColor);
            painter->setPen(Qt::NoPen);
            painter->drawRoundedRect(option->rect, secondLevelRoundingRadius, secondLevelRoundingRadius);
            if (panel->lineWidth > 0)
                proxy()->drawPrimitive(PE_FrameLineEdit, panel, painter, widget);
        }
        break;
    case PE_FrameLineEdit: {
        painter->setBrush(Qt::NoBrush);
        painter->setPen(QPen(frameColorLight));
        painter->drawRoundedRect(option->rect, secondLevelRoundingRadius, secondLevelRoundingRadius);
        QRegion clipRegion = option->rect;
        clipRegion -= option->rect.adjusted(2, 2, -2, -2);
        painter->setClipRegion(clipRegion);
        QColor lineColor = state & State_HasFocus ? option->palette.accent().color() : QColor(0,0,0);
        painter->setPen(QPen(lineColor));
        painter->drawLine(option->rect.bottomLeft() + QPoint(1,0), option->rect.bottomRight() + QPoint(-1,0));
        if (state & State_HasFocus)
            painter->drawLine(option->rect.bottomLeft() + QPoint(2,1), option->rect.bottomRight() + QPoint(-2,1));
    }
        break;
    case PE_Frame: {
        if (const auto *frame = qstyleoption_cast<const QStyleOptionFrame *>(option)) {
            if (frame->frameShape == QFrame::NoFrame)
                break;
            QRect rect = option->rect.adjusted(2,2,-2,-2);
            if (widget && widget->inherits("QComboBoxPrivateContainer")) {
                painter->setPen(Qt::NoPen);
                painter->setBrush(menuPanelFill);
                painter->drawRoundedRect(rect, secondLevelRoundingRadius, secondLevelRoundingRadius);

            }
            painter->setBrush(option->palette.base());
            painter->setPen(QPen(frameColorLight));
            painter->drawRoundedRect(rect, secondLevelRoundingRadius, secondLevelRoundingRadius);

            if (widget && widget->inherits("QTextEdit")) {
                QRegion clipRegion = option->rect;
                QColor lineColor = state & State_HasFocus ? option->palette.accent().color() : QColor(0,0,0,255);
                painter->setPen(QPen(lineColor));
                painter->drawLine(rect.bottomLeft() + QPoint(1,1), rect.bottomRight() + QPoint(-1,1));
                if (state & State_HasFocus)
                    painter->drawLine(rect.bottomLeft() + QPoint(2,2), rect.bottomRight() + QPoint(-2,2));
            }
        }
        break;
    }
    case QStyle::PE_PanelItemViewRow:
        if (const QStyleOptionViewItem *vopt = qstyleoption_cast<const QStyleOptionViewItem *>(option)) {
            if ((vopt->state & State_Selected || vopt->state & State_MouseOver) && vopt->showDecorationSelected) {
                painter->setBrush(subtleHighlightColor);
                painter->setPen(Qt::NoPen);
                painter->drawRoundedRect(vopt->rect.marginsRemoved(QMargins(0,2,-2,2)),2,2);
                int offset = (widget && widget->inherits("QTreeView")) ? 2 : 0;
                if (vopt->viewItemPosition == QStyleOptionViewItem::Beginning && option->state & State_Selected) {
                    painter->setPen(QPen(option->palette.accent().color()));
                    painter->drawLine(option->rect.x(),option->rect.y()+offset,option->rect.x(),option->rect.y() + option->rect.height()-2);
                    painter->drawLine(option->rect.x()+1,option->rect.y()+2,option->rect.x()+1,option->rect.y() + option->rect.height()-2);
                }
            }
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
    QRect rect(option->rect);
    State flags = option->state;

    painter->save();
    painter->setRenderHint(QPainter::Antialiasing);
    switch (element) {
    case CE_ToolButtonLabel:
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
                if (toolbutton->state & State_Raised)
                    painter->setPen(QPen(toolbutton->palette.buttonText().color()));
                else
                    painter->setPen(QPen(controlTextSecondary));
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
                    if (toolbutton->state & State_Raised)
                        painter->setPen(QPen(toolbutton->palette.buttonText().color()));
                    else
                        painter->setPen(QPen(controlTextSecondary));
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
        break;
    case QStyle::CE_ProgressBarGroove:{
        if (const QStyleOptionProgressBar* progbaropt = qstyleoption_cast<const QStyleOptionProgressBar*>(option)) {
            const QProgressBar* bar = qobject_cast<const QProgressBar*>(widget);
            QRectF rect = subElementRect(SE_ProgressBarContents, progbaropt, widget);
            QPointF center = rect.center();
            if (bar->orientation() & Qt::Horizontal) {
                rect.setHeight(1);
                rect.moveTop(center.y());
            } else {
                rect.setWidth(1);
                rect.moveLeft(center.x());
            }
            painter->setPen(Qt::NoPen);
            painter->setBrush(Qt::gray);
            painter->drawRoundedRect(rect, secondLevelRoundingRadius, secondLevelRoundingRadius);
        }
        break;
    }
    case QStyle::CE_ProgressBarContents:
        if (const QStyleOptionProgressBar* progbaropt = qstyleoption_cast<const QStyleOptionProgressBar*>(option)) {
            const QProgressBar* bar = qobject_cast<const QProgressBar*>(widget);
            QRectF rect = subElementRect(SE_ProgressBarContents, progbaropt, widget);
            QRectF originalRect = rect;
            QPointF center = rect.center();
            bool isIndeterminate = progbaropt->maximum == 0 && progbaropt->minimum == 0;
            float fillPercentage = 0;

            if (!isIndeterminate) {
                fillPercentage = ((float(progbaropt->progress) - float(progbaropt->minimum)) / (float(progbaropt->maximum) - float(progbaropt->minimum)));
                if (bar->orientation() == Qt::Horizontal) {
                    rect.setHeight(4);
                    rect.moveTop(center.y() - 1.5);
                    rect.setWidth(rect.width() * fillPercentage);
                } else {
                    float oldHeight = rect.height();
                    rect.setWidth(4);
                    rect.moveLeft(center.x() - 1.5);
                    rect.moveTop(oldHeight * (1.0f - fillPercentage));
                    rect.setHeight(oldHeight * fillPercentage);
                }
            } else {
                auto elapsedTime = std::chrono::time_point_cast<std::chrono::milliseconds>(std::chrono::system_clock::now());
                fillPercentage = (elapsedTime.time_since_epoch().count() % 5000)/(5000.0f*0.75);
                if (bar->orientation() == Qt::Horizontal) {
                    float barBegin = qMin(qMax(fillPercentage-0.25,0.0) * rect.width(), float(rect.width()));
                    float barEnd = qMin(fillPercentage * rect.width(), float(rect.width()));
                    rect = QRect(QPoint(rect.left() + barBegin, rect.top()), QPoint(rect.left() + barEnd, rect.bottom()));
                    rect.setHeight(4);
                    rect.moveTop(center.y() - 1.5);
                } else {
                    float barBegin = qMin(qMax(fillPercentage-0.25,0.0) * rect.height(), float(rect.height()));
                    float barEnd = qMin(fillPercentage * rect.height(), float(rect.height()));
                    rect = QRect(QPoint(rect.left(), rect.bottom() - barEnd), QPoint(rect.right(), rect.bottom() - barBegin));
                    rect.setWidth(4);
                    rect.moveLeft(center.x() - 1.5);
                }
                const_cast<QWidget*>(widget)->update();
            }
            if (progbaropt->invertedAppearance && bar->orientation() == Qt::Horizontal)
                rect.moveLeft(originalRect.width() * (1.0 - fillPercentage));
            else if (progbaropt->invertedAppearance && bar->orientation() == Qt::Vertical)
                rect.moveBottom(originalRect.height() * fillPercentage);
            painter->setPen(Qt::NoPen);
            painter->setBrush(progbaropt->palette.accent());
            painter->drawRoundedRect(rect, secondLevelRoundingRadius, secondLevelRoundingRadius);
        }
        break;
    case QStyle::CE_ProgressBarLabel:
        if (const QStyleOptionProgressBar* progbaropt = qstyleoption_cast<const QStyleOptionProgressBar*>(option)) {
            QRect rect = subElementRect(SE_ProgressBarLabel, progbaropt, widget);
            painter->setPen(progbaropt->palette.text().color());
            painter->drawText(rect, progbaropt->text,Qt::AlignVCenter|Qt::AlignLeft);
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
                if (btn->direction == Qt::LeftToRight)
                    textRect = textRect.adjusted(0, 0, -indicatorSize, 0);
                else
                    textRect = textRect.adjusted(indicatorSize, 0, 0, 0);
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


            if (btn->state & State_Sunken)
                painter->setPen(flags & State_On ? QPen(textOnAccentSecondary) : QPen(controlTextSecondary));
            else
                painter->setPen(flags & State_On ? QPen(textOnAccentPrimary) : QPen(btn->palette.buttonText().color()));
            proxy()->drawItemText(painter, textRect, tf, option->palette,btn->state & State_Enabled, btn->text);
        }
        break;
    case CE_PushButtonBevel:
        if (const QStyleOptionButton *btn = qstyleoption_cast<const QStyleOptionButton *>(option))  {
            if (btn->features.testFlag(QStyleOptionButton::Flat)) {
                painter->setPen(Qt::NoPen);
                if (flags & (State_Sunken | State_On)) {
                    painter->setBrush(subtlePressedColor);
                    painter->drawRoundedRect(rect, secondLevelRoundingRadius, secondLevelRoundingRadius);
                }
                else if (flags & State_MouseOver) {
                    painter->setBrush(subtleHighlightColor);
                    painter->drawRoundedRect(rect, secondLevelRoundingRadius, secondLevelRoundingRadius);
                }
            } else {
                QRectF rect = btn->rect.marginsRemoved(QMargins(2,2,2,2));
                painter->setPen(btn->features.testFlag(QStyleOptionButton::DefaultButton) ? QPen(option->palette.accent().color()) : QPen(controlStrokePrimary));
                if (flags & (State_Sunken))
                    painter->setBrush(flags & State_On ? option->palette.accent().color().lighter(120) : controlFillTertiary);
                else if (flags & State_MouseOver)
                    painter->setBrush(flags & State_On ? option->palette.accent().color().lighter(110) : controlFillSecondary);
                else
                    painter->setBrush(flags & State_On ? option->palette.accent() : option->palette.button());
                painter->drawRoundedRect(rect, secondLevelRoundingRadius, secondLevelRoundingRadius);
                painter->setPen(btn->features.testFlag(QStyleOptionButton::DefaultButton) ? QPen(controlStrokeOnAccentSecondary) : QPen(controlStrokeSecondary));
                if (flags & State_Raised)
                    painter->drawLine(rect.bottomLeft() + QPointF(4.0,0.5), rect.bottomRight() + QPointF(-4,0.5));
            }
        }
        break;
    case CE_MenuBarItem:
        if (const auto *mbi = qstyleoption_cast<const QStyleOptionMenuItem *>(option))  {
            bool active = mbi->state & State_Selected;
            bool hasFocus = mbi->state & State_HasFocus;
            bool down = mbi->state & State_Sunken;
            QStyleOptionMenuItem newMbi = *mbi;
            if (active || hasFocus) {
                if (active && down)
                    painter->setBrushOrigin(painter->brushOrigin() + QPoint(1, 1));
                if (active && hasFocus) {
                    painter->setBrush(subtleHighlightColor);
                    painter->setPen(Qt::NoPen);
                    QRect rect = mbi->rect.marginsRemoved(QMargins(2,2,2,2));
                    painter->drawRoundedRect(rect,secondLevelRoundingRadius,secondLevelRoundingRadius,Qt::AbsoluteSize);
                }
            }
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

            QBrush fill = (act == true) ? QBrush(subtleHighlightColor) : menuitem->palette.brush(QPalette::Button);
            painter->setBrush(fill);
            painter->setPen(Qt::NoPen);
            QRect rect = menuitem->rect;
            rect = rect.marginsRemoved(QMargins(2,2,2,2));
            if (act)
                painter->drawRoundedRect(rect,secondLevelRoundingRadius,secondLevelRoundingRadius,Qt::AbsoluteSize);

            if (menuitem->menuItemType == QStyleOptionMenuItem::Separator){
                int yoff = 4;
                painter->setPen(frameColorLight);
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
                QPixmap pixmap;
                if (checked)
                    pixmap = menuitem->icon.pixmap(proxy()->pixelMetric(PM_SmallIconSize, option, widget), mode, QIcon::On);
                else
                    pixmap = menuitem->icon.pixmap(proxy()->pixelMetric(PM_SmallIconSize, option, widget), mode);
                QRect pmr(QPoint(0, 0), pixmap.deviceIndependentSize().toSize());
                pmr.moveCenter(vCheckRect.center());
                painter->setPen(menuitem->palette.text().color());
                painter->drawPixmap(pmr.topLeft(), pixmap);
            } else if (checked) {
                QStyleOptionMenuItem newMi = *menuitem;
                newMi.state = State_None;
                if (!dis)
                    newMi.state |= State_Enabled;
                if (act)
                    newMi.state |= State_On | State_Selected;
                newMi.rect = visualRect(option->direction, menuitem->rect, QRect(menuitem->rect.x() + QWindowsStylePrivate::windowsItemFrame,
                                                                              menuitem->rect.y() + QWindowsStylePrivate::windowsItemFrame,
                                                                              checkcol - 2 * QWindowsStylePrivate::windowsItemFrame,
                                                                              menuitem->rect.height() - 2 * QWindowsStylePrivate::windowsItemFrame));

                QColor discol;
                if (dis) {
                    discol = menuitem->palette.text().color();
                    painter->setPen(discol);
                }
                int xm = int(QWindowsStylePrivate::windowsItemFrame) + checkcol / 4 + int(QWindowsStylePrivate::windowsItemHMargin);
                int xpos = menuitem->rect.x() + xm;
                QRect textRect(xpos, y + QWindowsStylePrivate::windowsItemVMargin,
                               w - xm - QWindowsStylePrivate::windowsRightBorder - tab + 1, h - 2 * QWindowsStylePrivate::windowsItemVMargin);
                QRect vTextRect = visualRect(option->direction, menuitem->rect, textRect);

                    painter->save();
                    painter->setFont(assetFont);
                    int text_flags = Qt::AlignVCenter | Qt::TextShowMnemonic | Qt::TextDontClip | Qt::TextSingleLine;
                    if (!proxy()->styleHint(SH_UnderlineShortcut, menuitem, widget))
                        text_flags |= Qt::TextHideMnemonic;
                    text_flags |= Qt::AlignLeft;

                    const QString textToDraw("\uE001");
                    painter->setPen(option->palette.text().color());
                    painter->drawText(vTextRect, text_flags, textToDraw);
                    painter->restore();
            }
            painter->setPen(act ? menuitem->palette.highlightedText().color() : menuitem->palette.buttonText().color());

            QColor discol;
            if (dis) {
                discol = menuitem->palette.text().color();
                painter->setPen(discol);
            }

            int xm = int(QWindowsStylePrivate::windowsItemFrame) + checkcol + int(QWindowsStylePrivate::windowsItemHMargin);
            int xpos = menuitem->rect.x() + xm;
            QRect textRect(xpos, y + QWindowsStylePrivate::windowsItemVMargin,
                           w - xm - QWindowsStylePrivate::windowsRightBorder - tab + 1, h - 2 * QWindowsStylePrivate::windowsItemVMargin);
            QRect vTextRect = visualRect(option->direction, menuitem->rect, textRect);
            QStringView s(menuitem->text);
            if (!s.isEmpty()) {                     // draw text
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
                        painter->setPen(discol);
                    }
                    painter->setPen(menuitem->palette.color(QPalette::Disabled, QPalette::Text));
                    painter->drawText(vShortcutRect, text_flags, textToDraw);
                    s = s.left(t);
                }
                QFont font = menuitem->font;
                if (menuitem->menuItemType == QStyleOptionMenuItem::DefaultItem)
                    font.setBold(true);
                painter->setFont(font);
                const QString textToDraw = s.left(t).toString();
                painter->setPen(discol);
                painter->drawText(vTextRect, text_flags, textToDraw);
                painter->restore();
            }
            if (menuitem->menuItemType == QStyleOptionMenuItem::SubMenu) {// draw sub menu arrow
                int dim = (h - 2 * QWindowsStylePrivate::windowsItemFrame) / 2;
                xpos = x + w - QWindowsStylePrivate::windowsArrowHMargin - QWindowsStylePrivate::windowsItemFrame - dim;
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
                const QString textToDraw("\uE013");
                painter->setPen(option->palette.text().color());
                painter->drawText(vSubMenuRect, text_flags, textToDraw);
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
            painter->setPen(frameColorLight);
            if (header->position == QStyleOptionHeader::OnlyOneSection) {
                break;
            }
            else if (header->position == QStyleOptionHeader::Beginning) {
                painter->drawLine(option->rect.topRight(), option->rect.bottomRight());
            }
            else if (header->position == QStyleOptionHeader::End) {
                painter->drawLine(option->rect.topLeft(), option->rect.bottomLeft());
            } else {
                painter->drawLine(option->rect.topRight(), option->rect.bottomRight());
                painter->drawLine(option->rect.topLeft(), option->rect.bottomLeft());
            }
            painter->drawLine(option->rect.bottomLeft(), option->rect.bottomRight());
        }
        break;
    }
    case QStyle::CE_ItemViewItem: {
        if (const QStyleOptionViewItem *vopt = qstyleoption_cast<const QStyleOptionViewItem *>(option)) {
            QRect checkRect = proxy()->subElementRect(SE_ItemViewItemCheckIndicator, vopt, widget);
            QRect iconRect = proxy()->subElementRect(SE_ItemViewItemDecoration, vopt, widget);
            QRect textRect = proxy()->subElementRect(SE_ItemViewItemText, vopt, widget);

            QRect rect = vopt->rect;

            painter->setPen(frameColorLight);
            if (vopt->viewItemPosition == QStyleOptionViewItem::OnlyOne || vopt->viewItemPosition == QStyleOptionViewItem::Invalid) {
            }
            else if (vopt->viewItemPosition == QStyleOptionViewItem::Beginning) {
                painter->drawLine(option->rect.topRight(), option->rect.bottomRight());
            }
            else if (vopt->viewItemPosition == QStyleOptionViewItem::End) {
                painter->drawLine(option->rect.topLeft(), option->rect.bottomLeft());
            } else {
                painter->drawLine(option->rect.topRight(), option->rect.bottomRight());
                painter->drawLine(option->rect.topLeft(), option->rect.bottomLeft());
            }
            painter->setPen(QPen(option->palette.buttonText().color()));

            bool isTreeView = widget && widget->inherits("QTreeView");
            if ((vopt->state & State_Selected || vopt->state & State_MouseOver) && !(isTreeView && vopt->state & State_MouseOver) && vopt->showDecorationSelected) {
                painter->setBrush(subtleHighlightColor);
                painter->setPen(Qt::NoPen);

                if (vopt->viewItemPosition == QStyleOptionViewItem::OnlyOne || vopt->viewItemPosition == QStyleOptionViewItem::Invalid) {
                    painter->drawRoundedRect(vopt->rect.marginsRemoved(QMargins(2,2,2,2)),secondLevelRoundingRadius,secondLevelRoundingRadius);
                }
                else if (vopt->viewItemPosition == QStyleOptionViewItem::Beginning) {
                    painter->drawRoundedRect(rect.marginsRemoved(QMargins(2,2,0,2)),secondLevelRoundingRadius,secondLevelRoundingRadius);
                }
                else if (vopt->viewItemPosition == QStyleOptionViewItem::End) {
                    painter->drawRoundedRect(vopt->rect.marginsRemoved(QMargins(0,2,2,2)),secondLevelRoundingRadius,secondLevelRoundingRadius);
                } else {
                    painter->drawRect(vopt->rect.marginsRemoved(QMargins(0,2,0,2)));
                }
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

            painter->setPen(QPen(option->palette.buttonText().color()));
            d->viewItemDrawText(painter, vopt, textRect);
            if (vopt->state & State_Selected && (vopt->viewItemPosition == QStyleOptionViewItem::Beginning || vopt->viewItemPosition == QStyleOptionViewItem::OnlyOne || vopt->viewItemPosition == QStyleOptionViewItem::Invalid)) {
                if (widget && widget->inherits("QListView") && qobject_cast<const QListView*>(widget)->viewMode() != QListView::IconMode) {
                    painter->setPen(QPen(vopt->palette.accent().color()));
                    painter->drawLine(option->rect.x(),option->rect.y()+2,option->rect.x(),option->rect.y() + option->rect.height()-2);
                    painter->drawLine(option->rect.x()+1,option->rect.y()+2,option->rect.x()+1,option->rect.y() + option->rect.height()-2);
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
    case QStyle::SH_ItemView_ShowDecorationSelected:
        return 1;
    default:
        return QWindowsVistaStyle::styleHint(hint, opt, widget, returnData);
    }
}

/*!
 \internal
 */
QSize QWindows11Style::sizeFromContents(ContentsType type, const QStyleOption *option,
                                           const QSize &size, const QWidget *widget) const
{
    QSize contentSize(size);

    switch (type) {

    case CT_Menu:
        contentSize += QSize(10, 0);
        break;

#if QT_CONFIG(menubar)
    case CT_MenuBarItem:
        if (!contentSize.isEmpty())
            contentSize += QSize(QWindowsVistaStylePrivate::windowsItemHMargin * 5 + 1 + 16, 5 + 16);
        break;
#endif

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
        return 16;
    case QStyle::PM_SliderLength:
        res = int(QStyleHelper::dpiScaled(16, option));
        break;

    default:
        res = QWindowsVistaStyle::pixelMetric(metric, option, widget);
    }

    return res;
}
void QWindows11Style::polish(QWidget* widget)
{
    QWindowsVistaStyle::polish(widget);
    if (widget->inherits("QScrollBar") || widget->inherits("QComboBoxPrivateContainer") || widget->inherits("QMenu")) {
        bool wasCreated = widget->testAttribute(Qt::WA_WState_Created);
        bool layoutDirection = widget->testAttribute(Qt::WA_RightToLeft);
        widget->setAttribute(Qt::WA_OpaquePaintEvent,false);
        widget->setAttribute(Qt::WA_TranslucentBackground);
        widget->setWindowFlag(Qt::FramelessWindowHint);
        widget->setWindowFlag(Qt::NoDropShadowWindowHint);
        widget->setAttribute(Qt::WA_RightToLeft, layoutDirection);
        widget->setAttribute(Qt::WA_WState_Created, wasCreated);
        auto pal = widget->palette();
        pal.setColor(widget->backgroundRole(), Qt::transparent);
        widget->setPalette(pal);
    }
    if (widget->inherits("QComboBoxPrivateContainer") || widget->inherits("QMenu")) {
        QGraphicsDropShadowEffect* dropshadow = new QGraphicsDropShadowEffect(widget);
        dropshadow->setBlurRadius(3);
        dropshadow->setXOffset(3);
        dropshadow->setYOffset(3);
        widget->setGraphicsEffect(dropshadow);
    }
    if (widget->inherits("QComboBox")) {

        QComboBox* cb = qobject_cast<QComboBox*>(widget);
        if (cb->isEditable()) {
            QLineEdit *le = cb->lineEdit();
            le->setFrame(false);
        }
    }
    if (widget->inherits("QGraphicsView") && !widget->inherits("QTextEdit")) {
        QPalette pal = widget->palette();
        pal.setColor(QPalette::Base, pal.window().color());
        widget->setPalette(pal);
    }
    else if (widget->inherits("QAbstractScrollArea")) {
        if (auto scrollarea = qobject_cast<QAbstractScrollArea*>(widget)) {
            QPalette pal = widget->palette();
            pal.setColor(scrollarea->viewport()->backgroundRole(), Qt::transparent);
            scrollarea->viewport()->setPalette(pal);
        }
    }
}

QT_END_NAMESPACE
