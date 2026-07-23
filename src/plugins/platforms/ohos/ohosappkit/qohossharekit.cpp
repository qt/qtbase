// Copyright (C) 2026 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#include "qohossharekit.h"
#include "qohossharekitbackend_p.h"
#include <QtCore/private/qohoscommon_p.h>
#include <QtCore/private/qohoslogger_p.h>
#include <QtCore/qmimedatabase.h>
#include <QtGui/qwindow.h>

QT_BEGIN_NAMESPACE

namespace QtOhosAppKit {

using namespace Private;

/*!
    \namespace QtOhosAppKit::ShareKit
    \inmodule QtOhosAppKit
    \since 5.12.12
    \brief The ShareKit to expose Share Kit API.
*/
namespace ShareKit {

namespace {

constexpr const char *mimeTextUriList = "text/uri-list";

std::optional<QOhosShareKit::ShareAbilityType> tryMapShareAbilityTypeToShareKitEnum(
    ShareAbilityType abilityType)
{
    switch (abilityType) {
    case ShareAbilityType::CopyToPasteboard:
        return std::make_optional(QOhosShareKit::ShareAbilityType::COPY_TO_PASTEBOARD);
    case ShareAbilityType::SaveToMediaAsset:
        return std::make_optional(QOhosShareKit::ShareAbilityType::SAVE_TO_MEDIA_ASSET);
    case ShareAbilityType::SaveAsFile:
        return std::make_optional(QOhosShareKit::ShareAbilityType::SAVE_AS_FILE);
    case ShareAbilityType::Print:
        return std::make_optional(QOhosShareKit::ShareAbilityType::PRINT);
    case ShareAbilityType::SaveToSuperHub:
        return std::make_optional(QOhosShareKit::ShareAbilityType::SAVE_TO_SUPERHUB);
    }

    return {};
}

std::vector<QOhosShareKit::ShareAbilityType> mapExcludedAbilitiesToShareKit(
    const QList<ShareAbilityType> &excludedAbilities)
{
    std::vector<QOhosShareKit::ShareAbilityType> shareKitExcludedAbilities;
    for (auto excludedAbilityType : excludedAbilities) {
        auto optShareKitExcludedAbilityType = tryMapShareAbilityTypeToShareKitEnum(excludedAbilityType);
        if (optShareKitExcludedAbilityType.has_value())
            shareKitExcludedAbilities.push_back(optShareKitExcludedAbilityType.value());
    }
    return shareKitExcludedAbilities;
}

class SharedRecordImpl : public SharedRecord
{
public:
    SharedRecordImpl(const QMimeType &mimeType, const QString &content, bool urlContent);
    SharedRecordImpl(const QFileInfo &fileInfo);

    QMimeType mimeType() const override;
    QString content() const override;
    QString filePath() const override;
    bool isUrlContent() const override;

    void setTitle(const QString &title) override;
    QString title() const override;

    void setLabel(const QString &label) override;
    QString label() const override;

    void setDescription(const QString &description) override;
    QString description() const override;

    void setThumbnail(const QByteArray &thumbnail) override;
    QByteArray thumbnail() const override;

    void setThumbnailFilePath(const QString &thumbnailFilePath) override;
    QString thumbnailFilePath() const override;

