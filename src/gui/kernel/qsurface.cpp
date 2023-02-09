// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#include "qsurface.h"
#include "qopenglcontext.h"
#include <qpa/qplatformintegration.h>
#include <QtGui/private/qguiapplication_p.h>

QT_BEGIN_NAMESPACE

QT_IMPL_METATYPE_EXTERN_TAGGED(QSurface*, QSurface_ptr)

/*!
    \class QSurface
    \inmodule QtGui
    \since 5.0
    \brief The QSurface class is an abstraction of renderable surfaces in Qt.

    The size of the surface is accessible with the size() function. The rendering
    specific attributes of the surface are accessible through the format() function.
 */


/*!
    \enum QSurface::SurfaceClass

    The SurfaceClass enum describes the actual subclass of the surface.

    \value Window The surface is an instance of QWindow.
    \value Offscreen The surface is an instance of QOffscreenSurface.
 */

/*!
    \enum QSurface::SurfaceType

    The SurfaceType enum describes what type of surface this is.

    \value RasterSurface The surface is composed of pixels and can be rendered to using
    a software rasterizer like Qt's raster paint engine.
    \value OpenGLSurface The surface is an OpenGL compatible surface and can be used
    in conjunction with QOpenGLContext.
    \value RasterGLSurface The surface can be rendered to using a software rasterizer,
    and also supports OpenGL. This surface type is intended for internal Qt use, and
    requires the use of private API.
    \value OpenVGSurface The surface is an OpenVG compatible surface and can be used
    in conjunction with OpenVG contexts.
    \value VulkanSurface The surface is a Vulkan compatible surface and can be used
    in conjunction with the Vulkan graphics API.
    \value MetalSurface The surface is a Metal compatible surface and can be used
    in conjunction with Apple's Metal graphics API. This surface type is only supported
    on \macos and iOS.
    \value Direct3DSurface The surface is a Direct 3D 11 and 12 compatible
    surface and can be used in conjunction with the DXGI and Direct3D APIs. This
    surface type is only supported on Windows.
 */

/*!
    \fn QSurfaceFormat QSurface::format() const

    Returns the format of the surface.
 */

/*!
  Returns true if the surface is OpenGL compatible and can be used in
  conjunction with QOpenGLContext; otherwise returns false.

  \since 5.3
*/

bool QSurface::supportsOpenGL() const
{
    SurfaceType type = surfaceType();
    if (type == RasterSurface) {
        QPlatformIntegration *integ = QGuiApplicationPrivate::instance()->platformIntegration();
        return integ->hasCapability(QPlatformIntegration::OpenGLOnRasterSurface);
    }
    return type == OpenGLSurface || type == RasterGLSurface;
}

/*!
    \fn QPlatformSurface *QSurface::surfaceHandle() const

    Returns a handle to the platform-specific implementation of the surface.
 */

/*!
    \fn SurfaceType QSurface::surfaceType() const

    Returns the type of the surface.
 */

/*!
    \fn QSize QSurface::size() const

    Returns the size of the surface in pixels.
 */

/*!
    Creates a surface with the given \a type.
*/
QSurface::QSurface(SurfaceClass type)
    : m_type(type), m_reserved(nullptr)
{
}

/*!
    Destroys the surface.
*/
QSurface::~QSurface()
{
#ifndef QT_NO_OPENGL
    QOpenGLContext *context = QOpenGLContext::currentContext();
    if (context && context->surface() == this)
        context->doneCurrent();
#endif
}

/*!
   Returns the surface class of this surface.
 */
QSurface::SurfaceClass QSurface::surfaceClass() const
{
    return m_type;
}

QT_END_NAMESPACE

#include "moc_qsurface.cpp"

