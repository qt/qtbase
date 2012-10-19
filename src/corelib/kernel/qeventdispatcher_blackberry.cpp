/****************************************************************************
**
** Copyright (C) 2012 Research In Motion
** Contact: http://www.qt-project.org/legal
**
** This file is part of the QtCore module of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:LGPL$
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and Digia.  For licensing terms and
** conditions see http://qt.digia.com/licensing.  For further information
** use the contact form at http://qt.digia.com/contact-us.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 2.1 as published by the Free Software
** Foundation and appearing in the file LICENSE.LGPL included in the
** packaging of this file.  Please review the following information to
** ensure the GNU Lesser General Public License version 2.1 requirements
** will be met: http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, Digia gives you certain additional
** rights.  These rights are described in the Digia Qt LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
** GNU General Public License Usage
** Alternatively, this file may be used under the terms of the GNU
** General Public License version 3.0 as published by the Free Software
** Foundation and appearing in the file LICENSE.GPL included in the
** packaging of this file.  Please review the following information to
** ensure the GNU General Public License version 3.0 requirements will be
** met: http://www.gnu.org/copyleft/gpl.html.
**
**
** $QT_END_LICENSE$
**
****************************************************************************/

#include "qeventdispatcher_blackberry_p.h"
#include "qsocketnotifier.h"
#include "qdebug.h"
#include "qelapsedtimer.h"

#include <bps/bps.h>
#include <bps/event.h>

//#define QEVENTDISPATCHERBLACKBERRY_DEBUG

#ifdef QEVENTDISPATCHERBLACKBERRY_DEBUG
#define qEventDispatcherDebug qDebug() << QThread::currentThread()
#else
#define qEventDispatcherDebug QT_NO_QDEBUG_MACRO()
#endif

struct bpsIOHandlerData {
    bpsIOHandlerData()
        : count(0), readfds(0), writefds(0), exceptfds(0)
    {
    }

    int count;
    fd_set *readfds;
    fd_set *writefds;
    fd_set *exceptfds;
};

static int bpsIOReadyDomain = -1;

static int bpsIOHandler(int fd, int io_events, void *data)
{
    qEventDispatcherDebug << Q_FUNC_INFO;
    // decode callback payload
    bpsIOHandlerData *ioData = static_cast<bpsIOHandlerData*>(data);

    // check if first file is ready
    bool firstReady = (ioData->count == 0);

    // update ready state of file
    if (io_events & BPS_IO_INPUT) {
        qEventDispatcherDebug << fd << "ready for Read";
        FD_SET(fd, ioData->readfds);
        ioData->count++;
    }

    if (io_events & BPS_IO_OUTPUT) {
        qEventDispatcherDebug << fd << "ready for Write";
        FD_SET(fd, ioData->writefds);
        ioData->count++;
    }

    if (io_events & BPS_IO_EXCEPT) {
        qEventDispatcherDebug << fd << "ready for Exception";
        FD_SET(fd, ioData->exceptfds);
        ioData->count++;
    }

    // force bps_get_event() to return immediately by posting an event to ourselves;
    // but this only needs to happen once if multiple files become ready at the same time
    if (firstReady) {
        qEventDispatcherDebug << "Sending bpsIOReadyDomain event";
        // create IO ready event
        bps_event_t *event;
        int result = bps_event_create(&event, bpsIOReadyDomain, 0, NULL, NULL);
        if (result != BPS_SUCCESS) {
            qWarning("QEventDispatcherBlackberryPrivate::QEventDispatcherBlackberry: bps_event_create() failed");
            return BPS_FAILURE;
        }

        // post IO ready event to our thread
        result = bps_push_event(event);
        if (result != BPS_SUCCESS) {
            qWarning("QEventDispatcherBlackberryPrivate::QEventDispatcherBlackberry: bps_push_event() failed");
            bps_event_destroy(event);
            return BPS_FAILURE;
        }
    }

    return BPS_SUCCESS;
}

