// Copyright (C) 2019 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#ifndef QCOLORTRANSFORM_P_H
#define QCOLORTRANSFORM_P_H

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

#include "qcolormatrix_p.h"
#include "qcolorspace_p.h"

#include <QtCore/qshareddata.h>
#include <QtGui/qrgbafloat.h>

QT_BEGIN_NAMESPACE

class QColorTransformPrivate : public QSharedData
{
public:
    QColorMatrix colorMatrix;
    QExplicitlySharedDataPointer<const QColorSpacePrivate> colorSpaceIn;
    QExplicitlySharedDataPointer<const QColorSpacePrivate> colorSpaceOut;

    static QColorTransformPrivate *get(const QColorTransform &q)
    { return q.d.data(); }

    void updateLutsIn() const;
    void updateLutsOut() const;
    bool isIdentity() const;

    Q_GUI_EXPORT void prepare();
    enum TransformFlag {
        Unpremultiplied = 0,
        InputOpaque = 1,
        InputPremultiplied = 2,
        OutputPremultiplied = 4,
        Premultiplied = (InputPremultiplied | OutputPremultiplied)
    };
    Q_DECLARE_FLAGS(TransformFlags, TransformFlag)

    void apply(QRgb *dst, const QRgb *src, qsizetype count, TransformFlags flags = Unpremultiplied) const;
    void apply(QRgba64 *dst, const QRgba64 *src, qsizetype count, TransformFlags flags = Unpremultiplied) const;
    void apply(QRgbaFloat32 *dst, const QRgbaFloat32 *src, qsizetype count,
               TransformFlags flags = Unpremultiplied) const;
    void apply(quint8 *dst, const QRgb *src, qsizetype count, TransformFlags flags = Unpremultiplied) const;
    void apply(quint16 *dst, const QRgba64 *src, qsizetype count, TransformFlags flags = Unpremultiplied) const;

    template<typename T>
    void apply(T *dst, const T *src, qsizetype count, TransformFlags flags) const;

    template<typename D, typename S>
    void applyReturnGray(D *dst, const S *src, qsizetype count, TransformFlags flags) const;

};

QT_END_NAMESPACE

#endif // QCOLORTRANSFORM_P_H
