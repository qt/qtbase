/****************************************************************************
**
** Copyright (C) 2014 Digia Plc
** All rights reserved.
** For any questions to Digia, please use contact form at http://qt.digia.com <http://qt.digia.com/>
**
** This file is part of the Qt Native Client platform plugin.
**
** No Commercial Usage
** This file contains pre-release code and may not be distributed.
** You may use this file in accordance with the terms and conditions
** contained in the Technology Preview License Agreement accompanying
** this package.
**
** If you have questions regarding the use of this file, please use
** contact form at http://qt.digia.com <http://qt.digia.com/>
**
****************************************************************************/

#include "qpepperclipboard.h"
#include "qpepperinstance_p.h"

#include <QtGui/QClipboard>

#include <QtCore/QDebug>
#include <QtCore/QMimeData>

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