QEventDispatcherBlackberryPrivate::QEventDispatcherBlackberryPrivate()
    : ioData(new bpsIOHandlerData)
{
    // prepare to use BPS
    int result = bps_initialize();
    if (result != BPS_SUCCESS)
        qFatal("QEventDispatcherBlackberryPrivate::QEventDispatcherBlackberry: bps_initialize() failed");

    // get domain for IO ready events - ignoring race condition here for now
    if (bpsIOReadyDomain == -1) {
        bpsIOReadyDomain = bps_register_domain();
        if (bpsIOReadyDomain == -1)
            qWarning("QEventDispatcherBlackberryPrivate::QEventDispatcherBlackberry: bps_register_domain() failed");
    }

    // Register thread_pipe[0] with bps
    int io_events = BPS_IO_INPUT;
    result = bps_add_fd(thread_pipe[0], io_events, &bpsIOHandler, ioData.data());
    if (result != BPS_SUCCESS)
        qWarning() << Q_FUNC_INFO << "bps_add_fd() failed";
}

QEventDispatcherBlackberryPrivate::~QEventDispatcherBlackberryPrivate()
{
    // Unregister thread_pipe[0] from bps
    const int result = bps_remove_fd(thread_pipe[0]);
    if (result != BPS_SUCCESS)
        qWarning() << Q_FUNC_INFO << "bps_remove_fd() failed";

    // we're done using BPS
    bps_shutdown();
}

/////////////////////////////////////////////////////////////////////////////

QEventDispatcherBlackberry::QEventDispatcherBlackberry(QObject *parent)
    : QEventDispatcherUNIX(*new QEventDispatcherBlackberryPrivate, parent)
{
}

QEventDispatcherBlackberry::QEventDispatcherBlackberry(QEventDispatcherBlackberryPrivate &dd, QObject *parent)
    : QEventDispatcherUNIX(dd, parent)
{
}

QEventDispatcherBlackberry::~QEventDispatcherBlackberry()
{
}

void QEventDispatcherBlackberry::registerSocketNotifier(QSocketNotifier *notifier)
{
    Q_ASSERT(notifier);

    // Register the fd with bps
    int sockfd = notifier->socket();
    int type = notifier->type();
    qEventDispatcherDebug << Q_FUNC_INFO << "fd =" << sockfd;

    int io_events = ioEvents(sockfd);

    if (io_events)
        bps_remove_fd(sockfd);

    // Call the base Unix implementation. Needed to allow select() to be called correctly
    QEventDispatcherUNIX::registerSocketNotifier(notifier);

    switch (type) {
    case QSocketNotifier::Read:
        qEventDispatcherDebug << "Registering" << sockfd << "for Reads";
        io_events |= BPS_IO_INPUT;
        break;
    case QSocketNotifier::Write:
        qEventDispatcherDebug << "Registering" << sockfd << "for Writes";
        io_events |= BPS_IO_OUTPUT;
        break;
    case QSocketNotifier::Exception:
    default:
        qEventDispatcherDebug << "Registering" << sockfd << "for Exceptions";
        io_events |= BPS_IO_EXCEPT;
        break;
    }

    Q_D(QEventDispatcherBlackberry);

    errno = 0;
    int result = bps_add_fd(sockfd, io_events, &bpsIOHandler, d->ioData.data());

    if (result != BPS_SUCCESS)
        qWarning() << Q_FUNC_INFO << "bps_add_fd() failed" << strerror(errno) << "code:" << errno;
}

void QEventDispatcherBlackberry::unregisterSocketNotifier(QSocketNotifier *notifier)
{
    // Allow the base Unix implementation to unregister the fd too
    QEventDispatcherUNIX::unregisterSocketNotifier(notifier);

    // Unregister the fd with bps
    int sockfd = notifier->socket();
    qEventDispatcherDebug << Q_FUNC_INFO << "fd =" << sockfd;

    const int io_events = ioEvents(sockfd);

    int result = bps_remove_fd(sockfd);
    if (result != BPS_SUCCESS)
        qWarning() << Q_FUNC_INFO << "bps_remove_fd() failed" << sockfd;


    /* if no other socket notifier is
     * watching sockfd, our job ends here
     */
    if (!io_events)
        return;

    Q_D(QEventDispatcherBlackberry);

    errno = 0;
    result = bps_add_fd(sockfd, io_events, &bpsIOHandler, d->ioData.data());
    if (result != BPS_SUCCESS) {
        qWarning() << Q_FUNC_INFO << "bps_add_fd() failed" << strerror(errno) << "code:" << errno;
    }
}

