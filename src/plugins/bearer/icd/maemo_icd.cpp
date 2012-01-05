/****************************************************************************
**
** Copyright (C) 2012 Nokia Corporation and/or its subsidiary(-ies).
** All rights reserved.
** Contact: Nokia Corporation (qt-info@nokia.com)
**
** This file is part of the plugins of the Qt Toolkit.
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


#include "maemo_icd.h"
#include "dbusdispatcher.h"

#include <QObject>
#include <QTimer>
#include <QCoreApplication>
#include <QEventLoop>
#include <QDebug>
#include <dbus/dbus.h>
#include <dbus/dbus-glib-lowlevel.h>

#include <stdio.h>
#include <sys/time.h>
#include <sys/types.h>
#include <unistd.h>

namespace Maemo {

#undef PRINT_DEBUGINFO
#ifdef PRINT_DEBUGINFO
  static FILE *fdebug = NULL;
#define PDEBUG(fmt, args...)						\
  do {									\
    struct timeval tv;							\
    gettimeofday(&tv, 0);						\
    fprintf(fdebug, "DEBUG[%d]:%ld.%ld:%s:%s():%d: " fmt,		\
	    getpid(),							\
	    tv.tv_sec, tv.tv_usec,					\
	    __FILE__, __FUNCTION__, __LINE__, args);			\
    fflush(fdebug);							\
  } while(0)
#else
#define PDEBUG(fmt...)
#endif


class IcdPrivate
{
public:
    IcdPrivate(Icd *myfriend)
    {
        init(10000, IcdNewDbusInterface, myfriend);
    }

    IcdPrivate(unsigned int timeout, Icd *myfriend)
    {
        init(timeout, IcdNewDbusInterface, myfriend);
    }

    IcdPrivate(unsigned int timeout, IcdDbusInterfaceVer ver, Icd *myfriend)
    {
        Q_UNUSED(ver);

        /* Note that the old Icd interface is currently disabled and
	 * the new one is always used.
	 */
        init(timeout, IcdNewDbusInterface, myfriend);
    }

    ~IcdPrivate()
    {
      QObject::disconnect(mDBus, 
			  SIGNAL(signalReceived(const QString&, 
						const QString&,
						const QList<QVariant>&)),
			  icd,
			  SLOT(icdSignalReceived(const QString&, 
						 const QString&,
						 const QList<QVariant>&)));

      QObject::disconnect(mDBus,
			  SIGNAL(callReply(const QString&,
					   const QList<QVariant>&,
					   const QString&)),
			  icd,
			  SLOT(icdCallReply(const QString&,
					    const QList<QVariant>&,
					    const QString&)));

        delete mDBus;
        mDBus = 0;
    }

    /* Icd2 dbus API functions */
    QStringList scan(icd_scan_request_flags flags,
		     QStringList &network_types,
		     QList<IcdScanResult>& scan_results,
		     QString& error);

    uint state(QString& service_type, uint service_attrs,
	       QString& service_id, QString& network_type,
	       uint network_attrs, QByteArray& network_id,
	       IcdStateResult &state_result);

    uint addrinfo(QString& service_type, uint service_attrs,
		  QString& service_id, QString& network_type,
		  uint network_attrs, QByteArray& network_id,
		  IcdAddressInfoResult& addr_result);

    uint state(QList<IcdStateResult>& state_results);
    uint statistics(QList<IcdStatisticsResult>& stats_results);
    uint addrinfo(QList<IcdAddressInfoResult>& addr_results);

    void signalReceived(const QString& interface, 
                        const QString& signal,
                        const QList<QVariant>& args);
    void callReply(const QString& method, 
                   const QList<QVariant>& args,
                   const QString& error);

