// Copyright (C) 2022 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#include "qwindows11style_p.h"
#include <qstylehints.h>
#include <private/qstyleanimation_p.h>
#include <private/qstylehelper_p.h>
#include <private/qapplication_p.h>
#include <qstyleoption.h>
#include <qpainter.h>
#include <QGraphicsDropShadowEffect>
#include <QLatin1StringView>
#include <QtWidgets/qcombobox.h>
#include <QtWidgets/qcommandlinkbutton.h>
#include <QtWidgets/qgraphicsview.h>
#include <QtWidgets/qlistview.h>
#include <QtWidgets/qmenu.h>
#if QT_CONFIG(mdiarea)
#include <QtWidgets/qmdiarea.h>
#endif
#include <QtWidgets/qtextedit.h>
#include <QtWidgets/qtreeview.h>
#include <QtWidgets/qdatetimeedit.h>

#include "qdrawutil.h"
#include <chrono>

QT_BEGIN_NAMESPACE

const static int topLevelRoundingRadius    = 8; //Radius for toplevel items like popups for round corners
const static int secondLevelRoundingRadius = 4; //Radius for second level items like hovered menu item round corners
constexpr QLatin1StringView originalWidthProperty("_q_windows11_style_original_width");

enum WINUI3Color {
    subtleHighlightColor,             //Subtle highlight based on alpha used for hovered elements
    subtlePressedColor,               //Subtle highlight based on alpha used for pressed elements
    frameColorLight,                  //Color of frame around flyouts and controls except for Checkbox and Radiobutton
    frameColorStrong,                 //Color of frame around Checkbox and Radiobuttons
    controlStrongFill,                //Color of controls with strong filling such as the right side of a slider
    controlStrokeSecondary,
    controlStrokePrimary,
    controlFillTertiary,              //Color of filled sunken controls
    controlFillSecondary,             //Color of filled hovered controls
    menuPanelFill,                    //Color of menu panel
    textOnAccentPrimary,              //Color of text on controls filled in accent color
    textOnAccentSecondary,            //Color of text of sunken controls in accent color
    controlTextSecondary,             //Color of text of sunken controls
    controlStrokeOnAccentSecondary,   //Color of frame around Buttons in accent color
    controlFillSolid,                 //Color for solid fill
    surfaceStroke,                    //Color of MDI window frames
    controlAccentDisabled,
    textAccentDisabled
};

const static QColor WINUI3ColorsLight [] {
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
};

const static QColor WINUI3ColorsDark[] {
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
};

const static QColor* WINUI3Colors[] {
    WINUI3ColorsLight,
    WINUI3ColorsDark
};

