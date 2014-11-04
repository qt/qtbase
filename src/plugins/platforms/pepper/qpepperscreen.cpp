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
#include "qpepperinstance.h"
#include "qpeppercursor.h"

#include <qdebug.h>

QT_BEGIN_NAMESPACE

// Qt running on NaCl/pepper has one "screen", which
// corresponds to the <div> rag that contains the application.
// Windows can then be show "fullscreen" and occupy the
// entire tag.
QPepperScreen::QPepperScreen()
: m_depth(32)
, m_format(QImage::Format_ARGB32_Premultiplied)
, m_cursor(new QPepperCursor)
{

}

QRect QPepperScreen::geometry() const
{
    // Note that the pepper instance geometry usually has a non-zero postion.
    return QPepperInstance::get()->geometry();
}

qreal QPepperScreen::devicePixelRatio() const
{
    return QPepperInstance::get()->devicePixelRatio();
}

QPlatformCursor *QPepperScreen::cursor() const
{
    return m_cursor.data();
}

void QPepperScreen::resizeMaximizedWindows()
{
    QPlatformScreen::resizeMaximizedWindows();
}

QT_END_NAMESPACE
