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

#include <QtCore/QMimeData>
#include <QtCore/QStringList>
#include <QDBusInterface>
#include <QDBusPendingCallWatcher>
#include <QDBusPendingReply>

// FIXME(loicm) The clipboard data format is not defined by Ubuntu Platform API
//     which makes it impossible to have non-Qt applications communicate with Qt
//     applications through the clipboard API. The solution would be to have
//     Ubuntu Platform define the data format or propose an API that supports
//     embedding different mime types in the clipboard.

// Data format:
//   number of mime types      (sizeof(int))
//   data layout               ((4 * sizeof(int)) * number of mime types)
//     mime type string offset (sizeof(int))
//     mime type string size   (sizeof(int))
//     data offset             (sizeof(int))
//     data size               (sizeof(int))
//   data                      (n bytes)

namespace {

const int maxFormatsCount = 16;
const int maxBufferSize = 4 * 1024 * 1024;  // 4 Mb

}

QMirClientClipboard::QMirClientClipboard()
    : mMimeData(new QMimeData)
    , mIsOutdated(true)
    , mUpdatesDisabled(false)
    , mDBusSetupDone(false)
{
}

QMirClientClipboard::~QMirClientClipboard()
{
    delete mMimeData;
}

void QMirClientClipboard::requestDBusClipboardContents()
{
    if (!mDBusSetupDone) {
        setupDBus();
    }

    if (!mPendingGetContentsCall.isNull())
        return;

    QDBusPendingCall pendingCall = mDBusClipboard->asyncCall(QStringLiteral("GetContents"));

    mPendingGetContentsCall = new QDBusPendingCallWatcher(pendingCall, this);

    QObject::connect(mPendingGetContentsCall.data(), &QDBusPendingCallWatcher::finished,
                     this, &QMirClientClipboard::onDBusClipboardGetContentsFinished);
}

void QMirClientClipboard::onDBusClipboardGetContentsFinished(QDBusPendingCallWatcher* call)
{
    Q_ASSERT(call == mPendingGetContentsCall.data());

    QDBusPendingReply<QByteArray> reply = *call;
    if (Q_UNLIKELY(reply.isError())) {
        qCritical("QMirClientClipboard - Failed to get system clipboard contents via D-Bus. %s, %s",
                qPrintable(reply.error().name()), qPrintable(reply.error().message()));
        // TODO: Might try again later a number of times...
    } else {
        QByteArray serializedMimeData = reply.argumentAt<0>();
        updateMimeData(serializedMimeData);
    }
    call->deleteLater();
}

void QMirClientClipboard::onDBusClipboardSetContentsFinished(QDBusPendingCallWatcher *call)
{
    QDBusPendingReply<void> reply = *call;
    if (Q_UNLIKELY(reply.isError())) {
        qCritical("QMirClientClipboard - Failed to set the system clipboard contents via D-Bus. %s, %s",
                qPrintable(reply.error().name()), qPrintable(reply.error().message()));
        // TODO: Might try again later a number of times...
    }
    call->deleteLater();
}

void QMirClientClipboard::updateMimeData(const QByteArray &serializedMimeData)
{
    if (mUpdatesDisabled)
        return;

    QMimeData *newMimeData = deserializeMimeData(serializedMimeData);
    if (newMimeData) {
        delete mMimeData;
        mMimeData = newMimeData;
        mIsOutdated = false;
        emitChanged(QClipboard::Clipboard);
    } else {
        qWarning("QMirClientClipboard - Got invalid serialized mime data. Ignoring it.");
    }
}

void QMirClientClipboard::setupDBus()
{
    QDBusConnection dbusConnection = QDBusConnection::sessionBus();

    bool ok = dbusConnection.connect(
            QStringLiteral("com.canonical.QtMir"),
            QStringLiteral("/com/canonical/QtMir/Clipboard"),
            QStringLiteral("com.canonical.QtMir.Clipboard"),
            QStringLiteral("ContentsChanged"),
            this, SLOT(updateMimeData(QByteArray)));
    if (Q_UNLIKELY(!ok))
        qCritical("QMirClientClipboard - Failed to connect to ContentsChanged signal form the D-Bus system clipboard.");

    mDBusClipboard = new QDBusInterface(QStringLiteral("com.canonical.QtMir"),
            QStringLiteral("/com/canonical/QtMir/Clipboard"),
            QStringLiteral("com.canonical.QtMir.Clipboard"),
            dbusConnection);

    mDBusSetupDone = true;
}

