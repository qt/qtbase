// Copyright (C) 2022 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only

#include "qwasmdom.h"

#include <QtCore/qdebug.h>
#include <QtCore/qdir.h>
#include <QtCore/qfile.h>
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

DataTransfer::DataTransfer(emscripten::val webDataTransfer)
    : webDataTransfer(webDataTransfer) {
}

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

// Converts a DataTransfer instance to a QMimeData instance. Invokes the
// given callback when the conversion is complete. The callback takes ownership
// of the QMimeData.
void DataTransfer::toMimeDataWithFile(std::function<void(QMimeData *)> callback)
{
    enum class ItemKind {
        File,
        String,
    };

    class MimeContext {

    public:
        MimeContext(int itemCount, std::function<void(QMimeData *)> callback)
        :m_remainingItemCount(itemCount), m_callback(callback)
        {

        }

        void deref() {
            if (--m_remainingItemCount > 0)
                return;

            QList<QUrl> allUrls;
            allUrls.append(mimeData->urls());
            allUrls.append(fileUrls);
            mimeData->setUrls(allUrls);

            m_callback(mimeData);

            // Delete files; we expect that the user callback reads/copies
            // file content before returning.
            // Fixme: tie file lifetime to lifetime of the QMimeData?
            for (QUrl fileUrl: fileUrls)
                QFile(fileUrl.toLocalFile()).remove();

            delete this;
        }

        QMimeData *mimeData = new QMimeData();
        QList<QUrl> fileUrls;

    private:
        int m_remainingItemCount;
        std::function<void(QMimeData *)> m_callback;
    };

    const auto items = webDataTransfer["items"];
    const int itemCount = items["length"].as<int>();
    const int fileCount = webDataTransfer["files"]["length"].as<int>();
    MimeContext *mimeContext = new MimeContext(itemCount, callback);

    for (int i = 0; i < itemCount; ++i) {
        const auto item = items[i];
        const auto itemKind =
                item["kind"].as<std::string>() == "string" ? ItemKind::String : ItemKind::File;
        const auto itemMimeType = QString::fromStdString(item["type"].as<std::string>());

        switch (itemKind) {
        case ItemKind::File: {
            qstdweb::File webfile(item.call<emscripten::val>("getAsFile"));

            if (webfile.size() > 1e+9) { // limit file size to 1 GB
                qWarning() << "File is too large (> 1GB) and will be skipped. File size is" << webfile.size();
                mimeContext->deref();
                continue;
            }

            QString mimeFormat = QString::fromStdString(webfile.type());
            QString fileName = QString::fromStdString(webfile.name());

            // there's a file, now read it
            QByteArray fileContent(webfile.size(), Qt::Uninitialized);
            webfile.stream(fileContent.data(), [=]() {
                
                // If we get a single file, and that file is an image, then
                // try to decode the image data. This handles the case where
                // image data (i.e. not an image file) is pasted. The browsers
                // will then create a fake "image.png" file which has the image
                // data. As a side effect Qt will also decode the image for 
                // single-image-file drops, since there is no way to differentiate
                // the fake "image.png" from a real one.
                if (fileCount == 1 && mimeFormat.contains("image/")) {
                    QImage image;
                    if (image.loadFromData(fileContent))
                        mimeContext->mimeData->setImageData(image);
                }

                QDir qtTmpDir("/qt/tmp/"); // "tmp": indicate that these files won't stay around
                qtTmpDir.mkpath(qtTmpDir.path());

                QUrl fileUrl = QUrl::fromLocalFile(qtTmpDir.filePath(QString::fromStdString(webfile.name())));
                mimeContext->fileUrls.append(fileUrl);

                QFile file(fileUrl.toLocalFile());
                if (!file.open(QFile::WriteOnly)) {
                    qWarning() << "File was not opened";
                    mimeContext->deref();
                    return;
                }
                if (file.write(fileContent) < 0)
                    qWarning() << "Write failed";
                file.close();
                mimeContext->deref();
            });
            break;
        }
        case ItemKind::String:
            if (itemMimeType.contains("STRING", Qt::CaseSensitive)
                || itemMimeType.contains("TEXT", Qt::CaseSensitive)) {
                mimeContext->deref();
                break;
            }
            QString a;
            QString data = QString::fromEcmaString(webDataTransfer.call<emscripten::val>(
                    "getData", emscripten::val(itemMimeType.toStdString())));

            if (!data.isEmpty()) {
                if (itemMimeType == "text/html")
                    mimeContext->mimeData->setHtml(data);
                else if (itemMimeType.isEmpty() || itemMimeType == "text/plain")
                    mimeContext->mimeData->setText(data); // the type can be empty
                else if (itemMimeType.isEmpty() || itemMimeType == "text/uri-list") {
                    QList<QUrl> urls;
                    urls.append(data);
                    mimeContext->mimeData->setUrls(urls);
                } else {
                    // TODO improve encoding
                    if (data.startsWith("QB64")) {
                        data.remove(0, 4);
                        mimeContext->mimeData->setData(itemMimeType,
                                                  QByteArray::fromBase64(QByteArray::fromStdString(
                                                          data.toStdString())));
                    } else {
                        mimeContext->mimeData->setData(itemMimeType, data.toLocal8Bit());
                    }
                }
            }
            mimeContext->deref();
            break;
        }
    } // for items
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
