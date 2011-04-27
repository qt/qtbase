/****************************************************************************
**
** Copyright (C) 2011 Nokia Corporation and/or its subsidiary(-ies).
** All rights reserved.
** Contact: Nokia Corporation (qt-info@nokia.com)
**
** This file is part of the plugins of the Qt Toolkit.
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

#ifndef QGRAPHICSSYSTEM_QVFB_H
#define QGRAPHICSSYSTEM_QVFB_H

#include <QPlatformScreen>
#include <QPlatformIntegration>

QT_BEGIN_NAMESPACE


class QVFbScreenPrivate;

class QVFbScreen : public QPlatformScreen
{
public:
    QVFbScreen(int id);
    ~QVFbScreen();

    QRect geometry() const;
     int depth() const;
     QImage::Format format() const;
     QSize physicalSize() const;

    QImage *screenImage();

    void setDirty(const QRect &rect);

public:

    QVFbScreenPrivate *d_ptr;
};

class QVFbIntegrationPrivate;


class QVFbIntegration : public QPlatformIntegration
{
public:
    QVFbIntegration(const QStringList &paramList);

    QPixmapData *createPixmapData(QPixmapData::PixelType type) const;
    QPlatformWindow *createPlatformWindow(QWidget *widget, WId winId) const;
    QWindowSurface *createWindowSurface(QWidget *widget, WId winId) const;

    QList<QPlatformScreen *> screens() const { return mScreens; }

    QPlatformFontDatabase *fontDatabase() const;

private:
    QVFbScreen *mPrimaryScreen;
    QList<QPlatformScreen *> mScreens;
    QPlatformFontDatabase *mFontDb;
};



QT_END_NAMESPACE


#endif
