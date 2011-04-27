/****************************************************************************
**
** Copyright (C) 2011 Nokia Corporation and/or its subsidiary(-ies).
** All rights reserved.
** Contact: Nokia Corporation (qt-info@nokia.com)
**
** This file is part of the QtGui module of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:LGPL$
** No Commercial Usage
** This file contains pre-release code and may not be distributed.
** You may use this file in accordance with the terms and conditions
** contained in the Technology Preview License Agreement accompanying
** this package.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 2.1 as published by the Free Software
** Foundation and appearing in the file LICENSE.LGPL included in the
** packaging of this file.  Please review the following information to
** ensure the GNU Lesser General Public License version 2.1 requirements
** will be met: http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, Nokia gives you certain additional
** rights.  These rights are described in the Nokia Qt LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
** If you have questions regarding the use of this file, please contact
** Nokia at qt-info@nokia.com.
**
**
**
**
**
**
**
**
** $QT_END_LICENSE$
**
****************************************************************************/

#include "qkbd_qws.h"
#include "qkbd_qws_p.h"

#ifndef QT_NO_QWS_KEYBOARD

#include <QFile>
#include <QDataStream>
#include <QStringList>

#ifdef Q_WS_QWS
#include "qwindowsystem_qws.h"
#include "qscreen_qws.h"
#endif

#ifdef Q_WS_QPA
#include <QWindowSystemInterface>
#include <QKeyEvent>
#endif

#include "qtimer.h"
#include <stdlib.h>

//#define QT_DEBUG_KEYMAP


QT_BEGIN_NAMESPACE

class QWSKbPrivate : public QObject
{
    Q_OBJECT
public:
    QWSKbPrivate(QWSKeyboardHandler *h, const QString &device)
        : m_handler(h), m_modifiers(0), m_composing(0), m_dead_unicode(0xffff),
          m_no_zap(false), m_do_compose(false),
          m_keymap(0), m_keymap_size(0), m_keycompose(0), m_keycompose_size(0)
    {
        m_ar_timer = new QTimer(this);
        m_ar_timer->setSingleShot(true);
        connect(m_ar_timer, SIGNAL(timeout()), SLOT(autoRepeat()));
        m_ar_delay = 400;
        m_ar_period = 80;

        memset(m_locks, 0, sizeof(m_locks));

        QString keymap;
        QStringList args = device.split(QLatin1Char(':'));
        foreach (const QString &arg, args) {
            if (arg.startsWith(QLatin1String("keymap=")))
                keymap = arg.mid(7);
            else if (arg == QLatin1String("disable-zap"))
                m_no_zap = true;
            else if (arg == QLatin1String("enable-compose"))
                m_do_compose = true;
            else if (arg.startsWith(QLatin1String("repeat-delay=")))
                m_ar_delay = arg.mid(13).toInt();
            else if (arg.startsWith(QLatin1String("repeat-rate=")))
                m_ar_period = arg.mid(12).toInt();
        }

        if (keymap.isEmpty() || !loadKeymap(keymap))
            unloadKeymap();
    }

    ~QWSKbPrivate()
    {
        unloadKeymap();
    }

    void beginAutoRepeat(int uni, int code, Qt::KeyboardModifiers mod)
    {
        m_ar_unicode = uni;
        m_ar_keycode = code;
        m_ar_modifier = mod;
        m_ar_timer->start(m_ar_delay);
    }

    void endAutoRepeat()
    {
        m_ar_timer->stop();
    }

    static Qt::KeyboardModifiers toQtModifiers(quint8 mod)
    {
        Qt::KeyboardModifiers qtmod = Qt::NoModifier;

        if (mod & (QWSKeyboard::ModShift | QWSKeyboard::ModShiftL | QWSKeyboard::ModShiftR))
            qtmod |= Qt::ShiftModifier;
        if (mod & (QWSKeyboard::ModControl | QWSKeyboard::ModCtrlL | QWSKeyboard::ModCtrlR))
            qtmod |= Qt::ControlModifier;
        if (mod & QWSKeyboard::ModAlt)
            qtmod |= Qt::AltModifier;

        return qtmod;
    }

