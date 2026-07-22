// Copyright (C) 2025 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#ifndef QOHOSWINDOWMANAGER_H
#define QOHOSWINDOWMANAGER_H

#include <QtCore/private/qohoscommon_p.h>
#include <QtCore/qpoint.h>
#include <QtCore/qrect.h>
#include <QtCore/qstringlist.h>
#include <QtCore/qstring.h>
#include <qohosenums.h>
#include <qohosplugincore.h>
#include <functional>
#include <optional>

QT_BEGIN_NAMESPACE

namespace QOhosWindowManager
{

using DocumentSelectMode = QtOhos::enums::ohos::file::picker::DocumentSelectMode;

enum class ResultMultiplicity {
    SINGLE,
    MULTIPLE,
};

struct OpenResult
{
    QStringList selectedUrls;
};

struct SaveResult
{
    QStringList savedUrls;
    int selectedFileSuffixChoiceIndex;
};

void showFileDialogOpen(
    QtOhos::QObjectThreadSafeRef contextWindowRef, QStringList filters, QString defaultPath,
    DocumentSelectMode documentSelectMode, ResultMultiplicity resultMultiplicity,
    QOhosConsumer<std::optional<OpenResult>> resultCallback);

void showFileDialogSave(
    QtOhos::QObjectThreadSafeRef contextWindowRef, QStringList newFileNames,
    QString defaultFilePath, QStringList fileSuffixChoices,
    QOhosConsumer<std::optional<SaveResult>> resultCallback);

}

QT_END_NAMESPACE

#endif // QOHOSWINDOWMANAGER_H
