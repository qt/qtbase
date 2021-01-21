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

    QRect geometry() const { return m_geometry; }
    int depth() const { return m_depth; }
    QImage::Format format() const { return m_format; }
    QSizeF physicalSize() const { return m_physicalSize; }
    QPlatformCursor *cursor() const { return m_cursor.data(); }

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
