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

    void setGeometry(const QRect &rect) override;

    void setWindowState(Qt::WindowStates state) override;
    void setParent(const QPlatformWindow *window) override;
    void handleContentOrientationChange(Qt::ScreenOrientation orientation) override;
    void setVisible(bool visible) override;
    void setOpacity(qreal level) override;

    bool isExposed() const override;
    void propagateSizeHints() override {}

    QMargins safeAreaMargins() const override;

    void raise() override{ raiseOrLower(true); }
    void lower() override { raiseOrLower(false); }

    bool shouldAutoActivateWindow() const;
    void requestActivateWindow() override;

    qreal devicePixelRatio() const override;

    bool setMouseGrabEnabled(bool grab) override { return grab; }
    bool setKeyboardGrabEnabled(bool grab) override { return grab; }

    WId winId() const override { return WId(m_view); }

    void clearAccessibleCache();

    QSurfaceFormat format() const override;

    void requestUpdate() override;

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

#ifndef QT_NO_DEBUG_STREAM
QDebug operator<<(QDebug debug, const QIOSWindow *window);
#endif

QT_END_NAMESPACE

#endif // QIOSWINDOW_H
