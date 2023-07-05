// Copyright (C) 2022 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#include "qsql_ibase_p.h"
#include <QtCore/qcoreapplication.h>
#include <QtCore/qdatetime.h>
#include <QtCore/qtimezone.h>
#include <QtCore/qdeadlinetimer.h>
#include <QtCore/qdebug.h>
#include <QtCore/qlist.h>
#include <QtCore/qmap.h>
#include <QtCore/qmutex.h>
#include <QtCore/qvariant.h>
#include <QtCore/qvarlengtharray.h>
#include <QtSql/qsqlerror.h>
#include <QtSql/qsqlfield.h>
#include <QtSql/qsqlindex.h>
#include <QtSql/qsqlquery.h>
#include <QtSql/private/qsqlcachedresult_p.h>
#include <QtSql/private/qsqldriver_p.h>
#include <stdlib.h>
#include <limits.h>
#include <math.h>
#include <mutex>

QT_BEGIN_NAMESPACE

using namespace Qt::StringLiterals;

#define FBVERSION SQL_DIALECT_V6

#ifndef SQLDA_CURRENT_VERSION
#define SQLDA_CURRENT_VERSION SQLDA_VERSION1
#endif

// Firebird uses blr_bool and not blr_boolean_dtype which is what Interbase uses
#ifndef blr_boolean_dtype
#define blr_boolean_dtype blr_bool
#endif

constexpr qsizetype QIBaseChunkSize = SHRT_MAX / 2;

#if (FB_API_VER >= 40)
typedef QMap<quint16, QByteArray> QFbTzIdToIanaIdMap;
typedef QMap<QByteArray, quint16> QIanaIdToFbTzIdMap;
Q_GLOBAL_STATIC(QFbTzIdToIanaIdMap, qFbTzIdToIanaIdMap)
Q_GLOBAL_STATIC(QIanaIdToFbTzIdMap, qIanaIdToFbTzIdMap)
std::once_flag initTZMappingFlag;
#endif

static bool getIBaseError(QString& msg, const ISC_STATUS* status, ISC_LONG &sqlcode)
{
    if (status[0] != 1 || status[1] <= 0)
        return false;

    msg.clear();
    sqlcode = isc_sqlcode(status);
    char buf[512];
    while(fb_interpret(buf, 512, &status)) {
        if (!msg.isEmpty())
            msg += " - "_L1;
        msg += QString::fromUtf8(buf);
    }
    return true;
}

static void createDA(XSQLDA *&sqlda)
{
    sqlda = (XSQLDA *) malloc(XSQLDA_LENGTH(1));
    if (sqlda == (XSQLDA*)0) return;
    sqlda->sqln = 1;
    sqlda->sqld = 0;
    sqlda->version = SQLDA_CURRENT_VERSION;
    sqlda->sqlvar[0].sqlind = 0;
    sqlda->sqlvar[0].sqldata = 0;
}

static void enlargeDA(XSQLDA *&sqlda, int n)
{
    if (sqlda != (XSQLDA*)0)
        free(sqlda);
    sqlda = (XSQLDA *) malloc(XSQLDA_LENGTH(n));
    if (sqlda == (XSQLDA*)0) return;
    sqlda->sqln = n;
    sqlda->version = SQLDA_CURRENT_VERSION;
}

static void initDA(XSQLDA *sqlda)
{
    for (int i = 0; i < sqlda->sqld; ++i) {
        switch (sqlda->sqlvar[i].sqltype & ~1) {
        case SQL_INT64:
        case SQL_LONG:
        case SQL_SHORT:
        case SQL_FLOAT:
        case SQL_DOUBLE:
        case SQL_TIMESTAMP:
#if (FB_API_VER >= 40)
        case SQL_TIMESTAMP_TZ:
#endif
        case SQL_TYPE_TIME:
        case SQL_TYPE_DATE:
        case SQL_TEXT:
        case SQL_BLOB:
        case SQL_BOOLEAN:
            sqlda->sqlvar[i].sqldata = new char[sqlda->sqlvar[i].sqllen];
            break;
        case SQL_ARRAY:
            sqlda->sqlvar[i].sqldata = new char[sizeof(ISC_QUAD)];
            memset(sqlda->sqlvar[i].sqldata, 0, sizeof(ISC_QUAD));
            break;
        case SQL_VARYING:
            sqlda->sqlvar[i].sqldata = new char[sqlda->sqlvar[i].sqllen + sizeof(short)];
            break;
        default:
            // not supported - do not bind.
            sqlda->sqlvar[i].sqldata = 0;
            break;
        }
        if (sqlda->sqlvar[i].sqltype & 1) {
            sqlda->sqlvar[i].sqlind = new short[1];
            *(sqlda->sqlvar[i].sqlind) = 0;
        } else {
            sqlda->sqlvar[i].sqlind = 0;
        }
    }
}

static void delDA(XSQLDA *&sqlda)
{
    if (!sqlda)
        return;
    for (int i = 0; i < sqlda->sqld; ++i) {
        delete [] sqlda->sqlvar[i].sqlind;
        delete [] sqlda->sqlvar[i].sqldata;
    }
    free(sqlda);
    sqlda = nullptr;
}

static QMetaType::Type qIBaseTypeName(int iType, bool hasScale)
{
    switch (iType) {
    case blr_varying:
    case blr_varying2:
    case blr_text:
    case blr_cstring:
    case blr_cstring2:
        return QMetaType::QString;
    case blr_sql_time:
        return QMetaType::QTime;
    case blr_sql_date:
        return QMetaType::QDate;
    case blr_timestamp:
#if (FB_API_VER >= 40)
    case blr_timestamp_tz:
#endif
        return QMetaType::QDateTime;
    case blr_blob:
        return QMetaType::QByteArray;
    case blr_quad:
    case blr_short:
    case blr_long:
        return (hasScale ? QMetaType::Double : QMetaType::Int);
    case blr_int64:
        return (hasScale ? QMetaType::Double : QMetaType::LongLong);
    case blr_float:
    case blr_d_float:
    case blr_double:
        return QMetaType::Double;
    case blr_boolean_dtype:
        return QMetaType::Bool;
    }
    qWarning("qIBaseTypeName: unknown datatype: %d", iType);
    return QMetaType::UnknownType;
}

static QMetaType::Type qIBaseTypeName2(int iType, bool hasScale)
{
    switch(iType & ~1) {
    case SQL_VARYING:
    case SQL_TEXT:
        return QMetaType::QString;
    case SQL_LONG:
    case SQL_SHORT:
        return (hasScale ? QMetaType::Double : QMetaType::Int);
    case SQL_INT64:
        return (hasScale ? QMetaType::Double : QMetaType::LongLong);
    case SQL_FLOAT:
    case SQL_DOUBLE:
        return QMetaType::Double;
    case SQL_TIMESTAMP:
#if (FB_API_VER >= 40)
    case SQL_TIMESTAMP_TZ:
#endif
        return QMetaType::QDateTime;
    case SQL_TYPE_TIME:
        return QMetaType::QTime;
    case SQL_TYPE_DATE:
        return QMetaType::QDate;
    case SQL_ARRAY:
        return QMetaType::QVariantList;
    case SQL_BLOB:
        return QMetaType::QByteArray;
    case SQL_BOOLEAN:
        return QMetaType::Bool;
    default:
        return QMetaType::UnknownType;
    }
}

static ISC_TIMESTAMP toTimeStamp(const QDateTime &dt)
{
    static const QTime midnight(0, 0, 0, 0);
    static const QDate basedate(1858, 11, 17);
    ISC_TIMESTAMP ts;
    ts.timestamp_time = midnight.msecsTo(dt.time()) * 10;
    ts.timestamp_date = basedate.daysTo(dt.date());
    return ts;
}

static QDateTime fromTimeStamp(char *buffer)
{
    static const QDate bd(1858, 11, 17);
    QTime t(0, 0);
    QDate d;

    // have to demangle the structure ourselves because isc_decode_time
    // strips the msecs
    auto timebuf = reinterpret_cast<ISC_TIMESTAMP*>(buffer);
    t = t.addMSecs(static_cast<int>(timebuf->timestamp_time / 10));
    d = bd.addDays(timebuf->timestamp_date);
    return QDateTime(d, t);
}

#if (FB_API_VER >= 40)
QDateTime fromTimeStampTz(char *buffer)
{
    static const QDate bd(1858, 11, 17);
    QTime t(0, 0);
    QDate d;

    // have to demangle the structure ourselves because isc_decode_time
    // strips the msecs
    auto timebuf = reinterpret_cast<ISC_TIMESTAMP_TZ*>(buffer);
    t = t.addMSecs(static_cast<int>(timebuf->utc_timestamp.timestamp_time / 10));
    d = bd.addDays(timebuf->utc_timestamp.timestamp_date);
    quint16 fpTzID = timebuf->time_zone;

    QByteArray timeZoneName = qFbTzIdToIanaIdMap()->value(fpTzID);
    if (!timeZoneName.isEmpty())
        return QDateTime(d, t, QTimeZone(timeZoneName));
    else
        return {};
}

