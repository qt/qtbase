/****************************************************************************
**
** Copyright (C) 2016 Canonical, Ltd.
** Contact: https://www.qt.io/licensing/
**
** This file is part of the plugins of the Qt Toolkit.
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


#include "qmirclientclipboard.h"
#include "qmirclientlogging.h"
#include "qmirclientwindow.h"

#include <QDBusPendingCallWatcher>
#include <QGuiApplication>
#include <QSignalBlocker>
#include <QtCore/QMimeData>
#include <QtCore/QStringList>

// content-hub
#include <com/ubuntu/content/hub.h>

// get this cumbersome nested namespace out of the way
using namespace com::ubuntu::content;

QMirClientClipboard::QMirClientClipboard()
    : mMimeData(new QMimeData)
    , mContentHub(Hub::Client::instance())
{
    connect(mContentHub, &Hub::pasteboardChanged, this, [this]() {
        if (mClipboardState == QMirClientClipboard::SyncedClipboard) {
            mClipboardState = QMirClientClipboard::OutdatedClipboard;
            emitChanged(QClipboard::Clipboard);
        }
    });

    connect(qGuiApp, &QGuiApplication::applicationStateChanged,
        this, &QMirClientClipboard::onApplicationStateChanged);

    requestMimeData();
}

QMirClientClipboard::~QMirClientClipboard()
{
    delete mMimeData;
}

QMimeData* QMirClientClipboard::mimeData(QClipboard::Mode mode)
{
    if (mode != QClipboard::Clipboard)
        return nullptr;

    // Blocks dataChanged() signal from being emitted. Makes no sense to emit it from
    // inside the data getter.
    const QSignalBlocker blocker(this);

    if (mClipboardState == OutdatedClipboard) {
        updateMimeData();
    } else if (mClipboardState == SyncingClipboard) {
        mPasteReply->waitForFinished();
    }

    return mMimeData;
}

void QMirClientClipboard::setMimeData(QMimeData* mimeData, QClipboard::Mode mode)
{
    QWindow *focusWindow = QGuiApplication::focusWindow();
    if (focusWindow && mode == QClipboard::Clipboard && mimeData != nullptr) {
        QString surfaceId = static_cast<QMirClientWindow*>(focusWindow->handle())->persistentSurfaceId();

        QDBusPendingCall reply = mContentHub->createPaste(surfaceId, *mimeData);

        // Don't care whether it succeeded
        QDBusPendingCallWatcher *watcher = new QDBusPendingCallWatcher(reply, this);
        connect(watcher, &QDBusPendingCallWatcher::finished,
                watcher, &QObject::deleteLater);

        mMimeData = mimeData;
        mClipboardState = SyncedClipboard;
        emitChanged(QClipboard::Clipboard);
    }
}

bool QMirClientClipboard::supportsMode(QClipboard::Mode mode) const
{
    return mode == QClipboard::Clipboard;
}

bool QMirClientClipboard::ownsMode(QClipboard::Mode mode) const
{
    Q_UNUSED(mode);
    return false;
}

void QMirClientClipboard::onApplicationStateChanged(Qt::ApplicationState state)
{
    if (state == Qt::ApplicationActive) {
        // Only focused or active applications might be allowed to paste, so we probably
        // missed changes in the clipboard while we were hidden, inactive or, more importantly,
        // suspended.
        requestMimeData();
    }
}

void QMirClientClipboard::updateMimeData()
{
    if (qGuiApp->applicationState() != Qt::ApplicationActive) {
        // Don't even bother asking as content-hub would probably ignore our request (and should).
        return;
    }

    delete mMimeData;

    QWindow *focusWindow = QGuiApplication::focusWindow();
    if (focusWindow) {
        QString surfaceId = static_cast<QMirClientWindow*>(focusWindow->handle())->persistentSurfaceId();
        mMimeData = mContentHub->latestPaste(surfaceId);
        mClipboardState = SyncedClipboard;
        emitChanged(QClipboard::Clipboard);
    }
}

void QMirClientClipboard::requestMimeData()
{
    if (qGuiApp->applicationState() != Qt::ApplicationActive) {
        // Don't even bother asking as content-hub would probably ignore our request (and should).
        return;
    }

    QWindow *focusWindow = QGuiApplication::focusWindow();
    if (!focusWindow) {
        return;
    }

    QString surfaceId = static_cast<QMirClientWindow*>(focusWindow->handle())->persistentSurfaceId();
    QDBusPendingCall reply = mContentHub->requestLatestPaste(surfaceId);
    mClipboardState = SyncingClipboard;

    mPasteReply = new QDBusPendingCallWatcher(reply, this);
    connect(mPasteReply, &QDBusPendingCallWatcher::finished,
            this, [this]() {
        delete mMimeData;
        mMimeData = mContentHub->paste(*mPasteReply);
        mClipboardState = SyncedClipboard;
        mPasteReply->deleteLater();
        mPasteReply = nullptr;
        emitChanged(QClipboard::Clipboard);
    });
}
