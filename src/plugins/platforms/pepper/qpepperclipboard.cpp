/****************************************************************************
**
** Copyright (C) 2015 Digia Plc and/or its subsidiary(-ies).
** Contact: http://www.qt-project.org/legal
**
** This file is part of the Qt for Native Client platform plugin.
**
** $QT_BEGIN_LICENSE:LGPL3$
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company. For licensing terms
** and conditions see http://www.qt.io/terms-conditions. For further
** information use the contact form at http://www.qt.io/contact-us.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 3 as published by the Free Software
** Foundation and appearing in the file LICENSE.LGPLv3 included in the
** packaging of this file. Please review the following information to
** ensure the GNU Lesser General Public License version 3 requirements
** will be met: https://www.gnu.org/licenses/lgpl.html.
**
** GNU General Public License Usage
** Alternatively, this file may be used under the terms of the GNU
** General Public License version 2.0 or later as published by the Free
** Software Foundation and appearing in the file LICENSE.GPL included in
** the packaging of this file. Please review the following information to
** ensure the GNU General Public License version 2.0 requirements will be
** met: http://www.gnu.org/licenses/gpl-2.0.html.
** $QT_END_LICENSE$
**
****************************************************************************/

#include "qpepperclipboard.h"

#include "qpepperinstance_p.h"

#include <QtCore/QDebug>
#include <QtCore/QMimeData>
#include <QtGui/QClipboard>

QT_BEGIN_NAMESPACE

Q_LOGGING_CATEGORY(QT_PLATFORM_PEPPER_CLIPBOARD, "qt.platform.pepper.clipboard")

QPepperClipboard::QPepperClipboard()
{
    qCDebug(QT_PLATFORM_PEPPER_CLIPBOARD) << "QPepperClipboard()";

    // NOT IMPLEMENTED, but this is how it should work:
    //
    // Under the web security model copy/paste operations have to
    // be initated by the user and clipboard access is only granted
    // during processing of those events. Set up javascript event
    // handlers that will listen to cut, copy, and paste.
    //
    // The javascript clipboard event handlers will forward the events
    // to the Qt pepper plugin, which whill trigger a copy/paste in
    // Qt Gui/app code, wich will call back to the setMimeData() / mimeData()
    // functions in this class.
    //
    // HOWEVER: the copy event handler is not called when the NaCl embed
    // has focus, even if Qt does not take any of the key events.
    //

    const char *copyEventhandler
        = "embed.addEventListener('copy', function (ev) {"
          "   ev.clipboardData.setData('text/plain', 'some text here, ' + new Date);"
          "   ev.preventDefault();"
          "   console.log('clipboard copy');"
          "   console.log(embed.postMessageAndAwaitResponse);"
          "})";

    QPepperInstancePrivate::get()->runJavascript(copyEventhandler);
}

QPepperClipboard::~QPepperClipboard() {}

QMimeData *QPepperClipboard::mimeData(QClipboard::Mode mode)
{
    qCDebug(QT_PLATFORM_PEPPER_CLIPBOARD) << "mimeData" << mode;
    if (mode != QClipboard::Clipboard)
        return 0;
    QByteArray message = "qtClipboardRequestPaste: ";
    QPepperInstancePrivate::get()->postMessage(message);
    return 0;
}

void QPepperClipboard::setMimeData(QMimeData *mimeData, QClipboard::Mode mode)
{
    qCDebug(QT_PLATFORM_PEPPER_CLIPBOARD) << "setMimeData" << mode;
    if (mode != QClipboard::Clipboard)
        return;
    QByteArray message = "qtClipboardRequestCopy:" + mimeData->text().toUtf8();
    QPepperInstancePrivate::get()->postMessage(message);
}

bool QPepperClipboard::supportsMode(QClipboard::Mode mode) const
{
    return mode == QClipboard::Clipboard;
}

bool QPepperClipboard::ownsMode(QClipboard::Mode mode) const
{
    return (mode == QClipboard::Clipboard);
}

QT_END_NAMESPACE
