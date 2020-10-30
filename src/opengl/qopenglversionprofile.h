/****************************************************************************
**
** Copyright (C) 2020 The Qt Company Ltd.
** Contact: https://www.qt.io/licensing/
**
** This file is part of the QtOpenGL module of the Qt Toolkit.
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

#ifndef QOPENGLVERSIONPROFILE_H
#define QOPENGLVERSIONPROFILE_H

#include <QtOpenGL/qtopenglglobal.h>

#include <QtGui/QSurfaceFormat>

#include <QtCore/QPair>
#include <QtCore/qhashfunctions.h>

QT_BEGIN_NAMESPACE

class QOpenGLVersionProfilePrivate;
class QDebug;

class Q_OPENGL_EXPORT QOpenGLVersionProfile
{
public:
    QOpenGLVersionProfile();
    explicit QOpenGLVersionProfile(const QSurfaceFormat &format);
    QOpenGLVersionProfile(const QOpenGLVersionProfile &other);
    ~QOpenGLVersionProfile();

    QOpenGLVersionProfile &operator=(const QOpenGLVersionProfile &rhs);

    QPair<int, int> version() const;
    void setVersion(int majorVersion, int minorVersion);

    QSurfaceFormat::OpenGLContextProfile profile() const;
    void setProfile(QSurfaceFormat::OpenGLContextProfile profile);

    bool hasProfiles() const;
    bool isLegacyVersion() const;
    bool isValid() const;

private:
    QOpenGLVersionProfilePrivate* d;

    friend bool operator==(const QOpenGLVersionProfile &lhs, const QOpenGLVersionProfile &rhs) noexcept
    {
        if (lhs.profile() != rhs.profile())
            return false;
        return lhs.version() == rhs.version();
    }

    friend bool operator!=(const QOpenGLVersionProfile &lhs, const QOpenGLVersionProfile &rhs) noexcept
    {
        return !operator==(lhs, rhs);
    }
};

inline size_t qHash(const QOpenGLVersionProfile &v, size_t seed = 0) noexcept
{
    return qHash(static_cast<int>(v.profile() * 1000)
               + v.version().first * 100 + v.version().second * 10, seed);
}


#ifndef QT_NO_DEBUG_STREAM
Q_OPENGL_EXPORT QDebug operator<<(QDebug debug, const QOpenGLVersionProfile &vp);
#endif // !QT_NO_DEBUG_STREAM

QT_END_NAMESPACE

#endif // QOPENGLVERSIONPROFILE_H
