/****************************************************************************
**
** Copyright (C) 2016 The Qt Company Ltd.
** Contact: https://www.qt.io/licensing/
**
** This file is part of the plugins of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:LGPL$
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company. For licensing terms
** and conditions see https://www.qt.io/terms-conditions. For further
** information use the contact form at https://www.qt.io/contact-us.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 3 as published by the Free Software
** Foundation and appearing in the file LICENSE.LGPL3 included in the
** packaging of this file. Please review the following information to
** ensure the GNU Lesser General Public License version 3 requirements
** will be met: https://www.gnu.org/licenses/lgpl-3.0.html.
**
** GNU General Public License Usage
** Alternatively, this file may be used under the terms of the GNU
** General Public License version 2.0 or (at your option) the GNU General
** Public license version 3 or any later version approved by the KDE Free
** Qt Foundation. The licenses are as published by the Free Software
** Foundation and appearing in the file LICENSE.GPL2 and LICENSE.GPL3
** included in the packaging of this file. Please review the following
** information to ensure the GNU General Public License requirements will
** be met: https://www.gnu.org/licenses/gpl-2.0.html and
** https://www.gnu.org/licenses/gpl-3.0.html.
**
** $QT_END_LICENSE$
**
****************************************************************************/

#ifndef QIOSWINDOW_H
#define QIOSWINDOW_H

#include <qpa/qplatformwindow.h>
#include <qpa/qwindowsysteminterface.h>

#import <UIKit/UIKit.h>

class QIOSContext;
class QIOSWindow;

@class QUIView;

QT_BEGIN_NAMESPACE

class QIOSWindow : public QObject, public QPlatformWindow
{
    Q_OBJECT

public:
    explicit QIOSWindow(QWindow *window);
    ~QIOSWindow();

    void setGeometry(const QRect &rect) Q_DECL_OVERRIDE;

    void setWindowState(Qt::WindowState state) Q_DECL_OVERRIDE;
    void setParent(const QPlatformWindow *window) Q_DECL_OVERRIDE;
    void handleContentOrientationChange(Qt::ScreenOrientation orientation) Q_DECL_OVERRIDE;
    void setVisible(bool visible) Q_DECL_OVERRIDE;
    void setOpacity(qreal level) Q_DECL_OVERRIDE;

    bool isExposed() const Q_DECL_OVERRIDE;
    void propagateSizeHints() Q_DECL_OVERRIDE {}

    void raise() Q_DECL_OVERRIDE{ raiseOrLower(true); }
    void lower() Q_DECL_OVERRIDE { raiseOrLower(false); }

    bool shouldAutoActivateWindow() const;
    void requestActivateWindow() Q_DECL_OVERRIDE;

    qreal devicePixelRatio() const Q_DECL_OVERRIDE;

    bool setMouseGrabEnabled(bool grab) Q_DECL_OVERRIDE { return grab; }
    bool setKeyboardGrabEnabled(bool grab) Q_DECL_OVERRIDE { return grab; }

    WId winId() const Q_DECL_OVERRIDE { return WId(m_view); }

    void clearAccessibleCache();

    QSurfaceFormat format() const Q_DECL_OVERRIDE;

    void requestUpdate() Q_DECL_OVERRIDE;

    CAEAGLLayer *eaglLayer() const;

private:
    void applicationStateChanged(Qt::ApplicationState state);
    void applyGeometry(const QRect &rect);

    QUIView *m_view;

    QRect m_normalGeometry;
    int m_windowLevel;

    void raiseOrLower(bool raise);
    void updateWindowLevel();
    bool blockedByModal();

    friend class QIOSScreen;
};

QT_END_NAMESPACE

#endif // QIOSWINDOW_H
