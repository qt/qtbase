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
#include <qpa/qwindowsysteminterface_p.h>

#include <QtGui/private/qguiapplication_p.h>

#include <QtXkbCommonSupport/private/qxkbcommon_p.h>

#include "qibusproxy.h"
#include "qibusproxyportal.h"
#include "qibusinputcontextproxy.h"
#include "qibustypes.h"

#include <sys/types.h>
#include <signal.h>

#include <QtDBus>

#ifndef IBUS_RELEASE_MASK
#define IBUS_RELEASE_MASK (1 << 30)
#define IBUS_SHIFT_MASK   (1 <<  0)
#define IBUS_CONTROL_MASK (1 <<  2)
#define IBUS_MOD1_MASK    (1 <<  3)
#define IBUS_META_MASK    (1 << 28)
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
        delete portalBus;
        delete connection;
    }

    static QString getSocketPath();

    QDBusConnection *createConnection();
    void initBus();
    void createBusProxy();

    QDBusConnection *connection;
    QIBusProxy *bus;
    QIBusProxyPortal *portalBus; // bus and portalBus are alternative.
    QIBusInputContextProxy *context;
    QDBusServiceWatcher serviceWatcher;

    bool usePortal; // return value of shouldConnectIbusPortal
    bool valid;
    bool busConnected;
    QString predit;
    QList<QInputMethodEvent::Attribute> attributes;
    bool needsSurroundingText;
    QLocale locale;
};


QIBusPlatformInputContext::QIBusPlatformInputContext ()
    : d(new QIBusPlatformInputContextPrivate())
{
    if (!d->usePortal) {
        QString socketPath = QIBusPlatformInputContextPrivate::getSocketPath();
        QFile file(socketPath);
        if (file.open(QFile::ReadOnly)) {
#if QT_CONFIG(filesystemwatcher)
            qCDebug(qtQpaInputMethods) << "socketWatcher.addPath" << socketPath;
            // If KDE session save is used or restart ibus-daemon,
            // the applications could run before ibus-daemon runs.
            // We watch the getSocketPath() to get the launching ibus-daemon.
            m_socketWatcher.addPath(socketPath);
            connect(&m_socketWatcher, SIGNAL(fileChanged(QString)), this, SLOT(socketChanged(QString)));
#endif
        }
        m_timer.setSingleShot(true);
        connect(&m_timer, SIGNAL(timeout()), this, SLOT(connectToBus()));
    }

    QObject::connect(&d->serviceWatcher, SIGNAL(serviceRegistered(QString)), this, SLOT(busRegistered(QString)));
    QObject::connect(&d->serviceWatcher, SIGNAL(serviceUnregistered(QString)), this, SLOT(busUnregistered(QString)));

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
    return d->valid && d->busConnected;
}

bool QIBusPlatformInputContext::hasCapability(Capability capability) const
{
    switch (capability) {
    case QPlatformInputContext::HiddenTextCapability:
        return false; // QTBUG-40691, do not show IME on desktop for password entry fields.
    default:
        break;
    }
    return true;
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
    d->attributes.clear();
}

void QIBusPlatformInputContext::commit()
{
    QPlatformInputContext::commit();

    if (!d->busConnected)
        return;

    QObject *input = qApp->focusObject();
    if (!input) {
        d->predit = QString();
        d->attributes.clear();
        return;
    }

    if (!d->predit.isEmpty()) {
        QInputMethodEvent event;
        event.setCommitString(d->predit);
        QCoreApplication::sendEvent(input, &event);
    }

    d->context->Reset();
    d->predit = QString();
    d->attributes.clear();
}


