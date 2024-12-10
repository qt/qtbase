// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#include "qdbustrayicon_p.h"

#ifndef QT_NO_SYSTEMTRAYICON

#include <QString>
#include <QDebug>
#include <QRect>
#include <QLoggingCategory>
#include <QStandardPaths>
#include <QFileInfo>
#include <QDir>
#include <QMetaObject>
#include <QMetaEnum>
#include <QDBusConnectionInterface>
#include <QDBusArgument>
#include <QDBusMetaType>
#include <QDBusServiceWatcher>

#include <qpa/qplatformmenu.h>
#include <qpa/qplatformintegration.h>
#include <qpa/qplatformservices.h>

#include <private/qdbusmenuconnection_p.h>
#include <private/qstatusnotifieritemadaptor_p.h>
#include <private/qdbusmenuadaptor_p.h>
#include <private/qdbusplatformmenu_p.h>
#include <private/qxdgnotificationproxy_p.h>
#include <private/qlockfile_p.h>
#include <private/qguiapplication_p.h>

// Defined in Windows headers which get included by qlockfile_p.h
#undef interface

QT_BEGIN_NAMESPACE

using namespace Qt::StringLiterals;

Q_LOGGING_CATEGORY(qLcTray, "qt.qpa.tray")

static QString iconTempPath()
{
    QString tempPath = QStandardPaths::writableLocation(QStandardPaths::RuntimeLocation);
    if (!tempPath.isEmpty()) {
        QString flatpakId = qEnvironmentVariable("FLATPAK_ID");
        if (!flatpakId.isEmpty() && QFileInfo::exists("/.flatpak-info"_L1))
            tempPath += "/app/"_L1 + flatpakId;
        return tempPath;
    }

    tempPath = QStandardPaths::writableLocation(QStandardPaths::GenericCacheLocation);

    if (!tempPath.isEmpty()) {
        QDir tempDir(tempPath);
        if (tempDir.exists())
            return tempPath;

        if (tempDir.mkpath(QStringLiteral("."))) {
            const QFile::Permissions permissions = QFile::ReadOwner | QFile::WriteOwner | QFile::ExeOwner;
            if (QFile(tempPath).setPermissions(permissions))
                return tempPath;
        }
    }

    return QDir::tempPath();
}

static const QString KDEItemFormat = QStringLiteral("org.kde.StatusNotifierItem-%1-%2");
static const QString KDEWatcherService = QStringLiteral("org.kde.StatusNotifierWatcher");
static const QString XdgNotificationService = QStringLiteral("org.freedesktop.Notifications");
static const QString XdgNotificationPath = QStringLiteral("/org/freedesktop/Notifications");
static const QString DefaultAction = QStringLiteral("default");
static int instanceCount = 0;

static inline QString tempFileTemplate()
{
    static const QString TempFileTemplate = iconTempPath() + "/qt-trayicon-XXXXXX.png"_L1;
    return TempFileTemplate;
}

/*!
    \class QDBusTrayIcon
    \internal
*/

QDBusTrayIcon::QDBusTrayIcon()
    : m_dbusConnection(nullptr)
    , m_adaptor(new QStatusNotifierItemAdaptor(this))
    , m_menuAdaptor(nullptr)
    , m_menu(nullptr)
    , m_notifier(nullptr)
    , m_instanceId(KDEItemFormat.arg(QCoreApplication::applicationPid()).arg(++instanceCount))
    , m_category(QStringLiteral("ApplicationStatus"))
    , m_defaultStatus(QStringLiteral("Active")) // be visible all the time.  QSystemTrayIcon has no API to control this.
    , m_status(m_defaultStatus)
    , m_tempIcon(nullptr)
    , m_tempAttentionIcon(nullptr)
    , m_registered(false)
{
    qCDebug(qLcTray);
    if (instanceCount == 1) {
        QDBusMenuItem::registerDBusTypes();
        qDBusRegisterMetaType<QXdgDBusImageStruct>();
        qDBusRegisterMetaType<QXdgDBusImageVector>();
        qDBusRegisterMetaType<QXdgDBusToolTipStruct>();
    }
    connect(this, SIGNAL(statusChanged(QString)), m_adaptor, SIGNAL(NewStatus(QString)));
    connect(this, SIGNAL(tooltipChanged()), m_adaptor, SIGNAL(NewToolTip()));
    connect(this, SIGNAL(iconChanged()), m_adaptor, SIGNAL(NewIcon()));
    connect(this, SIGNAL(attention()), m_adaptor, SIGNAL(NewAttentionIcon()));
    connect(this, SIGNAL(menuChanged()), m_adaptor, SIGNAL(NewMenu()));
    connect(this, SIGNAL(attention()), m_adaptor, SIGNAL(NewTitle()));
    connect(&m_attentionTimer, SIGNAL(timeout()), this, SLOT(attentionTimerExpired()));
    m_attentionTimer.setSingleShot(true);
}

