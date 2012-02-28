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

#include "qbbvirtualkeyboard.h"
#include "qbbscreen.h"

#include <QtGui/QPlatformScreen>
#include <QtGui/QPlatformWindow>

#include <QtCore/QDebug>

#include <errno.h>
#include <fcntl.h>
#include <stdlib.h>
#include <string.h>
#include <sys/iomsg.h>
#include <sys/pps.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>

const char  *QBBVirtualKeyboard::ms_PPSPath = "/pps/services/input/control?wait";
const size_t QBBVirtualKeyboard::ms_bufferSize = 2048;

static QBBVirtualKeyboard *s_instance = 0;

// Huge hack for keyboard shadow (see QNX PR 88400). Should be removed ASAP.
#define KEYBOARD_SHADOW_HEIGHT 8

QBBVirtualKeyboard::QBBVirtualKeyboard() :
        m_encoder(NULL),
        m_decoder(NULL),
        m_buffer(NULL),
        m_height(0),
        m_fd(-1),
        m_keyboardMode(Default),
        m_visible(false),
        m_locale(QLatin1String("en_US"))
{
    connect();
}

QBBVirtualKeyboard::~QBBVirtualKeyboard()
{
    close();
}

/* static */
QBBVirtualKeyboard& QBBVirtualKeyboard::instance()
{
    if (!s_instance) {
        s_instance = new QBBVirtualKeyboard();
        s_instance->start();
    }

    return *s_instance;
}

/* static */
void QBBVirtualKeyboard::destroy()
{
    if (s_instance) {
        delete s_instance;
        s_instance = 0;
    }
}

void QBBVirtualKeyboard::close()
{
    if (m_fd) {
        // any reads will fail after we close the fd, which is basically what we want.
        ::close(m_fd);
    }

    // Wait for the thread to die (should be immediate), then continue the cleanup.
    wait();

    if (m_fd) {
        m_fd = -1;
    }


    if (m_decoder)
    {
        pps_decoder_cleanup(m_decoder);
        delete m_decoder;
        m_decoder = NULL;
    }

    if (m_encoder)
    {
        pps_encoder_cleanup(m_encoder);
        delete m_encoder;
        m_encoder = NULL;
    }

    delete [] m_buffer;
    m_buffer = NULL;
}

bool QBBVirtualKeyboard::connect()
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
        qCritical("QBBVirtualKeyboard: Unable to open \"%s\" for keyboard: %s (%d).",
                ms_PPSPath, strerror(errno), errno);
        close();
        return false;
    }

    m_buffer = new char[ms_bufferSize];
    if (!m_buffer) {
        qCritical("QBBVirtualKeyboard: Unable to allocate buffer of %d bytes. Size is unavailable.",  ms_bufferSize);
        return false;
    }

    if (!queryPPSInfo())
        return false;

    start();

    return true;
}

bool QBBVirtualKeyboard::queryPPSInfo()
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

void QBBVirtualKeyboard::notifyClientActiveStateChange(bool active)
{
    if (!active)
        hideKeyboard();
}

void QBBVirtualKeyboard::run()
{
    ppsDataReady();
}

void QBBVirtualKeyboard::ppsDataReady()
{
    while (1) {
        ssize_t nread = read(m_fd, m_buffer, ms_bufferSize - 1);

#ifdef QBBVIRTUALKEYBOARD_DEBUG
        qDebug() << "QBB: keyboardMessage size: " << nread;
#endif
        if (nread < 0)
            break;

        // nread is the real space necessary, not the amount read.
        if (static_cast<size_t>(nread) > ms_bufferSize - 1) {
            qCritical("QBBVirtualKeyboard: Keyboard buffer size too short; need %u.", nread + 1);
            break;
        }

        m_buffer[nread] = 0;
        pps_decoder_parse_pps_str(m_decoder, m_buffer);
        pps_decoder_push(m_decoder, NULL);
#ifdef QBBVIRTUALKEYBOARD_DEBUG
        pps_decoder_dump_tree(m_decoder, stderr);
#endif

        const char *value;
        if (pps_decoder_get_string(m_decoder, "error", &value) == PPS_DECODER_OK) {
            qCritical("QBBVirtualKeyboard: Keyboard PPS decoder error: %s", value ? value : "[null]");
            continue;
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
                qCritical("QBBVirtualKeyboard: Unexpected keyboard PPS msg value: %s", value ? value : "[null]");
        } else if (pps_decoder_get_string(m_decoder, "res", &value) == PPS_DECODER_OK) {
            if (strcmp(value, "info") == 0)
                handleKeyboardInfoMessage();
            else
                qCritical("QBBVirtualKeyboard: Unexpected keyboard PPS res value: %s", value ? value : "[null]");
        } else
            qCritical("QBBVirtualKeyboard: Unexpected keyboard PPS message type");
    }

#ifdef QBBVIRTUALKEYBOARD_DEBUG
    qDebug() << "QBB: exiting keyboard thread";
#endif

    if (m_decoder)
        pps_decoder_cleanup(m_decoder);
}

