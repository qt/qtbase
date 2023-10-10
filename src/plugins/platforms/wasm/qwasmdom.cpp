// Copyright (C) 2022 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only

#include "qwasmdom.h"

#include <QMimeData>
#include <QtCore/qpoint.h>
#include <QtCore/qrect.h>
#include <QtGui/qimage.h>
#include <private/qstdweb_p.h>
#include <QtCore/qurl.h>

#include <utility>
#include <emscripten/wire.h>

QT_BEGIN_NAMESPACE

namespace dom {
namespace {
std::string dropActionToDropEffect(Qt::DropAction action)
{
    switch (action) {
    case Qt::DropAction::CopyAction:
        return "copy";
    case Qt::DropAction::IgnoreAction:
        return "none";
    case Qt::DropAction::LinkAction:
        return "link";
    case Qt::DropAction::MoveAction:
    case Qt::DropAction::TargetMoveAction:
        return "move";
    case Qt::DropAction::ActionMask:
        Q_ASSERT(false);
        return "";
    }
}
} // namespace

DataTransfer::DataTransfer(emscripten::val webDataTransfer) : webDataTransfer(webDataTransfer) { }

DataTransfer::~DataTransfer() = default;

DataTransfer::DataTransfer(const DataTransfer &other) = default;

DataTransfer::DataTransfer(DataTransfer &&other) = default;

DataTransfer &DataTransfer::operator=(const DataTransfer &other) = default;

DataTransfer &DataTransfer::operator=(DataTransfer &&other) = default;

void DataTransfer::setDragImage(emscripten::val element, const QPoint &hotspot)
{
    webDataTransfer.call<void>("setDragImage", element, emscripten::val(hotspot.x()),
                               emscripten::val(hotspot.y()));
}

void DataTransfer::setData(std::string format, std::string data)
{
    webDataTransfer.call<void>("setData", emscripten::val(std::move(format)),
                               emscripten::val(std::move(data)));
}

void DataTransfer::setDropAction(Qt::DropAction action)
{
    webDataTransfer.set("dropEffect", emscripten::val(dropActionToDropEffect(action)));
}

void DataTransfer::setDataFromMimeData(const QMimeData &mimeData)
{
    for (const auto &format : mimeData.formats()) {
        auto data = mimeData.data(format);

        auto encoded = format.startsWith("text/")
                ? QString::fromLocal8Bit(data).toStdString()
                : "QB64" + QString::fromLocal8Bit(data.toBase64()).toStdString();

        setData(format.toStdString(), std::move(encoded));
    }
}

QMimeData *DataTransfer::toMimeDataWithFile()
{
    QMimeData *resultMimeData = new QMimeData(); // QScopedPointer?

    enum class ItemKind {
        File,
        String,
    };

    const auto items = webDataTransfer["items"];
    QList<QUrl> uriList;
    for (int i = 0; i < items["length"].as<int>(); ++i) {
        const auto item = items[i];
        const auto itemKind =
                item["kind"].as<std::string>() == "string" ? ItemKind::String : ItemKind::File;
        const auto itemMimeType = QString::fromStdString(item["type"].as<std::string>());

        switch (itemKind) {
        case ItemKind::File: {
            m_webFile = item.call<emscripten::val>("getAsFile");
            qstdweb::File webfile(m_webFile);
            QUrl fileUrl(QStringLiteral("file:///") + QString::fromStdString(webfile.name()));
            uriList.append(fileUrl);
            break;
        }
        case ItemKind::String:
            if (itemMimeType.contains("STRING", Qt::CaseSensitive)
                || itemMimeType.contains("TEXT", Qt::CaseSensitive)) {
                break;
            }
            QString a;
            QString data = QString::fromEcmaString(webDataTransfer.call<emscripten::val>(
                    "getData", emscripten::val(itemMimeType.toStdString())));

            if (!data.isEmpty()) {
                if (itemMimeType == "text/html")
                    resultMimeData->setHtml(data);
                else if (itemMimeType.isEmpty() || itemMimeType == "text/plain")
                    resultMimeData->setText(data); // the type can be empty
                else {
                    // TODO improve encoding
                    if (data.startsWith("QB64")) {
                        data.remove(0, 4);
                        resultMimeData->setData(itemMimeType,
                                                  QByteArray::fromBase64(QByteArray::fromStdString(
                                                          data.toStdString())));
                    } else {
                        resultMimeData->setData(itemMimeType, data.toLocal8Bit());
                    }
                }
            }
            break;
        }
    }
    if (!uriList.isEmpty())
        resultMimeData->setUrls(uriList);
    return resultMimeData;
}

QMimeData *DataTransfer::toMimeDataPreview()
{
    auto data = new QMimeData();

    QList<QUrl> uriList;
    for (int i = 0; i < webDataTransfer["items"]["length"].as<int>(); ++i) {
        const auto item = webDataTransfer["items"][i];
        if (item["kind"].as<std::string>() == "file") {
            uriList.append(QUrl("blob://placeholder"));
        } else {
            const auto itemMimeType = QString::fromStdString(item["type"].as<std::string>());
            data->setData(itemMimeType, QByteArray());
        }
    }
    data->setUrls(uriList);
    return data;
}

void syncCSSClassWith(emscripten::val element, std::string cssClassName, bool flag)
{
    if (flag) {
        element["classList"].call<void>("add", emscripten::val(std::move(cssClassName)));
        return;
    }

    element["classList"].call<void>("remove", emscripten::val(std::move(cssClassName)));
}

QPointF mapPoint(emscripten::val source, emscripten::val target, const QPointF &point)
{
    const auto sourceBoundingRect =
            QRectF::fromDOMRect(source.call<emscripten::val>("getBoundingClientRect"));
    const auto targetBoundingRect =
            QRectF::fromDOMRect(target.call<emscripten::val>("getBoundingClientRect"));

    const auto offset = sourceBoundingRect.topLeft() - targetBoundingRect.topLeft();
    return point + offset;
}

void drawImageToWebImageDataArray(const QImage &sourceImage, emscripten::val destinationImageData,
                                  const QRect &sourceRect)
{
    Q_ASSERT_X(destinationImageData["constructor"]["name"].as<std::string>() == "ImageData",
               Q_FUNC_INFO, "The destination should be an ImageData instance");

    constexpr int BytesPerColor = 4;
    if (sourceRect.width() == sourceImage.width()) {
        // Copy a contiguous chunk of memory
        // ...............
        // OOOOOOOOOOOOOOO
        // OOOOOOOOOOOOOOO -> image data
        // OOOOOOOOOOOOOOO
        // ...............
        auto imageMemory = emscripten::typed_memory_view(sourceRect.width() * sourceRect.height()
                                                                 * BytesPerColor,
                                                         sourceImage.constScanLine(sourceRect.y()));
        destinationImageData["data"].call<void>(
                "set", imageMemory, sourceRect.y() * sourceImage.width() * BytesPerColor);
    } else {
        // Go through the scanlines manually to set the individual lines in bulk. This is
        // marginally less performant than the above.
        // ...............
        // ...OOOOOOOOO... r = 0  -> image data
        // ...OOOOOOOOO... r = 1  -> image data
        // ...OOOOOOOOO... r = 2  -> image data
        // ...............
        for (int row = 0; row < sourceRect.height(); ++row) {
            auto scanlineMemory =
                    emscripten::typed_memory_view(sourceRect.width() * BytesPerColor,
                                                  sourceImage.constScanLine(row + sourceRect.y())
                                                          + BytesPerColor * sourceRect.x());
            destinationImageData["data"].call<void>("set", scanlineMemory,
                                                    (sourceRect.y() + row) * sourceImage.width()
                                                                    * BytesPerColor
                                                            + sourceRect.x() * BytesPerColor);
        }
    }
}

} // namespace dom

QT_END_NAMESPACE
