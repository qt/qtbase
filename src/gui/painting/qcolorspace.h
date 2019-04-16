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

#ifndef QCOLORSPACE_H
#define QCOLORSPACE_H

#include <QtGui/qtguiglobal.h>
#include <QtGui/qcolortransform.h>
#include <QtCore/qshareddata.h>

QT_BEGIN_NAMESPACE

class QColorSpacePrivate;

class Q_GUI_EXPORT QColorSpace
{
    Q_GADGET
public:
    enum ColorSpaceId {
        Undefined = 0,
        Unknown = 1,
        SRgb,
        SRgbLinear,
        AdobeRgb,
        DisplayP3,
        ProPhotoRgb,
        Bt2020,
    };
    Q_ENUM(ColorSpaceId)
    enum class Gamut {
        Custom = 0,
        SRgb,
        AdobeRgb,
        DciP3D65,
        ProPhotoRgb,
        Bt2020,
    };
    Q_ENUM(Gamut)
    enum class TransferFunction {
        Custom = 0,
        Linear,
        Gamma,
        SRgb,
        ProPhotoRgb,
        Bt2020,
    };
    Q_ENUM(TransferFunction)

    QColorSpace(ColorSpaceId colorSpaceId = Undefined);
    QColorSpace(Gamut gamut, TransferFunction fun, float gamma = 0.0f);
    QColorSpace(Gamut gamut, float gamma);
    ~QColorSpace();

    QColorSpace(QColorSpace &&colorSpace);
    QColorSpace(const QColorSpace &colorSpace);
    QColorSpace &operator=(QColorSpace &&colorSpace);
    QColorSpace &operator=(const QColorSpace &colorSpace);

    ColorSpaceId colorSpaceId() const noexcept;
    Gamut gamut() const noexcept;
    TransferFunction transferFunction() const noexcept;
    float gamma() const noexcept;

    bool isValid() const noexcept;

    friend Q_GUI_EXPORT bool operator==(const QColorSpace &colorSpace1, const QColorSpace &colorSpace2);
    friend inline bool operator!=(const QColorSpace &colorSpace1, const QColorSpace &colorSpace2);

    static QColorSpace fromIccProfile(const QByteArray &iccProfile);
    QByteArray iccProfile() const;

    QColorTransform transformationToColorSpace(const QColorSpace &colorspace) const;

    QColorSpacePrivate *d_func();
    inline const QColorSpacePrivate *d_func() const { return d_ptr.constData(); }

private:
    friend class QColorSpacePrivate;
    QExplicitlySharedDataPointer<QColorSpacePrivate> d_ptr;
};

bool Q_GUI_EXPORT operator==(const QColorSpace &colorSpace1, const QColorSpace &colorSpace2);
inline bool operator!=(const QColorSpace &colorSpace1, const QColorSpace &colorSpace2)
{
    return !(colorSpace1 == colorSpace2);
}

// QColorSpace stream functions
#if !defined(QT_NO_DATASTREAM)
Q_GUI_EXPORT QDataStream &operator<<(QDataStream &, const QColorSpace &);
Q_GUI_EXPORT QDataStream &operator>>(QDataStream &, QColorSpace &);
#endif

#ifndef QT_NO_DEBUG_STREAM
Q_GUI_EXPORT QDebug operator<<(QDebug, const QColorSpace &);
#endif

QT_END_NAMESPACE

#endif // QCOLORSPACE_P_H