    void setExtraData(const QVariantMap &extraData) override;
    QVariantMap extraData() const override;

private:
    QMimeType m_mimeType;
    std::optional<QString> m_content;
    std::optional<QString> m_filePath;
    bool m_urlContent;
    std::optional<QString> m_title;
    std::optional<QString> m_label;
    std::optional<QString> m_description;
    std::optional<QByteArray> m_thumbnail;
    std::optional<QString> m_thumbnailFilePath;
    std::optional<QVariantMap> m_extraData;
};

SharedRecordImpl::SharedRecordImpl(const QMimeType &mimeType, const QString &content, bool urlContent)
    : SharedRecord()
    , m_mimeType(mimeType)
    , m_content(content)
    , m_urlContent(urlContent)
{
}

SharedRecordImpl::SharedRecordImpl(const QFileInfo &fileInfo)
    : SharedRecord()
    , m_mimeType(QMimeDatabase().mimeTypeForFile(fileInfo))
    , m_filePath(fileInfo.absoluteFilePath())
    , m_urlContent(false)
{
}

QMimeType SharedRecordImpl::mimeType() const
{
    return m_mimeType;
}

QString SharedRecordImpl::content() const
{
    return m_content.value_or(QString());
}

QString SharedRecordImpl::filePath() const
{
    return m_filePath.value_or(QString());
}

bool SharedRecordImpl::isUrlContent() const
{
    return m_urlContent;
}

void SharedRecordImpl::setTitle(const QString &title)
{
    m_title = title;
}

QString SharedRecordImpl::title() const
{
    return m_title.value_or(QString());
}

void SharedRecordImpl::setLabel(const QString &label)
{
    m_label = label;
}

QString SharedRecordImpl::label() const
{
    return m_label.value_or(QString());
}

void SharedRecordImpl::setDescription(const QString &description)
{
    m_description = description;
}

QString SharedRecordImpl::description() const
{
    return m_description.value_or(QString());
}

void SharedRecordImpl::setThumbnail(const QByteArray &thumbnail)
{
    m_thumbnail = thumbnail;
}

QByteArray SharedRecordImpl::thumbnail() const
{
    return m_thumbnail.value_or(QByteArray());
}

void SharedRecordImpl::setThumbnailFilePath(const QString &thumbnailFilePath)
{
    m_thumbnailFilePath = thumbnailFilePath;
}

QString SharedRecordImpl::thumbnailFilePath() const
{
    return m_thumbnailFilePath.value_or(QString());
}

void SharedRecordImpl::setExtraData(const QVariantMap &extraData)
{
    m_extraData = extraData;
}

QVariantMap SharedRecordImpl::extraData() const
{
    return m_extraData.value_or(QVariantMap());
}

class ShareControllerOptionsImpl : public ShareControllerOptions
{
public:
    void setAnchorOffset(QPoint anchorOffset) override;
    void setAnchor(QRect anchor) override;
    std::optional<QPoint> anchorOffset() const;
    std::optional<QSize> anchorSize() const;

    void setSingleSelectionMode(bool singleSelectionMode) override;
    std::optional<bool> isSingleSelection() const;

    void setDefaultPreviewMode(bool defaultPreviewMode) override;
    std::optional<bool> isDefaultPreview() const;

    void setExcludedAbilities(const QList<ShareAbilityType> &excludedAbilities) override;
    std::optional<QList<ShareAbilityType>> excludedAbilities() const;

private:
    std::optional<QPoint> m_anchorOffset;
    std::optional<QSize> m_anchorSize;
    std::optional<bool> m_singleSelectionMode;
    std::optional<bool> m_defaultPreviewMode;
    std::optional<QList<ShareAbilityType>> m_excludedAbilities;
};

void ShareControllerOptionsImpl::setAnchorOffset(QPoint anchorOffset)
{
    m_anchorOffset = anchorOffset;
}

void ShareControllerOptionsImpl::setAnchor(QRect anchor)
{
    m_anchorOffset = anchor.topLeft();
    m_anchorSize = anchor.size();
}

std::optional<QPoint> ShareControllerOptionsImpl::anchorOffset() const
{
    return m_anchorOffset;
}

std::optional<QSize> ShareControllerOptionsImpl::anchorSize() const
{
    return m_anchorSize;
}

void ShareControllerOptionsImpl::setSingleSelectionMode(bool singleSelectionMode)
{
    m_singleSelectionMode = singleSelectionMode;
}

std::optional<bool> ShareControllerOptionsImpl::isSingleSelection() const
{
    return m_singleSelectionMode;
}

void ShareControllerOptionsImpl::setDefaultPreviewMode(bool defaultPreviewMode)
{
    m_defaultPreviewMode = defaultPreviewMode;
}

std::optional<bool> ShareControllerOptionsImpl::isDefaultPreview() const
{
    return m_defaultPreviewMode;
}

void ShareControllerOptionsImpl::setExcludedAbilities(const QList<ShareAbilityType> &excludedAbilities)
{
    m_excludedAbilities = excludedAbilities;
}

std::optional<QList<ShareAbilityType>> ShareControllerOptionsImpl::excludedAbilities() const
{
    return m_excludedAbilities;
}

class ShareOperationResultImpl : public ShareOperationResult
{
public:
    ShareOperationResultImpl(
        const QOhosShareKit::ShareOperationResult &shareOperationResult);