public:
    DBusDispatcher *mDBus;
    QString mMethod;
    QString mInterface;
    QString mSignal;
    QString mError;
    QList<QVariant> mArgs;
    QList<QVariant> receivedSignals;
    unsigned int timeout;
    IcdDbusInterfaceVer icd_dbus_version;
    Icd *icd;

    void init(unsigned int dbus_timeout, IcdDbusInterfaceVer ver,
	      Icd *myfriend)
    {
      if (ver == IcdNewDbusInterface) {
	mDBus = new DBusDispatcher(ICD_DBUS_API_INTERFACE,
				   ICD_DBUS_API_PATH,
				   ICD_DBUS_API_INTERFACE);
      } else {
	mDBus = new DBusDispatcher(ICD_DBUS_SERVICE,
				   ICD_DBUS_PATH,
				   ICD_DBUS_INTERFACE);
      }
      icd_dbus_version = ver;

      /* This connect has a side effect as it means that only one
       * Icd object can exists in one time. This should be fixed!
       */
      QObject::connect(mDBus, 
		       SIGNAL(signalReceived(const QString&, 
					     const QString&,
					     const QList<QVariant>&)),
		       myfriend,
		       SLOT(icdSignalReceived(const QString&, 
					      const QString&,
					      const QList<QVariant>&)));

      QObject::connect(mDBus,
		       SIGNAL(callReply(const QString&,
					const QList<QVariant>&,
					const QString&)),
		       myfriend,
		       SLOT(icdCallReply(const QString&,
					 const QList<QVariant>&,
					 const QString&)));

      icd = myfriend;
      timeout = dbus_timeout;

#ifdef PRINT_DEBUGINFO
      if (!fdebug) {
	fdebug = fopen("/tmp/maemoicd.log", "a+");
      }
      PDEBUG("created %s\n", "IcdPrivate");
#endif
    }

    void clearState()
    {
      mMethod.clear();
      mInterface.clear();
      mSignal.clear();
      mError.clear();
      mArgs.clear();
      receivedSignals.clear();
    }

    bool doState();
};


void IcdPrivate::signalReceived(const QString& interface, 
				const QString& signal,
				const QList<QVariant>& args)
{
    // Signal handler, which simply records what has been signalled
    mInterface = interface;
    mSignal = signal;
    mArgs = args;

    //qDebug() << "signal" << signal << "received:" << args;
    receivedSignals << QVariant(interface) << QVariant(signal) << QVariant(args);
}


void IcdPrivate::callReply(const QString& method, 
			   const QList<QVariant>& /*args*/,
			   const QString& error)
{
    mMethod = method;
    mError = error;
}


static void get_scan_result(QList<QVariant>& args,
			    IcdScanResult& ret)
{
    int i=0;

    if (args.isEmpty())
      return;

    ret.status = args[i++].toUInt();
    ret.timestamp = args[i++].toUInt();
    ret.scan.service_type = args[i++].toString();
    ret.service_name = args[i++].toString();
    ret.scan.service_attrs = args[i++].toUInt();
    ret.scan.service_id = args[i++].toString();
    ret.service_priority = args[i++].toInt();
    ret.scan.network_type = args[i++].toString();
    ret.network_name = args[i++].toString();
    ret.scan.network_attrs = args[i++].toUInt();
    ret.scan.network_id = args[i++].toByteArray();
    ret.network_priority = args[i++].toInt();
    ret.signal_strength = args[i++].toInt();
    ret.station_id = args[i++].toString();
    ret.signal_dB = args[i++].toInt();
}


