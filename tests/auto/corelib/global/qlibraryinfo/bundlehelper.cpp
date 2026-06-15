// Copyright (C) 2024 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only

#include <QtCore>

int main(int argc, char **argv)
{
    QCoreApplication app(argc, argv);

    QJsonObject object;
    const QMetaEnum paths = QMetaEnum::fromType<QLibraryInfo::LibraryPath>();
    for (int i = 0; i < paths.keyCount(); ++i) {
        const auto path = QLibraryInfo::LibraryPath(paths.value(i));
        object.insert(QString::fromUtf8(paths.key(i)),
                      QJsonArray::fromStringList(QLibraryInfo::paths(path)));
    }

    QFile out;
    if (!out.open(stdout, QIODevice::WriteOnly))
        return 1;

    out.write(QJsonDocument(object).toJson(QJsonDocument::Indented));
    return 0;
}
