// Copyright (C) 2018 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only

#ifndef QWASMBASE64IMAGESTORE_H
#define QWASMBASE64IMAGESTORE_H

#include <string_view>

#include <QtCore/qtconfigmacros.h>

QT_BEGIN_NAMESPACE
class Base64IconStore
{
public:
    enum class IconType {
        Maximize,
        First = Maximize,
        QtLogo,
        Restore,
        X,
        Size,
    };

    Base64IconStore();
    ~Base64IconStore();

    static Base64IconStore *get();

    std::string_view getIcon(IconType type) const;

private:
    std::string m_storage[static_cast<size_t>(IconType::Size)];
};

QT_END_NAMESPACE
#endif // QWASMBASE64IMAGESTORE_H
