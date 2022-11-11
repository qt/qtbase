// Copyright (C) 2014 Robin Burchell <robin.burchell@viroteck.net>
// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#include "qtuiohandler_p.h"

#include "qtuiocursor_p.h"
#include "qtuiotoken_p.h"
#include "qoscbundle_p.h"
#include "qoscmessage_p.h"

#include <qpa/qwindowsysteminterface.h>

#include <QPointingDevice>
#include <QWindow>
#include <QGuiApplication>

#include <QLoggingCategory>
#include <QRect>
#include <qmath.h>

QT_BEGIN_NAMESPACE

Q_LOGGING_CATEGORY(lcTuioHandler, "qt.qpa.tuio.handler")
Q_LOGGING_CATEGORY(lcTuioSource, "qt.qpa.tuio.source")
Q_LOGGING_CATEGORY(lcTuioSet, "qt.qpa.tuio.set")

// With TUIO the first application takes exclusive ownership of the "device"
// we cannot attach more than one application to the same port anyway.
// Forcing delivery makes it easy to use simulators in the same machine
// and forget about headaches about unfocused TUIO windows.
static bool forceDelivery = qEnvironmentVariableIsSet("QT_TUIOTOUCH_DELIVER_WITHOUT_FOCUS");

QTuioHandler::QTuioHandler(const QString &specification)
{
    QStringList args = specification.split(':');
    int portNumber = 3333;
    int rotationAngle = 0;
    bool invertx = false;
    bool inverty = false;

    for (int i = 0; i < args.size(); ++i) {
        if (args.at(i).startsWith("udp=")) {
            QString portString = args.at(i).section('=', 1, 1);
            portNumber = portString.toInt();
        } else if (args.at(i).startsWith("tcp=")) {
            QString portString = args.at(i).section('=', 1, 1);
            portNumber = portString.toInt();
            qCWarning(lcTuioHandler) << "TCP is not yet supported. Falling back to UDP on " << portNumber;
        } else if (args.at(i) == "invertx") {
            invertx = true;
        } else if (args.at(i) == "inverty") {
            inverty = true;
        } else if (args.at(i).startsWith("rotate=")) {
            QString rotateArg = args.at(i).section('=', 1, 1);
            int argValue = rotateArg.toInt();
            switch (argValue) {
            case 90:
            case 180:
            case 270:
                rotationAngle = argValue;
            default:
                break;
            }
        }
    }

    if (rotationAngle)
        m_transform = QTransform::fromTranslate(0.5, 0.5).rotate(rotationAngle).translate(-0.5, -0.5);

    if (invertx)
        m_transform *= QTransform::fromTranslate(0.5, 0.5).scale(-1.0, 1.0).translate(-0.5, -0.5);

    if (inverty)
        m_transform *= QTransform::fromTranslate(0.5, 0.5).scale(1.0, -1.0).translate(-0.5, -0.5);

    // not leaked, QPointingDevice cleans up registered devices itself
    // TODO register each device based on SOURCE, not just an all-purpose generic touchscreen
    // TODO define seats when multiple connections occur
    m_device = new QPointingDevice(QLatin1String("TUIO"), 1, QInputDevice::DeviceType::TouchScreen,
                                   QPointingDevice::PointerType::Finger,
                                   QInputDevice::Capability::Position |
                                   QInputDevice::Capability::Area |
                                   QInputDevice::Capability::Velocity |
                                   QInputDevice::Capability::NormalizedPosition,
                                   16, 0);
    QWindowSystemInterface::registerInputDevice(m_device);

    if (!m_socket.bind(QHostAddress::Any, portNumber)) {
        qCWarning(lcTuioHandler) << "Failed to bind TUIO socket: " << m_socket.errorString();
        return;
    }

    connect(&m_socket, &QUdpSocket::readyRead, this, &QTuioHandler::processPackets);
}

QTuioHandler::~QTuioHandler()
{
}

