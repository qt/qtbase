/****************************************************************************
**
** Copyright (C) 2015 The Qt Company Ltd.
** Contact: http://www.qt.io/licensing/
**
** This file is part of the plugins of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:LGPL21$
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company. For licensing terms
** and conditions see http://www.qt.io/terms-conditions. For further
** information use the contact form at http://www.qt.io/contact-us.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 2.1 or version 3 as published by the Free
** Software Foundation and appearing in the file LICENSE.LGPLv21 and
** LICENSE.LGPLv3 included in the packaging of this file. Please review the
** following information to ensure the GNU Lesser General Public License
** requirements will be met: https://www.gnu.org/licenses/lgpl.html and
** http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** As a special exception, The Qt Company gives you certain additional
** rights. These rights are described in The Qt Company LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
** $QT_END_LICENSE$
**
****************************************************************************/
#include "qibusplatforminputcontext.h"

#include <QtDebug>
#include <QTextCharFormat>
#include <QGuiApplication>
#include <QDBusVariant>
#include <qwindow.h>
#include <qevent.h>

#include <qpa/qplatformcursor.h>
#include <qpa/qplatformscreen.h>
#include <qpa/qwindowsysteminterface.h>

#include "qibusproxy.h"
#include "qibusinputcontextproxy.h"
#include "qibustypes.h"

#include <sys/types.h>
#include <signal.h>

#include <QtDBus>

#ifndef IBUS_RELEASE_MASK
#define IBUS_RELEASE_MASK (1 << 30)
#endif

QT_BEGIN_NAMESPACE

Q_LOGGING_CATEGORY(qtQpaInputMethods, "qt.qpa.input.methods")

enum { debug = 0 };

class QIBusPlatformInputContextPrivate
{
public:
    QIBusPlatformInputContextPrivate();
    ~QIBusPlatformInputContextPrivate()
    {
        delete context;
        delete bus;
        delete connection;
    }

    static QDBusConnection *createConnection();

    QDBusConnection *connection;
    QIBusProxy *bus;
    QIBusInputContextProxy *context;

    bool valid;
    QString predit;
    bool needsSurroundingText;
};


QIBusPlatformInputContext::QIBusPlatformInputContext ()
    : d(new QIBusPlatformInputContextPrivate())
{
    if (d->context) {
        connect(d->context, SIGNAL(CommitText(QDBusVariant)), SLOT(commitText(QDBusVariant)));
        connect(d->context, SIGNAL(UpdatePreeditText(QDBusVariant,uint,bool)), this, SLOT(updatePreeditText(QDBusVariant,uint,bool)));
        connect(d->context, SIGNAL(DeleteSurroundingText(int,uint)), this, SLOT(deleteSurroundingText(int,uint)));
        connect(d->context, SIGNAL(RequireSurroundingText()), this, SLOT(surroundingTextRequired()));
    }
    QInputMethod *p = qApp->inputMethod();
    connect(p, SIGNAL(cursorRectangleChanged()), this, SLOT(cursorRectChanged()));
    m_eventFilterUseSynchronousMode = false;
    if (qEnvironmentVariableIsSet("IBUS_ENABLE_SYNC_MODE")) {
        bool ok;
        int enableSync = qgetenv("IBUS_ENABLE_SYNC_MODE").toInt(&ok);
        if (ok && enableSync == 1)
            m_eventFilterUseSynchronousMode = true;
    }
}

QIBusPlatformInputContext::~QIBusPlatformInputContext (void)
{
    delete d;
}

bool QIBusPlatformInputContext::isValid() const
{
    return d->valid;
}

void QIBusPlatformInputContext::invokeAction(QInputMethod::Action a, int)
{
    if (!d->valid)
        return;

    if (a == QInputMethod::Click)
        commit();
}

void QIBusPlatformInputContext::reset()
{
    QPlatformInputContext::reset();

    if (!d->valid)
        return;

    d->context->Reset();
    d->predit = QString();
}

