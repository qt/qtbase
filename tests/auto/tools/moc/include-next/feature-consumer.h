// Copyright (C) 2026 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only

// Miniature of the tst_qprinter failure: a Q_OBJECT class guarded by a feature
// macro that is only reachable through an #include_next forwarder. If moc cannot
// follow #include_next, the macro is undefined and the class is silently dropped
// from the generated meta-object (leading to an undefined vtable at link time).
#include <config.h>

#include <QtCore/qobject.h>

#if INCLUDE_NEXT_FEATURE
class IncludeNextFeatureObject : public QObject
{
    Q_OBJECT
public:
    IncludeNextFeatureObject() = default;
signals:
    void featureSignal();
};
#endif
