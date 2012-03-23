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

#ifndef VIRTUALKEYBOARD_H_
#define VIRTUALKEYBOARD_H_

#include <QtCore/QObject>
#include <QtCore/QLocale>
#include <QtGui/QPlatformScreen>
#include <QtGui/QWindowSystemInterface>

#include <stddef.h>
#include <vector>
#include <string>
#include <sys/pps.h>

class QSocketNotifier;

QT_BEGIN_NAMESPACE

/* Shamelessly copied from the browser - this should be rewritten once we have a proper PPS wrapper class */
class QQnxVirtualKeyboard : public QObject
{
    Q_OBJECT
public:
    // NOTE:  Not all the following keyboard modes are currently used.
    // Default - Regular Keyboard
    // Url/Email - Enhanced keys for each types.
    // Web - Regular keyboard with two blank keys, currently unused.
    // NumPunc - Numbers & Punctionation, alternate to Symbol
    // Symbol - All symbols, alternate to NumPunc, currently unused.
    // Phone - Phone enhanced keyboard - currently unused as no alternate keyboard available to access a-zA-Z
    // Pin - Keyboard for entering Pins (Hex values) currently unused.
    //
    // SPECIAL NOTE: Usage of NumPunc may have to be removed, ABC button is non-functional.
    //
    enum KeyboardMode { Default, Url, Email, Web, NumPunc, Symbol, Phone, Pin };

    static QQnxVirtualKeyboard& instance();
    static void destroy();

    bool showKeyboard();
    bool hideKeyboard();
    int  height() { return m_visible ? m_height : 0; }
    void setKeyboardMode(KeyboardMode);
    void notifyClientActiveStateChange(bool);
    bool isVisible() const { return m_visible; }
    QLocale locale() const { return m_locale; }

public Q_SLOTS:
    void start();

Q_SIGNALS:
    void localeChanged(const QLocale &locale);
    void visibilityChanged(bool visible);

private Q_SLOTS:
    void ppsDataReady();

private:
    QQnxVirtualKeyboard();
    virtual ~QQnxVirtualKeyboard();

    // Will be called internally if needed.
    bool connect();
    void close();
    bool queryPPSInfo();
    void handleKeyboardInfoMessage();
    void handleKeyboardStateChangeMessage(bool visible);
    void updateAvailableScreenGeometry();

    void applyKeyboardModeOptions();
    void addDefaultModeOptions();
    void addUrlModeOptions();
    void addEmailModeOptions();
    void addWebModeOptions();
    void addNumPuncModeOptions();
    void addSymbolModeOptions();
    void addPhoneModeOptions();
    void addPinModeOptions();

    pps_encoder_t  *m_encoder;
    pps_decoder_t  *m_decoder;
    char           *m_buffer;
    int             m_height;
    int             m_fd;
    KeyboardMode    m_keyboardMode;
    bool            m_visible;
    QLocale         m_locale;
    QSocketNotifier *m_readNotifier;

    // Path to keyboardManager in PPS.
    static const char *ms_PPSPath;
    static const size_t ms_bufferSize;
};

#endif /* VIRTUALKEYBOARD_H_ */
