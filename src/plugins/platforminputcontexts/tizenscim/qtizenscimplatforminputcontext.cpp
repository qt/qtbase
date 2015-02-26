/****************************************************************************
**
** Copyright (C) 2013 Tomasz Olszak <olszak.tomasz@gmail.com>.
** Contact: http://www.qt-project.org/legal
**
** This file is part of the plugins of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:LGPL$
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and Digia.  For licensing terms and
** conditions see http://qt.digia.com/licensing.  For further information
** use the contact form at http://qt.digia.com/contact-us.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 2.1 as published by the Free Software
** Foundation and appearing in the file LICENSE.LGPL included in the
** packaging of this file.  Please review the following information to
** ensure the GNU Lesser General Public License version 2.1 requirements
** will be met: http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, Digia gives you certain additional
** rights.  These rights are described in the Digia Qt LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
** GNU General Public License Usage
** Alternatively, this file may be used under the terms of the GNU
** General Public License version 3.0 as published by the Free Software
** Foundation and appearing in the file LICENSE.GPL included in the
** packaging of this file.  Please review the following information to
** ensure the GNU General Public License version 3.0 requirements will be
** met: http://www.gnu.org/copyleft/gpl.html.
**
**
** $QT_END_LICENSE$
**
****************************************************************************/

#include "qtizenscimplatforminputcontext.h"
#include "qcoreapplication.h"

Q_LOGGING_CATEGORY(QT_TIZENSCIM_INPUT_METHOD, "qt.tizenscim.inputmethod")

#undef null
#undef emit
#define Uses_ISF_IMCONTROL_CLIENT
#define Uses_SCIM_PANEL_CLIENT
#define Uses_SCIM_CONFIG_MODULE
#define Uses_SCIM_BACKEND
#define Uses_SCIM_IMENGINE_MODULE

#include <scim.h>
#include <scim_panel_client.h>
#include <scim_backend.h>
#include <scim_imengine.h>
#include <scim_imengine_module.h>

#include <QtCore/QCoreApplication>
#include <QtGui/QKeyEvent>
#include <QtCore/QDebug>
#include <QGuiApplication>
#include <QWindow>
#include <QtNetwork/QLocalSocket>
#include <QSocketNotifier>
#include <QMap>


#define SHIFT_MODE_OFF  0xffe1
#define SHIFT_MODE_ON   0xffe2
#define SHIFT_MODE_LOCK 0xffe6
#define SHIFT_MODE_ENABLE 0x9fe7
#define SHIFT_MODE_DISABLE 0x9fe8

using namespace scim;
QT_BEGIN_NAMESPACE

namespace TizenScim {
    enum Ecore_IMF_Input_Panel_Event {
      ECORE_IMF_INPUT_PANEL_STATE_EVENT,
      ECORE_IMF_INPUT_PANEL_LANGUAGE_EVENT,
      ECORE_IMF_INPUT_PANEL_SHIFT_MODE_EVENT,
      ECORE_IMF_INPUT_PANEL_GEOMETRY_EVENT,
      ECORE_IMF_CANDIDATE_PANEL_STATE_EVENT,
      ECORE_IMF_CANDIDATE_PANEL_GEOMETRY_EVENT
    };

    static QHash<int, int> scimKeyCodeToQtKeyCode;
    static QTizenScimInputContext *tizenInputContext;
    static int currentPanelShiftMode = SHIFT_MODE_OFF;
    static bool shiftModeEnabled = true;
    static int contextId = 0;
    static int clientId = 0;
    static String iseUuid = "";
    static PanelClient panelClient;
    static IMControlClient imControlClient;
    static QSocketNotifier *socketNotifier = 0;
    static QSocketNotifier *imControlSocketNotifier = 0;
    static QRectF panelClientRectangle;
    static QScopedPointer<ConfigModule> configModule;
    static ConfigPointer config;
    static String configModuleName = "simple";
    static bool visible = false;
    static QObject *focusObject = 0;
    static IMEngineFactoryPointer imEngineFactory;
    static IMEngineInstancePointer imEngine;
    static BackEndPointer backend;
    static void initializePanel();
    static void initializeScim();
    static void reloadConfigCallback(const ConfigPointer &config);
    static bool checkSocketFrontend();
    static void adjustCapsMode();
    // panelClient slots
    static void panelSlotProcessKeyEvent(int context, const KeyEvent &key);
    static void panelSlotCommitString(int context, const WideString &wstr);
    static void panelSlotForwardKeyEvent(int context, const KeyEvent &key);

