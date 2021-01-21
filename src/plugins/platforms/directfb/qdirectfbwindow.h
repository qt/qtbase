/****************************************************************************
**
** Copyright (C) 2021 The Qt Company Ltd.
** Contact: https://www.qt.io/licensing/
**
** This file is part of the plugins of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:COMM$
**
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company. For licensing terms
** and conditions see https://www.qt.io/terms-conditions. For further
** information use the contact form at https://www.qt.io/contact-us.
**
** $QT_END_LICENSE$
**
**
**
**
**
**
**
**
**
**
**
**
**
**
**
**
**
**
**
****************************************************************************/

#ifndef QDIRECTFBWINDOW_H
#define QDIRECTFBWINDOW_H

#include <qpa/qplatformwindow.h>

#include "qdirectfbconvenience.h"
#include "qdirectfbinput.h"

QT_BEGIN_NAMESPACE

class QDirectFbWindow : public QPlatformWindow
{
public:
    QDirectFbWindow(QWindow *tlw, QDirectFbInput *inputhandler);
    ~QDirectFbWindow();

    void setGeometry(const QRect &rect);
    void setOpacity(qreal level);

    void setVisible(bool visible);

    void setWindowFlags(Qt::WindowFlags flags);
    bool setKeyboardGrabEnabled(bool grab);
    bool setMouseGrabEnabled(bool grab);
    void raise();
    void lower();
    WId winId() const;

    virtual void createDirectFBWindow();
    IDirectFBWindow *dfbWindow() const;

    // helper to get access to DirectFB types
    IDirectFBSurface *dfbSurface();

protected:
    QDirectFBPointer<IDirectFBSurface> m_dfbSurface;
    QDirectFBPointer<IDirectFBWindow> m_dfbWindow;
    QDirectFbInput *m_inputHandler;
};

QT_END_NAMESPACE

#endif // QDIRECTFBWINDOW_H
