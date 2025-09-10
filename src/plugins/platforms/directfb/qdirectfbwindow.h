// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

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
    ~QDirectFbWindow() override;

    void setGeometry(const QRect &rect) override;
    void setOpacity(qreal level) override;

    void setVisible(bool visible) override;

    void setWindowFlags(Qt::WindowFlags flags) override;
    bool setKeyboardGrabEnabled(bool grab) override;
    bool setMouseGrabEnabled(bool grab) override;
    void raise() override;
    void lower() override;
    WId winId() const override;

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
