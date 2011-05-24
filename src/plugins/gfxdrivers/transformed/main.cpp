/****************************************************************************
**
** Copyright (C) 2011 Nokia Corporation and/or its subsidiary(-ies).
** All rights reserved.
** Contact: Nokia Corporation (qt-info@nokia.com)
**
** This file is part of the plugins of the Qt Toolkit.
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
** $QT_END_LICENSE$
**
****************************************************************************/

#include <qscreendriverplugin_qws.h>
#include <qscreentransformed_qws.h>
#include <qstringlist.h>
#ifndef QT_NO_LIBRARY
QT_BEGIN_NAMESPACE

class GfxTransformedDriver : public QScreenDriverPlugin
{
public:
    GfxTransformedDriver();

    QStringList keys() const;
    QScreen *create(const QString&, int displayId);
};

GfxTransformedDriver::GfxTransformedDriver()
: QScreenDriverPlugin()
{
}

QStringList GfxTransformedDriver::keys() const
{
    QStringList list;
    list << "Transformed";
    return list;
}

QScreen* GfxTransformedDriver::create(const QString& driver, int displayId)
{
#ifndef QT_NO_QWS_TRANSFORMED
    if (driver.toLower() == "transformed")
        return new QTransformedScreen(displayId);
#else //QT_NO_QWS_TRANSFORMED
    printf("QT buildt with QT_NO_QWS_TRANSFORMED. No screen driver returned\n");
#endif //QT_NO_QWS_TRANSFORMED
    return 0;
}

Q_EXPORT_STATIC_PLUGIN(GfxTransformedDriver)
Q_EXPORT_PLUGIN2(qgfxtransformed, GfxTransformedDriver)

QT_END_NAMESPACE
#endif //QT_NO_LIBRARY
