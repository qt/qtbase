/****************************************************************************
**
** Copyright (C) 2011 Nokia Corporation and/or its subsidiary(-ies).
** All rights reserved.
** Contact: Nokia Corporation (qt-info@nokia.com)
**
** This file is part of the QtGui module of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:LGPL$
** No Commercial Usage
** This file contains pre-release code and may not be distributed.
** You may use this file in accordance with the terms and conditions
** contained in the Technology Preview License Agreement accompanying
** this package.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 2.1 as published by the Free Software
** Foundation and appearing in the file LICENSE.LGPL included in the
** packaging of this file.  Please review the following information to
** ensure the GNU Lesser General Public License version 2.1 requirements
** will be met: http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, Nokia gives you certain additional
** rights.  These rights are described in the Nokia Qt LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
** If you have questions regarding the use of this file, please contact
** Nokia at qt-info@nokia.com.
**
**
**
**
**
**
**
**
** $QT_END_LICENSE$
**
****************************************************************************/

#include "qguiglformat_qpa.h"

#include <QtCore/qatomic.h>
#include <QtCore/QDebug>

class QGuiGLFormatPrivate
{
public:
    QGuiGLFormatPrivate()
        : ref(1)
        , opts(QGuiGLFormat::DoubleBuffer | QGuiGLFormat::WindowSurface)
        , redBufferSize(-1)
        , greenBufferSize(-1)
        , blueBufferSize(-1)
        , alphaBufferSize(-1)
        , depthSize(-1)
        , stencilSize(-1)
        , swapBehavior(QGuiGLFormat::DefaultSwapBehavior)
        , numSamples(-1)
    {
    }

    QGuiGLFormatPrivate(const QGuiGLFormatPrivate *other)
        : ref(1),
          opts(other->opts),
          redBufferSize(other->redBufferSize),
          greenBufferSize(other->greenBufferSize),
          blueBufferSize(other->blueBufferSize),
          alphaBufferSize(other->alphaBufferSize),
          depthSize(other->depthSize),
          stencilSize(other->stencilSize),
          swapBehavior(other->swapBehavior),
          numSamples(other->numSamples)
    {
    }

    QAtomicInt ref;
    QGuiGLFormat::FormatOptions opts;
    int redBufferSize;
    int greenBufferSize;
    int blueBufferSize;
    int alphaBufferSize;
    int depthSize;
    int stencilSize;
    QGuiGLFormat::SwapBehavior swapBehavior;
    int numSamples;
};

QGuiGLFormat::QGuiGLFormat()
{
    d = new QGuiGLFormatPrivate;
}

QGuiGLFormat::QGuiGLFormat(QGuiGLFormat::FormatOptions options)
{
    d = new QGuiGLFormatPrivate;
    d->opts = options;
}

/*!
    \internal
*/
void QGuiGLFormat::detach()
{
    if (d->ref != 1) {
        QGuiGLFormatPrivate *newd = new QGuiGLFormatPrivate(d);
        if (!d->ref.deref())
            delete d;
        d = newd;
    }
}

/*!
    Constructs a copy of \a other.
*/

QGuiGLFormat::QGuiGLFormat(const QGuiGLFormat &other)
{
    d = other.d;
    d->ref.ref();
}

/*!
    Assigns \a other to this object.
*/

QGuiGLFormat &QGuiGLFormat::operator=(const QGuiGLFormat &other)
{
    if (d != other.d) {
        other.d->ref.ref();
        if (!d->ref.deref())
            delete d;
        d = other.d;
    }
    return *this;
}

/*!
    Destroys the QGuiGLFormat.
*/
QGuiGLFormat::~QGuiGLFormat()
{
    if (!d->ref.deref())
        delete d;
}

/*!
    \fn bool QGuiGLFormat::stereo() const

    Returns true if stereo buffering is enabled; otherwise returns
    false. Stereo buffering is disabled by default.

    \sa setStereo()
*/

/*!
    If \a enable is true enables stereo buffering; otherwise disables
    stereo buffering.

    Stereo buffering is disabled by default.

    Stereo buffering provides extra color buffers to generate left-eye
    and right-eye images.

    \sa stereo()
*/

void QGuiGLFormat::setStereo(bool enable)
{
    if (enable) {
        d->opts |= QGuiGLFormat::StereoBuffers;
    } else {
        d->opts &= ~QGuiGLFormat::StereoBuffers;
    }
}

/*!
    Returns the number of samples per pixel when multisampling is
    enabled. By default, the highest number of samples that is
    available is used.

    \sa setSampleBuffers(), sampleBuffers(), setSamples()
*/
int QGuiGLFormat::samples() const
{
   return d->numSamples;
}

/*!
    Set the preferred number of samples per pixel when multisampling
    is enabled to \a numSamples. By default, the highest number of
    samples available is used.

    \sa setSampleBuffers(), sampleBuffers(), samples()
*/
void QGuiGLFormat::setSamples(int numSamples)
{
    detach();
    d->numSamples = numSamples;
}


