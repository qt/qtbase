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
****************************************************************************/

#ifndef QWASMOFFSCREENSURFACE_H
#define QWASMOFFSCREENSURFACE_H

#include <qpa/qplatformoffscreensurface.h>

QT_BEGIN_NAMESPACE

class QOffscreenSurface;
class QWasmOffscrenSurface : public QPlatformOffscreenSurface
{
public:
    explicit QWasmOffscrenSurface(QOffscreenSurface *offscreenSurface);
    ~QWasmOffscrenSurface();
private:

};

QT_END_NAMESPACE

#endif