const QColor shellCloseButtonColor(0xC4,0x2B,0x1C,0xFF); //Color of close Button in Titlebar

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
    highContrastTheme = QGuiApplicationPrivate::styleHints->colorScheme() == Qt::ColorScheme::Unknown;
    colorSchemeIndex = QGuiApplicationPrivate::styleHints->colorScheme() == Qt::ColorScheme::Light ? 0 : 1;
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

                bool doTransition = (((state & State_Sunken) != (oldState & State_Sunken)
                                     || ((oldIsInsideHandle) != (isInsideHandle))
                                     || oldActiveControls != int(option->activeSubControls))
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
            if (sb->frame && (sub & SC_SpinBoxFrame)) {
                painter->save();
                QRegion clipRegion = option->rect;
                clipRegion -= option->rect.adjusted(2, 2, -2, -2);
                painter->setClipRegion(clipRegion);
                QColor lineColor = state & State_HasFocus ? option->palette.accent().color() : QColor(0,0,0);
                painter->setPen(QPen(lineColor));
                painter->drawLine(option->rect.bottomLeft() + QPointF(7,-0.5), option->rect.bottomRight() + QPointF(-7,-0.5));
                painter->restore();
            }
            QRectF frameRect = option->rect;
            frameRect.adjust(0.5,0.5,-0.5,-0.5);
            QBrush fillColor = option->palette.brush(QPalette::Base);
            painter->setBrush(fillColor);
            painter->setPen(QPen(highContrastTheme == true ? sb->palette.buttonText().color() : WINUI3Colors[colorSchemeIndex][frameColorLight]));
            painter->drawRoundedRect(frameRect.adjusted(2,2,-2,-2), secondLevelRoundingRadius, secondLevelRoundingRadius);
            QPoint mousePos = widget ? widget->mapFromGlobal(QCursor::pos()) : QPoint();
            QColor hoverColor = WINUI3Colors[colorSchemeIndex][subtleHighlightColor];
            if (sub & SC_SpinBoxEditField) {
                QRect rect = proxy()->subControlRect(CC_SpinBox, option, SC_SpinBoxEditField, widget).adjusted(0, 0, 0, 1);
                if (rect.contains(mousePos) && !(state & State_HasFocus)) {
                    QBrush fillColor = QBrush(WINUI3Colors[colorSchemeIndex][subtleHighlightColor]);
                    painter->setBrush(fillColor);
                    painter->setPen(Qt::NoPen);
                    painter->drawRoundedRect(option->rect.adjusted(2,2,-2,-2), secondLevelRoundingRadius, secondLevelRoundingRadius);
                }
            }
            if (sub & SC_SpinBoxUp) {
                QRect rect = proxy()->subControlRect(CC_SpinBox, option, SC_SpinBoxUp, widget).adjusted(0, 0, 0, 1);
                if (rect.contains(mousePos)) {
                    painter->setPen(Qt::NoPen);
                    painter->setBrush(QBrush(hoverColor));
                    painter->drawRoundedRect(rect.adjusted(1,1,-1,-1),secondLevelRoundingRadius, secondLevelRoundingRadius);
                }
                painter->save();
                painter->translate(rect.center());
                painter->translate(-rect.center());
                painter->setFont(assetFont);
                painter->setPen(sb->palette.buttonText().color());
                painter->setBrush(Qt::NoBrush);
                painter->drawText(rect,"\uE018", Qt::AlignVCenter | Qt::AlignHCenter);
                painter->restore();
            }
            if (sub & SC_SpinBoxDown) {
                QRect rect = proxy()->subControlRect(CC_SpinBox, option, SC_SpinBoxDown, widget);
                if (rect.contains(mousePos)) {
                    painter->setPen(Qt::NoPen);
                    painter->setBrush(QBrush(hoverColor));
                    painter->drawRoundedRect(rect.adjusted(1,1,-1,-1), secondLevelRoundingRadius, secondLevelRoundingRadius);
                }
                painter->save();
                painter->translate(rect.center());
                painter->translate(-rect.center());
                painter->setFont(assetFont);
                painter->setPen(sb->palette.buttonText().color());
                painter->setBrush(Qt::NoBrush);
                painter->drawText(rect,"\uE019", Qt::AlignVCenter | Qt::AlignHCenter);
                painter->restore();
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
                    leftRect = QRect(rect.left(), rect.top(), (handlePos.x() - rect.left()), rect.height());
                    rightRect = QRect(handlePos.x(), rect.top(), (rect.width() - handlePos.x()), rect.height());
                } else {
                    rect = QRect(rect.center().x() - 2, slrect.top(), 4, slrect.height() - 5);
                    rightRect = QRect(rect.left(), rect.top(), rect.width(), (handlePos.y() - rect.top()));
                    leftRect = QRect(rect.left(), handlePos.y(), rect.width(), (rect.height() - handlePos.y()));
                }

                painter->setPen(Qt::NoPen);
                painter->setBrush(option->palette.accent());
                painter->drawRoundedRect(leftRect,1,1);
                painter->setBrush(QBrush(WINUI3Colors[colorSchemeIndex][controlStrongFill]));
                painter->drawRoundedRect(rightRect,1,1);

                painter->setPen(QPen(highContrastTheme == true ? slider->palette.buttonText().color() : WINUI3Colors[colorSchemeIndex][frameColorLight]));
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
                    painter->setPen(Qt::NoPen);
                    painter->setBrush(QBrush(WINUI3Colors[colorSchemeIndex][controlFillSolid]));
                    painter->drawEllipse(center, outerRadius, outerRadius);
                    painter->setBrush(option->palette.accent());
                    painter->drawEllipse(center, innerRadius, innerRadius);

                    painter->setPen(QPen(WINUI3Colors[colorSchemeIndex][controlStrokeSecondary]));
                    painter->setBrush(Qt::NoBrush);
                    painter->drawEllipse(center, outerRadius + 0.5, outerRadius + 0.5);
                    painter->drawEllipse(center, innerRadius + 0.5, innerRadius + 0.5);
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
        if (const QStyleOptionComboBox *combobox = qstyleoption_cast<const QStyleOptionComboBox *>(option)) {
            QBrush fillColor = combobox->palette.brush(QPalette::Base);
            QRectF rect = option->rect.adjusted(2,2,-2,-2);
            painter->setBrush(fillColor);
            painter->setPen(Qt::NoPen);
            painter->drawRoundedRect(rect, secondLevelRoundingRadius, secondLevelRoundingRadius);

            // In case the QComboBox is hovered overdraw the background with a alpha mask to
            // highlight the QComboBox.
            if (state & State_MouseOver) {
                fillColor = QBrush(WINUI3Colors[colorSchemeIndex][subtleHighlightColor]);
                painter->setBrush(fillColor);
                painter->setPen(Qt::NoPen);
                painter->drawRoundedRect(rect, secondLevelRoundingRadius, secondLevelRoundingRadius);
            }

            rect.adjust(0.5,0.5,-0.5,-0.5);
            painter->setBrush(Qt::NoBrush);
            painter->setPen(highContrastTheme == true ? combobox->palette.buttonText().color() : WINUI3Colors[colorSchemeIndex][frameColorLight]);
            painter->drawRoundedRect(rect, secondLevelRoundingRadius, secondLevelRoundingRadius);
            if (sub & SC_ComboBoxArrow) {
                QRectF rect = proxy()->subControlRect(CC_ComboBox, option, SC_ComboBoxArrow, widget).adjusted(-4, 0, -4, 1);
                painter->setFont(assetFont);
                painter->setPen(combobox->palette.text().color());
                painter->drawText(rect,"\uE019", Qt::AlignVCenter | Qt::AlignHCenter);
            }
            if (combobox->editable) {
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
                painter->setPen(Qt::NoPen);
                painter->drawRoundedRect(rect, topLevelRoundingRadius, topLevelRoundingRadius);

                painter->setBrush(Qt::NoBrush);
                painter->setPen(WINUI3Colors[colorSchemeIndex][frameColorLight]);
                painter->drawRoundedRect(rect.marginsRemoved(QMarginsF(0.5,0.5,0.5,0.5)), topLevelRoundingRadius + 0.5, topLevelRoundingRadius + 0.5);
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
                        rect.setWidth(1);
                    else
                        rect.setHeight(1);

                }
                rect.moveCenter(center);
                painter->setBrush(Qt::gray);
                painter->setPen(Qt::NoPen);
                painter->drawRoundedRect(rect, secondLevelRoundingRadius, secondLevelRoundingRadius);
            }
            if (sub & SC_ScrollBarAddLine) {
                QRectF rect = proxy()->subControlRect(CC_ScrollBar, option, SC_ScrollBarAddLine, widget);
                if (flags & State_MouseOver) {
                    painter->setFont(QFont("Segoe Fluent Icons",6));
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
                    const QString textToDraw("\uE8BB");
                    painter->setPen(QPen(hover ? option->palette.highlightedText().color() : option->palette.text().color()));
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
                    const QString textToDraw("\uE923");
                    painter->setPen(QPen(option->palette.text().color()));
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
                    const QString textToDraw("\uE921");
                    painter->setPen(QPen(option->palette.text().color()));
                    painter->setFont(buttonFont);
                    painter->drawText(minButtonRect, Qt::AlignVCenter | Qt::AlignHCenter, textToDraw);
                }
            }
        }
        break;
    case CC_TitleBar:
        if (const auto* titlebar = qstyleoption_cast<const QStyleOptionTitleBar*>(option)) {
            painter->setPen(Qt::NoPen);
            painter->setPen(QPen(WINUI3Colors[colorSchemeIndex][surfaceStroke]));
            painter->setBrush(titlebar->palette.button());
            painter->drawRect(titlebar->rect);

            // draw title
            QRect textRect = proxy()->subControlRect(CC_TitleBar, titlebar, SC_TitleBarLabel, widget);
            painter->setPen(titlebar->palette.text().color());
            // Note workspace also does elliding but it does not use the correct font
            QString title = painter->fontMetrics().elidedText(titlebar->text, Qt::ElideRight, textRect.width() - 14);
            painter->drawText(textRect.adjusted(1, 1, 1, 1), title, QTextOption(Qt::AlignHCenter | Qt::AlignVCenter));

            QFont buttonFont = QFont(assetFont);
            buttonFont.setPointSize(8);
            // min button
            if ((titlebar->subControls & SC_TitleBarMinButton) && (titlebar->titleBarFlags & Qt::WindowMinimizeButtonHint) &&
                !(titlebar->titleBarState& Qt::WindowMinimized)) {
                const QRect minButtonRect = proxy()->subControlRect(CC_TitleBar, titlebar, SC_TitleBarMinButton, widget);
                if (minButtonRect.isValid()) {
                    bool hover = (titlebar->activeSubControls & SC_TitleBarMinButton) && (titlebar->state & State_MouseOver);
                    if (hover)
                        painter->fillRect(minButtonRect,WINUI3Colors[colorSchemeIndex][subtleHighlightColor]);
                    const QString textToDraw("\uE921");
                    painter->setPen(QPen(titlebar->palette.text().color()));
                    painter->setFont(buttonFont);
                    painter->drawText(minButtonRect, Qt::AlignVCenter | Qt::AlignHCenter, textToDraw);
                }
            }
            // max button
            if ((titlebar->subControls & SC_TitleBarMaxButton) && (titlebar->titleBarFlags & Qt::WindowMaximizeButtonHint) &&
                !(titlebar->titleBarState & Qt::WindowMaximized)) {
                const QRectF maxButtonRect = proxy()->subControlRect(CC_TitleBar, titlebar, SC_TitleBarMaxButton, widget);
                if (maxButtonRect.isValid()) {
                    bool hover = (titlebar->activeSubControls & SC_TitleBarMaxButton) && (titlebar->state & State_MouseOver);
                    if (hover)
                        painter->fillRect(maxButtonRect,WINUI3Colors[colorSchemeIndex][subtleHighlightColor]);
                    const QString textToDraw("\uE922");
                    painter->setPen(QPen(titlebar->palette.text().color()));
                    painter->setFont(buttonFont);
                    painter->drawText(maxButtonRect, Qt::AlignVCenter | Qt::AlignHCenter, textToDraw);
                }
            }

            // close button
            if ((titlebar->subControls & SC_TitleBarCloseButton) && (titlebar->titleBarFlags & Qt::WindowSystemMenuHint)) {
                const QRect closeButtonRect = proxy()->subControlRect(CC_TitleBar, titlebar, SC_TitleBarCloseButton, widget);
                if (closeButtonRect.isValid()) {
                    bool hover = (titlebar->activeSubControls & SC_TitleBarCloseButton) && (titlebar->state & State_MouseOver);
                    if (hover)
                        painter->fillRect(closeButtonRect,shellCloseButtonColor);
                    const QString textToDraw("\uE8BB");
                    painter->setPen(QPen(hover ? titlebar->palette.highlightedText().color() : titlebar->palette.text().color()));
                    painter->setFont(buttonFont);
                    painter->drawText(closeButtonRect, Qt::AlignVCenter | Qt::AlignHCenter, textToDraw);
                }
            }

            // normalize button
            if ((titlebar->subControls & SC_TitleBarNormalButton) &&
                (((titlebar->titleBarFlags & Qt::WindowMinimizeButtonHint) &&
                  (titlebar->titleBarState & Qt::WindowMinimized)) ||
                 ((titlebar->titleBarFlags & Qt::WindowMaximizeButtonHint) &&
                  (titlebar->titleBarState & Qt::WindowMaximized)))) {
                const QRect normalButtonRect = proxy()->subControlRect(CC_TitleBar, titlebar, SC_TitleBarNormalButton, widget);
                if (normalButtonRect.isValid()) {
                    bool hover = (titlebar->activeSubControls & SC_TitleBarNormalButton) && (titlebar->state & State_MouseOver);
                    if (hover)
                        painter->fillRect(normalButtonRect,WINUI3Colors[colorSchemeIndex][subtleHighlightColor]);
                    const QString textToDraw("\uE923");
                    painter->setPen(QPen(titlebar->palette.text().color()));
                    painter->setFont(buttonFont);
                    painter->drawText(normalButtonRect, Qt::AlignVCenter | Qt::AlignHCenter, textToDraw);
                }
            }

            // context help button
            if (titlebar->subControls & SC_TitleBarContextHelpButton
                && (titlebar->titleBarFlags & Qt::WindowContextHelpButtonHint)) {
                const QRect contextHelpButtonRect = proxy()->subControlRect(CC_TitleBar, titlebar, SC_TitleBarContextHelpButton, widget);
                if (contextHelpButtonRect.isValid()) {
                    bool hover = (titlebar->activeSubControls & SC_TitleBarCloseButton) && (titlebar->state & State_MouseOver);
                    if (hover)
                        painter->fillRect(contextHelpButtonRect,WINUI3Colors[colorSchemeIndex][subtleHighlightColor]);
                    const QString textToDraw("\uE897");
                    painter->setPen(QPen(titlebar->palette.text().color()));
                    painter->setFont(buttonFont);
                    painter->drawText(contextHelpButtonRect, Qt::AlignVCenter | Qt::AlignHCenter, textToDraw);
                }
            }

            // shade button
            if (titlebar->subControls & SC_TitleBarShadeButton && (titlebar->titleBarFlags & Qt::WindowShadeButtonHint)) {
                const QRect shadeButtonRect = proxy()->subControlRect(CC_TitleBar, titlebar, SC_TitleBarShadeButton, widget);
                if (shadeButtonRect.isValid()) {
                    bool hover = (titlebar->activeSubControls & SC_TitleBarShadeButton) && (titlebar->state & State_MouseOver);
                    if (hover)
                        painter->fillRect(shadeButtonRect,WINUI3Colors[colorSchemeIndex][subtleHighlightColor]);
                    const QString textToDraw("\uE010");
                    painter->setPen(QPen(titlebar->palette.text().color()));
                    painter->setFont(buttonFont);
                    painter->drawText(shadeButtonRect, Qt::AlignVCenter | Qt::AlignHCenter, textToDraw);
                }
            }

             // unshade button
            if (titlebar->subControls & SC_TitleBarUnshadeButton && (titlebar->titleBarFlags & Qt::WindowShadeButtonHint)) {
                const QRect unshadeButtonRect = proxy()->subControlRect(CC_TitleBar, titlebar, SC_TitleBarUnshadeButton, widget);
                if (unshadeButtonRect.isValid()) {
                    bool hover = (titlebar->activeSubControls & SC_TitleBarUnshadeButton) && (titlebar->state & State_MouseOver);
                    if (hover)
                        painter->fillRect(unshadeButtonRect,WINUI3Colors[colorSchemeIndex][subtleHighlightColor]);
                    const QString textToDraw("\uE011");
                    painter->setPen(QPen(titlebar->palette.text().color()));
                    painter->setFont(buttonFont);
                    painter->drawText(unshadeButtonRect, Qt::AlignVCenter | Qt::AlignHCenter, textToDraw);
                }
            }

            // window icon for system menu
            if ((titlebar->subControls & SC_TitleBarSysMenu) && (titlebar->titleBarFlags & Qt::WindowSystemMenuHint)) {
                const QRect iconRect = proxy()->subControlRect(CC_TitleBar, titlebar, SC_TitleBarSysMenu, widget);
                if (iconRect.isValid()) {
                    if (!titlebar->icon.isNull()) {
                        titlebar->icon.paint(painter, iconRect);
                    } else {
                        QStyleOption tool = *titlebar;
                        QPixmap pm = proxy()->standardIcon(SP_TitleBarMenuButton, &tool, widget).pixmap(16, 16);
                        tool.rect = iconRect;
                        painter->save();
                        proxy()->drawItemPixmap(painter, iconRect, Qt::AlignCenter, pm);
                        painter->restore();
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
    case PE_PanelTipLabel: {
        QRectF tipRect = option->rect.marginsRemoved(QMargins(1,1,1,1));
        painter->setPen(Qt::NoPen);
        painter->setBrush(option->palette.toolTipBase());
        painter->drawRoundedRect(tipRect, secondLevelRoundingRadius, secondLevelRoundingRadius);

        painter->setPen(highContrastTheme == true ? option->palette.buttonText().color() : WINUI3Colors[colorSchemeIndex][frameColorLight]);
        painter->setBrush(Qt::NoBrush);
        painter->drawRoundedRect(tipRect.marginsAdded(QMarginsF(0.5,0.5,0.5,0.5)), secondLevelRoundingRadius, secondLevelRoundingRadius);
        break;
    }
    case PE_FrameTabWidget:
        if (const QStyleOptionTabWidgetFrame *frame = qstyleoption_cast<const QStyleOptionTabWidgetFrame *>(option)) {
            QRectF frameRect = frame->rect.marginsRemoved(QMargins(0,0,0,0));
            painter->setPen(Qt::NoPen);
            painter->setBrush(frame->palette.base());
            painter->drawRoundedRect(frameRect, secondLevelRoundingRadius, secondLevelRoundingRadius);

            painter->setPen(highContrastTheme == true ? frame->palette.buttonText().color() : WINUI3Colors[colorSchemeIndex][frameColorLight]);
            painter->setBrush(Qt::NoBrush);
            painter->drawRoundedRect(frameRect.marginsRemoved(QMarginsF(0.5,0.5,0.5,0.5)), secondLevelRoundingRadius, secondLevelRoundingRadius);
        }
        break;
    case PE_FrameGroupBox:
        if (const QStyleOptionFrame *frame = qstyleoption_cast<const QStyleOptionFrame *>(option)) {
            QRectF frameRect = frame->rect;
            frameRect.adjust(0.5,0.5,-0.5,-0.5);
            painter->setPen(highContrastTheme == true ? frame->palette.buttonText().color() : WINUI3Colors[colorSchemeIndex][frameColorStrong]);
            painter->setBrush(Qt::NoBrush);
            if (frame->features & QStyleOptionFrame::Flat) {
                QRect fr = frame->rect;
                QPoint p1(fr.x(), fr.y() + 1);
                QPoint p2(fr.x() + fr.width(), p1.y());
                painter->drawLine(p1,p2);
            } else {
                painter->drawRoundedRect(frameRect.marginsRemoved(QMargins(1,1,1,1)), secondLevelRoundingRadius, secondLevelRoundingRadius);
            }
        }
        break;
    case PE_IndicatorCheckBox:
        {
            QNumberStyleAnimation* animation = qobject_cast<QNumberStyleAnimation*>(d->animation(option->styleObject));
            QFontMetrics fm(assetFont);

            QRectF rect = option->rect;
            QPointF center = QPointF(rect.x() + rect.width() / 2, rect.y() + rect.height() / 2);
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
            painter->setPen(Qt::NoPen);
            painter->setBrush(fillBrush);
            painter->drawRoundedRect(rect, secondLevelRoundingRadius, secondLevelRoundingRadius, Qt::AbsoluteSize);

            painter->setPen(QPen(highContrastTheme == true ? option->palette.buttonText().color() : WINUI3Colors[colorSchemeIndex][frameColorStrong]));
            painter->setBrush(Qt::NoBrush);
            painter->drawRoundedRect(rect, secondLevelRoundingRadius + 0.5, secondLevelRoundingRadius + 0.5, Qt::AbsoluteSize);

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

            QRectF rect = option->rect;
            QPointF center = QPoint(rect.x() + rect.width() / 2, rect.y() + rect.height() / 2);
            rect.setWidth(15);
            rect.setHeight(15);
            rect.moveCenter(center);
            QRectF innerRect = rect;
            innerRect.setWidth(8);
            innerRect.setHeight(8);
            innerRect.moveCenter(center);

            painter->setPen(Qt::NoPen);
            painter->setBrush(option->palette.accent());
            if (option->state & State_MouseOver && option->state & State_Enabled)
                painter->setBrush(QBrush(option->palette.accent().color().lighter(107)));
            painter->drawEllipse(center, 7, 7);

            painter->setPen(QPen(WINUI3Colors[colorSchemeIndex][frameColorStrong]));
            painter->setBrush(Qt::NoBrush);
            painter->drawEllipse(center, 7.5, 7.5);

            painter->setPen(Qt::NoPen);
            painter->setBrush(QBrush(option->palette.window()));
            painter->drawEllipse(center,innerRadius, innerRadius);

            painter->setPen(QPen(WINUI3Colors[colorSchemeIndex][frameColorStrong]));
            painter->setBrush(Qt::NoBrush);
            painter->drawEllipse(center,innerRadius + 0.5, innerRadius + 0.5);

            painter->setPen(Qt::NoPen);
            painter->setBrush(QBrush(option->palette.window()));
            if (option->state & State_MouseOver && option->state & State_Enabled)
                painter->setBrush(QBrush(option->palette.window().color().darker(107)));
            painter->drawEllipse(center,innerRadius, innerRadius);
        }
        break;
    case PE_PanelButtonTool:
    case PE_PanelButtonBevel:{
            QRectF rect = option->rect.marginsRemoved(QMargins(2,2,2,2));
            rect.adjust(-0.5,-0.5,0.5,0.5);
            painter->setBrush(Qt::NoBrush);
            if (element == PE_PanelButtonTool
                && ((!(state & QStyle::State_MouseOver) && !(state & QStyle::State_Raised))
                    || !(state & QStyle::State_Enabled)))
                painter->setPen(Qt::NoPen);
            else
                painter->setPen(QPen(WINUI3Colors[colorSchemeIndex][controlStrokePrimary]));
            painter->drawRoundedRect(rect, secondLevelRoundingRadius, secondLevelRoundingRadius);

            rect = option->rect.marginsRemoved(QMargins(2,2,2,2));
            painter->setPen(Qt::NoPen);
            if (!(state & (State_Raised)))
                painter->setBrush(WINUI3Colors[colorSchemeIndex][controlFillTertiary]);
            else if (state & State_MouseOver)
                painter->setBrush(WINUI3Colors[colorSchemeIndex][controlFillSecondary]);
            else
                painter->setBrush(option->palette.button());
            painter->drawRoundedRect(rect, secondLevelRoundingRadius, secondLevelRoundingRadius);
            painter->setPen(QPen(WINUI3Colors[colorSchemeIndex][controlStrokeSecondary]));
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
        QPen pen(WINUI3Colors[colorSchemeIndex][frameColorLight]);
        painter->save();
        painter->setPen(pen);
        painter->setBrush(QBrush(WINUI3Colors[colorSchemeIndex][menuPanelFill]));
        painter->setRenderHint(QPainter::Antialiasing);
        painter->drawRoundedRect(rect.marginsRemoved(QMargins(2,2,12,2)), topLevelRoundingRadius, topLevelRoundingRadius);
        painter->restore();
        break;
    }
    case PE_PanelLineEdit:
        if (widget && widget->objectName() == "qt_spinbox_lineedit")
            break;
        if (const auto *panel = qstyleoption_cast<const QStyleOptionFrame *>(option)) {
            QRectF frameRect = option->rect;
            frameRect.adjust(0.5,0.5,-0.5,-0.5);
            QBrush fillColor = option->palette.brush(QPalette::Base);
            painter->setBrush(fillColor);
            painter->setPen(Qt::NoPen);
            painter->drawRoundedRect(frameRect, secondLevelRoundingRadius, secondLevelRoundingRadius);
            // In case the QLineEdit is hovered overdraw the background with a alpha mask to
            // highlight the QLineEdit.
            if (state & State_MouseOver && !(state & State_HasFocus)) {
                fillColor = QBrush(WINUI3Colors[colorSchemeIndex][subtleHighlightColor]);
                painter->setBrush(fillColor);
                painter->setPen(Qt::NoPen);
                painter->drawRoundedRect(frameRect, secondLevelRoundingRadius, secondLevelRoundingRadius);
            }
            if (panel->lineWidth > 0)
                proxy()->drawPrimitive(PE_FrameLineEdit, panel, painter, widget);
        }
        break;
    case PE_FrameLineEdit: {
        painter->setBrush(Qt::NoBrush);
        painter->setPen(highContrastTheme == true ? option->palette.buttonText().color() : QPen(WINUI3Colors[colorSchemeIndex][frameColorLight]));
        painter->drawRoundedRect(option->rect, secondLevelRoundingRadius, secondLevelRoundingRadius);
        QRegion clipRegion = option->rect;
        clipRegion -= option->rect.adjusted(2, 2, -2, -2);
        painter->setClipRegion(clipRegion);
        QColor lineColor = state & State_HasFocus ? option->palette.accent().color() : QColor(0,0,0);
        painter->setPen(QPen(lineColor));
        painter->drawLine(option->rect.bottomLeft() + QPointF(1,0.5), option->rect.bottomRight() + QPointF(-1,0.5));
    }
        break;
    case PE_Frame: {
        if (const auto *frame = qstyleoption_cast<const QStyleOptionFrame *>(option)) {
            if (frame->frameShape == QFrame::NoFrame)
                break;
            QRectF rect = option->rect.adjusted(1,1,-1,-1);
            if (widget && widget->inherits("QComboBoxPrivateContainer")) {
                painter->setPen(Qt::NoPen);
                painter->setBrush(WINUI3Colors[colorSchemeIndex][menuPanelFill]);
                painter->drawRoundedRect(rect, secondLevelRoundingRadius, secondLevelRoundingRadius);

            }
            painter->setBrush(option->palette.base());
            painter->setPen(Qt::NoPen);
            painter->drawRoundedRect(rect, secondLevelRoundingRadius, secondLevelRoundingRadius);

            painter->setBrush(Qt::NoBrush);
            painter->setPen(QPen(WINUI3Colors[colorSchemeIndex][frameColorLight]));
            painter->drawRoundedRect(rect.marginsRemoved(QMarginsF(0.5,0.5,0.5,0.5)), secondLevelRoundingRadius, secondLevelRoundingRadius);

            if (qobject_cast<const QTextEdit *>(widget)) {
                QRegion clipRegion = option->rect;
                QColor lineColor = state & State_HasFocus ? option->palette.accent().color() : QColor(0,0,0,255);
                painter->setPen(QPen(lineColor));
                painter->drawLine(option->rect.bottomLeft() + QPoint(1,-1), option->rect.bottomRight() + QPoint(-1,-1));
            }
        }
        break;
    }
    case QStyle::PE_PanelItemViewRow:
        if (const QStyleOptionViewItem *vopt = qstyleoption_cast<const QStyleOptionViewItem *>(option)) {
            painter->setPen(Qt::NoPen);
            if (vopt->features & QStyleOptionViewItem::Alternate)
                painter->setBrush(vopt->palette.alternateBase());
            else
                painter->setBrush(vopt->palette.base());
            int markerOffset = 2;
            painter->drawRect(vopt->rect.marginsRemoved(QMargins(markerOffset, 0, -markerOffset, 0)));
            if ((vopt->state & State_Selected || vopt->state & State_MouseOver) && vopt->showDecorationSelected) {
                painter->setBrush(WINUI3Colors[colorSchemeIndex][subtleHighlightColor]);
                painter->setPen(Qt::NoPen);
                painter->drawRoundedRect(vopt->rect.marginsRemoved(QMargins(0,2,-2,2)),2,2);
                const int offset = qobject_cast<const QTreeView *>(widget) ? 2 : 0;
                if (vopt->viewItemPosition == QStyleOptionViewItem::Beginning && option->state & State_Selected) {
                    painter->setPen(QPen(option->palette.accent().color()));
                    painter->drawLine(option->rect.x(),option->rect.y()+offset,option->rect.x(),option->rect.y() + option->rect.height()-2);
                    painter->drawLine(option->rect.x()+1,option->rect.y()+2,option->rect.x()+1,option->rect.y() + option->rect.height()-2);
                }
            }
        }
        break;
    case QStyle::PE_Widget: {
#if QT_CONFIG(dialogbuttonbox)
        const QDialogButtonBox *buttonBox = nullptr;
        if (qobject_cast<const QMessageBox *> (widget))
            buttonBox = widget->findChild<const QDialogButtonBox *>(QLatin1String("qt_msgbox_buttonbox"));
#if QT_CONFIG(inputdialog)
        else if (qobject_cast<const QInputDialog *> (widget))
            buttonBox = widget->findChild<const QDialogButtonBox *>(QLatin1String("qt_inputdlg_buttonbox"));
#endif // QT_CONFIG(inputdialog)
        if (buttonBox) {
            painter->fillRect(option->rect,option->palette.window());
        }
#endif
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

            painter->setPen(QPen(WINUI3Colors[colorSchemeIndex][surfaceStroke]));
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

            painter->setPen(QPen(WINUI3Colors[colorSchemeIndex][surfaceStroke]));
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
    QRect rect(option->rect);
    State flags = option->state;

    painter->save();
    painter->setRenderHint(QPainter::Antialiasing);
    switch (element) {
    case QStyle::CE_TabBarTabShape:
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
        break;
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
                if (toolbutton->state & State_Raised || d->defaultPalette.buttonText() != toolbutton->palette.buttonText())
                    painter->setPen(QPen(toolbutton->palette.buttonText().color()));
                else if (!(toolbutton->state & State_Enabled))
                    painter->setPen(flags & State_On ? QPen(WINUI3Colors[colorSchemeIndex][textAccentDisabled]) : QPen(toolbutton->palette.buttonText().color()));
                else
                    painter->setPen(QPen(WINUI3Colors[colorSchemeIndex][controlTextSecondary]));
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
                    if (toolbutton->state & State_Raised || d->defaultPalette.buttonText() != toolbutton->palette.buttonText())
                        painter->setPen(QPen(toolbutton->palette.buttonText().color()));
                    else if (!(toolbutton->state & State_Enabled))
                        painter->setPen(flags & State_On ? QPen(WINUI3Colors[colorSchemeIndex][textAccentDisabled]) : QPen(toolbutton->palette.buttonText().color()));
                    else
                        painter->setPen(QPen(WINUI3Colors[colorSchemeIndex][controlTextSecondary]));
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
                painter->setPen(flags & State_On ? QPen(WINUI3Colors[colorSchemeIndex][textOnAccentSecondary]) : QPen(WINUI3Colors[colorSchemeIndex][controlTextSecondary]));
            else if (!(btn->state & State_Enabled))
                painter->setPen(flags & State_On ? QPen(WINUI3Colors[colorSchemeIndex][textAccentDisabled]) : QPen(btn->palette.buttonText().color()));
            else
                painter->setPen(flags & State_On ? QPen(WINUI3Colors[colorSchemeIndex][textOnAccentPrimary]) : QPen(btn->palette.buttonText().color()));
            proxy()->drawItemText(painter, textRect, tf, option->palette,btn->state & State_Enabled, btn->text);
        }
        break;
    case CE_PushButtonBevel:
        if (const QStyleOptionButton *btn = qstyleoption_cast<const QStyleOptionButton *>(option))  {
            if (btn->features.testFlag(QStyleOptionButton::Flat)) {
                painter->setPen(Qt::NoPen);
                if (flags & (State_Sunken | State_On)) {
                    painter->setBrush(WINUI3Colors[colorSchemeIndex][subtlePressedColor]);
                }
                else if (flags & State_MouseOver) {
                    painter->setBrush(WINUI3Colors[colorSchemeIndex][subtleHighlightColor]);
                }
                painter->drawRoundedRect(rect, secondLevelRoundingRadius, secondLevelRoundingRadius);
            } else {
                QRectF rect = btn->rect.marginsRemoved(QMargins(2,2,2,2));
                painter->setPen(Qt::NoPen);
                if (flags & (State_Sunken))
                    painter->setBrush(flags & State_On ? option->palette.accent().color().lighter(120) : WINUI3Colors[colorSchemeIndex][controlFillTertiary]);
                else if (flags & State_MouseOver)
                    painter->setBrush(flags & State_On ? option->palette.accent().color().lighter(110) : WINUI3Colors[colorSchemeIndex][controlFillSecondary]);
                else if (!(flags & State_Enabled))
                    painter->setBrush(flags & State_On ? WINUI3Colors[colorSchemeIndex][controlAccentDisabled] : option->palette.button());
                else
                    painter->setBrush(flags & State_On ? option->palette.accent() : option->palette.button());
                painter->drawRoundedRect(rect, secondLevelRoundingRadius, secondLevelRoundingRadius);

                rect.adjust(0.5,0.5,-0.5,-0.5);
                painter->setBrush(Qt::NoBrush);
                painter->setPen(btn->features.testFlag(QStyleOptionButton::DefaultButton) ? QPen(option->palette.accent().color()) : QPen(WINUI3Colors[colorSchemeIndex][controlStrokePrimary]));
                painter->drawRoundedRect(rect, secondLevelRoundingRadius, secondLevelRoundingRadius);

                painter->setPen(btn->features.testFlag(QStyleOptionButton::DefaultButton) ? QPen(WINUI3Colors[colorSchemeIndex][controlStrokeOnAccentSecondary]) : QPen(WINUI3Colors[colorSchemeIndex][controlStrokeSecondary]));
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
            if (enabled && (active || hasFocus)) {
                if (active && down)
                    painter->setBrushOrigin(painter->brushOrigin() + QPoint(1, 1));
                if (active && hasFocus) {
                    painter->setBrush(WINUI3Colors[colorSchemeIndex][subtleHighlightColor]);
                    painter->setPen(Qt::NoPen);
                    QRect rect = mbi->rect.marginsRemoved(QMargins(5,0,5,0));
                    painter->drawRoundedRect(rect,secondLevelRoundingRadius,secondLevelRoundingRadius,Qt::AbsoluteSize);
                }
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

            QBrush fill = (act == true && dis == false) ? QBrush(WINUI3Colors[colorSchemeIndex][subtleHighlightColor]) : menuitem->palette.brush(QPalette::Button);
            painter->setBrush(fill);
            painter->setPen(Qt::NoPen);
            QRect rect = menuitem->rect;
            rect = rect.marginsRemoved(QMargins(2,2,2,2));
            if (act && dis == false)
                painter->drawRoundedRect(rect,secondLevelRoundingRadius,secondLevelRoundingRadius,Qt::AbsoluteSize);

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

            QColor discol = menuitem->palette.text().color();;
            if (dis) {
                discol = menuitem->palette.color(QPalette::Disabled, QPalette::WindowText);
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
    case QStyle::CE_ItemViewItem: {
        if (const QStyleOptionViewItem *vopt = qstyleoption_cast<const QStyleOptionViewItem *>(option)) {
            if (const QAbstractItemView *view = qobject_cast<const QAbstractItemView *>(widget)) {
                QRect checkRect = proxy()->subElementRect(SE_ItemViewItemCheckIndicator, vopt, widget);
                QRect iconRect = proxy()->subElementRect(SE_ItemViewItemDecoration, vopt, widget);
                QRect textRect = proxy()->subElementRect(SE_ItemViewItemText, vopt, widget);

                QRect rect = vopt->rect;

                painter->setPen(highContrastTheme == true ? vopt->palette.buttonText().color() : WINUI3Colors[colorSchemeIndex][frameColorLight]);
                if (vopt->viewItemPosition == QStyleOptionViewItem::OnlyOne || vopt->viewItemPosition == QStyleOptionViewItem::Invalid) {
                } else if (vopt->viewItemPosition == QStyleOptionViewItem::Beginning) {
                    painter->drawLine(QPointF(option->rect.topRight()) + QPointF(0.5,0.0),
                                      QPointF(option->rect.bottomRight()) + QPointF(0.5,0.0));
                } else if (vopt->viewItemPosition == QStyleOptionViewItem::End) {
                    painter->drawLine(QPointF(option->rect.topLeft()) - QPointF(0.5,0.0),
                                      QPointF(option->rect.bottomLeft()) - QPointF(0.5,0.0));
                } else {
                    painter->drawLine(QPointF(option->rect.topRight()) + QPointF(0.5,0.0),
                                      QPointF(option->rect.bottomRight()) + QPointF(0.5,0.0));
                    painter->drawLine(QPointF(option->rect.topLeft()) - QPointF(0.5,0.0),
                                      QPointF(option->rect.bottomLeft()) - QPointF(0.5,0.0));
                }

                const bool isTreeView = qobject_cast<const QTreeView *>(widget);

                if ((vopt->state & State_Selected || vopt->state & State_MouseOver) && !(isTreeView && vopt->state & State_MouseOver) && vopt->showDecorationSelected) {
                    painter->setBrush(WINUI3Colors[colorSchemeIndex][subtleHighlightColor]);
                    QWidget *editorWidget = view ? view->indexWidget(view->currentIndex()) : nullptr;
                    if (editorWidget) {
                        QPalette pal = editorWidget->palette();
                        QColor editorBgColor = vopt->backgroundBrush == Qt::NoBrush ? vopt->palette.color(widget->backgroundRole()) : vopt->backgroundBrush.color();
                        editorBgColor.setAlpha(255);
                        pal.setColor(editorWidget->backgroundRole(),editorBgColor);
                        editorWidget->setPalette(pal);
                    }
                } else {
                    painter->setBrush(vopt->backgroundBrush);
                }
                painter->setPen(Qt::NoPen);

                if (vopt->viewItemPosition == QStyleOptionViewItem::OnlyOne || vopt->viewItemPosition == QStyleOptionViewItem::Invalid) {
                    painter->drawRoundedRect(vopt->rect.marginsRemoved(QMargins(2,2,2,2)),secondLevelRoundingRadius,secondLevelRoundingRadius);
                } else if (vopt->viewItemPosition == QStyleOptionViewItem::Beginning) {
                    painter->drawRoundedRect(rect.marginsRemoved(QMargins(2,2,0,2)),secondLevelRoundingRadius,secondLevelRoundingRadius);
                } else if (vopt->viewItemPosition == QStyleOptionViewItem::End) {
                    painter->drawRoundedRect(vopt->rect.marginsRemoved(QMargins(0,2,2,2)),secondLevelRoundingRadius,secondLevelRoundingRadius);
                } else {
                    painter->drawRect(vopt->rect.marginsRemoved(QMargins(0,2,0,2)));
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

                painter->setPen(QPen(option->palette.text().color()));
                if (!view || !view->isPersistentEditorOpen(vopt->index))
                    d->viewItemDrawText(painter, vopt, textRect);
                if (vopt->state & State_Selected
                 && (vopt->viewItemPosition == QStyleOptionViewItem::Beginning
                  || vopt->viewItemPosition == QStyleOptionViewItem::OnlyOne
                  || vopt->viewItemPosition == QStyleOptionViewItem::Invalid)) {
                    if (const QListView *lv = qobject_cast<const QListView *>(widget);
                        lv && lv->viewMode() != QListView::IconMode) {
                        painter->setPen(QPen(vopt->palette.accent().color()));
                        painter->drawLine(option->rect.x(), option->rect.y() + 2,
                                          option->rect.x(),option->rect.y() + option->rect.height() - 2);
                        painter->drawLine(option->rect.x() + 1, option->rect.y() + 2,
                                          option->rect.x() + 1,option->rect.y() + option->rect.height() - 2);
                    }
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
    case QStyle::SE_LineEditContents:
        ret = option->rect.adjusted(8,0,-8,0);
        break;
    case QStyle::SE_ItemViewItemText:
        if (const auto *item = qstyleoption_cast<const QStyleOptionViewItem *>(option)) {
            const int decorationOffset = item->features.testFlag(QStyleOptionViewItem::HasDecoration) ? item->decorationSize.width() : 0;
            if (widget && widget->parentWidget()
             && widget->parentWidget()->inherits("QComboBoxPrivateContainer")) {
                ret = option->rect.adjusted(decorationOffset + 5, 0, -5, 0);
            } else {
                ret = QWindowsVistaStyle::subElementRect(element, option, widget);
            }
        } else {
            ret = QWindowsVistaStyle::subElementRect(element, option, widget);
        }
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
                    ret.setRect(titlebar->rect.left() + controlWidthMargin + indent, titlebar->rect.top() + iconSize/2,
                                iconSize, iconSize);
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

        switch (subControl) {
        case QStyle::SC_ScrollBarAddLine:
            if (const QStyleOptionSlider *scrollbar = qstyleoption_cast<const QStyleOptionSlider *>(option)) {
                if (scrollbar->orientation == Qt::Vertical) {
                    ret = ret.adjusted(2,2,-2,-3);
                } else {
                    ret = ret.adjusted(3,2,-2,-2);
                }
            }
            break;
        case QStyle::SC_ScrollBarSubLine:
            if (const QStyleOptionSlider *scrollbar = qstyleoption_cast<const QStyleOptionSlider *>(option)) {
                if (scrollbar->orientation == Qt::Vertical) {
                    ret = ret.adjusted(2,2,-2,-3);
                } else {
                    ret = ret.adjusted(3,2,-2,-2);
                }
            }
            break;
        default:
            break;
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

    case CT_Menu:
        contentSize += QSize(10, 0);
        break;

#if QT_CONFIG(menubar)
    case CT_MenuBarItem:
        if (!contentSize.isEmpty()) {
            constexpr int hMargin = 2 * 6;
            constexpr int hPadding = 2 * 11;
            constexpr int itemHeight = 32;
            contentSize.setWidth(contentSize.width() + hMargin + hPadding);
            contentSize.setHeight(itemHeight);
        }
        break;
#endif
    case QStyle::CT_SpinBox: {
        if (const auto *spinBoxOpt = qstyleoption_cast<const QStyleOptionSpinBox *>(option)) {
            // Add button + frame widths
            int width = 0;

            if (const QDateTimeEdit *spinBox = qobject_cast<const QDateTimeEdit *>(widget)) {
                const QSize textSizeMin = spinBoxOpt->fontMetrics.size(Qt::TextSingleLine, spinBox->minimumDateTime().toString(spinBox->displayFormat()));
                const QSize textSizeMax = spinBoxOpt->fontMetrics.size(Qt::TextSingleLine, spinBox->maximumDateTime().toString(spinBox->displayFormat()));
                width = qMax(textSizeMin.width(),textSizeMax.width());
            } else if (const QSpinBox *spinBox = qobject_cast<const QSpinBox *>(widget)) {
                const QSize textSizeMin = spinBoxOpt->fontMetrics.size(Qt::TextSingleLine, QString::number(spinBox->minimum()));
                const QSize textSizeMax = spinBoxOpt->fontMetrics.size(Qt::TextSingleLine, QString::number(spinBox->maximum()));
                width = qMax(textSizeMin.width(),textSizeMax.width());
                width += spinBoxOpt->fontMetrics.size(Qt::TextSingleLine, spinBox->prefix()).width();
                width += spinBoxOpt->fontMetrics.size(Qt::TextSingleLine, spinBox->suffix()).width();

            } else if (const QDoubleSpinBox *spinBox = qobject_cast<const QDoubleSpinBox *>(widget)) {
                const QSize textSizeMin = spinBoxOpt->fontMetrics.size(Qt::TextSingleLine, QString::number(spinBox->minimum()));
                const QSize textSizeMax = spinBoxOpt->fontMetrics.size(Qt::TextSingleLine, QString::number(spinBox->maximum()));
                width = qMax(textSizeMin.width(),textSizeMax.width());
                width += spinBoxOpt->fontMetrics.size(Qt::TextSingleLine, spinBox->prefix()).width();
                width += spinBoxOpt->fontMetrics.size(Qt::TextSingleLine, spinBox->suffix()).width();
            }
            const qreal dpi = QStyleHelper::dpi(option);
            const bool hasButtons = (spinBoxOpt->buttonSymbols != QAbstractSpinBox::NoButtons);
            const int buttonWidth = hasButtons ? 2 * qRound(QStyleHelper::dpiScaled(16, dpi)) : 0;
            const int frameWidth = spinBoxOpt->frame ? proxy()->pixelMetric(PM_SpinBoxFrameWidth,
                                                                            spinBoxOpt, widget) : 0;
            contentSize.setWidth(2 * 12 + width);
            contentSize += QSize(buttonWidth + 2 * frameWidth, 2 * frameWidth);
        }
        break;
    }
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
    default:
        res = QWindowsVistaStyle::pixelMetric(metric, option, widget);
    }

    return res;
}

void QWindows11Style::polish(QWidget* widget)
{
    QWindowsVistaStyle::polish(widget);
    const bool isScrollBar = qobject_cast<QScrollBar *>(widget);
    if (isScrollBar || qobject_cast<QMenu *>(widget) || widget->inherits("QComboBoxPrivateContainer")) {
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
        if (!isScrollBar) { // for menus and combobox containers...
            QGraphicsDropShadowEffect* dropshadow = new QGraphicsDropShadowEffect(widget);
            dropshadow->setBlurRadius(3);
            dropshadow->setXOffset(3);
            dropshadow->setYOffset(3);
            widget->setGraphicsEffect(dropshadow);
        }
    } else if (QComboBox* cb = qobject_cast<QComboBox*>(widget)) {
        if (cb->isEditable()) {
            QLineEdit *le = cb->lineEdit();
            le->setFrame(false);
        }
    } else if (qobject_cast<QCommandLinkButton *>(widget)) {
        widget->setProperty("_qt_usingVistaStyle",false);
        QPalette pal = widget->palette();
        pal.setColor(QPalette::ButtonText, pal.text().color());
        pal.setColor(QPalette::BrightText, pal.text().color());
        widget->setPalette(pal);
    } else if (widget->inherits("QAbstractSpinBox")) {
        const int minWidth = 2 * 24 + 40;
        const int originalWidth = widget->size().width();
        if (originalWidth < minWidth) {
            widget->resize(minWidth, widget->size().height());
            widget->setProperty(originalWidthProperty.constData(), originalWidth);
        }
    } else if (widget->inherits("QAbstractButton") || widget->inherits("QToolButton")) {
        widget->setAutoFillBackground(false);
        auto pal = widget->palette();
        if (colorSchemeIndex == 0) {
            pal.setColor(QPalette::Disabled, QPalette::ButtonText, QColor(0x00,0x00,0x00,0x5C));
            pal.setColor(QPalette::Disabled, QPalette::Button, QColor(0xF9,0xF9,0xF9,0x4D));
        }
        else {
            pal.setColor(QPalette::Disabled, QPalette::ButtonText, QColor(0xFF,0xFF,0xFF,0x87));
            pal.setColor(QPalette::Disabled, QPalette::Button, QColor(0xFF,0xFF,0xFF,0x6B));
        }
        widget->setPalette(pal);

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
        QPalette pal = scrollarea->viewport()->palette();
        const QPalette originalPalette = pal;
        pal.setColor(scrollarea->viewport()->backgroundRole(), Qt::transparent);
        scrollarea->viewport()->setPalette(pal);
        scrollarea->viewport()->setProperty("_q_original_background_palette", originalPalette);
    }
}

void QWindows11Style::unpolish(QWidget *widget)
{
    QWindowsVistaStyle::unpolish(widget);
    if (const auto *scrollarea = qobject_cast<QAbstractScrollArea *>(widget);
        scrollarea
#if QT_CONFIG(mdiarea)
        && !qobject_cast<QMdiArea *>(widget)
#endif
        ) {
        const QPalette pal = scrollarea->viewport()->property("_q_original_background_palette").value<QPalette>();
        scrollarea->viewport()->setPalette(pal);
        scrollarea->viewport()->setProperty("_q_original_background_palette", QVariant());
    }
    if (widget->inherits("QAbstractSpinBox")) {
        const QVariant originalWidth = widget->property(originalWidthProperty.constData());
        if (originalWidth.isValid()) {
            widget->resize(originalWidth.toInt(), widget->size().height());
            widget->setProperty(originalWidthProperty.constData(), QVariant());
        }
    }
}

/*
The colors for Windows 11 are taken from the official WinUI3 Figma style at
https://www.figma.com/community/file/1159947337437047524
*/
static void populateLightSystemBasePalette(QPalette &result)
{
    static QString oldStyleSheet;
    const bool styleSheetChanged = oldStyleSheet != qApp->styleSheet();

    QPalette standardPalette = QApplication::palette();
    const QColor textColor = QColor(0x00,0x00,0x00,0xE4);

    const QColor btnFace = QColor(0xFF,0xFF,0xFF,0xB3);
    const QColor alternateBase = QColor(0x00,0x00,0x00,0x09);
    const QColor btnHighlight = result.accent().color();
    const QColor btnColor = result.button().color();

    if (standardPalette.color(QPalette::Highlight) == result.color(QPalette::Highlight) || styleSheetChanged)
        result.setColor(QPalette::Highlight, btnHighlight);
    if (standardPalette.color(QPalette::WindowText) == result.color(QPalette::WindowText) || styleSheetChanged)
        result.setColor(QPalette::WindowText, textColor);
    if (standardPalette.color(QPalette::Button) == result.color(QPalette::Button) || styleSheetChanged)
        result.setColor(QPalette::Button, btnFace);
    if (standardPalette.color(QPalette::Light) == result.color(QPalette::Light) || styleSheetChanged)
        result.setColor(QPalette::Light, btnColor.lighter(150));
    if (standardPalette.color(QPalette::Dark) == result.color(QPalette::Dark) || styleSheetChanged)
        result.setColor(QPalette::Dark, btnColor.darker(200));
    if (standardPalette.color(QPalette::Mid) == result.color(QPalette::Mid) || styleSheetChanged)
        result.setColor(QPalette::Mid, btnColor.darker(150));
    if (standardPalette.color(QPalette::Text) == result.color(QPalette::Text) || styleSheetChanged)
        result.setColor(QPalette::Text, textColor);
    if (standardPalette.color(QPalette::BrightText) != result.color(QPalette::BrightText) || styleSheetChanged)
        result.setColor(QPalette::BrightText, btnHighlight);
    if (standardPalette.color(QPalette::Base) == result.color(QPalette::Base) || styleSheetChanged)
        result.setColor(QPalette::Base, btnFace);
    if (standardPalette.color(QPalette::Window) == result.color(QPalette::Window) || styleSheetChanged)
        result.setColor(QPalette::Window, QColor(0xF3,0xF3,0xF3,0xFF));
    if (standardPalette.color(QPalette::ButtonText) == result.color(QPalette::ButtonText) || styleSheetChanged)
        result.setColor(QPalette::ButtonText, textColor);
    if (standardPalette.color(QPalette::Midlight) == result.color(QPalette::Midlight) || styleSheetChanged)
        result.setColor(QPalette::Midlight, btnColor.lighter(125));
    if (standardPalette.color(QPalette::Shadow) == result.color(QPalette::Shadow) || styleSheetChanged)
        result.setColor(QPalette::Shadow, Qt::black);
    if (standardPalette.color(QPalette::ToolTipBase) == result.color(QPalette::ToolTipBase) || styleSheetChanged)
        result.setColor(QPalette::ToolTipBase, result.window().color());
    if (standardPalette.color(QPalette::ToolTipText) == result.color(QPalette::ToolTipText) || styleSheetChanged)
        result.setColor(QPalette::ToolTipText, result.windowText().color());
    if (standardPalette.color(QPalette::AlternateBase) == result.color(QPalette::AlternateBase) || styleSheetChanged)
        result.setColor(QPalette::AlternateBase, alternateBase);

    if (result.midlight() == result.button())
        result.setColor(QPalette::Midlight, btnColor.lighter(110));
    oldStyleSheet = qApp->styleSheet();
}

/*!
 \internal
 */
void QWindows11Style::polish(QPalette& pal)
{
    Q_D(QWindows11Style);
    highContrastTheme = QGuiApplicationPrivate::colorScheme() == Qt::ColorScheme::Unknown;
    colorSchemeIndex = QGuiApplicationPrivate::colorScheme() == Qt::ColorScheme::Light ? 0 : 1;

    if (!highContrastTheme && colorSchemeIndex == 0)
        populateLightSystemBasePalette(pal);

    if (standardPalette().color(QPalette::Inactive, QPalette::Button) == pal.color(QPalette::Inactive, QPalette::Button))
        pal.setColor(QPalette::Inactive, QPalette::Button, pal.button().color());
    if (standardPalette().color(QPalette::Inactive, QPalette::Window) == pal.color(QPalette::Inactive, QPalette::Window))
        pal.setColor(QPalette::Inactive, QPalette::Window, pal.window().color());
    if (standardPalette().color(QPalette::Inactive, QPalette::Light) == pal.color(QPalette::Inactive, QPalette::Light))
        pal.setColor(QPalette::Inactive, QPalette::Light, pal.light().color());
    if (standardPalette().color(QPalette::Inactive, QPalette::Dark) == pal.color(QPalette::Inactive, QPalette::Dark))
        pal.setColor(QPalette::Inactive, QPalette::Dark, pal.dark().color());
    if (standardPalette().color(QPalette::Inactive, QPalette::Accent) == pal.color(QPalette::Inactive, QPalette::Accent))
        pal.setColor(QPalette::Inactive, QPalette::Accent, pal.accent().color());
    if (standardPalette().color(QPalette::Inactive, QPalette::Highlight) == pal.color(QPalette::Inactive, QPalette::Highlight))
        pal.setColor(QPalette::Inactive, QPalette::Highlight, pal.highlight().color());
    if (standardPalette().color(QPalette::Inactive, QPalette::HighlightedText) == pal.color(QPalette::Inactive, QPalette::HighlightedText))
        pal.setColor(QPalette::Inactive, QPalette::HighlightedText, pal.highlightedText().color());
    if (standardPalette().color(QPalette::Inactive, QPalette::Text) == pal.color(QPalette::Inactive, QPalette::Text))
        pal.setColor(QPalette::Inactive, QPalette::Text, pal.text().color());
    if (standardPalette().color(QPalette::Inactive, QPalette::WindowText) == pal.color(QPalette::Inactive, QPalette::WindowText))
        pal.setColor(QPalette::Inactive, QPalette::WindowText, pal.windowText().color());

    if (highContrastTheme)
        pal.setColor(QPalette::Active, QPalette::HighlightedText, pal.windowText().color());
    d->defaultPalette = pal;
}

QT_END_NAMESPACE
