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

#ifndef QCOLORSPACE_P_H
#define QCOLORSPACE_P_H

//
//  W A R N I N G
//  -------------
//
// This file is not part of the Qt API.  It exists purely as an
// implementation detail.  This header file may change from version to
// version without notice, or even be removed.
//
// We mean it.
//

#include "qcolorspace.h"
#include "qcolormatrix_p.h"
#include "qcolortrc_p.h"
#include "qcolortrclut_p.h"

#include <QtCore/qmutex.h>
#include <QtCore/qpoint.h>
#include <QtCore/qshareddata.h>

QT_BEGIN_NAMESPACE

class Q_AUTOTEST_EXPORT QColorSpacePrimaries
{
public:
   QColorSpacePrimaries() = default;
   QColorSpacePrimaries(QColorSpace::Primaries primaries);
   QColorSpacePrimaries(QPointF whitePoint,
                        QPointF redPoint,
                        QPointF greenPoint,
                        QPointF bluePoint)
           : whitePoint(whitePoint)
           , redPoint(redPoint)
           , greenPoint(greenPoint)
           , bluePoint(bluePoint)
   { }

   QColorMatrix toXyzMatrix() const;
   bool areValid() const;

   QPointF whitePoint;
   QPointF redPoint;
   QPointF greenPoint;
   QPointF bluePoint;
};

class QColorSpacePrivate : public QSharedData
{
public:
    QColorSpacePrivate();
    QColorSpacePrivate(QColorSpace::NamedColorSpace namedColorSpace);
    QColorSpacePrivate(QColorSpace::Primaries primaries, QColorSpace::TransferFunction transferFunction, float gamma);
    QColorSpacePrivate(const QColorSpacePrimaries &primaries, QColorSpace::TransferFunction transferFunction, float gamma);
    QColorSpacePrivate(const QColorSpacePrivate &other) = default;

    // named different from get to avoid accidental detachs
    static QColorSpacePrivate *getWritable(QColorSpace &colorSpace)
    {
        if (!colorSpace.d_ptr) {
            colorSpace.d_ptr = new QColorSpacePrivate;
            colorSpace.d_ptr->ref.ref();
        } else if (colorSpace.d_ptr->ref.loadRelaxed() != 1) {
            colorSpace.d_ptr->ref.deref();
            colorSpace.d_ptr = new QColorSpacePrivate(*colorSpace.d_ptr);
            colorSpace.d_ptr->ref.ref();
        }
        Q_ASSERT(colorSpace.d_ptr->ref.loadRelaxed() == 1);
        return colorSpace.d_ptr;
    }

    static const QColorSpacePrivate *get(const QColorSpace &colorSpace)
    {
        return colorSpace.d_ptr;
    }

    void initialize();
    void setToXyzMatrix();
    void setTransferFunction();
    void identifyColorSpace();
    QColorTransform transformationToColorSpace(const QColorSpacePrivate *out) const;

    static constexpr QColorSpace::NamedColorSpace Unknown = QColorSpace::NamedColorSpace(0);
    QColorSpace::NamedColorSpace namedColorSpace = Unknown;

    QColorSpace::Primaries primaries = QColorSpace::Primaries::Custom;
    QColorSpace::TransferFunction transferFunction = QColorSpace::TransferFunction::Custom;
    float gamma = 0.0f;
    QColorVector whitePoint;

    QColorTrc trc[3];
    QColorMatrix toXyz;

    QString description;
    QByteArray iccProfile;

    static QBasicMutex s_lutWriteLock;
    struct LUT {
        LUT() = default;
        ~LUT() = default;
        LUT(const LUT &other)
        {
            if (other.generated.loadAcquire()) {
                table[0] = other.table[0];
                table[1] = other.table[1];
                table[2] = other.table[2];
                generated.storeRelaxed(1);
            }
        }
        QSharedPointer<QColorTrcLut> &operator[](int i) { return table[i]; }
        const QSharedPointer<QColorTrcLut> &operator[](int i) const  { return table[i]; }
        QSharedPointer<QColorTrcLut> table[3];
        QAtomicInt generated;
    } mutable lut;
};

QT_END_NAMESPACE

#endif // QCOLORSPACE_P_H
