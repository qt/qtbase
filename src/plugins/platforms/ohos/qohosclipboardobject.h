// Copyright (C) 2025 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#ifndef QOHOSCLIPBOARDOBJECT_H
#define QOHOSCLIPBOARDOBJECT_H

#include <QtCore/private/qohoscommon_p.h>
#include <QtCore/qglobal.h>
#include <QtCore/qmimedata.h>
#include <database/pasteboard/oh_pasteboard.h>
#include <memory>
#include <qohosplugincore.h>

QT_BEGIN_NAMESPACE

class QOhosClipboardObject
{
public:
    enum class PasteboardDataSource
    {
        OurProcess,
        OtherProcess,
    };

    struct PasteboardData
    {
        QOhosOptional<PasteboardDataSource> dataSource;
        std::unique_ptr<QMimeData> lazyFetchingData;
    };

    static std::unique_ptr<QOhosClipboardObject> makeInstance(
        std::function<void(QOhosOptional<PasteboardDataSource>)> &&pasteboardUpdatesNotifier);

    PasteboardData getPasteboardDataWithLazyFetch();
    void setMimeDataSync(
        std::shared_ptr<QMimeData> mimeData, const QOhosOptional<bool> &shareInAppOnly);

protected:
    QOhosClipboardObject(std::function<void(QOhosOptional<PasteboardDataSource>)> &&pasteboardUpdatesNotifier);

private:
    std::shared_ptr<::OH_Pasteboard> m_pasteboard;
    std::shared_ptr<void> m_pasteboardDataChangedListenerHandle;
};

QT_END_NAMESPACE

#endif
