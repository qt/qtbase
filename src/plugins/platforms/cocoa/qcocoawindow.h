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

#ifndef QCOCOAWINDOW_H
#define QCOCOAWINDOW_H

#include <Cocoa/Cocoa.h>

#include <QPlatformWindow>

#include "qcocoaglcontext.h"
#include "qnsview.h"

QT_BEGIN_NAMESPACE

class QCocoaWindow : public QPlatformWindow
{
public:
    QCocoaWindow(QWindow *tlw);
    ~QCocoaWindow();

    void setGeometry(const QRect &rect);
    void setVisible(bool visible);
    void setWindowTitle(const QString &title);
    void raise();
    void lower();

    WId winId() const;
    NSView *contentView() const;

    void windowDidResize();

    QPlatformGLContext *glContext() const;

private:
    friend class QCocoaWindowSurface;
    NSWindow *m_nsWindow;
    QNSView *m_contentView;
    NSView *m_windowSurfaceView;
    mutable QCocoaGLContext *m_glContext;
};

QT_END_NAMESPACE

#endif // QCOCOAWINDOW_H