void QTuioHandler::processPackets()
{
    while (m_socket.hasPendingDatagrams()) {
        QByteArray datagram;
        datagram.resize(m_socket.pendingDatagramSize());
        QHostAddress sender;
        quint16 senderPort;

        qint64 size = m_socket.readDatagram(datagram.data(), datagram.size(),
                                             &sender, &senderPort);

        if (size == -1)
            continue;

        if (size != datagram.size())
            datagram.resize(size);

        // "A typical TUIO bundle will contain an initial ALIVE message,
        // followed by an arbitrary number of SET messages that can fit into the
        // actual bundle capacity and a concluding FSEQ message. A minimal TUIO
        // bundle needs to contain at least the compulsory ALIVE and FSEQ
        // messages. The FSEQ frame ID is incremented for each delivered bundle,
        // while redundant bundles can be marked using the frame sequence ID
        // -1."
        QList<QOscMessage> messages;

        QOscBundle bundle(datagram);
        if (bundle.isValid()) {
            messages = bundle.messages();
        } else {
            QOscMessage msg(datagram);
            if (!msg.isValid()) {
                qCWarning(lcTuioSet) << "Got invalid datagram.";
                continue;
            }
            messages.push_back(msg);
        }

        for (const QOscMessage &message : std::as_const(messages)) {
            if (message.addressPattern() == "/tuio/2Dcur") {
                QList<QVariant> arguments = message.arguments();
                if (arguments.size() == 0) {
                    qCWarning(lcTuioHandler, "Ignoring TUIO message with no arguments");
                    continue;
                }

                QByteArray messageType = arguments.at(0).toByteArray();
                if (messageType == "source") {
                    process2DCurSource(message);
                } else if (messageType == "alive") {
                    process2DCurAlive(message);
                } else if (messageType == "set") {
                    process2DCurSet(message);
                } else if (messageType == "fseq") {
                    process2DCurFseq(message);
                } else {
                    qCWarning(lcTuioHandler) << "Ignoring unknown TUIO message type: " << messageType;
                    continue;
                }
            } else if (message.addressPattern() == "/tuio/2Dobj") {
                QList<QVariant> arguments = message.arguments();
                if (arguments.size() == 0) {
                    qCWarning(lcTuioHandler, "Ignoring TUIO message with no arguments");
                    continue;
                }

                QByteArray messageType = arguments.at(0).toByteArray();
                if (messageType == "source") {
                    process2DObjSource(message);
                } else if (messageType == "alive") {
                    process2DObjAlive(message);
                } else if (messageType == "set") {
                    process2DObjSet(message);
                } else if (messageType == "fseq") {
                    process2DObjFseq(message);
                } else {
                    qCWarning(lcTuioHandler) << "Ignoring unknown TUIO message type: " << messageType;
                    continue;
                }
            } else {
                qCWarning(lcTuioHandler) << "Ignoring unknown address pattern " << message.addressPattern();
                continue;
            }
        }
    }
}

void QTuioHandler::process2DCurSource(const QOscMessage &message)
{
    QList<QVariant> arguments = message.arguments();
    if (arguments.size() != 2) {
        qCWarning(lcTuioSource) << "Ignoring malformed TUIO source message: " << arguments.size();
        return;
    }

    if (QMetaType::Type(arguments.at(1).userType()) != QMetaType::QByteArray) {
        qCWarning(lcTuioSource, "Ignoring malformed TUIO source message (bad argument type)");
        return;
    }

    qCDebug(lcTuioSource) << "Got TUIO source message from: " << arguments.at(1).toByteArray();
}

void QTuioHandler::process2DCurAlive(const QOscMessage &message)
{
    QList<QVariant> arguments = message.arguments();

    // delta the notified cursors that are active, against the ones we already
    // know of.
    //
    // TBD: right now we're assuming one 2Dcur alive message corresponds to a
    // new data source from the input. is this correct, or do we need to store
    // changes and only process the deltas on fseq?
    QMap<int, QTuioCursor> oldActiveCursors = m_activeCursors;
    QMap<int, QTuioCursor> newActiveCursors;

    for (int i = 1; i < arguments.size(); ++i) {
        if (QMetaType::Type(arguments.at(i).userType()) != QMetaType::Int) {
            qCWarning(lcTuioHandler) << "Ignoring malformed TUIO alive message (bad argument on position" << i << arguments << ')';
            return;
        }

        int cursorId = arguments.at(i).toInt();
        if (!oldActiveCursors.contains(cursorId)) {
            // newly active
            QTuioCursor cursor(cursorId);
            cursor.setState(QEventPoint::State::Pressed);
            newActiveCursors.insert(cursorId, cursor);
        } else {
            // we already know about it, remove it so it isn't marked as released
            QTuioCursor cursor = oldActiveCursors.value(cursorId);
            cursor.setState(QEventPoint::State::Stationary); // position change in SET will update if needed
            newActiveCursors.insert(cursorId, cursor);
            oldActiveCursors.remove(cursorId);
        }
    }

    // anything left is dead now
    QMap<int, QTuioCursor>::ConstIterator it = oldActiveCursors.constBegin();

    // deadCursors should be cleared from the last FSEQ now
    m_deadCursors.reserve(oldActiveCursors.size());

    // TODO: there could be an issue of resource exhaustion here if FSEQ isn't
    // sent in a timely fashion. we should probably track message counts and
    // force-flush if we get too many built up.
    while (it != oldActiveCursors.constEnd()) {
        m_deadCursors.append(it.value());
        ++it;
    }

    m_activeCursors = newActiveCursors;
}

