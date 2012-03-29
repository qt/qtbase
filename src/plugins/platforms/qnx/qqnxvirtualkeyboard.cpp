/***************************************************************************
**
** Copyright (C) 2011 - 2012 Research In Motion
** Contact: http://www.qt-project.org/
**
** This file is part of the plugins of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:LGPL$
** GNU Lesser General Public License Usage
** This file may be used under the terms of the GNU Lesser General Public
** License version 2.1 as published by the Free Software Foundation and
** appearing in the file LICENSE.LGPL included in the packaging of this
** file. Please review the following information to ensure the GNU Lesser
** General Public License version 2.1 requirements will be met:
** http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, Nokia gives you certain additional
** rights. These rights are described in the Nokia Qt LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
** GNU General Public License Usage
** Alternatively, this file may be used under the terms of the GNU General
** Public License version 3.0 as published by the Free Software Foundation
** and appearing in the file LICENSE.GPL included in the packaging of this
** file. Please review the following information to ensure the GNU General
** Public License version 3.0 requirements will be met:
** http://www.gnu.org/copyleft/gpl.html.
**
** Other Usage
** Alternatively, this file may be used in accordance with the terms and
** conditions contained in a signed written agreement between you and Nokia.
**
**
**
**
**
**
** $QT_END_LICENSE$
**
****************************************************************************/

#include "qqnxvirtualkeyboard.h"
#include "qqnxscreen.h"

#include <QtCore/QDebug>
#include <QtCore/QSocketNotifier>
#include <QtCore/private/qcore_unix_p.h>

#include <errno.h>
#include <fcntl.h>
#include <stdlib.h>
#include <string.h>
#include <sys/iomsg.h>
#include <sys/pps.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>

QT_BEGIN_NAMESPACE

const char  *QQnxVirtualKeyboard::ms_PPSPath = "/pps/services/input/control?wait";
const size_t QQnxVirtualKeyboard::ms_bufferSize = 2048;

// Huge hack for keyboard shadow (see QNX PR 88400). Should be removed ASAP.
#define KEYBOARD_SHADOW_HEIGHT 8

QQnxVirtualKeyboard::QQnxVirtualKeyboard()
    : m_encoder(0),
      m_decoder(0),
      m_buffer(0),
      m_height(0),
      m_fd(-1),
      m_keyboardMode(Default),
      m_visible(false),
      m_locale(QLatin1String("en_US")),
      m_readNotifier(0)
{
}

QQnxVirtualKeyboard::~QQnxVirtualKeyboard()
{
    close();
}

void QQnxVirtualKeyboard::start()
{
#ifdef QQNXVIRTUALKEYBOARD_DEBUG
    qDebug() << "QQNX: starting keyboard event processing";
#endif
    if (!connect())
        return;
}

void QQnxVirtualKeyboard::close()
{
    delete m_readNotifier;
    m_readNotifier = 0;

    if (m_fd != -1) {
        ::close(m_fd);
        m_fd = -1;
    }

    if (m_decoder) {
        pps_decoder_cleanup(m_decoder);
        delete m_decoder;
        m_decoder = 0;
    }

    if (m_encoder) {
        pps_encoder_cleanup(m_encoder);
        delete m_encoder;
        m_encoder = 0;
    }

    delete [] m_buffer;
    m_buffer = 0;
}

bool QQnxVirtualKeyboard::connect()
{
    close();

    m_encoder = new pps_encoder_t;
    m_decoder = new pps_decoder_t;

    pps_encoder_initialize(m_encoder, false);
    pps_decoder_initialize(m_decoder, NULL);

    errno = 0;
    m_fd = ::open(ms_PPSPath, O_RDWR);
    if (m_fd == -1)
    {
        qCritical("QQnxVirtualKeyboard: Unable to open \"%s\" for keyboard: %s (%d).",
                ms_PPSPath, strerror(errno), errno);
        close();
        return false;
    }

    m_buffer = new char[ms_bufferSize];
    if (!m_buffer) {
        qCritical("QQnxVirtualKeyboard: Unable to allocate buffer of %d bytes. Size is unavailable.",  ms_bufferSize);
        return false;
    }

    if (!queryPPSInfo())
        return false;

    m_readNotifier = new QSocketNotifier(m_fd, QSocketNotifier::Read);
    QObject::connect(m_readNotifier, SIGNAL(activated(int)), this, SLOT(ppsDataReady()));

    return true;
}

bool QQnxVirtualKeyboard::queryPPSInfo()
{
    // Request info, requires id to regenerate res message.
    pps_encoder_add_string(m_encoder, "msg", "info");
    pps_encoder_add_string(m_encoder, "id", "libWebView");

    if (::write(m_fd, pps_encoder_buffer(m_encoder), pps_encoder_length(m_encoder)) == -1) {
        close();
        return false;
    }

    pps_encoder_reset(m_encoder);

    return true;
}

