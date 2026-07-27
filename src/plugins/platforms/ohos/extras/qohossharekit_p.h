// Copyright (C) 2026 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#ifndef QOHOSSHAREKIT_P_H
#define QOHOSSHAREKIT_P_H

//
//  W A R N I N G
//  -------------
//
// This file is not part of the Qt API.  It exists purely as an
// implementation detail.  This header file may change from version to
// version without notice, or even be removed.
//
// We mean it.
//

#include <QtCore/qbytearray.h>
#include <QtCore/qfileinfo.h>
#include <QtCore/qlist.h>
#include <QtCore/qmimetype.h>
#include <QtCore/qpoint.h>
#include <QtCore/qrect.h>
#include <QtCore/qstring.h>
#include <QtCore/qurl.h>
#include <QtCore/qvariant.h>
#include <QtGui/qwindow.h>
#include <QtHarmonyExtras/private/qtharmonyextrasglobal_p.h>
#include <functional>
#include <memory>

QT_BEGIN_NAMESPACE

namespace QtHarmonyExtras {

namespace ShareKit {

Q_NAMESPACE

enum class ShareAbilityType {
    CopyToPasteboard,
    SaveToMediaAsset,
    SaveAsFile,
    Print,
    SaveToSuperHub,
};
Q_ENUM_NS(ShareAbilityType)

class Q_HARMONYEXTRAS_EXPORT SharedRecord
{
public:
    virtual ~SharedRecord();

    virtual QMimeType mimeType() const = 0;
    virtual QString content() const = 0;
    virtual QString filePath() const = 0;
    virtual bool isUrlContent() const = 0;

    virtual void setTitle(const QString &title) = 0;
    virtual QString title() const = 0;

    virtual void setLabel(const QString &label) = 0;
    virtual QString label() const = 0;

    virtual void setDescription(const QString &description) = 0;
    virtual QString description() const = 0;

    virtual void setThumbnail(const QByteArray &thumbnail) = 0;
    virtual QByteArray thumbnail() const = 0;

    virtual void setThumbnailFilePath(const QString &thumbnailFilePath) = 0;
    virtual QString thumbnailFilePath() const = 0;

    virtual void setExtraData(const QVariantMap &extraData) = 0;
    virtual QVariantMap extraData() const = 0;

protected:
    SharedRecord();

private:
    Q_DISABLE_COPY(SharedRecord)
};

class Q_HARMONYEXTRAS_EXPORT ShareControllerOptions
{
public:
    virtual ~ShareControllerOptions();

    virtual void setAnchorOffset(QPoint anchorOffset) = 0;
    virtual void setAnchor(QRect anchor) = 0;
    virtual void setSingleSelectionMode(bool singleSelectionMode) = 0;
    virtual void setDefaultPreviewMode(bool defaultPreviewMode) = 0;
    virtual void setExcludedAbilities(const QList<ShareAbilityType> &excludedAbilities) = 0;

protected:
    ShareControllerOptions();

private:
    Q_DISABLE_COPY(ShareControllerOptions)
};

class Q_HARMONYEXTRAS_EXPORT ShareOperationResult
{
public:
    virtual ~ShareOperationResult();

    virtual QString targetAbilityName() const = 0;

protected:
    ShareOperationResult();

private:
    Q_DISABLE_COPY(ShareOperationResult)
};

Q_HARMONYEXTRAS_EXPORT std::shared_ptr<SharedRecord> createContentRecord(
    const QMimeType &mimeType, const QString &content);
Q_HARMONYEXTRAS_EXPORT std::shared_ptr<SharedRecord> createFileRecord(const QFileInfo &fileInfo);
Q_HARMONYEXTRAS_EXPORT std::shared_ptr<SharedRecord> createUrlRecord(const QUrl &url);

Q_HARMONYEXTRAS_EXPORT std::shared_ptr<ShareControllerOptions> createControllerOptions();

}

}

namespace QtHarmonyExtras::Private {

std::shared_ptr<void> shareData(
    QWindow *optMainWindow, const QList<std::shared_ptr<ShareKit::SharedRecord>> &records,
    std::shared_ptr<ShareKit::ShareControllerOptions> controllerOptions,
    std::function<void()> panelClosedCallback,
    std::function<void(std::shared_ptr<ShareKit::ShareOperationResult>)> shareCompletedCallback);

}

QT_END_NAMESPACE

#endif // QOHOSSHAREKIT_P_H