ISC_TIMESTAMP_TZ toTimeStampTz(const QDateTime &dt)
{
    static const QTime midnight(0, 0, 0, 0);
    static const QDate basedate(1858, 11, 17);
    ISC_TIMESTAMP_TZ ts;
    ts.utc_timestamp.timestamp_time = midnight.msecsTo(dt.time()) * 10;
    ts.utc_timestamp.timestamp_date = basedate.daysTo(dt.date());
    ts.time_zone = qIanaIdToFbTzIdMap()->value(dt.timeZone().id().simplified(), 0);
    return ts;
}
#endif

static ISC_TIME toTime(QTime t)
{
    static const QTime midnight(0, 0, 0, 0);
    return (ISC_TIME)midnight.msecsTo(t) * 10;
}

static QTime fromTime(char *buffer)
{
    QTime t(0, 0);
    // have to demangle the structure ourselves because isc_decode_time
    // strips the msecs
    t = t.addMSecs(int((*(ISC_TIME*)buffer) / 10));

    return t;
}

static ISC_DATE toDate(QDate t)
{
    static const QDate basedate(1858, 11, 17);
    ISC_DATE date;

    date = basedate.daysTo(t);
    return date;
}

static QDate fromDate(char *buffer)
{
    static const QDate bd(1858, 11, 17);
    QDate d;

    // have to demangle the structure ourselves because isc_decode_time
    // strips the msecs
    d = bd.addDays(int(((ISC_TIMESTAMP*)buffer)->timestamp_date));

    return d;
}

struct QIBaseEventBuffer {
    ISC_UCHAR *eventBuffer;
    ISC_UCHAR *resultBuffer;
    ISC_LONG bufferLength;
    ISC_LONG eventId;

    enum QIBaseSubscriptionState { Starting, Subscribed, Finished };
    QIBaseSubscriptionState subscriptionState;
};

class QIBaseDriverPrivate : public QSqlDriverPrivate
{
    Q_DECLARE_PUBLIC(QIBaseDriver)
public:
    QIBaseDriverPrivate() : QSqlDriverPrivate(), ibase(0), trans(0)
    { dbmsType = QSqlDriver::Interbase; }

    bool isError(const char *msg, QSqlError::ErrorType typ = QSqlError::UnknownError)
    {
        Q_Q(QIBaseDriver);
        QString imsg;
        ISC_LONG sqlcode;
        if (!getIBaseError(imsg, status, sqlcode))
            return false;

        q->setLastError(QSqlError(QCoreApplication::translate("QIBaseDriver", msg),
                                  imsg, typ,
                                  sqlcode != -1 ? QString::number(sqlcode) : QString()));
        return true;
    }

#if (FB_API_VER >= 40)
    void initTZMappingCache()
    {
        Q_Q(QIBaseDriver);
        QSqlQuery qry(q->createResult());
        qry.setForwardOnly(true);
        qry.exec(QString("select * from RDB$TIME_ZONES"_L1));
        if (qry.lastError().type()) {
            q->setLastError(QSqlError(
                                QCoreApplication::translate("QIBaseDriver",
                                    "failed to query time zone mapping from system table"),
                                qry.lastError().databaseText(),
                                QSqlError::StatementError,
                                qry.lastError().nativeErrorCode()));

            return;
        }

        while (qry.next()) {
            auto record = qry.record();
            quint16 fbTzId = record.value(0).value<quint16>();
            QByteArray ianaId = record.value(1).toByteArray().simplified();
            qFbTzIdToIanaIdMap()->insert(fbTzId, ianaId);
            qIanaIdToFbTzIdMap()->insert(ianaId, fbTzId);
        }
    }
#endif

public:
    isc_db_handle ibase;
    isc_tr_handle trans;
    ISC_STATUS status[20];
    QMap<QString, QIBaseEventBuffer*> eventBuffers;
};

typedef QMap<void *, QIBaseDriver *> QIBaseBufferDriverMap;
Q_GLOBAL_STATIC(QIBaseBufferDriverMap, qBufferDriverMap)
Q_GLOBAL_STATIC(QMutex, qMutex);

static void qFreeEventBuffer(QIBaseEventBuffer* eBuffer)
{
    qMutex()->lock();
    qBufferDriverMap()->remove(reinterpret_cast<void *>(eBuffer->resultBuffer));
    qMutex()->unlock();
    delete eBuffer;
}

class QIBaseResultPrivate;

class QIBaseResult : public QSqlCachedResult
{
    Q_DECLARE_PRIVATE(QIBaseResult)

public:
    explicit QIBaseResult(const QIBaseDriver* db);

    bool prepare(const QString &query) override;
    bool exec() override;
    QVariant handle() const override;

protected:
    bool gotoNext(QSqlCachedResult::ValueCache& row, int rowIdx) override;
    bool reset (const QString &query) override;
    int size() override;
    int numRowsAffected() override;
    QSqlRecord record() const override;
};

class QIBaseResultPrivate: public QSqlCachedResultPrivate
{
    Q_DECLARE_PUBLIC(QIBaseResult)

public:
    Q_DECLARE_SQLDRIVER_PRIVATE(QIBaseDriver)

    QIBaseResultPrivate(QIBaseResult *q, const QIBaseDriver *drv);
    ~QIBaseResultPrivate() { cleanup(); }

    void cleanup();
    bool isError(const char *msg, QSqlError::ErrorType typ = QSqlError::UnknownError)
    {
        Q_Q(QIBaseResult);
        QString imsg;
        ISC_LONG sqlcode;
        if (!getIBaseError(imsg, status, sqlcode))
            return false;

        q->setLastError(QSqlError(QCoreApplication::translate("QIBaseResult", msg),
                        imsg, typ,
                        sqlcode != -1 ? QString::number(sqlcode) : QString()));
        return true;
    }

    bool transaction();
    bool commit();

    bool isSelect();
    QVariant fetchBlob(ISC_QUAD *bId);
    bool writeBlob(qsizetype iPos, const QByteArray &ba);
    QVariant fetchArray(int pos, ISC_QUAD *arr);
    bool writeArray(qsizetype i, const QList<QVariant> &list);
public:
    ISC_STATUS status[20];
    isc_tr_handle trans;
    //indicator whether we have a local transaction or a transaction on driver level
    bool localTransaction;
    isc_stmt_handle stmt;
    isc_db_handle ibase;
    XSQLDA *sqlda; // output sqlda
    XSQLDA *inda; // input parameters
    int queryType;
};


QIBaseResultPrivate::QIBaseResultPrivate(QIBaseResult *q, const QIBaseDriver *drv)
    : QSqlCachedResultPrivate(q, drv),
      trans(0),
      localTransaction(!drv_d_func()->ibase),
      stmt(0),
      ibase(drv_d_func()->ibase),
      sqlda(nullptr),
      inda(nullptr),
      queryType(-1)
{
}

void QIBaseResultPrivate::cleanup()
{
    Q_Q(QIBaseResult);
    commit();
    if (!localTransaction)
        trans = 0;

    if (stmt) {
        isc_dsql_free_statement(status, &stmt, DSQL_drop);
        stmt = 0;
    }

    delDA(sqlda);
    delDA(inda);

    queryType = -1;
    q->cleanup();
}

bool QIBaseResultPrivate::writeBlob(qsizetype iPos, const QByteArray &ba)
{
    isc_blob_handle handle = 0;
    ISC_QUAD *bId = (ISC_QUAD*)inda->sqlvar[iPos].sqldata;
    isc_create_blob2(status, &ibase, &trans, &handle, bId, 0, 0);
    if (!isError(QT_TRANSLATE_NOOP("QIBaseResult", "Unable to create BLOB"),
                 QSqlError::StatementError)) {
        qsizetype i = 0;
        while (i < ba.size()) {
            isc_put_segment(status, &handle, qMin(ba.size() - i, QIBaseChunkSize),
                            ba.data() + i);
            if (isError(QT_TRANSLATE_NOOP("QIBaseResult", "Unable to write BLOB")))
                return false;
            i += qMin(ba.size() - i, QIBaseChunkSize);
        }
    }
    isc_close_blob(status, &handle);
    return true;
}

