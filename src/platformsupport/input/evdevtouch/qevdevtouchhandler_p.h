/****************************************************************************
**
** Copyright (C) 2021 The Qt Company Ltd.
** Copyright (C) 2016 Jolla Ltd, author: <gunnar.sletta@jollamobile.com>
** Contact: https://www.qt.io/licensing/
**
** This file is part of the plugins module of the Qt Toolkit.
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

#ifndef QEVDEVTOUCHHANDLER_P_H
#define QEVDEVTOUCHHANDLER_P_H

//
//  W A R N I N G
//  -------------
//
// This file is not part of the Qt API.  It exists purely as an
// implementation detail.  This header file may change from version to
// version without notice, or even be removed.
//
// We mean it.
//

#include <QtGui/private/qtguiglobal_p.h>
#include <QObject>
#include <QString>
#include <QList>
#include <QThread>
#include <QtCore/private/qthread_p.h>
#include <qpa/qwindowsysteminterface.h>
#include "qevdevtouchfilter_p.h"

#if QT_CONFIG(mtdev)
struct mtdev;
#endif

QT_BEGIN_NAMESPACE

class QSocketNotifier;
class QEvdevTouchScreenData;

class QEvdevTouchScreenHandler : public QObject
{
    Q_OBJECT

public:
    explicit QEvdevTouchScreenHandler(const QString &device, const QString &spec = QString(), QObject *parent = nullptr);
    ~QEvdevTouchScreenHandler();

    QTouchDevice *touchDevice() const;

    bool isFiltered() const;

    void readData();

signals:
    void touchPointsUpdated();

private:
    friend class QEvdevTouchScreenData;
    friend class QEvdevTouchScreenHandlerThread;

    void registerTouchDevice();
    void unregisterTouchDevice();

    QSocketNotifier *m_notify;
    int m_fd;
    QEvdevTouchScreenData *d;
    QTouchDevice *m_device;
#if QT_CONFIG(mtdev)
    mtdev *m_mtdev;
#endif
};

class QEvdevTouchScreenHandlerThread : public QDaemonThread
{
    Q_OBJECT
public:
    explicit QEvdevTouchScreenHandlerThread(const QString &device, const QString &spec, QObject *parent = nullptr);
    ~QEvdevTouchScreenHandlerThread();
    void run() override;

    bool isTouchDeviceRegistered() const;

    bool eventFilter(QObject *object, QEvent *event) override;

    void scheduleTouchPointUpdate();

signals:
    void touchDeviceRegistered();

private:
    Q_INVOKABLE void notifyTouchDeviceRegistered();

    void filterAndSendTouchPoints();
    QRect targetScreenGeometry() const;

    QString m_device;
    QString m_spec;
    QEvdevTouchScreenHandler *m_handler;
    bool m_touchDeviceRegistered;

    bool m_touchUpdatePending;
    QWindow *m_filterWindow;

    struct FilteredTouchPoint {
        QEvdevTouchFilter x;
        QEvdevTouchFilter y;
        QWindowSystemInterface::TouchPoint touchPoint;
    };
    QHash<int, FilteredTouchPoint> m_filteredPoints;

    float m_touchRate;
};

QT_END_NAMESPACE

#endif // QEVDEVTOUCH_P_H
