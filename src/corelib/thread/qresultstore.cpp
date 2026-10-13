// Copyright (C) 2020 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only
// Qt-Security score:significant reason:default

#include "qresultstore.h"

QT_BEGIN_NAMESPACE

namespace QtPrivate {

/*!
  \internal

  Finds result in \a store by \a index
 */
static ResultIteratorBase findResult(const QMap<int, ResultItem> &store, int index)
{
    if (store.isEmpty())
        return ResultIteratorBase(store.end());
    QMap<int, ResultItem>::const_iterator it = store.lowerBound(index);

    // lowerBound returns either an iterator to the result or an iterator
    // to the nearest greater index. If the latter happens it might be
    // that the result is stored in a vector at the previous index.
    if (it == store.end()) {
        --it;
        if (it.value().isVector() == false) {
            return ResultIteratorBase(store.end());
        }
    } else {
        if (it.key() > index) {
            if (it == store.begin())
                return ResultIteratorBase(store.end());
            --it;
        }
    }

    const int vectorIndex = index - it.key();

    if (vectorIndex >= it.value().count())
        return ResultIteratorBase(store.end());
    else if (it.value().isVector() == false && vectorIndex != 0)
        return ResultIteratorBase(store.end());
    return ResultIteratorBase(it, vectorIndex);
}

/*!
  \class QtPrivate::ResultItem
  \internal
 */

/*!
  \class QtPrivate::ResultIteratorBase
  \internal
 */

/*!
  \class QtPrivate::ResultStoreBase
  \internal
 */

ResultIteratorBase::ResultIteratorBase()
 : mapIterator(QMap<int, ResultItem>::const_iterator()), m_vectorIndex(0) { }
ResultIteratorBase::ResultIteratorBase(QMap<int, ResultItem>::const_iterator _mapIterator, int _vectorIndex)
 : mapIterator(_mapIterator), m_vectorIndex(_vectorIndex) { }

int ResultIteratorBase::vectorIndex() const { return m_vectorIndex; }
int ResultIteratorBase::resultIndex() const { return mapIterator.key() + m_vectorIndex; }

ResultIteratorBase ResultIteratorBase::operator++()
{
    if (canIncrementVectorIndex()) {
        ++m_vectorIndex;
    } else {
        ++mapIterator;
        m_vectorIndex = 0;
    }
    return *this;
}

int ResultIteratorBase::batchSize() const
{
    return mapIterator.value().count();
}

void ResultIteratorBase::batchedAdvance()
{
    ++mapIterator;
    m_vectorIndex = 0;
}

bool ResultIteratorBase::isVector() const
{
    return mapIterator.value().isVector();
}

bool ResultIteratorBase::canIncrementVectorIndex() const
{
    return (m_vectorIndex + 1 < mapIterator.value().m_count);
}

bool ResultIteratorBase::isValid() const
{
    return mapIterator.value().isValid();
}

ResultStoreBase::ResultStoreBase()
    : insertIndex(0), resultCount(0), m_filterMode(false), filteredResults(0) { }

ResultStoreBase::~ResultStoreBase()
{
    // QFutureInterface's dtor must delete the contents of m_results.
    Q_ASSERT(m_results.isEmpty());
}

void ResultStoreBase::setFilterMode(bool enable)
{
    m_filterMode = enable;
}

bool ResultStoreBase::filterMode() const
{
    return m_filterMode;
}

void ResultStoreBase::syncResultCount()
{
    ResultIteratorBase it = resultAt(resultCount);
    while (it != end()) {
        resultCount += it.batchSize();
        it = resultAt(resultCount);
    }
}

void ResultStoreBase::insertResultItemIfValid(int index, ResultItem &resultItem)
{
    if (resultItem.isValid()) {
        m_results[index] = resultItem;
        syncResultCount();
    } else {
        filteredResults += resultItem.count();
    }
}

int ResultStoreBase::insertResultItem(int index, ResultItem &resultItem)
{
    int storeIndex;
    if (m_filterMode && index != -1 && index > insertIndex) {
        pendingResults[index] = resultItem;
        storeIndex = index;
    } else {
        storeIndex = updateInsertIndex(index, resultItem.count());
        insertResultItemIfValid(storeIndex - filteredResults, resultItem);
    }
    syncPendingResults();
    return storeIndex;
}

bool ResultStoreBase::containsValidResultItem(int index) const
{
    // index might refer to either visible or pending result
    const bool inPending = m_filterMode && index != -1 && index > insertIndex;
    const auto &store = inPending ? pendingResults : m_results;
    auto it = findResult(store, index);
    return it != ResultIteratorBase(store.end()) && it.isValid();
}

void ResultStoreBase::syncPendingResults()
{
    // check if we can insert any of the pending results:
    QMap<int, ResultItem>::iterator it = pendingResults.begin();
    while (it != pendingResults.end()) {
        int index = it.key();
        if (index != resultCount + filteredResults)
            break;

        ResultItem result = it.value();
        insertResultItemIfValid(index - filteredResults, result);
        pendingResults.erase(it);
        it = pendingResults.begin();
    }
}

int ResultStoreBase::addResult(int index, const void *result)
{
    ResultItem resultItem(result, 0); // 0 means "not a vector"
    return insertResultItem(index, resultItem);
}

int ResultStoreBase::addResults(int index, const void *results, int vectorSize, int totalCount)
{
    if (m_filterMode == false || vectorSize == totalCount) {
        Q_ASSERT(vectorSize != 0);
        ResultItem resultItem(results, vectorSize);
        return insertResultItem(index, resultItem);
    } else {
        if (vectorSize > 0) {
            ResultItem filteredIn(results, vectorSize);
            insertResultItem(index, filteredIn);
        }
        ResultItem filteredAway(nullptr, totalCount - vectorSize);
        return insertResultItem(index + vectorSize, filteredAway);
    }
}

ResultIteratorBase ResultStoreBase::begin() const
{
    return ResultIteratorBase(m_results.begin());
}

ResultIteratorBase ResultStoreBase::end() const
{
    return ResultIteratorBase(m_results.end());
}

bool ResultStoreBase::hasNextResult() const
{
    return begin() != end();
}

ResultIteratorBase ResultStoreBase::resultAt(int index) const
{
    return findResult(m_results, index);
}

bool ResultStoreBase::contains(int index) const
{
    return (resultAt(index) != end());
}

int ResultStoreBase::count() const
{
    return resultCount;
}

// returns the insert index, calling this function with
// index equal to -1 returns the next available index.
int ResultStoreBase::updateInsertIndex(int index, int _count)
{
    if (index == -1) {
        index = insertIndex;
        insertIndex += _count;
    } else {
        insertIndex = qMax(index + _count, insertIndex);
    }
    return index;
}

} // namespace QtPrivate

QT_END_NAMESPACE
