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

#ifndef QDIRECTFBCONVENIENCE_H
#define QDIRECTFBCONVENIENCE_H

#include <QtGui/qimage.h>
#include <QtCore/QHash>
#include <QtCore/QEvent>
#include <QtGui/QPixmap>

#include <directfb.h>

class QDirectFbKeyMap: public QHash<DFBInputDeviceKeySymbol, Qt::Key>
{
public:
    QDirectFbKeyMap();
};


class QDirectFbConvenience
{
public:
    static QImage::Format imageFormatFromSurfaceFormat(const DFBSurfacePixelFormat format, const DFBSurfaceCapabilities caps);
    static bool pixelFomatHasAlpha(const DFBSurfacePixelFormat format) { return (1 << 16) & format; }
    static int colorDepthForSurface(const DFBSurfacePixelFormat format);

    //This is set by the graphicssystem constructor
    static IDirectFB *dfbInterface();
    static IDirectFBDisplayLayer *dfbDisplayLayer(int display = DLID_PRIMARY);

    static IDirectFBSurface *dfbSurfaceForPixmapData(QPixmapData *);

    static Qt::MouseButton mouseButton(DFBInputDeviceButtonIdentifier identifier);
    static Qt::MouseButtons mouseButtons(DFBInputDeviceButtonMask mask);
    static Qt::KeyboardModifiers keyboardModifiers(DFBInputDeviceModifierMask mask);
    static QEvent::Type eventType(DFBWindowEventType type);

    static QDirectFbKeyMap *keyMap();

private:
    static QDirectFbKeyMap *dfbKeymap;
    friend class QDirectFbIntegration;
};

#endif // QDIRECTFBCONVENIENCE_H