void QIBusPlatformInputContext::commit()
{
    QPlatformInputContext::commit();

    if (!d->valid)
        return;

    QObject *input = qApp->focusObject();
    if (!input) {
        d->predit = QString();
        return;
    }

    if (!d->predit.isEmpty()) {
        QInputMethodEvent event;
        event.setCommitString(d->predit);
        QCoreApplication::sendEvent(input, &event);
    }

    d->context->Reset();
    d->predit = QString();
}


void QIBusPlatformInputContext::update(Qt::InputMethodQueries q)
{
    QObject *input = qApp->focusObject();

    if (d->needsSurroundingText && input
            && (q.testFlag(Qt::ImSurroundingText)
                || q.testFlag(Qt::ImCursorPosition)
                || q.testFlag(Qt::ImAnchorPosition))) {
        QInputMethodQueryEvent srrndTextQuery(Qt::ImSurroundingText);
        QInputMethodQueryEvent cursorPosQuery(Qt::ImCursorPosition);
        QInputMethodQueryEvent anchorPosQuery(Qt::ImAnchorPosition);

        QCoreApplication::sendEvent(input, &srrndTextQuery);
        QCoreApplication::sendEvent(input, &cursorPosQuery);
        QCoreApplication::sendEvent(input, &anchorPosQuery);

        QString surroundingText = srrndTextQuery.value(Qt::ImSurroundingText).toString();
        uint cursorPosition = cursorPosQuery.value(Qt::ImCursorPosition).toUInt();
        uint anchorPosition = anchorPosQuery.value(Qt::ImAnchorPosition).toUInt();

        QIBusText text;
        text.text = surroundingText;

        QVariant variant;
        variant.setValue(text);
        QDBusVariant dbusText(variant);

        d->context->SetSurroundingText(dbusText, cursorPosition, anchorPosition);
    }
    QPlatformInputContext::update(q);
}

void QIBusPlatformInputContext::cursorRectChanged()
{
    if (!d->valid)
        return;

    QRect r = qApp->inputMethod()->cursorRectangle().toRect();
    if(!r.isValid())
        return;

    QWindow *inputWindow = qApp->focusWindow();
    if (!inputWindow)
        return;
    r.moveTopLeft(inputWindow->mapToGlobal(r.topLeft()));
    if (debug)
        qDebug() << "microFocus" << r;
    d->context->SetCursorLocation(r.x(), r.y(), r.width(), r.height());
}

void QIBusPlatformInputContext::setFocusObject(QObject *object)
{
    if (!d->valid)
        return;

    if (debug)
        qDebug() << "setFocusObject" << object;
    if (object)
        d->context->FocusIn();
    else
        d->context->FocusOut();
}

void QIBusPlatformInputContext::commitText(const QDBusVariant &text)
{
    QObject *input = qApp->focusObject();
    if (!input)
        return;

    const QDBusArgument arg = text.variant().value<QDBusArgument>();

    QIBusText t;
    if (debug)
        qDebug() << arg.currentSignature();
    arg >> t;
    if (debug)
        qDebug() << "commit text:" << t.text;

    QInputMethodEvent event;
    event.setCommitString(t.text);
    QCoreApplication::sendEvent(input, &event);

    d->predit = QString();
}

void QIBusPlatformInputContext::updatePreeditText(const QDBusVariant &text, uint cursorPos, bool visible)
{
    QObject *input = qApp->focusObject();
    if (!input)
        return;

    const QDBusArgument arg = text.variant().value<QDBusArgument>();

    QIBusText t;
    arg >> t;
    if (debug)
        qDebug() << "preedit text:" << t.text;

    QList<QInputMethodEvent::Attribute> attributes = t.attributes.imAttributes();
    if (!t.text.isEmpty())
        attributes += QInputMethodEvent::Attribute(QInputMethodEvent::Cursor, cursorPos, visible ? 1 : 0, QVariant());

    QInputMethodEvent event(t.text, attributes);
    QCoreApplication::sendEvent(input, &event);

    d->predit = t.text;
}

