// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

//#define QHOSTINFO_DEBUG

#include "qhostinfo_p.h"

#include <qbytearray.h>
#include <qfile.h>
#include <qplatformdefs.h>
#include <qurl.h>

#include <sys/types.h>
#include <netdb.h>
#include <netinet/in.h>
#include <resolv.h>

#ifndef _PATH_RESCONF
#  define _PATH_RESCONF "/etc/resolv.conf"
#endif

QT_BEGIN_NAMESPACE

using namespace Qt::StringLiterals;

static void maybeRefreshResolver()
{
#if defined(RES_NORELOAD)
    // If RES_NORELOAD is defined, then the libc is capable of watching
    // /etc/resolv.conf for changes and reloading as necessary. So accept
    // whatever is configured.
    return;
#elif defined(Q_OS_DARWIN)
    // Apple's libsystem_info.dylib:getaddrinfo() uses the
    // libsystem_dnssd.dylib to resolve hostnames. Using res_init() has no
    // effect on it and is thread-unsafe.
    return;
#elif defined(Q_OS_FREEBSD)
    // FreeBSD automatically refreshes:
    // https://github.com/freebsd/freebsd-src/blob/b3fe5d932264445cbf9a1c4eab01afb6179b499b/lib/libc/resolv/res_state.c#L69
    return;
#elif defined(Q_OS_OPENBSD)
    // OpenBSD automatically refreshes:
    // https://github.com/ligurio/openbsd-src/blob/b1ce0da17da254cc15b8aff25b3d55d3c7a82cec/lib/libc/asr/asr.c#L367
    return;
#elif defined(Q_OS_QNX)
    // res_init() is not thread-safe; executing it leads to state corruption.
    // Whether it reloads resolv.conf on its own is unknown.
    return;
#endif

#if QT_CONFIG(libresolv)
    // OSes known or thought to reach here: AIX, NetBSD, Solaris,
    // Linux with MUSL (though res_init() does nothing and is unnecessary)

    Q_CONSTINIT static QT_STATBUF lastStat = {};
    Q_CONSTINIT static QBasicMutex mutex = {};
    if (QT_STATBUF st; QT_STAT(_PATH_RESCONF, &st) == 0) {
        QMutexLocker locker(&mutex);
        bool refresh = false;
        if ((_res.options & RES_INIT) == 0)
            refresh = true;
        else if (lastStat.st_ctime != st.st_ctime)
            refresh = true;     // file was updated
        else if (lastStat.st_dev != st.st_dev || lastStat.st_ino != st.st_ino)
            refresh = true;     // file was replaced
        if (refresh) {
            lastStat = st;
            res_init();
        }
    }
#endif
}

QHostInfo QHostInfoAgent::fromName(const QString &hostName)
{
    QHostInfo results;

#if defined(QHOSTINFO_DEBUG)
    qDebug("QHostInfoAgent::fromName(%s) looking up...",
           hostName.toLatin1().constData());
#endif

    maybeRefreshResolver();

    QHostAddress address;
    if (address.setAddress(hostName))
        return reverseLookup(address);

    return lookup(hostName);
}

QString QHostInfo::localDomainName()
{
#if QT_CONFIG(libresolv)
    auto domainNameFromRes = [](res_state r) {
        QString domainName;
        if (r->defdname[0])
            domainName = QUrl::fromAce(r->defdname);
        if (domainName.isEmpty())
            domainName = QUrl::fromAce(r->dnsrch[0]);
        return domainName;
    };
    std::remove_pointer_t<res_state> state = {};
    if (res_ninit(&state) == 0) {
        // using thread-safe version
        auto guard = qScopeGuard([&] { res_nclose(&state); });
        return domainNameFromRes(&state);
    }

    // using thread-unsafe version
    maybeRefreshResolver();
    return domainNameFromRes(&_res);
#endif  // !QT_CONFIG(libresolv)

    // nothing worked, try doing it by ourselves:
    QFile resolvconf;
    resolvconf.setFileName(_PATH_RESCONF ""_L1);
    if (!resolvconf.open(QIODevice::ReadOnly))
        return QString();       // failure

    QString domainName;
    while (!resolvconf.atEnd()) {
        QByteArray line = resolvconf.readLine().trimmed();
        if (line.startsWith("domain "))
            return QUrl::fromAce(line.mid(sizeof "domain " - 1).trimmed());

        // in case there's no "domain" line, fall back to the first "search" entry
        if (domainName.isEmpty() && line.startsWith("search ")) {
            QByteArray searchDomain = line.mid(sizeof "search " - 1).trimmed();
            int pos = searchDomain.indexOf(' ');
            if (pos != -1)
                searchDomain.truncate(pos);
            domainName = QUrl::fromAce(searchDomain);
        }
    }

    // return the fallen-back-to searched domain
    return domainName;
}

QT_END_NAMESPACE
