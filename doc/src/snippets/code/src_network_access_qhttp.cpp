/****************************************************************************
**
** Copyright (C) 2013 Digia Plc and/or its subsidiary(-ies).
** Contact: http://www.qt-project.org/legal
**
** This file is part of the documentation of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:BSD$
** You may use this file under the terms of the BSD license as follows:
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
**   * Neither the name of Digia Plc and its Subsidiary(-ies) nor the names
**     of its contributors may be used to endorse or promote products derived
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
content-type: text/html
//! [0]


//! [1]
header.setValue("content-type", "text/html");
QString contentType = header.value("content-type");
//! [1]


//! [2]
QHttpRequestHeader header("GET", QUrl::toPercentEncoding("/index.html"));
header.setValue("Host", "qt-project.org");
http->setHost("qt-project.org");
http->request(header);
//! [2]


//! [3]
http->setHost("qt-project.org");                // id == 1
http->get(QUrl::toPercentEncoding("/index.html")); // id == 2
//! [3]


//! [4]
requestStarted(1)
requestFinished(1, false)

requestStarted(2)
stateChanged(Connecting)
stateChanged(Sending)
dataSendProgress(77, 77)
stateChanged(Reading)
responseHeaderReceived(responseheader)
dataReadProgress(5388, 0)
readyRead(responseheader)
dataReadProgress(18300, 0)
readyRead(responseheader)
stateChanged(Connected)
requestFinished(2, false)

done(false)

stateChanged(Closing)
stateChanged(Unconnected)
//! [4]


//! [5]
http->setHost("www.foo.bar");       // id == 1
http->get("/index.html");           // id == 2
http->post("register.html", data);  // id == 3
//! [5]


//! [6]
requestStarted(1)
requestFinished(1, false)

requestStarted(2)
stateChanged(HostLookup)
requestFinished(2, true)

done(true)

stateChanged(Unconnected)
//! [6]


//! [7]
void Ticker::getTicks()
{
  http = new QHttp(this);
  connect(http, SIGNAL(done(bool)), this, SLOT(showPage()));
  http->setProxy("proxy.example.com", 3128);
  http->setHost("ticker.example.com");
  http->get("/ticks.asp");
}

void Ticker::showPage()
{
  display(http->readAll());
}
//! [7]
