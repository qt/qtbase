// Copyright (C) 2026 The Qt Company Ltd.
// Copyright (C) 2026 Andreas Bacher
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only
// Qt-Security score:significant reason:default
#include "qsql_firebird_array_p.h"

#include "qsql_firebird_helpers_p.h"

#include <QtCore/qvarlengtharray.h>

#include <firebird/impl/blr.h>   // BLR type constants for array element types

#include <algorithm>
#include <cstring>
#include <limits>

QT_BEGIN_NAMESPACE

// Firebird OO API types are used throughout; avoid full Firebird:: qualification.
using namespace Firebird;

using namespace Qt::StringLiterals;

// Query Firebird system tables to populate ISC_ARRAY_DESC using OO API directly
static bool lookupArrayDesc(IAttachment *att, ITransaction *tra, IStatus *iStatus,
                            IMaster *master, const QByteArray &relName,
                            const QByteArray &fldName, ISC_ARRAY_DESC *desc)
{
    std::memset(desc, 0, sizeof(ISC_ARRAY_DESC));

    // ISC_ARRAY_DESC expects space-padded names (31 chars)
    const QByteArray relPadded = relName.leftJustified(31, ' ');
    const QByteArray fldPadded = fldName.leftJustified(31, ' ');
    std::memcpy(desc->array_desc_relation_name, relPadded.constData(), 31);
    std::memcpy(desc->array_desc_field_name, fldPadded.constData(), 31);

    try {
        ThrowStatusWrapper st(iStatus);

        /* Run a metadata SELECT with relName/fldName bound as the two text
           parameters — identifiers may legally contain quotes or non-ASCII, so
           they are never spliced into the SQL text. rowFn(buf, meta) is invoked
           for each fetched row until it returns false or the cursor is
           exhausted. The statement, metadata and cursor handles are freed by
           FbGuard on every path (close on the happy path, release on a throw)
           — no manual unwinding needed. */
        auto runQuery = [&](const QByteArray &sql, auto rowFn) {
            FbGuard<IStatement> stmtG(
                att->prepare(&st, tra, 0, sql.constData(), SQL_DIALECT_V6,
                             IStatement::PREPARE_PREFETCH_METADATA),
                fbRelease<IStatement>);

            FbGuard<IMessageMetadata> inMetaG(stmtG->getInputMetadata(&st),
                                              fbRelease<IMessageMetadata>);
            IMessageMetadata *inMeta = inMetaG.get();
            QByteArray inBuf(inMeta->getMessageLength(&st), '\0');
            const QByteArray *binds[2] = { &relName, &fldName };
            const unsigned paramCount = std::min(inMeta->getCount(&st), 2u);
            for (unsigned i = 0; i < paramCount; ++i) {
                char *data = inBuf.data() + inMeta->getOffset(&st, i);
                *reinterpret_cast<short *>(inBuf.data()
                                           + inMeta->getNullOffset(&st, i)) = 0;
                encodeTextValue(data, static_cast<int>(inMeta->getType(&st, i)),
                                inMeta->getLength(&st, i), *binds[i]);
            }

            FbGuard<IMessageMetadata> metaG(stmtG->getOutputMetadata(&st),
                                            fbRelease<IMessageMetadata>);
            IMessageMetadata *meta = metaG.get();
            QByteArray outBuf(meta->getMessageLength(&st), '\0');
            FbGuard<IResultSet> rsG(
                stmtG->openCursor(&st, tra, inMeta,
                                  reinterpret_cast<unsigned char *>(inBuf.data()),
                                  meta, 0),
                fbRelease<IResultSet>);
            while (rsG->fetchNext(&st, outBuf.data()) == IStatus::RESULT_OK) {
                if (!rowFn(outBuf, meta))
                    break;
            }
            rsG.closeWith([&](IResultSet *r) { r->close(&st); });
            stmtG.closeWith([&](IStatement *s) { s->free(&st); });
        };

        /* Query 1: field type/scale/length and dimension count. TRIM for
           matching — preserves original case for quoted identifiers. */
        const QByteArray sql1 = QByteArrayLiteral(
            "SELECT f.RDB$FIELD_TYPE, f.RDB$FIELD_SCALE, f.RDB$FIELD_LENGTH, "
            "f.RDB$DIMENSIONS "
            "FROM RDB$RELATION_FIELDS rf "
            "JOIN RDB$FIELDS f ON f.RDB$FIELD_NAME = rf.RDB$FIELD_SOURCE "
            "WHERE TRIM(rf.RDB$RELATION_NAME) = ? AND TRIM(rf.RDB$FIELD_NAME) = ?");

        bool found = false;
        runQuery(sql1, [&](const QByteArray &buf, IMessageMetadata *meta) {
            const unsigned o0 = meta->getOffset(&st, 0);
            const unsigned o1 = meta->getOffset(&st, 1);
            const unsigned o2 = meta->getOffset(&st, 2);
            const unsigned o3 = meta->getOffset(&st, 3);
            desc->array_desc_dtype = static_cast<ISC_UCHAR>(
                *reinterpret_cast<const short *>(buf.constData() + o0));
            desc->array_desc_scale = static_cast<ISC_SCHAR>(
                *reinterpret_cast<const short *>(buf.constData() + o1));
            desc->array_desc_length = static_cast<unsigned short>(
                *reinterpret_cast<const short *>(buf.constData() + o2));
            desc->array_desc_dimensions = static_cast<short>(
                *reinterpret_cast<const short *>(buf.constData() + o3));
            found = true;
            return false; // only the first row is needed
        });

        /* Reject dimension counts the array_desc_bounds array cannot hold
           (Firebird DDL enforces the same maximum; more means a corrupt or
           hostile database). */
        if (!found || desc->array_desc_dimensions <= 0
            || desc->array_desc_dimensions
                   > static_cast<int>(std::size(desc->array_desc_bounds)))
            return false;

        // Query 2: dimension bounds.
        const QByteArray sql2 = QByteArrayLiteral(
            "SELECT fd.RDB$DIMENSION, fd.RDB$LOWER_BOUND, fd.RDB$UPPER_BOUND "
            "FROM RDB$RELATION_FIELDS rf "
            "JOIN RDB$FIELDS f ON f.RDB$FIELD_NAME = rf.RDB$FIELD_SOURCE "
            "JOIN RDB$FIELD_DIMENSIONS fd ON fd.RDB$FIELD_NAME = f.RDB$FIELD_NAME "
            "WHERE TRIM(rf.RDB$RELATION_NAME) = ? AND TRIM(rf.RDB$FIELD_NAME) = ? "
            "ORDER BY fd.RDB$DIMENSION");

        runQuery(sql2, [&](const QByteArray &buf, IMessageMetadata *meta) {
            const unsigned d0 = meta->getOffset(&st, 0);
            const unsigned d1 = meta->getOffset(&st, 1);
            const unsigned d2 = meta->getOffset(&st, 2);
            const int dim = *reinterpret_cast<const short *>(buf.constData() + d0);
            if (dim >= 0 && dim < 16) {
                desc->array_desc_bounds[dim].array_bound_lower =
                    *reinterpret_cast<const short *>(buf.constData() + d1);
                desc->array_desc_bounds[dim].array_bound_upper =
                    *reinterpret_cast<const short *>(buf.constData() + d2);
            }
            return true; // read every row
        });

    } catch (const FbException &e) {
        const auto lerr = fbError(master, e.getStatus(), QSqlError::StatementError);
        qCWarning(lcFirebird) << "lookupArrayDesc:" << fbErrorLog(lerr);
        iStatus->init();
        return false;
    }

    return true;
}