void QIBusPlatformInputContext::update(Qt::InputMethodQueries q)
{
    QObject *input = qApp->focusObject();

    if (d->needsSurroundingText && input
            && (q.testFlag(Qt::ImSurroundingText)
                || q.testFlag(Qt::ImCursorPosition)
                || q.testFlag(Qt::ImAnchorPosition))) {

        QInputMethodQueryEvent query(Qt::ImSurroundingText | Qt::ImCursorPosition | Qt::ImAnchorPosition);

        QCoreApplication::sendEvent(input, &query);

        QString surroundingText = query.value(Qt::ImSurroundingText).toString();
        uint cursorPosition = query.value(Qt::ImCursorPosition).toUInt();
        uint anchorPosition = query.value(Qt::ImAnchorPosition).toUInt();

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

    // It would seem natural here to call FocusOut() on the input method if we
    // transition from an IME accepted focus object to one that does not accept it.
    // Mysteriously however that is not sufficient to fix bug QTBUG-63066.
    if (!inputMethodAccepted())
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

    const QDBusArgument arg = qvariant_cast<QDBusArgument>(text.variant());

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
    d->attributes.clear();
}

void QIBusPlatformInputContext::updatePreeditText(const QDBusVariant &text, uint cursorPos, bool visible)
{
    if (!qApp)
        return;

    QObject *input = qApp->focusObject();
    if (!input)
        return;

    const QDBusArgument arg = qvariant_cast<QDBusArgument>(text.variant());

    QIBusText t;
    arg >> t;
    if (debug)
        qDebug() << "preedit text:" << t.text;

    d->attributes = t.attributes.imAttributes();
    if (!t.text.isEmpty())
        d->attributes += QInputMethodEvent::Attribute(QInputMethodEvent::Cursor, cursorPos, visible ? 1 : 0, QVariant());

    QInputMethodEvent event(t.text, d->attributes);
    QCoreApplication::sendEvent(input, &event);

    d->predit = t.text;
}

void QIBusPlatformInputContext::forwardKeyEvent(uint keyval, uint keycode, uint state)
{
    if (!qApp)
        return;

    QObject *input = qApp->focusObject();
    if (!input)
        return;

    QEvent::Type type = QEvent::KeyPress;
    if (state & IBUS_RELEASE_MASK)
        type = QEvent::KeyRelease;

    state &= ~IBUS_RELEASE_MASK;
    keycode += 8;

    Qt::KeyboardModifiers modifiers = Qt::NoModifier;
    if (state & IBUS_SHIFT_MASK)
        modifiers |= Qt::ShiftModifier;
    if (state & IBUS_CONTROL_MASK)
        modifiers |= Qt::ControlModifier;
    if (state & IBUS_MOD1_MASK)
        modifiers |= Qt::AltModifier;
    if (state & IBUS_META_MASK)
        modifiers |= Qt::MetaModifier;

    int qtcode = QXkbCommon::keysymToQtKey(keyval, modifiers);
    QString text = QXkbCommon::lookupStringNoKeysymTransformations(keyval);

    if (debug)
        qDebug() << "forwardKeyEvent" << keyval << keycode << state << modifiers << qtcode << text;

    QKeyEvent event(type, qtcode, modifiers, keycode, keyval, state, text);
    QCoreApplication::sendEvent(input, &event);
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

void QIBusPlatformInputContext::hidePreeditText()
{
    QObject *input = QGuiApplication::focusObject();
    if (!input)
        return;

    QList<QInputMethodEvent::Attribute> attributes;
    QInputMethodEvent event(QString(), attributes);
    QCoreApplication::sendEvent(input, &event);
}

void QIBusPlatformInputContext::showPreeditText()
{
    QObject *input = QGuiApplication::focusObject();
    if (!input)
        return;

    QInputMethodEvent event(d->predit, d->attributes);
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
        bool filtered = reply.value();
        qCDebug(qtQpaInputMethods) << "filterEvent return" << code << sym << state << filtered;
        return filtered;
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
    const ulong time = static_cast<ulong>(args.at(0).toUInt());
    const QEvent::Type type = static_cast<QEvent::Type>(args.at(1).toUInt());
    const int qtcode = args.at(2).toInt();
    const quint32 code = args.at(3).toUInt();
    const quint32 sym = args.at(4).toUInt();
    const quint32 state = args.at(5).toUInt();
    const QString string = args.at(6).toString();
    const bool isAutoRepeat = args.at(7).toBool();

    // copied from QXcbKeyboard::handleKeyEvent()
    bool filtered = reply.value();
    qCDebug(qtQpaInputMethods) << "filterEventFinished return" << code << sym << state << filtered;
    if (!filtered) {
#ifndef QT_NO_CONTEXTMENU
        if (type == QEvent::KeyPress && qtcode == Qt::Key_Menu
            && window != NULL) {
            const QPoint globalPos = window->screen()->handle()->cursor()->pos();
            const QPoint pos = window->mapFromGlobal(globalPos);
            QWindowSystemInterfacePrivate::ContextMenuEvent contextMenuEvent(window, false, pos,
                                                                             globalPos, modifiers);
            QGuiApplicationPrivate::processWindowSystemEvent(&contextMenuEvent);
        }
#endif
        QWindowSystemInterfacePrivate::KeyEvent keyEvent(window, time, type, qtcode, modifiers,
                                                         code, sym, state, string, isAutoRepeat);
        QGuiApplicationPrivate::processWindowSystemEvent(&keyEvent);
    }
    call->deleteLater();
}

QLocale QIBusPlatformInputContext::locale() const
{
    // d->locale is not updated when IBus portal is used
    if (d->usePortal)
        return QPlatformInputContext::locale();
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

void QIBusPlatformInputContext::busRegistered(const QString &str)
{
    qCDebug(qtQpaInputMethods) << "busRegistered";
    Q_UNUSED (str);
    if (d->usePortal) {
        connectToBus();
    }
}

void QIBusPlatformInputContext::busUnregistered(const QString &str)
{
    qCDebug(qtQpaInputMethods) << "busUnregistered";
    Q_UNUSED (str);
    d->busConnected = false;
}

// When getSocketPath() is modified, the bus is not established yet
// so use m_timer.
void QIBusPlatformInputContext::connectToBus()
{
    qCDebug(qtQpaInputMethods) << "QIBusPlatformInputContext::connectToBus";
    d->initBus();
    connectToContextSignals();

#if QT_CONFIG(filesystemwatcher)
    if (!d->usePortal && m_socketWatcher.files().size() == 0)
        m_socketWatcher.addPath(QIBusPlatformInputContextPrivate::getSocketPath());
#endif
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
        connect(d->context, SIGNAL(ForwardKeyEvent(uint,uint,uint)), this, SLOT(forwardKeyEvent(uint,uint,uint)));
        connect(d->context, SIGNAL(DeleteSurroundingText(int,uint)), this, SLOT(deleteSurroundingText(int,uint)));
        connect(d->context, SIGNAL(RequireSurroundingText()), this, SLOT(surroundingTextRequired()));
        connect(d->context, SIGNAL(HidePreeditText()), this, SLOT(hidePreeditText()));
        connect(d->context, SIGNAL(ShowPreeditText()), this, SLOT(showPreeditText()));
    }
}

static inline bool checkRunningUnderFlatpak()
{
    return !QStandardPaths::locate(QStandardPaths::RuntimeLocation, QLatin1String("flatpak-info")).isEmpty();
}

static bool shouldConnectIbusPortal()
{
    // honor the same env as ibus-gtk
    return (checkRunningUnderFlatpak() || !qgetenv("IBUS_USE_PORTAL").isNull());
}

QIBusPlatformInputContextPrivate::QIBusPlatformInputContextPrivate()
    : connection(0),
      bus(0),
      portalBus(0),
      context(0),
      usePortal(shouldConnectIbusPortal()),
      valid(false),
      busConnected(false),
      needsSurroundingText(false)
{
    if (usePortal) {
        valid = true;
        if (debug)
            qDebug() << "use IBus portal";
    } else {
        valid = !QStandardPaths::findExecutable(QString::fromLocal8Bit("ibus-daemon"), QStringList()).isEmpty();
    }
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

    const char* ibusService = usePortal ? "org.freedesktop.portal.IBus" : "org.freedesktop.IBus";
    QDBusReply<QDBusObjectPath> ic;
    if (usePortal) {
        portalBus = new QIBusProxyPortal(QLatin1String(ibusService),
                                         QLatin1String("/org/freedesktop/IBus"),
                                         *connection);
        if (!portalBus->isValid()) {
            qWarning("QIBusPlatformInputContext: invalid portal bus.");
            return;
        }

        ic = portalBus->CreateInputContext(QLatin1String("QIBusInputContext"));
    } else {
        bus = new QIBusProxy(QLatin1String(ibusService),
                             QLatin1String("/org/freedesktop/IBus"),
                             *connection);
        if (!bus->isValid()) {
            qWarning("QIBusPlatformInputContext: invalid bus.");
            return;
        }

        ic = bus->CreateInputContext(QLatin1String("QIBusInputContext"));
    }

    serviceWatcher.removeWatchedService(ibusService);
    serviceWatcher.setConnection(*connection);
    serviceWatcher.addWatchedService(ibusService);

    if (!ic.isValid()) {
        qWarning("QIBusPlatformInputContext: CreateInputContext failed.");
        return;
    }

    context = new QIBusInputContextProxy(QLatin1String(ibusService), ic.value().path(), *connection);

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
    QByteArray display;
    QByteArray displayNumber = "0";
    bool isWayland = false;

    if (qEnvironmentVariableIsSet("IBUS_ADDRESS_FILE")) {
        QByteArray path = qgetenv("IBUS_ADDRESS_FILE");
        return QString::fromLocal8Bit(path);
    } else  if (qEnvironmentVariableIsSet("WAYLAND_DISPLAY")) {
        display = qgetenv("WAYLAND_DISPLAY");
        isWayland = true;
    } else {
        display = qgetenv("DISPLAY");
    }
    QByteArray host = "unix";

    if (isWayland) {
        displayNumber = display;
    } else {
        int pos = display.indexOf(':');
        if (pos > 0)
            host = display.left(pos);
        ++pos;
        int pos2 = display.indexOf('.', pos);
        if (pos2 > 0)
            displayNumber = display.mid(pos, pos2 - pos);
         else
            displayNumber = display.mid(pos);
    }

    if (debug)
        qDebug() << "host=" << host << "displayNumber" << displayNumber;

    return QStandardPaths::writableLocation(QStandardPaths::ConfigLocation) +
               QLatin1String("/ibus/bus/") +
               QLatin1String(QDBusConnection::localMachineId()) +
               QLatin1Char('-') + QString::fromLocal8Bit(host) + QLatin1Char('-') + QString::fromLocal8Bit(displayNumber);
}

QDBusConnection *QIBusPlatformInputContextPrivate::createConnection()
{
    if (usePortal)
        return new QDBusConnection(QDBusConnection::connectToBus(QDBusConnection::SessionBus, QLatin1String("QIBusProxy")));
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
