// Copyright (C) 2020 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#include "qopenglversionprofile.h"

#include <QtCore/QDebug>

QT_BEGIN_NAMESPACE

class QOpenGLVersionProfilePrivate
{
public:
    QOpenGLVersionProfilePrivate()
        : majorVersion(0),
          minorVersion(0),
          profile(QSurfaceFormat::NoProfile)
    {}

    int majorVersion;
    int minorVersion;
    QSurfaceFormat::OpenGLContextProfile profile;
};

/*!
    \class QOpenGLVersionProfile
    \inmodule QtOpenGL
    \since 5.1
    \brief The QOpenGLVersionProfile class represents the version and if applicable
           the profile of an OpenGL context.

    An object of this class can be passed to QOpenGLContext::versionFunctions() to
    request a functions object for a specific version and profile of OpenGL.

    It also contains some helper functions to check if a version supports profiles
    or is a legacy version.
*/

/*!
    Creates a default invalid QOpenGLVersionProfile object.
*/
QOpenGLVersionProfile::QOpenGLVersionProfile()
    : d(new QOpenGLVersionProfilePrivate)
{
}

/*!
    Creates a QOpenGLVersionProfile object initialised with the version and profile
    from \a format.
*/
QOpenGLVersionProfile::QOpenGLVersionProfile(const QSurfaceFormat &format)
    : d(new QOpenGLVersionProfilePrivate)
{
    d->majorVersion = format.majorVersion();
    d->minorVersion = format.minorVersion();
    d->profile = format.profile();
}

/*!
    Constructs a copy of \a other.
*/
QOpenGLVersionProfile::QOpenGLVersionProfile(const QOpenGLVersionProfile &other)
    : d(new QOpenGLVersionProfilePrivate)
{
    *d = *(other.d);
}

/*!
    Destroys the QOpenGLVersionProfile object.
*/
QOpenGLVersionProfile::~QOpenGLVersionProfile()
{
    delete d;
}

/*!
    Assigns the version and profile of \a rhs to this QOpenGLVersionProfile object.
*/
QOpenGLVersionProfile &QOpenGLVersionProfile::operator=(const QOpenGLVersionProfile &rhs)
{
    if (this == &rhs)
        return *this;
    *d = *(rhs.d);
    return *this;
}

/*!
    Returns a QPair<int,int> where the components represent the major and minor OpenGL
    version numbers respectively.

    \sa setVersion()
*/
QPair<int, int> QOpenGLVersionProfile::version() const
{
    return qMakePair( d->majorVersion, d->minorVersion);
}

/*!
    Sets the major and minor version numbers to \a majorVersion and \a minorVersion respectively.

    \sa version()
*/
void QOpenGLVersionProfile::setVersion(int majorVersion, int minorVersion)
{
    d->majorVersion = majorVersion;
    d->minorVersion = minorVersion;
}

/*!
    Returns the OpenGL profile. Only makes sense if profiles are supported by this version.

    \sa setProfile()
*/
QSurfaceFormat::OpenGLContextProfile QOpenGLVersionProfile::profile() const
{
    return d->profile;
}

/*!
    Sets the OpenGL profile \a profile. Only makes sense if profiles are supported by
    this version.

    \sa profile()
*/
void QOpenGLVersionProfile::setProfile(QSurfaceFormat::OpenGLContextProfile profile)
{
    d->profile = profile;
}

/*!
    Returns \c true if profiles are supported by the OpenGL version returned by version(). Only
    OpenGL versions >= 3.2 support profiles.

    \sa profile(), version()
*/
bool QOpenGLVersionProfile::hasProfiles() const
{
    return ( d->majorVersion > 3
             || (d->majorVersion == 3 && d->minorVersion > 1));
}

/*!
    Returns \c true is the OpenGL version returned by version() contains deprecated functions
    and does not support profiles i.e. if the OpenGL version is <= 3.1.
*/
bool QOpenGLVersionProfile::isLegacyVersion() const
{
    return (d->majorVersion < 3 || (d->majorVersion == 3 && d->minorVersion == 0));
}

/*!
    Returns \c true if the version number is valid. Note that for a default constructed
    QOpenGLVersionProfile object this function will return \c false.

    \sa setVersion(), version()
*/
bool QOpenGLVersionProfile::isValid() const
{
    return d->majorVersion > 0 && d->minorVersion >= 0;
}

#ifndef QT_NO_DEBUG_STREAM
QDebug operator<<(QDebug debug, const QOpenGLVersionProfile &vp)
{
    QDebugStateSaver saver(debug);
    debug.nospace();
    debug << "QOpenGLVersionProfile(";
    if (vp.isValid()) {
        debug << vp.version().first << '.' << vp.version().second
            << ", profile=" << vp.profile();
    } else {
        debug << "invalid";
    }
    debug << ')';
    return debug;
}

#endif // QT_NO_DEBUG_STREAM
QT_END_NAMESPACE
