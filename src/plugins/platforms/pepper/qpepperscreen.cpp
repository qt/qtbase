/****************************************************************************
**
** Copyright (C) 2014 Digia Plc
** All rights reserved.
** For any questions to Digia, please use contact form at http://qt.digia.com <http://qt.digia.com/>
**
** This file is part of the Qt Native Client platform plugin.
**
** No Commercial Usage
** This file contains pre-release code and may not be distributed.
** You may use this file in accordance with the terms and conditions
** contained in the Technology Preview License Agreement accompanying
** this package.
**
** If you have questions regarding the use of this file, please use
** contact form at http://qt.digia.com <http://qt.digia.com/>
**
****************************************************************************/

#include "qpepperscreen.h"
#include "qpepperhelpers.h"
#include "qpepperintegration.h"
#include "qpepperinstance_p.h"
#include "qpeppercursor.h"

#include <qdebug.h>

QT_BEGIN_NAMESPACE

// Qt running on NaCl/pepper has one screen, which corresponds to the <embed> element
// that contains the application instance. Geometry is reported to the instance via
// DidChangeView. Instance geometry has a position, which is the position of the <embed>
// element on the page. Screen geometry always has a position of (0, 0). Screen size
// is equal to instance size. Pepper event geometry is in screen coordinates: relative
// to then top-left (0,0). Top-level windows are positioned in screen geometry as usual.

QPepperScreen::QPepperScreen()
    : m_depth(32)
    , m_format(QImage::Format_ARGB32_Premultiplied)
    , m_cursor(new QPepperCursor)
{
}

QRect QPepperScreen::geometry() const
{
    return QRect(QPoint(), QPepperInstancePrivate::get()->geometry().size());
}

qreal QPepperScreen::devicePixelRatio() const
{
    return QPepperInstancePrivate::get()->devicePixelRatio();
}

QPlatformCursor *QPepperScreen::cursor() const { return m_cursor.data(); }

void QPepperScreen::resizeMaximizedWindows() { QPlatformScreen::resizeMaximizedWindows(); }

QT_END_NAMESPACE
