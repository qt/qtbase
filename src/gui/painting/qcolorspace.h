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
#include <QtCore/qobjectdefs.h>
#include <QtCore/qshareddata.h>
#include <QtCore/qvariant.h>

QT_BEGIN_NAMESPACE

class QColorSpacePrivate;
class QPointF;

QT_DECLARE_QESDP_SPECIALIZATION_DTOR_WITH_EXPORT(QColorSpacePrivate, Q_GUI_EXPORT)

class Q_GUI_EXPORT QColorSpace
{
    Q_GADGET
public:
    enum NamedColorSpace {
        SRgb = 1,
        SRgbLinear,
        AdobeRgb,
        DisplayP3,
        ProPhotoRgb
    };
    Q_ENUM(NamedColorSpace)
    enum class Primaries {
        Custom = 0,
        SRgb,
        AdobeRgb,
        DciP3D65,
        ProPhotoRgb
    };
    Q_ENUM(Primaries)
    enum class TransferFunction {
        Custom = 0,
        Linear,
        Gamma,
        SRgb,
        ProPhotoRgb
    };
    Q_ENUM(TransferFunction)

    QColorSpace() noexcept = default;
    QColorSpace(NamedColorSpace namedColorSpace);
    QColorSpace(Primaries primaries, TransferFunction transferFunction, float gamma = 0.0f);
    QColorSpace(Primaries primaries, float gamma);
    QColorSpace(Primaries primaries, const QList<uint16_t> &transferFunctionTable);
    QColorSpace(const QPointF &whitePoint, const QPointF &redPoint,
                const QPointF &greenPoint, const QPointF &bluePoint,
                TransferFunction transferFunction, float gamma = 0.0f);
    QColorSpace(const QPointF &whitePoint, const QPointF &redPoint,
                const QPointF &greenPoint, const QPointF &bluePoint,
                const QList<uint16_t> &transferFunctionTable);
    QColorSpace(const QPointF &whitePoint, const QPointF &redPoint,
                const QPointF &greenPoint, const QPointF &bluePoint,
                const QList<uint16_t> &redTransferFunctionTable,
                const QList<uint16_t> &greenTransferFunctionTable,
                const QList<uint16_t> &blueTransferFunctionTable);
    ~QColorSpace();

    QColorSpace(const QColorSpace &colorSpace) noexcept;
    QColorSpace &operator=(const QColorSpace &colorSpace) noexcept
    {
        QColorSpace copy(colorSpace);
        swap(copy);
        return *this;
    }

    QColorSpace(QColorSpace &&colorSpace) noexcept = default;
    QT_MOVE_ASSIGNMENT_OPERATOR_IMPL_VIA_MOVE_AND_SWAP(QColorSpace)

    void swap(QColorSpace &colorSpace) noexcept
    { qSwap(d_ptr, colorSpace.d_ptr); }

    Primaries primaries() const noexcept;
    TransferFunction transferFunction() const noexcept;
    float gamma() const noexcept;

    void setTransferFunction(TransferFunction transferFunction, float gamma = 0.0f);
    void setTransferFunction(const QList<uint16_t> &transferFunctionTable);
    void setTransferFunctions(const QList<uint16_t> &redTransferFunctionTable,
                              const QList<uint16_t> &greenTransferFunctionTable,
                              const QList<uint16_t> &blueTransferFunctionTable);
    QColorSpace withTransferFunction(TransferFunction transferFunction, float gamma = 0.0f) const;
    QColorSpace withTransferFunction(const QList<uint16_t> &transferFunctionTable) const;
    QColorSpace withTransferFunctions(const QList<uint16_t> &redTransferFunctionTable,
                                      const QList<uint16_t> &greenTransferFunctionTable,
                                      const QList<uint16_t> &blueTransferFunctionTable) const;

    void setPrimaries(Primaries primariesId);
    void setPrimaries(const QPointF &whitePoint, const QPointF &redPoint,
                      const QPointF &greenPoint, const QPointF &bluePoint);

    void detach();
    bool isValid() const noexcept;

    friend inline bool operator==(const QColorSpace &colorSpace1, const QColorSpace &colorSpace2)
    { return colorSpace1.equals(colorSpace2); }
    friend inline bool operator!=(const QColorSpace &colorSpace1, const QColorSpace &colorSpace2)
    { return !(colorSpace1 == colorSpace2); }

    static QColorSpace fromIccProfile(const QByteArray &iccProfile);
    QByteArray iccProfile() const;

    QColorTransform transformationToColorSpace(const QColorSpace &colorspace) const;

    operator QVariant() const;

private:
    friend class QColorSpacePrivate;
    bool equals(const QColorSpace &other) const;

    QExplicitlySharedDataPointer<QColorSpacePrivate> d_ptr;

#ifndef QT_NO_DEBUG_STREAM
    friend Q_GUI_EXPORT QDebug operator<<(QDebug dbg, const QColorSpace &colorSpace);
#endif
};

Q_DECLARE_SHARED(QColorSpace)

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
