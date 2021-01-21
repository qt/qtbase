/****************************************************************************
**
** Copyright (C) 2014 Robin Burchell <robin.burchell@viroteck.net>
** Copyright (C) 2021 The Qt Company Ltd.
** Contact: https://www.qt.io/licensing/
**
** This file is part of the QtCore module of the Qt Toolkit.
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

#ifndef QTUIOHANDLER_P_H
#define QTUIOHANDLER_P_H

#include <QObject>
#include <QMap>
#include <QUdpSocket>
#include <QVector>
#include <QTransform>

#include <qpa/qwindowsysteminterface.h>

QT_BEGIN_NAMESPACE

class QTouchDevice;
class QOscMessage;
class QTuioCursor;
class QTuioToken;

class QTuioHandler : public QObject
{
    Q_OBJECT

public:
    explicit QTuioHandler(const QString &specification);
    virtual ~QTuioHandler();

private slots:
    void processPackets();
    void process2DCurSource(const QOscMessage &message);
    void process2DCurAlive(const QOscMessage &message);
    void process2DCurSet(const QOscMessage &message);
    void process2DCurFseq(const QOscMessage &message);
    void process2DObjSource(const QOscMessage &message);
    void process2DObjAlive(const QOscMessage &message);
    void process2DObjSet(const QOscMessage &message);
    void process2DObjFseq(const QOscMessage &message);

private:
    QWindowSystemInterface::TouchPoint cursorToTouchPoint(const QTuioCursor &tc, QWindow *win);
    QWindowSystemInterface::TouchPoint tokenToTouchPoint(const QTuioToken &tc, QWindow *win);

    QTouchDevice *m_device;
    QUdpSocket m_socket;
    QMap<int, QTuioCursor> m_activeCursors;
    QVector<QTuioCursor> m_deadCursors;
    QMap<int, QTuioToken> m_activeTokens;
    QVector<QTuioToken> m_deadTokens;
    QTransform m_transform;
};

QT_END_NAMESPACE

#endif // QTUIOHANDLER_P_H
