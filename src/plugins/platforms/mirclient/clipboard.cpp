/*
 * Copyright (C) 2014 Canonical, Ltd.
 *
 * This program is free software: you can redistribute it and/or modify it under
 * the terms of the GNU Lesser General Public License version 3, as published by
 * the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful, but WITHOUT
 * ANY WARRANTY; without even the implied warranties of MERCHANTABILITY,
 * SATISFACTORY QUALITY, or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#include "clipboard.h"

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

UbuntuClipboard::UbuntuClipboard()
    : mMimeData(new QMimeData)
    , mIsOutdated(true)
    , mUpdatesDisabled(false)
    , mDBusSetupDone(false)
{
}

UbuntuClipboard::~UbuntuClipboard()
{
    delete mMimeData;
}

void UbuntuClipboard::requestDBusClipboardContents()
{
    if (!mDBusSetupDone) {
        setupDBus();
    }

    if (!mPendingGetContentsCall.isNull())
        return;

    QDBusPendingCall pendingCall = mDBusClipboard->asyncCall("GetContents");

    mPendingGetContentsCall = new QDBusPendingCallWatcher(pendingCall, this);

    QObject::connect(mPendingGetContentsCall.data(), SIGNAL(finished(QDBusPendingCallWatcher*)),
                     this, SLOT(onDBusClipboardGetContentsFinished(QDBusPendingCallWatcher*)));
}

void UbuntuClipboard::onDBusClipboardGetContentsFinished(QDBusPendingCallWatcher* call)
{
    Q_ASSERT(call == mPendingGetContentsCall.data());

    QDBusPendingReply<QByteArray> reply = *call;
    if (reply.isError()) {
        qCritical("UbuntuClipboard - Failed to get system clipboard contents via D-Bus. %s, %s",
                qPrintable(reply.error().name()), qPrintable(reply.error().message()));
        // TODO: Might try again later a number of times...
    } else {
        QByteArray serializedMimeData = reply.argumentAt<0>();
        updateMimeData(serializedMimeData);
    }
    call->deleteLater();
}

void UbuntuClipboard::onDBusClipboardSetContentsFinished(QDBusPendingCallWatcher *call)
{
    QDBusPendingReply<void> reply = *call;
    if (reply.isError()) {
        qCritical("UbuntuClipboard - Failed to set the system clipboard contents via D-Bus. %s, %s",
                qPrintable(reply.error().name()), qPrintable(reply.error().message()));
        // TODO: Might try again later a number of times...
    }
    call->deleteLater();
}

void UbuntuClipboard::updateMimeData(const QByteArray &serializedMimeData)
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
        qWarning("UbuntuClipboard - Got invalid serialized mime data. Ignoring it.");
    }
}

void UbuntuClipboard::setupDBus()
{
    QDBusConnection dbusConnection = QDBusConnection::sessionBus();

    bool ok = dbusConnection.connect(
            "com.canonical.QtMir",
            "/com/canonical/QtMir/Clipboard",
            "com.canonical.QtMir.Clipboard",
            "ContentsChanged",
            this, SLOT(updateMimeData(QByteArray)));
    if (!ok) {
        qCritical("UbuntuClipboard - Failed to connect to ContentsChanged signal form the D-Bus system clipboard.");
    }

    mDBusClipboard = new QDBusInterface("com.canonical.QtMir",
            "/com/canonical/QtMir/Clipboard",
            "com.canonical.QtMir.Clipboard",
            dbusConnection);

    mDBusSetupDone = true;
}

QByteArray UbuntuClipboard::serializeMimeData(QMimeData *mimeData) const
{
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
                const int formatOffset = offset;
                const int formatSize = formats[i].size();
                const int dataOffset = offset + formatSize;
                const int dataSize = mimeData->data(formats[i]).size();
                memcpy(&buffer[formatOffset], formats[i].toLatin1().data(), formatSize);
                memcpy(&buffer[dataOffset], mimeData->data(formats[i]).data(), dataSize);
                header[i*4+1] = formatOffset;
                header[i*4+2] = formatSize;
                header[i*4+3] = dataOffset;
                header[i*4+4] = dataSize;
                offset += formatSize + dataSize;
            }
        }
    } else {
        qWarning("UbuntuClipboard: Not sending contents (%d bytes) to the global clipboard as it's"
                " bigger than the maximum allowed size of %d bytes", bufferSize, maxBufferSize);
    }

    return serializedMimeData;
}

QMimeData *UbuntuClipboard::deserializeMimeData(const QByteArray &serializedMimeData) const
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

QMimeData* UbuntuClipboard::mimeData(QClipboard::Mode mode)
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

void UbuntuClipboard::setMimeData(QMimeData* mimeData, QClipboard::Mode mode)
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

    QByteArray serializedMimeData = serializeMimeData(mimeData);
    if (!serializedMimeData.isEmpty()) {
        setDBusClipboardContents(serializedMimeData);
    }

    mMimeData = mimeData;
    emitChanged(QClipboard::Clipboard);
}

bool UbuntuClipboard::supportsMode(QClipboard::Mode mode) const
{
    return mode == QClipboard::Clipboard;
}

bool UbuntuClipboard::ownsMode(QClipboard::Mode mode) const
{
    Q_UNUSED(mode);
    return false;
}

void UbuntuClipboard::setDBusClipboardContents(const QByteArray &clipboardContents)
{
    if (!mPendingSetContentsCall.isNull()) {
        // Ignore any previous set call as we are going to overwrite it anyway
        QObject::disconnect(mPendingSetContentsCall.data(), 0, this, 0);
        mUpdatesDisabled = true;
        mPendingSetContentsCall->waitForFinished();
        mUpdatesDisabled = false;
        delete mPendingSetContentsCall.data();
    }

    QDBusPendingCall pendingCall = mDBusClipboard->asyncCall("SetContents", clipboardContents);

    mPendingSetContentsCall = new QDBusPendingCallWatcher(pendingCall, this);

    QObject::connect(mPendingSetContentsCall.data(), SIGNAL(finished(QDBusPendingCallWatcher*)),
                     this, SLOT(onDBusClipboardSetContentsFinished(QDBusPendingCallWatcher*)));
}
