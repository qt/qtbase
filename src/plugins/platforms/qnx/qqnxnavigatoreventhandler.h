// Copyright (C) 2012 Research In Motion
// SPDX-License-Identifier: LicenseRef-Qt-Commercial

#ifndef QQNXNAVIGATOREVENTHANDLER_H
#define QQNXNAVIGATOREVENTHANDLER_H

#include <QObject>

QT_BEGIN_NAMESPACE

class QQnxNavigatorEventHandler : public QObject
{
    Q_OBJECT
public:
    explicit QQnxNavigatorEventHandler(QObject *parent = nullptr);

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
