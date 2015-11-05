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

#include "qpixmapstyle.h"

#include <QDebug>
#include <QTextEdit>
#include <QStringBuilder>
#include <QPainter>
#include <QPixmapCache>
#include <QStyleOption>
#include <QString>
#include <QProgressBar>
#include <QSlider>
#include <QEvent>
#include <QComboBox>
#include <QAbstractItemView>
#include <QListView>
#include <QTreeView>
#include <QStyledItemDelegate>
#include <QAbstractScrollArea>
#include <QScrollBar>

#include <qscroller.h>

QT_BEGIN_NAMESPACE

QPixmapStyle::QPixmapStyle() :
    QCommonStyle()
{
}

QPixmapStyle::~QPixmapStyle()
{
}

void QPixmapStyle::polish(QApplication *application)
{
    QCommonStyle::polish(application);
#if defined(Q_DEAD_CODE_FROM_QT4_WIN)
    QApplication::setEffectEnabled(Qt::UI_AnimateCombo, false);
#endif
}

void QPixmapStyle::polish(QPalette &palette)
{
    palette = proxy()->standardPalette();
}

void QPixmapStyle::polish(QWidget *widget)
{
    // Don't fill the interior of the QTextEdit
    if (qobject_cast<QTextEdit*>(widget)) {
        QPalette p = widget->palette();
        p.setBrush(QPalette::Base, Qt::NoBrush);
        widget->setPalette(p);
    }

    if (QProgressBar *pb = qobject_cast<QProgressBar*>(widget)) {
        // Center the text in the progress bar
        pb->setAlignment(Qt::AlignCenter);
        // Change the font size if needed, as it's used to compute the minimum size
        QFont font = pb->font();
        font.setPixelSize(m_descriptors.value(PB_HBackground).size.height()/2);
        pb->setFont(font);
    }

    if (qobject_cast<QSlider*>(widget))
        widget->installEventFilter(this);

    if (QComboBox *cb = qobject_cast<QComboBox*>(widget)) {
        widget->installEventFilter(this);
        // NOTE: This will break if the private API of QComboBox changes drastically
        // Make sure the popup is created so we can change the frame style
        QAbstractItemView *list = cb->view();
        list->setProperty("_pixmap_combobox_list", true);
        list->setItemDelegate(new QStyledItemDelegate(list));
        QPalette p = list->palette();
        p.setBrush(QPalette::Active, QPalette::Base, QBrush(Qt::transparent) );
        p.setBrush(QPalette::Active, QPalette::AlternateBase, QBrush(Qt::transparent) );
        p.setBrush(QPalette::Inactive, QPalette::Base, QBrush(Qt::transparent) );
        p.setBrush(QPalette::Inactive, QPalette::AlternateBase, QBrush(Qt::transparent) );
        p.setBrush(QPalette::Disabled, QPalette::Base, QBrush(Qt::transparent) );
        p.setBrush(QPalette::Disabled, QPalette::AlternateBase, QBrush(Qt::transparent) );
        list->setPalette(p);

        QFrame *frame = qobject_cast<QFrame*>(list->parent());
        if (frame) {
            const Descriptor &desc = m_descriptors.value(DD_PopupDown);
            const Pixmap &pix = m_pixmaps.value(DD_ItemSeparator);
            frame->setContentsMargins(pix.margins.left(), desc.margins.top(),
                                      pix.margins.right(), desc.margins.bottom());
            frame->setAttribute(Qt::WA_TranslucentBackground);
#ifdef Q_DEAD_CODE_FROM_QT4_WIN
            // FramelessWindowHint is needed on windows to make
            // WA_TranslucentBackground work properly
            frame->setWindowFlags(widget->windowFlags() | Qt::FramelessWindowHint);
#endif
        }
    }

    if (qstrcmp(widget->metaObject()->className(),"QComboBoxPrivateContainer") == 0)
        widget->installEventFilter(this);

    if (QAbstractScrollArea *scrollArea = qobject_cast<QAbstractScrollArea*>(widget)) {
        scrollArea->viewport()->setAutoFillBackground(false);
        if (QAbstractItemView *view = qobject_cast<QAbstractItemView*>(scrollArea)) {
            view->setHorizontalScrollMode(QAbstractItemView::ScrollPerPixel);
            view->setVerticalScrollMode(QAbstractItemView::ScrollPerPixel);
        }
        QScroller::grabGesture(scrollArea->viewport(), QScroller::LeftMouseButtonGesture);
    }

    if (qobject_cast<QScrollBar*>(widget))
        widget->setAttribute(Qt::WA_OpaquePaintEvent, false);

    QCommonStyle::polish(widget);
}

