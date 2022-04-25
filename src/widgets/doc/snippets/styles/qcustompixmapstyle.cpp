/***************************************************************************
**
** Copyright (C) 2016 The Qt Company Ltd.
** Contact: https://www.qt.io/licensing/
**
** This file is part of the plugins of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:LGPL$
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company. For licensing terms
** and conditions see https://www.qt.io/terms-conditions. For further
** information use the contact form at https://www.qt.io/contact-us.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 3 as published by the Free Software
** Foundation and appearing in the file LICENSE.LGPL3 included in the
** packaging of this file. Please review the following information to
** ensure the GNU Lesser General Public License version 3 requirements
** will be met: https://www.gnu.org/licenses/lgpl-3.0.html.
**
** GNU General Public License Usage
** Alternatively, this file may be used under the terms of the GNU
** General Public License version 2.0 or (at your option) the GNU General
** Public license version 3 or any later version approved by the KDE Free
** Qt Foundation. The licenses are as published by the Free Software
** Foundation and appearing in the file LICENSE.GPL2 and LICENSE.GPL3
** included in the packaging of this file. Please review the following
** information to ensure the GNU General Public License requirements will
** be met: https://www.gnu.org/licenses/gpl-2.0.html and
** https://www.gnu.org/licenses/gpl-3.0.html.
**
** $QT_END_LICENSE$
**
****************************************************************************/

#include "qcustompixmapstyle.h"

#include <QtGui>

using namespace Qt::StringLiterals;

//! [0]
QCustomPixmapStyle::QCustomPixmapStyle() :
    QPixmapStyle()
{
//! [1]
    addDescriptor(PB_Enabled,
                  "://button/core_button_inactive.png"_L1,
                  QMargins(13, 13, 13, 13),
                  QTileRules(Qt::RepeatTile, Qt::StretchTile));
//! [1]
    addDescriptor(PB_Checked,
                  "://button/core_button_enabled_selected.png"_L1,
                  QMargins(13, 13, 13, 13),
                  QTileRules(Qt::RepeatTile, Qt::StretchTile));
    addDescriptor(PB_Pressed,
                  "://button/core_button_pressed.png"_L1,
                  QMargins(13, 13, 13, 13),
                  QTileRules(Qt::RepeatTile, Qt::StretchTile));
    addDescriptor(PB_Disabled,
                  "://button/core_button_disabled.png"_L1,
                  QMargins(13, 13, 13, 13),
                  QTileRules(Qt::RepeatTile, Qt::StretchTile));
    addDescriptor(PB_PressedDisabled,
                  "://button/core_button_disabled_selected.png"_L1,
                  QMargins(13, 13, 13, 13),
                  QTileRules(Qt::RepeatTile, Qt::StretchTile));

//! [2]
    addDescriptor(LE_Enabled,
                  "://lineedit/core_textinput_bg.png"_L1,
                  QMargins(8, 8, 8, 8));
    addDescriptor(LE_Disabled,
                  "://lineedit/core_textinput_bg_disabled.png"_L1,
                  QMargins(8, 8, 8, 8));
    addDescriptor(LE_Focused,
                  "://lineedit/core_textinput_bg_highlight.png"_L1,
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