QByteArray QMirClientClipboard::serializeMimeData(QMimeData *mimeData) const
{
    Q_ASSERT(mimeData != nullptr);

    const QStringList formats = mimeData->formats();
    const int formatCount = qMin(formats.size(), maxFormatsCount);
    const int headerSize = sizeof(int) + (formatCount * 4 * sizeof(int));
    int bufferSize = headerSize;

    for (int i = 0; i < formatCount; i++)
        bufferSize += formats[i].size() + mimeData->data(formats[i]).size();

    QByteArray serializedMimeData;
    if (bufferSize <= maxBufferSize) {
        // Serialize data.
        serializedMimeData.resize(bufferSize);
        {
            char *buffer = serializedMimeData.data();
            int* header = reinterpret_cast<int*>(serializedMimeData.data());
            int offset = headerSize;
            header[0] = formatCount;
            for (int i = 0; i < formatCount; i++) {
                const QByteArray data = mimeData->data(formats[i]);
                const int formatOffset = offset;
                const int formatSize = formats[i].size();
                const int dataOffset = offset + formatSize;
                const int dataSize = data.size();
                memcpy(&buffer[formatOffset], formats[i].toLatin1().data(), formatSize);
                memcpy(&buffer[dataOffset], data.data(), dataSize);
                header[i*4+1] = formatOffset;
                header[i*4+2] = formatSize;
                header[i*4+3] = dataOffset;
                header[i*4+4] = dataSize;
                offset += formatSize + dataSize;
            }
        }
    } else {
        qWarning("QMirClientClipboard: Not sending contents (%d bytes) to the global clipboard as it's"
                " bigger than the maximum allowed size of %d bytes", bufferSize, maxBufferSize);
    }

    return serializedMimeData;
}

QMimeData *QMirClientClipboard::deserializeMimeData(const QByteArray &serializedMimeData) const
{
    if (static_cast<std::size_t>(serializedMimeData.size()) < sizeof(int)) {
        // Data is invalid
        return nullptr;
    }

    QMimeData *mimeData = new QMimeData;

    const char* const buffer = serializedMimeData.constData();
    const int* const header = reinterpret_cast<const int*>(serializedMimeData.constData());

    const int count = qMin(header[0], maxFormatsCount);

    for (int i = 0; i < count; i++) {
        const int formatOffset = header[i*4+1];
        const int formatSize = header[i*4+2];
        const int dataOffset = header[i*4+3];
        const int dataSize = header[i*4+4];

        if (formatOffset + formatSize <= serializedMimeData.size()
                && dataOffset + dataSize <= serializedMimeData.size()) {

            QString mimeType = QString::fromLatin1(&buffer[formatOffset], formatSize);
            QByteArray mimeDataBytes(&buffer[dataOffset], dataSize);

            mimeData->setData(mimeType, mimeDataBytes);
        }
    }

    return mimeData;
}

QMimeData* QMirClientClipboard::mimeData(QClipboard::Mode mode)
{
    if (mode != QClipboard::Clipboard)
        return nullptr;

    if (mIsOutdated && mPendingGetContentsCall.isNull()) {
        requestDBusClipboardContents();
    }

    // Return whatever we have at the moment instead of blocking until we have something.
    //
    // This might be called during app startup just for the sake of checking if some
    // "Paste" UI control should be enabled or not.
    // We will emit QClipboard::changed() once we finally have something.
    return mMimeData;
}

void QMirClientClipboard::setMimeData(QMimeData* mimeData, QClipboard::Mode mode)
{
    if (mode != QClipboard::Clipboard)
        return;

    if (!mPendingGetContentsCall.isNull()) {
        // Ignore whatever comes from the system clipboard as we are going to change it anyway
        QObject::disconnect(mPendingGetContentsCall.data(), 0, this, 0);
        mUpdatesDisabled = true;
        mPendingGetContentsCall->waitForFinished();
        mUpdatesDisabled = false;
        delete mPendingGetContentsCall.data();
    }

    if (mimeData != nullptr) {
        QByteArray serializedMimeData = serializeMimeData(mimeData);
        if (!serializedMimeData.isEmpty()) {
            setDBusClipboardContents(serializedMimeData);
        }

        mMimeData = mimeData;
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

void QMirClientClipboard::setDBusClipboardContents(const QByteArray &clipboardContents)
{
    if (!mDBusSetupDone) {
        setupDBus();
    }

    if (!mPendingSetContentsCall.isNull()) {
        // Ignore any previous set call as we are going to overwrite it anyway
        QObject::disconnect(mPendingSetContentsCall.data(), 0, this, 0);
        mUpdatesDisabled = true;
        mPendingSetContentsCall->waitForFinished();
        mUpdatesDisabled = false;
        delete mPendingSetContentsCall.data();
    }

    QDBusPendingCall pendingCall = mDBusClipboard->asyncCall(QStringLiteral("SetContents"), clipboardContents);

    mPendingSetContentsCall = new QDBusPendingCallWatcher(pendingCall, this);

    QObject::connect(mPendingSetContentsCall.data(), &QDBusPendingCallWatcher::finished,
                     this, &QMirClientClipboard::onDBusClipboardSetContentsFinished);
}