void QPixmapStyle::unpolish(QWidget *widget)
{
    if (qobject_cast<QSlider*>(widget) ||
            qobject_cast<QComboBox*>(widget)) {
        widget->removeEventFilter(this);
    }

    if (qstrcmp(widget->metaObject()->className(),"QComboBoxPrivateContainer") == 0)
        widget->removeEventFilter(this);

    if (QAbstractScrollArea *scrollArea = qobject_cast<QAbstractScrollArea*>(widget))
        QScroller::ungrabGesture(scrollArea->viewport());

    QCommonStyle::unpolish(widget);
}

void QPixmapStyle::drawPrimitive(PrimitiveElement element, const QStyleOption *option,
                                 QPainter *painter, const QWidget *widget) const
{
    switch (element) {
    case PE_FrameFocusRect: //disable focus rectangle
        break;
    case PE_PanelButtonBevel:
    case PE_PanelButtonCommand:
        drawPushButton(option, painter, widget);
        break;
    case PE_PanelLineEdit:
    case PE_FrameLineEdit:
        drawLineEdit(option, painter, widget);
        break;
    case PE_Frame:
    case PE_FrameDefaultButton:
        if (qobject_cast<const QTextEdit*>(widget))
            drawTextEdit(option, painter, widget);
        break;
    case PE_IndicatorCheckBox:
        drawCheckBox(option, painter, widget);
        break;
    case PE_IndicatorRadioButton:
        drawRadioButton(option, painter, widget);
        break;
    case PE_PanelItemViewItem:
        if (qobject_cast<const QListView*>(widget))
            drawPanelItemViewItem(option, painter, widget);
        else
            QCommonStyle::drawPrimitive(element, option, painter, widget);
        break;
    default:
        QCommonStyle::drawPrimitive(element, option, painter, widget);
    }
}

void QPixmapStyle::drawControl(ControlElement element, const QStyleOption *option,
                               QPainter *painter, const QWidget *widget) const
{
    switch (element) {
    case CE_ProgressBarGroove:
        drawProgressBarBackground(option, painter, widget);
        break;
    case CE_ProgressBarLabel:
        drawProgressBarLabel(option, painter, widget);
        break;
    case CE_ProgressBarContents:
        drawProgressBarFill(option, painter, widget);
        break;
    case CE_ShapedFrame:
        // NOTE: This will break if the private API of QComboBox changes drastically
        if (qstrcmp(widget->metaObject()->className(),"QComboBoxPrivateContainer") == 0) {
            const Descriptor &desc = m_descriptors.value(DD_PopupDown);
            const Pixmap &pix = m_pixmaps.value(DD_ItemSeparator);
            QRect rect = option->rect;
            rect.adjust(-pix.margins.left(), -desc.margins.top(),
                        pix.margins.right(), desc.margins.bottom());
            bool up = widget->property("_pixmapstyle_combobox_up").toBool();
            drawCachedPixmap(up ? DD_PopupUp : DD_PopupDown, rect, painter);
        }
        else {
            QCommonStyle::drawControl(element, option, painter, widget);
        }
        break;
    default:
        QCommonStyle::drawControl(element, option, painter, widget);
    }
}

void QPixmapStyle::drawComplexControl(ComplexControl cc, const QStyleOptionComplex *option,
                                      QPainter *painter, const QWidget *widget) const
{
    switch (cc) {
    case CC_Slider:
        drawSlider(option, painter, widget);
        break;
    case CC_ComboBox:
        drawComboBox(option, painter, widget);
        break;
    case CC_ScrollBar:
        drawScrollBar(option, painter, widget);
        break;
    default:
        QCommonStyle::drawComplexControl(cc, option, painter, widget);
    }
}

QSize QPixmapStyle::sizeFromContents(ContentsType type, const QStyleOption *option,
                                     const QSize &contentsSize, const QWidget *widget) const
{
    switch (type) {
    case CT_PushButton:
        return pushButtonSizeFromContents(option, contentsSize, widget);
    case CT_LineEdit:
        return lineEditSizeFromContents(option, contentsSize, widget);
    case CT_ProgressBar:
        return progressBarSizeFromContents(option, contentsSize, widget);
    case CT_Slider:
        return sliderSizeFromContents(option, contentsSize, widget);
    case CT_ComboBox:
        return comboBoxSizeFromContents(option, contentsSize, widget);
    case CT_ItemViewItem:
        return itemViewSizeFromContents(option, contentsSize, widget);
    default: ;
    }

    return QCommonStyle::sizeFromContents(type, option, contentsSize, widget);
}