void QQnxVirtualKeyboard::notifyClientActiveStateChange(bool active)
{
    if (!active)
        hideKeyboard();
}

void QQnxVirtualKeyboard::ppsDataReady()
{
    ssize_t nread = qt_safe_read(m_fd, m_buffer, ms_bufferSize - 1);

#ifdef QQNXVIRTUALKEYBOARD_DEBUG
    qDebug() << "QQNX: keyboardMessage size: " << nread;
#endif
    if (nread < 0){
        connect(); // reconnect
        return;
    }

    // nread is the real space necessary, not the amount read.
    if (static_cast<size_t>(nread) > ms_bufferSize - 1) {
        qCritical("QQnxVirtualKeyboard: Keyboard buffer size too short; need %u.", nread + 1);
        connect(); // reconnect
        return;
    }

    m_buffer[nread] = 0;
    pps_decoder_parse_pps_str(m_decoder, m_buffer);
    pps_decoder_push(m_decoder, NULL);
#ifdef QQNXVIRTUALKEYBOARD_DEBUG
    pps_decoder_dump_tree(m_decoder, stderr);
#endif

    const char *value;
    if (pps_decoder_get_string(m_decoder, "error", &value) == PPS_DECODER_OK) {
        qCritical("QQnxVirtualKeyboard: Keyboard PPS decoder error: %s", value ? value : "[null]");
        return;
    }

    if (pps_decoder_get_string(m_decoder, "msg", &value) == PPS_DECODER_OK) {
        if (strcmp(value, "show") == 0) {
            const bool oldVisible = m_visible;
            m_visible = true;
            handleKeyboardStateChangeMessage(true);
            if (oldVisible != m_visible)
                emit visibilityChanged(m_visible);
        } else if (strcmp(value, "hide") == 0) {
            const bool oldVisible = m_visible;
            m_visible = false;
            handleKeyboardStateChangeMessage(false);
            if (oldVisible != m_visible)
                emit visibilityChanged(m_visible);
        } else if (strcmp(value, "info") == 0)
            handleKeyboardInfoMessage();
        else if (strcmp(value, "connect") == 0) { }
        else
            qCritical("QQnxVirtualKeyboard: Unexpected keyboard PPS msg value: %s", value ? value : "[null]");
    } else if (pps_decoder_get_string(m_decoder, "res", &value) == PPS_DECODER_OK) {
        if (strcmp(value, "info") == 0)
            handleKeyboardInfoMessage();
        else
            qCritical("QQnxVirtualKeyboard: Unexpected keyboard PPS res value: %s", value ? value : "[null]");
    } else
        qCritical("QQnxVirtualKeyboard: Unexpected keyboard PPS message type");
}

void QQnxVirtualKeyboard::handleKeyboardInfoMessage()
{
    int newHeight = 0;
    const char *value;

    if (pps_decoder_push(m_decoder, "dat") != PPS_DECODER_OK) {
        qCritical("QQnxVirtualKeyboard: Keyboard PPS dat object not found");
        return;
    }
    if (pps_decoder_get_int(m_decoder, "size", &newHeight) != PPS_DECODER_OK) {
        qCritical("QQnxVirtualKeyboard: Keyboard PPS size field not found");
        return;
    }
    if (pps_decoder_push(m_decoder, "locale") != PPS_DECODER_OK) {
        qCritical("QQnxVirtualKeyboard: Keyboard PPS locale object not found");
        return;
    }
    if (pps_decoder_get_string(m_decoder, "languageId", &value) != PPS_DECODER_OK) {
        qCritical("QQnxVirtualKeyboard: Keyboard PPS languageId field not found");
        return;
    }
    const QString languageId = QString::fromLatin1(value);
    if (pps_decoder_get_string(m_decoder, "countryId", &value) != PPS_DECODER_OK) {
        qCritical("QQnxVirtualKeyboard: Keyboard PPS size countryId not found");
        return;
    }
    const QString countryId = QString::fromLatin1(value);

    // HUGE hack, should be removed ASAP.
    newHeight -= KEYBOARD_SHADOW_HEIGHT; // We want to ignore the 8 pixel shadow above the keyboard. (PR 88400)

    if (newHeight != m_height) {
        m_height = newHeight;
        if (m_visible)
            emit heightChanged(m_height);
    }

    const QLocale locale = QLocale(languageId + QLatin1Char('_') + countryId);
    if (locale != m_locale) {
        m_locale = locale;
        emit localeChanged(locale);
    }

#ifdef QQNXVIRTUALKEYBOARD_DEBUG
    qDebug() << "QQNX: handleKeyboardInfoMessage size=" << m_height << "locale=" << m_locale;
#endif
}

void QQnxVirtualKeyboard::handleKeyboardStateChangeMessage(bool visible)
{

#ifdef QQNXVIRTUALKEYBOARD_DEBUG
    qDebug() << "QQNX: handleKeyboardStateChangeMessage " << visible;
#endif
    if (visible != m_visible)
        emit heightChanged(height());

    if (visible)
        showKeyboard();
    else
        hideKeyboard();
}