/*! \internal
    A column's array descriptor is immutable while the statement lives (it
    mirrors the column's DDL), so resolve it through a per-result cache:
    lookupArrayDesc costs two server-side metadata SELECTs per call, which would
    otherwise be paid for every row fetched and every parameter bound.
*/
static bool cachedArrayDesc(IAttachment *att, ITransaction *tra, IStatus *iStatus,
                            IMaster *master,
                            ArrayDescCache &descCache,
                            const QByteArray &relName, const QByteArray &fldName,
                            ISC_ARRAY_DESC *desc)
{
    const ArrayFieldKey key = { relName, fldName };
    const auto it = descCache.constFind(key);
    if (it != descCache.constEnd()) {
        *desc = it.value();
        return true;
    }
    if (!lookupArrayDesc(att, tra, iStatus, master, relName, fldName, desc))
        return false;
    descCache.insert(key, *desc);
    return true;
}

/*! \internal
    Validate the descriptor's dimension bounds and compute the per-dimension
    element counts and the total slice byte length in 64-bit: bounds come from
    system tables, and a wrapped or non-positive total would otherwise allocate
    a short buffer that the read/write helpers overrun. get/putSlice take an
    int length, so INT_MAX is the hard upper limit. Shared by fetchArray and
    writeArray so the safety checks cannot drift apart.
*/
static bool arraySliceSize(const ISC_ARRAY_DESC &desc, const char *who,
                           const QByteArray &relName, const QByteArray &fldName,
                           QVarLengthArray<int> *numElements, qint64 *bufLen)
{
    qint64 arraySize = 1;
    const short dimensions = desc.array_desc_dimensions;
    if (numElements)
        numElements->resize(dimensions);
    for (short i = 0; i < dimensions; ++i) {
        const int sub = desc.array_desc_bounds[i].array_bound_upper -
                        desc.array_desc_bounds[i].array_bound_lower + 1;
        if (sub <= 0) {
            qCWarning(lcFirebird, "%s: invalid bounds for dimension %d of %s.%s",
                      who, i, relName.constData(), fldName.constData());
            return false;
        }
        if (numElements)
            (*numElements)[i] = sub;
        arraySize *= sub;
    }
    *bufLen = qint64(desc.array_desc_length) * arraySize;
    if (*bufLen <= 0 || *bufLen > (std::numeric_limits<int>::max)()) {
        qCWarning(lcFirebird, "%s: array %s.%s too large (%lld bytes)",
                  who, relName.constData(), fldName.constData(),
                  static_cast<long long>(*bufLen));
        return false;
    }
    return true;
}

