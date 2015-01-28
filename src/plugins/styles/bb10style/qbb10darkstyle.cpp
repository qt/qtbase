/***************************************************************************
**
** Copyright (C) 2014 BlackBerry Limited. All rights reserved.
** Contact: http://www.qt.io/licensing/
**
** This file is part of the plugins of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:LGPL21$
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company. For licensing terms
** and conditions see http://www.qt.io/terms-conditions. For further
** information use the contact form at http://www.qt.io/contact-us.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 2.1 or version 3 as published by the Free
** Software Foundation and appearing in the file LICENSE.LGPLv21 and
** LICENSE.LGPLv3 included in the packaging of this file. Please review the
** following information to ensure the GNU Lesser General Public License
** requirements will be met: https://www.gnu.org/licenses/lgpl.html and
** http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** As a special exception, The Qt Company gives you certain additional
** rights. These rights are described in The Qt Company LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
** $QT_END_LICENSE$
**
****************************************************************************/

#include "qbb10darkstyle.h"

#include <QApplication>
#include <QFont>
#include <QStyleOption>
#include <QProgressBar>
#include <QComboBox>
#include <QAbstractItemView>
#include <QPainter>
#include <QLineEdit>
#include <QTextEdit>

QT_BEGIN_NAMESPACE

QBB10DarkStyle::QBB10DarkStyle() :
    QPixmapStyle()
{
    addDescriptor(PB_Enabled,
                  QLatin1String("://dark/button/core_button_inactive.png"),
                  QMargins(13, 13, 13, 13),
                  QTileRules(Qt::RepeatTile, Qt::StretchTile));
    addDescriptor(PB_Checked,
                  QLatin1String("://dark/button/core_button_enabled_selected.png"),
                  QMargins(13, 13, 13, 13),
                  QTileRules(Qt::RepeatTile, Qt::StretchTile));
    addDescriptor(PB_Pressed,
                  QLatin1String("://dark/button/core_button_pressed.png"),
                  QMargins(13, 13, 13, 13),
                  QTileRules(Qt::RepeatTile, Qt::StretchTile));
    addDescriptor(PB_Disabled,
                  QLatin1String("://dark/button/core_button_disabled.png"),
                  QMargins(13, 13, 13, 13),
                  QTileRules(Qt::RepeatTile, Qt::StretchTile));
    addDescriptor(PB_PressedDisabled,
                  QLatin1String("://dark/button/core_button_disabled_selected.png"),
                  QMargins(13, 13, 13, 13),
                  QTileRules(Qt::RepeatTile, Qt::StretchTile));

    addDescriptor(LE_Enabled,
                  QLatin1String("://dark/lineedit/core_textinput_bg.png"),
                  QMargins(8, 8, 8, 8));
    addDescriptor(LE_Disabled,
                  QLatin1String("://dark/lineedit/core_textinput_bg_disabled.png"),
                  QMargins(8, 8, 8, 8));
    addDescriptor(LE_Focused,
                  QLatin1String("://dark/lineedit/core_textinput_bg_highlight.png"),
                  QMargins(8, 8, 8, 8));

    copyDescriptor(LE_Enabled, TE_Enabled);
    copyDescriptor(LE_Disabled, TE_Disabled);
    copyDescriptor(LE_Focused, TE_Focused);

    addPixmap(CB_Enabled,
              QLatin1String("://dark/checkbox/core_checkbox_enabled.png"),
              QMargins(16, 16, 16, 16));
    addPixmap(CB_Checked,
              QLatin1String("://dark/checkbox/core_checkbox_checked.png"),
              QMargins(16, 16, 16, 16));
    addPixmap(CB_Pressed,
              QLatin1String("://dark/checkbox/core_checkbox_pressed.png"),
              QMargins(16, 16, 16, 16));
    addPixmap(CB_PressedChecked,
              QLatin1String("://dark/checkbox/core_checkbox_pressed_checked.png"),
              QMargins(16, 16, 16, 16));
    addPixmap(CB_Disabled,
              QLatin1String("://dark/checkbox/core_checkbox_disabled.png"),
              QMargins(16, 16, 16, 16));
    addPixmap(CB_DisabledChecked,
              QLatin1String("://dark/checkbox/core_checkbox_disabled_checked.png"),
              QMargins(16, 16, 16, 16));

    addPixmap(RB_Enabled,
              QLatin1String("://dark/radiobutton/core_radiobutton_inactive.png"),
              QMargins(16, 16, 16, 16));
    addPixmap(RB_Checked,
              QLatin1String("://dark/radiobutton/core_radiobutton_checked.png"),
              QMargins(16, 16, 16, 16));
    addPixmap(RB_Pressed,
              QLatin1String("://dark/radiobutton/core_radiobutton_pressed.png"),
              QMargins(16, 16, 16, 16));
    addPixmap(RB_Disabled,
              QLatin1String("://dark/radiobutton/core_radiobutton_disabled.png"),
              QMargins(16, 16, 16, 16));
    addPixmap(RB_DisabledChecked,
              QLatin1String("://dark/radiobutton/core_radiobutton_disabled_checked.png"),
              QMargins(16, 16, 16, 16));

    addDescriptor(PB_HBackground,
                  QLatin1String("://dark/progressbar/core_progressindicator_bg.png"),
                  QMargins(10, 10, 10, 10),
                  QTileRules(Qt::RepeatTile, Qt::StretchTile));
    addDescriptor(PB_HContent,
                  QLatin1String("://dark/progressbar/core_progressindicator_fill.png"),
                  QMargins(10, 10, 10, 10),
                  QTileRules(Qt::RepeatTile, Qt::StretchTile));
    addDescriptor(PB_HComplete,
                  QLatin1String("://dark/progressbar/core_progressindicator_complete.png"),
                  QMargins(10, 10, 10, 10),
                  QTileRules(Qt::RepeatTile, Qt::StretchTile));
    addDescriptor(PB_VBackground,
                  QLatin1String("://dark/progressbar/core_progressindicator_vbg.png"),
                  QMargins(10, 10, 10, 10),
                  QTileRules(Qt::StretchTile, Qt::RepeatTile));
    addDescriptor(PB_VContent,
                  QLatin1String("://dark/progressbar/core_progressindicator_vfill.png"),
                  QMargins(10, 10, 10, 10),
                  QTileRules(Qt::StretchTile, Qt::RepeatTile));
    addDescriptor(PB_VComplete,
                  QLatin1String("://dark/progressbar/core_progressindicator_vcomplete.png"),
                  QMargins(10, 10, 10, 10),
                  QTileRules(Qt::StretchTile, Qt::RepeatTile));

    addDescriptor(SG_HEnabled,
                  QLatin1String("://dark/slider/core_slider_enabled.png"),
                  QMargins(50, 50, 50, 50),
                  QTileRules(Qt::RepeatTile, Qt::StretchTile));
    addDescriptor(SG_HDisabled,
                  QLatin1String("://dark/slider/core_slider_disabled.png"),
                  QMargins(50, 50, 50, 50),
                  QTileRules(Qt::RepeatTile, Qt::StretchTile));
    addDescriptor(SG_HActiveEnabled,
                  QLatin1String("://dark/slider/core_slider_inactive.png"),
                  QMargins(50, 50, 50, 50),
                  QTileRules(Qt::RepeatTile, Qt::StretchTile));
    addDescriptor(SG_HActivePressed,
                  QLatin1String("://dark/slider/core_slider_active.png"),
                  QMargins(50, 50, 50, 50),
                  QTileRules(Qt::RepeatTile, Qt::StretchTile));
    addDescriptor(SG_HActiveDisabled,
                  QLatin1String("://dark/slider/core_slider_cache.png"),
                  QMargins(50, 50, 50, 50),
                  QTileRules(Qt::RepeatTile, Qt::StretchTile));
    addDescriptor(SG_VEnabled,
                  QLatin1String("://dark/slider/core_slider_venabled.png"),
                  QMargins(50, 50, 50, 50),
                  QTileRules(Qt::StretchTile, Qt::RepeatTile));
    addDescriptor(SG_VDisabled,
                  QLatin1String("://dark/slider/core_slider_vdisabled.png"),
                  QMargins(50, 50, 50, 50),
                  QTileRules(Qt::StretchTile, Qt::RepeatTile));
    addDescriptor(SG_VActiveEnabled,
                  QLatin1String("://dark/slider/core_slider_vinactive.png"),
                  QMargins(50, 50, 50, 50),
                  QTileRules(Qt::StretchTile, Qt::RepeatTile));
    addDescriptor(SG_VActivePressed,
                  QLatin1String("://dark/slider/core_slider_vactive.png"),
                  QMargins(50, 50, 50, 50),
                  QTileRules(Qt::StretchTile, Qt::RepeatTile));
    addDescriptor(SG_VActiveDisabled,
                  QLatin1String("://dark/slider/core_slider_vcache.png"),
                  QMargins(50, 50, 50, 50),
                  QTileRules(Qt::StretchTile, Qt::RepeatTile));

    addPixmap(SH_HEnabled,
                  QLatin1String("://dark/slider/core_slider_handle.png"));
    addPixmap(SH_HDisabled,
                  QLatin1String("://dark/slider/core_slider_handle_disabled.png"));
    addPixmap(SH_HPressed,
                  QLatin1String("://dark/slider/core_slider_handle_pressed.png"));
    addPixmap(SH_VEnabled,
                  QLatin1String("://dark/slider/core_slider_handle.png"));
    addPixmap(SH_VDisabled,
                  QLatin1String("://dark/slider/core_slider_handle_disabled.png"));
    addPixmap(SH_VPressed,
                  QLatin1String("://dark/slider/core_slider_handle_pressed.png"));

    addDescriptor(DD_ButtonEnabled,
                  QLatin1String("://dark/combobox/core_dropdown_button.png"),
                  QMargins(14, 14, 14, 14),
                  QTileRules(Qt::RepeatTile, Qt::StretchTile));
    addDescriptor(DD_ButtonDisabled,
                  QLatin1String("://dark/combobox/core_dropdown_button_disabled.png"),
                  QMargins(14, 14, 14, 14),
                  QTileRules(Qt::RepeatTile, Qt::StretchTile));
    addDescriptor(DD_ButtonPressed,
                  QLatin1String("://dark/combobox/core_dropdown_button_pressed.png"),
                  QMargins(14, 14, 14, 14),
                  QTileRules(Qt::RepeatTile, Qt::StretchTile));
    addDescriptor(DD_ItemSelected,
                  QLatin1String("://dark/combobox/core_listitem_active.png"));
    addPixmap(DD_ArrowEnabled,
                  QLatin1String("://dark/combobox/core_dropdown_button_arrowdown.png"),
                  QMargins(35, 39, 35, 39));
    copyPixmap(DD_ArrowEnabled, DD_ArrowDisabled);
    addPixmap(DD_ArrowPressed,
                  QLatin1String("://dark/combobox/core_dropdown_button_arrowdown_pressed.png"),
                  QMargins(35, 39, 35, 39));
    addPixmap(DD_ArrowOpen,
                  QLatin1String("://dark/combobox/core_dropdown_button_arrowup.png"),
                  QMargins(35, 39, 35, 39));
    addDescriptor(DD_PopupDown,
                  QLatin1String("://dark/combobox/core_dropdown_menu.png"),
                  QMargins(12, 12, 12, 12), QTileRules(Qt::StretchTile, Qt::StretchTile));
    addDescriptor(DD_PopupUp,
                  QLatin1String("://dark/combobox/core_dropdown_menuup.png"),
                  QMargins(12, 12, 12, 12), QTileRules(Qt::StretchTile, Qt::StretchTile));
    addPixmap(DD_ItemSeparator,
                  QLatin1String("://dark/combobox/core_dropdown_divider.png"),
                  QMargins(5, 0, 5, 0));

    addDescriptor(ID_Selected,
                  QLatin1String("://dark/listitem/core_listitem_active.png"));
    addPixmap(ID_Separator,
                  QLatin1String("://dark/listitem/core_listitem_divider.png"));

    addDescriptor(SB_Horizontal,
                  QLatin1String("://dark/scrollbar/core_scrollbar.png"),
                  QMargins(7, 8, 7, 8),
                  QTileRules(Qt::RepeatTile, Qt::StretchTile));
    addDescriptor(SB_Vertical,
                  QLatin1String("://dark/scrollbar/core_scrollbar_v.png"),
                  QMargins(8, 7, 8, 7),
                  QTileRules(Qt::StretchTile, Qt::RepeatTile));
}