QRect QPixmapStyle::subElementRect(SubElement element, const QStyleOption *option,
                                   const QWidget *widget) const
{
    switch (element) {
    case SE_LineEditContents:
    {
        QRect rect = QCommonStyle::subElementRect(element, option, widget);
        const Descriptor &desc = m_descriptors.value(LE_Enabled);
        rect.adjust(desc.margins.left(), desc.margins.top(),
                    -desc.margins.right(), -desc.margins.bottom());
        rect = visualRect(option->direction, option->rect, rect);
        return rect;
    }
    default: ;
    }

    return QCommonStyle::subElementRect(element, option, widget);
}

QRect QPixmapStyle::subControlRect(ComplexControl cc, const QStyleOptionComplex *option,
                                   SubControl sc, const QWidget *widget) const
{
    switch (cc) {
    case CC_ComboBox:
        return comboBoxSubControlRect(option, sc, widget);
    case CC_ScrollBar:
        return scrollBarSubControlRect(option, sc, widget);
    default: ;
    }

    return QCommonStyle::subControlRect(cc, option, sc, widget);
}

int QPixmapStyle::pixelMetric(PixelMetric metric, const QStyleOption *option,
                              const QWidget *widget) const
{
    switch (metric) {
    case PM_ButtonShiftHorizontal:
    case PM_ButtonShiftVertical:
        return 0;
    case PM_DefaultFrameWidth:
        if (qobject_cast<const QTextEdit*>(widget)) {
            const Descriptor &desc = m_descriptors.value(LE_Enabled);
            return qMax(qMax(desc.margins.left(), desc.margins.right()),
                        qMax(desc.margins.top(), desc.margins.bottom()));
        }
        return 0;
    case PM_IndicatorWidth:
        return m_pixmaps.value(CB_Enabled).pixmap.width();
    case PM_IndicatorHeight:
        return m_pixmaps.value(CB_Enabled).pixmap.height();
    case PM_CheckBoxLabelSpacing:
    {
        const Pixmap &pix = m_pixmaps.value(CB_Enabled);
        return qMax(qMax(pix.margins.left(), pix.margins.right()),
                    qMax(pix.margins.top(), pix.margins.bottom()));
    }
    case PM_ExclusiveIndicatorWidth:
        return m_pixmaps.value(RB_Enabled).pixmap.width();
    case PM_ExclusiveIndicatorHeight:
        return m_pixmaps.value(RB_Enabled).pixmap.height();
    case PM_RadioButtonLabelSpacing:
    {
        const Pixmap &pix = m_pixmaps.value(RB_Enabled);
        return qMax(qMax(pix.margins.left(), pix.margins.right()),
                    qMax(pix.margins.top(), pix.margins.bottom()));
    }
    case PM_SliderThickness:
        if (const QStyleOptionSlider *slider =
                    qstyleoption_cast<const QStyleOptionSlider*>(option)) {
            const Descriptor desc = m_descriptors.value(slider->orientation == Qt::Horizontal
                                                        ? SG_HEnabled : SG_VEnabled);
            return slider->orientation == Qt::Horizontal
                                                ? desc.size.height() : desc.size.width();
        }
        break;
    case PM_SliderControlThickness:
        if (const QStyleOptionSlider *slider =
                    qstyleoption_cast<const QStyleOptionSlider*>(option)) {
            const Pixmap pix = m_pixmaps.value(slider->orientation == Qt::Horizontal
                                               ? SH_HEnabled : SH_VEnabled);
            return slider->orientation == Qt::Horizontal
                                                ? pix.pixmap.height() : pix.pixmap.width();
        }
        break;
    case PM_SliderLength:
        if (const QStyleOptionSlider *slider =
                    qstyleoption_cast<const QStyleOptionSlider*>(option)) {
            const Pixmap pix = m_pixmaps.value(slider->orientation == Qt::Horizontal
                                                ? SH_HEnabled : SH_VEnabled);
            return slider->orientation == Qt::Horizontal
                                                ? pix.pixmap.width() : pix.pixmap.height();
        }
        break;
    case PM_ScrollBarExtent:
        if (const QStyleOptionSlider *slider =
                    qstyleoption_cast<const QStyleOptionSlider*>(option)) {
            const Descriptor desc = m_descriptors.value(slider->orientation == Qt::Horizontal
                                                        ? SB_Horizontal : SB_Vertical);
            return slider->orientation == Qt::Horizontal
                                                ? desc.size.height() : desc.size.width();
        }
        break;
    case PM_ScrollBarSliderMin:
        return 0;
    default: ;
    }

    return QCommonStyle::pixelMetric(metric, option, widget);
}

