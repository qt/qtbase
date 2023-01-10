// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR BSD-3-Clause

//! [0]
// To find the IP address of qt-project.org
QHostInfo::lookupHost("qt-project.org", this, &MyWidget::printResults);

// To find the host name for 4.2.2.1
QHostInfo::lookupHost("4.2.2.1", this, &MyWidget::printResults);
//! [0]


//! [1]
QHostInfo info = QHostInfo::fromName("qt-project.org");
//! [1]


//! [2]
QHostInfo::lookupHost("www.kde.org", this, &MyWidget::lookedUp);
//! [2]


//! [3]
void MyWidget::lookedUp(const QHostInfo &host)
{
    if (host.error() != QHostInfo::NoError) {
        qDebug() << "Lookup failed:" << host.errorString();
        return;
    }

    const auto addresses = host.addresses();
    for (const QHostAddress &address : addresses)
        qDebug() << "Found address:" << address.toString();
}
//! [3]


//! [4]
QHostInfo::lookupHost("4.2.2.1", this, &MyWidget::lookedUp);
//! [4]


//! [5]
QHostInfo info;
...
if (!info.addresses().isEmpty()) {
    QHostAddress address = info.addresses().first();
    // use the first IP address
}
//! [5]
