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

QT_BEGIN_NAMESPACE

class QColorTransformPrivate : public QSharedData
{
public:
    QColorMatrix colorMatrix;
    QExplicitlySharedDataPointer<const QColorSpacePrivate> colorSpaceIn;
    QExplicitlySharedDataPointer<const QColorSpacePrivate> colorSpaceOut;

    void updateLutsIn() const;
    void updateLutsOut() const;
    bool simpleGammaCorrection() const;

    void prepare();
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

    template<typename T>
    void apply(T *dst, const T *src, qsizetype count, TransformFlags flags) const;
};

QT_END_NAMESPACE

#endif // QCOLORTRANSFORM_P_H
