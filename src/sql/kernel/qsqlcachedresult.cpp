// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#include "private/qsqlcachedresult_p.h"

#include <qvariant.h>
#include <QtSql/private/qsqldriver_p.h>

QT_BEGIN_NAMESPACE

/*
   QSqlCachedResult is a convenience class for databases that only allow
   forward only fetching. It will cache all the results so we can iterate
   backwards over the results again.

   All you need to do is to inherit from QSqlCachedResult and reimplement
   gotoNext(). gotoNext() will have a reference to the internal cache and
   will give you an index where you can start filling in your data. Special
   case: If the user actually wants a forward-only query, idx will be -1
   to indicate that we are not interested in the actual values.
*/

static constexpr qsizetype initial_cache_size = 128;

void QSqlCachedResultPrivate::cleanup()
{
    cache.clear();
    atEnd = false;
    colCount = 0;
    rowCacheEnd = 0;
}

void QSqlCachedResultPrivate::init(int count, bool fo)
{
    Q_ASSERT(count);
    cleanup();
    forwardOnly = fo;
    colCount = count;
    if (fo) {
        cache.resize(count);
        rowCacheEnd = count;
    } else {
        cache.resize(initial_cache_size * count);
    }
}

int QSqlCachedResultPrivate::nextIndex()
{
    if (forwardOnly)
        return 0;
    int newIdx = rowCacheEnd;
    if (newIdx + colCount > cache.size())
        cache.resize(qMin(cache.size() * 2, cache.size() + 10000));
    rowCacheEnd += colCount;

    return newIdx;
}

bool QSqlCachedResultPrivate::canSeek(int i) const
{
    if (forwardOnly || i < 0)
        return false;
    return rowCacheEnd >= (i + 1) * colCount;
}

void QSqlCachedResultPrivate::revertLast()
{
    if (forwardOnly)
        return;
    rowCacheEnd -= colCount;
}

inline int QSqlCachedResultPrivate::cacheCount() const
{
    Q_ASSERT(!forwardOnly);
    Q_ASSERT(colCount);
    return rowCacheEnd / colCount;
}

//////////////

QSqlCachedResult::QSqlCachedResult(QSqlCachedResultPrivate &d)
    : QSqlResult(d)
{
}

void QSqlCachedResult::init(int colCount)
{
    Q_D(QSqlCachedResult);
    d->init(colCount, isForwardOnly());
}

bool QSqlCachedResult::fetch(int i)
{
    Q_D(QSqlCachedResult);
    if ((!isActive()) || (i < 0))
        return false;
    if (at() == i)
        return true;
    if (d->forwardOnly) {
        // speed hack - do not copy values if not needed
        if (at() > i || at() == QSql::AfterLastRow)
            return false;
        while(at() < i - 1) {
            if (!gotoNext(d->cache, -1))
                return false;
            setAt(at() + 1);
        }
        if (!gotoNext(d->cache, 0))
            return false;
        setAt(at() + 1);
        return true;
    }
    if (d->canSeek(i)) {
        setAt(i);
        return true;
    }
    if (d->rowCacheEnd > 0)
        setAt(d->cacheCount());
    while (at() < i + 1) {
        if (!cacheNext()) {
            if (d->canSeek(i))
                break;
            return false;
        }
    }
    setAt(i);

    return true;
}

bool QSqlCachedResult::fetchNext()
{
    Q_D(QSqlCachedResult);
    if (d->canSeek(at() + 1)) {
        setAt(at() + 1);
        return true;
    }
    return cacheNext();
}

bool QSqlCachedResult::fetchPrevious()
{
    return fetch(at() - 1);
}

bool QSqlCachedResult::fetchFirst()
{
    Q_D(QSqlCachedResult);
    if (d->forwardOnly && at() != QSql::BeforeFirstRow) {
        return false;
    }
    if (d->canSeek(0)) {
        setAt(0);
        return true;
    }
    return cacheNext();
}

bool QSqlCachedResult::fetchLast()
{
    Q_D(QSqlCachedResult);
    if (d->atEnd) {
        if (d->forwardOnly)
            return false;
        else
            return fetch(d->cacheCount() - 1);
    }

    int i = at();
    while (fetchNext())
        ++i; /* brute force */
    if (d->forwardOnly && at() == QSql::AfterLastRow) {
        setAt(i);
        return true;
    } else {
        return fetch(i);
    }
}

QVariant QSqlCachedResult::data(int i)
{
    Q_D(const QSqlCachedResult);
    int idx = d->forwardOnly ? i : at() * d->colCount + i;
    if (i >= d->colCount || i < 0 || at() < 0 || idx >= d->rowCacheEnd)
        return QVariant();

    return d->cache.at(idx);
}

bool QSqlCachedResult::isNull(int i)
{
    Q_D(const QSqlCachedResult);
    int idx = d->forwardOnly ? i : at() * d->colCount + i;
    if (i >= d->colCount || i < 0 || at() < 0 || idx >= d->rowCacheEnd)
        return true;

    return d->cache.at(idx).isNull();
}

void QSqlCachedResult::cleanup()
{
    Q_D(QSqlCachedResult);
    setAt(QSql::BeforeFirstRow);
    setActive(false);
    d->cleanup();
}

void QSqlCachedResult::clearValues()
{
    Q_D(QSqlCachedResult);
    setAt(QSql::BeforeFirstRow);
    d->rowCacheEnd = 0;
    d->atEnd = false;
}

bool QSqlCachedResult::cacheNext()
{
    Q_D(QSqlCachedResult);
    if (d->atEnd)
        return false;

    if (isForwardOnly()) {
        d->cache.resize(d->colCount);
    }

    if (!gotoNext(d->cache, d->nextIndex())) {
        d->revertLast();
        d->atEnd = true;
        return false;
    }
    setAt(at() + 1);
    return true;
}

int QSqlCachedResult::colCount() const
{
    Q_D(const QSqlCachedResult);
    return d->colCount;
}

QSqlCachedResult::ValueCache &QSqlCachedResult::cache()
{
    Q_D(QSqlCachedResult);
    return d->cache;
}

void QSqlCachedResult::virtual_hook(int id, void *data)
{
    QSqlResult::virtual_hook(id, data);
}

void QSqlCachedResult::detachFromResultSet()
{
    cleanup();
}

void QSqlCachedResult::setNumericalPrecisionPolicy(QSql::NumericalPrecisionPolicy policy)
{
    QSqlResult::setNumericalPrecisionPolicy(policy);
    cleanup();
}


QT_END_NAMESPACE