void QIBusPlatformInputContext::surroundingTextRequired()
{
    if (debug)
        qDebug() << "surroundingTextRequired";
    d->needsSurroundingText = true;
    update(Qt::ImSurroundingText);
}

void QIBusPlatformInputContext::deleteSurroundingText(int offset, uint n_chars)
{
    QObject *input = qApp->focusObject();
    if (!input)
        return;

    if (debug)
        qDebug() << "deleteSurroundingText" << offset << n_chars;

    QInputMethodEvent event;
    event.setCommitString("", offset, n_chars);
    QCoreApplication::sendEvent(input, &event);
}

bool QIBusPlatformInputContext::filterEvent(const QEvent *event)
{
    if (!d->valid)
        return false;

    if (!inputMethodAccepted())
        return false;

    const QKeyEvent *keyEvent = static_cast<const QKeyEvent *>(event);
    quint32 sym = keyEvent->nativeVirtualKey();
    quint32 code = keyEvent->nativeScanCode();
    quint32 state = keyEvent->nativeModifiers();

    if (keyEvent->type() != QEvent::KeyPress)
        state |= IBUS_RELEASE_MASK;

    code -= 8; // ###
    QDBusPendingReply<bool> reply = d->context->ProcessKeyEvent(sym, code, state);

    if (m_eventFilterUseSynchronousMode || reply.isFinished()) {
        bool retval = reply.value();
        qCDebug(qtQpaInputMethods) << "filterEvent return" << code << sym << state << retval;
        return retval;
    }

    Qt::KeyboardModifiers modifiers = keyEvent->modifiers();

    QVariantList args;
    args << QVariant::fromValue(keyEvent->timestamp());
    args << QVariant::fromValue(static_cast<uint>(keyEvent->type()));
    args << QVariant::fromValue(keyEvent->key());
    args << QVariant::fromValue(code) << QVariant::fromValue(sym) << QVariant::fromValue(state);
    args << QVariant::fromValue(keyEvent->text());
    args << QVariant::fromValue(keyEvent->isAutoRepeat());
    args << QVariant::fromValue(keyEvent->count());

    QIBusFilterEventWatcher *watcher = new QIBusFilterEventWatcher(reply, this, qApp->focusObject(), modifiers, args);
    QObject::connect(watcher, &QDBusPendingCallWatcher::finished, this, &QIBusPlatformInputContext::filterEventFinished);

    return true;
}

void QIBusPlatformInputContext::filterEventFinished(QDBusPendingCallWatcher *call)
{
    QIBusFilterEventWatcher *watcher = (QIBusFilterEventWatcher *) call;
    QDBusPendingReply<bool> reply = *call;

    if (reply.isError()) {
        call->deleteLater();
        return;
    }

    // Use watcher's window instead of the current focused window
    // since there is a time lag until filterEventFinished() returns.
    QObject *input = watcher->input();

    if (!input) {
        call->deleteLater();
        return;
    }

    Qt::KeyboardModifiers modifiers = watcher->modifiers();
    QVariantList args = watcher->arguments();
    const ulong time = static_cast<const ulong>(args.at(0).toUInt());
    const QEvent::Type type = static_cast<const QEvent::Type>(args.at(1).toUInt());
    const int qtcode = args.at(2).toInt();
    const quint32 code = args.at(3).toUInt();
    const quint32 sym = args.at(4).toUInt();
    const quint32 state = args.at(5).toUInt();
    const QString string = args.at(6).toString();
    const bool isAutoRepeat = args.at(7).toBool();
    const int count = args.at(8).toInt();

    // copied from QXcbKeyboard::handleKeyEvent()
    bool retval = reply.value();
    qCDebug(qtQpaInputMethods) << "filterEventFinished return" << code << sym << state << retval;
    if (!retval) {
#ifndef QT_NO_CONTEXTMENU
        QWindow *window = dynamic_cast<QWindow *>(input);
        if (type == QEvent::KeyPress && qtcode == Qt::Key_Menu
            && window != NULL) {
            const QPoint globalPos = window->screen()->handle()->cursor()->pos();
            const QPoint pos = window->mapFromGlobal(globalPos);
            QWindowSystemInterface::handleContextMenuEvent(window, false, pos,
                                                           globalPos, modifiers);
        }
#endif // QT_NO_CONTEXTMENU
        QKeyEvent event(type, qtcode, modifiers, code, sym,
                        state, string, isAutoRepeat, count);
        event.setTimestamp(time);
        QCoreApplication::sendEvent(input, &event);
    }
    call->deleteLater();
}

