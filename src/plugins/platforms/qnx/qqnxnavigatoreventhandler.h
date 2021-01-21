/***************************************************************************
**
** Copyright (C) 2012 Research In Motion
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

#ifndef QQNXNAVIGATOREVENTHANDLER_H
#define QQNXNAVIGATOREVENTHANDLER_H

#include <QObject>

QT_BEGIN_NAMESPACE

class QQnxNavigatorEventHandler : public QObject
{
    Q_OBJECT
public:
    explicit QQnxNavigatorEventHandler(QObject *parent = 0);

    bool handleOrientationCheck(int angle);
    void handleOrientationChange(int angle);
    void handleSwipeDown();
    void handleExit();
    void handleWindowGroupActivated(const QByteArray &id);
    void handleWindowGroupDeactivated(const QByteArray &id);
    void handleWindowGroupStateChanged(const QByteArray &id, Qt::WindowState state);

Q_SIGNALS:
    void rotationChanged(int angle);
    void windowGroupActivated(const QByteArray &id);
    void windowGroupDeactivated(const QByteArray &id);
    void windowGroupStateChanged(const QByteArray &id, Qt::WindowState state);
    void swipeDown();
};

QT_END_NAMESPACE

#endif // QQNXNAVIGATOREVENTHANDLER_H
