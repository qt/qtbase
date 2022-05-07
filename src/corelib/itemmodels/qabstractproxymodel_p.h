/****************************************************************************
**
** Copyright (C) 2022 The Qt Company Ltd.
** Contact: https://www.qt.io/licensing/
**
** This file is part of the QtCore module of the Qt Toolkit.
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
******************************************************************************/

#ifndef QABSTRACTPROXYMODEL_P_H
#define QABSTRACTPROXYMODEL_P_H

//
//  W A R N I N G
//  -------------
//
// This file is not part of the Qt API.  It exists for the convenience
// of QAbstractItemModel*.  This header file may change from version
// to version without notice, or even be removed.
//
// We mean it.
//
//

#include "private/qabstractitemmodel_p.h"
#include "private/qproperty_p.h"

QT_REQUIRE_CONFIG(proxymodel);

QT_BEGIN_NAMESPACE

class QAbstractProxyModelBindable : public QUntypedBindable
{
public:
    explicit QAbstractProxyModelBindable(QUntypedPropertyData *d,
                                         const QtPrivate::QBindableInterface *i)
        : QUntypedBindable(d, i)
    {
    }
};

class Q_CORE_EXPORT QAbstractProxyModelPrivate : public QAbstractItemModelPrivate
{
    Q_DECLARE_PUBLIC(QAbstractProxyModel)
public:
    QAbstractProxyModelPrivate() : QAbstractItemModelPrivate() { }
    void setModelForwarder(QAbstractItemModel *sourceModel)
    {
        q_func()->setSourceModel(sourceModel);
    }
    void modelChangedForwarder()
    {
        Q_EMIT q_func()->sourceModelChanged(QAbstractProxyModel::QPrivateSignal());
    }
    Q_OBJECT_COMPAT_PROPERTY_WITH_ARGS(QAbstractProxyModelPrivate, QAbstractItemModel *, model,
                                       &QAbstractProxyModelPrivate::setModelForwarder,
                                       &QAbstractProxyModelPrivate::modelChangedForwarder, nullptr)
    virtual void _q_sourceModelDestroyed();
    void mapDropCoordinatesToSource(int row, int column, const QModelIndex &parent,
                                    int *source_row, int *source_column, QModelIndex *source_parent) const;

    using ModelPropertyType = decltype(model);
};

namespace QtPrivate {

/*!
    The biggest trick for adding new QProperty binding support here is the
    getter for the sourceModel property (QAbstractProxyModel::sourceModel),
    which returns nullptr, while internally using a global staticEmptyModel()
    instance.
    This lead to inconsistency while binding to a proxy model without source
    model. The bound object would point to staticEmptyModel() instance, while
    sourceModel() getter returns nullptr.
    To solve this issue we need to implement a custom QBindableInterface, with
    custom getter and makeBinding methods, that would introduce the required
    logic.
*/

inline QAbstractItemModel *normalizePotentiallyEmptyModel(QAbstractItemModel *model)
{
    if (model == QAbstractItemModelPrivate::staticEmptyModel())
        return nullptr;
    return model;
}

class QBindableInterfaceForSourceModel
{
    using PropertyType = QAbstractProxyModelPrivate::ModelPropertyType;
    using Parent = QBindableInterfaceForProperty<PropertyType>;
    using T = typename PropertyType::value_type;

public:
    static constexpr QBindableInterface iface = {
        [](const QUntypedPropertyData *d, void *value) -> void {
            const auto val = static_cast<const PropertyType *>(d)->value();
            *static_cast<T *>(value) = normalizePotentiallyEmptyModel(val);
        },
        Parent::iface.setter,
        Parent::iface.getBinding,
        Parent::iface.setBinding,
        [](const QUntypedPropertyData *d,
           const QPropertyBindingSourceLocation &location) -> QUntypedPropertyBinding {
            return Qt::makePropertyBinding(
                    [d]() -> T {
                        return normalizePotentiallyEmptyModel(
                                static_cast<const PropertyType *>(d)->value());
                    },
                    location);
        },
        Parent::iface.setObserver,
        Parent::iface.metaType
    };
};

} // namespace QtPrivate

QT_END_NAMESPACE

#endif // QABSTRACTPROXYMODEL_P_H
