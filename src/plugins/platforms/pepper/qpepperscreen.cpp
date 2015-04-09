/****************************************************************************
**
** Copyright (C) 2015 Digia Plc and/or its subsidiary(-ies).
** Contact: http://www.qt-project.org/legal
**
** This file is part of the Qt for Native Client platform plugin.
**
** $QT_BEGIN_LICENSE:LGPL3$
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
** General Public License version 3 as published by the Free Software
** Foundation and appearing in the file LICENSE.LGPLv3 included in the
** packaging of this file. Please review the following information to
** ensure the GNU Lesser General Public License version 3 requirements
** will be met: https://www.gnu.org/licenses/lgpl.html.
**
** GNU General Public License Usage
** Alternatively, this file may be used under the terms of the GNU
** General Public License version 2.0 or later as published by the Free
** Software Foundation and appearing in the file LICENSE.GPL included in
** the packaging of this file. Please review the following information to
** ensure the GNU General Public License version 2.0 requirements will be
** met: http://www.gnu.org/licenses/gpl-2.0.html.
** $QT_END_LICENSE$
**
****************************************************************************/

#include "qpepperscreen.h"

#include "qpeppercursor.h"
#include "qpepperhelpers.h"
#include "qpepperinstance_p.h"
#include "qpepperintegration.h"

#include <QtCore/QDebug>

QT_BEGIN_NAMESPACE

// Qt running on NaCl/pepper has one screen, which corresponds to the <embed> element
// that contains the application instance. Geometry is reported to the instance via
// DidChangeView. Instance geometry has a position, which is the position of the <embed>
// element on the page. Screen geometry always has a position of (0, 0). Screen size
// is equal to instance size. Pepper event geometry is in screen coordinates: relative
// to then top-left (0,0). Top-level windows are positioned in screen geometry as usual.

QPepperScreen::QPepperScreen()
    : m_cursor(new QPepperCursor)
{
}

QRect QPepperScreen::geometry() const
{
    return QRect(QPoint(), QPepperInstancePrivate::get()->geometry().size());
}

int QPepperScreen::depth() const { return 32; }

QImage::Format QPepperScreen::format() const { return QImage::Format_ARGB32_Premultiplied; }

qreal QPepperScreen::devicePixelRatio() const
{
    return QPepperInstancePrivate::get()->devicePixelRatio();
}

QPlatformCursor *QPepperScreen::cursor() const { return m_cursor.data(); }

void QPepperScreen::resizeMaximizedWindows() { QPlatformScreen::resizeMaximizedWindows(); }

QT_END_NAMESPACE