void QTuioHandler::process2DCurSet(const QOscMessage &message)
{
    QList<QVariant> arguments = message.arguments();
    if (arguments.size() < 7) {
        qCWarning(lcTuioSet) << "Ignoring malformed TUIO set message with too few arguments: " << arguments.size();
        return;
    }

    if (QMetaType::Type(arguments.at(1).userType()) != QMetaType::Int   ||
        QMetaType::Type(arguments.at(2).userType()) != QMetaType::Float ||
        QMetaType::Type(arguments.at(3).userType()) != QMetaType::Float ||
        QMetaType::Type(arguments.at(4).userType()) != QMetaType::Float ||
        QMetaType::Type(arguments.at(5).userType()) != QMetaType::Float ||
        QMetaType::Type(arguments.at(6).userType()) != QMetaType::Float
       ) {
        qCWarning(lcTuioSet) << "Ignoring malformed TUIO set message with bad types: " << arguments;
        return;
    }

    int cursorId = arguments.at(1).toInt();
    float x = arguments.at(2).toFloat();
    float y = arguments.at(3).toFloat();
    float vx = arguments.at(4).toFloat();
    float vy = arguments.at(5).toFloat();
    float acceleration = arguments.at(6).toFloat();

    QMap<int, QTuioCursor>::Iterator it = m_activeCursors.find(cursorId);
    if (it == m_activeCursors.end()) {
        qCWarning(lcTuioSet) << "Ignoring malformed TUIO set for nonexistent cursor " << cursorId;
        return;
    }

    qCDebug(lcTuioSet) << "Processing SET for " << cursorId << " x: " << x << y << vx << vy << acceleration;
    QTuioCursor &cur = *it;
    cur.setX(x);
    cur.setY(y);
    cur.setVX(vx);
    cur.setVY(vy);
    cur.setAcceleration(acceleration);
}

QWindowSystemInterface::TouchPoint QTuioHandler::cursorToTouchPoint(const QTuioCursor &tc, QWindow *win)
{
    QWindowSystemInterface::TouchPoint tp;
    tp.id = tc.id();
    tp.pressure = 1.0f;

    tp.normalPosition = QPointF(tc.x(), tc.y());

    if (!m_transform.isIdentity())
        tp.normalPosition = m_transform.map(tp.normalPosition);

    tp.state = tc.state();

    // we map the touch to the size of the window. we do this, because frankly,
    // trying to figure out which part of the screen to hit in order to press an
    // element on the UI is pretty tricky when one is not using an overlay-style
    // TUIO device.
    //
    // in the future, it might make sense to make this choice optional,
    // dependent on the spec.
    QPointF relPos = QPointF(win->size().width() * tp.normalPosition.x(), win->size().height() * tp.normalPosition.y());
    QPointF delta = relPos - relPos.toPoint();
    tp.area.moveCenter(win->mapToGlobal(relPos.toPoint()) + delta);
    tp.velocity = QVector2D(win->size().width() * tc.vx(), win->size().height() * tc.vy());
    return tp;
}


