/****************************************************************************
**
** Copyright (C) 2021 The Qt Company Ltd.
** Contact: https://www.qt.io/licensing/
**
** This file is part of the QtSql module of the Qt Toolkit.
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

#ifndef QSQLCACHEDRESULT_P_H
#define QSQLCACHEDRESULT_P_H

//
//  W A R N I N G
//  -------------
//
// This file is not part of the Qt API.  It exists for the convenience
// of other Qt classes.  This header file may change from version to
// version without notice, or even be removed.
//
// We mean it.
//

#include <QtSql/private/qtsqlglobal_p.h>
#include "QtSql/qsqlresult.h"
#include "QtSql/private/qsqlresult_p.h"

QT_BEGIN_NAMESPACE

class QVariant;
template <typename T> class QVector;

class QSqlCachedResultPrivate;

class Q_SQL_EXPORT QSqlCachedResult: public QSqlResult
{
    Q_DECLARE_PRIVATE(QSqlCachedResult)

public:
    typedef QVector<QVariant> ValueCache;

protected:
    QSqlCachedResult(QSqlCachedResultPrivate &d);

    void init(int colCount);
    void cleanup();
    void clearValues();

    virtual bool gotoNext(ValueCache &values, int index) = 0;

    QVariant data(int i) override;
    bool isNull(int i) override;
    bool fetch(int i) override;
    bool fetchNext() override;
    bool fetchPrevious() override;
    bool fetchFirst() override;
    bool fetchLast() override;

    int colCount() const;
    ValueCache &cache();

    void virtual_hook(int id, void *data) override;
    void detachFromResultSet() override;
    void setNumericalPrecisionPolicy(QSql::NumericalPrecisionPolicy policy) override;
private:
    bool cacheNext();
};

class Q_SQL_EXPORT QSqlCachedResultPrivate: public QSqlResultPrivate
{
    Q_DECLARE_PUBLIC(QSqlCachedResult)

public:
    QSqlCachedResultPrivate(QSqlCachedResult *q, const QSqlDriver *drv);
    bool canSeek(int i) const;
    inline int cacheCount() const;
    void init(int count, bool fo);
    void cleanup();
    int nextIndex();
    void revertLast();

    QSqlCachedResult::ValueCache cache;
    int rowCacheEnd;
    int colCount;
    bool atEnd;
};

QT_END_NAMESPACE

#endif // QSQLCACHEDRESULT_P_H
