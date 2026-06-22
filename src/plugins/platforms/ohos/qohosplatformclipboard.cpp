// Copyright (C) 2025 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#include "qohosplatformclipboard.h"
#include "qohosjsmain.h"
#include "qohosplatformintegration.h"
#include <QtCore/qmetaobject.h>
#include <QtCore/qobject.h>
#include <QtCore/private/qohoscommon_p.h>
#include <QtCore/private/qohoslogger_p.h>
#include <qohosclipboardobject.h>
#include <qohosplugincore.h>
#include <functional>
#include <utility>

QT_BEGIN_NAMESPACE

namespace {

std::shared_ptr<QOhosClipboardObject> makeClipboardObjectInstance(
    std::function<void(QOhosOptional<QOhosClipboardObject::PasteboardDataSource>)> &&pasteboardUpdatesNotifier)
{
    return QOhosClipboardObject::makeInstance(std::move(pasteboardUpdatesNotifier));
}

}

QOhosOptional<bool> &QOhosPlatformClipboard::shareInAppOnlyFlagRef()
{
    static QOhosOptional<bool> shareInAppOnly;
    return shareInAppOnly;
}

QOhosPlatformClipboard::QOhosPlatformClipboard()
{
    m_clipboardObject = makeClipboardObjectInstance(
        [this](QOhosOptional<QOhosClipboardObject::PasteboardDataSource> dataSource) {
            if (dataSource == QOhosClipboardObject::PasteboardDataSource::OtherProcess
                || (!dataSource.has_value() && m_mimeData)) {
                m_mimeData = nullptr;
                m_mimeDataIsOurs = false;
            }
            emitChanged(QClipboard::Clipboard);
        });
}

QOhosPlatformClipboard::~QOhosPlatformClipboard() = default;

QMimeData *QOhosPlatformClipboard::mimeData(QClipboard::Mode mode)
{
    Q_UNUSED(mode);
    Q_ASSERT(supportsMode(mode));

    // FIXME:
    // as per client's request - try to fetch the clipboard content if the previous attempt resulted
    // in an empty QMimeData object (in this case: due to clipboard content being inaccessible for
    // an app with all windows inactive).
    if (!m_mimeDataIsOurs && (m_mimeData == nullptr || m_mimeData->formats().isEmpty()))
        m_mimeData = m_clipboardObject->getPasteboardDataWithLazyFetch().lazyFetchingData;

    return m_mimeData.get();
}

void QOhosPlatformClipboard::setMimeData(QMimeData *mimeData, QClipboard::Mode mode)
{
    Q_ASSERT(supportsMode(mode));

    if (supportsMode(mode)) {
        bool callerReusedMimeDataPointer = mimeData == m_mimeData.get();
        if (!callerReusedMimeDataPointer)
            m_mimeData = std::shared_ptr<QMimeData>(mimeData);
        m_mimeDataIsOurs = true;
        if (mimeData != nullptr) {
            m_clipboardObject->setMimeDataSync(m_mimeData, shareInAppOnlyFlagRef());
            // HACK: reset share option state for next to come UdmfData to cross-app + cross-device.
            shareInAppOnlyFlagRef() = makeEmptyQOhosOptional();
        } else {
            m_clipboardObject->setMimeDataSync(std::make_shared<QMimeData>(), {});
        }
    }
}

void QOhosPlatformClipboard::setInAppOnlyPasteboardShareOption(bool shareInAppOnly)
{
    shareInAppOnlyFlagRef() = shareInAppOnly;
}

bool QOhosPlatformClipboard::supportsMode(QClipboard::Mode mode) const
{
    return mode == QClipboard::Clipboard;
}

bool QOhosPlatformClipboard::ownsMode(QClipboard::Mode mode) const
{
    Q_UNUSED(mode);
    return false;
}

std::shared_ptr<QMimeData> QOhosPlatformClipboard::getPasteboardDataWithLazyFetchOrLocalIfOwner() const
{
    auto pasteboardData = m_clipboardObject->getPasteboardDataWithLazyFetch();

    if (!pasteboardData.dataSource.has_value())
        return std::make_shared<QMimeData>();

    return pasteboardData.dataSource == QOhosClipboardObject::PasteboardDataSource::OtherProcess
        ? std::move(pasteboardData.lazyFetchingData)
        : m_mimeData
            ? m_mimeData
            : std::make_shared<QMimeData>();
}

QT_END_NAMESPACE