QBB10DarkStyle::~QBB10DarkStyle()
{
}

void QBB10DarkStyle::polish(QApplication *application)
{
    QPixmapStyle::polish(application);
}

void QBB10DarkStyle::polish(QWidget *widget)
{
    // Hide the text by default
    if (QProgressBar *pb = qobject_cast<QProgressBar*>(widget))
        pb->setTextVisible(false);

    if (QComboBox *cb = qobject_cast<QComboBox*>(widget)) {
        QAbstractItemView *list = cb->view();
        QPalette p = list->palette();
        p.setBrush(QPalette::HighlightedText, p.brush(QPalette::Text));
        list->setPalette(p);
    }

    if (qobject_cast<QLineEdit*>(widget) || qobject_cast<QTextEdit*>(widget)) {
        QPalette p = widget->palette();
        p.setBrush(QPalette::Text, QColor(38, 38, 38));
        widget->setPalette(p);
    }

    if (qobject_cast<QAbstractItemView*>(widget)) {
        QPalette p = widget->palette();
        p.setBrush(QPalette::Disabled, QPalette::HighlightedText, p.brush(QPalette::Text));
        widget->setPalette(p);
    }

    QPixmapStyle::polish(widget);
}

QPalette QBB10DarkStyle::standardPalette() const
{
    QPalette p;

    QColor color = QColor(250, 250, 250);
    p.setBrush(QPalette::ButtonText, color);
    p.setBrush(QPalette::WindowText, color);
    p.setBrush(QPalette::Text, color);

    color.setAlpha(179);
    p.setBrush(QPalette::Disabled, QPalette::ButtonText, color);
    p.setBrush(QPalette::Disabled, QPalette::WindowText, color);
    p.setBrush(QPalette::Disabled, QPalette::Text, color);

    p.setColor(QPalette::Window, QColor(18, 18, 18));

    p.setBrush(QPalette::Highlight, QColor(0, 168, 223));
    p.setBrush(QPalette::HighlightedText, QColor(250, 250,250));

    return p;
}

