// Copyright (C) 2019 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#ifndef QWASMLOCALFILEACCESS_P_H
#define QWASMLOCALFILEACCESS_P_H

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

#include <private/qglobal_p.h>
#include <cstdint>
#include <functional>

QT_BEGIN_NAMESPACE

namespace QWasmLocalFileAccess {

enum class FileSelectMode { SingleFile, MultipleFiles };

Q_CORE_EXPORT void openFiles(const std::string &accept, FileSelectMode fileSelectMode,
    const std::function<void (int fileCount)> &fileDialogClosed,
    const std::function<char *(uint64_t size, const std::string& name)> &acceptFile,
    const std::function<void()> &fileDataReady);

Q_CORE_EXPORT void openFile(const std::string &accept,
    const std::function<void (bool fileSelected)> &fileDialogClosed,
    const std::function<char *(uint64_t size, const std::string& name)> &acceptFile,
    const std::function<void()> &fileDataReady);

Q_CORE_EXPORT void saveFile(const QByteArray &data, const std::string &fileNameHint);
Q_CORE_EXPORT void saveFile(const char *content, size_t size, const std::string &fileNameHint);

} // namespace QWasmLocalFileAccess

QT_END_NAMESPACE

#endif // QWASMLOCALFILEACCESS_P_H
