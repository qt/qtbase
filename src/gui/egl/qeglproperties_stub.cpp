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

#include <QtCore/qdebug.h>
#include <QtCore/qstringlist.h>

#include "qeglproperties_p.h"
#include "qeglcontext_p.h"

QT_BEGIN_NAMESPACE

static void noegl(const char *fn)
{
    qWarning() << fn << " called, but Qt configured without EGL" << endl;
}

#define NOEGL noegl(__FUNCTION__);

// Initialize a property block.
QEglProperties::QEglProperties()
{
    NOEGL
}

QEglProperties::QEglProperties(EGLConfig cfg)
{
    Q_UNUSED(cfg)
    NOEGL
}

// Fetch the current value associated with a property.
int QEglProperties::value(int name) const
{
    Q_UNUSED(name)
    NOEGL
    return 0;
}

// Set the value associated with a property, replacing an existing
// value if there is one.
void QEglProperties::setValue(int name, int value)
{
    Q_UNUSED(name)
    Q_UNUSED(value)
    NOEGL
}

// Remove a property value.  Returns false if the property is not present.
bool QEglProperties::removeValue(int name)
{
    Q_UNUSED(name)
    NOEGL
    return false;
}

void QEglProperties::setDeviceType(int devType)
{
    Q_UNUSED(devType)
    NOEGL
}


// Sets the red, green, blue, and alpha sizes based on a pixel format.
// Normally used to match a configuration request to the screen format.
void QEglProperties::setPixelFormat(QImage::Format pixelFormat)
{
    Q_UNUSED(pixelFormat)
    NOEGL

}

void QEglProperties::setRenderableType(QEgl::API api)
{
    Q_UNUSED(api);
    NOEGL
}

// Reduce the complexity of a configuration request to ask for less
// because the previous request did not result in success.  Returns
// true if the complexity was reduced, or false if no further
// reductions in complexity are possible.
bool QEglProperties::reduceConfiguration()
{
    NOEGL
    return false;
}

static void addTag(QString& str, const QString& tag)
{
    Q_UNUSED(str)
    Q_UNUSED(tag)
    NOEGL
}

// Convert a property list to a string suitable for debug output.
QString QEglProperties::toString() const
{
    NOEGL
    return QString();
}

void QEglProperties::setPaintDeviceFormat(QPaintDevice *dev)
{
    Q_UNUSED(dev)
    NOEGL
}

QT_END_NAMESPACE


