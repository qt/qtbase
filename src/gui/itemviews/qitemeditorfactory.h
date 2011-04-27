/****************************************************************************
**
** Copyright (C) 2011 Nokia Corporation and/or its subsidiary(-ies).
** All rights reserved.
** Contact: Nokia Corporation (qt-info@nokia.com)
**
** This file is part of the QtGui module of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:LGPL$
** No Commercial Usage
** This file contains pre-release code and may not be distributed.
** You may use this file in accordance with the terms and conditions
** contained in the Technology Preview License Agreement accompanying
** this package.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 2.1 as published by the Free Software
** Foundation and appearing in the file LICENSE.LGPL included in the
** packaging of this file.  Please review the following information to
** ensure the GNU Lesser General Public License version 2.1 requirements
** will be met: http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, Nokia gives you certain additional
** rights.  These rights are described in the Nokia Qt LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
** If you have questions regarding the use of this file, please contact
** Nokia at qt-info@nokia.com.
**
**
**
**
**
**
**
**
** $QT_END_LICENSE$
**
****************************************************************************/

#ifndef QITEMEDITORFACTORY_H
#define QITEMEDITORFACTORY_H

#include <QtCore/qmetaobject.h>
#include <QtCore/qbytearray.h>
#include <QtCore/qhash.h>
#include <QtCore/qvariant.h>

QT_BEGIN_HEADER

QT_BEGIN_NAMESPACE

QT_MODULE(Gui)

#ifndef QT_NO_ITEMVIEWS

class QWidget;

class Q_GUI_EXPORT QItemEditorCreatorBase
{
public:
    virtual ~QItemEditorCreatorBase() {}

    virtual QWidget *createWidget(QWidget *parent) const = 0;
    virtual QByteArray valuePropertyName() const = 0;
};

template <class T>
class QItemEditorCreator : public QItemEditorCreatorBase
{
public:
    inline QItemEditorCreator(const QByteArray &valuePropertyName);
    inline QWidget *createWidget(QWidget *parent) const { return new T(parent); }
    inline QByteArray valuePropertyName() const { return propertyName; }

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
    inline QWidget *createWidget(QWidget *parent) const { return new T(parent); }
    inline QByteArray valuePropertyName() const { return propertyName; }

private:
    QByteArray propertyName;
};


template <class T>
Q_INLINE_TEMPLATE QItemEditorCreator<T>::QItemEditorCreator(const QByteArray &avaluePropertyName)
    : propertyName(avaluePropertyName) {}

class Q_GUI_EXPORT QItemEditorFactory
{
public:
    inline QItemEditorFactory() {}
    virtual ~QItemEditorFactory();

    virtual QWidget *createEditor(QVariant::Type type, QWidget *parent) const;
    virtual QByteArray valuePropertyName(QVariant::Type type) const;

    void registerEditor(QVariant::Type type, QItemEditorCreatorBase *creator);

    static const QItemEditorFactory *defaultFactory();
    static void setDefaultFactory(QItemEditorFactory *factory);

private:
    QHash<QVariant::Type, QItemEditorCreatorBase *> creatorMap;
};

#endif // QT_NO_ITEMVIEWS

QT_END_NAMESPACE

QT_END_HEADER

#endif // QITEMEDITORFACTORY_H
