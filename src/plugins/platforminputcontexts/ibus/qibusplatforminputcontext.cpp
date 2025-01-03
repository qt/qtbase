// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only
#include "qibusplatforminputcontext.h"

#include <QDebug>
#include <QTextCharFormat>
#include <QGuiApplication>
#include <QWindow>
#include <QEvent>
#include <QFile>
#include <QFileInfo>
#include <QStandardPaths>
#include <QDBusVariant>
#include <QDBusPendingReply>
#include <QDBusReply>
#include <QDBusServiceWatcher>

#include "qibusproxy.h"
#include "qibusproxyportal.h"
#include "qibusinputcontextproxy.h"
#include "qibustypes.h"

#include <qpa/qplatformcursor.h>
#include <qpa/qplatformscreen.h>
#include <qpa/qwindowsysteminterface_p.h>

#include <private/qguiapplication_p.h>
#include <private/qxkbcommon_p.h>

#include <memory>

#include <sys/types.h>
#include <signal.h>


#ifndef IBUS_RELEASE_MASK
#define IBUS_RELEASE_MASK (1 << 30)
#define IBUS_SHIFT_MASK   (1 <<  0)
#define IBUS_CONTROL_MASK (1 <<  2)
#define IBUS_MOD1_MASK    (1 <<  3)
#define IBUS_META_MASK    (1 << 28)
#endif

QT_BEGIN_NAMESPACE

using namespace Qt::StringLiterals;

enum { debug = 0 };

class QIBusPlatformInputContextPrivate
{
    Q_DISABLE_COPY_MOVE(QIBusPlatformInputContextPrivate)
public:
    QIBusPlatformInputContextPrivate();
    ~QIBusPlatformInputContextPrivate()
    {
        // dereference QDBusConnection to actually disconnect
        serviceWatcher.setConnection(QDBusConnection(QString()));
        context = nullptr;
        portalBus = nullptr;
        bus = nullptr;
        QDBusConnection::disconnectFromBus("QIBusProxy"_L1);
    }

    static QString getSocketPath();

    void createConnection();
    void initBus();
    void createBusProxy();

    std::unique_ptr<QIBusProxy> bus;
    std::unique_ptr<QIBusProxyPortal> portalBus; // bus and portalBus are alternative.
    std::unique_ptr<QIBusInputContextProxy> context;
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
    if (!d->busConnected)
        return;

    d->context->Reset();
    d->predit = QString();
    d->attributes.clear();
}

void QIBusPlatformInputContext::commit()
{
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
}

void QIBusPlatformInputContext::cursorRectChanged()
{
    if (!d->busConnected)
        return;

    QRect r = qApp->inputMethod()->cursorRectangle().toRect();
    if (!r.isValid())
        return;

    QWindow *inputWindow = qApp->focusWindow();
    if (!inputWindow)
        return;
    if (!inputWindow->screen())
        return;

    if (QGuiApplication::platformName().startsWith("wayland"_L1)) {
        auto margins = inputWindow->frameMargins();
        r.translate(margins.left(), margins.top());
        qreal scale = inputWindow->devicePixelRatio();
        QRect newRect = QRect(r.x() * scale, r.y() * scale, r.width() * scale, r.height() * scale);
        if (debug)
            qDebug() << "microFocus" << newRect;
        d->context->SetCursorLocationRelative(newRect.x(), newRect.y(),
                                              newRect.width(), newRect.height());
        return;
    }

    // x11/xcb
    auto screenGeometry = inputWindow->screen()->geometry();
    auto point = inputWindow->mapToGlobal(r.topLeft());
    qreal scale = inputWindow->devicePixelRatio();
    auto native = (point - screenGeometry.topLeft()) * scale + screenGeometry.topLeft();
    QRect newRect(native, r.size() * scale);
    if (debug)
        qDebug() << "microFocus" << newRect;
    d->context->SetCursorLocation(newRect.x(), newRect.y(),
                                  newRect.width(), newRect.height());
}