QIBusPlatformInputContextPrivate::QIBusPlatformInputContextPrivate()
    : connection(createConnection()),
      bus(0),
      context(0),
      valid(false),
      needsSurroundingText(false)
{
    if (!connection || !connection->isConnected())
        return;

    bus = new QIBusProxy(QLatin1String("org.freedesktop.IBus"),
                         QLatin1String("/org/freedesktop/IBus"),
                         *connection);
    if (!bus->isValid()) {
        qWarning("QIBusPlatformInputContext: invalid bus.");
        return;
    }

    QDBusReply<QDBusObjectPath> ic = bus->CreateInputContext(QLatin1String("QIBusInputContext"));
    if (!ic.isValid()) {
        qWarning("QIBusPlatformInputContext: CreateInputContext failed.");
        return;
    }

    context = new QIBusInputContextProxy(QLatin1String("org.freedesktop.IBus"), ic.value().path(), *connection);

    if (!context->isValid()) {
        qWarning("QIBusPlatformInputContext: invalid input context.");
        return;
    }

    enum Capabilities {
        IBUS_CAP_PREEDIT_TEXT       = 1 << 0,
        IBUS_CAP_AUXILIARY_TEXT     = 1 << 1,
        IBUS_CAP_LOOKUP_TABLE       = 1 << 2,
        IBUS_CAP_FOCUS              = 1 << 3,
        IBUS_CAP_PROPERTY           = 1 << 4,
        IBUS_CAP_SURROUNDING_TEXT   = 1 << 5
    };
    context->SetCapabilities(IBUS_CAP_PREEDIT_TEXT|IBUS_CAP_FOCUS|IBUS_CAP_SURROUNDING_TEXT);

    if (debug)
        qDebug(">>>> valid!");
    valid = true;
}

QDBusConnection *QIBusPlatformInputContextPrivate::createConnection()
{
    QByteArray display(qgetenv("DISPLAY"));
    QByteArray host = "unix";
    QByteArray displayNumber = "0";

    int pos = display.indexOf(':');
    if (pos > 0)
        host = display.left(pos);
    ++pos;
    int pos2 = display.indexOf('.', pos);
    if (pos2 > 0)
        displayNumber = display.mid(pos, pos2 - pos);
    else
        displayNumber = display.right(pos);
    if (debug)
        qDebug() << "host=" << host << "displayNumber" << displayNumber;

    QFile file(QDir::homePath() + QLatin1String("/.config/ibus/bus/") +
               QLatin1String(QDBusConnection::localMachineId()) +
               QLatin1Char('-') + QString::fromLocal8Bit(host) + QLatin1Char('-') + QString::fromLocal8Bit(displayNumber));

    if (!file.open(QFile::ReadOnly))
        return 0;

    QByteArray address;
    int pid = -1;

    while (!file.atEnd()) {
        QByteArray line = file.readLine().trimmed();
        if (line.startsWith('#'))
            continue;

        if (line.startsWith("IBUS_ADDRESS="))
            address = line.mid(sizeof("IBUS_ADDRESS=") - 1);
        if (line.startsWith("IBUS_DAEMON_PID="))
            pid = line.mid(sizeof("IBUS_DAEMON_PID=") - 1).toInt();
    }

    if (debug)
        qDebug() << "IBUS_ADDRESS=" << address << "PID=" << pid;
    if (address.isEmpty() || pid < 0 || kill(pid, 0) != 0)
        return 0;

    return new QDBusConnection(QDBusConnection::connectToBus(QString::fromLatin1(address), QLatin1String("QIBusProxy")));
}

QT_END_NAMESPACE
