/****************************************************************************
**
** Copyright (C) 2016 The Qt Company Ltd.
** Contact: https://www.qt.io/licensing/
**
** This file is part of the QtNetwork module of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:LGPL$
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company. For licensing terms
** and conditions see https://www.qt.io/terms-conditions. For further
** information use the contact form at https://www.qt.io/contact-us.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 3 as published by the Free Software
** Foundation and appearing in the file LICENSE.LGPL3 included in the
** packaging of this file. Please review the following information to
** ensure the GNU Lesser General Public License version 3 requirements
** will be met: https://www.gnu.org/licenses/lgpl-3.0.html.
**
** GNU General Public License Usage
** Alternatively, this file may be used under the terms of the GNU
** General Public License version 2.0 or (at your option) the GNU General
** Public license version 3 or any later version approved by the KDE Free
** Qt Foundation. The licenses are as published by the Free Software
** Foundation and appearing in the file LICENSE.GPL2 and LICENSE.GPL3
** included in the packaging of this file. Please review the following
** information to ensure the GNU General Public License requirements will
** be met: https://www.gnu.org/licenses/gpl-2.0.html and
** https://www.gnu.org/licenses/gpl-3.0.html.
**
** $QT_END_LICENSE$
**
****************************************************************************/

#include "hpacktable_p.h"

#include <QtCore/qdebug.h>

#include <algorithm>
#include <cstring>
#include <limits>


QT_BEGIN_NAMESPACE