int QPixmapStyle::styleHint(StyleHint hint, const QStyleOption *option,
                            const QWidget *widget, QStyleHintReturn *returnData) const
{
    switch (hint) {
    case SH_EtchDisabledText:
        return false;
    case SH_ComboBox_Popup:
        return false;
    default: ;
    }

    return QCommonStyle::styleHint(hint, option, widget, returnData);
}

QStyle::SubControl QPixmapStyle::hitTestComplexControl(QStyle::ComplexControl control,
                                                       const QStyleOptionComplex *option,
                                                       const QPoint &pos,
                                                       const QWidget *widget) const
{
    const SubControl sc = QCommonStyle::hitTestComplexControl(control, option, pos, widget);
    if (control == CC_ScrollBar) {
        if (sc == SC_ScrollBarAddLine)
            return SC_ScrollBarAddPage;
        else if (sc == SC_ScrollBarSubLine)
            return SC_ScrollBarSubPage;
    }

    return sc;
}

bool QPixmapStyle::eventFilter(QObject *watched, QEvent *event)
{
    if (QSlider *slider = qobject_cast<QSlider*>(watched)) {
        switch (event->type()) {
        case QEvent::MouseButtonPress:
        case QEvent::MouseButtonRelease:
        case QEvent::MouseMove:
            slider->update();
            break;
        default: ;
        }
    }

    if (QComboBox *comboBox = qobject_cast<QComboBox*>(watched)) {
        switch (event->type()) {
        case QEvent::MouseButtonPress:
            event->ignore();
            comboBox->setProperty("_pixmapstyle_combobox_pressed", true);
            comboBox->repaint();
            return true;
        case QEvent::MouseButtonRelease:
            comboBox->setProperty("_pixmapstyle_combobox_pressed", false);
            comboBox->repaint();
            if ( comboBox->view() ) {
                if ( comboBox->view()->isVisible() || (!comboBox->isEnabled()))
                    comboBox->hidePopup();
                else
                    comboBox->showPopup();
            }
            break;
        default: ;
        }
    }

    if (qstrcmp(watched->metaObject()->className(),"QComboBoxPrivateContainer") == 0) {
        if (event->type() == QEvent::Show) {
            QWidget *widget = qobject_cast<QWidget*>(watched);
            int yPopup = widget->geometry().top();
            int yCombo = widget->parentWidget()->mapToGlobal(QPoint(0, 0)).y();
            QRect geom = widget->geometry();
            const Descriptor &desc = m_descriptors.value(DD_ButtonEnabled);
            const bool up = yPopup < yCombo;
            geom.moveTop(geom.top() + (up ? desc.margins.top() : -desc.margins.bottom()));
            widget->setGeometry(geom);
            widget->setProperty("_pixmapstyle_combobox_up", up);
            widget->parentWidget()->setProperty("_pixmapstyle_combobox_up", up);
        }
    }

    return QCommonStyle::eventFilter(watched, event);
}

void QPixmapStyle::addDescriptor(QPixmapStyle::ControlDescriptor control, const QString &fileName,
                                 QMargins margins, QTileRules tileRules)
{
    Descriptor desc;

    QImage image(fileName);
    if (image.isNull())
        return;

    desc.fileName = fileName;
    desc.margins = margins;
    desc.tileRules = tileRules;
    desc.size = image.size();

    m_descriptors[control] = desc;
}

void QPixmapStyle::copyDescriptor(QPixmapStyle::ControlDescriptor source,
                                  QPixmapStyle::ControlDescriptor dest)
{
    m_descriptors[dest] = m_descriptors.value(source);
}

void QPixmapStyle::drawCachedPixmap(QPixmapStyle::ControlDescriptor control, const QRect &rect,
                                    QPainter *p) const
{
    if (!m_descriptors.contains(control))
        return;
    const Descriptor &desc = m_descriptors.value(control);
    const QPixmap pix = getCachedPixmap(control, desc, rect.size());
    Q_ASSERT(!pix.isNull());
    p->drawPixmap(rect, pix);
}

void QPixmapStyle::addPixmap(ControlPixmap control, const QString &fileName,
                             QMargins margins)
{
    Pixmap pix;

    QPixmap image(fileName);
    if (image.isNull())
        return;

    pix.pixmap = image;
    pix.margins = margins;

    m_pixmaps[control] = pix;
}

