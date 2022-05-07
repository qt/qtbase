/***************************************************************************
**
** Copyright (C) 2022 The Qt Company Ltd.
** Contact: https://www.qt.io/licensing/
**
** This file is part of the plugins of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:COMM$
**
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company. For licensing terms
** and conditions see https://www.qt.io/terms-conditions. For further
** information use the contact form at https://www.qt.io/contact-us.
**
** $QT_END_LICENSE$
**
**
**
**
**
**
**
**
**
**
**
**
**
**
**
**
**
**
**
******************************************************************************/

#include "qcustompixmapstyle.h"

#include <QtGui>

//! [0]
QCustomPixmapStyle::QCustomPixmapStyle() :
    QPixmapStyle()
{
//! [1]
    addDescriptor(PB_Enabled,
                  QLatin1String("://button/core_button_inactive.png"),
                  QMargins(13, 13, 13, 13),
                  QTileRules(Qt::RepeatTile, Qt::StretchTile));
//! [1]
    addDescriptor(PB_Checked,
                  QLatin1String("://button/core_button_enabled_selected.png"),
                  QMargins(13, 13, 13, 13),
                  QTileRules(Qt::RepeatTile, Qt::StretchTile));
    addDescriptor(PB_Pressed,
                  QLatin1String("://button/core_button_pressed.png"),
                  QMargins(13, 13, 13, 13),
                  QTileRules(Qt::RepeatTile, Qt::StretchTile));
    addDescriptor(PB_Disabled,
                  QLatin1String("://button/core_button_disabled.png"),
                  QMargins(13, 13, 13, 13),
                  QTileRules(Qt::RepeatTile, Qt::StretchTile));
    addDescriptor(PB_PressedDisabled,
                  QLatin1String("://button/core_button_disabled_selected.png"),
                  QMargins(13, 13, 13, 13),
                  QTileRules(Qt::RepeatTile, Qt::StretchTile));

//! [2]
    addDescriptor(LE_Enabled,
                  QLatin1String("://lineedit/core_textinput_bg.png"),
                  QMargins(8, 8, 8, 8));
    addDescriptor(LE_Disabled,
                  QLatin1String("://lineedit/core_textinput_bg_disabled.png"),
                  QMargins(8, 8, 8, 8));
    addDescriptor(LE_Focused,
                  QLatin1String("://lineedit/core_textinput_bg_highlight.png"),
                  QMargins(8, 8, 8, 8));

    copyDescriptor(LE_Enabled, TE_Enabled);
    copyDescriptor(LE_Disabled, TE_Disabled);
    copyDescriptor(LE_Focused, TE_Focused);
//! [2]
}
//! [0]

QCustomPixmapStyle::~QCustomPixmapStyle()
{
}