QStringList IcdPrivate::scan(icd_scan_request_flags flags,
			     QStringList &network_types,
			     QList<IcdScanResult>& scan_results,
			     QString& error)
{
    Q_UNUSED(network_types);

    QStringList scanned_types;
    QTimer timer;
    QVariant reply;
    QVariantList vl;
    bool last_result = false;
    IcdScanResult result;
    int all_waited;
    
    clearState();
    reply = mDBus->call(ICD_DBUS_API_SCAN_REQ, (uint)flags);
    if (reply.type() != QVariant::List)
        return scanned_types;
    vl = reply.toList();
    if (vl.isEmpty()) {
        error = "Scan did not return anything.";
	return scanned_types;
    }
    reply = vl.first();
    scanned_types = reply.toStringList();
    //qDebug() << "Scanning:" << scanned_types;
    all_waited = scanned_types.size();

    timer.setSingleShot(true);
    timer.start(timeout);

    scan_results.clear();
    while (!last_result) {
	while (timer.isActive() && mInterface.isEmpty() && mError.isEmpty()) {
	    QCoreApplication::processEvents(QEventLoop::AllEvents, 1000);
	}

	if (!timer.isActive()) {
	    //qDebug() << "Timeout happened";
	    break;
	}

	if (mSignal != ICD_DBUS_API_SCAN_SIG) {
	    //qDebug() << "Received" << mSignal << "while waiting" << ICD_DBUS_API_SCAN_SIG << ", ignoring";
	    mInterface.clear();
	    continue;
	}

	if (mError.isEmpty()) {
  	    QString msgInterface = receivedSignals.takeFirst().toString();
	    QString msgSignal = receivedSignals.takeFirst().toString();
	    QList<QVariant> msgArgs = receivedSignals.takeFirst().toList();
	    //qDebug() << "Signal" << msgSignal << "received.";
	    //qDebug() << "Params:" << msgArgs;

	    while (!msgSignal.isEmpty()) {
		get_scan_result(msgArgs, result);

#if 0
		qDebug() << "Received: " <<
		    "status =" << result.status <<
		    ", timestamp =" << result.timestamp <<
		    ", service_type =" << result.scan.service_type <<
		    ", service_name =" << result.service_name <<
		    ", service_attrs =" << result.scan.service_attrs <<
		    ", service_id =" << result.scan.service_id <<
		    ", service_priority =" << result.service_priority <<
		    ", network_type =" << result.scan.network_type <<
		    ", network_name =" << result.network_name <<
		    ", network_attrs =" << result.scan.network_attrs <<
		    ", network_id =" << "-" <<
		    ", network_priority =" << result.network_priority <<
		    ", signal_strength =" << result.signal_strength <<
		    ", station_id =" << result.station_id <<
		    ", signal_dB =" << result.signal_dB;
#endif

		if (result.status == ICD_SCAN_COMPLETE) {
		    //qDebug() << "waited =" << all_waited;
		    if (--all_waited == 0) {
		        last_result = true;
			break;
		    }
		} else
		    scan_results << result;

		if (receivedSignals.isEmpty())
		    break;

		msgInterface = receivedSignals.takeFirst().toString();
		msgSignal = receivedSignals.takeFirst().toString();
		msgArgs = receivedSignals.takeFirst().toList();
	    }
	    mInterface.clear();

	} else {
	    qWarning() << "Error while scanning:" << mError;
	    break;
	}
    }
    timer.stop();

    error = mError;
    return scanned_types;
}


static void get_state_all_result(QList<QVariant>& args,
				 IcdStateResult& ret)
{
    int i=0;

    ret.params.service_type = args[i++].toString();
    ret.params.service_attrs = args[i++].toUInt();
    ret.params.service_id = args[i++].toString();
    ret.params.network_type = args[i++].toString();
    ret.params.network_attrs = args[i++].toUInt();
    ret.params.network_id = args[i++].toByteArray();
    ret.error = args[i++].toString();
    ret.state = args[i++].toInt();
}


static void get_state_all_result2(QList<QVariant>& args,
				 IcdStateResult& ret)
{
    int i=0;

    ret.params.network_type = args[i++].toString();
    ret.state = args[i++].toInt();

    // Initialize the other values so that the caller can
    // notice we only returned partial status
    ret.params.service_type = QString();
    ret.params.service_attrs = 0;
    ret.params.service_id = QString();
    ret.params.network_attrs = 0;
    ret.params.network_id = QByteArray();
    ret.error = QString();
}


