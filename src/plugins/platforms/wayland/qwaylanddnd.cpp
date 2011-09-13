/****************************************************************************
**
** Copyright (C) 2011 Nokia Corporation and/or its subsidiary(-ies).
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

#include "qwaylanddnd.h"
#include "qwaylandinputdevice.h"
#include <QStringList>
#include <QFile>
#include <QtGui/private/qdnd_p.h>
#include <QGuiApplication>
#include <QSocketNotifier>
#include <sys/time.h>
#include <errno.h>
#include <unistd.h>
#include <fcntl.h>
#include <QDebug>

#include <QPlatformCursor>
#include "qwaylandcursor.h"

class QWaylandDragWrapper
{
public:
    QWaylandDragWrapper(QWaylandDisplay *display, QMimeData *data);
    ~QWaylandDragWrapper();
    QMimeData *mimeData() const { return mData; }

private:
    static void target(void *data, wl_drag *drag, const char *mimeType);
    static void finish(void *data, wl_drag *drag, int fd);
    static void reject(void *data, wl_drag *drag);
    static const wl_drag_listener dragListener;

    QWaylandDisplay *mDisplay;
    wl_drag *mDrag;
    QMimeData *mData;
    QString mAcceptedType;
};

class QWaylandDragOfferWrapper
{
public:
    QWaylandDragOfferWrapper(QWaylandDisplay *display, QMimeData *data, uint32_t id);
    ~QWaylandDragOfferWrapper();

private:
    static void offer(void *data, struct wl_drag_offer *offer, const char *mimeType);
    static void pointerFocus(void *data, struct wl_drag_offer *offer, uint32_t time,
                             wl_surface *surface,
                             int32_t x, int32_t y,
                             int32_t surfaceX, int32_t surfaceY);
    static void motion(void *data, struct wl_drag_offer *offer, uint32_t time,
                             int32_t x, int32_t y,
                             int32_t surfaceX, int32_t surfaceY);
    static void drop(void *data, struct wl_drag_offer *offer);
    static const wl_drag_offer_listener dragOfferListener;

    void sendEventToWindow(struct wl_drag_offer *offer, uint32_t time,
                           wl_surface *surface, const QPoint &pos);

    QWaylandDisplay *mDisplay;
    QMimeData *mData;
    struct wl_drag_offer *mOffer;
    QMimeData mOfferedTypes; // no data in this one, just the formats
    wl_surface *mFocusSurface;
    bool mAccepted;
    QPoint mLastEventPos;
    friend class QWaylandDrag;
};

static QWaylandDrag *dnd = 0;

QWaylandDrag *QWaylandDrag::instance(QWaylandDisplay *display)
{
    if (!dnd)
        dnd = new QWaylandDrag(display);
    return dnd;
}

QWaylandDrag::QWaylandDrag(QWaylandDisplay *display)
    : mDisplay(display), mDropData(0), mCurrentDrag(0), mCurrentOffer(0)
{
    mDropData = new QMimeData;
}

QWaylandDrag::~QWaylandDrag()
{
    delete mCurrentDrag;
    delete mCurrentOffer;
    delete mDropData;
}

QMimeData *QWaylandDrag::platformDropData()
{
    return mDropData;
}

static void showDragPixmap(bool show)
{
    QCursor c(QDragManager::self()->object->pixmap());
    QList<QWeakPointer<QPlatformCursor> > cursors = QPlatformCursorPrivate::getInstances();
    foreach (QWeakPointer<QPlatformCursor> cursor, cursors)
        if (cursor)
            static_cast<QWaylandCursor *>(cursor.data())->setupPixmapCursor(show ? &c : 0);
}


QWaylandDragWrapper::QWaylandDragWrapper(QWaylandDisplay *display, QMimeData *data)
    : mDisplay(display), mDrag(0), mData(data)
{
    QWaylandWindow *w = mDisplay->inputDevices().at(0)->pointerFocus();
    if (!w) {
        qWarning("QWaylandDragWrapper: No window with pointer focus?!");
        return;
    }
    qDebug() << "QWaylandDragWrapper" << data->formats();
    struct wl_shell *shell = display->wl_shell();
    mDrag = wl_shell_create_drag(shell);
    wl_drag_add_listener(mDrag, &dragListener, this);
    foreach (const QString &format, data->formats())
        wl_drag_offer(mDrag, format.toLatin1().constData());
    struct timeval tv;
    gettimeofday(&tv, 0);
    wl_drag_activate(mDrag,
                     w->wl_surface(),
                     display->inputDevices().at(0)->wl_input_device(),
                     tv.tv_sec * 1000 + tv.tv_usec / 1000);
    showDragPixmap(true);
}

QWaylandDragWrapper::~QWaylandDragWrapper()
{
    QWaylandDrag *dragHandler = QWaylandDrag::instance(mDisplay);
    if (dragHandler->mCurrentDrag == this)
        dragHandler->mCurrentDrag = 0;
    wl_drag_destroy(mDrag);
}

const wl_drag_listener QWaylandDragWrapper::dragListener = {
    QWaylandDragWrapper::target,
    QWaylandDragWrapper::finish,
    QWaylandDragWrapper::reject
};

void QWaylandDragWrapper::target(void *data, wl_drag *drag, const char *mimeType)
{
    Q_UNUSED(drag);
    QWaylandDragWrapper *self = static_cast<QWaylandDragWrapper *>(data);
    self->mAcceptedType = mimeType ? QString::fromLatin1(mimeType) : QString();
    qDebug() << "target" << self->mAcceptedType;
    QDragManager *manager = QDragManager::self();
    if (mimeType)
        manager->global_accepted_action = manager->defaultAction(manager->possible_actions,
                                                                 QGuiApplication::keyboardModifiers());
    else
        manager->global_accepted_action = Qt::IgnoreAction;
}

void QWaylandDragWrapper::finish(void *data, wl_drag *drag, int fd)
{
    Q_UNUSED(drag);
    QWaylandDragWrapper *self = static_cast<QWaylandDragWrapper *>(data);
    qDebug() << "finish" << self->mAcceptedType;
    if (self->mAcceptedType.isEmpty())
        return; // no drag target was valid when the drag finished
    QByteArray content = self->mData->data(self->mAcceptedType);
    if (!content.isEmpty()) {
        QFile f;
        if (f.open(fd, QIODevice::WriteOnly))
            f.write(content);
    }
    close(fd);
    // Drag finished on source side with drop.

    QDragManager::self()->stopDrag();
    showDragPixmap(false);
    delete self;
    qDebug() << " *** DRAG OVER WITH DROP";
}

void QWaylandDragWrapper::reject(void *data, wl_drag *drag)
{
    Q_UNUSED(drag);
    QWaylandDragWrapper *self = static_cast<QWaylandDragWrapper *>(data);
    self->mAcceptedType = QString();
    qDebug() << "reject";
    QDragManager::self()->global_accepted_action = Qt::IgnoreAction;
}


QWaylandDragOfferWrapper::QWaylandDragOfferWrapper(QWaylandDisplay *display,
                                                   QMimeData *data,
                                                   uint32_t id)
    : mDisplay(display), mData(data), mOffer(0), mFocusSurface(0),
      mAccepted(false)
{
    mOffer = wl_drag_offer_create(mDisplay->wl_display(), id, 1);
    wl_drag_offer_add_listener(mOffer, &dragOfferListener, this);
}

QWaylandDragOfferWrapper::~QWaylandDragOfferWrapper()
{
    QWaylandDrag *dragHandler = QWaylandDrag::instance(mDisplay);
    if (dragHandler->mCurrentOffer == this)
        dragHandler->mCurrentOffer = 0;
    wl_drag_offer_destroy(mOffer);
}

const wl_drag_offer_listener QWaylandDragOfferWrapper::dragOfferListener = {
    QWaylandDragOfferWrapper::offer,
    QWaylandDragOfferWrapper::pointerFocus,
    QWaylandDragOfferWrapper::motion,
    QWaylandDragOfferWrapper::drop
};

void QWaylandDragOfferWrapper::offer(void *data, struct wl_drag_offer *offer, const char *mimeType)
{
    // Called for each type before pointerFocus.
    Q_UNUSED(offer);
    QWaylandDragOfferWrapper *self = static_cast<QWaylandDragOfferWrapper *>(data);
    self->mOfferedTypes.setData(QString::fromLatin1(mimeType), QByteArray());
}

void QWaylandDragOfferWrapper::pointerFocus(void *data, struct wl_drag_offer *offer, uint32_t time,
                                            wl_surface *surface,
                                            int32_t x, int32_t y,
                                            int32_t surfaceX, int32_t surfaceY)
{
    qDebug() << "pointerFocus" << surface << x << y << surfaceX << surfaceY;
    QWaylandDragOfferWrapper *self = static_cast<QWaylandDragOfferWrapper *>(data);
    QWaylandDrag *mgr = QWaylandDrag::instance(self->mDisplay);

    if (!surface) {
        if (self->mFocusSurface) {
            // This is a DragLeave.
            QWindow *window = static_cast<QWaylandWindow *>(
                wl_surface_get_user_data(self->mFocusSurface))->window();
            QWindowSystemInterface::handleDrag(window, 0, QPoint());
            if (self->mAccepted) {
                wl_drag_offer_reject(offer);
                self->mAccepted = false;
            }
            if (!mgr->mCurrentDrag) // no drag -> this is not the source side -> offer can be destroyed
                delete mgr->mCurrentOffer;
        } else {
            // Drag finished on source side without drop.
            QDragManager::self()->stopDrag();
            showDragPixmap(false);
            delete mgr->mCurrentDrag;
            qDebug() << " *** DRAG OVER WITHOUT DROP";
        }
    }

    self->mFocusSurface = surface;

    // This is a DragMove or DragEnter+DragMove.
    if (surface)
        self->sendEventToWindow(offer, time, surface, QPoint(surfaceX, surfaceY));
}

void QWaylandDragOfferWrapper::motion(void *data, struct wl_drag_offer *offer, uint32_t time,
                                      int32_t x, int32_t y,
                                      int32_t surfaceX, int32_t surfaceY)
{
    Q_UNUSED(x);
    Q_UNUSED(y);
    QWaylandDragOfferWrapper *self = static_cast<QWaylandDragOfferWrapper *>(data);
    if (!self->mFocusSurface)
        return;
//    qDebug() << "motion" << self->mFocusSurface << x << y << surfaceX << surfaceY;
    self->sendEventToWindow(offer, time, self->mFocusSurface, QPoint(surfaceX, surfaceY));
}

void QWaylandDragOfferWrapper::sendEventToWindow(struct wl_drag_offer *offer, uint32_t time,
                                                 wl_surface *surface, const QPoint &pos)
{
    QWindow *window = static_cast<QWaylandWindow *>(wl_surface_get_user_data(surface))->window();
    Qt::DropAction action = QWindowSystemInterface::handleDrag(window, &mOfferedTypes, pos);
    bool accepted = (action != Qt::IgnoreAction && !mOfferedTypes.formats().isEmpty());
    if (accepted != mAccepted) {
        mAccepted = accepted;
        if (mAccepted) {
            // What can we do, just accept the first type...
            QByteArray ba = mOfferedTypes.formats().first().toLatin1();
            qDebug() << "wl_drag_offer_accept" << ba;
            wl_drag_offer_accept(offer, time, ba.constData());
        } else {
            qDebug() << "wl_drag_offer_reject";
            wl_drag_offer_reject(offer);
        }
    }
    mLastEventPos = pos;
}

void QWaylandDragOfferWrapper::drop(void *data, struct wl_drag_offer *offer)
{
    QWaylandDragOfferWrapper *self = static_cast<QWaylandDragOfferWrapper *>(data);
    if (!self->mAccepted) {
        wl_drag_offer_reject(offer);
        return;
    }

    QWaylandDrag *mgr = QWaylandDrag::instance(self->mDisplay);
    QMimeData *mimeData = QWaylandDrag::instance(self->mDisplay)->platformDropData();
    mimeData->clear();
    if (mgr->mCurrentDrag) { // means this offer is the client's own
        QMimeData *localData = mgr->mCurrentDrag->mimeData();
        foreach (const QString &format, localData->formats())
            mimeData->setData(format, localData->data(format));
        QWindow *window = static_cast<QWaylandWindow *>(
            wl_surface_get_user_data(self->mFocusSurface))->window();
        QWindowSystemInterface::handleDrop(window, mimeData, self->mLastEventPos);
        // Drag finished with drop (source == target).
        QDragManager::self()->stopDrag();
        showDragPixmap(false);
        delete mgr->mCurrentOffer;
        qDebug() << " *** DRAG OVER WITH DROP, SOURCE == TARGET";
    } else {
        // ### TODO
        // This is a bit broken: The QMimeData will only contain the data for
        // the first type. The Wayland protocol and QDropEvents/QMimeData do not
        // match perfectly at the moment.
        QString format = self->mOfferedTypes.formats().first();
        QByteArray mimeTypeBa = format.toLatin1();
        int pipefd[2];
        if (pipe(pipefd) == -1) {
            qWarning("QWaylandDragOfferWrapper: pipe() failed");
            return;
        }
        fcntl(pipefd[0], F_SETFL, fcntl(pipefd[0], F_GETFL, 0) | O_NONBLOCK);
        wl_drag_offer_receive(offer, pipefd[1]);
        mgr->mPipeData.clear();
        mgr->mMimeFormat = format;
        mgr->mPipeWriteEnd = pipefd[1];
        mgr->mPipeWatcher = new QSocketNotifier(pipefd[0], QSocketNotifier::Read);
        QObject::connect(mgr->mPipeWatcher, SIGNAL(activated(int)), mgr, SLOT(pipeReadable(int)));
    }
}


void QWaylandDrag::pipeReadable(int fd)
{
    if (mPipeWriteEnd) {
        close(mPipeWriteEnd);
        mPipeWriteEnd = 0;
    }
    char buf[256];
    int n;
    while ((n = read(fd, &buf, sizeof buf)) > 0 || errno == EINTR)
        if (n > 0)
            mPipeData.append(buf, n);
    if (n == -1 && (errno == EAGAIN || errno == EWOULDBLOCK))
        return;
    delete mPipeWatcher;
    close(fd);

    QMimeData *mimeData = platformDropData();
    mimeData->setData(mMimeFormat, mPipeData);
    foreach (const QString &format, mimeData->formats())
        qDebug() << "  got type" << format << "with data" << mimeData->data(format);

    QWindow *window = static_cast<QWaylandWindow *>(
        wl_surface_get_user_data(mCurrentOffer->mFocusSurface))->window();
    QWindowSystemInterface::handleDrop(window, mimeData, mCurrentOffer->mLastEventPos);

    // Drag finished on target side with drop.
    delete mCurrentOffer;
    qDebug() << " *** DRAG OVER ON TARGET WITH DROP";
}

void QWaylandDrag::createDragOffer(uint32_t id)
{
    delete mCurrentOffer;
    mCurrentOffer = new QWaylandDragOfferWrapper(mDisplay, mDropData, id);
}

void QWaylandDrag::startDrag()
{
    QDragManager *manager = QDragManager::self();

    // No need for the traditional desktop-oriented event handling in QDragManager.
    manager->unmanageEvents();

    delete mCurrentDrag;
    mCurrentDrag = new QWaylandDragWrapper(mDisplay, manager->dropData());
}
