/****************************************************************************
**
** Copyright (C) 2016 The Qt Company Ltd.
** Contact: https://www.qt.io/licensing/
**
** This file is part of the plugins of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:LGPL$
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company. For licensing terms
** and conditions see https://www.qt.io/terms-conditions. For further
** information use the contact form at https://www.qt.io/contact-us.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 3 as published by the Free Software
** Foundation and appearing in the file LICENSE.LGPL3 included in the
** packaging of this file. Please review the following information to
** ensure the GNU Lesser General Public License version 3 requirements
** will be met: https://www.gnu.org/licenses/lgpl-3.0.html.
**
** GNU General Public License Usage
** Alternatively, this file may be used under the terms of the GNU
** General Public License version 2.0 or (at your option) the GNU General
** Public license version 3 or any later version approved by the KDE Free
** Qt Foundation. The licenses are as published by the Free Software
** Foundation and appearing in the file LICENSE.GPL2 and LICENSE.GPL3
** included in the packaging of this file. Please review the following
** information to ensure the GNU General Public License requirements will
** be met: https://www.gnu.org/licenses/gpl-2.0.html and
** https://www.gnu.org/licenses/gpl-3.0.html.
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

    static QString getSocketPath();
    static QDBusConnection *createConnection();

    void initBus();
    void createBusProxy();

    QDBusConnection *connection;
    QIBusProxy *bus;
    QIBusInputContextProxy *context;

    bool valid;
    bool busConnected;
    QString predit;
    bool needsSurroundingText;
    QLocale locale;
};


QIBusPlatformInputContext::QIBusPlatformInputContext ()
    : d(new QIBusPlatformInputContextPrivate())
{
    QString socketPath = QIBusPlatformInputContextPrivate::getSocketPath();
    QFile file(socketPath);
    if (file.open(QFile::ReadOnly)) {
        // If KDE session save is used or restart ibus-daemon,
        // the applications could run before ibus-daemon runs.
        // We watch the getSocketPath() to get the launching ibus-daemon.
        m_socketWatcher.addPath(socketPath);
        connect(&m_socketWatcher, SIGNAL(fileChanged(QString)), this, SLOT(socketChanged(QString)));
    }

    m_timer.setSingleShot(true);
    connect(&m_timer, SIGNAL(timeout()), this, SLOT(connectToBus()));

    connectToContextSignals();

    QInputMethod *p = qApp->inputMethod();
    connect(p, SIGNAL(cursorRectangleChanged()), this, SLOT(cursorRectChanged()));
    m_eventFilterUseSynchronousMode = false;
    if (qEnvironmentVariableIsSet("IBUS_ENABLE_SYNC_MODE")) {
        bool ok;
        int enableSync = qEnvironmentVariableIntValue("IBUS_ENABLE_SYNC_MODE", &ok);
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
    if (!d->busConnected)
        return;

    if (a == QInputMethod::Click)
        commit();
}

void QIBusPlatformInputContext::reset()
{
    QPlatformInputContext::reset();

    if (!d->busConnected)
        return;

    d->context->Reset();
    d->predit = QString();
}

void QIBusPlatformInputContext::commit()
{
    QPlatformInputContext::commit();

    if (!d->busConnected)
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
    if (!d->busConnected)
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
    if (!d->busConnected)
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
        qDebug("surroundingTextRequired");
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
    if (!d->busConnected)
        return false;

    if (!inputMethodAccepted())
        return false;

    const QKeyEvent *keyEvent = static_cast<const QKeyEvent *>(event);
    quint32 sym = keyEvent->nativeVirtualKey();
    quint32 code = keyEvent->nativeScanCode();
    quint32 state = keyEvent->nativeModifiers();
    quint32 ibusState = state;

    if (keyEvent->type() != QEvent::KeyPress)
        ibusState |= IBUS_RELEASE_MASK;

    QDBusPendingReply<bool> reply = d->context->ProcessKeyEvent(sym, code - 8, ibusState);

    if (m_eventFilterUseSynchronousMode || reply.isFinished()) {
        bool retval = reply.value();
        qCDebug(qtQpaInputMethods) << "filterEvent return" << code << sym << state << retval;
        return retval;
    }

    Qt::KeyboardModifiers modifiers = keyEvent->modifiers();
    const int qtcode = keyEvent->key();

    // From QKeyEvent::modifiers()
    switch (qtcode) {
    case Qt::Key_Shift:
        modifiers ^= Qt::ShiftModifier;
        break;
    case Qt::Key_Control:
        modifiers ^= Qt::ControlModifier;
        break;
    case Qt::Key_Alt:
        modifiers ^= Qt::AltModifier;
        break;
    case Qt::Key_Meta:
        modifiers ^= Qt::MetaModifier;
        break;
    case Qt::Key_AltGr:
        modifiers ^= Qt::GroupSwitchModifier;
        break;
    }

    QVariantList args;
    args << QVariant::fromValue(keyEvent->timestamp());
    args << QVariant::fromValue(static_cast<uint>(keyEvent->type()));
    args << QVariant::fromValue(qtcode);
    args << QVariant::fromValue(code) << QVariant::fromValue(sym) << QVariant::fromValue(state);
    args << QVariant::fromValue(keyEvent->text());
    args << QVariant::fromValue(keyEvent->isAutoRepeat());

    QIBusFilterEventWatcher *watcher = new QIBusFilterEventWatcher(reply, this, QGuiApplication::focusWindow(), modifiers, args);
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
    QWindow *window = watcher->window();

    if (!window) {
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

    // copied from QXcbKeyboard::handleKeyEvent()
    bool retval = reply.value();
    qCDebug(qtQpaInputMethods) << "filterEventFinished return" << code << sym << state << retval;
    if (!retval) {
#ifndef QT_NO_CONTEXTMENU
        if (type == QEvent::KeyPress && qtcode == Qt::Key_Menu
            && window != NULL) {
            const QPoint globalPos = window->screen()->handle()->cursor()->pos();
            const QPoint pos = window->mapFromGlobal(globalPos);
            QWindowSystemInterface::handleContextMenuEvent(window, false, pos,
                                                           globalPos, modifiers);
        }
#endif // QT_NO_CONTEXTMENU
        QWindowSystemInterface::handleExtendedKeyEvent(window, time, type, qtcode, modifiers,
                                                       code, sym, state, string, isAutoRepeat);

    }
    call->deleteLater();
}

QLocale QIBusPlatformInputContext::locale() const
{
    return d->locale;
}

void QIBusPlatformInputContext::socketChanged(const QString &str)
{
    qCDebug(qtQpaInputMethods) << "socketChanged";
    Q_UNUSED (str);

    m_timer.stop();

    if (d->context)
        disconnect(d->context);
    if (d->bus && d->bus->isValid())
        disconnect(d->bus);
    if (d->connection)
        d->connection->disconnectFromBus(QLatin1String("QIBusProxy"));

    m_timer.start(100);
}

// When getSocketPath() is modified, the bus is not established yet
// so use m_timer.
void QIBusPlatformInputContext::connectToBus()
{
    qCDebug(qtQpaInputMethods) << "QIBusPlatformInputContext::connectToBus";
    d->initBus();
    connectToContextSignals();

    if (m_socketWatcher.files().size() == 0)
        m_socketWatcher.addPath(QIBusPlatformInputContextPrivate::getSocketPath());
}

void QIBusPlatformInputContext::globalEngineChanged(const QString &engine_name)
{
    if (!d->bus || !d->bus->isValid())
        return;

    QIBusEngineDesc desc = d->bus->getGlobalEngine();
    Q_ASSERT(engine_name == desc.engine_name);
    QLocale locale(desc.language);
    if (d->locale != locale) {
        d->locale = locale;
        emitLocaleChanged();
    }
}

void QIBusPlatformInputContext::connectToContextSignals()
{
    if (d->bus && d->bus->isValid()) {
        connect(d->bus, SIGNAL(GlobalEngineChanged(QString)), this, SLOT(globalEngineChanged(QString)));
    }

    if (d->context) {
        connect(d->context, SIGNAL(CommitText(QDBusVariant)), SLOT(commitText(QDBusVariant)));
        connect(d->context, SIGNAL(UpdatePreeditText(QDBusVariant,uint,bool)), this, SLOT(updatePreeditText(QDBusVariant,uint,bool)));
        connect(d->context, SIGNAL(DeleteSurroundingText(int,uint)), this, SLOT(deleteSurroundingText(int,uint)));
        connect(d->context, SIGNAL(RequireSurroundingText()), this, SLOT(surroundingTextRequired()));
    }
}

QIBusPlatformInputContextPrivate::QIBusPlatformInputContextPrivate()
    : connection(0),
      bus(0),
      context(0),
      valid(false),
      busConnected(false),
      needsSurroundingText(false)
{
    valid = !QStandardPaths::findExecutable(QString::fromLocal8Bit("ibus-daemon"), QStringList()).isEmpty();
    if (!valid)
        return;
    initBus();

    if (bus && bus->isValid()) {
        QIBusEngineDesc desc = bus->getGlobalEngine();
        locale = QLocale(desc.language);
    }
}

void QIBusPlatformInputContextPrivate::initBus()
{
    connection = createConnection();
    busConnected = false;
    createBusProxy();
}

void QIBusPlatformInputContextPrivate::createBusProxy()
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
        qDebug(">>>> bus connected!");
    busConnected = true;
}

QString QIBusPlatformInputContextPrivate::getSocketPath()
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

    return QDir::homePath() + QLatin1String("/.config/ibus/bus/") +
               QLatin1String(QDBusConnection::localMachineId()) +
               QLatin1Char('-') + QString::fromLocal8Bit(host) + QLatin1Char('-') + QString::fromLocal8Bit(displayNumber);
}

QDBusConnection *QIBusPlatformInputContextPrivate::createConnection()
{
    QFile file(getSocketPath());

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