void QPixmapStyle::copyPixmap(QPixmapStyle::ControlPixmap source, QPixmapStyle::ControlPixmap dest)
{
    m_pixmaps[dest] = m_pixmaps.value(source);
}

void QPixmapStyle::drawPushButton(const QStyleOption *option,
                                  QPainter *painter, const QWidget *) const
{
    const bool checked = option->state & State_On;
    const bool pressed = option->state & State_Sunken;
    const bool enabled = option->state & State_Enabled;

    ControlDescriptor control = PB_Enabled;
    if (enabled)
        control = pressed ? PB_Pressed : (checked ? PB_Checked : PB_Enabled);
    else
        control = checked ? PB_PressedDisabled : PB_Disabled;
    drawCachedPixmap(control, option->rect, painter);
}

void QPixmapStyle::drawLineEdit(const QStyleOption *option,
                                QPainter *painter, const QWidget *widget) const
{
    // Don't draw for the line edit inside a combobox
    if (widget && qobject_cast<const QComboBox*>(widget->parentWidget()))
        return;

    const bool enabled = option->state & State_Enabled;
    const bool focused = option->state & State_HasFocus;
    ControlDescriptor control = enabled ? (focused ? LE_Focused : LE_Enabled) : LE_Disabled;
    drawCachedPixmap(control, option->rect, painter);
}

void QPixmapStyle::drawTextEdit(const QStyleOption *option,
                                QPainter *painter, const QWidget *) const
{
    const bool enabled = option->state & State_Enabled;
    const bool focused = option->state & State_HasFocus;
    ControlDescriptor control = enabled ? (focused ? TE_Focused : TE_Enabled) : TE_Disabled;
    drawCachedPixmap(control, option->rect, painter);
}

void QPixmapStyle::drawCheckBox(const QStyleOption *option,
                                QPainter *painter, const QWidget *) const
{
    const QStyleOptionButton *button = qstyleoption_cast<const QStyleOptionButton*>(option);

    const bool down = button->state & State_Sunken;
    const bool enabled = button->state & State_Enabled;
    const bool on = button->state & State_On;

    ControlPixmap control;
    if (enabled)
        control = on ? (down ? CB_PressedChecked : CB_Checked) : (down ? CB_Pressed : CB_Enabled);
    else
        control = on ? CB_DisabledChecked : CB_Disabled;
    painter->drawPixmap(button->rect, m_pixmaps.value(control).pixmap);
}

void QPixmapStyle::drawRadioButton(const QStyleOption *option,
                                   QPainter *painter, const QWidget *) const
{
    const QStyleOptionButton *button = qstyleoption_cast<const QStyleOptionButton*>(option);

    const bool down = button->state & State_Sunken;
    const bool enabled = button->state & State_Enabled;
    const bool on = button->state & State_On;

    ControlPixmap control;
    if (enabled)
        control = on ? RB_Checked : (down ? RB_Pressed : RB_Enabled);
    else
        control = on ? RB_DisabledChecked : RB_Disabled;
    painter->drawPixmap(button->rect, m_pixmaps.value(control).pixmap);
}

void QPixmapStyle::drawPanelItemViewItem(const QStyleOption *option, QPainter *painter,
                                         const QWidget *widget) const
{
    ControlPixmap cp = ID_Separator;
    ControlDescriptor cd = ID_Selected;

    if (widget && widget->property("_pixmap_combobox_list").toBool()) {
        cp = DD_ItemSeparator;
        cd = DD_ItemSelected;
    }

    QPixmap pix = m_pixmaps.value(cp).pixmap;
    QRect rect = option->rect;
    rect.setBottom(rect.top() + pix.height()-1);
    painter->drawPixmap(rect, pix);
    if (option->state & QStyle::State_Selected) {
        rect = option->rect;
        rect.setTop(rect.top() + pix.height());
        drawCachedPixmap(cd, rect, painter);
    }
}

void QPixmapStyle::drawProgressBarBackground(const QStyleOption *option,
                                             QPainter *painter, const QWidget *) const
{
    bool vertical = false;
    if (const QStyleOptionProgressBar *pb =
            qstyleoption_cast<const QStyleOptionProgressBar *>(option)) {
        vertical = pb->orientation == Qt::Vertical;
    }
    drawCachedPixmap(vertical ? PB_VBackground : PB_HBackground, option->rect, painter);
}