QDBusTrayIcon::~QDBusTrayIcon()
{
}

void QDBusTrayIcon::init()
{
    qCDebug(qLcTray) << "registering" << m_instanceId;
    m_registered = dBusConnection()->registerTrayIcon(this);
    QObject::connect(dBusConnection()->dbusWatcher(), &QDBusServiceWatcher::serviceRegistered,
                     this, &QDBusTrayIcon::watcherServiceRegistered);
}

void QDBusTrayIcon::cleanup()
{
    qCDebug(qLcTray) << "unregistering" << m_instanceId;
    if (m_registered)
        dBusConnection()->unregisterTrayIcon(this);
    delete m_dbusConnection;
    m_dbusConnection = nullptr;
    delete m_notifier;
    m_notifier = nullptr;
    m_registered = false;
}

void QDBusTrayIcon::watcherServiceRegistered(const QString &serviceName)
{
    Q_UNUSED(serviceName);
    // We have the icon registered, but the watcher has restarted or
    // changed, so we need to tell it about our icon again
    if (m_registered)
        dBusConnection()->registerTrayIconWithWatcher(this);
}

void QDBusTrayIcon::attentionTimerExpired()
{
    m_messageTitle = QString();
    m_message = QString();
    m_attentionIcon = QIcon();
    emit attention();
    emit tooltipChanged();
    setStatus(m_defaultStatus);
}

void QDBusTrayIcon::setStatus(const QString &status)
{
    qCDebug(qLcTray) << status;
    if (m_status == status)
        return;
    m_status = status;
    emit statusChanged(m_status);
}

QTemporaryFile *QDBusTrayIcon::tempIcon(const QIcon &icon)
{
    // Hack for indicator-application, which doesn't handle icons sent across D-Bus:
    // save the icon to a temp file and set the icon name to that filename.
    static bool necessity_checked = false;
    static bool necessary = false;
    if (!necessity_checked) {
        QDBusConnection session = QDBusConnection::sessionBus();
        uint pid = session.interface()->servicePid(KDEWatcherService).value();
        QString processName = QLockFilePrivate::processNameByPid(pid);
        necessary = processName.endsWith("indicator-application-service"_L1);
        if (!necessary) {
            necessary = session.interface()->isServiceRegistered(
                QStringLiteral("com.canonical.indicator.application"));
        }
        if (!necessary) {
            necessary = session.interface()->isServiceRegistered(
                QStringLiteral("org.ayatana.indicator.application"));
        }
        if (!necessary && QGuiApplication::desktopSettingsAware()) {
            // Accessing to process name might be not allowed if the application
            // is confined, thus we can just rely on the current desktop in use
            const QPlatformServices *services = QGuiApplicationPrivate::platformIntegration()->services();
            if (services)
                necessary = services->desktopEnvironment().split(':').contains("UNITY");
        }
        necessity_checked = true;
    }
    if (!necessary)
        return nullptr;
    QTemporaryFile *ret = new QTemporaryFile(tempFileTemplate(), this);
    if (!ret->open()) {
        delete ret;
        return nullptr;
    }
    icon.pixmap(QSize(22, 22)).save(ret);
    ret->close();
    return ret;
}

QDBusMenuConnection * QDBusTrayIcon::dBusConnection()
{
    if (!m_dbusConnection) {
        m_dbusConnection = new QDBusMenuConnection(this, m_instanceId);
        m_notifier = new QXdgNotificationInterface(XdgNotificationService,
            XdgNotificationPath, m_dbusConnection->connection(), this);
        connect(m_notifier, SIGNAL(NotificationClosed(uint,uint)), this, SLOT(notificationClosed(uint,uint)));
        connect(m_notifier, SIGNAL(ActionInvoked(uint,QString)), this, SLOT(actionInvoked(uint,QString)));
    }
    return m_dbusConnection;
}

void QDBusTrayIcon::updateIcon(const QIcon &icon)
{
    m_iconName = icon.name();
    m_icon = icon;
    if (m_iconName.isEmpty()) {
        if (m_tempIcon)
            delete m_tempIcon;
        m_tempIcon = tempIcon(icon);
        if (m_tempIcon)
            m_iconName = m_tempIcon->fileName();
    }
    qCDebug(qLcTray) << m_iconName << icon.availableSizes();
    emit iconChanged();
}