    void unloadKeymap();
    bool loadKeymap(const QString &file);

private slots:
    void autoRepeat()
    {
        m_handler->processKeyEvent(m_ar_unicode, m_ar_keycode, m_ar_modifier, false, true);
        m_handler->processKeyEvent(m_ar_unicode, m_ar_keycode, m_ar_modifier, true, true);
        m_ar_timer->start(m_ar_period);
    }

private:
    QWSKeyboardHandler *m_handler;

    // auto repeat simulation
    int m_ar_unicode;
    int m_ar_keycode;
    Qt::KeyboardModifiers m_ar_modifier;
    int m_ar_delay;
    int m_ar_period;
    QTimer *m_ar_timer;

    // keymap handling
    quint8 m_modifiers;
    quint8 m_locks[3];
    int m_composing;
    quint16 m_dead_unicode;

    bool m_no_zap;
    bool m_do_compose;

    const QWSKeyboard::Mapping *m_keymap;
    int m_keymap_size;
    const QWSKeyboard::Composing *m_keycompose;
    int m_keycompose_size;

    static const QWSKeyboard::Mapping s_keymap_default[];
    static const QWSKeyboard::Composing s_keycompose_default[];

    friend class QWSKeyboardHandler;
};

// simple builtin US keymap
#include "qkbd_defaultmap_qws_p.h"

// the unloadKeymap() function needs to be AFTER the defaultmap include,
// since the sizeof(s_keymap_default) wouldn't work otherwise.

void QWSKbPrivate::unloadKeymap()
{
    if (m_keymap && m_keymap != s_keymap_default)
        delete [] m_keymap;
    if (m_keycompose && m_keycompose != s_keycompose_default)
        delete [] m_keycompose;

    m_keymap = s_keymap_default;
    m_keymap_size = sizeof(s_keymap_default) / sizeof(s_keymap_default[0]);
    m_keycompose = s_keycompose_default;
    m_keycompose_size = sizeof(s_keycompose_default) / sizeof(s_keycompose_default[0]);

    // reset state, so we could switch keymaps at runtime
    m_modifiers = 0;
    memset(m_locks, 0, sizeof(m_locks));
    m_composing = 0;
    m_dead_unicode = 0xffff;
}

bool QWSKbPrivate::loadKeymap(const QString &file)
{
    QFile f(file);

    if (!f.open(QIODevice::ReadOnly)) {
        qWarning("Could not open keymap file '%s'", qPrintable(file));
        return false;
    }

    // .qmap files have a very simple structure:
    // quint32 magic           (QWSKeyboard::FileMagic)
    // quint32 version         (1)
    // quint32 keymap_size     (# of struct QWSKeyboard::Mappings)
    // quint32 keycompose_size (# of struct QWSKeyboard::Composings)
    // all QWSKeyboard::Mappings via QDataStream::operator(<<|>>)
    // all QWSKeyboard::Composings via QDataStream::operator(<<|>>)

    quint32 qmap_magic, qmap_version, qmap_keymap_size, qmap_keycompose_size;

    QDataStream ds(&f);

    ds >> qmap_magic >> qmap_version >> qmap_keymap_size >> qmap_keycompose_size;

    if (ds.status() != QDataStream::Ok || qmap_magic != QWSKeyboard::FileMagic || qmap_version != 1 || qmap_keymap_size == 0) {
        qWarning("'%s' is ot a valid.qmap keymap file.", qPrintable(file));
        return false;
    }

    QWSKeyboard::Mapping *qmap_keymap = new QWSKeyboard::Mapping[qmap_keymap_size];
    QWSKeyboard::Composing *qmap_keycompose = qmap_keycompose_size ? new QWSKeyboard::Composing[qmap_keycompose_size] : 0;

    for (quint32 i = 0; i < qmap_keymap_size; ++i)
        ds >> qmap_keymap[i];
    for (quint32 i = 0; i < qmap_keycompose_size; ++i)
        ds >> qmap_keycompose[i];

    if (ds.status() != QDataStream::Ok) {
        delete [] qmap_keymap;
        delete [] qmap_keycompose;

        qWarning("Keymap file '%s' can not be loaded.", qPrintable(file));
        return false;
    }

    // unload currently active and clear state
    unloadKeymap();

    m_keymap = qmap_keymap;
    m_keymap_size = qmap_keymap_size;
    m_keycompose = qmap_keycompose;
    m_keycompose_size = qmap_keycompose_size;

    m_do_compose = true;

    return true;
}


