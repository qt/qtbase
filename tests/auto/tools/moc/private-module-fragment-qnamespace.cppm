// Copyright (C) 2026 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only

module;

export module PrivateModuleFragmentQNamespace;

export int primaryFn();

module :private;

// Q_NAMESPACE is not supported in a private module fragment: nothing outside this
// translation unit can ever see this namespace, not even other units of the same
// module. moc should reject this outright.
namespace Invalid
{
    Q_NAMESPACE
}