void QIBusPlatformInputContext::setFocusObject(QObject *object)
{
    if (!d->busConnected)
        return;

    // It would seem natural here to call FocusOut() on the input method if we
    // transition from an IME accepted focus object to one that does not accept it.
    // Mysteriously however that is not sufficient to fix bug QTBUG-63066.
    if (object && !inputMethodAccepted())
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
            && window != nullptr) {
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

    // dereference QDBusConnection to actually disconnect
    d->serviceWatcher.setConnection(QDBusConnection(QString()));
    d->context = nullptr;
    d->bus = nullptr;
    d->busConnected = false;
    QDBusConnection::disconnectFromBus("QIBusProxy"_L1);

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
        connect(d->bus.get(), SIGNAL(GlobalEngineChanged(QString)), this, SLOT(globalEngineChanged(QString)));
    }

    if (d->context) {
        connect(d->context.get(), SIGNAL(CommitText(QDBusVariant)), SLOT(commitText(QDBusVariant)));
        connect(d->context.get(), SIGNAL(UpdatePreeditText(QDBusVariant,uint,bool)), this, SLOT(updatePreeditText(QDBusVariant,uint,bool)));
        connect(d->context.get(), SIGNAL(ForwardKeyEvent(uint,uint,uint)), this, SLOT(forwardKeyEvent(uint,uint,uint)));
        connect(d->context.get(), SIGNAL(DeleteSurroundingText(int,uint)), this, SLOT(deleteSurroundingText(int,uint)));
        connect(d->context.get(), SIGNAL(RequireSurroundingText()), this, SLOT(surroundingTextRequired()));
        connect(d->context.get(), SIGNAL(HidePreeditText()), this, SLOT(hidePreeditText()));
        connect(d->context.get(), SIGNAL(ShowPreeditText()), this, SLOT(showPreeditText()));
    }
}

static inline bool checkNeedPortalSupport()
{
    return QFileInfo::exists("/.flatpak-info"_L1) || qEnvironmentVariableIsSet("SNAP");
}

static bool shouldConnectIbusPortal()
{
    // honor the same env as ibus-gtk
    return (checkNeedPortalSupport() || qEnvironmentVariableIsSet("IBUS_USE_PORTAL"));
}

QIBusPlatformInputContextPrivate::QIBusPlatformInputContextPrivate()
    : usePortal(shouldConnectIbusPortal()),
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
    createConnection();
    busConnected = false;
    createBusProxy();
}

void QIBusPlatformInputContextPrivate::createBusProxy()
{
    QDBusConnection connection("QIBusProxy"_L1);
    if (!connection.isConnected())
        return;

    const char* ibusService = usePortal ? "org.freedesktop.portal.IBus" : "org.freedesktop.IBus";
    QDBusReply<QDBusObjectPath> ic;
    if (usePortal) {
        portalBus = std::make_unique<QIBusProxyPortal>(QLatin1StringView(ibusService),
                                                       "/org/freedesktop/IBus"_L1,
                                                       connection);
        if (!portalBus->isValid()) {
            qWarning("QIBusPlatformInputContext: invalid portal bus.");
            return;
        }

        ic = portalBus->CreateInputContext("QIBusInputContext"_L1);
    } else {
        bus = std::make_unique<QIBusProxy>(QLatin1StringView(ibusService),
                                           "/org/freedesktop/IBus"_L1,
                                           connection);
        if (!bus->isValid()) {
            qWarning("QIBusPlatformInputContext: invalid bus.");
            return;
        }

        ic = bus->CreateInputContext("QIBusInputContext"_L1);
    }

    serviceWatcher.removeWatchedService(ibusService);
    serviceWatcher.setConnection(connection);
    serviceWatcher.addWatchedService(ibusService);

    if (!ic.isValid()) {
        qWarning("QIBusPlatformInputContext: CreateInputContext failed.");
        return;
    }

    context = std::make_unique<QIBusInputContextProxy>(QLatin1StringView(ibusService), ic.value().path(), connection);

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
               "/ibus/bus/"_L1 +
               QLatin1StringView(QDBusConnection::localMachineId()) +
               u'-' + QString::fromLocal8Bit(host) + u'-' + QString::fromLocal8Bit(displayNumber);
}

void QIBusPlatformInputContextPrivate::createConnection()
{
    if (usePortal) {
        QDBusConnection::connectToBus(QDBusConnection::SessionBus, "QIBusProxy"_L1);
        return;
    }

    QFile file(getSocketPath());
    if (!file.open(QFile::ReadOnly))
        return;

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
        return;

    QDBusConnection::connectToBus(QString::fromLatin1(address), "QIBusProxy"_L1);
}

QT_END_NAMESPACE

#include "moc_qibusplatforminputcontext.cpp"