// Recursive: parse flat buffer into QVariantList according to array dimensions
static const char *readArrayBuffer(QList<QVariant> &list, const char *buffer,
                                   short curDim, const int *numElements,
                                   ISC_ARRAY_DESC *desc, IMaster *master,
                                   IStatus *iStatus)
{
    const short dims = desc->array_desc_dimensions;
    const unsigned char dtype = desc->array_desc_dtype;
    unsigned short strLen = desc->array_desc_length;

    if (curDim < dims - 1) {
        // Non-leaf dimension: build sublists
        for (int i = 0; i < numElements[curDim]; ++i) {
            QList<QVariant> subList;
            buffer = readArrayBuffer(subList, buffer, curDim + 1, numElements, desc, master,
                                     iStatus);
            list.append(QVariant(subList));
        }
    } else {
        // Leaf dimension: read actual values directly into list
        switch (dtype) {
        case blr_varying:
        case blr_text: {
            for (int i = 0; i < numElements[curDim]; ++i) {
                // Trim trailing spaces and nulls (text/varying delivered as blr_text via SDL)
                int o = strLen;
                while (o > 0 && (buffer[o - 1] == ' ' || buffer[o - 1] == '\0'))
                    --o;
                list.append(QString::fromUtf8(buffer, o));
                buffer += strLen;
            }
            break;
        }
        case blr_short:
            for (int i = 0; i < numElements[curDim]; ++i) {
                list.append(static_cast<int>(*reinterpret_cast<const short *>(buffer)));
                buffer += sizeof(short);
            }
            break;
        case blr_long:
            for (int i = 0; i < numElements[curDim]; ++i) {
                list.append(*reinterpret_cast<const int *>(buffer));
                buffer += sizeof(int);
            }
            break;
        case blr_int64:
            for (int i = 0; i < numElements[curDim]; ++i) {
                list.append(*reinterpret_cast<const qint64 *>(buffer));
                buffer += sizeof(qint64);
            }
            break;
        case blr_float:
            for (int i = 0; i < numElements[curDim]; ++i) {
                list.append(static_cast<double>(*reinterpret_cast<const float *>(buffer)));
                buffer += sizeof(float);
            }
            break;
        case blr_double:
        case blr_d_float:
            for (int i = 0; i < numElements[curDim]; ++i) {
                list.append(*reinterpret_cast<const double *>(buffer));
                buffer += sizeof(double);
            }
            break;
        case blr_sql_date:
            for (int i = 0; i < numElements[curDim]; ++i) {
                list.append(decodeFirebirdDate(master->getUtilInterface(),
                                               *reinterpret_cast<const ISC_DATE *>(buffer)));
                buffer += sizeof(ISC_DATE);
            }
            break;
        case blr_sql_time:
            for (int i = 0; i < numElements[curDim]; ++i) {
                list.append(decodeFirebirdTime(master->getUtilInterface(),
                                               *reinterpret_cast<const ISC_TIME *>(buffer)));
                buffer += sizeof(ISC_TIME);
            }
            break;
        case blr_timestamp:
            for (int i = 0; i < numElements[curDim]; ++i) {
                list.append(decodeFirebirdTimestamp(master->getUtilInterface(),
                                                   *reinterpret_cast<const ISC_TIMESTAMP *>(buffer)));
                buffer += sizeof(ISC_TIMESTAMP);
            }
            break;
        case blr_timestamp_tz:
            for (int i = 0; i < numElements[curDim]; ++i) {
                list.append(decodeFirebirdTimestampTz(iStatus, master->getUtilInterface(),
                                                     *reinterpret_cast<const ISC_TIMESTAMP_TZ *>(buffer)));
                buffer += sizeof(ISC_TIMESTAMP_TZ);
            }
            break;
        case blr_bool:
            for (int i = 0; i < numElements[curDim]; ++i) {
                list.append(*reinterpret_cast<const FB_BOOLEAN *>(buffer) != FB_FALSE);
                buffer += sizeof(FB_BOOLEAN);
            }
            break;
        default:
            qCWarning(lcFirebird, "readArrayBuffer: unsupported BLR type %d", dtype);
            buffer += strLen * numElements[curDim];
            break;
        }
    }
    return buffer;
}

