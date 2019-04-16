/****************************************************************************
**
** Copyright (C) 2018 The Qt Company Ltd.
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

#ifndef QCOLORTRANSFORM_H
#define QCOLORTRANSFORM_H

#include <QtGui/qtguiglobal.h>
#include <QtCore/qsharedpointer.h>
#include <QtGui/qrgb.h>

QT_BEGIN_NAMESPACE

class QColor;
class QRgba64;
class QColorSpacePrivate;
class QColorTransformPrivate;

class Q_GUI_EXPORT QColorTransform
{
public:
    QColorTransform() noexcept : d_ptr(nullptr) { }
    ~QColorTransform() noexcept;
    QColorTransform(const QColorTransform &colorTransform) noexcept
            : d_ptr(colorTransform.d_ptr)
    { }
    QColorTransform(QColorTransform &&colorTransform) noexcept
            : d_ptr(std::move(colorTransform.d_ptr))
    { }
    QColorTransform &operator=(const QColorTransform &other) noexcept
    {
        d_ptr = other.d_ptr;
        return *this;
    }
    QColorTransform &operator=(QColorTransform &&other) noexcept
    {
        d_ptr = std::move(other.d_ptr);
        return *this;
    }

    bool isNull() const { return d_ptr.isNull(); }

    QRgb map(const QRgb &argb) const;
    QRgba64 map(const QRgba64 &rgba64) const;
    QColor map(const QColor &color) const;

private:
    friend class QColorSpace;
    friend class QColorSpacePrivate;
    friend class QImage;

    Q_DECLARE_PRIVATE(QColorTransform)
    QSharedPointer<QColorTransformPrivate> d_ptr;
};

QT_END_NAMESPACE

#endif // QCOLORTRANSFORM_H
