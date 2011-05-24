/****************************************************************************
**
** Copyright (C) 2011 Nokia Corporation and/or its subsidiary(-ies).
** All rights reserved.
** Contact: Nokia Corporation (qt-info@nokia.com)
**
** This file is part of the QtOpenVG module of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:LGPL$
** GNU Lesser General Public License Usage
** This file may be used under the terms of the GNU Lesser General Public
** License version 2.1 as published by the Free Software Foundation and
** appearing in the file LICENSE.LGPL included in the packaging of this
** file. Please review the following information to ensure the GNU Lesser
** General Public License version 2.1 requirements will be met:
** http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, Nokia gives you certain additional
** rights. These rights are described in the Nokia Qt LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
** GNU General Public License Usage
** Alternatively, this file may be used under the terms of the GNU General
** Public License version 3.0 as published by the Free Software Foundation
** and appearing in the file LICENSE.GPL included in the packaging of this
** file. Please review the following information to ensure the GNU General
** Public License version 3.0 requirements will be met:
** http://www.gnu.org/copyleft/gpl.html.
**
** Other Usage
** Alternatively, this file may be used in accordance with the terms and
** conditions contained in a signed written agreement between you and Nokia.
**
**
**
**
**
** $QT_END_LICENSE$
**
****************************************************************************/

#ifndef QVG_P_H
#define QVG_P_H

#include "qvg.h"

//
//  W A R N I N G
//  -------------
//
// This file is not part of the Qt API.  It exists for the convenience
// of other Qt classes.  This header file may change from version to
// version without notice, or even be removed.
//
// We mean it.
//

// vgDrawGlyphs() only exists in OpenVG 1.1 and higher.
#if !defined(OPENVG_VERSION_1_1) && !defined(QVG_NO_DRAW_GLYPHS)
#define QVG_NO_DRAW_GLYPHS 1
#endif

#include <QtGui/qimage.h>

#if !defined(QT_NO_EGL)
#include <QtGui/private/qeglcontext_p.h>
#endif

QT_BEGIN_NAMESPACE

class QVGPaintEngine;

#if !defined(QT_NO_EGL)

class QEglContext;

// Create an EGL context, but don't bind it to a surface.  If single-context
// mode is enabled, this will return the previously-created context.
// "devType" indicates the type of device using the context, usually
// QInternal::Widget or QInternal::Pixmap.
Q_OPENVG_EXPORT QEglContext *qt_vg_create_context
    (QPaintDevice *device, int devType);

// Destroy an EGL context that was created by qt_vg_create_context().
// If single-context mode is enabled, this will decrease the reference count.
// "devType" indicates the type of device destroying the context, usually
// QInternal::Widget or QInternal::Pixmap.
Q_OPENVG_EXPORT void qt_vg_destroy_context
    (QEglContext *context, int devType);

// Return the shared pbuffer surface that can be made current to
// destroy VGImage objects when there is no other surface available.
Q_OPENVG_EXPORT EGLSurface qt_vg_shared_surface(void);

// Convert the configuration format in a context to a VG or QImage format.
Q_OPENVG_EXPORT VGImageFormat qt_vg_config_to_vg_format(QEglContext *context);
Q_OPENVG_EXPORT QImage::Format qt_vg_config_to_image_format(QEglContext *context);

#endif

// Create a paint engine.  Returns the common engine in single-context mode.
Q_OPENVG_EXPORT QVGPaintEngine *qt_vg_create_paint_engine(void);

// Destroy a paint engine.  Does nothing in single-context mode.
Q_OPENVG_EXPORT void qt_vg_destroy_paint_engine(QVGPaintEngine *engine);

// Convert between QImage and VGImage format values.
Q_OPENVG_EXPORT VGImageFormat qt_vg_image_to_vg_format(QImage::Format format);

QT_END_NAMESPACE

#endif
