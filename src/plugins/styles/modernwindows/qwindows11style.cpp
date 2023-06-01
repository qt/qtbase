// Copyright (C) 2022 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#include "qwindows11style_p.h"
#include <qstylehints.h>
#include <private/qstyleanimation_p.h>
#include <private/qstylehelper_p.h>
#include <qstyleoption.h>
#include <qpainter.h>
#include <qglobal.h>
#include <QGraphicsDropShadowEffect>

#include "qdrawutil.h"

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
    int state = option->state;
    painter->save();
    painter->setRenderHint(QPainter::Antialiasing);
    switch (element) {
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
    painter->save();
    painter->setRenderHint(QPainter::Antialiasing);
    switch (element) {
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

void QWindows11Style::polish(QWidget* widget)
{
    QWindowsVistaStyle::polish(widget);
    if (widget->inherits("QScrollBar") || widget->inherits("QComboBoxPrivateContainer")) {
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
    if (widget->inherits("QComboBoxPrivateContainer")) {
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
