// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial

#include "qdirectfbeglhooks.h"

/**
 * This file is compiled in case there is no platform specific hook. On an
 * optimizing compiler these functions should never be called.
 */

void QDirectFBEGLHooks::platformInit()
{
}

void QDirectFBEGLHooks::platformDestroy()
{
}

bool QDirectFBEGLHooks::hasCapability(QPlatformIntegration::Capability) const
{
    return false;
}

