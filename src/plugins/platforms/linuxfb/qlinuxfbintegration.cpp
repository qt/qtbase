// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#include "qlinuxfbintegration.h"
#include "qlinuxfbscreen.h"
#if QT_CONFIG(kms)
#include "qlinuxfbdrmscreen.h"
#endif

#include <QtGui/private/qgenericunixfontdatabase_p.h>
#include <QtGui/private/qgenericunixservices_p.h>
#include <QtGui/private/qgenericunixeventdispatcher_p.h>

#include <QtFbSupport/private/qfbvthandler_p.h>
#include <QtFbSupport/private/qfbbackingstore_p.h>
#include <QtFbSupport/private/qfbwindow_p.h>
#include <QtFbSupport/private/qfbcursor_p.h>

#include <QtGui/private/qguiapplication_p.h>
#include <qpa/qplatforminputcontextfactory_p.h>
#include <qpa/qwindowsysteminterface.h>

#if QT_CONFIG(libinput)
#include <QtInputSupport/private/qlibinputhandler_p.h>
#endif

#if QT_CONFIG(evdev)
#include <QtInputSupport/private/qevdevmousemanager_p.h>
#include <QtInputSupport/private/qevdevkeyboardmanager_p.h>
#include <QtInputSupport/private/qevdevtouchmanager_p.h>
#endif

#if QT_CONFIG(tslib)
#include <QtInputSupport/private/qtslib_p.h>
#endif

QT_BEGIN_NAMESPACE

using namespace Qt::StringLiterals;

QLinuxFbIntegration::QLinuxFbIntegration(const QStringList &paramList)
    : m_primaryScreen(nullptr),
      m_fontDb(new QGenericUnixFontDatabase),
      m_services(new QGenericUnixServices),
      m_kbdMgr(nullptr)
{
#if QT_CONFIG(kms)
    if (qEnvironmentVariableIntValue("QT_QPA_FB_DRM") != 0)
        m_primaryScreen = new QLinuxFbDrmScreen(paramList);
#endif
    if (!m_primaryScreen)
        m_primaryScreen = new QLinuxFbScreen(paramList);
}

QLinuxFbIntegration::~QLinuxFbIntegration()
{
    QWindowSystemInterface::handleScreenRemoved(m_primaryScreen);
}

void QLinuxFbIntegration::initialize()
{
    if (m_primaryScreen->initialize())
        QWindowSystemInterface::handleScreenAdded(m_primaryScreen);
    else
        qWarning("linuxfb: Failed to initialize screen");

    m_inputContext = QPlatformInputContextFactory::create();

    m_vtHandler.reset(new QFbVtHandler);

    if (!qEnvironmentVariableIntValue("QT_QPA_FB_DISABLE_INPUT"))
        createInputHandlers();
}

bool QLinuxFbIntegration::hasCapability(QPlatformIntegration::Capability cap) const
{
    switch (cap) {
    case ThreadedPixmaps: return true;
    case WindowManagement: return false;
    case RhiBasedRendering: return false;
    default: return QPlatformIntegration::hasCapability(cap);
    }
}

QPlatformBackingStore *QLinuxFbIntegration::createPlatformBackingStore(QWindow *window) const
{
    return new QFbBackingStore(window);
}

QPlatformWindow *QLinuxFbIntegration::createPlatformWindow(QWindow *window) const
{
    return new QFbWindow(window);
}

QAbstractEventDispatcher *QLinuxFbIntegration::createEventDispatcher() const
{
    return createUnixEventDispatcher();
}

QList<QPlatformScreen *> QLinuxFbIntegration::screens() const
{
    QList<QPlatformScreen *> list;
    list.append(m_primaryScreen);
    return list;
}

QPlatformFontDatabase *QLinuxFbIntegration::fontDatabase() const
{
    return m_fontDb.data();
}

QPlatformServices *QLinuxFbIntegration::services() const
{
    return m_services.data();
}

void QLinuxFbIntegration::createInputHandlers()
{
#if QT_CONFIG(libinput)
    if (!qEnvironmentVariableIntValue("QT_QPA_FB_NO_LIBINPUT")) {
        new QLibInputHandler("libinput"_L1, QString());
        return;
    }
#endif

#if QT_CONFIG(tslib)
    bool useTslib = qEnvironmentVariableIntValue("QT_QPA_FB_TSLIB");
    if (useTslib)
        new QTsLibMouseHandler("TsLib"_L1, QString());
#endif

#if QT_CONFIG(evdev)
    m_kbdMgr = new QEvdevKeyboardManager("EvdevKeyboard"_L1, QString(), this);
    new QEvdevMouseManager("EvdevMouse"_L1, QString(), this);
#if QT_CONFIG(tslib)
    if (!useTslib)
#endif
        new QEvdevTouchManager("EvdevTouch"_L1, QString() /* spec */, this);
#endif
}

QPlatformNativeInterface *QLinuxFbIntegration::nativeInterface() const
{
    return const_cast<QLinuxFbIntegration *>(this);
}

QFunctionPointer QLinuxFbIntegration::platformFunction(const QByteArray &function) const
{
    Q_UNUSED(function);
    return nullptr;
}

#if QT_CONFIG(evdev)
void QLinuxFbIntegration::loadKeymap(const QString &filename)
{
    if (m_kbdMgr)
        m_kbdMgr->loadKeymap(filename);
    else
        qWarning("QLinuxFbIntegration: Cannot load keymap, no keyboard handler found");
}

void QLinuxFbIntegration::switchLang()
{
    if (m_kbdMgr)
        m_kbdMgr->switchLang();
    else
        qWarning("QLinuxFbIntegration: Cannot switch language, no keyboard handler found");
}
#endif

QT_END_NAMESPACE
