// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only
// Qt-Security score:significant reason:default

#include "qwaylanddatasource_p.h"
#include "qwaylanddataoffer_p.h"
#include "qwaylanddatadevicemanager_p.h"

#include "qwaylandinputdevice_p.h"
#include "qwaylandmimehelper_p.h"
#include "qwaylandpipewritehelper_p.h"
#include <QtCore/QFile>

#include <QtCore/QDebug>
#if QT_CONFIG(xdg_desktop_portal_file_transfer)
#include <QtGui/private/qxdgdesktopportalfiletransfer_p.h>
#endif

#include <algorithm>
#include <unistd.h>
#include <signal.h>
#include <fcntl.h>

QT_BEGIN_NAMESPACE
using namespace std::chrono;

using namespace Qt::StringLiterals;

namespace QtWaylandClient {

QWaylandDataSource::QWaylandDataSource(QWaylandDataDeviceManager *dataDeviceManager, QMimeData *mimeData)
    : QtWayland::wl_data_source(dataDeviceManager->create_data_source())
    , m_mime_data(mimeData)
{
    if (!mimeData)
        return;
    const auto formats = QInternalMimeData::formatsHelper(mimeData);
    for (const QString &format : formats) {
        offer(format);
    }
#if QT_CONFIG(xdg_desktop_portal_file_transfer)
    if (mimeData->hasUrls() && !formats.contains(QXdgDesktopPortalFileTransfer::fileTransferMimeType())) {
        const auto urls = mimeData->urls();
        if (std::any_of(urls.begin(), urls.end(), std::mem_fn(&QUrl::isLocalFile))) {
            offer(QXdgDesktopPortalFileTransfer::fileTransferMimeType());
        }
    }
#endif
}

QWaylandDataSource::~QWaylandDataSource()
{
    destroy();
}

void QWaylandDataSource::data_source_cancelled()
{
    Q_EMIT cancelled();
}

void QWaylandDataSource::data_source_send(const QString &mime_type, int32_t fd)
{
    QByteArray content = QWaylandMimeHelper::getByteArray(m_mime_data, mime_type);
#if QT_CONFIG(xdg_desktop_portal_file_transfer)
    if (content.isEmpty() && mime_type == QXdgDesktopPortalFileTransfer::fileTransferMimeType()) {
        QList<QUrl> urls = m_mime_data->urls();
        urls.removeIf(std::not_fn(std::mem_fn(&QUrl::isLocalFile)));
        if (!urls.empty())
            content = QXdgDesktopPortalFileTransfer::exportFiles(urls).toUtf8();
    }
#endif
    if (!content.isEmpty()) {
        switch (QWaylandPipeWriteHelper::safeWriteWithTimeout(fd, content.constData(), content.size(), PIPE_BUF, 5s)) {
            case QWaylandPipeWriteHelper::SafeWriteResult::Ok:
                break;
            case QWaylandPipeWriteHelper::SafeWriteResult::Timeout:
                qWarning() << "QWaylandDataSource: timeout writing to pipe";
                break;
            case QWaylandPipeWriteHelper::SafeWriteResult::Closed:
                qWarning() << "QWaylandDataSource: peer closed pipe";
                break;
            case QWaylandPipeWriteHelper::SafeWriteResult::Error:
                qWarning() << "QWaylandDataSource: write() failed";
                break;
        }
    }
    close(fd);
}

void QWaylandDataSource::data_source_target(const QString &mime_type)
{
    m_accepted = !mime_type.isEmpty();
    Q_EMIT dndResponseUpdated(m_accepted, m_dropAction);
}

void QWaylandDataSource::data_source_action(uint32_t action)
{
    Qt::DropAction qtAction = Qt::IgnoreAction;

    if (action == WL_DATA_DEVICE_MANAGER_DND_ACTION_MOVE)
        qtAction = Qt::MoveAction;
    else if (action == WL_DATA_DEVICE_MANAGER_DND_ACTION_COPY)
        qtAction = Qt::CopyAction;

    m_dropAction = qtAction;
    Q_EMIT dndResponseUpdated(m_accepted, m_dropAction);
}

void QWaylandDataSource::data_source_dnd_finished()
{
    Q_EMIT finished();
}

void QWaylandDataSource::data_source_dnd_drop_performed()
{

    Q_EMIT dndDropped(m_accepted, m_dropAction);
}

}

QT_END_NAMESPACE

#include "moc_qwaylanddatasource_p.cpp"
