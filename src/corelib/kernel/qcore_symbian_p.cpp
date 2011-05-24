/****************************************************************************
**
** Copyright (C) 2011 Nokia Corporation and/or its subsidiary(-ies).
** All rights reserved.
** Contact: Nokia Corporation (qt-info@nokia.com)
**
** This file is part of the QtCore module of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:LGPL$
** GNU Lesser General Public License Usage
** This file may be used under the terms of the GNU Lesser General Public
** License version 2.1 as published by the Free Software Foundation and
** appearing in the file LICENSE.LGPL included in the packaging of this
** file. Please review the following information to ensure the GNU Lesser
** General Public License version 2.1 requirements will be met:
** http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, Nokia gives you certain additional
** rights. These rights are described in the Nokia Qt LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
** GNU General Public License Usage
** Alternatively, this file may be used under the terms of the GNU General
** Public License version 3.0 as published by the Free Software Foundation
** and appearing in the file LICENSE.GPL included in the packaging of this
** file. Please review the following information to ensure the GNU General
** Public License version 3.0 requirements will be met:
** http://www.gnu.org/copyleft/gpl.html.
**
** Other Usage
** Alternatively, this file may be used in accordance with the terms and
** conditions contained in a signed written agreement between you and Nokia.
**
**
**
**
**
** $QT_END_LICENSE$
**
****************************************************************************/

#include <exception>
#include <e32base.h>
#include <e32uid.h>
#include "qcore_symbian_p.h"
#include <string>
#include <in_sock.h>

QT_BEGIN_NAMESPACE

/*
    Helper function for calling into Symbian classes that expect a TDes&.
    This function converts a QString to a TDes by allocating memory that
    must be deleted by the caller.
*/

Q_CORE_EXPORT HBufC* qt_QString2HBufC(const QString& aString)
{
    HBufC *buffer;
#ifdef QT_NO_UNICODE
    TPtrC8 ptr(reinterpret_cast<const TUint8*>(aString.toLocal8Bit().constData()));
#else
    TPtrC16 ptr(qt_QString2TPtrC(aString));
#endif
    buffer = HBufC::New(ptr.Length());
    Q_CHECK_PTR(buffer);
    buffer->Des().Copy(ptr);
    return buffer;
}

Q_CORE_EXPORT QString qt_TDesC2QString(const TDesC& aDescriptor)
{
#ifdef QT_NO_UNICODE
    return QString::fromLocal8Bit(aDescriptor.Ptr(), aDescriptor.Length());
#else
    return QString(reinterpret_cast<const QChar *>(aDescriptor.Ptr()), aDescriptor.Length());
#endif
}

QHBufC::QHBufC()
    : m_hBufC(0)
{
}

QHBufC::QHBufC(const QHBufC &src)
	: m_hBufC(src.m_hBufC->Alloc())
{
    Q_CHECK_PTR(m_hBufC);
}

/*!
  \internal
  Constructs a QHBufC from an HBufC. Note that the QHBufC instance takes
  ownership of the HBufC.
*/
QHBufC::QHBufC(HBufC *src)
    : m_hBufC(src)
{
}

QHBufC::QHBufC(const QString &src)
{
    m_hBufC = qt_QString2HBufC(src);
}

QHBufC::~QHBufC()
{
    if (m_hBufC)
        delete m_hBufC;
}

class QS60PluginResolver
{
public:
    QS60PluginResolver()
        : initTried(false) {}

    ~QS60PluginResolver() {
        lib.Close();
    }

    TLibraryFunction resolve(int ordinal) {
        if (!initTried) {
            init();
            initTried = true;
        }

        if (lib.Handle())
            return lib.Lookup(ordinal);
        else
            return reinterpret_cast<TLibraryFunction>(NULL);
    }

private:
    void init()
    {
        _LIT(KLibName_3_1, "qts60plugin_3_1" QT_LIBINFIX_UNICODE L".dll");
        _LIT(KLibName_3_2, "qts60plugin_3_2" QT_LIBINFIX_UNICODE L".dll");
        _LIT(KLibName_5_0, "qts60plugin_5_0" QT_LIBINFIX_UNICODE L".dll");

        TPtrC libName;
        TInt uidValue;
        switch (QSysInfo::s60Version()) {
        case QSysInfo::SV_S60_3_1:
            libName.Set(KLibName_3_1);
            uidValue = 0x2001E620;
            break;
        case QSysInfo::SV_S60_3_2:
            libName.Set(KLibName_3_2);
            uidValue = 0x2001E621;
            break;
        case QSysInfo::SV_S60_5_0: // Fall through to default
        default:
            // Default to 5.0 version, as any unknown platform is likely to be newer than that
            libName.Set(KLibName_5_0);
            uidValue = 0x2001E622;
            break;
        }

        TUidType libUid(KDynamicLibraryUid, KSharedLibraryUid, TUid::Uid(uidValue));
        lib.Load(libName, libUid);

        // Duplicate lib handle to enable process wide access to it. Since Duplicate overwrites
        // existing handle without closing it, store original for subsequent closing.
        RLibrary origHandleCloser = lib;
        lib.Duplicate(RThread(), EOwnerProcess);
        origHandleCloser.Close();
    }

