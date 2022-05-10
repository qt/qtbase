// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR BSD-3-Clause

//! [0]
QNetworkProxy proxy;
proxy.setType(QNetworkProxy::Socks5Proxy);
proxy.setHostName("proxy.example.com");
proxy.setPort(1080);
proxy.setUser("username");
proxy.setPassword("password");
QNetworkProxy::setApplicationProxy(proxy);
//! [0]


//! [1]
serverSocket->setProxy(QNetworkProxy::NoProxy);
//! [1]
