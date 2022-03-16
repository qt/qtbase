// Copyright (C) 2018 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#ifndef QCOLORTRANSFORM_H
#define QCOLORTRANSFORM_H

#include <QtGui/qtguiglobal.h>
#include <QtGui/qrgb.h>
#include <QtCore/qshareddata.h>

QT_BEGIN_NAMESPACE

class QColor;
class QRgba64;
class QColorSpacePrivate;
class QColorTransformPrivate;
QT_DECLARE_QESDP_SPECIALIZATION_DTOR_WITH_EXPORT(QColorTransformPrivate, Q_GUI_EXPORT)

class QColorTransform
{
public:
    QColorTransform() noexcept = default;
    Q_GUI_EXPORT ~QColorTransform();
    Q_GUI_EXPORT QColorTransform(const QColorTransform &colorTransform) noexcept;
    QColorTransform(QColorTransform &&colorTransform) = default;
    QColorTransform &operator=(const QColorTransform &other) noexcept
    {
        QColorTransform{other}.swap(*this);
        return *this;
    }
    QT_MOVE_ASSIGNMENT_OPERATOR_IMPL_VIA_MOVE_AND_SWAP(QColorTransform)

    void swap(QColorTransform &other) noexcept { d.swap(other.d); }
    Q_GUI_EXPORT bool isIdentity() const noexcept;

    Q_GUI_EXPORT QRgb map(QRgb argb) const;
    Q_GUI_EXPORT QRgba64 map(QRgba64 rgba64) const;
    Q_GUI_EXPORT QColor map(const QColor &color) const;

    friend bool operator==(const QColorTransform &ct1, const QColorTransform &ct2)
    { return ct1.compare(ct2); }
    friend bool operator!=(const QColorTransform &ct1, const QColorTransform &ct2)
    { return !ct1.compare(ct2); }

private:
    friend class QColorSpacePrivate;
    friend class QColorTransformPrivate;
    Q_GUI_EXPORT bool compare(const QColorTransform &other) const;

    QExplicitlySharedDataPointer<QColorTransformPrivate> d;
};

Q_DECLARE_SHARED(QColorTransform)

QT_END_NAMESPACE

#endif // QCOLORTRANSFORM_H