int QEventDispatcherBlackberry::select(int nfds, fd_set *readfds, fd_set *writefds, fd_set *exceptfds,
                                       timeval *timeout)
{
    Q_UNUSED(nfds);

    // Make a note of the start time
    timeval startTime = qt_gettime();

    // prepare file sets for bps callback
    Q_D(QEventDispatcherBlackberry);
    d->ioData->count = 0;
    d->ioData->readfds = readfds;
    d->ioData->writefds = writefds;
    d->ioData->exceptfds = exceptfds;

    // reset all file sets
    if (readfds)
        FD_ZERO(readfds);

    if (writefds)
        FD_ZERO(writefds);

    if (exceptfds)
        FD_ZERO(exceptfds);

    // Convert timeout to milliseconds
    int timeoutTotal = -1;
    if (timeout)
        timeoutTotal = (timeout->tv_sec * 1000) + (timeout->tv_usec / 1000);

    int timeoutLeft = timeoutTotal;

    bps_event_t *event = 0;
    unsigned int eventCount = 0;

    // This loop exists such that we can drain the bps event queue of all native events
    // more efficiently than if we were to return control to Qt after each event. This
    // is important for handling touch events which can come in rapidly.
    forever {
        // Only emit the awake() and aboutToBlock() signals in the second iteration. For the
        // first iteration, the UNIX event dispatcher will have taken care of that already.
        // Also native events are actually processed one loop iteration after they were
        // retrieved with bps_get_event().

        // Filtering the native event should happen between the awake() and aboutToBlock()
        // signal emissions. The calls awake() - filterNativeEvent() - aboutToBlock() -
        // bps_get_event() need not to be interrupted by a break or return statement.
        if (eventCount > 0) {
            if (event) {
                emit awake();
                filterNativeEvent(QByteArrayLiteral("bps_event_t"), static_cast<void*>(event), 0);
                emit aboutToBlock();
            }

            // Update the timeout
            // Clock source is monotonic, so we can recalculate how much timeout is left
            if (timeoutTotal != -1) {
                timeval t2 = qt_gettime();
                timeoutLeft = timeoutTotal - ((t2.tv_sec * 1000 + t2.tv_usec / 1000)
                                              - (startTime.tv_sec * 1000 + startTime.tv_usec / 1000));
                if (timeoutLeft < 0)
                    timeoutLeft = 0;
            }
        }

        // Wait for event or file to be ready
        event = 0;
        const int result = bps_get_event(&event, timeoutLeft);
        if (result != BPS_SUCCESS)
            qWarning("QEventDispatcherBlackberry::select: bps_get_event() failed");

        if (!event)    // In case of !event, we break out of the loop to let Qt process the timers
            break;     // (since timeout has expired) and socket notifiers that are now ready.

        if (bps_event_get_domain(event) == bpsIOReadyDomain) {
            timeoutTotal = 0;   // in order to immediately drain the event queue of native events
            event = 0;          // (especially touch move events) we don't break out here
        }

        ++eventCount;

        // Make sure we are not trapped in this loop due to continuous native events
        // also we cannot recalculate the timeout without a monotonic clock as the time may have changed
        const unsigned int maximumEventCount = 12;
        if (Q_UNLIKELY((eventCount > maximumEventCount && timeoutLeft == 0)
                       || !QElapsedTimer::isMonotonic())) {
            if (event)
                filterNativeEvent(QByteArrayLiteral("bps_event_t"), static_cast<void*>(event), 0);
            break;
        }
    }

    // the number of bits set in the file sets
    return d->ioData->count;
}

int QEventDispatcherBlackberry::ioEvents(int fd)
{
    int io_events = 0;

    Q_D(QEventDispatcherBlackberry);

    if (FD_ISSET(fd, &d->sn_vec[0].enabled_fds))
        io_events |= BPS_IO_INPUT;

    if (FD_ISSET(fd, &d->sn_vec[1].enabled_fds))
        io_events |= BPS_IO_OUTPUT;

    if (FD_ISSET(fd, &d->sn_vec[2].enabled_fds))
        io_events |= BPS_IO_EXCEPT;

    return io_events;
}

QT_END_NAMESPACE
