/****************************************************************************
**
** Copyright (C) 2012 Nokia Corporation and/or its subsidiary(-ies).
** All rights reserved.
** Contact: http://www.qt-project.org/
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

#ifndef QGRAPHICSSYSTEM_TESTLITE_H
#define QGRAPHICSSYSTEM_TESTLITE_H

//make sure textstream is included before any X11 headers
#include <QtCore/QTextStream>

#include <QtGui/QPlatformIntegration>
#include <QtGui/QPlatformScreen>
#include <QtGui/QPlatformNativeInterface>

#include "qxlibstatic.h"

QT_BEGIN_NAMESPACE

class QXlibScreen;

class QXlibIntegration : public QPlatformIntegration
{
public:
    QXlibIntegration();

    bool hasCapability(Capability cap) const;

    QPlatformWindow *createPlatformWindow(QWindow *window) const;
    QPlatformBackingStore *createPlatformBackingStore(QWindow *window) const;
    QPlatformOpenGLContext *createPlatformOpenGLContext(QOpenGLContext *context) const;

    QAbstractEventDispatcher *guiThreadEventDispatcher() const;

    QPixmap grabWindow(WId window, int x, int y, int width, int height) const;

    QList<QPlatformScreen *> screens() const { return mScreens; }

    QPlatformFontDatabase *fontDatabase() const;
    QPlatformClipboard *clipboard() const;

    QPlatformNativeInterface *nativeInterface() const;

private:
    QXlibScreen *mPrimaryScreen;
    QList<QPlatformScreen *> mScreens;
    QPlatformFontDatabase *mFontDb;
    QPlatformClipboard *mClipboard;
    QPlatformNativeInterface *mNativeInterface;
    QAbstractEventDispatcher *mEventDispatcher;
};

QT_END_NAMESPACE

#endif
