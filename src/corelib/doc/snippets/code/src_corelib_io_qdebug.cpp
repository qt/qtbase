/****************************************************************************
**
** Copyright (C) 2018 The Qt Company Ltd.
** Contact: https://www.qt.io/licensing/
**
** This file is part of the documentation of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:BSD$
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company. For licensing terms
** and conditions see https://www.qt.io/terms-conditions. For further
** information use the contact form at https://www.qt.io/contact-us.
**
** BSD License Usage
** Alternatively, you may use this file under the terms of the BSD license
** as follows:
**
** "Redistribution and use in source and binary forms, with or without
** modification, are permitted provided that the following conditions are
** met:
**   * Redistributions of source code must retain the above copyright
**     notice, this list of conditions and the following disclaimer.
**   * Redistributions in binary form must reproduce the above copyright
**     notice, this list of conditions and the following disclaimer in
**     the documentation and/or other materials provided with the
**     distribution.
**   * Neither the name of The Qt Company Ltd nor the names of its
**     contributors may be used to endorse or promote products derived
**     from this software without specific prior written permission.
**
**
** THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
** "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
** LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
** A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
** OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
** SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
** LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
** DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
** THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
** (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
** OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE."
**
** $QT_END_LICENSE$
**
****************************************************************************/

//! [0]
    QString s;

    s = "a";
    qDebug().noquote() << s;    // prints: a
    qDebug() << s;              // prints: "a"

    s = "\"a\r\n\"";
    qDebug() << s;              // prints: "\"a\r\n\""

    s = "\033";                 // escape character
    qDebug() << s;              // prints: "\u001B"

    s = "\u00AD";               // SOFT HYPHEN
    qDebug() << s;              // prints: "\u00AD"

    s = "\u00E1";               // LATIN SMALL LETTER A WITH ACUTE
    qDebug() << s;              // prints: "á"

    s = "a\u0301";              // "a" followed by COMBINING ACUTE ACCENT
    qDebug() << s;              // prints: "á";

    s = "\u0430\u0301";         // CYRILLIC SMALL LETTER A followed by COMBINING ACUTE ACCENT
    qDebug() << s;              // prints: "а́"
//! [0]

//! [1]
    QByteArray ba;

    ba = "a";
    qDebug().noquote() << ba;    // prints: a
    qDebug() << ba;              // prints: "a"

    ba = "\"a\r\n\"";
    qDebug() << ba;              // prints: "\"a\r\n\""

    ba = "\033";                 // escape character
    qDebug() << ba;              // prints: "\x1B"

    ba = "\xC3\xA1";
    qDebug() << ba;              // prints: "\xC3\xA1"

    ba = QByteArray("a\0b", 3);
    qDebug() << ba               // prints: "\a\x00""b"
//! [1]
