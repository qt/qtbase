/****************************************************************************
**
** Copyright (C) 2021 The Qt Company Ltd.
** Contact: https://www.qt.io/licensing/
**
** This file is part of the QtWidgets module of the Qt Toolkit.
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

#ifndef QDATAWIDGETMAPPER_H
#define QDATAWIDGETMAPPER_H

#include <QtWidgets/qtwidgetsglobal.h>
#include "QtCore/qobject.h"

QT_REQUIRE_CONFIG(datawidgetmapper);

QT_BEGIN_NAMESPACE

class QAbstractItemDelegate;
class QAbstractItemModel;
class QModelIndex;
class QDataWidgetMapperPrivate;

class Q_WIDGETS_EXPORT QDataWidgetMapper: public QObject
{
    Q_OBJECT

    Q_PROPERTY(int currentIndex READ currentIndex WRITE setCurrentIndex NOTIFY currentIndexChanged)
    Q_PROPERTY(Qt::Orientation orientation READ orientation WRITE setOrientation)
    Q_PROPERTY(SubmitPolicy submitPolicy READ submitPolicy WRITE setSubmitPolicy)

public:
    explicit QDataWidgetMapper(QObject *parent = nullptr);
    ~QDataWidgetMapper();

    void setModel(QAbstractItemModel *model);
    QAbstractItemModel *model() const;

    void setItemDelegate(QAbstractItemDelegate *delegate);
    QAbstractItemDelegate *itemDelegate() const;

    void setRootIndex(const QModelIndex &index);
    QModelIndex rootIndex() const;

    void setOrientation(Qt::Orientation aOrientation);
    Qt::Orientation orientation() const;

    enum SubmitPolicy { AutoSubmit, ManualSubmit };
    Q_ENUM(SubmitPolicy)
    void setSubmitPolicy(SubmitPolicy policy);
    SubmitPolicy submitPolicy() const;

    void addMapping(QWidget *widget, int section);
    void addMapping(QWidget *widget, int section, const QByteArray &propertyName);
    void removeMapping(QWidget *widget);
    int mappedSection(QWidget *widget) const;
    QByteArray mappedPropertyName(QWidget *widget) const;
    QWidget *mappedWidgetAt(int section) const;
    void clearMapping();

    int currentIndex() const;

public Q_SLOTS:
    void revert();
    bool submit();

    void toFirst();
    void toLast();
    void toNext();
    void toPrevious();
    virtual void setCurrentIndex(int index);
    void setCurrentModelIndex(const QModelIndex &index);

Q_SIGNALS:
    void currentIndexChanged(int index);

private:
    Q_DECLARE_PRIVATE(QDataWidgetMapper)
    Q_DISABLE_COPY(QDataWidgetMapper)
    Q_PRIVATE_SLOT(d_func(), void _q_dataChanged(const QModelIndex &, const QModelIndex &, const QVector<int> &))
    Q_PRIVATE_SLOT(d_func(), void _q_commitData(QWidget *))
    Q_PRIVATE_SLOT(d_func(), void _q_closeEditor(QWidget *, QAbstractItemDelegate::EndEditHint))
    Q_PRIVATE_SLOT(d_func(), void _q_modelDestroyed())
};

QT_END_NAMESPACE

#endif