    void panelSlotProcessKeyEvent(int /*context*/, const KeyEvent &key)
    {
        qCDebug(QT_TIZENSCIM_INPUT_METHOD) << QString::number(key.code, 16);

        bool doReturn = true;
        if (key.code == SHIFT_MODE_ENABLE)
            shiftModeEnabled = true;
        else if (key.code == SHIFT_MODE_DISABLE)
            shiftModeEnabled = false;
        else if (key.code == SHIFT_MODE_ON
                 || key.code == SHIFT_MODE_OFF
                 || key.code == SHIFT_MODE_LOCK) {
            currentPanelShiftMode = key.code;
        } else
            doReturn = false;

        if (doReturn)
            return;

        QEvent::Type type = (key.is_key_press()) ? QEvent::KeyPress : QEvent::KeyRelease;
        Qt::KeyboardModifiers modifiers = Qt::NoModifier;
        if (key.is_shift_down())
            modifiers |= Qt::ShiftModifier;
        if (key.is_control_down())
            modifiers |= Qt::ControlModifier;
        if (key.is_alt_down())
            modifiers |= Qt::AltModifier;
        if (key.is_meta_down())
            modifiers |= Qt::MetaModifier;
        if (key.is_num_lock_down())
            modifiers |= Qt::KeypadModifier;

        if ( (key.code & 0xff000000) == 0x01000000) {
            //key code is directly encoded 24-bit UCS character
            QInputMethodEvent imevt;
            imevt.setCommitString(QString::fromUcs4(&key.code,1));
            QCoreApplication::sendEvent(TizenScim::focusObject,&imevt);
            adjustCapsMode();
            return;
        }
        int tmpKey;
        QScopedPointer<QKeyEvent> evt;
        //only if ascii is greater that 20 because that's where Qt:Key is equal ascii code
        if (key.get_ascii_code() >= 0x20) {
            evt.reset(new QKeyEvent(type,key.get_ascii_code(),modifiers,QChar(key.get_ascii_code())));
        } else if ((tmpKey = scimKeyCodeToQtKeyCode.value(key.code))){
            evt.reset(new QKeyEvent(type,tmpKey,modifiers));
        }

        if (!evt.isNull() && qApp->focusWindow()) {
            QCoreApplication::sendEvent(qApp->focusWindow(),evt.data());
            adjustCapsMode();
        } else {
            qCDebug(QT_TIZENSCIM_INPUT_METHOD) << "not recognized key code:" << key.code << QString::number(key.code,16);
        }

    }
    void adjustCapsMode()
    {
        if (shiftModeEnabled && currentPanelShiftMode == SHIFT_MODE_ON) {
            TizenScim::panelClient.prepare(TizenScim::contextId);
            TizenScim::panelClient.set_caps_mode(false);
            TizenScim::panelClient.send();
            currentPanelShiftMode = SHIFT_MODE_OFF;
        }
    }

    void panelSlotCommitString(int /*context*/, const WideString &wstr)
    {
        if (!focusObject)
            return;
        QInputMethodEvent imevt;
        imevt.setCommitString(QString::fromStdWString(wstr));
        QCoreApplication::sendEvent(focusObject,&imevt);
        adjustCapsMode();
    }
    void panelSlotForwardKeyEvent(int context, const KeyEvent &key)
    {
        panelSlotProcessKeyEvent(context,key);
    }

