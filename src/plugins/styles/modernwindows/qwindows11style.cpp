// Copyright (C) 2022 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#include "qwindows11style_p.h"

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
    State state = option->state;
    SubControls sub = option->subControls;
    State flags = option->state;
    if (widget && widget->testAttribute(Qt::WA_UnderMouse) && widget->isActiveWindow())
        flags |= State_MouseOver;

    painter->save();
    painter->setRenderHint(QPainter::Antialiasing);
    switch (control) {
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

void QWindows11Style::polish(QWidget* widget)
{
    QWindowsVistaStyle::polish(widget);
    if (widget->inherits("QScrollBar")) {
        bool wasCreated = widget->testAttribute(Qt::WA_WState_Created);
        widget->setAttribute(Qt::WA_OpaquePaintEvent,false);
        widget->setAttribute(Qt::WA_TranslucentBackground);
        widget->setWindowFlag(Qt::FramelessWindowHint);
        widget->setWindowFlag(Qt::NoDropShadowWindowHint);
        widget->setAttribute(Qt::WA_WState_Created, wasCreated);
        auto pal = widget->palette();
        pal.setColor(widget->backgroundRole(), Qt::transparent);
        widget->setPalette(pal);
    }
    if (widget->inherits("QGraphicsView")) {
        QPalette pal = widget->palette();
        pal.setColor(QPalette::Base, pal.window().color());
        widget->setPalette(pal);
    }
}

QT_END_NAMESPACE
