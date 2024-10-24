// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only


#include "qspiapplicationadaptor_p.h"

#include <QtCore/qcoreapplication.h>
#include <QtDBus/qdbuspendingreply.h>
#include <qdebug.h>

#if QT_CONFIG(accessibility)
#include "deviceeventcontroller_adaptor.h"
#include "atspi/atspi-constants.h"

#if __has_include(<xcb/xproto.h>)
#include <xcb/xproto.h>
#endif

//#define KEYBOARD_DEBUG

QT_BEGIN_NAMESPACE

using namespace Qt::Literals::StringLiterals;

/*!
    \class QSpiApplicationAdaptor
    \internal

    \brief QSpiApplicationAdaptor

    QSpiApplicationAdaptor
*/

QSpiApplicationAdaptor::QSpiApplicationAdaptor(const QDBusConnection &connection, QObject *parent)
    : QObject(parent), dbusConnection(connection)
{
}

enum QSpiKeyEventType {
      QSPI_KEY_EVENT_PRESS,
      QSPI_KEY_EVENT_RELEASE,
      QSPI_KEY_EVENT_LAST_DEFINED
};

void QSpiApplicationAdaptor::sendEvents(bool active)
{
    if (active) {
        qApp->installEventFilter(this);
    } else {
        qApp->removeEventFilter(this);
    }
}


bool QSpiApplicationAdaptor::eventFilter(QObject *target, QEvent *event)
{
    if (!event->spontaneous())
        return false;

    switch (event->type()) {
    case QEvent::WindowActivate:
        emit windowActivated(target, true);
        break;
    case QEvent::WindowDeactivate:
        emit windowActivated(target, false);
        break;
    case QEvent::KeyPress:
    case QEvent::KeyRelease: {
        QKeyEvent *keyEvent = static_cast <QKeyEvent *>(event);
        QSpiDeviceEvent de;

        if (event->type() == QEvent::KeyPress)
            de.type = QSPI_KEY_EVENT_PRESS;
        else
            de.type = QSPI_KEY_EVENT_RELEASE;

        de.id = keyEvent->nativeVirtualKey();
        de.hardwareCode = keyEvent->nativeScanCode();

        de.timestamp = QDateTime::currentMSecsSinceEpoch();

        if (keyEvent->key() == Qt::Key_Tab)
            de.text = QStringLiteral("Tab");
        else if (keyEvent->key() == Qt::Key_Backtab)
            de.text = QStringLiteral("Backtab");
        else if (keyEvent->key() == Qt::Key_Control)
            de.text = QStringLiteral("Control_L");
        else if (keyEvent->key() == Qt::Key_Left)
            de.text = (keyEvent->modifiers() & Qt::KeypadModifier) ? QStringLiteral("KP_Left") : QStringLiteral("Left");
        else if (keyEvent->key() == Qt::Key_Right)
            de.text = (keyEvent->modifiers() & Qt::KeypadModifier) ? QStringLiteral("KP_Right") : QStringLiteral("Right");
        else if (keyEvent->key() == Qt::Key_Up)
            de.text = (keyEvent->modifiers() & Qt::KeypadModifier) ? QStringLiteral("KP_Up") : QStringLiteral("Up");
        else if (keyEvent->key() == Qt::Key_Down)
            de.text = (keyEvent->modifiers() & Qt::KeypadModifier) ? QStringLiteral("KP_Down") : QStringLiteral("Down");
        else if (keyEvent->key() == Qt::Key_Enter || keyEvent->key() == Qt::Key_Return)
            de.text = QStringLiteral("Return");
        else if (keyEvent->key() == Qt::Key_Backspace)
            de.text = QStringLiteral("BackSpace");
        else if (keyEvent->key() == Qt::Key_Delete)
            de.text = QStringLiteral("Delete");
        else if (keyEvent->key() == Qt::Key_PageUp)
            de.text = (keyEvent->modifiers() & Qt::KeypadModifier) ? QStringLiteral("KP_Page_Up") : QStringLiteral("Page_Up");
        else if (keyEvent->key() == Qt::Key_PageDown)
            de.text = (keyEvent->modifiers() & Qt::KeypadModifier) ? QStringLiteral("KP_Page_Up") : QStringLiteral("Page_Down");
        else if (keyEvent->key() == Qt::Key_Home)
            de.text = (keyEvent->modifiers() & Qt::KeypadModifier) ? QStringLiteral("KP_Home") : QStringLiteral("Home");
        else if (keyEvent->key() == Qt::Key_End)
            de.text = (keyEvent->modifiers() & Qt::KeypadModifier) ? QStringLiteral("KP_End") : QStringLiteral("End");
        else if (keyEvent->key() == Qt::Key_Clear && (keyEvent->modifiers() & Qt::KeypadModifier))
            de.text = QStringLiteral("KP_Begin"); // Key pad 5
        else if (keyEvent->key() == Qt::Key_Escape)
            de.text = QStringLiteral("Escape");
        else if (keyEvent->key() == Qt::Key_Space)
            de.text = QStringLiteral("space");
        else if (keyEvent->key() == Qt::Key_CapsLock)
            de.text = QStringLiteral("Caps_Lock");
        else if (keyEvent->key() == Qt::Key_NumLock)
            de.text = QStringLiteral("Num_Lock");
        else if (keyEvent->key() == Qt::Key_Insert)
            de.text = QStringLiteral("Insert");
        else
            de.text = keyEvent->text();

        // This is a bit dubious, Gnome uses some gtk function here.
        // Long term the spec will hopefully change to just use keycodes.
        de.isText = !de.text.isEmpty();

        de.modifiers = 0;
        if ((keyEvent->modifiers() & Qt::ShiftModifier) && (keyEvent->key() != Qt::Key_Shift))
            de.modifiers |= 1 << ATSPI_MODIFIER_SHIFT;
#ifdef XCB_MOD_MASK_LOCK
        if (QGuiApplication::platformName().startsWith("xcb"_L1)) {
            // TODO rather introduce Qt::CapslockModifier into KeyboardModifier
            if (keyEvent->nativeModifiers() & XCB_MOD_MASK_LOCK )
                de.modifiers |= 1 << ATSPI_MODIFIER_SHIFTLOCK;
        }
#endif
        if ((keyEvent->modifiers() & Qt::ControlModifier) && (keyEvent->key() != Qt::Key_Control))
            de.modifiers |= 1 << ATSPI_MODIFIER_CONTROL;
        if ((keyEvent->modifiers() & Qt::AltModifier) && (keyEvent->key() != Qt::Key_Alt))
            de.modifiers |= 1 << ATSPI_MODIFIER_ALT;
        if ((keyEvent->modifiers() & Qt::MetaModifier) && (keyEvent->key() != Qt::Key_Meta))
            de.modifiers |= 1 << ATSPI_MODIFIER_META;

#ifdef KEYBOARD_DEBUG
        qDebug() << "Key event text:" << event->type() << de.text
                 << "native virtual key:" << de.id
                 << "hardware code/scancode:" << de.hardwareCode
                 << "modifiers:" << de.modifiers
                 << "text:" << de.text;
#endif

        QDBusMessage m = QDBusMessage::createMethodCall(QStringLiteral("org.a11y.atspi.Registry"),
                                                        QStringLiteral("/org/a11y/atspi/registry/deviceeventcontroller"),
                                                        QStringLiteral("org.a11y.atspi.DeviceEventController"), QStringLiteral("NotifyListenersSync"));
        m.setArguments(QVariantList() << QVariant::fromValue(de));

        // FIXME: this is critical, the timeout should probably be pretty low to allow normal processing
        int timeout = 100;
        bool sent = dbusConnection.callWithCallback(m, this, SLOT(notifyKeyboardListenerCallback(QDBusMessage)),
                        SLOT(notifyKeyboardListenerError(QDBusError,QDBusMessage)), timeout);
        if (sent) {
            //queue the event and send it after callback
            keyEvents.enqueue(std::pair{QPointer<QObject>(target), copyKeyEvent(keyEvent)});
            return true;
        }
        break;
    }
    default:
        break;
    }
    return false;
}