/*!
    \class QWSKeyboardHandler
    \ingroup qws

    \brief The QWSKeyboardHandler class is a base class for keyboard
    drivers in Qt for Embedded Linux.

    Note that this class is only available in \l{Qt for Embedded Linux}.

    \l{Qt for Embedded Linux} provides ready-made drivers for several keyboard
    protocols, see the \l{Qt for Embedded Linux Character Input}{character
    input} documentation for details. Custom keyboard drivers can be
    implemented by subclassing the QWSKeyboardHandler class and
    creating a keyboard driver plugin (derived from
    QKbdDriverPlugin). The default implementation of the
    QKbdDriverFactory class will automatically detect the plugin, and
    load the driver into the server application at run-time using Qt's
    \l{How to Create Qt Plugins}{plugin system}.

    The keyboard driver receives keyboard events from the system
    device and encapsulates each event with an instance of the
    QWSEvent class which it then passes to the server application (the
    server is responsible for propagating the event to the appropriate
    client). To receive keyboard events, a QWSKeyboardHandler object
    will usually create a QSocketNotifier object for the given
    device. The QSocketNotifier class provides support for monitoring
    activity on a file descriptor. When the socket notifier receives
    data, it will call the keyboard driver's processKeyEvent()
    function to send the event to the \l{Qt for Embedded Linux} server
    application for relaying to clients.


    QWSKeyboardHandler also provides functions to control
    auto-repetion of key sequences, beginAutoRepeat() and
    endAutoRepeat(), and the transformDirKey() function enabling
    transformation of arrow keys according to the display orientation.

    \sa QKbdDriverPlugin, QKbdDriverFactory, {Qt for Embedded Linux Character Input}
*/


/*!
    Constructs a keyboard driver. The \a device argument is passed by the
    QWS_KEYBOARD environment variable.

    Call the QWSServer::setKeyboardHandler() function to make the
    newly created keyboard driver, the primary driver. Note that the
    primary driver is controlled by the system, i.e., the system will
    delete it upon exit.
*/
QWSKeyboardHandler::QWSKeyboardHandler(const QString &device)
{
    d = new QWSKbPrivate(this, device);
}

/*!
    \overload
*/
QWSKeyboardHandler::QWSKeyboardHandler()
{
    d = new QWSKbPrivate(this, QString());
}



/*!
    Destroys this keyboard driver.

    Do not call this function if this driver is the primary keyboard
    handler, i.e., if QWSServer::setKeyboardHandler() function has
    been called passing this driver as argument. The primary keyboard
    driver is deleted by the system.
*/
QWSKeyboardHandler::~QWSKeyboardHandler()
{
    delete d;
}


/*!
    Sends a key event to the \l{Qt for Embedded Linux} server application.

    The key event is identified by its \a unicode value and the \a
    keycode, \a modifiers, \a isPress and \a autoRepeat parameters.

    The \a keycode parameter is the Qt keycode value as defined by the
    Qt::Key enum. The \a modifiers is an OR combination of
    Qt::KeyboardModifier values, indicating whether \gui
    Shift/Alt/Ctrl keys are pressed. The \a isPress parameter is true
    if the event is a key press event and \a autoRepeat is true if the
    event is caused by an auto-repeat mechanism and not an actual key
    press.

    Note that this function does not handle key mapping. Please use
    processKeycode() if you need that functionality.

    \sa processKeycode(), beginAutoRepeat(), endAutoRepeat(), transformDirKey()
*/
void QWSKeyboardHandler::processKeyEvent(int unicode, int keycode, Qt::KeyboardModifiers modifiers,
                        bool isPress, bool autoRepeat)
{
#if defined(Q_WS_QWS)
    qwsServer->processKeyEvent(unicode, keycode, modifiers, isPress, autoRepeat);
#elif defined(Q_WS_QPA)
    QEvent::Type type = isPress ? QEvent::KeyPress : QEvent::KeyRelease;
    QString str;
    if (unicode != 0xffff)
        str = QString(unicode);
    QWindowSystemInterface::handleKeyEvent(0, type, keycode, modifiers, str);
#endif
}