void QBB10DarkStyle::drawControl(QStyle::ControlElement element, const QStyleOption *option,
                                  QPainter *painter, const QWidget *widget) const
{
    switch (element) {
    case CE_PushButtonLabel:
    {
        const bool on = option->state & State_On || option->state & State_Sunken;
        const QStyleOptionButton *button = qstyleoption_cast<const QStyleOptionButton*>(option);
        QStyleOptionButton newOpt = *button;
        if (on)
            newOpt.palette.setBrush(QPalette::ButtonText, QColor(38, 38, 38));
        QPixmapStyle::drawControl(CE_PushButtonLabel, &newOpt, painter, widget);
        break;
    }
    case CE_ProgressBarLabel:
        // Don't draw the progress bar label
        break;
    default:
        QPixmapStyle::drawControl(element, option, painter, widget);
    }
}

void QBB10DarkStyle::drawPrimitive(QStyle::PrimitiveElement element, const QStyleOption *option,
                                    QPainter *painter, const QWidget *widget) const
{
    QPixmapStyle::drawPrimitive(element, option, painter, widget);

    if (element == PE_PanelItemViewItem) {
        // Draw the checkbox for current item
        if (widget->property("_pixmap_combobox_list").toBool()
                && option->state & QStyle::State_Selected) {
            QPixmap pix(QLatin1String("://dark/combobox/core_dropdown_checkmark.png"));
            QRect rect = option->rect;
            const int margin = rect.height() / 2;
            QPoint pos(rect.right() - margin - pix.width() / 2,
                       rect.top() + margin - pix.height() / 2);
            painter->drawPixmap(pos, pix);
        }
    }
}

QT_END_NAMESPACE
