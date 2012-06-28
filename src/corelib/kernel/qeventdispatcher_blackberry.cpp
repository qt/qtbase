/****************************************************************************
**
** Copyright (C) 2012 Research In Motion
** Contact: http://www.qt-project.org/
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
**
** $QT_END_LICENSE$
**
****************************************************************************/

#include "qeventdispatcher_blackberry_p.h"
#include "qsocketnotifier.h"
#include "qdebug.h"

#include <bps/bps.h>
#include <bps/event.h>

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
    // decode callback payload
    bpsIOHandlerData *ioData = static_cast<bpsIOHandlerData*>(data);

    // check if first file is ready
    bool firstReady = (ioData->count == 0);

    // update ready state of file
    if (io_events & BPS_IO_INPUT) {
        FD_SET(fd, ioData->readfds);
        ioData->count++;
    }

    if (io_events & BPS_IO_OUTPUT) {
        FD_SET(fd, ioData->writefds);
        ioData->count++;
    }

    if (io_events & BPS_IO_EXCEPT) {
        FD_SET(fd, ioData->exceptfds);
        ioData->count++;
    }

    // force bps_get_event() to return immediately by posting an event to ourselves;
    // but this only needs to happen once if multiple files become ready at the same time
    if (firstReady) {
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

    // \TODO Reinstate this when bps is fixed. See comment in select() below.
    // Register thread_pipe[0] with bps
    /*
    int io_events = BPS_IO_INPUT;
    result = bps_add_fd(thread_pipe[0], io_events, &bpsIOHandler, ioData.data());
    if (result != BPS_SUCCESS)
        qWarning() << Q_FUNC_INFO << "bps_add_fd() failed";
    */
}

QEventDispatcherBlackberryPrivate::~QEventDispatcherBlackberryPrivate()
{
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

    int io_events = ioEvents(sockfd);

    if (io_events)
        bps_remove_fd(sockfd);

    // Call the base Unix implementation. Needed to allow select() to be called correctly
    QEventDispatcherUNIX::registerSocketNotifier(notifier);

    switch (type) {
    case QSocketNotifier::Read:
        io_events |= BPS_IO_INPUT;
        break;
    case QSocketNotifier::Write:
        io_events |= BPS_IO_OUTPUT;
        break;
    case QSocketNotifier::Exception:
    default:
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

    // prepare file sets for bps callback
    Q_D(QEventDispatcherBlackberry);
    d->ioData->count = 0;
    d->ioData->readfds = readfds;
    d->ioData->writefds = writefds;
    d->ioData->exceptfds = exceptfds;

    // \TODO Remove this when bps is fixed
    //
    // Work around a bug in BPS with which if we register the thread_pipe[0] fd with bps in the
    // private class' ctor once only then we get spurious notifications that thread_pipe[0] is
    // ready for reading. The first time the notification is correct and the pipe is emptied in
    // the calling doSelect() function. The 2nd notification is an error and the resulting attempt
    // to read and call to wakeUps.testAndSetRelease(1, 0) fails as there has been no intervening
    // call to QEventDispatcherUNIX::wakeUp().
    //
    // Registering thread_pipe[0] here and unregistering it at the end of this call works around
    // this issue.
    int io_events = BPS_IO_INPUT;
    int result = bps_add_fd(d->thread_pipe[0], io_events, &bpsIOHandler, d->ioData.data());
    if (result != BPS_SUCCESS)
        qWarning() << Q_FUNC_INFO << "bps_add_fd() failed";

    // reset all file sets
    if (readfds)
        FD_ZERO(readfds);

    if (writefds)
        FD_ZERO(writefds);

    if (exceptfds)
        FD_ZERO(exceptfds);

    // convert timeout to milliseconds
    int timeout_ms = -1;
    if (timeout)
        timeout_ms = (timeout->tv_sec * 1000) + (timeout->tv_usec / 1000);

    // wait for event or file to be ready
    bps_event_t *event = NULL;
    result = bps_get_event(&event, timeout_ms);
    if (result != BPS_SUCCESS)
        qWarning("QEventDispatcherBlackberry::select: bps_get_event() failed");

    // pass all received events through filter - except IO ready events
    if (event && bps_event_get_domain(event) != bpsIOReadyDomain)
        filterEvent((void*)event);

    // \TODO Remove this when bps is fixed (see comment above)
    result = bps_remove_fd(d->thread_pipe[0]);
    if (result != BPS_SUCCESS)
        qWarning() << Q_FUNC_INFO << "bps_remove_fd() failed";

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
