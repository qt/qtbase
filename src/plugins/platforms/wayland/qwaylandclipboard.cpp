// Copyright (C) 2016 The Qt Company Ltd.
// Copyright (C) 2024 Jie Liu <liujie01@kylinos.cn>
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only
// Qt-Security score:significant reason:default

#include "qwaylandclipboard_p.h"
#include "qwaylanddisplay_p.h"
#include "qwaylandinputdevice_p.h"
#include "qwaylanddatacontrolv1_p.h"
#include "qwaylanddataoffer_p.h"
#include "qwaylanddatasource_p.h"
#include "qwaylanddatadevice_p.h"
#if QT_CONFIG(wayland_client_primary_selection)
#include "qwaylandprimaryselectionv1_p.h"
#endif

QT_BEGIN_NAMESPACE

namespace QtWaylandClient {

QWaylandClipboard::QWaylandClipboard(QWaylandDisplay *display)
    : mDisplay(display)
{
    m_clientClipboard[QClipboard::Clipboard] = nullptr;
    m_clientClipboard[QClipboard::Selection] = nullptr;
}

QWaylandClipboard::~QWaylandClipboard()
{
    if (m_clientClipboard[QClipboard::Clipboard] != m_clientClipboard[QClipboard::Selection])
        delete m_clientClipboard[QClipboard::Clipboard];
    delete m_clientClipboard[QClipboard::Selection];
}

QMimeData *QWaylandClipboard::mimeData(QClipboard::Mode mode)
{
    auto *seat = mDisplay->currentInputDevice();
    if (!seat)
        return &m_emptyData;

    switch (mode) {
    case QClipboard::Clipboard:
        if (auto *dataControlDevice = seat->dataControlDevice()) {
            if (dataControlDevice->selectionSource())
                return m_clientClipboard[QClipboard::Clipboard];
            if (auto *offer = dataControlDevice->selectionOffer())
                return offer->mimeData();
        }
        if (auto *dataDevice = seat->dataDevice()) {
            if (dataDevice->selectionSource())
                return m_clientClipboard[QClipboard::Clipboard];
            if (auto *offer = dataDevice->selectionOffer())
                return offer->mimeData();
        }
        return &m_emptyData;
    case QClipboard::Selection:
        if (auto *dataControlDevice = seat->dataControlDevice()) {
            if (dataControlDevice->primarySelectionSource())
                return m_clientClipboard[QClipboard::Selection];
            if (auto *offer = dataControlDevice->primarySelectionOffer())
                return offer->mimeData();
        }
#if QT_CONFIG(wayland_client_primary_selection)
        if (auto *selectionDevice = seat->primarySelectionDevice()) {
            if (selectionDevice->selectionSource())
                return m_clientClipboard[QClipboard::Selection];
            if (auto *offer = selectionDevice->selectionOffer())
                return offer->mimeData();
        }
#endif
        return &m_emptyData;
    default:
        return &m_emptyData;
    }
}

void QWaylandClipboard::setMimeData(QMimeData *data, QClipboard::Mode mode)
{
    auto *seat = mDisplay->currentInputDevice();
    if (!seat) {
        qCWarning(lcQpaWayland) << "Can't set clipboard contents with no wl_seats available";
        return;
    }

    if (data && m_clientClipboard[mode] == data) // Already set before?
        return;

    static const QString plain = QStringLiteral("text/plain");
    static const QString utf8 = QStringLiteral("text/plain;charset=utf-8");

    if (data && data->hasFormat(plain) && !data->hasFormat(utf8))
        data->setData(utf8, data->data(plain));

    auto oldMimeData = std::exchange(m_clientClipboard[mode], data);
    const auto otherMode = mode == QClipboard::Clipboard ? QClipboard::Selection
                                                         : QClipboard::Clipboard;
    if (oldMimeData != m_clientClipboard[otherMode])
        delete oldMimeData;

    switch (mode) {
    case QClipboard::Clipboard:
        if (auto *dataControlDevice = seat->dataControlDevice()) {
            dataControlDevice->setSelectionSource(data ? new QWaylandDataControlSourceV1(mDisplay->dataControlManager(),
                                                                                         m_clientClipboard[QClipboard::Clipboard]) : nullptr);
            emitChanged(mode);
        } else if (auto *dataDevice = seat->dataDevice()) {
            dataDevice->setSelectionSource(data ? new QWaylandDataSource(mDisplay->dndSelectionHandler(),
                                                                         m_clientClipboard[QClipboard::Clipboard]) : nullptr);
            emitChanged(mode);
        }
        break;
    case QClipboard::Selection:
        if (auto *dataControlDevice = seat->dataControlDevice()) {
            dataControlDevice->setPrimarySelectionSource(data ? new QWaylandDataControlSourceV1(mDisplay->dataControlManager(),
                                                                                                m_clientClipboard[QClipboard::Selection]) : nullptr);
            emitChanged(mode);
#if QT_CONFIG(wayland_client_primary_selection)
        } else if (auto *selectionDevice = seat->primarySelectionDevice()) {
            selectionDevice->setSelectionSource(data ? new QWaylandPrimarySelectionSourceV1(mDisplay->primarySelectionManager(),
                                                                                            m_clientClipboard[QClipboard::Selection]) : nullptr);
            emitChanged(mode);
#endif
        }
        break;
    default:
        break;
    }
}

bool QWaylandClipboard::supportsMode(QClipboard::Mode mode) const
{
    if (mode == QClipboard::Selection) {
        auto *seat = mDisplay->currentInputDevice();
        if (!seat)
            return false;
        if (seat->dataControlDevice())
            return true;
#if QT_CONFIG(wayland_client_primary_selection)
        if (seat->primarySelectionDevice())
            return true;
#endif
        return false;
    }
    return mode == QClipboard::Clipboard;
}

bool QWaylandClipboard::ownsMode(QClipboard::Mode mode) const
{
    QWaylandInputDevice *seat = mDisplay->currentInputDevice();
    if (!seat)
        return false;

    switch (mode) {
    case QClipboard::Clipboard:
        if (seat->dataControlDevice() && seat->dataControlDevice()->selectionSource() != nullptr)
            return true;
        return seat->dataDevice() && seat->dataDevice()->selectionSource() != nullptr;
    case QClipboard::Selection:
        if (seat->dataControlDevice() && seat->dataControlDevice()->primarySelectionSource() != nullptr)
            return true;
#if QT_CONFIG(wayland_client_primary_selection)
        return seat->primarySelectionDevice() && seat->primarySelectionDevice()->selectionSource() != nullptr;
#endif
    default:
        return false;
    }
}

}

QT_END_NAMESPACE
