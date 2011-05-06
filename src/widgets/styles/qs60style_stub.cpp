/****************************************************************************
**
** Copyright (C) 2009 Nokia Corporation and/or its subsidiary(-ies).
** All rights reserved.
** Contact: Nokia Corporation (qt-info@nokia.com)
**
** This file is part of the QtGui module of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:LGPL$
** No Commercial Usage
** This file contains pre-release code and may not be distributed.
** You may use this file in accordance with the terms and conditions
** contained in the Technology Preview License Agreement accompanying
** this package.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 2.1 as published by the Free Software
** Foundation and appearing in the file LICENSE.LGPL included in the
** packaging of this file.  Please review the following information to
** ensure the GNU Lesser General Public License version 2.1 requirements
** will be met: http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, Nokia gives you certain additional
** rights.  These rights are described in the Nokia Qt LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
** If you have questions regarding the use of this file, please contact
** Nokia at qt-info@nokia.com.
**
**
**
**
**
**
**
**
** $QT_END_LICENSE$
**
****************************************************************************/

#include "qs60style.h"
#include "qdebug.h"

#if defined(QT_NO_STYLE_S60)
QT_BEGIN_NAMESPACE

QS60Style::QS60Style()
{
    qWarning() << "QS60Style stub created";
}

QS60Style::~QS60Style()
{
}

void QS60Style::drawComplexControl(ComplexControl , const QStyleOptionComplex *, QPainter *, const QWidget *) const
{
}

void QS60Style::drawControl(ControlElement , const QStyleOption *, QPainter *, const QWidget *) const
{
}

void QS60Style::drawPrimitive(PrimitiveElement , const QStyleOption *, QPainter *, const QWidget *) const
{
}

int QS60Style::pixelMetric(PixelMetric , const QStyleOption *, const QWidget *) const
{
    return 0;
}

QSize QS60Style::sizeFromContents(ContentsType , const QStyleOption *, const QSize &, const QWidget *) const
{
    return QSize();
}

int QS60Style::styleHint(StyleHint , const QStyleOption *, const QWidget *, QStyleHintReturn *) const
{
    return 0;
}

QRect QS60Style::subControlRect(ComplexControl , const QStyleOptionComplex *, SubControl , const QWidget *) const
{
    return QRect();
}

QRect QS60Style::subElementRect(SubElement , const QStyleOption *, const QWidget *) const
{
    return QRect();
}

void QS60Style::polish(QWidget *)
{
}

void QS60Style::unpolish(QWidget *)
{
}

void QS60Style::polish(QApplication *)
{
}

void QS60Style::unpolish(QApplication *)
{
}

bool QS60Style::event(QEvent *)
{
    return false;
}

QIcon QS60Style::standardIconImplementation(StandardPixmap , const QStyleOption *, const QWidget *) const
{
    return QIcon();
}

void QS60Style::timerEvent(QTimerEvent *)
{
}

bool QS60Style::eventFilter(QObject *, QEvent *)
{
    return false;
}

QT_END_NAMESPACE

#endif // QT_NO_STYLE_S60
