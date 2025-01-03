// Copyright (C) 2014 Robin Burchell <robin.burchell@viroteck.net>
// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#ifndef QTUIOHANDLER_P_H
#define QTUIOHANDLER_P_H

#include <QList>
#include <QObject>
#include <QMap>
#include <QUdpSocket>
#include <QTransform>

#include <qpa/qwindowsysteminterface.h>

QT_BEGIN_NAMESPACE

class QPointingDevice;
class QOscMessage;
class QTuioCursor;
class QTuioToken;

class QTuioHandler : public QObject
{
    Q_OBJECT
    Q_MOC_INCLUDE("qoscmessage_p.h")

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

    QPointingDevice *m_device = nullptr;
    QUdpSocket m_socket;
    QMap<int, QTuioCursor> m_activeCursors;
    QList<QTuioCursor> m_deadCursors;
    QMap<int, QTuioToken> m_activeTokens;
    QList<QTuioToken> m_deadTokens;
    QTransform m_transform;
};

QT_END_NAMESPACE

#endif // QTUIOHANDLER_P_H
