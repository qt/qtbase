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

#ifndef QPIXMAPSTYLE_H
#define QPIXMAPSTYLE_H

#include <QCommonStyle>
#include <QString>
#include <QPixmap>
#include <QMargins>
#include <QTileRules>
#include <QHash>
#include <QPainter>

QT_BEGIN_NAMESPACE

class QPixmapStyle : public QCommonStyle
{
    Q_OBJECT

public:
    struct Descriptor {
        QString fileName;
        QSize size;
        QMargins margins;
        QTileRules tileRules;
    };

    enum ControlDescriptor {
        BG_Background=0,
        LE_Enabled,             // QLineEdit
        LE_Disabled,
        LE_Focused,
        PB_Enabled,             // QPushButton
        PB_Pressed,
        PB_PressedDisabled,
        PB_Checked,
        PB_Disabled,
        TE_Enabled,             // QTextEdit
        TE_Disabled,
        TE_Focused,
        PB_HBackground,         // Horizontal QProgressBar
        PB_HContent,
        PB_HComplete,
        PB_VBackground,         // Vertical QProgressBar
        PB_VContent,
        PB_VComplete,
        SG_HEnabled,            // Horizontal QSlider groove
        SG_HDisabled,
        SG_HActiveEnabled,
        SG_HActivePressed,
        SG_HActiveDisabled,
        SG_VEnabled,            // Vertical QSlider groove
        SG_VDisabled,
        SG_VActiveEnabled,
        SG_VActivePressed,
        SG_VActiveDisabled,
        DD_ButtonEnabled,       // QComboBox (DropDown)
        DD_ButtonDisabled,
        DD_ButtonPressed,
        DD_PopupDown,
        DD_PopupUp,
        DD_ItemSelected,
        ID_Selected,            // QStyledItemDelegate
        SB_Horizontal,          // QScrollBar
        SB_Vertical
    };

    struct Pixmap {
        QPixmap pixmap;
        QMargins margins;
    };
    enum ControlPixmap {
        CB_Enabled,             // QCheckBox
        CB_Checked,
        CB_Pressed,
        CB_PressedChecked,
        CB_Disabled,
        CB_DisabledChecked,
        RB_Enabled,             // QRadioButton
        RB_Checked,
        RB_Pressed,
        RB_Disabled,
        RB_DisabledChecked,
        SH_HEnabled,            // Horizontal QSlider handle
        SH_HDisabled,
        SH_HPressed,
        SH_VEnabled,            // Vertical QSlider handle
        SH_VDisabled,
        SH_VPressed,
        DD_ArrowEnabled,        // QComboBox (DropDown) arrow
        DD_ArrowDisabled,
        DD_ArrowPressed,
        DD_ArrowOpen,
        DD_ItemSeparator,
        ID_Separator            // QStyledItemDelegate separator
    };

public:
    QPixmapStyle();
    ~QPixmapStyle();

    void polish(QApplication *application);
    void polish(QPalette &palette);
    void polish(QWidget *widget);
    void unpolish(QWidget *widget);

    void drawPrimitive(PrimitiveElement element, const QStyleOption *option,
            QPainter *painter, const QWidget *widget = 0) const;
    void drawControl(ControlElement element, const QStyleOption *option,
            QPainter *painter, const QWidget *widget = 0) const;
    void drawComplexControl(ComplexControl cc, const QStyleOptionComplex *option,
                            QPainter *painter, const QWidget *widget=0) const;

    QSize sizeFromContents(ContentsType type, const QStyleOption *option,
            const QSize &contentsSize, const QWidget *widget = 0) const;
    QRect subElementRect(SubElement element, const QStyleOption *option,
            const QWidget *widget = 0) const;
    QRect subControlRect(ComplexControl cc, const QStyleOptionComplex *option,
                         SubControl sc, const QWidget *widget = 0) const;

