/****************************************************************************
**
** Copyright (C) 2021 The Qt Company Ltd.
** Contact: https://www.qt.io/licensing/
**
** This file is part of the plugins of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:COMM$
**
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company. For licensing terms
** and conditions see https://www.qt.io/terms-conditions. For further
** information use the contact form at https://www.qt.io/contact-us.
**
** $QT_END_LICENSE$
**
**
**
**
**
**
**
**
**
**
**
**
**
**
**
**
**
**
**
****************************************************************************/

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