/*!
    \fn int QWSKeyboardHandler::transformDirKey(int keycode)

    Transforms the arrow key specified by the given \a keycode, to the
    orientation of the display and returns the transformed keycode.

    The \a keycode is a Qt::Key value. The values identifying arrow
    keys are:

    \list
        \o Qt::Key_Left
        \o Qt::Key_Up
        \o Qt::Key_Right
        \o Qt::Key_Down
    \endlist

    \sa processKeyEvent()
 */
int QWSKeyboardHandler::transformDirKey(int key)
{
#ifdef Q_WS_QWS
    static int dir_keyrot = -1;
    if (dir_keyrot < 0) {
        // get the rotation
        switch (qgetenv("QWS_CURSOR_ROTATION").toInt()) {
        case 90: dir_keyrot = 1; break;
        case 180: dir_keyrot = 2; break;
        case 270: dir_keyrot = 3; break;
        default: dir_keyrot = 0; break;
        }
    }
    int xf = qt_screen->transformOrientation() + dir_keyrot;
    return (key-Qt::Key_Left+xf)%4+Qt::Key_Left;
#else
    return 0;
#endif
}

/*!
    \fn void QWSKeyboardHandler::beginAutoRepeat(int unicode, int keycode, Qt::KeyboardModifiers modifier)

    Begins auto-repeating the specified key press; after a short delay
    the key press is sent periodically until the endAutoRepeat()
    function is called.

    The key press is specified by its \a unicode, \a keycode and \a
    modifier state.

    \sa endAutoRepeat(), processKeyEvent()
*/
void QWSKeyboardHandler::beginAutoRepeat(int uni, int code, Qt::KeyboardModifiers mod)
{
    d->beginAutoRepeat(uni, code, mod);
}

/*!
    Stops auto-repeating a key press.

    \sa beginAutoRepeat(), processKeyEvent()
*/
void QWSKeyboardHandler::endAutoRepeat()
{
    d->endAutoRepeat();
}

/*!
    \enum QWSKeyboardHandler::KeycodeAction

    This enum describes the various special actions that actual
    QWSKeyboardHandler implementations have to take care of.

    \value None No further action required.

    \value CapsLockOn  Set the state of the Caps lock LED to on.
    \value CapsLockOff Set the state of the Caps lock LED to off.
    \value NumLockOn  Set the state of the Num lock LED to on.
    \value NumLockOff Set the state of the Num lock LED to off.
    \value ScrollLockOn  Set the state of the Scroll lock LED to on.
    \value ScrollLockOff Set the state of the Scroll lock LED to off.

    \value PreviousConsole Switch to the previous virtual console (by
                           default Ctrl+Alt+Left on Linux).
    \value NextConsole Switch to the next virtual console (by default
                       Ctrl+Alt+Right on Linux).
    \value SwitchConsoleFirst Switch to the first virtual console (0).
    \value SwitchConsoleLast Switch to the last virtual console (255).
    \value SwitchConsoleMask If the KeyAction value is between SwitchConsoleFirst
                             and SwitchConsoleLast, you can use this mask to get
                             the specific virtual console number to switch to.

    \value Reboot Reboot the machine - this is ignored in both the TTY and
                  LinuxInput handlers though (by default Ctrl+Alt+Del on Linux).

    \sa processKeycode()
*/