QVariant QIBaseResultPrivate::fetchBlob(ISC_QUAD *bId)
{
    isc_blob_handle handle = 0;

    isc_open_blob2(status, &ibase, &trans, &handle, bId, 0, 0);
    if (isError(QT_TRANSLATE_NOOP("QIBaseResult", "Unable to open BLOB"),
                QSqlError::StatementError))
        return QVariant();

    unsigned short len = 0;
    QByteArray ba;
    int chunkSize = QIBaseChunkSize;
    ba.resize(chunkSize);
    qsizetype read = 0;
    while (isc_get_segment(status, &handle, &len, chunkSize, ba.data() + read) == 0 || status[1] == isc_segment) {
        read += len;
        ba.resize(read + chunkSize);
    }
    ba.resize(read);

    bool isErr = (status[1] == isc_segstr_eof ? false :
                    isError(QT_TRANSLATE_NOOP("QIBaseResult",
                                                "Unable to read BLOB"),
                                                QSqlError::StatementError));

    isc_close_blob(status, &handle);

    if (isErr)
        return QVariant();

    ba.resize(read);
    return ba;
}

template<typename T>
static QList<QVariant> toList(char** buf, int count)
{
    QList<QVariant> res;
    for (int i = 0; i < count; ++i) {
        res.append(*(T*)(*buf));
        *buf += sizeof(T);
    }
    return res;
}

static char* readArrayBuffer(QList<QVariant>& list, char *buffer, short curDim,
                             short* numElements, ISC_ARRAY_DESC *arrayDesc)
{
    const short dim = arrayDesc->array_desc_dimensions - 1;
    const unsigned char dataType = arrayDesc->array_desc_dtype;
    QList<QVariant> valList;
    unsigned short strLen = arrayDesc->array_desc_length;

    if (curDim != dim) {
        for(int i = 0; i < numElements[curDim]; ++i)
            buffer = readArrayBuffer(list, buffer, curDim + 1, numElements, arrayDesc);
    } else {
        switch(dataType) {
            case blr_varying:
            case blr_varying2:
                 strLen += 2; // for the two terminating null values
                 Q_FALLTHROUGH();
            case blr_text:
            case blr_text2: {
                int o;
                for (int i = 0; i < numElements[dim]; ++i) {
                    for(o = 0; o < strLen && buffer[o]!=0; ++o )
                        ;

                    valList.append(QString::fromUtf8(buffer, o));
                    buffer += strLen;
                }
                break; }
            case blr_long:
                valList = toList<int>(&buffer, numElements[dim]);
                break;
            case blr_short:
                valList = toList<short>(&buffer, numElements[dim]);
                break;
            case blr_int64:
                valList = toList<qint64>(&buffer, numElements[dim]);
                break;
            case blr_float:
                valList = toList<float>(&buffer, numElements[dim]);
                break;
            case blr_double:
                valList = toList<double>(&buffer, numElements[dim]);
                break;
            case blr_timestamp:
                for(int i = 0; i < numElements[dim]; ++i) {
                    valList.append(fromTimeStamp(buffer));
                    buffer += sizeof(ISC_TIMESTAMP);
                }
                break;
#if (FB_API_VER >= 40)
            case blr_timestamp_tz:
                for (int i = 0; i < numElements[dim]; ++i) {
                    valList.append(fromTimeStampTz(buffer));
                    buffer += sizeof(ISC_TIMESTAMP_TZ);
                }
                break;
#endif
            case blr_sql_time:
                for (int i = 0; i < numElements[dim]; ++i) {
                    valList.append(fromTime(buffer));
                    buffer += sizeof(ISC_TIME);
                }
                break;
            case blr_sql_date:
                for(int i = 0; i < numElements[dim]; ++i) {
                    valList.append(fromDate(buffer));
                    buffer += sizeof(ISC_DATE);
                }
                break;
            case blr_boolean_dtype:
                valList = toList<bool>(&buffer, numElements[dim]);
                break;
        }
    }
    if (dim > 0)
        list.append(valList);
    else
        list += valList;
    return buffer;
}

QVariant QIBaseResultPrivate::fetchArray(int pos, ISC_QUAD *arr)
{
    QList<QVariant> list;
    ISC_ARRAY_DESC desc;

    if (!arr)
        return list;

    QByteArray relname(sqlda->sqlvar[pos].relname, sqlda->sqlvar[pos].relname_length);
    QByteArray sqlname(sqlda->sqlvar[pos].sqlname, sqlda->sqlvar[pos].sqlname_length);

    isc_array_lookup_bounds(status, &ibase, &trans, relname.data(), sqlname.data(), &desc);
    if (isError(QT_TRANSLATE_NOOP("QIBaseResult", "Could not find array"),
                QSqlError::StatementError))
        return list;


    int arraySize = 1, subArraySize;
    short dimensions = desc.array_desc_dimensions;
    QVarLengthArray<short> numElements(dimensions);

    for(int i = 0; i < dimensions; ++i) {
        subArraySize = (desc.array_desc_bounds[i].array_bound_upper -
                      desc.array_desc_bounds[i].array_bound_lower + 1);
        numElements[i] = subArraySize;
        arraySize = subArraySize * arraySize;
    }

    ISC_LONG bufLen;
    QByteArray ba;
    /* varying arrayelements are stored with 2 trailing null bytes
       indicating the length of the string
     */
    if (desc.array_desc_dtype == blr_varying
        || desc.array_desc_dtype == blr_varying2) {
        desc.array_desc_length += 2;
        bufLen = desc.array_desc_length * arraySize * sizeof(short);
    } else {
        bufLen = desc.array_desc_length *  arraySize;
    }


    ba.resize(int(bufLen));
    isc_array_get_slice(status, &ibase, &trans, arr, &desc, ba.data(), &bufLen);
    if (isError(QT_TRANSLATE_NOOP("QIBaseResult", "Could not get array data"),
                QSqlError::StatementError))
        return list;

    readArrayBuffer(list, ba.data(), 0, numElements.data(), &desc);

    return QVariant(list);
}

template<typename T>
static char* fillList(char *buffer, const QList<QVariant> &list, T* = nullptr)
{
    for (const auto &elem : list) {
        T val = qvariant_cast<T>(elem);
        memcpy(buffer, &val, sizeof(T));
        buffer += sizeof(T);
    }
    return buffer;
}

template<>
char* fillList<float>(char *buffer, const QList<QVariant> &list, float*)
{
    for (const auto &elem : list) {
        double val = qvariant_cast<double>(elem);
        float val2 = (float)val;
        memcpy(buffer, &val2, sizeof(float));
        buffer += sizeof(float);
    }
    return buffer;
}

static char* qFillBufferWithString(char *buffer, const QString& string,
                                   short buflen, bool varying, bool array)
{
    QByteArray str = string.toUtf8(); // keep a copy of the string alive in this scope
    if (varying) {
        short tmpBuflen = buflen;
        if (str.length() < buflen)
            buflen = str.length();
        if (array) { // interbase stores varying arrayelements different than normal varying elements
            memcpy(buffer, str.data(), buflen);
            memset(buffer + buflen, 0, tmpBuflen - buflen);
        } else {
            *(short*)buffer = buflen; // first two bytes is the length
            memcpy(buffer + sizeof(short), str.data(), buflen);
        }
        buffer += tmpBuflen;
    } else {
        str = str.leftJustified(buflen, ' ', true);
        memcpy(buffer, str.data(), buflen);
        buffer += buflen;
    }
    return buffer;
}

static char* createArrayBuffer(char *buffer, const QList<QVariant> &list,
                               QMetaType::Type type, short curDim, ISC_ARRAY_DESC *arrayDesc,
                               QString& error)
{
    ISC_ARRAY_BOUND *bounds = arrayDesc->array_desc_bounds;
    short dim = arrayDesc->array_desc_dimensions - 1;

    int elements = (bounds[curDim].array_bound_upper -
                    bounds[curDim].array_bound_lower + 1);

    if (list.size() != elements) { // size mismatch
        error = "Expected size: %1. Supplied size: %2"_L1;
        error = "Array size mismatch. Fieldname: %1 "_L1
                + error.arg(elements).arg(list.size());
        return 0;
    }

    if (curDim != dim) {
        for (const auto &elem : list) {

          if (elem.typeId() != QMetaType::QVariantList) { // dimensions mismatch
              error = "Array dimensons mismatch. Fieldname: %1"_L1;
              return 0;
          }

          buffer = createArrayBuffer(buffer, elem.toList(), type, curDim + 1,
                                     arrayDesc, error);
          if (!buffer)
              return 0;
        }
    } else {
        switch(type) {
        case QMetaType::Int:
        case QMetaType::UInt:
            if (arrayDesc->array_desc_dtype == blr_short)
                buffer = fillList<short>(buffer, list);
            else
                buffer = fillList<int>(buffer, list);
            break;
        case QMetaType::Double:
            if (arrayDesc->array_desc_dtype == blr_float)
                buffer = fillList<float>(buffer, list, static_cast<float *>(0));
            else
                buffer = fillList<double>(buffer, list);
            break;
        case QMetaType::LongLong:
            buffer = fillList<qint64>(buffer, list);
            break;
        case QMetaType::ULongLong:
            buffer = fillList<quint64>(buffer, list);
            break;
        case QMetaType::QString:
            for (const auto &elem : list)
                buffer = qFillBufferWithString(buffer, elem.toString(),
                                               arrayDesc->array_desc_length,
                                               arrayDesc->array_desc_dtype == blr_varying,
                                               true);
            break;
        case QMetaType::QDate:
            for (const auto &elem : list) {
                *((ISC_DATE*)buffer) = toDate(elem.toDate());
                buffer += sizeof(ISC_DATE);
            }
            break;
        case QMetaType::QTime:
            for (const auto &elem : list) {
                *((ISC_TIME*)buffer) = toTime(elem.toTime());
                buffer += sizeof(ISC_TIME);
            }
            break;
        case QMetaType::QDateTime:
            for (const auto &elem : list) {
                switch (arrayDesc->array_desc_dtype) {
                case blr_timestamp:
                    *((ISC_TIMESTAMP*)buffer) = toTimeStamp(elem.toDateTime());
                    buffer += sizeof(ISC_TIMESTAMP);
                    break;
#if (FB_API_VER >= 40)
                case blr_timestamp_tz:
                    *((ISC_TIMESTAMP_TZ*)buffer) = toTimeStampTz(elem.toDateTime());
                    buffer += sizeof(ISC_TIMESTAMP_TZ);
                    break;
#endif
                default:
                    break;
                }
            }
            break;
        case QMetaType::Bool:
            buffer = fillList<bool>(buffer, list);
            break;
        default:
            break;
        }
    }
    return buffer;
}