    int pixelMetric(PixelMetric metric, const QStyleOption *option = 0,
            const QWidget *widget = 0) const;
    int styleHint(StyleHint hint, const QStyleOption *option,
                  const QWidget *widget, QStyleHintReturn *returnData) const;
    SubControl hitTestComplexControl(ComplexControl control, const QStyleOptionComplex *option,
                                     const QPoint &pos, const QWidget *widget) const;

    bool eventFilter(QObject *watched, QEvent *event);

protected:
    void addDescriptor(ControlDescriptor control, const QString &fileName,
                       QMargins margins = QMargins(),
                       QTileRules tileRules = QTileRules(Qt::RepeatTile, Qt::RepeatTile));
    void copyDescriptor(ControlDescriptor source, ControlDescriptor dest);
    void drawCachedPixmap(ControlDescriptor control, const QRect &rect, QPainter *p) const;

    void addPixmap(ControlPixmap control, const QString &fileName,
                   QMargins margins = QMargins());
    void copyPixmap(ControlPixmap source, ControlPixmap dest);

    void drawPushButton(const QStyleOption *option,
                        QPainter *painter, const QWidget *widget) const;
    void drawLineEdit(const QStyleOption *option,
                      QPainter *painter, const QWidget *widget) const;
    void drawTextEdit(const QStyleOption *option,
                      QPainter *painter, const QWidget *widget) const;
    void drawCheckBox(const QStyleOption *option,
                      QPainter *painter, const QWidget *widget) const;
    void drawRadioButton(const QStyleOption *option,
                         QPainter *painter, const QWidget *widget) const;
    void drawPanelItemViewItem(const QStyleOption *option,
                               QPainter *painter, const QWidget *widget) const;
    void drawProgressBarBackground(const QStyleOption *option,
                                   QPainter *painter, const QWidget *widget) const;
    void drawProgressBarLabel(const QStyleOption *option,
                              QPainter *painter, const QWidget *widget) const;
    void drawProgressBarFill(const QStyleOption *option,
                             QPainter *painter, const QWidget *widget) const;
    void drawSlider(const QStyleOptionComplex *option,
                    QPainter *painter, const QWidget *widget) const;
    void drawComboBox(const QStyleOptionComplex *option,
                      QPainter *painter, const QWidget *widget) const;
    void drawScrollBar(const QStyleOptionComplex *option,
                       QPainter *painter, const QWidget *widget) const;

    QSize pushButtonSizeFromContents(const QStyleOption *option,
                                     const QSize &contentsSize, const QWidget *widget) const;
    QSize lineEditSizeFromContents(const QStyleOption *option,
                                   const QSize &contentsSize, const QWidget *widget) const;
    QSize progressBarSizeFromContents(const QStyleOption *option,
                                      const QSize &contentsSize, const QWidget *widget) const;
    QSize sliderSizeFromContents(const QStyleOption *option,
                                 const QSize &contentsSize, const QWidget *widget) const;
    QSize comboBoxSizeFromContents(const QStyleOption *option,
                                   const QSize &contentsSize, const QWidget *widget) const;
    QSize itemViewSizeFromContents(const QStyleOption *option,
                                   const QSize &contentsSize, const QWidget *widget) const;

    QRect comboBoxSubControlRect(const QStyleOptionComplex *option,
                                 SubControl sc, const QWidget *widget) const;
    QRect scrollBarSubControlRect(const QStyleOptionComplex *option,
                                  SubControl sc, const QWidget *widget) const;

private:
    QPixmap getCachedPixmap(ControlDescriptor control,
                            const Descriptor &desc, const QSize &size) const;

    QSize computeSize(const Descriptor &desc, int width, int height) const;

private:
    QHash<ControlDescriptor, Descriptor> m_descriptors;
    QHash<ControlPixmap, Pixmap> m_pixmaps;
};

QT_END_NAMESPACE

#endif // QPIXMAPSTYLE_H