void QTuioHandler::process2DCurFseq(const QOscMessage &message)
{
    Q_UNUSED(message); // TODO: do we need to do anything with the frame id?

    QWindow *win = QGuiApplication::focusWindow();
    if (!win && QGuiApplication::topLevelWindows().size() > 0 && forceDelivery)
          win = QGuiApplication::topLevelWindows().at(0);

    if (!win)
        return;

    QList<QWindowSystemInterface::TouchPoint> tpl;
    tpl.reserve(m_activeCursors.size() + m_deadCursors.size());

    for (const QTuioCursor &tc : std::as_const(m_activeCursors)) {
        QWindowSystemInterface::TouchPoint tp = cursorToTouchPoint(tc, win);
        tpl.append(tp);
    }

    for (const QTuioCursor &tc : std::as_const(m_deadCursors)) {
        QWindowSystemInterface::TouchPoint tp = cursorToTouchPoint(tc, win);
        tp.state = QEventPoint::State::Released;
        tpl.append(tp);
    }
    QWindowSystemInterface::handleTouchEvent(win, m_device, tpl);

    m_deadCursors.clear();
}

void QTuioHandler::process2DObjSource(const QOscMessage &message)
{
    QList<QVariant> arguments = message.arguments();
    if (arguments.size() != 2) {
        qCWarning(lcTuioSource ) << "Ignoring malformed TUIO source message: " << arguments.size();
        return;
    }

    if (QMetaType::Type(arguments.at(1).userType()) != QMetaType::QByteArray) {
        qCWarning(lcTuioSource, "Ignoring malformed TUIO source message (bad argument type)");
        return;
    }

    qCDebug(lcTuioSource) << "Got TUIO source message from: " << arguments.at(1).toByteArray();
}

void QTuioHandler::process2DObjAlive(const QOscMessage &message)
{
    QList<QVariant> arguments = message.arguments();

    // delta the notified tokens that are active, against the ones we already
    // know of.
    //
    // TBD: right now we're assuming one 2DObj alive message corresponds to a
    // new data source from the input. is this correct, or do we need to store
    // changes and only process the deltas on fseq?
    QMap<int, QTuioToken> oldActiveTokens = m_activeTokens;
    QMap<int, QTuioToken> newActiveTokens;

    for (int i = 1; i < arguments.size(); ++i) {
        if (QMetaType::Type(arguments.at(i).userType()) != QMetaType::Int) {
            qCWarning(lcTuioHandler) << "Ignoring malformed TUIO alive message (bad argument on position" << i << arguments << ')';
            return;
        }

        int sessionId = arguments.at(i).toInt();
        if (!oldActiveTokens.contains(sessionId)) {
            // newly active
            QTuioToken token(sessionId);
            token.setState(QEventPoint::State::Pressed);
            newActiveTokens.insert(sessionId, token);
        } else {
            // we already know about it, remove it so it isn't marked as released
            QTuioToken token = oldActiveTokens.value(sessionId);
            token.setState(QEventPoint::State::Stationary); // position change in SET will update if needed
            newActiveTokens.insert(sessionId, token);
            oldActiveTokens.remove(sessionId);
        }
    }

    // anything left is dead now
    QMap<int, QTuioToken>::ConstIterator it = oldActiveTokens.constBegin();

    // deadTokens should be cleared from the last FSEQ now
    m_deadTokens.reserve(oldActiveTokens.size());

    // TODO: there could be an issue of resource exhaustion here if FSEQ isn't
    // sent in a timely fashion. we should probably track message counts and
    // force-flush if we get too many built up.
    while (it != oldActiveTokens.constEnd()) {
        m_deadTokens.append(it.value());
        ++it;
    }

    m_activeTokens = newActiveTokens;
}