bool QIBaseResultPrivate::writeArray(qsizetype column, const QList<QVariant> &list)
{
    Q_Q(QIBaseResult);
    QString error;
    ISC_QUAD *arrayId = (ISC_QUAD*) inda->sqlvar[column].sqldata;
    ISC_ARRAY_DESC desc;

    QByteArray relname(inda->sqlvar[column].relname, inda->sqlvar[column].relname_length);
    QByteArray sqlname(inda->sqlvar[column].sqlname, inda->sqlvar[column].sqlname_length);

    isc_array_lookup_bounds(status, &ibase, &trans, relname.data(), sqlname.data(), &desc);
    if (isError(QT_TRANSLATE_NOOP("QIBaseResult", "Could not find array"),
                QSqlError::StatementError))
        return false;

    short arraySize = 1;
    ISC_LONG bufLen;

    short dimensions = desc.array_desc_dimensions;
    for(int i = 0; i < dimensions; ++i) {
        arraySize *= (desc.array_desc_bounds[i].array_bound_upper -
                      desc.array_desc_bounds[i].array_bound_lower + 1);
    }

    /* varying arrayelements are stored with 2 trailing null bytes
       indicating the length of the string
     */
    if (desc.array_desc_dtype == blr_varying ||
       desc.array_desc_dtype == blr_varying2)
        desc.array_desc_length += 2;

    bufLen = desc.array_desc_length * arraySize;
    QByteArray ba;
    ba.resize(int(bufLen));

    if (list.size() > arraySize) {
        error = "Array size mismatch: size of %1 is %2, size of provided list is %3"_L1;
        error = error.arg(QLatin1StringView(sqlname)).arg(arraySize).arg(list.size());
        q->setLastError(QSqlError(error, ""_L1, QSqlError::StatementError));
        return false;
    }

    if (!createArrayBuffer(ba.data(), list,
                           qIBaseTypeName(desc.array_desc_dtype, inda->sqlvar[column].sqlscale < 0),
                           0, &desc, error)) {
        q->setLastError(QSqlError(error.arg(QLatin1StringView(sqlname)), ""_L1,
                        QSqlError::StatementError));
        return false;
    }

    /* readjust the buffer size*/
    if (desc.array_desc_dtype == blr_varying
        || desc.array_desc_dtype == blr_varying2)
        desc.array_desc_length -= 2;

    isc_array_put_slice(status, &ibase, &trans, arrayId, &desc, ba.data(), &bufLen);
    return true;
}


bool QIBaseResultPrivate::isSelect()
{
    char acBuffer[9];
    char qType = isc_info_sql_stmt_type;
    isc_dsql_sql_info(status, &stmt, 1, &qType, sizeof(acBuffer), acBuffer);
    if (isError(QT_TRANSLATE_NOOP("QIBaseResult", "Could not get query info"),
                QSqlError::StatementError))
        return false;
    int iLength = isc_vax_integer(&acBuffer[1], 2);
    queryType = isc_vax_integer(&acBuffer[3], iLength);
    return (queryType == isc_info_sql_stmt_select || queryType == isc_info_sql_stmt_exec_procedure);
}

bool QIBaseResultPrivate::transaction()
{
    if (trans)
        return true;
    if (drv_d_func()->trans) {
        localTransaction = false;
        trans = drv_d_func()->trans;
        return true;
    }
    localTransaction = true;

    isc_start_transaction(status, &trans, 1, &ibase, 0, NULL);
    if (isError(QT_TRANSLATE_NOOP("QIBaseResult", "Could not start transaction"),
                QSqlError::TransactionError))
        return false;

    return true;
}

// does nothing if the transaction is on the
// driver level
bool QIBaseResultPrivate::commit()
{
    if (!trans)
        return false;
    // don't commit driver's transaction, the driver will do it for us
    if (!localTransaction)
        return true;

    isc_commit_transaction(status, &trans);
    trans = 0;
    return !isError(QT_TRANSLATE_NOOP("QIBaseResult", "Unable to commit transaction"),
                    QSqlError::TransactionError);
}

//////////

QIBaseResult::QIBaseResult(const QIBaseDriver *db)
    : QSqlCachedResult(*new QIBaseResultPrivate(this, db))
{
}

bool QIBaseResult::prepare(const QString& query)
{
    Q_D(QIBaseResult);
//     qDebug("prepare: %ls", qUtf16Printable(query));
    if (!driver() || !driver()->isOpen() || driver()->isOpenError())
        return false;
    d->cleanup();
    setActive(false);
    setAt(QSql::BeforeFirstRow);

    createDA(d->sqlda);
    if (d->sqlda == (XSQLDA*)0) {
        qWarning()<<"QIOBaseResult: createDA(): failed to allocate memory";
        return false;
    }

    createDA(d->inda);
    if (d->inda == (XSQLDA*)0){
        qWarning()<<"QIOBaseResult: createDA():  failed to allocate memory";
        return false;
    }

    if (!d->transaction())
        return false;

    isc_dsql_allocate_statement(d->status, &d->ibase, &d->stmt);
    if (d->isError(QT_TRANSLATE_NOOP("QIBaseResult", "Could not allocate statement"),
                   QSqlError::StatementError))
        return false;
    isc_dsql_prepare(d->status, &d->trans, &d->stmt, 0,
        const_cast<char*>(query.toUtf8().constData()), FBVERSION, d->sqlda);
    if (d->isError(QT_TRANSLATE_NOOP("QIBaseResult", "Could not prepare statement"),
                   QSqlError::StatementError))
        return false;

    isc_dsql_describe_bind(d->status, &d->stmt, FBVERSION, d->inda);
    if (d->isError(QT_TRANSLATE_NOOP("QIBaseResult",
                    "Could not describe input statement"), QSqlError::StatementError))
        return false;
    if (d->inda->sqld > d->inda->sqln) {
        enlargeDA(d->inda, d->inda->sqld);
        if (d->inda == (XSQLDA*)0) {
            qWarning()<<"QIOBaseResult: enlargeDA(): failed to allocate memory";
            return false;
        }

        isc_dsql_describe_bind(d->status, &d->stmt, FBVERSION, d->inda);
        if (d->isError(QT_TRANSLATE_NOOP("QIBaseResult",
                        "Could not describe input statement"), QSqlError::StatementError))
            return false;
    }
    initDA(d->inda);
    if (d->sqlda->sqld > d->sqlda->sqln) {
        // need more field descriptors
        enlargeDA(d->sqlda, d->sqlda->sqld);
        if (d->sqlda == (XSQLDA*)0) {
            qWarning()<<"QIOBaseResult: enlargeDA(): failed to allocate memory";
            return false;
        }

        isc_dsql_describe(d->status, &d->stmt, FBVERSION, d->sqlda);
        if (d->isError(QT_TRANSLATE_NOOP("QIBaseResult", "Could not describe statement"),
                       QSqlError::StatementError))
            return false;
    }
    initDA(d->sqlda);

    setSelect(d->isSelect());
    if (!isSelect()) {
        free(d->sqlda);
        d->sqlda = 0;
    }

    return true;
}

