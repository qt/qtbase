// Copyright (C) 2017 Klarälvdalens Datakonsult AB, a KDAB Group company, info@kdab.com, author Rafael Roquetto <rafael.roquetto@kdab.com>
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#include "qtheaders.h"

const char *qtHeaders()
{
    static const char headers[] = ""
              "#include <QString>\n"
              "#include <QByteArray>\n"
              "#include <QUrl>\n"
              "#include <QRect>\n";

    return headers;
}
