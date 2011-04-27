/****************************************************************************
**
** Copyright (C) 2011 Nokia Corporation and/or its subsidiary(-ies).
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

#ifndef QPLATFORMSCREEN_H
#define QPLATFORMSCREEN_H

#include <QtCore/qmetatype.h>
#include <QtCore/qnamespace.h>
#include <QtCore/qcoreevent.h>
#include <QtCore/qvariant.h>
#include <QtCore/qrect.h>
#include <QtCore/qobject.h>

#include <QtGui/qcursor.h>
#include <QtGui/qimage.h>
#include <QtGui/qwidget.h>

QT_BEGIN_HEADER

QT_BEGIN_NAMESPACE

QT_MODULE(Gui)

class Q_GUI_EXPORT QPlatformScreen : public QObject
{
    Q_OBJECT
public:
    virtual ~QPlatformScreen() { }

    virtual QRect geometry() const = 0;
    virtual QRect availableGeometry() const {return geometry();}
    virtual int depth() const = 0;
    virtual QImage::Format format() const = 0;
    virtual QSize physicalSize() const;
    //jl: should setDirty be removed.
    virtual void setDirty(const QRect &) {}
    virtual QWidget *topLevelAt(const QPoint &point) const;

    //jl: should this function be in QPlatformIntegration
    //jl: maybe screenForWidget is a better name?
    static QPlatformScreen *platformScreenForWidget(const QWidget *widget);
};

QT_END_NAMESPACE

QT_END_HEADER

#endif // QPLATFORMSCREEN_H