uint IcdPrivate::state(QString& service_type, uint service_attrs,
		       QString& service_id, QString& network_type,
		       uint network_attrs, QByteArray& network_id,
		       IcdStateResult& state_result)
{
    QTimer timer;
    QVariant reply;
    uint total_signals;
    QVariantList vl;

    clearState();

    reply = mDBus->call(ICD_DBUS_API_STATE_REQ,
			service_type, service_attrs, service_id,
			network_type, network_attrs, network_id);
    if (reply.type() != QVariant::List)
        return 0;
    vl = reply.toList();
    if (vl.isEmpty())
        return 0;
    reply = vl.first();
    total_signals = reply.toUInt();
    if (!total_signals)
        return 0;

    timer.setSingleShot(true);
    timer.start(timeout);

    mInterface.clear();
    while (timer.isActive() && mInterface.isEmpty()) {
        QCoreApplication::processEvents(QEventLoop::AllEvents, 1000);

	if (mSignal != ICD_DBUS_API_STATE_SIG) {
            mInterface.clear();
	    continue;
	}
    }

    timer.stop();

    if (mError.isEmpty()) {
        if (!mArgs.isEmpty()) {
	    if (mArgs.size()>2)
	        get_state_all_result(mArgs, state_result);
	    else {
	        // We are not connected as we did not get the status we asked
	        return 0;
	    }
	}
    } else {
        qWarning() << "Error:" << mError;
    }

    // The returned value should be one because we asked for one state
    return total_signals;
}


/* Special version of the state() call which does not call event loop.
 * Needed in order to fix NB#175098 where Qt4.7 webkit crashes because event
 * loop is run when webkit does not expect it. This function is called from
 * bearer management API syncStateWithInterface() in QNetworkSession
 * constructor.
 */
uint IcdPrivate::state(QList<IcdStateResult>& state_results)
{
    QVariant reply;
    QVariantList vl;
    uint signals_left, total_signals;
    IcdStateResult result;
    time_t started;
    int timeout_secs = timeout / 1000;

    PDEBUG("%s\n", "state_results");

    clearState();
    reply = mDBus->call(ICD_DBUS_API_STATE_REQ);
    if (reply.type() != QVariant::List)
        return 0;
    vl = reply.toList();
    if (vl.isEmpty())
        return 0;
    reply = vl.first();
    signals_left = total_signals = reply.toUInt();
    if (!signals_left)
        return 0;

    started = time(0);
    state_results.clear();
    mError.clear();
    while (signals_left) {
        mInterface.clear();
	while ((time(0)<=(started+timeout_secs)) && mInterface.isEmpty()) {
	    mDBus->synchronousDispatch(1000);
        QCoreApplication::sendPostedEvents(icd, QEvent::MetaCall);
	}

        if (time(0)>(started+timeout_secs)) {
	    total_signals = 0;
	    break;
	}

	if (mSignal != ICD_DBUS_API_STATE_SIG) {
	    continue;
	}

	if (mError.isEmpty()) {
	    if (!mArgs.isEmpty()) {
	        if (mArgs.size()==2)
	            get_state_all_result2(mArgs, result);
		else
	            get_state_all_result(mArgs, result);
		state_results << result;
	    }
	    signals_left--;
	} else {
	    qWarning() << "Error:" << mError;
	    break;
	}
    }

    PDEBUG("total_signals=%d\n", total_signals);
    return total_signals;
}


static void get_statistics_all_result(QList<QVariant>& args,
				      IcdStatisticsResult& ret)
{
    int i=0;

    if (args.isEmpty())
      return;

    ret.params.service_type = args[i++].toString();
    ret.params.service_attrs = args[i++].toUInt();
    ret.params.service_id = args[i++].toString();
    ret.params.network_type = args[i++].toString();
    ret.params.network_attrs = args[i++].toUInt();
    ret.params.network_id = args[i++].toByteArray();
    ret.time_active = args[i++].toUInt();
    ret.signal_strength = (enum icd_nw_levels)args[i++].toUInt();
    ret.bytes_sent = args[i++].toUInt();
    ret.bytes_received = args[i++].toUInt();
}


