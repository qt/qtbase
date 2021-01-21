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

#ifndef QITEMEDITORFACTORY_H
#define QITEMEDITORFACTORY_H

#include <QtWidgets/qtwidgetsglobal.h>
#include <QtCore/qmetaobject.h>
#include <QtCore/qbytearray.h>
#include <QtCore/qhash.h>
#include <QtCore/qvariant.h>

QT_REQUIRE_CONFIG(itemviews);

QT_BEGIN_NAMESPACE

class QWidget;

class Q_WIDGETS_EXPORT QItemEditorCreatorBase
{
public:
    virtual ~QItemEditorCreatorBase();

    virtual QWidget *createWidget(QWidget *parent) const = 0;
    virtual QByteArray valuePropertyName() const = 0;
};

template <class T>
class QItemEditorCreator : public QItemEditorCreatorBase
{
public:
    inline explicit QItemEditorCreator(const QByteArray &valuePropertyName);
    inline QWidget *createWidget(QWidget *parent) const override { return new T(parent); }
    inline QByteArray valuePropertyName() const override { return propertyName; }

private:
    QByteArray propertyName;
};

template <class T>
class QStandardItemEditorCreator: public QItemEditorCreatorBase
{
public:
    inline QStandardItemEditorCreator()
        : propertyName(T::staticMetaObject.userProperty().name())
    {}
    inline QWidget *createWidget(QWidget *parent) const override { return new T(parent); }
    inline QByteArray valuePropertyName() const override { return propertyName; }

private:
    QByteArray propertyName;
};


template <class T>
Q_INLINE_TEMPLATE QItemEditorCreator<T>::QItemEditorCreator(const QByteArray &avaluePropertyName)
    : propertyName(avaluePropertyName) {}

class Q_WIDGETS_EXPORT QItemEditorFactory
{
public:
    inline QItemEditorFactory() {}
    virtual ~QItemEditorFactory();

    virtual QWidget *createEditor(int userType, QWidget *parent) const;
    virtual QByteArray valuePropertyName(int userType) const;

    void registerEditor(int userType, QItemEditorCreatorBase *creator);

    static const QItemEditorFactory *defaultFactory();
    static void setDefaultFactory(QItemEditorFactory *factory);

private:
    QHash<int, QItemEditorCreatorBase *> creatorMap;
};

QT_END_NAMESPACE

#endif // QITEMEDITORFACTORY_H
