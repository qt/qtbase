/****************************************************************************
**
** Copyright (C) 2021 The Qt Company Ltd.
** Contact: https://www.qt.io/licensing/
**
** This file is part of the QtGui module of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:COMM$
**
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company. For licensing terms
** and conditions see https://www.qt.io/terms-conditions. For further
** information use the contact form at https://www.qt.io/contact-us.
**
** $QT_END_LICENSE$
**
**
**
**
**
**
**
**
**
**
**
**
**
**
**
**
**
**
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

    QColorSpace();
    QColorSpace(NamedColorSpace namedColorSpace);
    QColorSpace(Primaries primaries, TransferFunction transferFunction, float gamma = 0.0f);
    QColorSpace(Primaries primaries, float gamma);
    QColorSpace(const QPointF &whitePoint, const QPointF &redPoint,
                const QPointF &greenPoint, const QPointF &bluePoint,
                TransferFunction transferFunction, float gamma = 0.0f);
    ~QColorSpace();

    QColorSpace(const QColorSpace &colorSpace);
    QColorSpace &operator=(const QColorSpace &colorSpace);

    QColorSpace(QColorSpace &&colorSpace) noexcept
            : d_ptr(qExchange(colorSpace.d_ptr, nullptr))
    { }
    QColorSpace &operator=(QColorSpace &&colorSpace) noexcept
    {
        // Make the deallocation of this->d_ptr happen in ~QColorSpace()
        QColorSpace(std::move(colorSpace)).swap(*this);
        return *this;
    }

    void swap(QColorSpace &colorSpace) noexcept
    { qSwap(d_ptr, colorSpace.d_ptr); }

    Primaries primaries() const noexcept;
    TransferFunction transferFunction() const noexcept;
    float gamma() const noexcept;

    void setTransferFunction(TransferFunction transferFunction, float gamma = 0.0f);
    QColorSpace withTransferFunction(TransferFunction transferFunction, float gamma = 0.0f) const;

    void setPrimaries(Primaries primariesId);
    void setPrimaries(const QPointF &whitePoint, const QPointF &redPoint,
                      const QPointF &greenPoint, const QPointF &bluePoint);

    bool isValid() const noexcept;

    friend Q_GUI_EXPORT bool operator==(const QColorSpace &colorSpace1, const QColorSpace &colorSpace2);
    friend inline bool operator!=(const QColorSpace &colorSpace1, const QColorSpace &colorSpace2);

    static QColorSpace fromIccProfile(const QByteArray &iccProfile);
    QByteArray iccProfile() const;

    QColorTransform transformationToColorSpace(const QColorSpace &colorspace) const;

    operator QVariant() const;

private:
    Q_DECLARE_PRIVATE(QColorSpace)
    QColorSpacePrivate *d_ptr = nullptr;

#ifndef QT_NO_DEBUG_STREAM
    friend Q_GUI_EXPORT QDebug operator<<(QDebug dbg, const QColorSpace &colorSpace);
#endif
};

bool Q_GUI_EXPORT operator==(const QColorSpace &colorSpace1, const QColorSpace &colorSpace2);
inline bool operator!=(const QColorSpace &colorSpace1, const QColorSpace &colorSpace2)
{
    return !(colorSpace1 == colorSpace2);
}

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