    QString targetAbilityName() const override;

private:
    QString m_targetAbilityName;
};

ShareOperationResultImpl::ShareOperationResultImpl(
    const QOhosShareKit::ShareOperationResult &shareOperationResult)
    : ShareOperationResult()
    , m_targetAbilityName(QString::fromStdString(shareOperationResult.targetAbilityName))
{
}

QString ShareOperationResultImpl::targetAbilityName() const
{
    return m_targetAbilityName;
}

std::vector<QOhosShareKit::SharedRecord> convertToShareKitSharedRecords(
    const QList<std::shared_ptr<SharedRecord>> &dataToShare)
{
    std::vector<QOhosShareKit::SharedRecord> records;

    for (const auto &sharedRecord : dataToShare) {
        const auto *sharedRecordImpl = static_cast<const SharedRecordImpl *>(sharedRecord.get());

        auto shareKitSharedRecord = QOhosShareKit::SharedRecord{
            .mimeType = !sharedRecordImpl->isUrlContent()
                ? sharedRecordImpl->mimeType().name().toStdString()
                : std::string(mimeTextUriList),
        };

        if (!sharedRecordImpl->content().isNull()) {
            shareKitSharedRecord.content = sharedRecordImpl->content().toStdString();
        } else if (!sharedRecordImpl->filePath().isNull()) {
            shareKitSharedRecord.filePath = sharedRecordImpl->filePath().toStdString();
        } else {
            qOhosPrintfWarning("%s: SharedRecord doesn't have content or uri, skipping...", Q_FUNC_INFO);
            continue;
        }

        if (!sharedRecordImpl->title().isNull())
            shareKitSharedRecord.title = sharedRecordImpl->title().toStdString();
        if (!sharedRecordImpl->label().isNull())
            shareKitSharedRecord.label = sharedRecordImpl->label().toStdString();
        if (!sharedRecordImpl->description().isNull())
            shareKitSharedRecord.description = sharedRecordImpl->description().toStdString();
        if (!sharedRecordImpl->thumbnail().isNull())
            shareKitSharedRecord.thumbnail = sharedRecordImpl->thumbnail();
        if (!sharedRecordImpl->thumbnailFilePath().isNull())
            shareKitSharedRecord.thumbnailFilePath = sharedRecordImpl->thumbnailFilePath().toStdString();
        if (!sharedRecordImpl->extraData().isEmpty())
            shareKitSharedRecord.extraData = sharedRecordImpl->extraData();

        records.push_back(shareKitSharedRecord);
    }

    return records;
}

}

/*!
    \class QtOhosAppKit::ShareKit::SharedRecord
    \inmodule QtOhosAppKit
    \since 5.12.12
    \brief The SharedRecord class represents a record to be shared with other application. A record can be created using
    \sa QtOhosAppKit::ShareKit::createContentRecord(), QtOhosAppKit::ShareKit::createFileRecord() or QtOhosAppKit::ShareKit::createUrlRecord().
    See \l {https://developer.huawei.com/consumer/en/doc/harmonyos-references/share-system-share#section20696483813}
    {SharedRecord}.
*/

/*!
    \fn virtual QMimeType QtOhosAppKit::ShareKit::SharedRecord::mimeType() const = 0

    Gets the shared record associated mime type.
    See \l {https://developer.huawei.com/consumer/en/doc/harmonyos-references/share-system-share#section20696483813}
    {SharedRecord.utd}
*/

/*!
    \fn virtual QString QtOhosAppKit::ShareKit::SharedRecord::content() const = 0

    Gets the shared record optional content. Either content or file path must be set. If there is no content null string is provided.
    See \l {https://developer.huawei.com/consumer/en/doc/harmonyos-references/share-system-share#section20696483813}
    {SharedRecord.content}
*/

/*!
    \fn virtual QString QtOhosAppKit::ShareKit::SharedRecord::filePath() const = 0

    Gets the shared record optional file path. Either content or file path must be set. If there is no file path null string is provided.
    See \l {https://developer.huawei.com/consumer/en/doc/harmonyos-references/share-system-share#section20696483813}
    {SharedRecord.content}
*/

/*!
    \fn virtual bool QtOhosAppKit::ShareKit::SharedRecord::isUrlContent() const = 0

    Provides information if content() contains URL string. For URL content the mimeType() should not be used.
    See \l {https://developer.huawei.com/consumer/en/doc/harmonyos-references/share-system-share#section20696483813}
    {SharedRecord.content}
*/

/*!
    \fn virtual void QtOhosAppKit::ShareKit::SharedRecord::setTitle(const QString &title) = 0

    Sets the title of shared content with a given \a title.
    See \l {https://developer.huawei.com/consumer/en/doc/harmonyos-references/share-system-share#section20696483813}
    {SharedRecord.title}
*/

/*!
    \fn virtual QString QtOhosAppKit::ShareKit::SharedRecord::title() const = 0

    Gets the optional title of the shared record. If there is no title null string is provided.
    See \l {https://developer.huawei.com/consumer/en/doc/harmonyos-references/share-system-share#section20696483813}
    {SharedRecord.title}
*/

/*!
    \fn virtual void QtOhosAppKit::ShareKit::SharedRecord::setLabel(const QString &label) = 0

    Sets the label indicating the current data record type with a given \a label.
    See \l {https://developer.huawei.com/consumer/en/doc/harmonyos-references/share-system-share#section20696483813}
    {SharedRecord.label}
*/

/*!
    \fn virtual QString QtOhosAppKit::ShareKit::SharedRecord::label() const = 0

    Gets the optional label of the shared record. If there is no label null string is provided.
    See \l {https://developer.huawei.com/consumer/en/doc/harmonyos-references/share-system-share#section20696483813}
    {SharedRecord.label}
*/

/*!
    \fn virtual void QtOhosAppKit::ShareKit::SharedRecord::setDescription(const QString &description) = 0

    Sets data record description with a given \a description.
    See \l {https://developer.huawei.com/consumer/en/doc/harmonyos-references/share-system-share#section20696483813}
    {SharedRecord.description}
*/

/*!
    \fn virtual QString QtOhosAppKit::ShareKit::SharedRecord::description() const = 0

    Gets the optional description of the shared record. If there is no description null string is provided.
    See \l {https://developer.huawei.com/consumer/en/doc/harmonyos-references/share-system-share#section20696483813}
    {SharedRecord.description}
*/

/*!
    \fn virtual void QtOhosAppKit::ShareKit::SharedRecord::setThumbnail(const QByteArray &thumbnail) = 0

    Sets data record thumbnail with a given \a thumbnail. The thumbnail is an image file content.
    See \l {https://developer.huawei.com/consumer/en/doc/harmonyos-references/share-system-share#section20696483813}
    {SharedRecord.thumbnail}
*/

/*!
    \fn virtual QByteArray QtOhosAppKit::ShareKit::SharedRecord::thumbnail() const = 0

    Gets the optional thumbnail content of the shared record. If there is no thumbnail empty byte array is provided.
    See \l {https://developer.huawei.com/consumer/en/doc/harmonyos-references/share-system-share#section20696483813}
    {SharedRecord.thumbnail}
*/

/*!
    \fn virtual void QtOhosAppKit::ShareKit::SharedRecord::setThumbnailFilePath(const QString &thumbnailFilePath) = 0

    Sets data record thumbnail uri with a given \a thumbnailFilePath.
    See \l {https://developer.huawei.com/consumer/en/doc/harmonyos-references/share-system-share#section20696483813}
    {SharedRecord.thumbnailUri}
*/

/*!
    \fn virtual QString QtOhosAppKit::ShareKit::SharedRecord::thumbnailFilePath() const = 0

    Gets the optional thumbnail file path of the shared record. If there is no thumbnail file path null string is provided.
    See \l {https://developer.huawei.com/consumer/en/doc/harmonyos-references/share-system-share#section20696483813}
    {SharedRecord.thumbnailFilePath}
*/

/*!
    \fn virtual void QtOhosAppKit::ShareKit::SharedRecord::setExtraData(const QVariantMap &extraData) = 0

    Sets exatra data for sharing with a given \a extraData.
    See \l {https://developer.huawei.com/consumer/en/doc/harmonyos-references/share-system-share#section20696483813}
    {SharedRecord.extraData}
*/

/*!
    \fn virtual QVariantMap QtOhosAppKit::ShareKit::SharedRecord::extraData() const = 0

    Gets the optional extra data of the shared record. If there is no extra data empty variant map is provided.
    See \l {https://developer.huawei.com/consumer/en/doc/harmonyos-references/share-system-share#section20696483813}
    {SharedRecord.extraData}
*/

SharedRecord::SharedRecord() = default;
SharedRecord::~SharedRecord() = default;

/*!
    \class QtOhosAppKit::ShareKit::ShareControllerOptions
    \inmodule QtOhosAppKit
    \since 5.12.12
    \brief The ShareControllerOptions class is to configure items, such as the preview mode of the shared content, selection mode,
    and other information, and pop-up window anchor. It determines the display style of the sharing panel.
    See \l {https://developer.huawei.com/consumer/en/doc/harmonyos-references/share-system-share#section107934816010}
    {ShareControllerOptions}
*/

/*!
    \fn virtual void QtOhosAppKit::ShareKit::ShareControllerOptions::setAnchorOffset(QPoint anchorOffset) = 0

    Sets sharing pop-up window anchor window offset with a given \a anchorOffset.
    See \l {https://developer.huawei.com/consumer/en/doc/harmonyos-references/share-system-share#section107934816010}
    {ShareControllerOptions.anchor}
*/

/*!
    \fn virtual void QtOhosAppKit::ShareKit::ShareControllerOptions::setAnchor(QRect anchor) = 0

    Sets anchor offset and size with a given \a anchor.
    See \l {https://developer.huawei.com/consumer/en/doc/harmonyos-references/share-system-share#section19505934714}
    {ShareControllerAnchor}
*/

/*!
    \fn virtual void QtOhosAppKit::ShareKit::ShareControllerOptions::setSingleSelectionMode(bool singleSelectionMode) = 0

    Sets sharing selection mode with a given \a singleSelectionMode. If singleSelectionMode is true,
    single selection is set, batch mode otherwise.
    See \l {https://developer.huawei.com/consumer/en/doc/harmonyos-references/share-system-share#section107934816010}
    {ShareControllerOptions.selectionMode}
*/


/*!
    \fn virtual void QtOhosAppKit::ShareKit::ShareControllerOptions::setDefaultPreviewMode(bool defaultPreviewMode) = 0

    Set sharing panel preview mode with a given \a defaultPreviewMode. If defaultPreviewMode is true, default preview
    mode (thumbnail card) is set, detail mode otherwise. Detail mode is recommended for images and videos.
    See \l {https://developer.huawei.com/consumer/en/doc/harmonyos-references/share-system-share#section107934816010}
    {ShareControllerOptions.previewMode}
*/

/*!
    \fn virtual void QtOhosAppKit::ShareKit::ShareControllerOptions::setExcludedAbilities(const QList<ShareAbilityType> &excludedAbilities) = 0

    Set a list of capabilities that do not need to be displayed in the operation area.
    See \l {https://developer.huawei.com/consumer/en/doc/harmonyos-references/share-system-share#section107934816010}
    {ShareControllerOptions.excludedAbilities}
*/

ShareControllerOptions::ShareControllerOptions() = default;
ShareControllerOptions::~ShareControllerOptions() = default;

/*!
    \class QtOhosAppKit::ShareKit::ShareOperationResult
    \inmodule QtOhosAppKit
    \since 5.12.12
    \brief ShareOperationResult wraps Ohos \l {https://developer.huawei.com/consumer/en/doc/harmonyos-references/share-system-share#section17135118312}
    {ShareOperationResult} interface.

    It keeps information about share target ability.
*/

/*!
    \fn virtual QString QtOhosAppKit::ShareKit::ShareOperationResult::targetAbilityName() const = 0

    Gets the target ability name. For more info how the name is built please check following link.
    See \l {https://developer.huawei.com/consumer/en/doc/harmonyos-references/share-system-share#section20696483813}
    {SharedRecord.utd}
*/

ShareOperationResult::ShareOperationResult() = default;
ShareOperationResult::~ShareOperationResult() = default;

/*!
    \fn std::shared_ptr<QtOhosAppKit::ShareKit::SharedRecord> QtOhosAppKit::ShareKit::createContentRecord(const QMimeType &mimeType, const QString &content)

    Creates a shared "content" record with a given \a mimeType and \a content. Shared record can be created
    with content (this function) or as a file shared record \sa QtOhosAppKit::ShareKit::createFileRecord().
    See \l {https://developer.huawei.com/consumer/en/doc/harmonyos-references/share-system-share#section20696483813}
    {SharedRecord.content}
*/
std::shared_ptr<SharedRecord> createContentRecord(
    const QMimeType &mimeType, const QString &content)
{
    return std::make_shared<SharedRecordImpl>(mimeType, content, false);
}

/*!
    \fn std::shared_ptr<QtOhosAppKit::ShareKit::SharedRecord> QtOhosAppKit::ShareKit::createFileRecord(const QFileInfo &fileInfo)

    Creates a shared "file" record with a given \a fileInfo. Shared record can be created
    with file (this function) or as a content record \sa QtOhosAppKit::ShareKit::createContentRecord().
    See \l {https://developer.huawei.com/consumer/en/doc/harmonyos-references/share-system-share#section20696483813}
    {SharedRecord.uri}
*/
std::shared_ptr<SharedRecord> createFileRecord(const QFileInfo &fileInfo)
{
    return std::make_shared<SharedRecordImpl>(fileInfo);
}

/*!
    \fn std::shared_ptr<QtOhosAppKit::ShareKit::SharedRecord> QtOhosAppKit::ShareKit::createUrlRecord(const QUrl &url)

    Creates a shared record with a given \a url.
    See \l {https://developer.huawei.com/consumer/en/doc/harmonyos-references/share-system-share#section20696483813}
    {SharedRecord.content}
*/
std::shared_ptr<SharedRecord> createUrlRecord(const QUrl &url)
{
    return std::make_shared<SharedRecordImpl>(QMimeType(), url.toString(), true);
}

/*!
    \fn std::shared_ptr<QtOhosAppKit::ShareKit::ShareControllerOptions> QtOhosAppKit::ShareKit::createControllerOptions()

    Creates a controller options instnace. Controller options can be used to configure preview mode,
    selection mode and pop-up window anchor.
    See \l {https://developer.huawei.com/consumer/en/doc/harmonyos-references/share-system-share#section107934816010}
    {ShareControllerOptions}
*/
std::shared_ptr<ShareControllerOptions> createControllerOptions()
{
    return std::make_shared<ShareControllerOptionsImpl>();
}

}

namespace Private {

using namespace ShareKit;

std::shared_ptr<void> shareData(
    QWindow *optMainWindow, const QList<std::shared_ptr<SharedRecord>> &records,
    std::shared_ptr<ShareControllerOptions> controllerOptions,
    std::function<void()> panelClosedCallback,
    std::function<void(std::shared_ptr<ShareKit::ShareOperationResult>)> shareCompletedCallback)
{
    QOhosShareKit::ControllerOptions shareKitControllerOptions;
    if (controllerOptions) {
        const auto *controllerOptionsImpl = static_cast<const ShareControllerOptionsImpl *>(controllerOptions.get());

        const auto optAnchorOffset = controllerOptionsImpl->anchorOffset();
        if (optAnchorOffset.has_value()) {
            shareKitControllerOptions.anchor = QOhosShareKit::ShareControllerAnchor{
                .windowOffset = optAnchorOffset.value(),
                .size = controllerOptionsImpl->anchorSize(),
            };
        }

        const auto optSingleSelection = controllerOptionsImpl->isSingleSelection();
        if (optSingleSelection.has_value()) {
            shareKitControllerOptions.selectionMode = optSingleSelection.value()
                ? QOhosShareKit::SelectionMode::SINGLE
                : QOhosShareKit::SelectionMode::BATCH;
        }

        const auto optDefaultPreview = controllerOptionsImpl->isDefaultPreview();
        if (optDefaultPreview.has_value()) {
            shareKitControllerOptions.previewMode = optDefaultPreview.value()
                ? QOhosShareKit::SharePreviewMode::DEFAULT
                : QOhosShareKit::SharePreviewMode::DETAIL;
        }

        const auto optExcludedAbilities = controllerOptionsImpl->excludedAbilities();
        if (optExcludedAbilities.has_value())
            shareKitControllerOptions.excludedAbilities =
                mapExcludedAbilitiesToShareKit(optExcludedAbilities.value());
    }

    return QOhosShareKit::shareData(
        optMainWindow, convertToShareKitSharedRecords(records), shareKitControllerOptions,
        std::move(panelClosedCallback),
        [shareCompletedCallback = std::move(shareCompletedCallback)](auto shareOperationResult) {
            shareCompletedCallback(std::make_shared<ShareOperationResultImpl>(shareOperationResult));
        });
}

}

}

QT_END_NAMESPACE