QKeyEvent* QSpiApplicationAdaptor::copyKeyEvent(QKeyEvent* old)
{
    return new QKeyEvent(old->type(), old->key(), old->modifiers(),
                         old->nativeScanCode(), old->nativeVirtualKey(), old->nativeModifiers(),
                         old->text(), old->isAutoRepeat(), old->count());
}

void QSpiApplicationAdaptor::notifyKeyboardListenerCallback(const QDBusMessage& message)
{
    if (!keyEvents.size()) {
        qWarning("QSpiApplication::notifyKeyboardListenerCallback with no queued key called");
        return;
    }
    Q_ASSERT(message.arguments().size() == 1);
    if (message.arguments().at(0).toBool() == true) {
        std::pair<QPointer<QObject>, QKeyEvent*> event = keyEvents.dequeue();
        delete event.second;
    } else {
        std::pair<QPointer<QObject>, QKeyEvent*> event = keyEvents.dequeue();
        if (event.first)
            QCoreApplication::postEvent(event.first.data(), event.second);
    }
}

void QSpiApplicationAdaptor::notifyKeyboardListenerError(const QDBusError& error, const QDBusMessage& /*message*/)
{
    qWarning() << "QSpiApplication::keyEventError " << error.name() << error.message();
    while (!keyEvents.isEmpty()) {
        std::pair<QPointer<QObject>, QKeyEvent*> event = keyEvents.dequeue();
        if (event.first)
            QCoreApplication::postEvent(event.first.data(), event.second);
    }
}

QT_END_NAMESPACE

#include "moc_qspiapplicationadaptor_p.cpp"

#endif // QT_CONFIG(accessibility)
