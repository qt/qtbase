// Copyright (C) 2025 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only
#ifndef FAULTY_REGISTRATION
#define FAULTY_REGISTRATION

#include <QObject>

struct Faulty : QObject {
    Q_OBJECT
    QML_ELEMENT
};

#endif