bool QQnxVirtualKeyboard::showKeyboard()
{
#ifdef QQNXVIRTUALKEYBOARD_DEBUG
    qDebug() << "QQNX: showKeyboard()";
#endif

    // Try to connect.
    if (m_fd == -1 && !connect())
        return false;

    // NOTE:  This must be done everytime the keyboard is shown even if there is no change because
    // hiding the keyboard wipes the setting.
    applyKeyboardModeOptions();

    if (m_visible)
        return true;

    pps_encoder_reset(m_encoder);

    // Send the show message.
    pps_encoder_add_string(m_encoder, "msg", "show");

    if (::write(m_fd, pps_encoder_buffer(m_encoder), pps_encoder_length(m_encoder)) == -1) {
        close();
        return false;
    }

    pps_encoder_reset(m_encoder);

    // Return true if no error occurs.  Sizing response will be triggered when confirmation of
    // the change arrives.
    return true;
}

bool QQnxVirtualKeyboard::hideKeyboard()
{
#ifdef QQNXVIRTUALKEYBOARD_DEBUG
    qDebug() << "QQNX: hideKeyboard()";
#endif

    if (m_fd == -1 && !connect())
        return false;

    pps_encoder_add_string(m_encoder, "msg", "hide");

    if (::write(m_fd, pps_encoder_buffer(m_encoder), pps_encoder_length(m_encoder)) == -1) {
        close();

        //Try again.
        if (connect()) {
            if (::write(m_fd, pps_encoder_buffer(m_encoder), pps_encoder_length(m_encoder)) == -1) {
                close();
                return false;
            }
        }
        else
            return false;
    }

    pps_encoder_reset(m_encoder);

    // Return true if no error occurs.  Sizing response will be triggered when confirmation of
    // the change arrives.
    return true;
}

void QQnxVirtualKeyboard::setKeyboardMode(KeyboardMode mode)
{
    if (m_keyboardMode == mode)
        return;

    m_keyboardMode = mode;
    if (m_visible)
        applyKeyboardModeOptions();
}

void QQnxVirtualKeyboard::applyKeyboardModeOptions()
{
    // Try to connect.
    if (m_fd == -1 && !connect())
        return;

    // Send the options message.
    pps_encoder_add_string(m_encoder, "msg", "options");

    pps_encoder_start_object(m_encoder, "dat");
    switch (m_keyboardMode) {
    case Url:
        addUrlModeOptions();
        break;
    case Email:
        addEmailModeOptions();
        break;
    case Web:
        addWebModeOptions();
        break;
    case NumPunc:
        addNumPuncModeOptions();
        break;
    case Symbol:
        addSymbolModeOptions();
        break;
    case Phone:
        addPhoneModeOptions();
        break;
    case Pin:
        addPinModeOptions();
        break;
    case Default:
    default:
        addDefaultModeOptions();
        break;
    }

    pps_encoder_end_object(m_encoder);

    if (::write(m_fd, pps_encoder_buffer(m_encoder), pps_encoder_length(m_encoder)) == -1) {
        close();
    }

    pps_encoder_reset(m_encoder);
}

void QQnxVirtualKeyboard::addDefaultModeOptions()
{
    pps_encoder_add_string(m_encoder, "enter", "enter.default");
    pps_encoder_add_string(m_encoder, "type", "default");
}

void QQnxVirtualKeyboard::addUrlModeOptions()
{
    pps_encoder_add_string(m_encoder, "enter", "enter.default");
    pps_encoder_add_string(m_encoder, "type", "url");
}

void QQnxVirtualKeyboard::addEmailModeOptions()
{
    pps_encoder_add_string(m_encoder, "enter", "enter.default");
    pps_encoder_add_string(m_encoder, "type", "email");
}

void QQnxVirtualKeyboard::addWebModeOptions()
{
    pps_encoder_add_string(m_encoder, "enter", "enter.default");
    pps_encoder_add_string(m_encoder, "type", "web");
}

void QQnxVirtualKeyboard::addNumPuncModeOptions()
{
    pps_encoder_add_string(m_encoder, "enter", "enter.default");
    pps_encoder_add_string(m_encoder, "type", "numPunc");
}

void QQnxVirtualKeyboard::addPhoneModeOptions()
{
    pps_encoder_add_string(m_encoder, "enter", "enter.default");
    pps_encoder_add_string(m_encoder, "type", "phone");
}

void QQnxVirtualKeyboard::addPinModeOptions()
{
    pps_encoder_add_string(m_encoder, "enter", "enter.default");
    pps_encoder_add_string(m_encoder, "type", "pin");
}

void QQnxVirtualKeyboard::addSymbolModeOptions()
{
    pps_encoder_add_string(m_encoder, "enter", "enter.default");
    pps_encoder_add_string(m_encoder, "type", "symbol");
}

QT_END_NAMESPACE