// Recursive: serialize QVariantList into flat buffer
static char *createArrayBuffer(char *buffer, const QList<QVariant> &list,
                               QMetaType::Type type, short curDim,
                               ISC_ARRAY_DESC *desc, IMaster *master,
                               IStatus *iStatus, QString &error)
{
    ISC_ARRAY_BOUND *bounds = desc->array_desc_bounds;
    short dim = desc->array_desc_dimensions - 1;

    const qsizetype elements = bounds[curDim].array_bound_upper
                               - bounds[curDim].array_bound_lower + 1;
    if (list.size() != elements) {
        error = QString::fromLatin1("Array size mismatch: expected %1, got %2")
                    .arg(elements).arg(list.size());
        return nullptr;
    }

    if (curDim != dim) {
        for (const auto &elem : list) {
            if (elem.typeId() != QMetaType::QVariantList) {
                error = u"Array dimensions mismatch"_s;
                return nullptr;
            }
            buffer = createArrayBuffer(buffer, elem.toList(), type, curDim + 1, desc, master,
                                       iStatus, error);
            if (!buffer)
                return nullptr;
        }
    } else {
        switch (type) {
        case QMetaType::Short:
        case QMetaType::UShort:
        case QMetaType::Int:
        case QMetaType::UInt:
            if (desc->array_desc_dtype == blr_short) {
                for (const auto &v : list) {
                    *reinterpret_cast<short *>(buffer) = static_cast<short>(v.toInt());
                    buffer += sizeof(short);
                }
            } else {
                for (const auto &v : list) {
                    *reinterpret_cast<int *>(buffer) = v.toInt();
                    buffer += sizeof(int);
                }
            }
            break;
        case QMetaType::Float:
        case QMetaType::Double:
            if (desc->array_desc_dtype == blr_float) {
                for (const auto &v : list) {
                    *reinterpret_cast<float *>(buffer) = static_cast<float>(v.toDouble());
                    buffer += sizeof(float);
                }
            } else {
                for (const auto &v : list) {
                    *reinterpret_cast<double *>(buffer) = v.toDouble();
                    buffer += sizeof(double);
                }
            }
            break;
        case QMetaType::LongLong:
            for (const auto &v : list) {
                *reinterpret_cast<qint64 *>(buffer) = v.toLongLong();
                buffer += sizeof(qint64);
            }
            break;
        case QMetaType::QString:
            for (const auto &v : list) {
                const QByteArray utf8 = v.toString().toUtf8();
                unsigned short len = desc->array_desc_length;
                if (desc->array_desc_dtype == blr_varying || desc->array_desc_dtype == blr_text) {
                    // Both varying and text use blr_text format in SDL: raw data, space-padded
                    int copyLen = std::min(static_cast<int>(utf8.size()), static_cast<int>(len));
                    std::memcpy(buffer, utf8.constData(), copyLen);
                    if (copyLen < static_cast<int>(len))
                        std::memset(buffer + copyLen, ' ', len - copyLen);
                    buffer += len;
                }
            }
            break;
        case QMetaType::QDate:
            for (const auto &v : list) {
                *reinterpret_cast<ISC_DATE *>(buffer) =
                    encodeQDate(master->getUtilInterface(), v.toDate());
                buffer += sizeof(ISC_DATE);
            }
            break;
        case QMetaType::QTime:
            for (const auto &v : list) {
                *reinterpret_cast<ISC_TIME *>(buffer) =
                    encodeQTime(master->getUtilInterface(), v.toTime());
                buffer += sizeof(ISC_TIME);
            }
            break;
        case QMetaType::QDateTime:
            for (const auto &v : list) {
                QDateTime dt = v.toDateTime();
                if (desc->array_desc_dtype == blr_timestamp_tz) {
                    *reinterpret_cast<ISC_TIMESTAMP_TZ *>(buffer) =
                        encodeQDateTimeTz(iStatus, master->getUtilInterface(), dt);
                    buffer += sizeof(ISC_TIMESTAMP_TZ);
                } else {
                    *reinterpret_cast<ISC_TIMESTAMP *>(buffer) =
                        encodeQDateTime(master->getUtilInterface(), dt);
                    buffer += sizeof(ISC_TIMESTAMP);
                }
            }
            break;
        case QMetaType::Bool:
            for (const auto &v : list) {
                *reinterpret_cast<FB_BOOLEAN *>(buffer) = v.toBool() ? FB_TRUE : FB_FALSE;
                buffer += sizeof(FB_BOOLEAN);
            }
            break;
        default:
            error = QString::fromLatin1("Unsupported array element type %1").arg(static_cast<int>(type));
            return nullptr;
        }
    }
    return buffer;
}