    void panelSlotResetKeyboardIse(int /*context*/) {
    }
    void panelSlotUpdateKeyboardIse(int context) {
        qCDebug(QT_TIZENSCIM_INPUT_METHOD) << "context:" << context;
    }

    void panelSlotUpdateIseInputContext(int /*context*/, int type, int value) {
        if (type == (uint32)ECORE_IMF_INPUT_PANEL_STATE_EVENT) {
            switch (value) {
                case SCIM_INPUT_PANEL_STATE_HIDE:
                    visible = false;
                    if (tizenInputContext)
                        tizenInputContext->emitInputPanelVisibleChanged();
                    break;
                case SCIM_INPUT_PANEL_STATE_SHOW:
                    visible = true;
                    if (tizenInputContext)
                        tizenInputContext->emitInputPanelVisibleChanged();
                    break;
                case SCIM_INPUT_PANEL_STATE_WILL_SHOW:
                    break;
                case SCIM_INPUT_PANEL_STATE_WILL_HIDE:
                    break;
                default:
                    break;
            }
        }
    }

    void panelSlotUpdateIsfCandidatePanel(int context, int type, int value) {
        qCDebug(QT_TIZENSCIM_INPUT_METHOD) << "context:" << context << type << value;
    }



    void panelClientSocketDataReady(int /*socket*/)
    {
        if (panelClient.has_pending_event()) {
            panelClient.filter_event();
        }
    }

    bool checkSocketFrontend()
    {
        SocketAddress address;
        SocketClient client;

        uint32 magic;
        address.set_address (scim_get_default_socket_frontend_address());
        if (!client.connect (address)) {
            qCDebug(QT_TIZENSCIM_INPUT_METHOD) << "client.connect failed";
            return false;
        }
        if (!scim_socket_open_connection (magic,
                                          String ("ConnectionTester"),
                                          String ("SocketFrontEnd"),
                                          client,
                                          1000)) {
            qCWarning(QT_TIZENSCIM_INPUT_METHOD) << "scim_socket_open_connection failed";
            return false;
        }
        return true;
    }