namespace HPack
{

HeaderSize entry_size(const QByteArray &name, const QByteArray &value)
{
    // 32 comes from HPACK:
    // "4.1 Calculating Table Size
    // Note: The additional 32 octets account for an estimated overhead associated
    // with an entry. For example, an entry structure using two 64-bit pointers
    // to reference the name and the value of the entry and two 64-bit integers
    // for counting the number of references to the name and value would have
    // 32 octets of overhead."

    const unsigned sum = unsigned(name.size()) + value.size();
    if (std::numeric_limits<unsigned>::max() - 32 < sum)
        return HeaderSize();
    return HeaderSize(true, quint32(sum + 32));
}

namespace
{

int compare(const QByteArray &lhs, const QByteArray &rhs)
{
    if (const int minLen = std::min(lhs.size(), rhs.size())) {
        // We use memcmp, since strings in headers are allowed
        // to contain '\0'.
        const int cmp = std::memcmp(lhs.constData(), rhs.constData(), minLen);
        if (cmp)
            return cmp;
    }

    return lhs.size() - rhs.size();
}

} // unnamed namespace

FieldLookupTable::SearchEntry::SearchEntry()
    : field(),
      chunk(),
      offset(),
      table()
{
}

FieldLookupTable::SearchEntry::SearchEntry(const HeaderField *f,
                                           const Chunk *c,
                                           quint32 o,
                                           const FieldLookupTable *t)
    : field(f),
      chunk(c),
      offset(o),
      table(t)
{
    Q_ASSERT(field);
}

bool FieldLookupTable::SearchEntry::operator < (const SearchEntry &rhs)const
{
    Q_ASSERT(field);
    Q_ASSERT(rhs.field);

    int cmp = compare(field->name, rhs.field->name);
    if (cmp)
        return cmp < 0;

    cmp = compare(field->value, rhs.field->value);
    if (cmp)
        return cmp < 0;

    if (!chunk) // 'this' is not in the searchIndex.
        return rhs.chunk;

    if (!rhs.chunk) // not in the searchIndex.
        return false;

    Q_ASSERT(table);
    Q_ASSERT(rhs.table == table);

    const quint32 leftChunkIndex = table->indexOfChunk(chunk);
    const quint32 rightChunkIndex = rhs.table->indexOfChunk(rhs.chunk);

    // Later added - smaller is chunk index (since we push_front).
    if (leftChunkIndex != rightChunkIndex)
        return leftChunkIndex > rightChunkIndex;

    // Later added - smaller is offset.
    return offset > rhs.offset;
}

// This data is from HPACK's specs and it's quite
// conveniently sorted == works with binary search as it is.
// Later this can probably change and instead of simple
// vector we'll just reuse FieldLookupTable.
// TODO: it makes sense to generate this table while ...
// configuring/building Qt (some script downloading/parsing/generating
// would be quite handy).
const std::vector<HeaderField> &staticTable()
{
    static std::vector<HeaderField> table = {
    {":authority", ""},
    {":method", "GET"},
    {":method", "POST"},
    {":path", "/"},
    {":path", "/index.html"},
    {":scheme", "http"},
    {":scheme", "https"},
    {":status", "200"},
    {":status", "204"},
    {":status", "206"},
    {":status", "304"},
    {":status", "400"},
    {":status", "404"},
    {":status", "500"},
    {"accept-charset", ""},
    {"accept-encoding", "gzip, deflate"},
    {"accept-language", ""},
    {"accept-ranges", ""},
    {"accept", ""},
    {"access-control-allow-origin", ""},
    {"age", ""},
    {"allow", ""},
    {"authorization", ""},
    {"cache-control", ""},
    {"content-disposition", ""},
    {"content-encoding", ""},
    {"content-language", ""},
    {"content-length", ""},
    {"content-location", ""},
    {"content-range", ""},
    {"content-type", ""},
    {"cookie", ""},
    {"date", ""},
    {"etag", ""},
    {"expect", ""},
    {"expires", ""},
    {"from", ""},
    {"host", ""},
    {"if-match", ""},
    {"if-modified-since", ""},
    {"if-none-match", ""},
    {"if-range", ""},
    {"if-unmodified-since", ""},
    {"last-modified", ""},
    {"link", ""},
    {"location", ""},
    {"max-forwards", ""},
    {"proxy-authenticate", ""},
    {"proxy-authorization", ""},
    {"range", ""},
    {"referer", ""},
    {"refresh", ""},
    {"retry-after", ""},
    {"server", ""},
    {"set-cookie", ""},
    {"strict-transport-security", ""},
    {"transfer-encoding", ""},
    {"user-agent", ""},
    {"vary", ""},
    {"via", ""},
    {"www-authenticate", ""}
    };

    return table;
}

FieldLookupTable::FieldLookupTable(quint32 maxSize, bool use)
    : maxTableSize(maxSize),
      tableCapacity(maxSize),
      useIndex(use),
      nDynamic(),
      begin(),
      end(),
      dataSize()
{
}


bool FieldLookupTable::prependField(const QByteArray &name, const QByteArray &value)
{
    const auto entrySize = entry_size(name, value);
    if (!entrySize.first)
        return false;

    if (entrySize.second > tableCapacity) {
        clearDynamicTable();
        return true;
    }

    while (nDynamic && tableCapacity - dataSize < entrySize.second)
        evictEntry();

    if (!begin) {
        // Either no more space or empty table ...
        chunks.push_front(ChunkPtr(new Chunk(ChunkSize)));
        end += ChunkSize;
        begin = ChunkSize;
    }

    --begin;

    dataSize += entrySize.second;
    ++nDynamic;

    auto &newField = front();
    newField.name = name;
    newField.value = value;

    if (useIndex) {
        const auto result = searchIndex.insert(frontKey());
        Q_UNUSED(result) Q_ASSERT(result.second);
    }

    return true;
}

void FieldLookupTable::evictEntry()
{
    if (!nDynamic)
        return;

    Q_ASSERT(end != begin);

    if (useIndex) {
        const auto res = searchIndex.erase(backKey());
        Q_UNUSED(res) Q_ASSERT(res == 1);
    }

    const HeaderField &field = back();
    const auto entrySize = entry_size(field);
    Q_ASSERT(entrySize.first);
    Q_ASSERT(dataSize >= entrySize.second);
    dataSize -= entrySize.second;

    --nDynamic;
    --end;

    if (end == begin) {
        Q_ASSERT(chunks.size() == 1);
        end = ChunkSize;
        begin = end;
    } else if (!(end % ChunkSize)) {
        chunks.pop_back();
    }
}

quint32 FieldLookupTable::numberOfEntries() const
{
    return quint32(staticTable().size()) + nDynamic;
}

quint32 FieldLookupTable::numberOfStaticEntries() const
{
    return quint32(staticTable().size());
}

quint32 FieldLookupTable::numberOfDynamicEntries() const
{
    return nDynamic;
}

quint32 FieldLookupTable::dynamicDataSize() const
{
    return dataSize;
}

void FieldLookupTable::clearDynamicTable()
{
    searchIndex.clear();
    chunks.clear();
    begin = 0;
    end = 0;
    nDynamic = 0;
    dataSize = 0;
}

bool FieldLookupTable::indexIsValid(quint32 index) const
{
    return index && index <= staticTable().size() + nDynamic;
}

quint32 FieldLookupTable::indexOf(const QByteArray &name, const QByteArray &value)const
{
    // Start from the static part first:
    const auto &table = staticTable();
    const HeaderField field(name, value);
    const auto staticPos = std::lower_bound(table.begin(), table.end(), field,
                                            [](const HeaderField &lhs, const HeaderField &rhs) {
                                                int cmp = compare(lhs.name, rhs.name);
                                                if (cmp)
                                                    return cmp < 0;
                                                return compare(lhs.value, rhs.value) < 0;
                                            });
    if (staticPos != table.end()) {
        if (staticPos->name == name && staticPos->value == value)
            return staticPos - table.begin() + 1;
    }

    // Now we have to lookup in our dynamic part ...
    if (!useIndex) {
        qCritical("lookup in dynamic table requires search index enabled");
        return 0;
    }

    const SearchEntry key(&field, nullptr, 0, this);
    const auto pos = searchIndex.lower_bound(key);
    if (pos != searchIndex.end()) {
        const HeaderField &found = *pos->field;
        if (found.name == name && found.value == value)
            return keyToIndex(*pos);
    }

    return 0;
}

quint32 FieldLookupTable::indexOf(const QByteArray &name) const
{
    // Start from the static part first:
    const auto &table = staticTable();
    const HeaderField field(name, QByteArray());
    const auto staticPos = std::lower_bound(table.begin(), table.end(), field,
                                            [](const HeaderField &lhs, const HeaderField &rhs) {
                                                return compare(lhs.name, rhs.name) < 0;
                                            });
    if (staticPos != table.end()) {
        if (staticPos->name == name)
            return staticPos - table.begin() + 1;
    }

    // Now we have to lookup in our dynamic part ...
    if (!useIndex) {
        qCritical("lookup in dynamic table requires search index enabled");
        return 0;
    }

    const SearchEntry key(&field, nullptr, 0, this);
    const auto pos = searchIndex.lower_bound(key);
    if (pos != searchIndex.end()) {
        const HeaderField &found = *pos->field;
        if (found.name == name)
            return keyToIndex(*pos);
    }

    return 0;
}

bool FieldLookupTable::field(quint32 index, QByteArray *name, QByteArray *value) const
{
    Q_ASSERT(name);
    Q_ASSERT(value);

    if (!indexIsValid(index))
        return false;

    const auto &table = staticTable();
    if (index - 1 < table.size()) {
        *name = table[index - 1].name;
        *value = table[index - 1].value;
        return true;
    }

    index = index - 1 - quint32(table.size()) + begin;
    const auto chunkIndex = index / ChunkSize;
    Q_ASSERT(chunkIndex < chunks.size());
    const auto offset = index % ChunkSize;
    const HeaderField &found = (*chunks[chunkIndex])[offset];
    *name = found.name;
    *value = found.value;

    return true;
}

bool FieldLookupTable::fieldName(quint32 index, QByteArray *dst) const
{
    Q_ASSERT(dst);
    return field(index, dst, &dummyDst);
}

bool FieldLookupTable::fieldValue(quint32 index, QByteArray *dst) const
{
    Q_ASSERT(dst);
    return field(index, &dummyDst, dst);
}

const HeaderField &FieldLookupTable::front() const
{
    Q_ASSERT(nDynamic && begin != end && chunks.size());
    return (*chunks[0])[begin];
}

HeaderField &FieldLookupTable::front()
{
    Q_ASSERT(nDynamic && begin != end && chunks.size());
    return (*chunks[0])[begin];
}

const HeaderField &FieldLookupTable::back() const
{
    Q_ASSERT(nDynamic && end && end != begin);

    const quint32 absIndex = end - 1;
    const quint32 chunkIndex = absIndex / ChunkSize;
    Q_ASSERT(chunkIndex < chunks.size());
    const quint32 offset = absIndex % ChunkSize;
    return (*chunks[chunkIndex])[offset];
}

quint32 FieldLookupTable::indexOfChunk(const Chunk *chunk) const
{
    Q_ASSERT(chunk);

    for (size_type i = 0; i < chunks.size(); ++i) {
        if (chunks[i].get() == chunk)
            return quint32(i);
    }

    Q_UNREACHABLE();
    return 0;
}

quint32 FieldLookupTable::keyToIndex(const SearchEntry &key) const
{
    Q_ASSERT(key.chunk);

    const auto chunkIndex = indexOfChunk(key.chunk);
    const auto offset = key.offset;
    Q_ASSERT(offset < ChunkSize);
    Q_ASSERT(chunkIndex || offset >= begin);

    return quint32(offset + chunkIndex * ChunkSize - begin + 1 + staticTable().size());
}

FieldLookupTable::SearchEntry FieldLookupTable::frontKey() const
{
    Q_ASSERT(chunks.size() && end != begin);
    return SearchEntry(&front(), chunks.front().get(), begin, this);
}

FieldLookupTable::SearchEntry FieldLookupTable::backKey() const
{
    Q_ASSERT(chunks.size() && end != begin);

    const HeaderField &field = back();
    const quint32 absIndex = end - 1;
    const auto offset = absIndex % ChunkSize;
    const auto chunk = chunks[absIndex / ChunkSize].get();

    return SearchEntry(&field, chunk, offset, this);
}

bool FieldLookupTable::updateDynamicTableSize(quint32 size)
{
    if (!size) {
        clearDynamicTable();
        return true;
    }

    if (size > maxTableSize)
        return false;

    tableCapacity = size;
    while (nDynamic && dataSize > tableCapacity)
        evictEntry();

    return true;
}

void FieldLookupTable::setMaxDynamicTableSize(quint32 size)
{
    // This is for an external user, for example, HTTP2 protocol
    // layer that can receive SETTINGS frame from its peer.
    // No validity checks here, up to this external user.
    // We update max size and capacity (this can also result in
    // items evicted or even dynamic table completely cleared).
    maxTableSize = size;
    updateDynamicTableSize(size);
}

}

QT_END_NAMESPACE
