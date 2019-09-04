/****************************************************************************
**
** Copyright (C) 2016 The Qt Company Ltd.
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
QUrl url("http://www.example.com/List of holidays.xml");
// url.toEncoded() == "http://www.example.com/List%20of%20holidays.xml"
//! [0]


//! [1]
QUrl url = QUrl::fromEncoded("http://qt-project.org/List%20of%20holidays.xml");
//! [1]


//! [2]
bool checkUrl(const QUrl &url) {
    if (!url.isValid()) {
        qDebug("Invalid URL: %s", qUtf8Printable(url.toString()));
        return false;
    }

    return true;
}
//! [2]


//! [3]
QTcpSocket sock;
sock.connectToHost(url.host(), url.port(80));
//! [3]


//! [4]
http://www.example.com/cgi-bin/drawgraph.cgi?type(pie)color(green)
//! [4]


//! [5]
QUrl baseUrl("http://qt.digia.com/Support/");
QUrl relativeUrl("../Product/Library/");
qDebug(baseUrl.resolved(relativeUrl).toString());
// prints "http://qt.digia.com/Product/Library/"
//! [5]


//! [6]
QByteArray ba = QUrl::toPercentEncoding("{a fishy string?}", "{}", "s");
qDebug(ba.constData());
// prints "{a fi%73hy %73tring%3F}"
//! [6]

//! [7]
QUrl url("http://qt-project.org/support/file.html");
// url.adjusted(RemoveFilename) == "http://qt-project.org/support/"
// url.fileName() == "file.html"
//! [7]

//! [8]
    qDebug() << QUrl("main.qml").isRelative();          // true: no scheme
    qDebug() << QUrl("qml/main.qml").isRelative();      // true: no scheme
    qDebug() << QUrl("file:main.qml").isRelative();     // false: has "file" scheme
    qDebug() << QUrl("file:qml/main.qml").isRelative(); // false: has "file" scheme
//! [8]

//! [9]
    // Absolute URL, relative path
    QUrl url("file:file.txt");
    qDebug() << url.isRelative();                 // false: has "file" scheme
    qDebug() << QDir::isAbsolutePath(url.path()); // false: relative path

    // Relative URL, absolute path
    url = QUrl("/home/user/file.txt");
    qDebug() << url.isRelative();                 // true: has no scheme
    qDebug() << QDir::isAbsolutePath(url.path()); // true: absolute path
//! [9]

//! [10]
    QUrl original("http://example.com/?q=a%2B%3Db%26c");
    QUrl copy(original);
    copy.setQuery(copy.query(QUrl::FullyDecoded), QUrl::DecodedMode);

    qDebug() << original.toString();   // prints: http://example.com/?q=a%2B%3Db%26c
    qDebug() << copy.toString();       // prints: http://example.com/?q=a+=b&c
//! [10]

//! [11]
    QUrl url;
    url.setScheme("ftp");
//! [11]

//! [12]
    qDebug() << QUrl("file:file.txt").path();                   // "file.txt"
    qDebug() << QUrl("/home/user/file.txt").path();             // "/home/user/file.txt"
    qDebug() << QUrl("http://www.example.com/test/123").path(); // "/test/123"
//! [12]

//! [13]
    qDebug() << QUrl("/foo%FFbar").path();
//! [13]

//! [14]
    qDebug() << QUrl("/foo+bar%2B").path(); // "/foo+bar+"
//! [14]

//! [15]
    const QUrl url("/tmp/Mambo %235%3F.mp3");
    qDebug() << url.path(QUrl::FullyDecoded);  // "/tmp/Mambo #5?.mp3"
    qDebug() << url.path(QUrl::PrettyDecoded); // "/tmp/Mambo #5?.mp3"
    qDebug() << url.path(QUrl::FullyEncoded);  // "/tmp/Mambo%20%235%3F.mp3"
//! [15]

//! [16]
    qDebug() << QUrl::fromLocalFile("file.txt");            // QUrl("file:file.txt")
    qDebug() << QUrl::fromLocalFile("/home/user/file.txt"); // QUrl("file:///home/user/file.txt")
    qDebug() << QUrl::fromLocalFile("file:file.txt");       // doesn't make sense; expects path, not url with scheme
//! [16]

//! [17]
    QUrl url = QUrl::fromLocalFile("file.txt");
    QUrl baseUrl = QUrl("file:/home/user/");
    // wrong: prints QUrl("file:file.txt"), as url already has a scheme
    qDebug() << baseUrl.resolved(url);
//! [17]

//! [18]
    // correct: prints QUrl("file:///home/user/file.txt")
    url.setScheme(QString());
    qDebug() << baseUrl.resolved(url);
//! [18]

//! [19]
    QUrl url = QUrl("file.txt");
    QUrl baseUrl = QUrl("file:/home/user/");
    // prints QUrl("file:///home/user/file.txt")
    qDebug() << baseUrl.resolved(url);
//! [19]

//! [20]
    qDebug() << QUrl("file:file.txt").toLocalFile();            // "file.txt"
    qDebug() << QUrl("file:/home/user/file.txt").toLocalFile(); // "/home/user/file.txt"
    qDebug() << QUrl("file.txt").toLocalFile();                 // ""; wasn't a local file as it had no scheme
//! [20]