    void initializePanel()
    {
        panelClient.signal_connect_process_key_event(slot(panelSlotProcessKeyEvent));
        panelClient.signal_connect_commit_string(slot(panelSlotCommitString));
        panelClient.signal_connect_forward_key_event(slot(panelSlotForwardKeyEvent));

        panelClient.signal_connect_reset_keyboard_ise(slot(panelSlotResetKeyboardIse));
        panelClient.signal_connect_update_keyboard_ise(slot(panelSlotUpdateKeyboardIse));
        panelClient.signal_connect_update_ise_input_context(slot(panelSlotUpdateIseInputContext));
        panelClient.signal_connect_update_isf_candidate_panel(slot(panelSlotUpdateIsfCandidatePanel));

        String display_name;
        const char *p = getenv("DISPLAY");
        if (p)
            display_name = String(p);

        if (panelClient.open_connection(config->get_name(), display_name) >= 0) {
            int fd = panelClient.get_connection_number();
            socketNotifier = new QSocketNotifier(fd,QSocketNotifier::Read);

            QObject::connect(socketNotifier, &QSocketNotifier::activated, panelClientSocketDataReady);
            socketNotifier->setEnabled(true);
            if (panelClient.get_client_id(TizenScim::clientId)) {
                panelClient.prepare(0);
                panelClient.register_client(TizenScim::clientId);
                panelClient.send();
            }
        } else {
            qCWarning(QT_TIZENSCIM_INPUT_METHOD) << "panelClient.open_connection failed";
        }

        imControlClient.open_connection();

        TizenScim::iseUuid = SCIM_COMPOSE_KEY_FACTORY_UUID;
        if (TizenScim::contextId == 0) {
            TizenScim::contextId = qApp->applicationPid() % 50000;
        }
        panelClient.prepare(TizenScim::contextId);
        panelClient.register_input_context(TizenScim::contextId, TizenScim::iseUuid);
        panelClient.send();
    }
    void initializeScim()
    {
        if (!checkSocketFrontend()) {
            qCDebug(QT_TIZENSCIM_INPUT_METHOD) << "Launching a ISF daemon with Socket FrontEnd...\n";
            //get modules list
            const char *new_argv [] = { "--no-stay", 0 };
            int scim_launch_ret = scim_launch(true,
                                              configModuleName,
                                              "none",
                                              "socket",
                                              (char **)new_argv);
            qCDebug(QT_TIZENSCIM_INPUT_METHOD) << "scim_launch:" << scim_launch_ret;
        }

        qCDebug(QT_TIZENSCIM_INPUT_METHOD) << "Loading Config module: " << QString::fromStdString(configModuleName) << "...";
        configModule.reset(new ConfigModule(configModuleName));

        //create config instance
        if (configModule && configModule->valid ())
            config = configModule->create_config();

        if (config.null()) {
            qCWarning(QT_TIZENSCIM_INPUT_METHOD) << "Config module cannot be loaded, using dummy Config.";
            config = new DummyConfig ();
            configModuleName = "dummy";
        }
        reloadConfigCallback(config);
        config->signal_connect_reload(slot(reloadConfigCallback));
        TizenScim::scimKeyCodeToQtKeyCode.insert(SCIM_KEY_BackSpace, Qt::Key_Backspace);
        TizenScim::scimKeyCodeToQtKeyCode.insert(SCIM_KEY_Tab, Qt::Key_Tab);
        TizenScim::scimKeyCodeToQtKeyCode.insert(SCIM_KEY_Clear, Qt::Key_Clear);
        TizenScim::scimKeyCodeToQtKeyCode.insert(SCIM_KEY_Return, Qt::Key_Return);
        TizenScim::scimKeyCodeToQtKeyCode.insert(SCIM_KEY_Pause, Qt::Key_Pause);
        TizenScim::scimKeyCodeToQtKeyCode.insert(SCIM_KEY_Scroll_Lock, Qt::Key_ScrollLock);
        TizenScim::scimKeyCodeToQtKeyCode.insert(SCIM_KEY_Sys_Req, Qt::Key_SysReq);
        TizenScim::scimKeyCodeToQtKeyCode.insert(SCIM_KEY_Escape, Qt::Key_Escape);
        TizenScim::scimKeyCodeToQtKeyCode.insert(SCIM_KEY_Delete, Qt::Key_Delete);
    }

    void reloadConfigCallback(const ConfigPointer &/*config*/)
    {
    }
} // namespace TizenScim


QTizenScimInputContext::QTizenScimInputContext()
{
    TizenScim::tizenInputContext  = this;
    if (!qgetenv("QT_TIZENSCIM_SCIM_ENGINE_DEBUG").isEmpty()) {
        DebugOutput::set_verbose_level(SCIM_DEBUG_MAX_VERBOSE);
        DebugOutput::enable_debug_by_name("all");
    }

    if (!qgetenv("QT_TIZENSCIM_INPUT_METHOD_DEBUG").isEmpty())
            QLoggingCategory::setFilterRules(QStringLiteral("qt.tizenscim.inputmethod.debug = true"));
    TizenScim::initializeScim();
    TizenScim::initializePanel();
}

QTizenScimInputContext::~QTizenScimInputContext() {
    if (TizenScim::socketNotifier) {
        delete TizenScim::socketNotifier;
        TizenScim::socketNotifier = 0;
    }
    if (TizenScim::imControlSocketNotifier) {
        delete TizenScim::imControlSocketNotifier;
        TizenScim::imControlSocketNotifier = 0;
    }
}

bool QTizenScimInputContext::filterEvent(const QEvent *event)
{
    return QPlatformInputContext::filterEvent(event);
}

bool QTizenScimInputContext::isValid() const
{
    return TizenScim::imControlClient.is_connected();
}

