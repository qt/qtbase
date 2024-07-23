// Copyright (C) 2024 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#ifndef QFONTVARIABLEAXIS_H
#define QFONTVARIABLEAXIS_H

#include <QtGui/qtguiglobal.h>
#include <QtGui/qfont.h>

#include <QtCore/qshareddata.h>

QT_BEGIN_NAMESPACE

class QFontVariableAxisPrivate;
QT_DECLARE_QESDP_SPECIALIZATION_DTOR_WITH_EXPORT(QFontVariableAxisPrivate, Q_GUI_EXPORT)

class Q_GUI_EXPORT QFontVariableAxis
{
    Q_GADGET
    Q_DECLARE_PRIVATE(QFontVariableAxis)

    Q_PROPERTY(QByteArray tag READ tagString CONSTANT)
    Q_PROPERTY(QString name READ name CONSTANT)
    Q_PROPERTY(qreal minimumValue READ minimumValue CONSTANT)
    Q_PROPERTY(qreal maximumValue READ maximumValue CONSTANT)
    Q_PROPERTY(qreal defaultValue READ defaultValue CONSTANT)
public:
    QFontVariableAxis();
    QFontVariableAxis(QFontVariableAxis &&other) noexcept = default;
    QFontVariableAxis(const QFontVariableAxis &axis);
    ~QFontVariableAxis();
    QT_MOVE_ASSIGNMENT_OPERATOR_IMPL_VIA_PURE_SWAP(QFontVariableAxis)
    void swap(QFontVariableAxis &other) noexcept
    {
        d_ptr.swap(other.d_ptr);
    }

    QFontVariableAxis &operator=(const QFontVariableAxis &axis);

    QFont::Tag tag() const;
    void setTag(QFont::Tag tag);

    QString name() const;
    void setName(const QString &name);

    qreal minimumValue() const;
    void setMinimumValue(qreal minimumValue);

    qreal maximumValue() const;
    void setMaximumValue(qreal maximumValue);

    qreal defaultValue() const;
    void setDefaultValue(qreal defaultValue);

private:
    QByteArray tagString() const { return tag().toString(); }
    void detach();

    QExplicitlySharedDataPointer<QFontVariableAxisPrivate> d_ptr;
};

Q_DECLARE_SHARED(QFontVariableAxis)

QT_END_NAMESPACE

#endif // QFONTVARIABLEAXIS_H

