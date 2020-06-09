/****************************************************************************
**
** Copyright (C) 2016 The Qt Company Ltd.
** Contact: https://www.qt.io/licensing/
**
** This file is part of the QtNetwork module of the Qt Toolkit.
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

//#define QHOSTINFO_DEBUG

#include "qplatformdefs.h"

#include "qhostinfo_p.h"
#include "private/qnativesocketengine_p.h"
#include "qiodevice.h"
#include <qbytearray.h>
#if QT_CONFIG(library)
#include <qlibrary.h>
#endif
#include <qbasicatomic.h>
#include <qurl.h>
#include <qfile.h>
#include <private/qnet_unix_p.h>

#include <sys/types.h>
#include <netdb.h>
#include <arpa/inet.h>
#if defined(Q_OS_VXWORKS)
#  include <hostLib.h>
#else
#  include <resolv.h>
#endif

#if defined(__GNU_LIBRARY__) && !defined(__UCLIBC__)
#  include <gnu/lib-names.h>
#endif

#if defined(Q_OS_FREEBSD) || QT_CONFIG(dlopen)
#  include <dlfcn.h>
#endif

QT_BEGIN_NAMESPACE

enum LibResolvFeature {
    NeedResInit,
    NeedResNInit
};

typedef struct __res_state *res_state_ptr;

typedef int (*res_init_proto)(void);
static res_init_proto local_res_init = nullptr;
typedef int (*res_ninit_proto)(res_state_ptr);
static res_ninit_proto local_res_ninit = nullptr;
typedef void (*res_nclose_proto)(res_state_ptr);
static res_nclose_proto local_res_nclose = nullptr;
static res_state_ptr local_res = nullptr;

#if QT_CONFIG(library) && !defined(Q_OS_QNX)
namespace {
struct LibResolv
{
    enum {
#ifdef RES_NORELOAD
        // If RES_NORELOAD is defined, then the libc is capable of watching
        // /etc/resolv.conf for changes and reloading as necessary. So accept
        // whatever is configured.
        ReinitNecessary = false
#else
        ReinitNecessary = true
#endif
    };

    QLibrary lib;
    LibResolv();
    ~LibResolv() { lib.unload(); }
};
}

static QFunctionPointer resolveSymbol(QLibrary &lib, const char *sym)
{
    if (lib.isLoaded())
        return lib.resolve(sym);

#if defined(RTLD_DEFAULT) && (defined(Q_OS_FREEBSD) || QT_CONFIG(dlopen))
    return reinterpret_cast<QFunctionPointer>(dlsym(RTLD_DEFAULT, sym));
#else
    return nullptr;
#endif
}

LibResolv::LibResolv()
{
    QLibrary lib;
#ifdef LIBRESOLV_SO
    lib.setFileName(QStringLiteral(LIBRESOLV_SO));
    if (!lib.load())
#endif
    {
        lib.setFileName(QLatin1String("resolv"));
        lib.load();
    }

    // res_ninit is required for localDomainName()
    local_res_ninit = res_ninit_proto(resolveSymbol(lib, "__res_ninit"));
    if (!local_res_ninit)
        local_res_ninit = res_ninit_proto(resolveSymbol(lib, "res_ninit"));
    if (local_res_ninit) {
        // we must now find res_nclose
        local_res_nclose = res_nclose_proto(resolveSymbol(lib, "res_nclose"));
        if (!local_res_nclose)
            local_res_nclose = res_nclose_proto(resolveSymbol(lib, "__res_nclose"));
        if (!local_res_nclose)
            local_res_ninit = nullptr;
    }

    if (ReinitNecessary || !local_res_ninit) {
        local_res_init = res_init_proto(resolveSymbol(lib, "__res_init"));
        if (!local_res_init)
            local_res_init = res_init_proto(resolveSymbol(lib, "res_init"));

        if (local_res_init && !local_res_ninit) {
            // if we can't get a thread-safe context, we have to use the global _res state
            local_res = res_state_ptr(resolveSymbol(lib, "_res"));
        }
    }
}

LibResolv* libResolv()
{
    static LibResolv* theLibResolv = nullptr;
    static QBasicMutex theMutex;

    const QMutexLocker locker(&theMutex);
    if (theLibResolv == nullptr) {
        theLibResolv = new LibResolv();
        Q_ASSERT(QCoreApplication::instance());
        QObject::connect(QCoreApplication::instance(), &QCoreApplication::destroyed, [] {
            const QMutexLocker locker(&theMutex);
            delete theLibResolv;
            theLibResolv = nullptr;
        });
    }

    return theLibResolv;
}

static void resolveLibrary(LibResolvFeature f)
{
    if (LibResolv::ReinitNecessary || f == NeedResNInit)
        libResolv();
}
#else // QT_CONFIG(library) || Q_OS_QNX
static void resolveLibrary(LibResolvFeature)
{
}
#endif // QT_CONFIG(library) || Q_OS_QNX

QHostInfo QHostInfoAgent::fromName(const QString &hostName)
{
    QHostInfo results;

#if defined(QHOSTINFO_DEBUG)
    qDebug("QHostInfoAgent::fromName(%s) looking up...",
           hostName.toLatin1().constData());
#endif

    // Load res_init on demand.
    resolveLibrary(NeedResInit);

    // If res_init is available, poll it.
    if (local_res_init)
        local_res_init();

    QHostAddress address;
    if (address.setAddress(hostName))
        return reverseLookup(address);

    return lookup(hostName);
}

QString QHostInfo::localDomainName()
{
#if !defined(Q_OS_VXWORKS) && !defined(Q_OS_ANDROID)
    resolveLibrary(NeedResNInit);
    if (local_res_ninit) {
        // using thread-safe version
        res_state_ptr state = res_state_ptr(malloc(sizeof(*state)));
        Q_CHECK_PTR(state);
        memset(state, 0, sizeof(*state));
        local_res_ninit(state);
        QString domainName = QUrl::fromAce(state->defdname);
        if (domainName.isEmpty())
            domainName = QUrl::fromAce(state->dnsrch[0]);
        local_res_nclose(state);
        free(state);

        return domainName;
    }

    if (local_res_init && local_res) {
        // using thread-unsafe version

        local_res_init();
        QString domainName = QUrl::fromAce(local_res->defdname);
        if (domainName.isEmpty())
            domainName = QUrl::fromAce(local_res->dnsrch[0]);
        return domainName;
    }
#endif
    // nothing worked, try doing it by ourselves:
    QFile resolvconf;
#if defined(_PATH_RESCONF)
    resolvconf.setFileName(QFile::decodeName(_PATH_RESCONF));
#else
    resolvconf.setFileName(QLatin1String("/etc/resolv.conf"));
#endif
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
