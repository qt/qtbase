/****************************************************************************
**
** Copyright (C) 2018 The Qt Company Ltd.
** Contact: https://www.qt.io/licensing/
**
** This file is part of the QtCore module of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:LGPL$
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company. For licensing terms
** and conditions see https://www.qt.io/terms-conditions. For further
** information use the contact form at https://www.qt.io/contact-us.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 3 as published by the Free Software
** Foundation and appearing in the file LICENSE.LGPL3 included in the
** packaging of this file. Please review the following information to
** ensure the GNU Lesser General Public License version 3 requirements
** will be met: https://www.gnu.org/licenses/lgpl-3.0.html.
**
** GNU General Public License Usage
** Alternatively, this file may be used under the terms of the GNU
** General Public License version 2.0 or (at your option) the GNU General
** Public license version 3 or any later version approved by the KDE Free
** Qt Foundation. The licenses are as published by the Free Software
** Foundation and appearing in the file LICENSE.GPL2 and LICENSE.GPL3
** included in the packaging of this file. Please review the following
** information to ensure the GNU General Public License requirements will
** be met: https://www.gnu.org/licenses/gpl-2.0.html and
** https://www.gnu.org/licenses/gpl-3.0.html.
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