bool QIBaseResult::exec()
{
    Q_D(QIBaseResult);
    bool ok = true;

    if (!d->trans)
        d->transaction();

    if (!driver() || !driver()->isOpen() || driver()->isOpenError())
        return false;
    setActive(false);
    setAt(QSql::BeforeFirstRow);

    if (d->inda) {
        const QList<QVariant> &values = boundValues();
        if (values.count() > d->inda->sqld) {
            qWarning() << "QIBaseResult::exec: Parameter mismatch, expected"_L1 <<
                          d->inda->sqld << ", got"_L1 << values.count() <<
                          "parameters"_L1;
            return false;
        }
        for (qsizetype para = 0; para < values.count(); ++para) {
            if (!d->inda->sqlvar[para].sqldata)
                // skip unknown datatypes
                continue;
            const QVariant &val = values[para];
            if (d->inda->sqlvar[para].sqltype & 1) {
                if (QSqlResultPrivate::isVariantNull(val)) {
                    // set null indicator
                    *(d->inda->sqlvar[para].sqlind) = -1;
                    // and set the value to 0, otherwise it would count as empty string.
                    // it seems to be working with just setting sqlind to -1
                    //*((char*)d->inda->sqlvar[para].sqldata) = 0;
                    continue;
                }
                // a value of 0 means non-null.
                *(d->inda->sqlvar[para].sqlind) = 0;
            } else {
                if (QSqlResultPrivate::isVariantNull(val)) {
                    qWarning() << "QIBaseResult::exec: Null value replaced by default (zero)"_L1
                               << "value for type of column"_L1 << d->inda->sqlvar[para].ownname
                               << ", which is not nullable."_L1;
                }
            }
            switch(d->inda->sqlvar[para].sqltype & ~1) {
            case SQL_INT64:
                if (d->inda->sqlvar[para].sqlscale < 0)
                    *((qint64*)d->inda->sqlvar[para].sqldata) =
                        (qint64)floor(0.5 + val.toDouble() * pow(10.0, d->inda->sqlvar[para].sqlscale * -1));
                else
                    *((qint64*)d->inda->sqlvar[para].sqldata) = val.toLongLong();
                break;
            case SQL_LONG:
                if (d->inda->sqlvar[para].sqllen == 4) {
                    if (d->inda->sqlvar[para].sqlscale < 0)
                        *((qint32*)d->inda->sqlvar[para].sqldata) =
                            (qint32)floor(0.5 + val.toDouble() * pow(10.0, d->inda->sqlvar[para].sqlscale * -1));
                    else
                        *((qint32*)d->inda->sqlvar[para].sqldata) = (qint32)val.toInt();
                } else {
                    *((qint64*)d->inda->sqlvar[para].sqldata) = val.toLongLong();
                }
                break;
            case SQL_SHORT:
                if (d->inda->sqlvar[para].sqlscale < 0)
                    *((short*)d->inda->sqlvar[para].sqldata) =
                        (short)floor(0.5 + val.toDouble() * pow(10.0, d->inda->sqlvar[para].sqlscale * -1));
                else
                    *((short*)d->inda->sqlvar[para].sqldata) = (short)val.toInt();
                break;
            case SQL_FLOAT:
                *((float*)d->inda->sqlvar[para].sqldata) = (float)val.toDouble();
                break;
            case SQL_DOUBLE:
                *((double*)d->inda->sqlvar[para].sqldata) = val.toDouble();
                break;
            case SQL_TIMESTAMP:
                *((ISC_TIMESTAMP*)d->inda->sqlvar[para].sqldata) = toTimeStamp(val.toDateTime());
                break;
#if (FB_API_VER >= 40)
            case SQL_TIMESTAMP_TZ:
               *((ISC_TIMESTAMP_TZ*)d->inda->sqlvar[para].sqldata) = toTimeStampTz(val.toDateTime());
               break;
#endif
            case SQL_TYPE_TIME:
                *((ISC_TIME*)d->inda->sqlvar[para].sqldata) = toTime(val.toTime());
                break;
            case SQL_TYPE_DATE:
                *((ISC_DATE*)d->inda->sqlvar[para].sqldata) = toDate(val.toDate());
                break;
            case SQL_VARYING:
            case SQL_TEXT:
                qFillBufferWithString(d->inda->sqlvar[para].sqldata, val.toString(),
                                      d->inda->sqlvar[para].sqllen,
                                      (d->inda->sqlvar[para].sqltype & ~1) == SQL_VARYING, false);
                break;
            case SQL_BLOB:
                    ok &= d->writeBlob(para, val.toByteArray());
                    break;
            case SQL_ARRAY:
                    ok &= d->writeArray(para, val.toList());
                    break;
            case SQL_BOOLEAN:
                *((bool*)d->inda->sqlvar[para].sqldata) = val.toBool();
                break;
            default:
                    qWarning("QIBaseResult::exec: Unknown datatype %d",
                             d->inda->sqlvar[para].sqltype & ~1);
                    break;
            }
        }
    }

    if (ok) {
        isc_dsql_free_statement(d->status, &d->stmt, DSQL_close);
        QString imsg;
        ISC_LONG sqlcode;
        if (getIBaseError(imsg, d->status, sqlcode) && sqlcode != -501) {
            setLastError(QSqlError(QCoreApplication::translate("QIBaseResult", "Unable to close statement"),
                                   imsg, QSqlError::UnknownError,
                                   sqlcode != -1 ? QString::number(sqlcode) : QString()));
            return false;
        }
        if (colCount() && d->queryType != isc_info_sql_stmt_exec_procedure) {
            cleanup();
        }
        if (d->queryType == isc_info_sql_stmt_exec_procedure)
            isc_dsql_execute2(d->status, &d->trans, &d->stmt, FBVERSION, d->inda, d->sqlda);
        else
            isc_dsql_execute(d->status, &d->trans, &d->stmt, FBVERSION, d->inda);
        if (d->isError(QT_TRANSLATE_NOOP("QIBaseResult", "Unable to execute query")))
            return false;

        // Not all stored procedures necessarily return values.
        if (d->queryType == isc_info_sql_stmt_exec_procedure && d->sqlda && d->sqlda->sqld == 0)
            delDA(d->sqlda);

        if (d->sqlda)
            init(d->sqlda->sqld);

        if (!isSelect())
             d->commit();

        setActive(true);
        return true;
    }
    return false;
}

bool QIBaseResult::reset (const QString& query)
{
    if (!prepare(query))
        return false;
    return exec();
}