void QTizenScimInputContext::setFocusObject(QObject *object)
{
    qCDebug(QT_TIZENSCIM_INPUT_METHOD) << object;
    if (TizenScim::focusObject == object)
        return;

    TizenScim::panelClient.prepare(TizenScim::contextId);
    if (inputMethodAccepted()) {
        TizenScim::panelClient.focus_in(TizenScim::contextId, TizenScim::iseUuid);

        QRectF itemRect = qGuiApp->inputMethod()->inputItemRectangle();
        QRect rect = qGuiApp->inputMethod()->inputItemTransform().mapRect(itemRect).toRect();
        QWindow *window = qGuiApp->focusWindow();
        if (window)
            rect = QRect(window->mapToGlobal(rect.topLeft()), rect.size());

        QInputMethodQueryEvent queryEvent(Qt::ImCursorPosition);
        if (object) {
            qGuiApp->sendEvent(object,&queryEvent);
            TizenScim::panelClient.update_cursor_position(TizenScim::contextId, queryEvent.value(Qt::ImCursorPosition).toInt());
            TizenScim::panelClient.update_spot_location(TizenScim::contextId, rect.left(), rect.top(), rect.bottom());
        }

    } else {
        TizenScim::panelClient.focus_out(TizenScim::contextId);
        if (TizenScim::visible)
            hideInputPanel();
    }
    TizenScim::panelClient.send();
    TizenScim::focusObject = object;
}

void QTizenScimInputContext::reset()
{
    qCDebug(QT_TIZENSCIM_INPUT_METHOD);
    if (qGuiApp->focusObject()) {
        TizenScim::panelClient.prepare(TizenScim::contextId);
        TizenScim::panelClient.reset_input_context(TizenScim::contextId);
        TizenScim::panelClient.send();
    } else {
        hideInputPanel();
    }
    QPlatformInputContext::reset();
}

void QTizenScimInputContext::update(Qt::InputMethodQueries q)
{
    qCDebug(QT_TIZENSCIM_INPUT_METHOD);
    QPlatformInputContext::update(q);
    if (q & (Qt::ImCursorPosition | Qt::ImCursorRectangle)) {
        QInputMethodQueryEvent queryEvent(q);
        if (TizenScim::focusObject) {
            qGuiApp->sendEvent(TizenScim::focusObject, &queryEvent);
        }
        TizenScim::panelClient.prepare(TizenScim::contextId);
        TizenScim::panelClient.update_cursor_position(TizenScim::contextId, queryEvent.value(Qt::ImCursorPosition).toInt());
        TizenScim::panelClient.send();
    }
}

void QTizenScimInputContext::commit()
{
    qCDebug(QT_TIZENSCIM_INPUT_METHOD);
    QPlatformInputContext::commit();
}

void QTizenScimInputContext::showInputPanel()
{
    qCDebug(QT_TIZENSCIM_INPUT_METHOD);
    if (TizenScim::visible)
        return;

    char imdata[100] = {0};
    const int len = 100;
    int tmp = -1;

    TizenScim::panelClient.prepare(TizenScim::contextId);
    //ficus in need to be invoked before show_ise - if not the virtual keyboard
    //key events are not generated when clicking keyboard buttons
    //don't have idea why

    TizenScim::panelClient.focus_in(TizenScim::contextId, TizenScim::iseUuid);
    /*I don't know what imdata is for but len need to be > 0 */
    TizenScim::panelClient.show_ise(TizenScim::clientId, TizenScim::contextId, &imdata, len, &tmp);
    TizenScim::panelClient.send();
}

void QTizenScimInputContext::hideInputPanel()
{
    qCDebug(QT_TIZENSCIM_INPUT_METHOD);
    if (!TizenScim::visible)
        return;
    TizenScim::panelClient.prepare(TizenScim::contextId);
    TizenScim::panelClient.hide_ise(TizenScim::clientId,TizenScim::contextId);
    TizenScim::panelClient.send();
}

bool QTizenScimInputContext::isInputPanelVisible() const
{
    return TizenScim::visible;
}

QRectF QTizenScimInputContext::keyboardRect() const
{
    return TizenScim::panelClientRectangle;
}

QT_END_NAMESPACE