    RLibrary lib;
    bool initTried;
};

Q_GLOBAL_STATIC(QS60PluginResolver, qt_s60_plugin_resolver);

/*!
  \internal
  Resolves a platform version specific function from S60 plugin.
  If plugin is missing or resolving fails for another reason, NULL is returned.
*/
Q_CORE_EXPORT TLibraryFunction qt_resolveS60PluginFunc(int ordinal)
{
    return qt_s60_plugin_resolver()->resolve(ordinal);
}

class QS60RFsSession
{
public:
    QS60RFsSession() {
        qt_symbian_throwIfError(iFs.Connect());
        qt_symbian_throwIfError(iFs.ShareProtected());
    }

    ~QS60RFsSession() {
        iFs.Close();
    }

    RFs& GetRFs() {
        return iFs;
    }

private:

    RFs iFs;
};

uint qHash(const RSubSessionBase& key)
{
    return qHash(key.SubSessionHandle());
}

Q_GLOBAL_STATIC(QS60RFsSession, qt_s60_RFsSession);

Q_CORE_EXPORT RFs& qt_s60GetRFs()
{
    return qt_s60_RFsSession()->GetRFs();
}

QSymbianSocketManager::QSymbianSocketManager() :
    iNextSocket(0), iDefaultConnection(0)
{
    TSessionPref preferences;
    // ### In future this could be changed to KAfInet6 when that is more common than IPv4
    preferences.iAddrFamily = KAfInet;
    preferences.iProtocol = KProtocolInetIp;
    //use global message pool, as we do not know how many sockets app will use
    //TODO: is this the right choice?
    qt_symbian_throwIfError(iSocketServ.Connect(preferences, -1));
    qt_symbian_throwIfError(iSocketServ.ShareAuto());
}

QSymbianSocketManager::~QSymbianSocketManager()
{
    iSocketServ.Close();
    if(!socketMap.isEmpty()) {
        qWarning("leaked %d sockets on exit", socketMap.count());
    }
}

RSocketServ& QSymbianSocketManager::getSocketServer() {
    return iSocketServ;
}

int QSymbianSocketManager::addSocket(const RSocket& socket) {
    QHashableSocket sock(static_cast<const QHashableSocket &>(socket));
    QMutexLocker l(&iMutex);
    Q_ASSERT(!socketMap.contains(sock));
    if(socketMap.contains(sock))
        return socketMap.value(sock);
    // allocate socket number
    int guard = 0;
    while(reverseSocketMap.contains(iNextSocket)) {
        iNextSocket++;
        iNextSocket %= max_sockets;
        guard++;
        if(guard > max_sockets)
            return -1;
    }
    int id = iNextSocket;

    socketMap[sock] = id;
    reverseSocketMap[id] = sock;
    return id + socket_offset;
}

bool QSymbianSocketManager::removeSocket(const RSocket &socket) {
    QHashableSocket sock(static_cast<const QHashableSocket &>(socket));
    QMutexLocker l(&iMutex);
    if(!socketMap.contains(sock))
        return false;
    int id = socketMap.value(sock);
    socketMap.remove(sock);
    reverseSocketMap.remove(id);
    return true;
}

int QSymbianSocketManager::lookupSocket(const RSocket& socket) const {
    QHashableSocket sock(static_cast<const QHashableSocket &>(socket));
    QMutexLocker l(&iMutex);
    if(!socketMap.contains(sock))
        return -1;
    int id = socketMap.value(sock);
    return id + socket_offset;
}

bool QSymbianSocketManager::lookupSocket(int fd, RSocket& socket) const {
    QMutexLocker l(&iMutex);
    int id = fd - socket_offset;
    if(!reverseSocketMap.contains(id))
        return false;
    socket = reverseSocketMap.value(id);
    return true;
}

void QSymbianSocketManager::setDefaultConnection(RConnection* con)
{
    iDefaultConnection = con;
}

RConnection* QSymbianSocketManager::defaultConnection() const
{
    return iDefaultConnection;
}

Q_GLOBAL_STATIC(QSymbianSocketManager, qt_symbianSocketManager);

QSymbianSocketManager& QSymbianSocketManager::instance()
{
    return *(qt_symbianSocketManager());
}

Q_CORE_EXPORT RSocketServ& qt_symbianGetSocketServer()
{
    return QSymbianSocketManager::instance().getSocketServer();
}

QT_END_NAMESPACE