/*! \internal
    Build SDL (Slice Description Language) binary for array get/putSlice.
    Supports identifiers up to 255 chars (unlike isc_array_gen_sdl which is limited to 31).
*/
static QByteArray buildArraySdl(const ISC_ARRAY_DESC &desc,
                                const QByteArray &relationName,
                                const QByteArray &fieldName)
{
    QByteArray sdl;
    sdl.append(char(isc_sdl_version1));
    // struct with 1 element type descriptor
    sdl.append(char(isc_sdl_struct));
    sdl.append(char(1));
    // BLR-style element type encoding (varies by type)
    unsigned char dtype = desc.array_desc_dtype;
    // For varying columns, request text format in SDL to avoid ISC_VARYING prefix complexity
    unsigned char sdlDtype = (dtype == blr_varying) ? blr_text : dtype;
    sdl.append(char(sdlDtype));
    switch (sdlDtype) {
    case blr_short:     // 7: type + scale
    case blr_long:      // 8: type + scale
    case blr_int64:     // 16: type + scale
    case blr_int128:    // 26: type + scale
        sdl.append(char(desc.array_desc_scale));
        break;
    case blr_text:      // 14: type + length(2 LE)
    case blr_varying:   // 37: type + length(2 LE)
    case blr_cstring:   // 40: type + length(2 LE)
        sdl.append(char(desc.array_desc_length & 0xFF));
        sdl.append(char((desc.array_desc_length >> 8) & 0xFF));
        break;
    // Types with no extra bytes: float, double, date, time, timestamp, bool, etc.
    default:
        break;
    }
    // relation name
    sdl.append(char(isc_sdl_relation));
    sdl.append(char(relationName.size()));
    sdl.append(relationName);
    // field name
    sdl.append(char(isc_sdl_field));
    sdl.append(char(fieldName.size()));
    sdl.append(fieldName);
    /* dimension loops (nested: outermost first, each body is next instruction)
       Use do1 (lower=1) or do2 (explicit lower), NOT do3 (which adds increment) */
    short dims = desc.array_desc_dimensions;
    for (short d = 0; d < dims; ++d) {
        short lo = desc.array_desc_bounds[d].array_bound_lower;
        short hi = desc.array_desc_bounds[d].array_bound_upper;
        if (lo == 1) {
            // do1: only upper bound, lower defaults to 1, increment defaults to 1
            sdl.append(char(isc_sdl_do1));
            sdl.append(char(d));
        } else {
            // do2: lower + upper bounds, increment defaults to 1
            sdl.append(char(isc_sdl_do2));
            sdl.append(char(d));
            // lower bound
            if (lo >= -128 && lo <= 127) {
                sdl.append(char(isc_sdl_tiny_integer));
                sdl.append(char(static_cast<signed char>(lo)));
            } else {
                sdl.append(char(isc_sdl_short_integer));
                sdl.append(char(lo & 0xFF));
                sdl.append(char((lo >> 8) & 0xFF));
            }
        }
        // upper bound
        if (hi >= -128 && hi <= 127) {
            sdl.append(char(isc_sdl_tiny_integer));
            sdl.append(char(static_cast<signed char>(hi)));
        } else {
            sdl.append(char(isc_sdl_short_integer));
            sdl.append(char(hi & 0xFF));
            sdl.append(char((hi >> 8) & 0xFF));
        }
    }
    // element accessor: element 1, scalar 0 ndim, variable per dim
    sdl.append(char(isc_sdl_element));
    sdl.append(char(1));
    sdl.append(char(isc_sdl_scalar));
    sdl.append(char(0));
    sdl.append(char(dims));
    for (short d = 0; d < dims; ++d) {
        sdl.append(char(isc_sdl_variable));
        sdl.append(char(d));
    }
    sdl.append(char(isc_sdl_eoc));
    return sdl;
}

