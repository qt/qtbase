/****************************************************************************
**
** Copyright (C) 2012 Nokia Corporation and/or its subsidiary(-ies).
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
**
** $QT_END_LICENSE$
**
****************************************************************************/

#ifndef QIOSSCREEN_H
#define QIOSSCREEN_H

#include <UIKit/UIKit.h>

#include <qpa/qplatformscreen.h>

QT_BEGIN_NAMESPACE

class QIOSScreen : public QPlatformScreen
{
public:
    QIOSScreen(unsigned int screenIndex);
    ~QIOSScreen();

    enum ScreenIndex { MainScreen = 0 };

    QRect geometry() const { return m_geometry; }
    int depth() const { return m_depth; }
    QImage::Format format() const { return m_format; }
    QSizeF physicalSize() const { return m_physicalSize; }

    UIScreen *uiScreen() const;

    void updateInterfaceOrientation();
private:
    UIScreen *m_uiScreen;
    QRect m_geometry;
    int m_depth;
    QImage::Format m_format;
    QSize m_physicalSize;
};

QT_END_NAMESPACE

#endif
