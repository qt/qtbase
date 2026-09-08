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
#include <QtCore/private/qohospathutils_p.h>
#include <optional>
#include <string>

QT_BEGIN_NAMESPACE

namespace {

bool tryDeleteToTrash(const QString &filePath)
{
    return QOhosJsThreadGateway::evalWithPromise<bool>(
        [&](QOhosJsState &jsState, QOhosTaskPromise<bool> resultPromise) {
            auto optFileOhosUri = tryMapPathToOhosFileUri(filePath.toStdString());
            if (!optFileOhosUri.has_value()) {
                resultPromise(false);
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
                auto thenCatchPromises = std::move(resultPromise).makeThenCatchBranches(Q_FUNC_INFO);
                QNapi::checkedCast<QNapi::Promise>(deletePromiseOrValue)
                .onThen(
                    [thenPromise = std::move(thenCatchPromises.first)](const QOhosCallbackInfo &cbInfo) {
                        std::string deletedPath = cbInfo.getFirstArg<QNapi::String>(Q_FUNC_INFO);
                        thenPromise(!deletedPath.empty());
                    })
                .onCatch(
                    [catchPromise = std::move(thenCatchPromises.second)](const QOhosCallbackInfo &cbInfo) {
                        QtOhos::logJsCallbackError(cbInfo, "Got error from deleteToTrash()");
                        catchPromise(false);
                    });
            } else {
                qOhosPrintfWarning("Got non-Promise from deleteToTrash()");
                resultPromise(false);
            }
        },
        Q_FUNC_INFO);
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