void QTuioHandler::process2DObjSet(const QOscMessage &message)
{
    QList<QVariant> arguments = message.arguments();
    if (arguments.size() < 7) {
        qCWarning(lcTuioSet) << "Ignoring malformed TUIO set message with too few arguments: " << arguments.size();
        return;
    }

    if (QMetaType::Type(arguments.at(1).userType()) != QMetaType::Int ||
            QMetaType::Type(arguments.at(2).userType()) != QMetaType::Int ||
            QMetaType::Type(arguments.at(3).userType()) != QMetaType::Float ||
            QMetaType::Type(arguments.at(4).userType()) != QMetaType::Float ||
            QMetaType::Type(arguments.at(5).userType()) != QMetaType::Float ||
            QMetaType::Type(arguments.at(6).userType()) != QMetaType::Float ||
            QMetaType::Type(arguments.at(7).userType()) != QMetaType::Float ||
            QMetaType::Type(arguments.at(8).userType()) != QMetaType::Float ||
            QMetaType::Type(arguments.at(9).userType()) != QMetaType::Float ||
            QMetaType::Type(arguments.at(10).userType()) != QMetaType::Float) {
        qCWarning(lcTuioSet) << "Ignoring malformed TUIO set message with bad types: " << arguments;
        return;
    }

    int id = arguments.at(1).toInt();
    int classId = arguments.at(2).toInt();
    float x = arguments.at(3).toFloat();
    float y = arguments.at(4).toFloat();
    float angle = arguments.at(5).toFloat();
    float vx = arguments.at(6).toFloat();
    float vy = arguments.at(7).toFloat();
    float angularVelocity = arguments.at(8).toFloat();
    float acceleration = arguments.at(9).toFloat();
    float angularAcceleration = arguments.at(10).toFloat();

    QMap<int, QTuioToken>::Iterator it = m_activeTokens.find(id);
    if (it == m_activeTokens.end()) {
        qCWarning(lcTuioSet) << "Ignoring malformed TUIO set for nonexistent token " << classId;
        return;
    }

    qCDebug(lcTuioSet) << "Processing SET for token " << classId << id << " @ " << x << y << " angle: " << angle <<
                          "vel" << vx << vy << angularVelocity << "acc" << acceleration << angularAcceleration;
    QTuioToken &tok = *it;
    tok.setClassId(classId);
    tok.setX(x);
    tok.setY(y);
    tok.setVX(vx);
    tok.setVY(vy);
    tok.setAcceleration(acceleration);
    tok.setAngle(angle);
    tok.setAngularVelocity(angularAcceleration);
    tok.setAngularAcceleration(angularAcceleration);
}

QWindowSystemInterface::TouchPoint QTuioHandler::tokenToTouchPoint(const QTuioToken &tc, QWindow *win)
{
    QWindowSystemInterface::TouchPoint tp;
    tp.id = tc.id();
    tp.uniqueId = tc.classId(); // TODO TUIO 2.0: populate a QVariant, and register the mapping from int to arbitrary UID data
    tp.pressure = 1.0f;

    tp.normalPosition = QPointF(tc.x(), tc.y());

    if (!m_transform.isIdentity())
        tp.normalPosition = m_transform.map(tp.normalPosition);

    tp.state = tc.state();

    // We map the token position to the size of the window.
    QPointF relPos = QPointF(win->size().width() * tp.normalPosition.x(), win->size().height() * tp.normalPosition.y());
    QPointF delta = relPos - relPos.toPoint();
    tp.area.moveCenter(win->mapToGlobal(relPos.toPoint()) + delta);
    tp.velocity = QVector2D(win->size().width() * tc.vx(), win->size().height() * tc.vy());
    tp.rotation = qRadiansToDegrees(tc.angle());
    return tp;
}


void QTuioHandler::process2DObjFseq(const QOscMessage &message)
{
    Q_UNUSED(message); // TODO: do we need to do anything with the frame id?

    QWindow *win = QGuiApplication::focusWindow();
    if (!win && QGuiApplication::topLevelWindows().size() > 0 && forceDelivery)
          win = QGuiApplication::topLevelWindows().at(0);

    if (!win)
        return;

    QList<QWindowSystemInterface::TouchPoint> tpl;
    tpl.reserve(m_activeTokens.size() + m_deadTokens.size());

    for (const QTuioToken & t : std::as_const(m_activeTokens)) {
        QWindowSystemInterface::TouchPoint tp = tokenToTouchPoint(t, win);
        tpl.append(tp);
    }

    for (const QTuioToken & t : std::as_const(m_deadTokens)) {
        QWindowSystemInterface::TouchPoint tp = tokenToTouchPoint(t, win);
        tp.state = QEventPoint::State::Released;
        tp.velocity = QVector2D();
        tpl.append(tp);
    }
    QWindowSystemInterface::handleTouchEvent(win, m_device, tpl);

    m_deadTokens.clear();
}

QT_END_NAMESPACE

#include "moc_qtuiohandler_p.cpp"