// Fetch array data via OO API (IAttachment::getSlice)
QVariant fetchArray(IAttachment *att, ITransaction *tra, IStatus *iStatus,
                    IMaster *master,
                    ArrayDescCache &descCache,
                    const ISC_QUAD &arrayId, const QString &relation,
                    const QString &field)
{
    QList<QVariant> list;
    if (arrayId.gds_quad_high == 0 && arrayId.gds_quad_low == 0)
        return QVariant(list);

    QByteArray relBytes = relation.trimmed().toUtf8();
    QByteArray fldBytes = field.trimmed().toUtf8();

    ISC_ARRAY_DESC desc;
    if (!cachedArrayDesc(att, tra, iStatus, master, descCache, relBytes, fldBytes, &desc)) {
        qCWarning(lcFirebird, "fetchArray: could not look up array descriptor for %s.%s",
                  relBytes.constData(), fldBytes.constData());
        return QVariant(list);
    }

    // With blr_text SDL for varying, no +2 prefix needed
    QVarLengthArray<int> numElements;
    qint64 bufLen = 0;
    if (!arraySliceSize(desc, "fetchArray", relBytes, fldBytes, &numElements, &bufLen))
        return QVariant(list);

    // Generate SDL with original desc_length
    QByteArray sdlBuf = buildArraySdl(desc, relBytes, fldBytes);

    // Read array data via OO API
    QByteArray ba(bufLen, '\0');
    ISC_QUAD id = arrayId;
    try {
        ThrowStatusWrapper st(iStatus);
        int bytesRead = att->getSlice(&st, tra, &id,
                                      static_cast<unsigned>(sdlBuf.size()),
                                      reinterpret_cast<const unsigned char *>(sdlBuf.constData()),
                                      0, nullptr,
                                      static_cast<int>(bufLen),
                                      reinterpret_cast<unsigned char *>(ba.data()));
        Q_UNUSED(bytesRead);
    } catch (const FbException &e) {
        /* Surface to the caller (QFirebirdResult::data) rather than returning a
           silently empty array on a read failure. */
        qCWarning(lcFirebird) << "fetchArray: getSlice:"
                              << fbErrorLog(fbError(master, e.getStatus(), QSqlError::StatementError));
        throw;
    }

    readArrayBuffer(list, ba.constData(), 0, numElements.constData(), &desc, master, iStatus);
    return QVariant(list);
}