bool QIBaseResult::gotoNext(QSqlCachedResult::ValueCache& row, int rowIdx)
{
    Q_D(QIBaseResult);
    ISC_STATUS stat = 0;

    // Stored Procedures are special - they populate our d->sqlda when executing,
    // so we don't have to call isc_dsql_fetch
    if (d->queryType == isc_info_sql_stmt_exec_procedure) {
        // the first "fetch" shall succeed, all consecutive ones will fail since
        // we only have one row to fetch for stored procedures
        if (rowIdx != 0)
            stat = 100;
    } else {
        stat = isc_dsql_fetch(d->status, &d->stmt, FBVERSION, d->sqlda);
    }

    if (stat == 100) {
        // no more rows
        setAt(QSql::AfterLastRow);
        return false;
    }
    if (d->isError(QT_TRANSLATE_NOOP("QIBaseResult", "Could not fetch next item"),
                   QSqlError::StatementError))
        return false;
    if (rowIdx < 0) // not interested in actual values
        return true;

    for (int i = 0; i < d->sqlda->sqld; ++i) {
        int idx = rowIdx + i;
        char *buf = d->sqlda->sqlvar[i].sqldata;
        int size = d->sqlda->sqlvar[i].sqllen;
        Q_ASSERT(buf);

        if ((d->sqlda->sqlvar[i].sqltype & 1) && *d->sqlda->sqlvar[i].sqlind) {
            // null value
            QVariant v;
            v.convert(QMetaType(qIBaseTypeName2(d->sqlda->sqlvar[i].sqltype,
                                                d->sqlda->sqlvar[i].sqlscale < 0)));
            if (v.userType() == QMetaType::Double) {
                switch(numericalPrecisionPolicy()) {
                case QSql::LowPrecisionInt32:
                    v.convert(QMetaType(QMetaType::Int));
                    break;
                case QSql::LowPrecisionInt64:
                    v.convert(QMetaType(QMetaType::LongLong));
                    break;
                case QSql::HighPrecision:
                    v.convert(QMetaType(QMetaType::QString));
                    break;
                case QSql::LowPrecisionDouble:
                    // no conversion
                    break;
                }
            }
            row[idx] = v;
            continue;
        }

        switch(d->sqlda->sqlvar[i].sqltype & ~1) {
        case SQL_VARYING:
            // pascal strings - a short with a length information followed by the data
            row[idx] = QString::fromUtf8(buf + sizeof(short), *(short*)buf);
            break;
        case SQL_INT64:
            if (d->sqlda->sqlvar[i].sqlscale < 0)
                row[idx] = *(qint64*)buf * pow(10.0, d->sqlda->sqlvar[i].sqlscale);
            else
                row[idx] = QVariant(*(qint64*)buf);
            break;
        case SQL_LONG:
            if (d->sqlda->sqlvar[i].sqllen == 4)
                if (d->sqlda->sqlvar[i].sqlscale < 0)
                    row[idx] = QVariant(*(qint32*)buf * pow(10.0, d->sqlda->sqlvar[i].sqlscale));
                else
                    row[idx] = QVariant(*(qint32*)buf);
            else
                row[idx] = QVariant(*(qint64*)buf);
            break;
        case SQL_SHORT:
            if (d->sqlda->sqlvar[i].sqlscale < 0)
                row[idx] = QVariant(long((*(short*)buf)) * pow(10.0, d->sqlda->sqlvar[i].sqlscale));
            else
                row[idx] = QVariant(int((*(short*)buf)));
            break;
        case SQL_FLOAT:
            row[idx] = QVariant(double((*(float*)buf)));
            break;
        case SQL_DOUBLE:
            row[idx] = QVariant(*(double*)buf);
            break;
        case SQL_TIMESTAMP:
            row[idx] = fromTimeStamp(buf);
            break;
        case SQL_TYPE_TIME:
            row[idx] = fromTime(buf);
            break;
        case SQL_TYPE_DATE:
            row[idx] = fromDate(buf);
            break;
        case SQL_TEXT:
            row[idx] = QString::fromUtf8(buf, size);
            break;
        case SQL_BLOB:
            row[idx] = d->fetchBlob((ISC_QUAD*)buf);
            break;
        case SQL_ARRAY:
            row[idx] = d->fetchArray(i, (ISC_QUAD*)buf);
            break;
        case SQL_BOOLEAN:
            row[idx] = QVariant(bool((*(bool*)buf)));
            break;
#if (FB_API_VER >= 40)
        case SQL_TIMESTAMP_TZ:
            row[idx] = fromTimeStampTz(buf);
            break;
#endif
        default:
            // unknown type - don't even try to fetch
            row[idx] = QVariant();
            break;
        }
        if (d->sqlda->sqlvar[i].sqlscale < 0) {
            QVariant v = row[idx];
            switch(numericalPrecisionPolicy()) {
            case QSql::LowPrecisionInt32:
                if (v.convert(QMetaType(QMetaType::Int)))
                    row[idx]=v;
                break;
            case QSql::LowPrecisionInt64:
                if (v.convert(QMetaType(QMetaType::LongLong)))
                    row[idx]=v;
                break;
            case QSql::LowPrecisionDouble:
                if (v.convert(QMetaType(QMetaType::Double)))
                    row[idx]=v;
                break;
            case QSql::HighPrecision:
                if (v.convert(QMetaType(QMetaType::QString)))
                    row[idx]=v;
                break;
            }
        }
    }

    return true;
}

int QIBaseResult::size()
{
    return -1;

#if 0 /// ### FIXME
    static char sizeInfo[] = {isc_info_sql_records};
    char buf[64];

    //qDebug() << sizeInfo;
    if (!isActive() || !isSelect())
        return -1;

        char ct;
        short len;
        int val = 0;
//    while(val == 0) {
        isc_dsql_sql_info(d->status, &d->stmt, sizeof(sizeInfo), sizeInfo, sizeof(buf), buf);
//        isc_database_info(d->status, &d->ibase, sizeof(sizeInfo), sizeInfo, sizeof(buf), buf);

        for(int i = 0; i < 66; ++i)
            qDebug() << QString::number(buf[i]);

        for (char* c = buf + 3; *c != isc_info_end; /*nothing*/) {
            ct = *(c++);
            len = isc_vax_integer(c, 2);
            c += 2;
            val = isc_vax_integer(c, len);
            c += len;
            qDebug() << "size" << val;
            if (ct == isc_info_req_select_count)
                return val;
        }
        //qDebug("size -1");
        return -1;

        unsigned int i, result_size;
        if (buf[0] == isc_info_sql_records) {
            i = 3;
            result_size = isc_vax_integer(&buf[1],2);
            while (buf[i] != isc_info_end && i < result_size) {
                len = (short)isc_vax_integer(&buf[i+1],2);
                if (buf[i] == isc_info_req_select_count)
                     return (isc_vax_integer(&buf[i+3],len));
                i += len+3;
           }
        }
//    }
    return -1;
#endif
}

int QIBaseResult::numRowsAffected()
{
    Q_D(QIBaseResult);
    static char acCountInfo[] = {isc_info_sql_records};
    char cCountType;
    bool bIsProcedure = false;

    switch (d->queryType) {
    case isc_info_sql_stmt_select:
        cCountType = isc_info_req_select_count;
        break;
    case isc_info_sql_stmt_update:
        cCountType = isc_info_req_update_count;
        break;
    case isc_info_sql_stmt_delete:
        cCountType = isc_info_req_delete_count;
        break;
    case isc_info_sql_stmt_insert:
        cCountType = isc_info_req_insert_count;
        break;
    case isc_info_sql_stmt_exec_procedure:
        bIsProcedure = true; // will sum all changes
        break;
    default:
        qWarning() << "numRowsAffected: Unknown statement type (" << d->queryType << ")";
        return -1;
    }

    char acBuffer[33];
    int iResult = -1;
    isc_dsql_sql_info(d->status, &d->stmt, sizeof(acCountInfo), acCountInfo, sizeof(acBuffer), acBuffer);
    if (d->isError(QT_TRANSLATE_NOOP("QIBaseResult", "Could not get statement info"),
                   QSqlError::StatementError))
        return -1;
    for (char *pcBuf = acBuffer + 3; *pcBuf != isc_info_end; /*nothing*/) {
        char cType = *pcBuf++;
        short sLength = isc_vax_integer (pcBuf, 2);
        pcBuf += 2;
        int iValue = isc_vax_integer (pcBuf, sLength);
        pcBuf += sLength;
        if (bIsProcedure) {
            if (cType == isc_info_req_insert_count || cType == isc_info_req_update_count
                || cType == isc_info_req_delete_count) {
                if (iResult == -1)
                    iResult = 0;
                iResult += iValue;
            }
        } else if (cType == cCountType) {
            iResult = iValue;
            break;
        }
    }
    return iResult;
}

QSqlRecord QIBaseResult::record() const
{
    Q_D(const QIBaseResult);
    QSqlRecord rec;
    if (!isActive() || !d->sqlda)
        return rec;

    XSQLVAR v;
    for (int i = 0; i < d->sqlda->sqld; ++i) {
        v = d->sqlda->sqlvar[i];
        QSqlField f(QString::fromLatin1(v.aliasname, v.aliasname_length).simplified(),
                    QMetaType(qIBaseTypeName2(v.sqltype, v.sqlscale < 0)),
                                    QString::fromLatin1(v.relname, v.relname_length));
        f.setLength(v.sqllen);
        f.setPrecision(qAbs(v.sqlscale));
        f.setRequiredStatus((v.sqltype & 1) == 0 ? QSqlField::Required : QSqlField::Optional);
        if (v.sqlscale < 0) {
            QSqlQuery q(driver()->createResult());
            q.setForwardOnly(true);
            q.exec("select b.RDB$FIELD_PRECISION, b.RDB$FIELD_SCALE, b.RDB$FIELD_LENGTH, a.RDB$NULL_FLAG "
                    "FROM RDB$RELATION_FIELDS a, RDB$FIELDS b "
                    "WHERE b.RDB$FIELD_NAME = a.RDB$FIELD_SOURCE "
                    "AND a.RDB$RELATION_NAME = '"_L1 + QString::fromLatin1(v.relname, v.relname_length) + "' "
                    "AND a.RDB$FIELD_NAME = '"_L1 + QString::fromLatin1(v.sqlname, v.sqlname_length) + "' "_L1);
            if (q.first()) {
                if (v.sqlscale < 0) {
                    f.setLength(q.value(0).toInt());
                    f.setPrecision(qAbs(q.value(1).toInt()));
                } else {
                    f.setLength(q.value(2).toInt());
                    f.setPrecision(0);
                }
                f.setRequiredStatus(q.value(3).toBool() ? QSqlField::Required : QSqlField::Optional);
            }
        }
        f.setSqlType(v.sqltype);
        rec.append(f);
    }
    return rec;
}

QVariant QIBaseResult::handle() const
{
    Q_D(const QIBaseResult);
    return QVariant(QMetaType::fromType<isc_stmt_handle>(), &d->stmt);
}

/*********************************/

QIBaseDriver::QIBaseDriver(QObject * parent)
    : QSqlDriver(*new QIBaseDriverPrivate, parent)
{
}

