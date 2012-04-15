/****************************************************************************
**
** Copyright (C) 2012 Nokia Corporation and/or its subsidiary(-ies).
** Contact: http://www.qt-project.org/
**
** This file is part of the QtGui module of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:LGPL$
** GNU Lesser General Public License Usage
** This file may be used under the terms of the GNU Lesser General Public
** License version 2.1 as published by the Free Software Foundation and
** appearing in the file LICENSE.LGPL included in the packaging of this
** file. Please review the following information to ensure the GNU Lesser
** General Public License version 2.1 requirements will be met:
** http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, Nokia gives you certain additional
** rights. These rights are described in the Nokia Qt LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
** GNU General Public License Usage
** Alternatively, this file may be used under the terms of the GNU General
** Public License version 3.0 as published by the Free Software Foundation
** and appearing in the file LICENSE.GPL included in the packaging of this
** file. Please review the following information to ensure the GNU General
** Public License version 3.0 requirements will be met:
** http://www.gnu.org/copyleft/gpl.html.
**
** Other Usage
** Alternatively, this file may be used in accordance with the terms and
** conditions contained in a signed written agreement between you and Nokia.
**
**
**
**
**
**
** $QT_END_LICENSE$
**
****************************************************************************/

#include "qplatformscreenpageflipper_qpa.h"

QT_BEGIN_NAMESPACE

/*!
    \class QPlatformScreenBuffer
    \since 5.0
    \internal
    \preliminary
    \ingroup qpa

    \brief The QPlatformScreenBuffer class provides an abstraction for screen buffers.
 */
QPlatformScreenBuffer::QPlatformScreenBuffer()
    : m_destroyed(false)
    , m_ready(true)
{

}

QPlatformScreenBuffer::~QPlatformScreenBuffer()
{

}

bool QPlatformScreenBuffer::isDestroyed() const
{
    return m_destroyed;
}

bool QPlatformScreenBuffer::isReady() const
{
    return m_ready;
}

void QPlatformScreenBuffer::aboutToBeDisplayed()
{
}

void QPlatformScreenBuffer::displayed()
{
}


/*!
    \class QPlatformScreenPageFlipper
    \since 5.0
    \internal
    \preliminary
    \ingroup qpa

    \brief The QPlatformScreenPageFlipper class provides an abstraction for flipping the screen
    page.
 */

QPlatformScreenPageFlipper::QPlatformScreenPageFlipper(QObject *parent)
    :QObject(parent)
{

}

QT_END_NAMESPACE

