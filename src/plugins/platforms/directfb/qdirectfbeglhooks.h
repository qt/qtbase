// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial

#ifndef QDIRECTFBEGL_HOOKS_H
#define QDIRECTFBEGL_HOOKS_H

#include <qpa/qplatformintegration.h>

QT_BEGIN_NAMESPACE

struct QDirectFBEGLHooks {
    void platformInit();
    void platformDestroy();
    bool hasCapability(QPlatformIntegration::Capability) const;
};

QT_END_NAMESPACE

#endif