uint IcdPrivate::statistics(QList<IcdStatisticsResult>& stats_results)
{
    QTimer timer;
    QVariant reply;
    QVariantList vl;
    uint signals_left, total_signals;
    IcdStatisticsResult result;

    clearState();
    reply = mDBus->call(ICD_DBUS_API_STATISTICS_REQ);
    if (reply.type() != QVariant::List)
        return 0;
    vl = reply.toList();
    if (vl.isEmpty())
        return 0;
    reply = vl.first();
    if (reply.type() != QVariant::UInt)
        return 0;
    signals_left = total_signals = reply.toUInt();

    if (!signals_left)
        return 0;

    timer.setSingleShot(true);
    timer.start(timeout);
    stats_results.clear();
    while (signals_left) {
	mInterface.clear();
	while (timer.isActive() && mInterface.isEmpty()) {
	    QCoreApplication::processEvents(QEventLoop::AllEvents, 1000);
	}

	if (!timer.isActive()) {
	    total_signals = 0;
	    break;
	}

	if (mSignal != ICD_DBUS_API_STATISTICS_SIG) {
	    continue;
	}

	if (mError.isEmpty()) {
  	    get_statistics_all_result(mArgs, result);
	    stats_results << result;
	    signals_left--;
	} else {
	    qWarning() << "Error:" << mError;
	    break;
	}
    }
    timer.stop();

    return total_signals;
}


static void get_addrinfo_all_result(QList<QVariant>& args,
				    IcdAddressInfoResult& ret)
{
    int i=0;

    if (args.isEmpty())
      return;

    ret.params.service_type = args[i++].toString();
    ret.params.service_attrs = args[i++].toUInt();
    ret.params.service_id = args[i++].toString();
    ret.params.network_type = args[i++].toString();
    ret.params.network_attrs = args[i++].toUInt();
    ret.params.network_id = args[i++].toByteArray();

    QVariantList vl = args[i].toList();
    QVariant reply = vl.first();
    QList<QVariant> lst = reply.toList();
    for (int k=0; k<lst.size()/6; k=k+6) {
        IcdIPInformation ip_info;
	ip_info.address = lst[k].toString();
	ip_info.netmask = lst[k++].toString();
	ip_info.default_gateway = lst[k++].toString();
	ip_info.dns1 = lst[k++].toString();
	ip_info.dns2 = lst[k++].toString();
	ip_info.dns3 = lst[k++].toString();

	ret.ip_info << ip_info;
    }
}


/* Special version of the addrinfo() call which does not call event loop.
 * Needed in order to fix NB#175098 where Qt4.7 webkit crashes because event
 * loop is run when webkit does not expect it. This function is called from
 * bearer management API syncStateWithInterface() in QNetworkSession
 * constructor.
 */
uint IcdPrivate::addrinfo(QList<IcdAddressInfoResult>& addr_results)
{
    QVariant reply;
    QVariantList vl;
    uint signals_left, total_signals;
    IcdAddressInfoResult result;
    time_t started;
    int timeout_secs = timeout / 1000;

    PDEBUG("%s\n", "addr_results");

    clearState();
    reply = mDBus->call(ICD_DBUS_API_ADDRINFO_REQ);
    if (reply.type() != QVariant::List)
        return 0;
    vl = reply.toList();
    if (vl.isEmpty())
        return 0;
    reply = vl.first();
    if (reply.type() != QVariant::UInt)
        return 0;
    signals_left = total_signals = reply.toUInt();
    if (!signals_left)
        return 0;

    started = time(0);
    addr_results.clear();
    while (signals_left) {
        mInterface.clear();
	while ((time(0)<=(started+timeout_secs)) && mInterface.isEmpty()) {
	    mDBus->synchronousDispatch(1000);
        QCoreApplication::sendPostedEvents(icd, QEvent::MetaCall);
	}

        if (time(0)>(started+timeout_secs)) {
	    total_signals = 0;
	    break;
	}

	if (mSignal != ICD_DBUS_API_ADDRINFO_SIG) {
	    continue;
	}

	if (mError.isEmpty()) {
  	    get_addrinfo_all_result(mArgs, result);
	    addr_results << result;
	    signals_left--;
	} else {
	    qWarning() << "Error:" << mError;
	    break;
	}
    }

    PDEBUG("total_signals=%d\n", total_signals);
    return total_signals;
}


