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

#include <qstylehints.h>
#include <qpa/qplatformintegration.h>
#include <private/qguiapplication_p.h>

QT_BEGIN_NAMESPACE

static inline QVariant hint(QPlatformIntegration::StyleHint h)
{
    return QGuiApplicationPrivate::platformIntegration()->styleHint(h);
}

/*!
    \class QStyleHints
    \since 5.0
    \brief The QStyleHints contains platform specific hints and settings.
 */
QStyleHints::QStyleHints()
    : QObject()
{
}


int QStyleHints::mouseDoubleClickInterval() const
{
    return hint(QPlatformIntegration::MouseDoubleClickInterval).toInt();
}

int QStyleHints::startDragDistance() const
{
    return hint(QPlatformIntegration::StartDragDistance).toInt();
}

int QStyleHints::startDragTime() const
{
    return hint(QPlatformIntegration::StartDragTime).toInt();
}

int QStyleHints::startDragVelocity() const
{
    return hint(QPlatformIntegration::StartDragVelocity).toInt();
}

int QStyleHints::keyboardInputInterval() const
{
    return hint(QPlatformIntegration::KeyboardInputInterval).toInt();
}

int QStyleHints::keyboardAutoRepeatRate() const
{
    return hint(QPlatformIntegration::KeyboardAutoRepeatRate).toInt();
}

int QStyleHints::cursorFlashTime() const
{
    return hint(QPlatformIntegration::CursorFlashTime).toInt();
}

bool QStyleHints::showIsFullScreen() const
{
    return hint(QPlatformIntegration::ShowIsFullScreen).toBool();
}

int QStyleHints::passwordMaskDelay() const
{
    return hint(QPlatformIntegration::PasswordMaskDelay).toInt();
}

qreal QStyleHints::fontSmoothingGamma() const
{
    return hint(QPlatformIntegration::FontSmoothingGamma).toReal();
}

QT_END_NAMESPACE
