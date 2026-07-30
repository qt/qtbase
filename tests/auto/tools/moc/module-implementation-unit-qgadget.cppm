// Copyright (C) 2026 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only

module;

module ModuleImplementationUnitQGadget;

// Q_GADGET is not supported in a module implementation unit: nothing outside this
// translation unit can ever see this class, since implementation units aren't
// importable by anything. moc should reject this outright.
class Invalid
{
    Q_GADGET
};
