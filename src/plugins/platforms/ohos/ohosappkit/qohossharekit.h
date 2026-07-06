// Copyright (C) 2026 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#ifndef QOHOSSHAREKIT_H
#define QOHOSSHAREKIT_H

#include <QtCore/qbytearray.h>
#include <QtCore/qfileinfo.h>
#include <QtCore/qlist.h>
#include <QtCore/qmimetype.h>
#include <QtCore/qpoint.h>
#include <QtCore/qrect.h>
#include <QtCore/qsharedpointer.h>
#include <QtCore/qstring.h>
#include <QtCore/qurl.h>
#include <QtCore/qvariant.h>
#include <QtOhosAppKit/qtohosappkitglobal.h>

QT_BEGIN_NAMESPACE

namespace QtOhosAppKit {

namespace ShareKit {

enum class ShareAbilityType {
    CopyToPasteboard,
    SaveToMediaAsset,
    SaveAsFile,
    Print,
    SaveToSuperHub,
};

class Q_OHOSAPPKIT_EXPORT QOhosSharedRecord
{
public:
    virtual ~QOhosSharedRecord();

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
    QOhosSharedRecord();

private:
    Q_DISABLE_COPY(QOhosSharedRecord)
};

class Q_OHOSAPPKIT_EXPORT QOhosShareControllerOptions
{
public:
    virtual ~QOhosShareControllerOptions();

    virtual void setAnchor(const QPoint &anchorOffset) = 0;
    virtual void setAnchor(const QRect &anchor) = 0;
    virtual void setSingleSelectionMode(bool singleSelectionMode) = 0;
    virtual void setDefaultPreviewMode(bool defaultPreviewMode) = 0;
    virtual void setExcludedAbilities(const QList<ShareAbilityType> &excludedAbilities) = 0;

protected:
    QOhosShareControllerOptions();

private:
    Q_DISABLE_COPY(QOhosShareControllerOptions)
};

class Q_OHOSAPPKIT_EXPORT QOhosShareOperationResult
{
public:
    virtual ~QOhosShareOperationResult();

    virtual QString targetAbilityName() const = 0;

protected:
    QOhosShareOperationResult();

private:
    Q_DISABLE_COPY(QOhosShareOperationResult)
};

Q_OHOSAPPKIT_EXPORT QSharedPointer<QOhosSharedRecord> createContentRecord(
    const QMimeType &mimeType, const QString &content);
Q_OHOSAPPKIT_EXPORT QSharedPointer<QOhosSharedRecord> createFileRecord(const QFileInfo &fileInfo);
Q_OHOSAPPKIT_EXPORT QSharedPointer<QOhosSharedRecord> createUrlRecord(const QUrl &url);

Q_OHOSAPPKIT_EXPORT QSharedPointer<QOhosShareControllerOptions> createControllerOptions();

}

}

QT_END_NAMESPACE

#endif // QOHOSSHAREKIT_H
