// Copyright (C) 2026 David Redondo <kde@david-redondo.de>
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#include "qxdgdesktopportalfiletransfer_p.h"

#include <QtCore/QFile>
#include <QtCore/QMap>
#include <QtCore/QUrl>
#include <QtCore/QVariant>
#include <QtDBus/QDBusConnection>
#include <QtDBus/QDBusMessage>
#include <QtDBus/QDBusReply>
#include <QtDBus/QDBusUnixFileDescriptor>

#include <QtCore/private/qcore_unix_p.h>

#include <unistd.h>
#include <fcntl.h>

QT_BEGIN_NAMESPACE

using namespace Qt::StringLiterals;

Q_LOGGING_CATEGORY(lcQpaPortalFileTransfer, "qt.qpa.portalfiletransfer")

namespace QXdgDesktopPortalFileTransfer {

constexpr auto portalService = "org.freedesktop.portal.Documents"_L1;
constexpr auto portalPath = "/org/freedesktop/portal/documents"_L1;
constexpr auto fileTransferInterface = "org.freedesktop.portal.FileTransfer"_L1;
constexpr int portalWaitTimeMs = 10;

QLatin1StringView fileTransferMimeType()
{
    return "application/vnd.portal.filetransfer"_L1;
}

QString exportFiles(const QList<QUrl> &urls)
{
    auto startMessage = QDBusMessage::createMethodCall(portalService, portalPath,
                                                       fileTransferInterface, "StartTransfer"_L1);
    const QVariantMap options{ { "writable"_L1, true }, { "autostop"_L1, true } };
    startMessage.setArguments({ options });
    const QDBusReply<QString> key = QDBusConnection::sessionBus().call(startMessage);
    if (!key.isValid()) {
        qCWarning(lcQpaPortalFileTransfer) << "failed to start transfer" << key.error();
        return QString();
    }
    QList<QDBusUnixFileDescriptor> fds;
    for (const auto &url : urls) {
        if (int fd = qt_safe_open(QFile::encodeName(url.toLocalFile()), O_PATH); fd >= 0)
            fds.emplaceBack().giveFileDescriptor(fd);
        else
            qCInfo(lcQpaPortalFileTransfer) << "failed to open" << url;
    }
    auto nextChunk = [&fds](auto lastEnd) {
        constexpr qsizetype chunkSize = 16;
        const auto remaining = fds.cend() - lastEnd;
        return std::pair{lastEnd, std::next(lastEnd, std::min(remaining, chunkSize))};
    };
    for (auto chunk = nextChunk(fds.cbegin()); chunk.first != fds.cend(); chunk = nextChunk(chunk.second)) {
        auto addFilesMessage = QDBusMessage::createMethodCall(portalService, portalPath, fileTransferInterface, "AddFiles"_L1);
        addFilesMessage.setArguments({key.value(), QVariant::fromValue(QList<QDBusUnixFileDescriptor>{chunk.first, chunk.second}), QVariantMap{}});
        const QDBusReply<void> addFilesResult = QDBusConnection::sessionBus().call(addFilesMessage, QDBus::Block, portalWaitTimeMs);
        if (!addFilesResult.isValid()) {
            qCWarning(lcQpaPortalFileTransfer) << "failed to add files" << addFilesResult.error();
            break;
        }
    }
    return key.value();
}

QList<QString> retrieveFiles(const QString &key)
{
    qCDebug(lcQpaPortalFileTransfer) << "retrieving files for" << key;
    auto message = QDBusMessage::createMethodCall(portalService, portalPath, fileTransferInterface,  "RetrieveFiles"_L1);
    message.setArguments({key, QVariantMap()});
    const QDBusReply<QStringList> reply = QDBusConnection::sessionBus().call(message, QDBus::Block, portalWaitTimeMs);
    if (!reply.isValid()) {
        qCDebug(lcQpaPortalFileTransfer) << "error retrieving files" << reply.error();
        return {};
    }
    const QStringList paths = reply.value();
    return paths;
}

} // namespace QXdgDesktopPortalFileTransfer

QT_END_NAMESPACE
