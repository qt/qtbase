// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR BSD-3-Clause

#include <QStyle>
#include <QWidget>
#include <QStyleOptionButton>
#include <QStylePainter>
#include <QRect>
#include <QPalette>
#include <QFontMetrics>
#include <QCheckBox>
#include <QCommonStyle>

class CheckBox : public QCheckBox
{
public:
   void examples();
};

class CommonStyle : public QCommonStyle
{
public:
    void examples();
};

void CheckBox::examples()
{
    {
        QWidget *q;
        QStyleOptionButton opt;
        bool down, tristate, noChange, checked, hovering;
        const QString text;
        const QIcon icon;

    //! [0]
        opt.initFrom(q);
        if (down)
            opt.state |= QStyle::State_Sunken;
        if (tristate && noChange)
            opt.state |= QStyle::State_NoChange;
        else
            opt.state |= checked ? QStyle::State_On : QStyle::State_Off;
        if (q->testAttribute(Qt::WA_Hover) && q->underMouse()) {
            if (hovering)
                opt.state |= QStyle::State_MouseOver;
            else
                opt.state &= ~QStyle::State_MouseOver;
        }
        opt.text = text;
        opt.icon = icon;
        opt.iconSize = q->size();
    //! [0]
    }

    {
        QStyle::State state;
        QWidget *widget;
        Qt::LayoutDirection direction;
        QRect rect;
        QPalette palette;
        QFontMetrics fontMetrics = widget->fontMetrics();

    //! [1]
        state = QStyle::State_None;
        if (widget->isEnabled())
            state |= QStyle::State_Enabled;
        if (widget->hasFocus())
            state |= QStyle::State_HasFocus;
        if (widget->window()->testAttribute(Qt::WA_KeyboardFocusChange))
            state |= QStyle::State_KeyboardFocusChange;
        if (widget->underMouse())
            state |= QStyle::State_MouseOver;
        if (widget->window()->isActiveWindow())
            state |= QStyle::State_Active;

        direction = widget->layoutDirection();
        rect = widget->rect();
        palette = widget->palette();
        fontMetrics = widget->fontMetrics();
    //! [1]
    }

    {
        QStyle *d;
    //! [2]
        QStylePainter p;
        QStyleOptionButton opt;
        initStyleOption(&opt);
        p.drawControl(QStyle::CE_CheckBox, opt);
    //! [2]
    }
}

void CommonStyle::examples()
{
    {
        QPainter *p;
        QStyleOptionButton *btn;
        QWidget *widget;
        bool State_HasFocus;

    //! [3]
        QStyleOptionButton subopt = *btn;
        subopt.rect = subElementRect(QStyle::SE_CheckBoxIndicator, btn, widget);
        drawPrimitive(QStyle::PE_IndicatorCheckBox, &subopt, p, widget);
        subopt.rect = subElementRect(QStyle::SE_CheckBoxContents, btn, widget);
        drawControl(QStyle::CE_CheckBoxLabel, &subopt, p, widget);
        if (btn->state & State_HasFocus) {
            QStyleOptionFocusRect fropt;
            fropt.QStyleOption::operator=(*btn);
            fropt.rect = subElementRect(QStyle::SE_CheckBoxFocusRect, btn, widget);
            drawPrimitive(QStyle::PE_FrameFocusRect, &fropt, p, widget);
        }
    //! [3]
    }

    {
        QStyleOptionButton *opt;
        QWidget *widget;
        QPainter *p;

    //! [4]
        const QStyleOptionButton *btn = qstyleoption_cast<const QStyleOptionButton *>(opt);
        uint alignment = visualAlignment(btn->direction, Qt::AlignLeft | Qt::AlignVCenter);
        if (!styleHint(SH_UnderlineShortcut, btn, widget))
            alignment |= Qt::TextHideMnemonic;
        QPixmap pix;
        QRect textRect = btn->rect;
        if (!btn->icon.isNull()) {
            const auto dpr = p->device()->devicePixelRatio();
            pix = btn->icon.pixmap(btn->iconSize, dpr,
                                    btn->state & State_Enabled ? QIcon::Normal : QIcon::Disabled);
            drawItemPixmap(p, btn->rect, alignment, pix);
            if (btn->direction == Qt::RightToLeft)
                textRect.setRight(textRect.right() - btn->iconSize.width() - 4);
            else
                textRect.setLeft(textRect.left() + btn->iconSize.width() + 4);
        }
        if (!btn->text.isEmpty()){
            drawItemText(p, textRect, alignment | Qt::TextShowMnemonic,
                btn->palette, btn->state & State_Enabled, btn->text, QPalette::WindowText);
        }
    //! [4]
    }
}