void QBBVirtualKeyboard::handleKeyboardInfoMessage()
{
    int newHeight = 0;
    const char *value;

    if (pps_decoder_push(m_decoder, "dat") != PPS_DECODER_OK) {
        qCritical("QBBVirtualKeyboard: Keyboard PPS dat object not found");
        return;
    }
    if (pps_decoder_get_int(m_decoder, "size", &newHeight) != PPS_DECODER_OK) {
        qCritical("QBBVirtualKeyboard: Keyboard PPS size field not found");
        return;
    }
    if (pps_decoder_push(m_decoder, "locale") != PPS_DECODER_OK) {
        qCritical("QBBVirtualKeyboard: Keyboard PPS locale object not found");
        return;
    }
    if (pps_decoder_get_string(m_decoder, "languageId", &value) != PPS_DECODER_OK) {
        qCritical("QBBVirtualKeyboard: Keyboard PPS languageId field not found");
        return;
    }
    const QString languageId = QString::fromLatin1(value);
    if (pps_decoder_get_string(m_decoder, "countryId", &value) != PPS_DECODER_OK) {
        qCritical("QBBVirtualKeyboard: Keyboard PPS size countryId not found");
        return;
    }
    const QString countryId = QString::fromLatin1(value);

    // HUGE hack, should be removed ASAP.
    newHeight -= KEYBOARD_SHADOW_HEIGHT; // We want to ignore the 8 pixel shadow above the keyboard. (PR 88400)

    if (newHeight != m_height) {
        m_height = newHeight;
        updateAvailableScreenGeometry();
    }

    const QLocale locale = QLocale(languageId + QLatin1Char('_') + countryId);
    if (locale != m_locale) {
        m_locale = locale;
        emit localeChanged(locale);
    }

#ifdef QBBVIRTUALKEYBOARD_DEBUG
    qDebug() << "QBB: handleKeyboardInfoMessage size=" << m_height << "locale=" << m_locale;
#endif
}

void QBBVirtualKeyboard::handleKeyboardStateChangeMessage(bool visible)
{

#ifdef QBBVIRTUALKEYBOARD_DEBUG
    qDebug() << "QBB: handleKeyboardStateChangeMessage " << visible;
#endif
    updateAvailableScreenGeometry();

    if (visible)
        showKeyboard();
    else
        hideKeyboard();
}

void QBBVirtualKeyboard::updateAvailableScreenGeometry()
{
#ifdef QBBVIRTUALKEYBOARD_DEBUG
    qDebug() << "QBB: updateAvailableScreenGeometry: keyboard visible=" << m_visible << ", keyboard height=" << m_height;
#endif

    // TODO: What screen index should be used? I assume primaryScreen here because it works, and
    //       we do it for handleScreenGeometryChange elsewhere but since we have support
    //       for more than one screen, that's not going to always work.
    QBBScreen *platformScreen = QBBScreen::primaryDisplay();
    QWindowSystemInterface::handleScreenAvailableGeometryChange(platformScreen->screen(), platformScreen->availableGeometry());
}

bool QBBVirtualKeyboard::showKeyboard()
{
#ifdef QBBVIRTUALKEYBOARD_DEBUG
    qDebug() << "QBB: showKeyboard()";
#endif

    // Try to connect.
    if (m_fd == -1 && !connect())
        return false;

    // NOTE:  This must be done everytime the keyboard is shown even if there is no change because
    // hiding the keyboard wipes the setting.
    applyKeyboardModeOptions();

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

bool QBBVirtualKeyboard::hideKeyboard()
{
#ifdef QBBVIRTUALKEYBOARD_DEBUG
    qDebug() << "QBB: hideKeyboard()";
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

void QBBVirtualKeyboard::setKeyboardMode(KeyboardMode mode)
{
    m_keyboardMode = mode;
}

void QBBVirtualKeyboard::applyKeyboardModeOptions()
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

void QBBVirtualKeyboard::addDefaultModeOptions()
{
    pps_encoder_add_string(m_encoder, "enter", "enter.default");
    pps_encoder_add_string(m_encoder, "type", "default");
}

void QBBVirtualKeyboard::addUrlModeOptions()
{
    pps_encoder_add_string(m_encoder, "enter", "enter.default");
    pps_encoder_add_string(m_encoder, "type", "url");
}

void QBBVirtualKeyboard::addEmailModeOptions()
{
    pps_encoder_add_string(m_encoder, "enter", "enter.default");
    pps_encoder_add_string(m_encoder, "type", "email");
}

void QBBVirtualKeyboard::addWebModeOptions()
{
    pps_encoder_add_string(m_encoder, "enter", "enter.default");
    pps_encoder_add_string(m_encoder, "type", "web");
}

void QBBVirtualKeyboard::addNumPuncModeOptions()
{
    pps_encoder_add_string(m_encoder, "enter", "enter.default");
    pps_encoder_add_string(m_encoder, "type", "numPunc");
}

void QBBVirtualKeyboard::addPhoneModeOptions()
{
    pps_encoder_add_string(m_encoder, "enter", "enter.default");
    pps_encoder_add_string(m_encoder, "type", "phone");
}

void QBBVirtualKeyboard::addPinModeOptions()
{
    pps_encoder_add_string(m_encoder, "enter", "enter.default");
    pps_encoder_add_string(m_encoder, "type", "pin");
}

void QBBVirtualKeyboard::addSymbolModeOptions()
{
    pps_encoder_add_string(m_encoder, "enter", "enter.default");
    pps_encoder_add_string(m_encoder, "type", "symbol");
}
