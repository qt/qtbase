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

#ifndef QPLATFORMINTEGRATION_DIRECTFB_H
#define QPLATFORMINTEGRATION_DIRECTFB_H

#include "qdirectfbinput.h"

#include <QtGui/QPlatformIntegration>
#include <directfb.h>
#include <directfb_version.h>

QT_BEGIN_NAMESPACE

class QThread;
class QDirectFBCursor;

class QDirectFbScreen : public QPlatformScreen
{
Q_OBJECT
public:
    QDirectFbScreen(int display);
    ~QDirectFbScreen();

    QRect geometry() const { return m_geometry; }
    int depth() const { return m_depth; }
    QImage::Format format() const { return m_format; }
    QSize physicalSize() const { return m_physicalSize; }

public:
    QRect m_geometry;
    int m_depth;
    QImage::Format m_format;
    QSize m_physicalSize;

    IDirectFBDisplayLayer *m_layer;

private:
    QDirectFBCursor * cursor;

};

class QDirectFbIntegration : public QPlatformIntegration
{
public:
    QDirectFbIntegration();
    ~QDirectFbIntegration();

    QPixmapData *createPixmapData(QPixmapData::PixelType type) const;
    QPlatformWindow *createPlatformWindow(QWidget *widget, WId winId = 0) const;
    QWindowSurface *createWindowSurface(QWidget *widget, WId winId) const;

    QList<QPlatformScreen *> screens() const { return mScreens; }

    QPlatformFontDatabase *fontDatabase() const;

private:
    QList<QPlatformScreen *> mScreens;
    QDirectFbInput *mInput;
    QThread *mInputRunner;
    QPlatformFontDatabase *mFontDb;
};

QT_END_NAMESPACE

#endif
