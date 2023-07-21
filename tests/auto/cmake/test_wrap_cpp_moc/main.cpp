// Copyright (C) 2024 The Qt Company Ltd.
// SPDX-License-Identifier: BSD-3-Clause

#include <QObject>

class MyObject2 : public QObject {
    Q_OBJECT
public:
    MyObject2() = default;
};

#include "main.moc"

int main()
{
    return 0;
}