void QPixmapStyle::drawProgressBarLabel(const QStyleOption *option,
                                        QPainter *painter, const QWidget *) const
{
    if (const QStyleOptionProgressBar *pb =
                    qstyleoption_cast<const QStyleOptionProgressBar *>(option)) {
        const bool vertical = pb->orientation == Qt::Vertical;
        if (!vertical) {
            QPalette::ColorRole textRole = QPalette::ButtonText;
            proxy()->drawItemText(painter, pb->rect,
                                  Qt::AlignCenter | Qt::TextSingleLine, pb->palette,
                                  pb->state & State_Enabled, pb->text, textRole);
        }
    }
}

void QPixmapStyle::drawProgressBarFill(const QStyleOption *option,
                                       QPainter *painter, const QWidget *) const
{
    const QStyleOptionProgressBar *pbar =
                qstyleoption_cast<const QStyleOptionProgressBar*>(option);
    const bool vertical = pbar->orientation == Qt::Vertical;
    const bool flip = (pbar->direction == Qt::RightToLeft) ^ pbar->invertedAppearance;

    if (pbar->progress == pbar->maximum) {
        drawCachedPixmap(vertical ? PB_VComplete : PB_HComplete, option->rect, painter);

    } else {
        if (pbar->progress == 0)
            return;
        const int maximum = pbar->maximum;
        const qreal ratio = qreal(vertical?option->rect.height():option->rect.width())/maximum;
        const int progress = pbar->progress*ratio;

        QRect optRect = option->rect;
        if (vertical) {
            if (flip)
                optRect.setBottom(optRect.top()+progress-1);
            else
                optRect.setTop(optRect.bottom()-progress+1);
        } else {
            if (flip)
                optRect.setLeft(optRect.right()-progress+1);
            else
                optRect.setRight(optRect.left()+progress-1);
        }

        drawCachedPixmap(vertical ? PB_VContent : PB_HContent, optRect, painter);
    }
}

void QPixmapStyle::drawSlider(const QStyleOptionComplex *option,
                              QPainter *painter, const QWidget *widget) const
{
    const QStyleOptionSlider *slider = qstyleoption_cast<const QStyleOptionSlider*>(option);
    if (!slider)
        return;

    const bool enabled = option->state & State_Enabled;
    const bool pressed = option->state & State_Sunken;
    const Qt::Orientation orient = slider->orientation;

    const QRect handle = proxy()->subControlRect(CC_Slider, option, SC_SliderHandle, widget);
    if (option->subControls & SC_SliderGroove) {
        QRect groove = proxy()->subControlRect(CC_Slider, option, SC_SliderGroove, widget);
        if (groove.isValid()) {
            // Draw the background
            ControlDescriptor control;
            if (orient == Qt::Horizontal)
                control = enabled ? SG_HEnabled : SG_HDisabled;
            else
                control = enabled ? SG_VEnabled : SG_VDisabled;
            drawCachedPixmap(control, groove, painter);

            // Draw the active part
            if (orient == Qt::Horizontal) {
                control = enabled ? (pressed ? SG_HActivePressed : SG_HActiveEnabled )
                                  : SG_HActiveDisabled;
            } else {
                control = enabled ? (pressed ? SG_VActivePressed : SG_VActiveEnabled )
                                  : SG_VActiveDisabled;
            }
            const Descriptor &desc = m_descriptors.value(control);
            const QPixmap pix = getCachedPixmap(control, desc, groove.size());
            if (!pix.isNull()) {
                groove.setRight(orient == Qt::Horizontal
                               ? handle.center().x() : handle.center().y());
                painter->drawPixmap(groove, pix, groove);
            }
        }
    }
    if (option->subControls & SC_SliderHandle) {
        if (handle.isValid()) {
            ControlPixmap pix;
            if (orient == Qt::Horizontal)
                pix = enabled ? (pressed ? SH_HPressed : SH_HEnabled) : SH_HDisabled;
            else
                pix = enabled ? (pressed ? SH_VPressed : SH_VEnabled) : SH_VDisabled;
            painter->drawPixmap(handle, m_pixmaps.value(pix).pixmap);
        }
    }
}

