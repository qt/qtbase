// Copyright (C) 2026 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only

module;

#include <QObject>

export module PrivateModuleFragmentQObject;

export int primaryFn();

module :private;

// Q_OBJECT is not supported in a private module fragment: nothing outside this
// translation unit can ever see this class, not even other units of the same
// module. moc should reject this outright.
class Invalid : public QObject
{
    Q_OBJECT
};