// Write array data via OO API (IAttachment::putSlice)
bool writeArray(IAttachment *att, ITransaction *tra, IStatus *iStatus,
                IMaster *master,
                ArrayDescCache &descCache,
                ISC_QUAD *arrayId, const QString &relation,
                const QString &field, const QList<QVariant> &list)
{
    QByteArray relBytes = relation.trimmed().toUtf8();
    QByteArray fldBytes = field.trimmed().toUtf8();

    ISC_ARRAY_DESC desc;
    if (!cachedArrayDesc(att, tra, iStatus, master, descCache, relBytes, fldBytes, &desc)) {
        qCWarning(lcFirebird, "writeArray: could not look up array descriptor for %s.%s",
                  relBytes.constData(), fldBytes.constData());
        return false;
    }

    // With blr_text SDL for varying, no +2 prefix needed
    qint64 bufLen = 0;
    if (!arraySliceSize(desc, "writeArray", relBytes, fldBytes, nullptr, &bufLen))
        return false;

    // Generate SDL with original desc_length
    QByteArray sdlBuf = buildArraySdl(desc, relBytes, fldBytes);

    QByteArray ba(bufLen, '\0');

    QMetaType::Type elType = blrTypeToQt(desc.array_desc_dtype, desc.array_desc_scale < 0);
    QString error;
    if (!createArrayBuffer(ba.data(), list, elType, 0, &desc, master, iStatus, error)) {
        qCWarning(lcFirebird) << "writeArray:" << error;
        return false;
    }

    try {
        ThrowStatusWrapper st(iStatus);
        att->putSlice(&st, tra, arrayId,
                      static_cast<unsigned>(sdlBuf.size()),
                      reinterpret_cast<const unsigned char *>(sdlBuf.constData()),
                      0, nullptr,
                      static_cast<int>(bufLen),
                      reinterpret_cast<unsigned char *>(ba.data()));
    } catch (const FbException &e) {
        const auto werr = fbError(master, e.getStatus(), QSqlError::StatementError);
        qCWarning(lcFirebird) << "writeArray: putSlice:" << fbErrorLog(werr);
        return false;
    }

    return true;
}

QT_END_NAMESPACE