/*!
    \fn QWSKeyboardHandler::KeycodeAction QWSKeyboardHandler::processKeycode(quint16 keycode, bool isPress, bool autoRepeat)

	\since 4.6
	
    Maps \a keycode according to a keymap and sends that key event to the
    \l{Qt for Embedded Linux} server application.

    Please see the \l{Qt for Embedded Linux Character Input} and the \l
    {kmap2qmap} documentations for a description on how to create and use
    keymap files.

    The key event is identified by its \a keycode value and the \a isPress
    and \a autoRepeat parameters.

    The \a keycode parameter is \bold NOT the Qt keycode value as defined by
    the Qt::Key enum. This functions expects a standard Linux 16 bit kernel
    keycode as it is used in the Linux Input Event sub-system. This
    \a keycode is transformed to a Qt::Key code by using either a
    compiled-in US keyboard layout or by dynamically loading a keymap at
    startup which can be specified via the QWS_KEYBOARD environment
    variable.

    The \a isPress parameter is true if the event is a key press event and
    \a autoRepeat is true if the event is caused by an auto-repeat mechanism
    and not an actual key press.

    The return value indicates if the actual QWSKeyboardHandler
    implementation needs to take care of a special action, like console
    switching or LED handling.

    If standard Linux console keymaps are used, \a keycode must be one of the
    standardized values defined in \c /usr/include/linux/input.h

    \sa processKeyEvent(), KeycodeAction
*/

