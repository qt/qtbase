/****************************************************************************
**
** Copyright (C) 2011 Nokia Corporation and/or its subsidiary(-ies).
** All rights reserved.
** Contact: Nokia Corporation (qt-info@nokia.com)
**
** This file is part of the QtGui module of the Qt Toolkit.
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

#include "qsurfaceformat.h"

#include <QtCore/qatomic.h>
#include <QtCore/QDebug>

#ifdef major
#undef major
#endif

#ifdef minor
#undef minor
#endif

class QSurfaceFormatPrivate
{
public:
    explicit QSurfaceFormatPrivate(QSurfaceFormat::FormatOptions _opts = 0)
        : ref(1)
        , opts(_opts)
        , redBufferSize(-1)
        , greenBufferSize(-1)
        , blueBufferSize(-1)
        , alphaBufferSize(-1)
        , depthSize(-1)
        , stencilSize(-1)
        , swapBehavior(QSurfaceFormat::DefaultSwapBehavior)
        , numSamples(-1)
        , profile(QSurfaceFormat::NoProfile)
        , major(1)
        , minor(1)
    {
    }

    QSurfaceFormatPrivate(const QSurfaceFormatPrivate *other)
        : ref(1),
          opts(other->opts),
          redBufferSize(other->redBufferSize),
          greenBufferSize(other->greenBufferSize),
          blueBufferSize(other->blueBufferSize),
          alphaBufferSize(other->alphaBufferSize),
          depthSize(other->depthSize),
          stencilSize(other->stencilSize),
          swapBehavior(other->swapBehavior),
          numSamples(other->numSamples),
          profile(other->profile),
          major(other->major),
          minor(other->minor)
    {
    }

    QAtomicInt ref;
    QSurfaceFormat::FormatOptions opts;
    int redBufferSize;
    int greenBufferSize;
    int blueBufferSize;
    int alphaBufferSize;
    int depthSize;
    int stencilSize;
    QSurfaceFormat::SwapBehavior swapBehavior;
    int numSamples;
    QSurfaceFormat::OpenGLContextProfile profile;
    int major;
    int minor;
};

QSurfaceFormat::QSurfaceFormat() : d(new QSurfaceFormatPrivate)
{
}

QSurfaceFormat::QSurfaceFormat(QSurfaceFormat::FormatOptions options) :
    d(new QSurfaceFormatPrivate(options))
{
}

/*!
    \internal
*/
void QSurfaceFormat::detach()
{
    if (d->ref != 1) {
        QSurfaceFormatPrivate *newd = new QSurfaceFormatPrivate(d);
        if (!d->ref.deref())
            delete d;
        d = newd;
    }
}

/*!
    Constructs a copy of \a other.
*/

QSurfaceFormat::QSurfaceFormat(const QSurfaceFormat &other)
{
    d = other.d;
    d->ref.ref();
}

/*!
    Assigns \a other to this object.
*/

QSurfaceFormat &QSurfaceFormat::operator=(const QSurfaceFormat &other)
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
    Destroys the QSurfaceFormat.
*/
QSurfaceFormat::~QSurfaceFormat()
{
    if (!d->ref.deref())
        delete d;
}

/*!
    \fn bool QSurfaceFormat::stereo() const

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

void QSurfaceFormat::setStereo(bool enable)
{
    QSurfaceFormat::FormatOptions newOptions = d->opts;
    if (enable) {
        newOptions |= QSurfaceFormat::StereoBuffers;
    } else {
        newOptions &= ~QSurfaceFormat::StereoBuffers;
    }
    if (int(newOptions) != int(d->opts)) {
        detach();
        d->opts = newOptions;
    }
}

/*!
    Returns the number of samples per pixel when multisampling is
    enabled. By default, the highest number of samples that is
    available is used.

    \sa setSampleBuffers(), sampleBuffers(), setSamples()
*/
int QSurfaceFormat::samples() const
{
   return d->numSamples;
}

/*!
    Set the preferred number of samples per pixel when multisampling
    is enabled to \a numSamples. By default, the highest number of
    samples available is used.

    \sa setSampleBuffers(), sampleBuffers(), samples()
*/
void QSurfaceFormat::setSamples(int numSamples)
{
    if (d->numSamples != numSamples) {
        detach();
        d->numSamples = numSamples;
    }
}

/*!
    Sets the format option to \a opt.

    \sa testOption()
*/

void QSurfaceFormat::setOption(QSurfaceFormat::FormatOptions opt)
{
    const QSurfaceFormat::FormatOptions newOptions = d->opts | opt;
    if (int(newOptions) != int(d->opts)) {
        detach();
        d->opts = newOptions;
    }
}

/*!
    Returns true if format option \a opt is set; otherwise returns false.

    \sa setOption()
*/

bool QSurfaceFormat::testOption(QSurfaceFormat::FormatOptions opt) const
{
    return d->opts & opt;
}

/*!
    Set the minimum depth buffer size to \a size.

    \sa depthBufferSize(), setDepth(), depth()
*/
void QSurfaceFormat::setDepthBufferSize(int size)
{
    if (d->depthSize != size) {
        detach();
        d->depthSize = size;
    }
}

