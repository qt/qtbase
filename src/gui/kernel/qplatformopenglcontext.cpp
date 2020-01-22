/****************************************************************************
**
** Copyright (C) 2016 The Qt Company Ltd.
** Contact: https://www.qt.io/licensing/
**
** This file is part of the QtGui module of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:LGPL$
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company. For licensing terms
** and conditions see https://www.qt.io/terms-conditions. For further
** information use the contact form at https://www.qt.io/contact-us.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 3 as published by the Free Software
** Foundation and appearing in the file LICENSE.LGPL3 included in the
** packaging of this file. Please review the following information to
** ensure the GNU Lesser General Public License version 3 requirements
** will be met: https://www.gnu.org/licenses/lgpl-3.0.html.
**
** GNU General Public License Usage
** Alternatively, this file may be used under the terms of the GNU
** General Public License version 2.0 or (at your option) the GNU General
** Public license version 3 or any later version approved by the KDE Free
** Qt Foundation. The licenses are as published by the Free Software
** Foundation and appearing in the file LICENSE.GPL2 and LICENSE.GPL3
** included in the packaging of this file. Please review the following
** information to ensure the GNU General Public License requirements will
** be met: https://www.gnu.org/licenses/gpl-2.0.html and
** https://www.gnu.org/licenses/gpl-3.0.html.
**
** $QT_END_LICENSE$
**
****************************************************************************/

#include "qplatformopenglcontext.h"

#include <QOpenGLFunctions>

QT_BEGIN_NAMESPACE

/*!
    \class QPlatformOpenGLContext
    \since 4.8
    \internal
    \preliminary
    \ingroup qpa

    \brief The QPlatformOpenGLContext class provides an abstraction for native GL contexts.

    In QPA the way to support OpenGL or OpenVG or other technologies that requires a native GL
    context is through the QPlatformOpenGLContext wrapper.

    There is no factory function for QPlatformOpenGLContexts, but rather only one accessor function.
    The only place to retrieve a QPlatformOpenGLContext from is through a QPlatformWindow.

    The context which is current for a specific thread can be collected by the currentContext()
    function. This is how QPlatformOpenGLContext also makes it possible to use the Qt GUI module
    withhout using QOpenGLWidget. When using QOpenGLContext::currentContext(), it will ask
    QPlatformOpenGLContext for the currentContext. Then a corresponding QOpenGLContext will be returned,
    which maps to the QPlatformOpenGLContext.
*/

/*! \fn void QPlatformOpenGLContext::swapBuffers(QPlatformSurface *surface)
    Reimplement in subclass to native swap buffers calls

    The implementation must support being called in a thread different than the gui-thread.
*/

/*! \fn QFunctionPointer QPlatformOpenGLContext::getProcAddress(const char *procName)

    Reimplement in subclass to allow dynamic querying of OpenGL symbols. As opposed to e.g. the wglGetProcAddress
    function on Windows, Qt expects this methods to be able to return valid function pointers even for standard
    OpenGL symbols.
*/

class QPlatformOpenGLContextPrivate
{
public:
    QPlatformOpenGLContextPrivate() : context(nullptr) {}

    QOpenGLContext *context;
};

QPlatformOpenGLContext::QPlatformOpenGLContext()
    : d_ptr(new QPlatformOpenGLContextPrivate)
{
}

QPlatformOpenGLContext::~QPlatformOpenGLContext()
{
}

/*!
  Called after a new instance is constructed. The default implementation does nothing.

  Subclasses can use this function to perform additional initialization that relies on
  virtual functions.
 */
void QPlatformOpenGLContext::initialize()
{
}

/*!
    Reimplement in subclass if your platform uses framebuffer objects for surfaces.

    The default implementation returns 0.
*/
GLuint QPlatformOpenGLContext::defaultFramebufferObject(QPlatformSurface *) const
{
    return 0;
}

QOpenGLContext *QPlatformOpenGLContext::context() const
{
    Q_D(const QPlatformOpenGLContext);
    return d->context;
}

void QPlatformOpenGLContext::setContext(QOpenGLContext *context)
{
    Q_D(QPlatformOpenGLContext);
    d->context = context;
}

bool QPlatformOpenGLContext::parseOpenGLVersion(const QByteArray &versionString, int &major, int &minor)
{
    bool majorOk = false;
    bool minorOk = false;
    QList<QByteArray> parts = versionString.split(' ');
    if (versionString.startsWith(QByteArrayLiteral("OpenGL ES"))) {
        if (parts.size() >= 3) {
            QList<QByteArray> versionParts = parts.at(2).split('.');
            if (versionParts.size() >= 2) {
                major = versionParts.at(0).toInt(&majorOk);
                minor = versionParts.at(1).toInt(&minorOk);
                // Nexus 6 has "OpenGL ES 3.0V@95.0 (GIT@I86da836d38)"
                if (!minorOk)
                    if (int idx = versionParts.at(1).indexOf('V'))
                        minor = versionParts.at(1).left(idx).toInt(&minorOk);
            } else {
                qWarning("Unrecognized OpenGL ES version");
            }
        } else {
            // If < 3 parts to the name, it is an unrecognised OpenGL ES
            qWarning("Unrecognised OpenGL ES version");
        }
    } else {
        // Not OpenGL ES, but regular OpenGL, the version numbers are first in the string
        QList<QByteArray> versionParts = parts.at(0).split('.');
        if (versionParts.size() >= 2) {
            major = versionParts.at(0).toInt(&majorOk);
            minor = versionParts.at(1).toInt(&minorOk);
        } else {
            qWarning("Unrecognized OpenGL version");
        }
    }

    if (!majorOk || !minorOk)
        qWarning("Unrecognized OpenGL version");
    return (majorOk && minorOk);
}

QT_END_NAMESPACE
