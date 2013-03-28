/***************************************************************************
**
** Copyright (C) 2012 Research In Motion
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

#include "qqnxvirtualkeyboardbps.h"

#include <QDebug>

#include <bps/event.h>
#include <bps/locale.h>
#include <bps/virtualkeyboard.h>

#if defined(QQNXVIRTUALKEYBOARD_DEBUG)
#define qVirtualKeyboardDebug qDebug
#else
#define qVirtualKeyboardDebug QT_NO_QDEBUG_MACRO
#endif

QT_BEGIN_NAMESPACE

QQnxVirtualKeyboardBps::QQnxVirtualKeyboardBps(QObject *parent)
    : QQnxAbstractVirtualKeyboard(parent)
{
    if (locale_request_events(0) != BPS_SUCCESS)
        qWarning("QQNX: Failed to register for locale events");

    if (virtualkeyboard_request_events(0) != BPS_SUCCESS)
        qWarning("QQNX: Failed to register for virtual keyboard events");

    int height = 0;
    if (virtualkeyboard_get_height(&height) != BPS_SUCCESS)
        qWarning("QQNX: Failed to get virtual keyboard height");

    setHeight(height);
}

bool QQnxVirtualKeyboardBps::handleEvent(bps_event_t *event)
{
    const int eventDomain = bps_event_get_domain(event);
    if (eventDomain == locale_get_domain())
        return handleLocaleEvent(event);

    if (eventDomain == virtualkeyboard_get_domain())
        return handleVirtualKeyboardEvent(event);

    return false;
}

bool QQnxVirtualKeyboardBps::showKeyboard()
{
    qVirtualKeyboardDebug() << Q_FUNC_INFO << "current visibility=" << isVisible();

    // They keyboard's mode is global between applications, we have to set it each time
    if ( !isVisible() )
        applyKeyboardMode(keyboardMode());

    virtualkeyboard_show();
    return true;
}

bool QQnxVirtualKeyboardBps::hideKeyboard()
{
    qVirtualKeyboardDebug() << Q_FUNC_INFO << "current visibility=" << isVisible();
    virtualkeyboard_hide();
    return true;
}

void QQnxVirtualKeyboardBps::applyKeyboardMode(KeyboardMode mode)
{
    virtualkeyboard_layout_t layout = VIRTUALKEYBOARD_LAYOUT_DEFAULT;

    switch (mode) {
    case Url:
        layout = VIRTUALKEYBOARD_LAYOUT_URL;
        break;

    case Email:
        layout = VIRTUALKEYBOARD_LAYOUT_EMAIL;
        break;

    case Web:
        layout = VIRTUALKEYBOARD_LAYOUT_WEB;
        break;

    case NumPunc:
        layout = VIRTUALKEYBOARD_LAYOUT_NUM_PUNC;
        break;

    case Symbol:
        layout = VIRTUALKEYBOARD_LAYOUT_SYMBOL;
        break;

    case Phone:
        layout = VIRTUALKEYBOARD_LAYOUT_PHONE;
        break;

    case Pin:
        layout = VIRTUALKEYBOARD_LAYOUT_PIN;
        break;

    case Default: // fall through
    default:
        layout = VIRTUALKEYBOARD_LAYOUT_DEFAULT;
        break;
    }

    qVirtualKeyboardDebug() << Q_FUNC_INFO << "mode=" << mode;

    virtualkeyboard_change_options(layout, VIRTUALKEYBOARD_ENTER_DEFAULT);
}

bool QQnxVirtualKeyboardBps::handleLocaleEvent(bps_event_t *event)
{
    if (bps_event_get_code(event) == LOCALE_INFO) {
        const QString language = QString::fromLatin1(locale_event_get_language(event));
        const QString country  = QString::fromLatin1(locale_event_get_country(event));
        const QLocale newLocale(language + QLatin1Char('_') + country);

        qVirtualKeyboardDebug() << Q_FUNC_INFO << "current locale" << locale() << "new locale=" << newLocale;
        setLocale(newLocale);
        return true;
    }

    qVirtualKeyboardDebug() << Q_FUNC_INFO << "Unhandled locale event. code=" << bps_event_get_code(event);

    return false;
}

bool QQnxVirtualKeyboardBps::handleVirtualKeyboardEvent(bps_event_t *event)
{
    switch (bps_event_get_code(event)) {
    case VIRTUALKEYBOARD_EVENT_VISIBLE:
        qVirtualKeyboardDebug() << Q_FUNC_INFO << "EVENT VISIBLE: current visibility=" << isVisible();
        setVisible(true);
        break;

    case VIRTUALKEYBOARD_EVENT_HIDDEN:
        qVirtualKeyboardDebug() << Q_FUNC_INFO << "EVENT HIDDEN: current visibility=" << isVisible();
        setVisible(false);
        break;

    case VIRTUALKEYBOARD_EVENT_INFO: {
        const int newHeight = virtualkeyboard_event_get_height(event);
        qVirtualKeyboardDebug() << Q_FUNC_INFO << "EVENT INFO: current height=" << height() << "new height=" << newHeight;
        setHeight(newHeight);
        break;
    }

    default:
        qVirtualKeyboardDebug() << Q_FUNC_INFO << "Unhandled virtual keyboard event. code=" << bps_event_get_code(event);
        return false;
    }

    return true;
}

QT_END_NAMESPACE