void QPixmapStyle::drawComboBox(const QStyleOptionComplex *option,
                                QPainter *painter, const QWidget *widget) const
{
    const bool enabled = option->state & State_Enabled;
    const bool pressed = widget->property("_pixmapstyle_combobox_pressed").toBool();
    const bool opened = option->state & State_On;

    ControlDescriptor control =
        enabled ? (pressed ? DD_ButtonPressed : DD_ButtonEnabled) : DD_ButtonDisabled;
    drawCachedPixmap(control, option->rect, painter);

    ControlPixmap cp = enabled ? (opened ? DD_ArrowOpen
                                    : (pressed ? DD_ArrowPressed : DD_ArrowEnabled))
                                    : DD_ArrowDisabled;
    Pixmap pix = m_pixmaps.value(cp);
    QRect rect = comboBoxSubControlRect(option, SC_ComboBoxArrow, widget);
    painter->drawPixmap(rect, pix.pixmap);
}

void QPixmapStyle::drawScrollBar(const QStyleOptionComplex *option,
                                 QPainter *painter, const QWidget *widget) const
{
    if (const QStyleOptionSlider *slider =
                    qstyleoption_cast<const QStyleOptionSlider*>(option)) {
        // Do not draw the scrollbar
        if (slider->minimum == slider->maximum)
            return;

        QRect rect = scrollBarSubControlRect(option, SC_ScrollBarSlider, widget);
        ControlDescriptor control = slider->orientation == Qt::Horizontal
                ? SB_Horizontal : SB_Vertical;
        drawCachedPixmap(control, rect, painter);
    }
}

QSize QPixmapStyle::pushButtonSizeFromContents(const QStyleOption *option,
                                               const QSize &contentsSize,
                                               const QWidget *widget) const
{
    const Descriptor &desc = m_descriptors.value(PB_Enabled);
    const int bm = proxy()->pixelMetric(PM_ButtonMargin, option, widget);

    int w = contentsSize.width();
    int h = contentsSize.height();
    w += desc.margins.left() + desc.margins.right() + bm;
    h += desc.margins.top() + desc.margins.bottom() + bm;

    return computeSize(desc, w, h);
}

QSize QPixmapStyle::lineEditSizeFromContents(const QStyleOption *,
                                             const QSize &contentsSize, const QWidget *) const
{
    const Descriptor &desc = m_descriptors.value(LE_Enabled);
    const int border = 2 * proxy()->pixelMetric(PM_DefaultFrameWidth);

    int w = contentsSize.width() + border + desc.margins.left() + desc.margins.right();
    int h = contentsSize.height() + border + desc.margins.top() + desc.margins.bottom();

    return computeSize(desc, w, h);
}

QSize QPixmapStyle::progressBarSizeFromContents(const QStyleOption *option,
                                                const QSize &contentsSize,
                                                const QWidget *widget) const
{
    bool vertical = false;
    if (const QStyleOptionProgressBar *pb =
                    qstyleoption_cast<const QStyleOptionProgressBar *>(option)) {
        vertical = pb->orientation == Qt::Vertical;
    }
    QSize result = QCommonStyle::sizeFromContents(CT_Slider, option, contentsSize, widget);
    if (vertical) {
        const Descriptor desc = m_descriptors.value(PB_VBackground);
        return QSize(desc.size.height(), result.height());
    } else {
        const Descriptor desc = m_descriptors.value(PB_HBackground);
        return QSize(result.width(), desc.size.height());
    }
}

QSize QPixmapStyle::sliderSizeFromContents(const QStyleOption *option,
                                           const QSize &contentsSize,
                                           const QWidget *widget) const
{
    const QStyleOptionSlider *slider = qstyleoption_cast<const QStyleOptionSlider*>(option);
    if (!slider)
        return QSize();

    QSize result = QCommonStyle::sizeFromContents(CT_Slider, option, contentsSize, widget);

    const Descriptor desc = m_descriptors.value(slider->orientation == Qt::Horizontal
                                                ? SG_HEnabled : SG_VEnabled);

    if (slider->orientation == Qt::Horizontal)
        return QSize(result.width(), desc.size.height());
    else
        return QSize(desc.size.width(), result.height());
}

QSize QPixmapStyle::comboBoxSizeFromContents(const QStyleOption *option,
                                             const QSize &contentsSize,
                                             const QWidget *widget) const
{
    const Descriptor &desc = m_descriptors.value(DD_ButtonEnabled);

    QSize result = QCommonStyle::sizeFromContents(CT_ComboBox, option, contentsSize, widget);
    return computeSize(desc, result.width(), result.height());
}

