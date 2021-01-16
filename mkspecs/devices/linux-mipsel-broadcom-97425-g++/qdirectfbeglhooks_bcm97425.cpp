/****************************************************************************
**
** Copyright (C) 2021 The Qt Company Ltd.
** Contact: https://www.qt.io/licensing/
**
** This file is part of the qmake spec of the Qt Toolkit.
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
#include "qdirectfbconvenience.h"

#include "default_directfb.h"

QT_BEGIN_NAMESPACE

// Exported to the directfb plugin
QDirectFBEGLHooks platform_hook;
static void *dbpl_handle;

void QDirectFBEGLHooks::platformInit()
{
    DBPL_RegisterDirectFBDisplayPlatform(&dbpl_handle, QDirectFbConvenience::dfbInterface());
}

void QDirectFBEGLHooks::platformDestroy()
{
    DBPL_UnregisterDirectFBDisplayPlatform(dbpl_handle);
    dbpl_handle = 0;
}

bool QDirectFBEGLHooks::hasCapability(QPlatformIntegration::Capability cap) const
{
    switch (cap) {
    case QPlatformIntegration::ThreadedOpenGL:
        return true;
    default:
        return false;
    }
}

QT_END_NAMESPACE
