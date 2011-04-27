/****************************************************************************
**
** Copyright (C) 2011 Nokia Corporation and/or its subsidiary(-ies).
** All rights reserved.
** Contact: Nokia Corporation (qt-info@nokia.com)
**
** This file is part of the QtTest module of the Qt Toolkit.
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

#ifndef QTESTDATA_H
#define QTESTDATA_H

#include <QtTest/qtest_global.h>

#include <QtCore/qmetatype.h>
#include <QtCore/qstring.h>

QT_BEGIN_HEADER

QT_BEGIN_NAMESPACE

QT_MODULE(Test)

class QTestTable;
class QTestDataPrivate;

class Q_TESTLIB_EXPORT QTestData
{
public:
    ~QTestData();

    void append(int type, const void *data);
    void *data(int index) const;
    const char *dataTag() const;
    QTestTable *parent() const;
    int dataCount() const;

private:
    friend class QTestTable;
    QTestData(const char *tag = 0, QTestTable *parent = 0);

    Q_DISABLE_COPY(QTestData)

    QTestDataPrivate *d;
};

template<typename T>
QTestData &operator<<(QTestData &data, const T &value)
{
    data.append(qMetaTypeId<T>(), &value);
    return data;
}

inline QTestData &operator<<(QTestData &data, const char * value)
{
    QString str = QString::fromAscii(value);
    data.append(QMetaType::QString, &str);
    return data;
}

QT_END_NAMESPACE

QT_END_HEADER

#endif
