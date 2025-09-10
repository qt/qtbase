// Copyright (C) 2018 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only

#include "qwasmbase64iconstore.h"

#include <QtCore/qfile.h>

QT_BEGIN_NAMESPACE

Q_GLOBAL_STATIC(Base64IconStore, globalWasmWindowIconStore);

Base64IconStore::Base64IconStore()
{
    QString iconSources[static_cast<size_t>(IconType::Size)] = {
        QStringLiteral(":/wasm-window/maximize.svg"), QStringLiteral(":/wasm-window/qtlogo.svg"),
        QStringLiteral(":/wasm-window/restore.svg"), QStringLiteral(":/wasm-window/x.svg")
    };

    for (size_t iconType = static_cast<size_t>(IconType::First);
         iconType < static_cast<size_t>(IconType::Size); ++iconType) {
        QFile svgFile(iconSources[static_cast<size_t>(iconType)]);
        if (!svgFile.open(QIODevice::ReadOnly))
            Q_ASSERT(false); // A resource should always be opened.
        m_storage[static_cast<size_t>(iconType)] = svgFile.readAll().toBase64();
    }
}

Base64IconStore::~Base64IconStore() = default;

Base64IconStore *Base64IconStore::get()
{
    return globalWasmWindowIconStore();
}

std::string_view Base64IconStore::getIcon(IconType type) const
{
    return m_storage[static_cast<size_t>(type)];
}

QT_END_NAMESPACE
