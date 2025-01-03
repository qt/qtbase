// Copyright (C) 2012 Research In Motion
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#include "qqnxbuttoneventnotifier.h"

#include <QtGui/QGuiApplication>
#include <qpa/qwindowsysteminterface.h>

#include <QtCore/qhash.h>
#include <QtCore/qbytearray.h>
#include <QtCore/QDebug>
#include <QtCore/QMetaEnum>
#include <QtCore/QSocketNotifier>
#include <QtCore/private/qcore_unix_p.h>

#if defined(QQNXBUTTON_DEBUG)
#define qButtonDebug qDebug
#else
#define qButtonDebug QT_NO_QDEBUG_MACRO
#endif

QT_BEGIN_NAMESPACE

const char *QQnxButtonEventNotifier::ppsPath = "/pps/system/buttons/status";
const size_t QQnxButtonEventNotifier::ppsBufferSize = 256;

QQnxButtonEventNotifier::QQnxButtonEventNotifier(QObject *parent)
    : QObject(parent),
      m_fd(-1),
      m_readNotifier(0)
{
    // Set initial state of buttons to ButtonUp and
    // fetch the new button ids
    int enumeratorIndex = QQnxButtonEventNotifier::staticMetaObject.indexOfEnumerator(QByteArrayLiteral("ButtonId"));
    QMetaEnum enumerator = QQnxButtonEventNotifier::staticMetaObject.enumerator(enumeratorIndex);
    m_buttonKeys.reserve(ButtonCount - bid_minus);
    for (int buttonId = bid_minus; buttonId < ButtonCount; ++buttonId) {
        m_buttonKeys.append(enumerator.valueToKey(buttonId));
        m_state[buttonId] = ButtonUp;
    }
}

QQnxButtonEventNotifier::~QQnxButtonEventNotifier()
{
    close();
}

void QQnxButtonEventNotifier::start()
{
    qButtonDebug("starting hardware button event processing");
    if (m_fd != -1)
        return;

    // Open the pps interface
    errno = 0;
    m_fd = qt_safe_open(ppsPath, O_RDONLY);
    if (m_fd == -1) {
#if defined (QQNXBUTTON_DEBUG)
        qWarning("QQNX: failed to open buttons pps, errno=%d", errno);
#endif
        return;
    }

    m_readNotifier = new QSocketNotifier(m_fd, QSocketNotifier::Read);
    QObject::connect(m_readNotifier, SIGNAL(activated(QSocketDescriptor)), this, SLOT(updateButtonStates()));

    qButtonDebug("successfully connected to Navigator. fd = %d", m_fd);
}

void QQnxButtonEventNotifier::updateButtonStates()
{
    // Allocate buffer for pps data
    char buffer[ppsBufferSize];

    // Attempt to read pps data
    errno = 0;
    int bytes = qt_safe_read(m_fd, buffer, ppsBufferSize - 1);
    qButtonDebug() << "Read" << bytes << "bytes of data";
    if (bytes == -1) {
        qWarning("QQNX: failed to read hardware buttons pps object, errno=%d", errno);
        return;
    }

    // We seem to get a spurious read notification after the real one. Ignore it
    if (bytes == 0)
        return;

    // Ensure data is null terminated
    buffer[bytes] = '\0';

    qButtonDebug("received PPS message:\n%s", buffer);

    // Process received message
    QByteArray ppsData = QByteArray::fromRawData(buffer, bytes);
    QHash<QByteArray, QByteArray> fields;
    if (!parsePPS(ppsData, &fields))
        return;

    // Update our state and inject key events as needed
    for (int buttonId = bid_minus; buttonId < ButtonCount; ++buttonId) {
        // Extract the new button state
        QByteArray key = m_buttonKeys.at(buttonId);
        ButtonState newState = (fields.value(key) == "b_up" ? ButtonUp : ButtonDown);

        // If state has changed, update our state and inject a keypress event
        if (m_state[buttonId] != newState) {
            qButtonDebug() << "Hardware button event: button =" << key << "state =" << fields.value(key);
            m_state[buttonId] = newState;

            // Is it a key press or key release event?
            QEvent::Type type = (newState == ButtonDown) ? QEvent::KeyPress : QEvent::KeyRelease;

            Qt::Key key;
            switch (buttonId) {
                case bid_minus:
                    key = Qt::Key_VolumeDown;
                    break;

                case bid_playpause:
                    key = Qt::Key_Play;
                    break;

                case bid_plus:
                    key = Qt::Key_VolumeUp;
                    break;

                case bid_power:
                    key = Qt::Key_PowerDown;
                    break;

                default:
                    qButtonDebug("Unknown hardware button");
                    continue;
            }

            // No modifiers
            Qt::KeyboardModifiers modifier = Qt::NoModifier;

            // Post the event
            QWindowSystemInterface::handleKeyEvent(QGuiApplication::focusWindow(), type, key, modifier);
        }
    }
}

void QQnxButtonEventNotifier::close()
{
    delete m_readNotifier;
    m_readNotifier = 0;

    if (m_fd != -1) {
        qt_safe_close(m_fd);
        m_fd = -1;
    }
}

bool QQnxButtonEventNotifier::parsePPS(const QByteArray &ppsData, QHash<QByteArray, QByteArray> *messageFields) const
{
    // tokenize pps data into lines
    QList<QByteArray> lines = ppsData.split('\n');

    // validate pps object
    if (lines.size() == 0 || !lines.at(0).contains(QByteArrayLiteral("@status"))) {
        qWarning("QQNX: unrecognized pps object, data=%s", ppsData.constData());
        return false;
    }

    // parse pps object attributes and extract values
    for (int i = 1; i < lines.size(); i++) {

        // tokenize current attribute
        const QByteArray &attr = lines.at(i);

        qButtonDebug() << "attr=" << attr;

        int doubleColon = attr.indexOf(QByteArrayLiteral("::"));
        if (doubleColon == -1) {
            // abort - malformed attribute
            continue;
        }

        QByteArray key = attr.left(doubleColon);
        QByteArray value = attr.mid(doubleColon + 2);
        messageFields->insert(key, value);
    }
    return true;
}

QT_END_NAMESPACE
