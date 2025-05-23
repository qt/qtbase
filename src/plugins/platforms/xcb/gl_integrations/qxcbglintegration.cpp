// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial

#include "qxcbglintegration.h"

QT_BEGIN_NAMESPACE

Q_LOGGING_CATEGORY(lcQpaGl, "qt.qpa.gl")

QXcbGlIntegration::QXcbGlIntegration()
{
}
QXcbGlIntegration::~QXcbGlIntegration()
{
}

bool QXcbGlIntegration::handleXcbEvent(xcb_generic_event_t *event, uint responseType)
{
    Q_UNUSED(event);
    Q_UNUSED(responseType);
    return false;
}

QT_END_NAMESPACE