QSize QPixmapStyle::itemViewSizeFromContents(const QStyleOption *option,
                                             const QSize &contentsSize,
                                             const QWidget *widget) const
{
    QSize size = QCommonStyle::sizeFromContents(CT_ItemViewItem, option, contentsSize, widget);

    ControlPixmap cp = ID_Separator;
    ControlDescriptor cd = ID_Selected;
    if (widget && widget->property("_pixmap_combobox_list").toBool()) {
        cp = DD_ItemSeparator;
        cd = DD_ItemSelected;
    }

    const Descriptor &desc = m_descriptors.value(cd);
    const Pixmap &pix = m_pixmaps.value(cp);
    size.setHeight(qMax(size.height(),
                        desc.size.height() + pix.pixmap.height()));
    return size;
}

QRect QPixmapStyle::comboBoxSubControlRect(const QStyleOptionComplex *option,
                                           QStyle::SubControl sc, const QWidget *) const
{
    QRect r = option->rect; // Default size
    const Pixmap &pix = m_pixmaps.value(DD_ArrowEnabled);
    const Descriptor &desc = m_descriptors.value(DD_ButtonEnabled);

    switch (sc) {
    case SC_ComboBoxArrow:
        r.setRect(r.right() - pix.margins.right() - pix.pixmap.width(),
                    r.top() + pix.margins.top(),
                    pix.pixmap.width(), pix.pixmap.height());
        break;
    case SC_ComboBoxEditField:
        r.adjust(desc.margins.left(), desc.margins.right(),
                 -desc.margins.right(), -desc.margins.bottom());
        r.setRight(r.right() - pix.margins.right() - pix.margins.left() - pix.pixmap.width());
        break;
    default:
        break;
    }

    r = visualRect(option->direction, option->rect, r);
    return r;
}

QRect QPixmapStyle::scrollBarSubControlRect(const QStyleOptionComplex *option,
                                            QStyle::SubControl sc, const QWidget *) const
{
    if (const QStyleOptionSlider *slider =
                qstyleoption_cast<const QStyleOptionSlider*>(option)) {
        int length = (slider->orientation == Qt::Horizontal)
                ? slider->rect.width() : slider->rect.height();
        int page = length * slider->pageStep
                / (slider->maximum - slider->minimum + slider->pageStep);
        int pos = length * slider->sliderValue
                / (slider->maximum - slider->minimum + slider->pageStep);
        pos = qMin(pos+page, length) - page;

        QRect rect = slider->rect;

        if (slider->orientation == Qt::Horizontal) {
            switch (sc) {
            case SC_ScrollBarAddPage:
                rect.setLeft(pos+page);
                return rect;
            case SC_ScrollBarSubPage:
                rect.setRight(pos);
                return rect;
            case SC_ScrollBarGroove:
                return rect;
            case SC_ScrollBarSlider:
                rect.setLeft(pos);
                rect.setRight(pos+page);
                return rect;
            default: ;
            }
        } else {
            switch (sc) {
            case SC_ScrollBarAddPage:
                rect.setTop(pos+page);
                return rect;
            case SC_ScrollBarSubPage:
                rect.setBottom(pos);
                return rect;
            case SC_ScrollBarGroove:
                return rect;
            case SC_ScrollBarSlider:
                rect.setTop(pos);
                rect.setBottom(pos+page);
                return rect;
            default: ;
            }
        }
    }
    return QRect();
}

static QPixmap scale(int w, int h, const QPixmap &pixmap, const QPixmapStyle::Descriptor &desc)
{
    QPixmap result(w, h);
    {
        const QColor transparent(0, 0, 0, 0);
        result.fill( transparent );
        QPainter p( &result );
        const QMargins margins = desc.margins;
        qDrawBorderPixmap(&p, result.rect(), margins, pixmap,
                          pixmap.rect(), margins, desc.tileRules);
    }
    return result;
}

QPixmap QPixmapStyle::getCachedPixmap(ControlDescriptor control, const Descriptor &desc,
                                      const QSize &size) const
{
    const QString sizeString = QString::number(size.width()) % QLatin1Char('*')
            % QString::number(size.height());
    const QString key = QLatin1String(metaObject()->className()) % QString::number(control)
            % QLatin1Char('@') % sizeString;

    QPixmap result;

    if (!QPixmapCache::find( key, &result)) {
        QPixmap source(desc.fileName);
        result = scale(size.width(), size.height(), source, desc);
        QPixmapCache::insert(key, result);
    }
    return result;
}

QSize QPixmapStyle::computeSize(const QPixmapStyle::Descriptor &desc, int width, int height) const
{
    if (desc.tileRules.horizontal != Qt::RepeatTile)
        width = qMax(width, desc.size.width());
    if (desc.tileRules.vertical != Qt::RepeatTile)
        height = qMax(height, desc.size.height());
    return QSize(width, height);
}

QT_END_NAMESPACE