/*!
    \fn bool QGuiGLFormat::hasWindowSurface() const

    Returns true if the corresponding widget has an instance of QWindowSurface.

    Otherwise returns false.

    WindowSurface is enabled by default.

    \sa setOverlay()
*/

void QGuiGLFormat::setWindowSurface(bool enable)
{
    if (enable) {
        d->opts |= QGuiGLFormat::WindowSurface;
    } else {
        d->opts &= ~QGuiGLFormat::WindowSurface;
    }
}

/*!
    Sets the format option to \a opt.

    \sa testOption()
*/

void QGuiGLFormat::setOption(QGuiGLFormat::FormatOptions opt)
{
    detach();
    d->opts |= opt;
}

/*!
    Returns true if format option \a opt is set; otherwise returns false.

    \sa setOption()
*/

bool QGuiGLFormat::testOption(QGuiGLFormat::FormatOptions opt) const
{
    return d->opts & opt;
}

/*!
    Set the minimum depth buffer size to \a size.

    \sa depthBufferSize(), setDepth(), depth()
*/
void QGuiGLFormat::setDepthBufferSize(int size)
{
    detach();
    d->depthSize = size;
}

/*!
    Returns the depth buffer size.

    \sa depth(), setDepth(), setDepthBufferSize()
*/
int QGuiGLFormat::depthBufferSize() const
{
   return d->depthSize;
}

void QGuiGLFormat::setSwapBehavior(SwapBehavior behavior)
{
    d->swapBehavior = behavior;
}

QGuiGLFormat::SwapBehavior QGuiGLFormat::swapBehavior() const
{
    return d->swapBehavior;
}

bool QGuiGLFormat::hasAlpha() const
{
    return d->alphaBufferSize > 0;
}

/*!
    Set the preferred stencil buffer size to \a size.

    \sa stencilBufferSize(), setStencil(), stencil()
*/
void QGuiGLFormat::setStencilBufferSize(int size)
{
    detach();
    d->stencilSize = size;
}

/*!
    Returns the stencil buffer size.

    \sa stencil(), setStencil(), setStencilBufferSize()
*/
int QGuiGLFormat::stencilBufferSize() const
{
   return d->stencilSize;
}

int QGuiGLFormat::redBufferSize() const
{
    return d->redBufferSize;
}

int QGuiGLFormat::greenBufferSize() const
{
    return d->greenBufferSize;
}

int QGuiGLFormat::blueBufferSize() const
{
    return d->blueBufferSize;
}

int QGuiGLFormat::alphaBufferSize() const
{
    return d->alphaBufferSize;
}

void QGuiGLFormat::setRedBufferSize(int size)
{
    d->redBufferSize = size;
}

void QGuiGLFormat::setGreenBufferSize(int size)
{
    d->greenBufferSize = size;
}

void QGuiGLFormat::setBlueBufferSize(int size)
{
    d->blueBufferSize = size;
}

void QGuiGLFormat::setAlphaBufferSize(int size)
{
    d->alphaBufferSize = size;
}

bool operator==(const QGuiGLFormat& a, const QGuiGLFormat& b)
{
    return (a.d == b.d) || ((int) a.d->opts == (int) b.d->opts
        && a.d->stencilSize == b.d->stencilSize
        && a.d->redBufferSize == b.d->redBufferSize
        && a.d->greenBufferSize == b.d->greenBufferSize
        && a.d->blueBufferSize == b.d->blueBufferSize
        && a.d->alphaBufferSize == b.d->alphaBufferSize
        && a.d->depthSize == b.d->depthSize
        && a.d->numSamples == b.d->numSamples
        && a.d->swapBehavior == b.d->swapBehavior);
}


/*!
    Returns false if all the options of the two QGuiGLFormat objects
    \a a and \a b are equal; otherwise returns true.

    \relates QGuiGLFormat
*/

bool operator!=(const QGuiGLFormat& a, const QGuiGLFormat& b)
{
    return !(a == b);
}

#ifndef QT_NO_DEBUG_STREAM
QDebug operator<<(QDebug dbg, const QGuiGLFormat &f)
{
    const QGuiGLFormatPrivate * const d = f.d;

    dbg.nospace() << "QGuiGLFormat("
                  << "options " << d->opts
                  << ", depthBufferSize " << d->depthSize
                  << ", redBufferSize " << d->redBufferSize
                  << ", greenBufferSize " << d->greenBufferSize
                  << ", blueBufferSize " << d->blueBufferSize
                  << ", alphaBufferSize " << d->alphaBufferSize
                  << ", stencilBufferSize " << d->stencilSize
                  << ", samples " << d->numSamples
                  << ", swapBehavior " << d->swapBehavior
                  << ')';

    return dbg.space();
}
#endif