QIBaseDriver::QIBaseDriver(isc_db_handle connection, QObject *parent)
    : QSqlDriver(*new QIBaseDriverPrivate, parent)
{
    Q_D(QIBaseDriver);
    d->ibase = connection;
    setOpen(true);
    setOpenError(false);
}

QIBaseDriver::~QIBaseDriver()
{
}

bool QIBaseDriver::hasFeature(DriverFeature f) const
{
    switch (f) {
    case QuerySize:
    case NamedPlaceholders:
    case LastInsertId:
    case BatchOperations:
    case SimpleLocking:
    case FinishQuery:
    case MultipleResultSets:
    case CancelQuery:
        return false;
    case Transactions:
    case PreparedQueries:
    case PositionalPlaceholders:
    case Unicode:
    case BLOB:
    case EventNotifications:
    case LowPrecisionNumbers:
        return true;
    }
    return false;
}

bool QIBaseDriver::open(const QString &db,
                        const QString &user,
                        const QString &password,
                        const QString &host,
                        int port,
                        const QString &connOpts)
{
    Q_D(QIBaseDriver);
    if (isOpen())
        close();

    const auto opts(QStringView(connOpts).split(u';', Qt::SkipEmptyParts));

    QByteArray role;
    for (const auto &opt : opts) {
        const auto tmp(opt.trimmed());
        qsizetype idx;
        if ((idx = tmp.indexOf(u'=')) != -1) {
            const auto val = tmp.mid(idx + 1).trimmed();
            const auto opt = tmp.left(idx).trimmed().toString();
            if (opt.toUpper() == "ISC_DPB_SQL_ROLE_NAME"_L1) {
                role = val.toLocal8Bit();
                role.truncate(255);
            }
        }
    }

    QByteArray enc = "UTF8";
    QByteArray usr = user.toLocal8Bit();
    QByteArray pass = password.toLocal8Bit();
    usr.truncate(255);
    pass.truncate(255);

    QByteArray ba;
    ba.reserve(usr.length() + pass.length() + enc.length() + role.length() + 9);
    ba.append(char(isc_dpb_version1));
    ba.append(char(isc_dpb_user_name));
    ba.append(char(usr.length()));
    ba.append(usr.data(), usr.length());
    ba.append(char(isc_dpb_password));
    ba.append(char(pass.length()));
    ba.append(pass.data(), pass.length());
    ba.append(char(isc_dpb_lc_ctype));
    ba.append(char(enc.length()));
    ba.append(enc.data(), enc.length());

    if (!role.isEmpty()) {
        ba.append(char(isc_dpb_sql_role_name));
        ba.append(char(role.length()));
        ba.append(role.data(), role.length());
    }

    QString portString;
    if (port != -1)
        portString = QStringLiteral("/%1").arg(port);

    QString ldb;
    if (!host.isEmpty())
        ldb += host + portString + u':';
    ldb += db;
    isc_attach_database(d->status, 0, const_cast<char *>(ldb.toLocal8Bit().constData()),
                        &d->ibase, ba.size(), ba.data());
    if (d->isError(QT_TRANSLATE_NOOP("QIBaseDriver", "Error opening database"),
                   QSqlError::ConnectionError)) {
        setOpenError(true);
        return false;
    }

    setOpen(true);
    setOpenError(false);
#if (FB_API_VER >= 40)
    std::call_once(initTZMappingFlag, [d](){ d->initTZMappingCache(); });
    if (lastError().isValid())
    {
        setOpen(true);
        return false;
    }
#endif
    return true;
}

void QIBaseDriver::close()
{
    Q_D(QIBaseDriver);
    if (isOpen()) {

        if (d->eventBuffers.size()) {
            ISC_STATUS status[20];
            QMap<QString, QIBaseEventBuffer *>::const_iterator i;
            for (i = d->eventBuffers.constBegin(); i != d->eventBuffers.constEnd(); ++i) {
                QIBaseEventBuffer *eBuffer = i.value();
                eBuffer->subscriptionState = QIBaseEventBuffer::Finished;
                isc_cancel_events(status, &d->ibase, &eBuffer->eventId);
                qFreeEventBuffer(eBuffer);
            }
            d->eventBuffers.clear();
        }

        isc_detach_database(d->status, &d->ibase);
        d->ibase = 0;
        setOpen(false);
        setOpenError(false);
    }
}

QSqlResult *QIBaseDriver::createResult() const
{
    return new QIBaseResult(this);
}

bool QIBaseDriver::beginTransaction()
{
    Q_D(QIBaseDriver);
    if (!isOpen() || isOpenError())
        return false;
    if (d->trans)
        return false;

    isc_start_transaction(d->status, &d->trans, 1, &d->ibase, 0, NULL);
    return !d->isError(QT_TRANSLATE_NOOP("QIBaseDriver", "Could not start transaction"),
                       QSqlError::TransactionError);
}

bool QIBaseDriver::commitTransaction()
{
    Q_D(QIBaseDriver);
    if (!isOpen() || isOpenError())
        return false;
    if (!d->trans)
        return false;

    isc_commit_transaction(d->status, &d->trans);
    d->trans = 0;
    return !d->isError(QT_TRANSLATE_NOOP("QIBaseDriver", "Unable to commit transaction"),
                       QSqlError::TransactionError);
}

bool QIBaseDriver::rollbackTransaction()
{
    Q_D(QIBaseDriver);
    if (!isOpen() || isOpenError())
        return false;
    if (!d->trans)
        return false;

    isc_rollback_transaction(d->status, &d->trans);
    d->trans = 0;
    return !d->isError(QT_TRANSLATE_NOOP("QIBaseDriver", "Unable to rollback transaction"),
                       QSqlError::TransactionError);
}

QStringList QIBaseDriver::tables(QSql::TableType type) const
{
    QStringList res;
    if (!isOpen())
        return res;

    QString typeFilter;

    if (type == QSql::SystemTables) {
        typeFilter += "RDB$SYSTEM_FLAG != 0"_L1;
    } else if (type == (QSql::SystemTables | QSql::Views)) {
        typeFilter += "RDB$SYSTEM_FLAG != 0 OR RDB$VIEW_BLR NOT NULL"_L1;
    } else {
        if (!(type & QSql::SystemTables))
            typeFilter += "RDB$SYSTEM_FLAG = 0 AND "_L1;
        if (!(type & QSql::Views))
            typeFilter += "RDB$VIEW_BLR IS NULL AND "_L1;
        if (!(type & QSql::Tables))
            typeFilter += "RDB$VIEW_BLR IS NOT NULL AND "_L1;
        if (!typeFilter.isEmpty())
            typeFilter.chop(5);
    }
    if (!typeFilter.isEmpty())
        typeFilter.prepend("where "_L1);

    QSqlQuery q(createResult());
    q.setForwardOnly(true);
    if (!q.exec("select rdb$relation_name from rdb$relations "_L1 + typeFilter))
        return res;
    while (q.next())
        res << q.value(0).toString().simplified();

    return res;
}

QSqlRecord QIBaseDriver::record(const QString& tablename) const
{
    QSqlRecord rec;
    if (!isOpen())
        return rec;

    const QString table = stripDelimiters(tablename, QSqlDriver::TableName);
    QSqlQuery q(createResult());
    q.setForwardOnly(true);
    q.exec("SELECT a.RDB$FIELD_NAME, b.RDB$FIELD_TYPE, b.RDB$FIELD_LENGTH, "
           "b.RDB$FIELD_SCALE, b.RDB$FIELD_PRECISION, a.RDB$NULL_FLAG "
           "FROM RDB$RELATION_FIELDS a, RDB$FIELDS b "
           "WHERE b.RDB$FIELD_NAME = a.RDB$FIELD_SOURCE "
           "AND a.RDB$RELATION_NAME = '"_L1 + table + "' "
           "ORDER BY a.RDB$FIELD_POSITION"_L1);

    while (q.next()) {
        int type = q.value(1).toInt();
        bool hasScale = q.value(3).toInt() < 0;
        QSqlField f(q.value(0).toString().simplified(), QMetaType(qIBaseTypeName(type, hasScale)), tablename);
        if (hasScale) {
            f.setLength(q.value(4).toInt());
            f.setPrecision(qAbs(q.value(3).toInt()));
        } else {
            f.setLength(q.value(2).toInt());
            f.setPrecision(0);
        }
        f.setRequired(q.value(5).toInt() > 0);
        f.setSqlType(type);

        rec.append(f);
    }
    return rec;
}

