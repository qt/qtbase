/****************************************************************************
**
** Copyright (C) 2021 The Qt Company Ltd.
** Contact: https://www.qt.io/licensing/
**
** This file is part of the QtTest module of the Qt Toolkit.
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

#ifndef QTESTDATA_H
#define QTESTDATA_H

#include <QtTest/qttestglobal.h>

#include <QtCore/qmetatype.h>
#include <QtCore/qstring.h>

QT_BEGIN_NAMESPACE


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
    QTestData(const char *tag, QTestTable *parent);

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
    QString str = QString::fromUtf8(value);
    data.append(QMetaType::QString, &str);
    return data;
}

#ifdef QT_USE_QSTRINGBUILDER
template<typename A, typename B>
inline QTestData &operator<<(QTestData &data, const QStringBuilder<A, B> &value)
{
    return data << typename QConcatenable<QStringBuilder<A, B> >::ConvertTo(value);
}
#endif

QT_END_NAMESPACE

#endif
