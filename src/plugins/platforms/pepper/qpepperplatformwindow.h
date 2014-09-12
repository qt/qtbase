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

#ifndef QPEPPERPLATFORMWINDOW_H
#define QPEPPERPLATFORMWINDOW_H

#include <qpa/qplatformwindow.h>

#include "qpepperhelpers.h"
#include "qpeppercompositor.h"
#include "qpepperintegration.h"

QT_BEGIN_NAMESPACE

Q_DECLARE_LOGGING_CATEGORY(QT_PLATFORM_PEPPER_WINDOW)

class QPepperWindowSurface;
class QPepperGLContext;
class QPepperPlatformWindow : public QPlatformWindow
{
public:
    QPepperPlatformWindow(QWindow *window);
    ~QPepperPlatformWindow();

    WId winId() const;
    void setVisible(bool visible);
    void setWindowState(Qt::WindowState state);
    void raise();
    void lower();
    void setGeometry(const QRect &rect);
    void setParent(const QPlatformWindow *window);

    bool setKeyboardGrabEnabled(bool grab);
    bool setMouseGrabEnabled(bool grab);

    qreal devicePixelRatio() const;

    bool m_isVisible;
    bool m_trackInstanceSize;
    bool m_useCompositor;
private:
    QPepperIntegration *m_pepperIntegration;
    QPepperCompositor *m_compositor;
};

QT_END_NAMESPACE

#endif