QSqlIndex QIBaseDriver::primaryIndex(const QString &table) const
{
    QSqlIndex index(table);
    if (!isOpen())
        return index;

    const QString tablename = stripDelimiters(table, QSqlDriver::TableName);
    QSqlQuery q(createResult());
    q.setForwardOnly(true);
    q.exec("SELECT a.RDB$INDEX_NAME, b.RDB$FIELD_NAME, d.RDB$FIELD_TYPE, d.RDB$FIELD_SCALE "
           "FROM RDB$RELATION_CONSTRAINTS a, RDB$INDEX_SEGMENTS b, RDB$RELATION_FIELDS c, RDB$FIELDS d "
           "WHERE a.RDB$CONSTRAINT_TYPE = 'PRIMARY KEY' "
           "AND a.RDB$RELATION_NAME = '"_L1 + tablename +
           " 'AND a.RDB$INDEX_NAME = b.RDB$INDEX_NAME "
           "AND c.RDB$RELATION_NAME = a.RDB$RELATION_NAME "
           "AND c.RDB$FIELD_NAME = b.RDB$FIELD_NAME "
           "AND d.RDB$FIELD_NAME = c.RDB$FIELD_SOURCE "
           "ORDER BY b.RDB$FIELD_POSITION"_L1);

    while (q.next()) {
        QSqlField field(q.value(1).toString().simplified(),
                        QMetaType(qIBaseTypeName(q.value(2).toInt(), q.value(3).toInt() < 0)),
                        tablename);
        index.append(field); //TODO: asc? desc?
        index.setName(q.value(0).toString());
    }

    return index;
}

QString QIBaseDriver::formatValue(const QSqlField &field, bool trimStrings) const
{
    switch (field.metaType().id()) {
    case QMetaType::QDateTime: {
        QDateTime datetime = field.value().toDateTime();
        if (datetime.isValid())
            return u'\'' + QString::number(datetime.date().year()) + u'-' +
                QString::number(datetime.date().month()) + u'-' +
                QString::number(datetime.date().day()) + u' ' +
                QString::number(datetime.time().hour()) + u':' +
                QString::number(datetime.time().minute()) + u':' +
                QString::number(datetime.time().second()) + u'.' +
                QString::number(datetime.time().msec()).rightJustified(3, u'0', true) +
                u'\'';
        else
            return "NULL"_L1;
    }
    case QMetaType::QTime: {
        QTime time = field.value().toTime();
        if (time.isValid())
            return u'\'' + QString::number(time.hour()) + u':' +
                QString::number(time.minute()) + u':' +
                QString::number(time.second()) + u'.' +
                QString::number(time.msec()).rightJustified(3, u'0', true) +
                u'\'';
        else
            return "NULL"_L1;
    }
    case QMetaType::QDate: {
        QDate date = field.value().toDate();
        if (date.isValid())
            return u'\'' + QString::number(date.year()) + u'-' +
                QString::number(date.month()) + u'-' +
                QString::number(date.day()) + u'\'';
            else
                return "NULL"_L1;
    }
    default:
        return QSqlDriver::formatValue(field, trimStrings);
    }
}

QVariant QIBaseDriver::handle() const
{
    Q_D(const QIBaseDriver);
    return QVariant(QMetaType::fromType<isc_db_handle>(), &d->ibase);
}

static ISC_EVENT_CALLBACK qEventCallback(char *result, ISC_USHORT length, const ISC_UCHAR *updated)
{
    if (!updated)
        return 0;


    memcpy(result, updated, length);
    qMutex()->lock();
    QIBaseDriver *driver = qBufferDriverMap()->value(result);
    qMutex()->unlock();

    // We use an asynchronous call (i.e., queued connection) because the event callback
    // is executed in a different thread than the one in which the driver lives.
    if (driver)
        QMetaObject::invokeMethod(driver, "qHandleEventNotification", Qt::QueuedConnection, Q_ARG(void *, reinterpret_cast<void *>(result)));

    return 0;
}

bool QIBaseDriver::subscribeToNotification(const QString &name)
{
    Q_D(QIBaseDriver);
    if (!isOpen()) {
        qWarning("QIBaseDriver::subscribeFromNotificationImplementation: database not open.");
        return false;
    }

    if (d->eventBuffers.contains(name)) {
        qWarning("QIBaseDriver::subscribeToNotificationImplementation: already subscribing to '%ls'.",
            qUtf16Printable(name));
        return false;
    }

    QIBaseEventBuffer *eBuffer = new QIBaseEventBuffer;
    eBuffer->subscriptionState = QIBaseEventBuffer::Starting;
    eBuffer->bufferLength = isc_event_block(&eBuffer->eventBuffer,
                                            &eBuffer->resultBuffer,
                                            1,
                                            name.toLocal8Bit().constData());

    qMutex()->lock();
    qBufferDriverMap()->insert(eBuffer->resultBuffer, this);
    qMutex()->unlock();

    d->eventBuffers.insert(name, eBuffer);

    ISC_STATUS status[20];
    isc_que_events(status,
                   &d->ibase,
                   &eBuffer->eventId,
                   eBuffer->bufferLength,
                   eBuffer->eventBuffer,
                   reinterpret_cast<ISC_EVENT_CALLBACK>(reinterpret_cast<void *>
                                                                     (&qEventCallback)),
                   eBuffer->resultBuffer);

    if (status[0] == 1 && status[1]) {
        setLastError(QSqlError(QString::fromLatin1("Could not subscribe to event notifications for %1.").arg(name)));
        d->eventBuffers.remove(name);
        qFreeEventBuffer(eBuffer);
        return false;
    }

    return true;
}

bool QIBaseDriver::unsubscribeFromNotification(const QString &name)
{
    Q_D(QIBaseDriver);
    if (!isOpen()) {
        qWarning("QIBaseDriver::unsubscribeFromNotificationImplementation: database not open.");
        return false;
    }

    if (!d->eventBuffers.contains(name)) {
        qWarning("QIBaseDriver::QIBaseSubscriptionState not subscribed to '%ls'.",
            qUtf16Printable(name));
        return false;
    }

    QIBaseEventBuffer *eBuffer = d->eventBuffers.value(name);
    ISC_STATUS status[20];
    eBuffer->subscriptionState = QIBaseEventBuffer::Finished;
    isc_cancel_events(status, &d->ibase, &eBuffer->eventId);

    if (status[0] == 1 && status[1]) {
        setLastError(QSqlError(QString::fromLatin1("Could not unsubscribe from event notifications for %1.").arg(name)));
        return false;
    }

    d->eventBuffers.remove(name);
    qFreeEventBuffer(eBuffer);

    return true;
}

QStringList QIBaseDriver::subscribedToNotifications() const
{
    Q_D(const QIBaseDriver);
    return QStringList(d->eventBuffers.keys());
}

void QIBaseDriver::qHandleEventNotification(void *updatedResultBuffer)
{
    Q_D(QIBaseDriver);
    QMap<QString, QIBaseEventBuffer *>::const_iterator i;
    for (i = d->eventBuffers.constBegin(); i != d->eventBuffers.constEnd(); ++i) {
        QIBaseEventBuffer* eBuffer = i.value();
        if (reinterpret_cast<void *>(eBuffer->resultBuffer) != updatedResultBuffer)
            continue;

        ISC_ULONG counts[20];
        memset(counts, 0, sizeof(counts));
        isc_event_counts(counts, eBuffer->bufferLength, eBuffer->eventBuffer, eBuffer->resultBuffer);
        if (counts[0]) {

            if (eBuffer->subscriptionState == QIBaseEventBuffer::Subscribed)
                emit notification(i.key(), QSqlDriver::UnknownSource, QVariant());
            else if (eBuffer->subscriptionState == QIBaseEventBuffer::Starting)
                eBuffer->subscriptionState = QIBaseEventBuffer::Subscribed;

            ISC_STATUS status[20];
            isc_que_events(status,
                           &d->ibase,
                           &eBuffer->eventId,
                           eBuffer->bufferLength,
                           eBuffer->eventBuffer,
                           reinterpret_cast<ISC_EVENT_CALLBACK>(reinterpret_cast<void *>
                                                                (&qEventCallback)),
                                   eBuffer->resultBuffer);
            if (Q_UNLIKELY(status[0] == 1 && status[1])) {
                qCritical("QIBaseDriver::qHandleEventNotification: could not resubscribe to '%ls'",
                    qUtf16Printable(i.key()));
            }

            return;
        }
    }
}

QString QIBaseDriver::escapeIdentifier(const QString &identifier, IdentifierType) const
{
    QString res = identifier;
    if (!identifier.isEmpty() && !identifier.startsWith(u'"') && !identifier.endsWith(u'"') ) {
        res.replace(u'"', "\"\""_L1);
        res.replace(u'.', "\".\""_L1);
        res = u'"' + res + u'"';
    }
    return res;
}

int QIBaseDriver::maximumIdentifierLength(IdentifierType type) const
{
    Q_UNUSED(type);
    return 31;
}

QT_END_NAMESPACE
