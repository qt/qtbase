// Copyright (C) 2025 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#ifndef QOHOSWINDOWMANAGER_H
#define QOHOSWINDOWMANAGER_H

#include <QtCore/private/qohoscommon_p.h>
#include <QtCore/qpoint.h>
#include <QtCore/qrect.h>
#include <QtCore/qstringlist.h>
#include <QtCore/qstring.h>
#include <array>
#include <qohosinternalwindowid_p.h>
#include <qohosplugincore.h>
#include <utility>
#include <functional>

QT_BEGIN_NAMESPACE

namespace QOhosWindowManager
{

enum class DocumentSelectMode {
    FILE,
    FOLDER,
    MIXED,
};

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
    QtOhos::InternalWindowId contextWinId, QStringList filters, QString defaultPath,
    DocumentSelectMode documentSelectMode, ResultMultiplicity resultMultiplicity,
    QOhosConsumer<QOhosOptional<OpenResult>> resultCallback);

void showFileDialogSave(
    QtOhos::InternalWindowId contextWinId, QStringList newFileNames,
    QString defaultFilePath, QStringList fileSuffixChoices,
    QOhosConsumer<QOhosOptional<SaveResult>> resultCallback);

void showFileDialogAuthorization(
    QtOhos::InternalWindowId contextWinId, QString filePath,
    QOhosConsumer<bool> resultCallback);

}

namespace QtOhos
{

template<>
struct OhosEnumMeta<QOhosWindowManager::DocumentSelectMode>
{
    static constexpr const char *fullTypeName = "@ohos.file.picker.DocumentSelectMode";
    static constexpr std::array<std::pair<QOhosWindowManager::DocumentSelectMode, const char *>, 3> enumeratorsNames = {{
        {QOhosWindowManager::DocumentSelectMode::FILE, "FILE"},
        {QOhosWindowManager::DocumentSelectMode::FOLDER, "FOLDER"},
        {QOhosWindowManager::DocumentSelectMode::MIXED, "MIXED"},
    }};
};

}

QT_END_NAMESPACE

#endif // QOHOSWINDOWMANAGER_H