uint IcdPrivate::addrinfo(QString& service_type, uint service_attrs,
			  QString& service_id, QString& network_type,
			  uint network_attrs, QByteArray& network_id,
			  IcdAddressInfoResult& addr_result)
{
    QTimer timer;
    QVariant reply;
    uint total_signals;
    QVariantList vl;

    clearState();

    reply = mDBus->call(ICD_DBUS_API_ADDRINFO_REQ,
			service_type, service_attrs, service_id,
			network_type, network_attrs, network_id);
    if (reply.type() != QVariant::List)
        return 0;
    vl = reply.toList();
    if (vl.isEmpty())
        return 0;
    reply = vl.first();
    total_signals = reply.toUInt();

    if (!total_signals)
        return 0;

    timer.setSingleShot(true);
    timer.start(timeout);

    mInterface.clear();
    while (timer.isActive() && mInterface.isEmpty()) {
        QCoreApplication::processEvents(QEventLoop::AllEvents, 1000);

	if (mSignal != ICD_DBUS_API_ADDRINFO_SIG) {
            mInterface.clear();
	    continue;
	}
    }

    timer.stop();

    if (mError.isEmpty()) {
        get_addrinfo_all_result(mArgs, addr_result);
    } else {
        qWarning() << "Error:" << mError;
    }

    // The returned value should be one because we asked for one addrinfo
    return total_signals;
}


Icd::Icd(QObject *parent)
  : QObject(parent), d(new IcdPrivate(this))
{
}

Icd::Icd(unsigned int timeout, QObject *parent)
  : QObject(parent), d(new IcdPrivate(timeout, this))
{
}

Icd::Icd(unsigned int timeout, IcdDbusInterfaceVer ver, QObject *parent)
  : QObject(parent), d(new IcdPrivate(timeout, ver, this))
{
}

Icd::~Icd()
{
    delete d;
}


QStringList Icd::scan(icd_scan_request_flags flags,
		      QStringList &network_types,
		      QList<IcdScanResult>& scan_results,
		      QString& error)
{
    return d->scan(flags, network_types, scan_results, error);
}


uint Icd::state(QString& service_type, uint service_attrs,
		QString& service_id, QString& network_type,
		uint network_attrs, QByteArray& network_id,
		IcdStateResult &state_result)
{
    return d->state(service_type, service_attrs, service_id,
		    network_type, network_attrs, network_id,
		    state_result);
}


uint Icd::addrinfo(QString& service_type, uint service_attrs,
		   QString& service_id, QString& network_type,
		   uint network_attrs, QByteArray& network_id,
		   IcdAddressInfoResult& addr_result)
{
    return d->addrinfo(service_type, service_attrs, service_id,
		       network_type, network_attrs, network_id,
		       addr_result);
}


uint Icd::state(QList<IcdStateResult>& state_results)
{
    return d->state(state_results);
}


uint Icd::statistics(QList<IcdStatisticsResult>& stats_results)
{
    return d->statistics(stats_results);
}


uint Icd::addrinfo(QList<IcdAddressInfoResult>& addr_results)
{
    return d->addrinfo(addr_results);
}


void Icd::icdSignalReceived(const QString& interface, 
			 const QString& signal,
			 const QList<QVariant>& args)
{
    d->signalReceived(interface, signal, args);
}


void Icd::icdCallReply(const QString& method, 
		    const QList<QVariant>& args,
		    const QString& error)
{
    d->callReply(method, args, error);
}

}  // Maemo namespace