/*!
    Returns the depth buffer size.

    \sa depth(), setDepth(), setDepthBufferSize()
*/
int QSurfaceFormat::depthBufferSize() const
{
   return d->depthSize;
}

void QSurfaceFormat::setSwapBehavior(SwapBehavior behavior)
{
    if (d->swapBehavior != behavior) {
        detach();
        d->swapBehavior = behavior;
    }
}


QSurfaceFormat::SwapBehavior QSurfaceFormat::swapBehavior() const
{
    return d->swapBehavior;
}

bool QSurfaceFormat::hasAlpha() const
{
    return d->alphaBufferSize > 0;
}

/*!
    Set the preferred stencil buffer size to \a size.

    \sa stencilBufferSize(), setStencil(), stencil()
*/
void QSurfaceFormat::setStencilBufferSize(int size)
{
    if (d->stencilSize != size) {
        detach();
        d->stencilSize = size;
    }
}

/*!
    Returns the stencil buffer size.

    \sa stencil(), setStencil(), setStencilBufferSize()
*/
int QSurfaceFormat::stencilBufferSize() const
{
   return d->stencilSize;
}

int QSurfaceFormat::redBufferSize() const
{
    return d->redBufferSize;
}

int QSurfaceFormat::greenBufferSize() const
{
    return d->greenBufferSize;
}

int QSurfaceFormat::blueBufferSize() const
{
    return d->blueBufferSize;
}

int QSurfaceFormat::alphaBufferSize() const
{
    return d->alphaBufferSize;
}

void QSurfaceFormat::setRedBufferSize(int size)
{
    if (d->redBufferSize != size) {
        detach();
        d->redBufferSize = size;
    }
}

void QSurfaceFormat::setGreenBufferSize(int size)
{
    if (d->greenBufferSize != size) {
        detach();
        d->greenBufferSize = size;
    }
}

void QSurfaceFormat::setBlueBufferSize(int size)
{
    if (d->blueBufferSize != size) {
        detach();
        d->blueBufferSize = size;
    }
}

void QSurfaceFormat::setAlphaBufferSize(int size)
{
    if (d->alphaBufferSize != size) {
        detach();
        d->alphaBufferSize = size;
    }
}

/*!
   Sets the desired OpenGL context profile.

   This setting is ignored if the requested OpenGL version is
   less than 3.2.
*/
void QSurfaceFormat::setProfile(OpenGLContextProfile profile)
{
    if (d->profile != profile) {
        detach();
        d->profile = profile;
    }
}

QSurfaceFormat::OpenGLContextProfile QSurfaceFormat::profile() const
{
    return d->profile;
}

/*!
    Sets the desired major OpenGL version.
*/
void QSurfaceFormat::setMajorVersion(int major)
{
    d->major = major;
}

/*!
    Returns the major OpenGL version.
*/
int QSurfaceFormat::majorVersion() const
{
    return d->major;
}

/*!
    Sets the desired minor OpenGL version.
*/
void QSurfaceFormat::setMinorVersion(int minor)
{
    d->minor = minor;
}

/*!
    Returns the minor OpenGL version.
*/
int QSurfaceFormat::minorVersion() const
{
    return d->minor;
}

bool operator==(const QSurfaceFormat& a, const QSurfaceFormat& b)
{
    return (a.d == b.d) || ((int) a.d->opts == (int) b.d->opts
        && a.d->stencilSize == b.d->stencilSize
        && a.d->redBufferSize == b.d->redBufferSize
        && a.d->greenBufferSize == b.d->greenBufferSize
        && a.d->blueBufferSize == b.d->blueBufferSize
        && a.d->alphaBufferSize == b.d->alphaBufferSize
        && a.d->depthSize == b.d->depthSize
        && a.d->numSamples == b.d->numSamples
        && a.d->swapBehavior == b.d->swapBehavior
        && a.d->profile == b.d->profile);
}


/*!
    Returns false if all the options of the two QSurfaceFormat objects
    \a a and \a b are equal; otherwise returns true.

    \relates QSurfaceFormat
*/

bool operator!=(const QSurfaceFormat& a, const QSurfaceFormat& b)
{
    return !(a == b);
}

#ifndef QT_NO_DEBUG_STREAM
QDebug operator<<(QDebug dbg, const QSurfaceFormat &f)
{
    const QSurfaceFormatPrivate * const d = f.d;

    dbg.nospace() << "QSurfaceFormat("
                  << "options " << d->opts
                  << ", depthBufferSize " << d->depthSize
                  << ", redBufferSize " << d->redBufferSize
                  << ", greenBufferSize " << d->greenBufferSize
                  << ", blueBufferSize " << d->blueBufferSize
                  << ", alphaBufferSize " << d->alphaBufferSize
                  << ", stencilBufferSize " << d->stencilSize
                  << ", samples " << d->numSamples
                  << ", swapBehavior " << d->swapBehavior
                  << ", profile  " << d->profile
                  << ')';

    return dbg.space();
}
#endif
