// Copyright (C) 2025 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only
// Qt-Security score:significant reason:default

#include "qplatformdefs.h"
#include "qfilesystemengine_p.h"
#include "qfile.h"
#include "qurl.h"

#include <QtCore/private/qcore_ohos_p.h>
#include <QtCore/private/qnapi_p.h>
#include <QtCore/private/qohoscommon_p.h>
#include <QtCore/private/qohoslogger_p.h>
#include <QtCore/qscopeguard.h>
#include <cstdlib>
#include <filemanagement/file_uri/oh_file_uri.h>
#include <optional>
#include <string>

QT_BEGIN_NAMESPACE

namespace {

std::optional<std::string> tryMapPathToOhosUri(const std::string &inputPath)
{
    char *outputPtr = nullptr;
    auto outputPtrGuard = qScopeGuard(
        [&]() {
            ::free(outputPtr);
        });
    auto getUriResult = ::OH_FileUri_GetUriFromPath(inputPath.c_str(), inputPath.size(), &outputPtr);

    std::optional<std::string> outputOhosUri;
    if (getUriResult == FileManagement_ErrCode::ERR_OK && outputPtr != nullptr) {
        outputOhosUri.emplace(outputPtr);
    } else {
        qOhosPrintfWarning(
            "%s: path to OHOS URI conversion failed for input path '%s', retval: %d",
            Q_FUNC_INFO, inputPath.c_str(), static_cast<int>(getUriResult));
    }

    return outputOhosUri;
}

bool tryDeleteToTrash(const QString &filePath)
{
    return QOhosJsThreadGateway::evalWithConsumer<bool>(
        [&](QOhosJsState &jsState, QOhosConsumer<bool> resultConsumer) {
            auto optFileOhosUri = tryMapPathToOhosUri(filePath.toStdString());
            if (!optFileOhosUri.has_value()) {
                resultConsumer(false);
                return;
            }

            QNapi::Value deletePromiseOrValue;
            try {
                deletePromiseOrValue = jsState.eval(
                    "@kit.FileManagerServiceKit.fileManagerService.deleteToTrash(*)",
                    {optFileOhosUri.value()});
            } catch (const Napi::Error &error) {
                qOhosPrintfError(
                    "deleteToTrash('%s') failed with error: %s",
                    optFileOhosUri.value().c_str(), error.what());
            }

            if (deletePromiseOrValue.IsPromise()) {
                QNapi::checkedCast<QNapi::Promise>(deletePromiseOrValue)
                .withContext(std::move(resultConsumer))
                .onThenWithContext(
                    [](const QOhosCallbackInfo &cbInfo, auto &resultConsumer) {
                        std::string deletedPath = cbInfo.getFirstArg<QNapi::String>(Q_FUNC_INFO);
                        resultConsumer(!deletedPath.empty());
                    })
                .onCatchWithContext(
                    [](const QOhosCallbackInfo &cbInfo, auto &resultConsumer) {
                        QtOhos::logJsCallbackError(cbInfo, "Got error from deleteToTrash()");
                        resultConsumer(false);
                    });
            } else {
                qOhosPrintfWarning("Got non-Promise from deleteToTrash()");
                resultConsumer(false);
            }
        });
}

}

bool QFileSystemEngine::supportsMoveFileToTrash()
{
    return true;
}

bool QFileSystemEngine::moveFileToTrash(const QFileSystemEntry &source,
                                        QFileSystemEntry &, QSystemError &error)
{
    bool movedToTrash = tryDeleteToTrash(source.filePath());
    if (!movedToTrash)
        error = QSystemError(ENOSYS, QSystemError::StandardLibraryError);
    return movedToTrash;
}

QT_END_NAMESPACE
