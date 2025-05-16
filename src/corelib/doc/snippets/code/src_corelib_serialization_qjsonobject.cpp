// Copyright (C) 2025 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR BSD-3-Clause

//! [1]
QJsonObject obj{
    { "something", "is" },
    { "in", "this" },
    { "object", 42 },
};

for (auto [key, value] : obj.asKeyValueRange()) {
    qDebug() << key << "->" << value;
    if (key == "object")
        value = "!"; // modify the object at this key
}
qDebug() << obj["object"]; // QJsonValue(string, "!")
//! [1]