void QDBusTrayIcon::updateToolTip(const QString &tooltip)
{
    qCDebug(qLcTray) << tooltip;
    m_tooltip = tooltip;
    emit tooltipChanged();
}

QPlatformMenu *QDBusTrayIcon::createMenu() const
{
    return new QDBusPlatformMenu();
}

void QDBusTrayIcon::updateMenu(QPlatformMenu * menu)
{
    qCDebug(qLcTray) << menu;
    QDBusPlatformMenu *newMenu = qobject_cast<QDBusPlatformMenu *>(menu);
    if (m_menu != newMenu) {
        if (m_menu) {
            dBusConnection()->unregisterTrayIconMenu(this);
            delete m_menuAdaptor;
        }
        m_menu = newMenu;
        m_menuAdaptor = new QDBusMenuAdaptor(m_menu);
        // TODO connect(m_menu, , m_menuAdaptor, SIGNAL(ItemActivationRequested(int,uint)));
        connect(m_menu, SIGNAL(propertiesUpdated(QDBusMenuItemList,QDBusMenuItemKeysList)),
                m_menuAdaptor, SIGNAL(ItemsPropertiesUpdated(QDBusMenuItemList,QDBusMenuItemKeysList)));
        connect(m_menu, SIGNAL(updated(uint,int)),
                m_menuAdaptor, SIGNAL(LayoutUpdated(uint,int)));
        dBusConnection()->registerTrayIconMenu(this);
        emit menuChanged();
    }
}

void QDBusTrayIcon::showMessage(const QString &title, const QString &msg, const QIcon &icon,
                                QPlatformSystemTrayIcon::MessageIcon iconType, int msecs)
{
    m_messageTitle = title;
    m_message = msg;
    m_attentionIcon = icon;
    QStringList notificationActions;
    switch (iconType) {
    case Information:
        m_attentionIconName = QStringLiteral("dialog-information");
        break;
    case Warning:
        m_attentionIconName = QStringLiteral("dialog-warning");
        break;
    case Critical:
        m_attentionIconName = QStringLiteral("dialog-error");
        // If there are actions, the desktop notification may appear as a message dialog
        // with button(s), which will interrupt the user and require a response.
        // That is an optional feature in implementations of org.freedesktop.Notifications
        notificationActions << DefaultAction << tr("OK");
        break;
    default:
        m_attentionIconName.clear();
        break;
    }
    if (m_attentionIconName.isEmpty()) {
        if (m_tempAttentionIcon)
            delete m_tempAttentionIcon;
        m_tempAttentionIcon = tempIcon(icon);
        if (m_tempAttentionIcon)
            m_attentionIconName = m_tempAttentionIcon->fileName();
    }
    qCDebug(qLcTray) << title << msg <<
        QPlatformSystemTrayIcon::metaObject()->enumerator(
            QPlatformSystemTrayIcon::staticMetaObject.indexOfEnumerator("MessageIcon")).valueToKey(iconType)
        << m_attentionIconName << msecs;
    setStatus(QStringLiteral("NeedsAttention"));
    m_attentionTimer.start(msecs);
    emit tooltipChanged();
    emit attention();

    // Desktop notification
    QVariantMap hints;
    // urgency levels according to https://developer.gnome.org/notification-spec/#urgency-levels
    // 0 low, 1 normal, 2 critical
    int urgency = static_cast<int>(iconType) - 1;
    if (urgency < 0) // no icon
        urgency = 0;
    hints.insert("urgency"_L1, QVariant(urgency));
    m_notifier->notify(QCoreApplication::applicationName(), 0,
                       m_attentionIconName, title, msg, notificationActions, hints, msecs);
}

void QDBusTrayIcon::actionInvoked(uint id, const QString &action)
{
    qCDebug(qLcTray) << id << action;
    emit messageClicked();
}

void QDBusTrayIcon::notificationClosed(uint id, uint reason)
{
    qCDebug(qLcTray) << id << reason;
}

bool QDBusTrayIcon::isSystemTrayAvailable() const
{
    QDBusMenuConnection * conn = const_cast<QDBusTrayIcon *>(this)->dBusConnection();

    // If the KDE watcher service is registered, we must be on a desktop
    // where a StatusNotifier-conforming system tray exists.
    qCDebug(qLcTray) << conn->isWatcherRegistered();
    return conn->isWatcherRegistered();
}

QT_END_NAMESPACE

#include "moc_qdbustrayicon_p.cpp"
#endif //QT_NO_SYSTEMTRAYICON
