// Copyright (C) 2025 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only

#include <QtCore/qobject.h>

namespace DisableDeprecatedMacro {
class Object : public QObject
{
    Q_OBJECT
public:
    using QObject::QObject;
    enum E { A, B, C, D };
    enum F { None = 0x0, One = 0x1, Two = 0x2, Three = 0x4 };
    Q_DECLARE_FLAGS(Flags, F)
#if QT_DEPRECATED_SINCE(6, 12)
    QT_WARNING_PUSH
    QT_WARNING_DISABLE_DEPRECATED
    Q_ENUMS(E)
    Q_FLAGS(Flags)
    QT_WARNING_POP
#endif
};
} // namespace DisableDeprecatedMacro

extern void warnings_linker_hook()
{
    [[maybe_unused]] DisableDeprecatedMacro::Object o;
}

#include "warnings.moc"
