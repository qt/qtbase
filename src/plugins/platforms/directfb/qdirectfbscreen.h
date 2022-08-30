// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#ifndef QDIRECTFBSCREEN_H
#define QDIRECTFBSCREEN_H

#include "qdirectfbconvenience.h"
#include "qdirectfbcursor.h"

#include <qpa/qplatformintegration.h>

#include <directfb.h>

QT_BEGIN_NAMESPACE


class QDirectFbScreen : public QPlatformScreen
{
public:
    QDirectFbScreen(int display);

    QRect geometry() const override { return m_geometry; }
    int depth() const override { return m_depth; }
    QImage::Format format() const override { return m_format; }
    QSizeF physicalSize() const override { return m_physicalSize; }
    QPlatformCursor *cursor() const override { return m_cursor.data(); }

    // DirectFb helpers
    IDirectFBDisplayLayer *dfbLayer() const;

public:
    QRect m_geometry;
    int m_depth;
    QImage::Format m_format;
    QSizeF m_physicalSize;

    QDirectFBPointer<IDirectFBDisplayLayer> m_layer;

private:
    QScopedPointer<QDirectFBCursor> m_cursor;
};

QT_END_NAMESPACE

#endif
