// Copyright (C) 2025 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only

#include <string_view>
#include <QtCore/QtCore>

#ifdef QT_NO_NAMESPACE
#  ifdef QT_NAMESPACE
static_assert(false);
#  endif // QT_NAMESPACE
#else
static_assert(std::string_view(QT_STRINGIFY(QT_NAMESPACE))
           == std::string_view(QT_STRINGIFY(QT_NAMESPACE_FROM_PROPERTY)));
#endif

int main()
{}
