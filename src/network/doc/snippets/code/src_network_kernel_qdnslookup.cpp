// Copyright (C) 2016 Jeremy Lainé <jeremy.laine@m4x.org>
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR BSD-3-Clause

//! [0]
void MyObject::lookupServers()
{
    // Create a DNS lookup.
    dns = new QDnsLookup(this);
    connect(dns, &QDnsLookup::finished, this, &MyObject::handleServers);

    // Find the XMPP servers for gmail.com
    dns->setType(QDnsLookup::SRV);
    dns->setName("_xmpp-client._tcp.gmail.com");
    dns->lookup();
}
//! [0]


//! [1]
void MyObject::handleServers()
{
    // Check the lookup succeeded.
    if (dns->error() != QDnsLookup::NoError) {
        qWarning("DNS lookup failed");
        dns->deleteLater();
        return;
    }

    // Handle the results.
    const auto records = dns->serviceRecords();
    for (const QDnsServiceRecord &record : records) {
        ...
    }
    dns->deleteLater();
}
//! [1]