QWSKeyboardHandler::KeycodeAction QWSKeyboardHandler::processKeycode(quint16 keycode, bool pressed, bool autorepeat)
{
    KeycodeAction result = None;
    bool first_press = pressed && !autorepeat;

    const QWSKeyboard::Mapping *map_plain = 0;
    const QWSKeyboard::Mapping *map_withmod = 0;

    // get a specific and plain mapping for the keycode and the current modifiers
    for (int i = 0; i < d->m_keymap_size && !(map_plain && map_withmod); ++i) {
        const QWSKeyboard::Mapping *m = d->m_keymap + i;
        if (m->keycode == keycode) {
            if (m->modifiers == 0)
                map_plain = m;

            quint8 testmods = d->m_modifiers;
            if (d->m_locks[0] /*CapsLock*/ && (m->flags & QWSKeyboard::IsLetter))
                testmods ^= QWSKeyboard::ModShift;
            if (m->modifiers == testmods)
                map_withmod = m;
        }
    }

#ifdef QT_DEBUG_KEYMAP
    qWarning("Processing key event: keycode=%3d, modifiers=%02x pressed=%d, autorepeat=%d  |  plain=%d, withmod=%d, size=%d", \
             keycode, d->m_modifiers, pressed ? 1 : 0, autorepeat ? 1 : 0, \
             map_plain ? map_plain - d->m_keymap : -1, \
             map_withmod ? map_withmod - d->m_keymap : -1, \
             d->m_keymap_size);
#endif

    const QWSKeyboard::Mapping *it = map_withmod ? map_withmod : map_plain;

    if (!it) {
#ifdef QT_DEBUG_KEYMAP
        // we couldn't even find a plain mapping
        qWarning("Could not find a suitable mapping for keycode: %3d, modifiers: %02x", keycode, d->m_modifiers);
#endif
        return result;
    }

    bool skip = false;
    quint16 unicode = it->unicode;
    quint32 qtcode = it->qtcode;

    if ((it->flags & QWSKeyboard::IsModifier) && it->special) {
        // this is a modifier, i.e. Shift, Alt, ...
        if (pressed)
            d->m_modifiers |= quint8(it->special);
        else
            d->m_modifiers &= ~quint8(it->special);
    } else if (qtcode >= Qt::Key_CapsLock && qtcode <= Qt::Key_ScrollLock) {
        // (Caps|Num|Scroll)Lock
        if (first_press) {
            quint8 &lock = d->m_locks[qtcode - Qt::Key_CapsLock];
            lock ^= 1;

            switch (qtcode) {
            case Qt::Key_CapsLock  : result = lock ? CapsLockOn : CapsLockOff; break;
            case Qt::Key_NumLock   : result = lock ? NumLockOn : NumLockOff; break;
            case Qt::Key_ScrollLock: result = lock ? ScrollLockOn : ScrollLockOff; break;
            default                : break;
            }
        }
    } else if ((it->flags & QWSKeyboard::IsSystem) && it->special && first_press) {
        switch (it->special) {
        case QWSKeyboard::SystemReboot:
            result = Reboot;
            break;

        case QWSKeyboard::SystemZap:
            if (!d->m_no_zap)
                qApp->quit();
            break;

        case QWSKeyboard::SystemConsolePrevious:
            result = PreviousConsole;
            break;

        case QWSKeyboard::SystemConsoleNext:
            result = NextConsole;
            break;

        default:
            if (it->special >= QWSKeyboard::SystemConsoleFirst &&
                it->special <= QWSKeyboard::SystemConsoleLast) {
                result = KeycodeAction(SwitchConsoleFirst + ((it->special & QWSKeyboard::SystemConsoleMask) & SwitchConsoleMask));
            }
            break;
        }

        skip = true; // no need to tell QWS about it
    } else if ((qtcode == Qt::Key_Multi_key) && d->m_do_compose) {
        // the Compose key was pressed
        if (first_press)
            d->m_composing = 2;
        skip = true;
    } else if ((it->flags & QWSKeyboard::IsDead) && d->m_do_compose) {
        // a Dead key was pressed
        if (first_press && d->m_composing == 1 && d->m_dead_unicode == unicode) { // twice
            d->m_composing = 0;
            qtcode = Qt::Key_unknown; // otherwise it would be Qt::Key_Dead...
        } else if (first_press && unicode != 0xffff) {
            d->m_dead_unicode = unicode;
            d->m_composing = 1;
            skip = true;
        } else {
            skip = true;
        }
    }

    if (!skip) {
        // a normal key was pressed
        const int modmask = Qt::ShiftModifier | Qt::ControlModifier | Qt::AltModifier | Qt::MetaModifier | Qt::KeypadModifier;

        // we couldn't find a specific mapping for the current modifiers,
        // or that mapping didn't have special modifiers:
        // so just report the plain mapping with additional modifiers.
        if ((it == map_plain && it != map_withmod) ||
            (map_withmod && !(map_withmod->qtcode & modmask))) {
            qtcode |= QWSKbPrivate::toQtModifiers(d->m_modifiers);
        }

        if (d->m_composing == 2 && first_press && !(it->flags & QWSKeyboard::IsModifier)) {
            // the last key press was the Compose key
            if (unicode != 0xffff) {
                int idx = 0;
                // check if this code is in the compose table at all
                for ( ; idx < d->m_keycompose_size; ++idx) {
                    if (d->m_keycompose[idx].first == unicode)
                        break;
                }
                if (idx < d->m_keycompose_size) {
                    // found it -> simulate a Dead key press
                    d->m_dead_unicode = unicode;
                    unicode = 0xffff;
                    d->m_composing = 1;
                    skip = true;
                } else {
                    d->m_composing = 0;
                }
            } else {
                d->m_composing = 0;
            }
        } else if (d->m_composing == 1 && first_press && !(it->flags & QWSKeyboard::IsModifier)) {
            // the last key press was a Dead key
            bool valid = false;
            if (unicode != 0xffff) {
                int idx = 0;
                // check if this code is in the compose table at all
                for ( ; idx < d->m_keycompose_size; ++idx) {
                    if (d->m_keycompose[idx].first == d->m_dead_unicode && d->m_keycompose[idx].second == unicode)
                        break;
                }
                if (idx < d->m_keycompose_size) {
                    quint16 composed = d->m_keycompose[idx].result;
                    if (composed != 0xffff) {
                        unicode = composed;
                        qtcode = Qt::Key_unknown;
                        valid = true;
                    }
                }
            }
            if (!valid) {
                unicode = d->m_dead_unicode;
                qtcode = Qt::Key_unknown;
            }
            d->m_composing = 0;
        }

        if (!skip) {
#ifdef QT_DEBUG_KEYMAP
            qWarning("Processing: uni=%04x, qt=%08x, qtmod=%08x", unicode, qtcode & ~modmask, (qtcode & modmask));
#endif

            // send the result to the QWS server
            processKeyEvent(unicode, qtcode & ~modmask, Qt::KeyboardModifiers(qtcode & modmask), pressed, autorepeat);
        }
    }
    return result;
}

QT_END_NAMESPACE

#include "qkbd_qws.moc"

#endif // QT_NO_QWS_KEYBOARD
