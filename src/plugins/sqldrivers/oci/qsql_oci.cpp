// Copyright (C) 2021 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#include "qsql_oci_p.h"

#include <qcoreapplication.h>
#include <qdatetime.h>
#include <qdebug.h>
#include <qlist.h>
#include <qloggingcategory.h>
#include <qmetatype.h>
#if QT_CONFIG(regularexpression)
#include <qregularexpression.h>
#endif
#include <qshareddata.h>
#include <qsqlerror.h>
#include <qsqlfield.h>
#include <qsqlindex.h>
#include <qsqlquery.h>
#include <QtSql/private/qsqlcachedresult_p.h>
#include <QtSql/private/qsqldriver_p.h>
#include <qstringlist.h>
#if QT_CONFIG(timezone)
#include <qtimezone.h>
#endif
#include <qvariant.h>
#include <qvarlengtharray.h>

// This is needed for oracle oci when compiling with mingw-w64 headers
#if defined(__MINGW64_VERSION_MAJOR) && defined(_WIN64)
#define _int64 __int64
#endif

#include <oci.h>

#include <stdlib.h>

#define QOCI_DYNAMIC_CHUNK_SIZE 65535
#define QOCI_PREFETCH_MEM  10240

// setting this define will allow using a query from a different
// thread than its database connection.
// warning - this is not fully tested and can lead to race conditions
#define QOCI_THREADED

//#define QOCI_DEBUG

Q_DECLARE_OPAQUE_POINTER(QOCIResult*)
Q_DECLARE_METATYPE(QOCIResult*)
Q_DECLARE_OPAQUE_POINTER(OCIEnv*)
Q_DECLARE_METATYPE(OCIEnv*)
Q_DECLARE_OPAQUE_POINTER(OCIStmt*)
Q_DECLARE_METATYPE(OCIStmt*)

QT_BEGIN_NAMESPACE

Q_STATIC_LOGGING_CATEGORY(lcOci, "qt.sql.oci")

using namespace Qt::StringLiterals;

#if Q_BYTE_ORDER == Q_LITTLE_ENDIAN
enum { QOCIEncoding = 2002 }; // AL16UTF16LE
#else
enum { QOCIEncoding = 2000 }; // AL16UTF16
#endif

#ifdef OCI_ATTR_CHARSET_FORM
// Always set the OCI_ATTR_CHARSET_FORM to SQLCS_NCHAR is safe
// because Oracle server will deal with the implicit Conversion
// Between CHAR and NCHAR.
// see: http://download.oracle.com/docs/cd/A91202_01/901_doc/appdev.901/a89857/oci05bnd.htm#422705
static const ub1 qOraCharsetForm = SQLCS_NCHAR;
#endif

#if defined (OCI_UTF16ID)
static const ub2 qOraCharset = OCI_UTF16ID;
#else
static const ub2 qOraCharset = OCI_UCS2ID;
#endif

typedef QVarLengthArray<sb2, 32> IndicatorArray;
typedef QVarLengthArray<ub4, 32> SizeArray;

static QByteArray qMakeOCINumber(const qlonglong &ll, OCIError *err);
static QByteArray qMakeOCINumber(const qulonglong& ull, OCIError* err);

static qlonglong qMakeLongLong(const char* ociNumber, OCIError* err);
static qulonglong qMakeULongLong(const char* ociNumber, OCIError* err);

static QString qOraWarn(OCIError *err, int *errorCode = 0);

#ifndef Q_CC_SUN
static // for some reason, Sun CC can't use qOraWarning when it's declared static
#endif
void qOraWarning(const char* msg, OCIError *err);
static QSqlError qMakeError(const QString& errString, QSqlError::ErrorType type, OCIError *err);



class QOCIRowId: public QSharedData
{
public:
    QOCIRowId(OCIEnv *env);
    ~QOCIRowId();

    OCIRowid *id;

private:
    QOCIRowId(const QOCIRowId &other): QSharedData(other) { Q_ASSERT(false); }
};

QOCIRowId::QOCIRowId(OCIEnv *env)
    : id(0)
{
    OCIDescriptorAlloc (env, reinterpret_cast<dvoid **>(&id),
                        OCI_DTYPE_ROWID, 0, 0);
}

QOCIRowId::~QOCIRowId()
{
    if (id)
        OCIDescriptorFree(id, OCI_DTYPE_ROWID);
}

class QOCIDateTime
{
public:
    QOCIDateTime(OCIEnv *env, OCIError *err, const QDateTime &dt = QDateTime());
    ~QOCIDateTime();
    OCIDateTime *dateTime;
    static QDateTime fromOCIDateTime(OCIEnv *env, OCIError *err, OCIDateTime *dt);
};

QOCIDateTime::QOCIDateTime(OCIEnv *env, OCIError *err, const QDateTime &dt)
    : dateTime(nullptr)
{
    OCIDescriptorAlloc(env, reinterpret_cast<void**>(&dateTime), OCI_DTYPE_TIMESTAMP_TZ, 0, 0);
    if (dt.isValid()) {
        const QDate date = dt.date();
        const QTime time = dt.time();
        // Zone in +hh:mm format
        const QString timeZone = dt.toString("ttt"_L1);
        const OraText *tz = reinterpret_cast<const OraText *>(timeZone.utf16());
        OCIDateTimeConstruct(env, err, dateTime, date.year(), date.month(), date.day(), time.hour(),
                             time.minute(), time.second(), time.msec() * 1000000,
                             const_cast<OraText *>(tz), timeZone.length() * sizeof(QChar));
    }
}

QOCIDateTime::~QOCIDateTime()
{
    if (dateTime != nullptr)
        OCIDescriptorFree(dateTime, OCI_DTYPE_TIMESTAMP_TZ);
}

QDateTime QOCIDateTime::fromOCIDateTime(OCIEnv *env, OCIError *err, OCIDateTime *dateTime)
{
    sb2 year;
    ub1 month, day, hour, minute, second;
    ub4 nsec;
    sb1 tzHour, tzMinute;

    OCIDateTimeGetDate(env, err, dateTime, &year, &month, &day);
    OCIDateTimeGetTime(env, err, dateTime, &hour, &minute, &second, &nsec);
    OCIDateTimeGetTimeZoneOffset(env, err, dateTime, &tzHour, &tzMinute);
    int secondsOffset = (qAbs(tzHour) * 60 + tzMinute) * 60;
    if (tzHour < 0)
        secondsOffset = -secondsOffset;
    // OCIDateTimeGetTime gives "fractions of second" as nanoseconds
    return QDateTime(QDate(year, month, day), QTime(hour, minute, second, nsec / 1000000),
                     QTimeZone::fromSecondsAheadOfUtc(secondsOffset));
}

struct TempStorage {
    QList<QByteArray> rawData;
    QList<QOCIDateTime *> dateTimes;
};

typedef QSharedDataPointer<QOCIRowId> QOCIRowIdPointer;
QT_BEGIN_INCLUDE_NAMESPACE
Q_DECLARE_METATYPE(QOCIRowIdPointer)
QT_END_INCLUDE_NAMESPACE

class QOCIDriverPrivate : public QSqlDriverPrivate
{
    Q_DECLARE_PUBLIC(QOCIDriver)

public:
    QOCIDriverPrivate();

    OCIEnv *env = nullptr;
    OCISvcCtx *svc = nullptr;
    OCIServer *srvhp = nullptr;
    OCISession *authp = nullptr;
    OCITrans *trans = nullptr;
    OCIError *err = nullptr;
    ub4 authMode = OCI_DEFAULT;
    bool transaction = false;
    int serverVersion = -1;
    int prefetchRows = -1;
    int prefetchMem = QOCI_PREFETCH_MEM;
    QString user;

    void allocErrorHandle();
};

class QOCICols;

class QOCIResultPrivate: public QSqlCachedResultPrivate
{
public:
    Q_DECLARE_PUBLIC(QOCIResult)
    Q_DECLARE_SQLDRIVER_PRIVATE(QOCIDriver)
    QOCIResultPrivate(QOCIResult *q, const QOCIDriver *drv);
    ~QOCIResultPrivate();

    QOCICols *cols = nullptr;
    OCIEnv *env;
    OCIError *err = nullptr;
    OCISvcCtx *&svc;
    OCIStmt *stmtp = nullptr;
    bool transaction;
    int serverVersion;
    int prefetchRows, prefetchMem;

    void setStatementAttributes();
    int bindValue(OCIStmt *stmtp, OCIBind **hbnd, OCIError *err, int pos,
                  const QVariant &val, dvoid *indPtr, ub4 *tmpSize, TempStorage &tmpStorage);
    int bindValues(QVariantList &values, IndicatorArray &indicators, SizeArray &tmpSizes,
                   TempStorage &tmpStorage);
    void outValues(QVariantList &values, IndicatorArray &indicators,
                   TempStorage &tmpStorage);
    inline bool isOutValue(int i) const
    { Q_Q(const QOCIResult); return q->bindValueType(i) & QSql::Out; }
    inline bool isBinaryValue(int i) const
    { Q_Q(const QOCIResult); return q->bindValueType(i) & QSql::Binary; }

    void setCharset(dvoid* handle, ub4 type) const
    {
        int r = 0;
        Q_ASSERT(handle);

#ifdef OCI_ATTR_CHARSET_FORM
        r = OCIAttrSet(handle,
                       type,
                       // this const cast is safe since OCI doesn't touch
                       // the charset.
                       const_cast<void *>(static_cast<const void *>(&qOraCharsetForm)),
                       0,
                       OCI_ATTR_CHARSET_FORM,
                       //Strange Oracle bug: some Oracle servers crash the server process with non-zero error handle (mostly for 10g).
                       //So ignore the error message here.
                       0);
        #ifdef QOCI_DEBUG
        if (r != 0)
            qCWarning(lcOci, "QOCIResultPrivate::setCharset: Couldn't set OCI_ATTR_CHARSET_FORM.");
        #endif
#endif

        r = OCIAttrSet(handle,
                       type,
                       // this const cast is safe since OCI doesn't touch
                       // the charset.
                       const_cast<void *>(static_cast<const void *>(&qOraCharset)),
                       0,
                       OCI_ATTR_CHARSET_ID,
                       err);
        if (r != 0)
            qOraWarning("QOCIResultPrivate::setCharsetI Couldn't set OCI_ATTR_CHARSET_ID: ", err);

    }
};

void QOCIResultPrivate::setStatementAttributes()
{
    Q_ASSERT(stmtp);

    int r = 0;

    if (prefetchRows >= 0) {
        r = OCIAttrSet(stmtp,
                       OCI_HTYPE_STMT,
                       &prefetchRows,
                       0,
                       OCI_ATTR_PREFETCH_ROWS,
                       err);
        if (r != 0)
            qOraWarning("QOCIResultPrivate::setStatementAttributes:"
                        " Couldn't set OCI_ATTR_PREFETCH_ROWS: ", err);
    }
    if (prefetchMem >= 0) {
        r = OCIAttrSet(stmtp,
                       OCI_HTYPE_STMT,
                       &prefetchMem,
                       0,
                       OCI_ATTR_PREFETCH_MEMORY,
                       err);
        if (r != 0)
            qOraWarning("QOCIResultPrivate::setStatementAttributes:"
                        " Couldn't set OCI_ATTR_PREFETCH_MEMORY: ", err);
    }
}

int QOCIResultPrivate::bindValue(OCIStmt *stmtp, OCIBind **hbnd, OCIError *err, int pos,
                                 const QVariant &val, dvoid *indPtr, ub4 *tmpSize, TempStorage &tmpStorage)
{
    int r = OCI_SUCCESS;
    void *data = const_cast<void *>(val.constData());

    switch (val.typeId()) {
    case QMetaType::QByteArray:
        r = OCIBindByPos2(stmtp, hbnd, err,
                          pos + 1,
                          isOutValue(pos)
                            ?  const_cast<char *>(reinterpret_cast<QByteArray *>(data)->constData())
                            : reinterpret_cast<QByteArray *>(data)->data(),
                          reinterpret_cast<QByteArray *>(data)->size(),
                          SQLT_BIN, indPtr, 0, 0, 0, 0, OCI_DEFAULT);
        break;
    case QMetaType::QTime:
    case QMetaType::QDate:
    case QMetaType::QDateTime: {
        QOCIDateTime *ptr = new QOCIDateTime(env, err, val.toDateTime());
        r = OCIBindByPos2(stmtp, hbnd, err,
                          pos + 1,
                          &ptr->dateTime,
                          sizeof(OCIDateTime *),
                          SQLT_TIMESTAMP_TZ, indPtr, 0, 0, 0, 0, OCI_DEFAULT);
        tmpStorage.dateTimes.append(ptr);
        break;
    }
    case QMetaType::Int:
        r = OCIBindByPos2(stmtp, hbnd, err,
                          pos + 1,
                          // if it's an out value, the data is already detached
                          // so the const cast is safe.
                          const_cast<void *>(data),
                          sizeof(int),
                          SQLT_INT, indPtr, 0, 0, 0, 0, OCI_DEFAULT);
        break;
    case QMetaType::UInt:
        r = OCIBindByPos2(stmtp, hbnd, err,
                          pos + 1,
                          // if it's an out value, the data is already detached
                          // so the const cast is safe.
                          const_cast<void *>(data),
                          sizeof(uint),
                          SQLT_UIN, indPtr, 0, 0, 0, 0, OCI_DEFAULT);
        break;
    case QMetaType::LongLong:
    {
        QByteArray ba = qMakeOCINumber(val.toLongLong(), err);
        r = OCIBindByPos2(stmtp, hbnd, err,
                          pos + 1,
                          ba.data(),
                          ba.size(),
                          SQLT_VNU, indPtr, 0, 0, 0, 0, OCI_DEFAULT);
        tmpStorage.rawData.append(ba);
        break;
    }
    case QMetaType::ULongLong:
    {
        QByteArray ba = qMakeOCINumber(val.toULongLong(), err);
        r = OCIBindByPos2(stmtp, hbnd, err,
                          pos + 1,
                          ba.data(),
                          ba.size(),
                          SQLT_VNU, indPtr, 0, 0, 0, 0, OCI_DEFAULT);
        tmpStorage.rawData.append(ba);
        break;
    }
    case QMetaType::Double:
        r = OCIBindByPos2(stmtp, hbnd, err,
                          pos + 1,
                          // if it's an out value, the data is already detached
                          // so the const cast is safe.
                          const_cast<void *>(data),
                          sizeof(double),
                          SQLT_FLT, indPtr, 0, 0, 0, 0, OCI_DEFAULT);
        break;
    case QMetaType::QString: {
        const QString s = val.toString();
        if (isBinaryValue(pos)) {
            r = OCIBindByPos2(stmtp, hbnd, err,
                              pos + 1,
                              const_cast<ushort *>(s.utf16()),
                              s.length() * sizeof(QChar),
                              SQLT_LNG, indPtr, 0, 0, 0, 0, OCI_DEFAULT);
            break;
        } else if (!isOutValue(pos)) {
            // don't detach the string
            r = OCIBindByPos2(stmtp, hbnd, err,
                              pos + 1,
                              // safe since oracle doesn't touch OUT values
                              const_cast<ushort *>(s.utf16()),
                              (s.length() + 1) * sizeof(QChar),
                              SQLT_STR, indPtr, 0, 0, 0, 0, OCI_DEFAULT);
            if (r == OCI_SUCCESS)
                setCharset(*hbnd, OCI_HTYPE_BIND);
            break;
        }
    } // fall through for OUT values
    Q_FALLTHROUGH();
    default: {
        if (val.typeId() >= QMetaType::User) {
            if (val.canConvert<QOCIRowIdPointer>() && !isOutValue(pos)) {
                // use a const pointer to prevent a detach
                const QOCIRowIdPointer rptr = qvariant_cast<QOCIRowIdPointer>(val);
                r = OCIBindByPos2(stmtp, hbnd, err,
                                  pos + 1,
                                  // it's an IN value, so const_cast is ok
                                  const_cast<OCIRowid **>(&rptr->id),
                                  -1,
                                  SQLT_RDD, indPtr, 0, 0, 0, 0, OCI_DEFAULT);
            } else if (val.canConvert<QOCIResult *>() && isOutValue(pos)) {
                QOCIResult *res = qvariant_cast<QOCIResult *>(val);
                QOCIResultPrivate *resPrivate = static_cast<QOCIResultPrivate *>(res->d_ptr);

                if (res->internal_prepare()) {
                    r = OCIBindByPos2(stmtp, hbnd, err,
                                      pos + 1,
                                      &resPrivate->stmtp,
                                      (sb4)0,
                                      SQLT_RSET, indPtr, 0, 0, 0, 0, OCI_DEFAULT);

                    res->isCursor = true;
                }
            } else {
                qCWarning(lcOci, "Unknown bind variable");
                r = OCI_ERROR;
            }
        } else {
            const QString s = val.toString();
            // create a deep-copy
            QByteArray ba(reinterpret_cast<const char *>(s.utf16()), (s.length() + 1) * sizeof(QChar));
            if (isOutValue(pos)) {
                ba.reserve((s.capacity() + 1) * sizeof(QChar));
                *tmpSize = ba.size();
                r = OCIBindByPos2(stmtp, hbnd, err,
                                  pos + 1,
                                  ba.data(),
                                  ba.capacity(),
                                  SQLT_STR, indPtr, tmpSize, 0, 0, 0, OCI_DEFAULT);
            } else {
                r = OCIBindByPos2(stmtp, hbnd, err,
                                  pos + 1,
                                  ba.data(),
                                  ba.size(),
                                  SQLT_STR, indPtr, 0, 0, 0, 0, OCI_DEFAULT);
            }
            if (r == OCI_SUCCESS)
                setCharset(*hbnd, OCI_HTYPE_BIND);
            tmpStorage.rawData.append(ba);
        }
        break;
    } // default case
    } // switch
    if (r != OCI_SUCCESS)
        qOraWarning("QOCIResultPrivate::bindValue:", err);
    return r;
}

int QOCIResultPrivate::bindValues(QVariantList &values, IndicatorArray &indicators,
                                  SizeArray &tmpSizes, TempStorage &tmpStorage)
{
    int r = OCI_SUCCESS;
    for (int i = 0; i < values.count(); ++i) {
        if (isOutValue(i))
            values[i].detach();
        const QVariant &val = values.at(i);

        OCIBind * hbnd = nullptr; // Oracle handles these automatically
        sb2 *indPtr = &indicators[i];
        *indPtr = QSqlResultPrivate::isVariantNull(val) ? -1 : 0;

        bindValue(stmtp, &hbnd, err, i, val, indPtr, &tmpSizes[i], tmpStorage);
    }
    return r;
}

// will assign out value and remove its temp storage.
static void qOraOutValue(QVariant &value, TempStorage &tmpStorage, OCIEnv *env, OCIError* err)
{
    switch (value.typeId()) {
    case QMetaType::QTime:
        value = QOCIDateTime::fromOCIDateTime(env, err,
                                              tmpStorage.dateTimes.takeFirst()->dateTime).time();
        break;
    case QMetaType::QDate:
        value = QOCIDateTime::fromOCIDateTime(env, err,
                                              tmpStorage.dateTimes.takeFirst()->dateTime).date();
        break;
    case QMetaType::QDateTime:
        value = QOCIDateTime::fromOCIDateTime(env, err,
                                              tmpStorage.dateTimes.takeFirst()->dateTime);
        break;
    case QMetaType::LongLong:
        value = qMakeLongLong(tmpStorage.rawData.takeFirst(), err);
        break;
    case QMetaType::ULongLong:
        value = qMakeULongLong(tmpStorage.rawData.takeFirst(), err);
        break;
    case QMetaType::QString:
        value = QString(
                reinterpret_cast<const QChar *>(tmpStorage.rawData.takeFirst().constData()));
        break;
    default:
        break; //nothing
    }
}

void QOCIResultPrivate::outValues(QVariantList &values, IndicatorArray &indicators,
                                  TempStorage &tmpStorage)
{
    for (int i = 0; i < values.count(); ++i) {

        if (!isOutValue(i))
            continue;

        qOraOutValue(values[i], tmpStorage, env, err);

        auto typ = values.at(i).metaType();
        if (indicators[i] == -1) // NULL
            values[i] = QVariant(typ);
        else
            values[i] = QVariant(typ, values.at(i).constData());
    }
}


QOCIDriverPrivate::QOCIDriverPrivate()
    : QSqlDriverPrivate()
{
    dbmsType = QSqlDriver::Oracle;
}

void QOCIDriverPrivate::allocErrorHandle()
{
    Q_ASSERT(!err);
    int r = OCIHandleAlloc(env,
                           reinterpret_cast<void **>(&err),
                           OCI_HTYPE_ERROR,
                           0, nullptr);
    if (r != OCI_SUCCESS)
        qCWarning(lcOci, "QOCIDriver: unable to allocate error handle");
}

struct OraFieldInfo
{
    QString name;
    QMetaType type;
    ub1 oraIsNull;
    ub4 oraType;
    sb1 oraScale;
    ub4 oraLength; // size in bytes
    ub4 oraFieldLength; // amount of characters
    sb2 oraPrecision;
};

QString qOraWarn(OCIError *err, int *errorCode)
{
    sb4 errcode;
    text errbuf[1024];
    errbuf[0] = 0;
    errbuf[1] = 0;

    OCIErrorGet(err,
                1,
                0,
                &errcode,
                errbuf,
                sizeof(errbuf),
                OCI_HTYPE_ERROR);
    if (errorCode)
        *errorCode = errcode;
    return QString(reinterpret_cast<const QChar *>(errbuf));
}

void qOraWarning(const char* msg, OCIError *err)
{
    qCWarning(lcOci, "%s %ls", msg, qUtf16Printable(qOraWarn(err)));
}

static int qOraErrorNumber(OCIError *err)
{
    sb4 errcode;
    OCIErrorGet(err,
                1,
                0,
                &errcode,
                0,
                0,
                OCI_HTYPE_ERROR);
    return errcode;
}

QSqlError qMakeError(const QString& errString, QSqlError::ErrorType type, OCIError *err)
{
    int errorCode = 0;
    const QString oraErrorString = qOraWarn(err, &errorCode);
    return QSqlError(errString, oraErrorString, type,
                     errorCode != -1 ? QString::number(errorCode) : QString());
}

QMetaType qDecodeOCIType(const QString& ocitype, QSql::NumericalPrecisionPolicy precisionPolicy)
{
    int type = QMetaType::UnknownType;
    if (ocitype == "VARCHAR2"_L1 || ocitype == "VARCHAR"_L1
         || ocitype.startsWith("INTERVAL"_L1)
         || ocitype == "CHAR"_L1 || ocitype == "NVARCHAR2"_L1
         || ocitype == "NCHAR"_L1)
        type = QMetaType::QString;
    else if (ocitype == "NUMBER"_L1
             || ocitype == "FLOAT"_L1
             || ocitype == "BINARY_FLOAT"_L1
             || ocitype == "BINARY_DOUBLE"_L1) {
        switch(precisionPolicy) {
            case QSql::LowPrecisionInt32:
                type = QMetaType::Int;
                break;
            case QSql::LowPrecisionInt64:
                type = QMetaType::LongLong;
                break;
            case QSql::LowPrecisionDouble:
                type = QMetaType::Double;
                break;
            case QSql::HighPrecision:
            default:
                type = QMetaType::QString;
                break;
        }
    }
    else if (ocitype == "LONG"_L1 || ocitype == "NCLOB"_L1 || ocitype == "CLOB"_L1)
        type = QMetaType::QByteArray;
    else if (ocitype == "RAW"_L1 || ocitype == "LONG RAW"_L1
             || ocitype == "ROWID"_L1 || ocitype == "BLOB"_L1
             || ocitype == "CFILE"_L1 || ocitype == "BFILE"_L1)
        type = QMetaType::QByteArray;
    else if (ocitype == "DATE"_L1 ||  ocitype.startsWith("TIME"_L1))
        type = QMetaType::QDateTime;
    else if (ocitype == "UNDEFINED"_L1)
        type = QMetaType::UnknownType;
    if (type == QMetaType::UnknownType)
        qCWarning(lcOci, "qDecodeOCIType: unknown type: %ls", qUtf16Printable(ocitype));
    return QMetaType(type);
}

QMetaType qDecodeOCIType(int ocitype, QSql::NumericalPrecisionPolicy precisionPolicy)
{
    int type = QMetaType::UnknownType;
    switch (ocitype) {
    case SQLT_STR:
    case SQLT_VST:
    case SQLT_CHR:
    case SQLT_AFC:
    case SQLT_VCS:
    case SQLT_AVC:
    case SQLT_RDD:
    case SQLT_LNG:
#ifdef SQLT_INTERVAL_YM
    case SQLT_INTERVAL_YM:
#endif
#ifdef SQLT_INTERVAL_DS
    case SQLT_INTERVAL_DS:
#endif
        type = QMetaType::QString;
        break;
    case SQLT_INT:
        type = QMetaType::Int;
        break;
    case SQLT_FLT:
    case SQLT_NUM:
    case SQLT_VNU:
    case SQLT_UIN:
        switch(precisionPolicy) {
            case QSql::LowPrecisionInt32:
                type = QMetaType::Int;
                break;
            case QSql::LowPrecisionInt64:
                type = QMetaType::LongLong;
                break;
            case QSql::LowPrecisionDouble:
                type = QMetaType::Double;
                break;
            case QSql::HighPrecision:
            default:
                type = QMetaType::QString;
                break;
        }
        break;
    case SQLT_VBI:
    case SQLT_BIN:
    case SQLT_LBI:
    case SQLT_LVC:
    case SQLT_LVB:
    case SQLT_BLOB:
    case SQLT_CLOB:
    case SQLT_FILE:
    case SQLT_NTY:
    case SQLT_REF:
    case SQLT_RID:
        type = QMetaType::QByteArray;
        break;
    case SQLT_DAT:
    case SQLT_ODT:
    case SQLT_TIMESTAMP:
    case SQLT_TIMESTAMP_TZ:
    case SQLT_TIMESTAMP_LTZ:
        type = QMetaType::QDateTime;
        break;
    default:
        qCWarning(lcOci, "qDecodeOCIType: unknown OCI datatype: %d", ocitype);
        break;
    }
        return QMetaType(type);
}

static QSqlField qFromOraInf(const OraFieldInfo &ofi)
{
    QSqlField f(ofi.name, ofi.type);
    f.setRequired(ofi.oraIsNull == 0);

    if (ofi.type.id() == QMetaType::QString && ofi.oraType != SQLT_NUM && ofi.oraType != SQLT_VNU)
        f.setLength(ofi.oraFieldLength);
    else
        f.setLength(ofi.oraPrecision == 0 ? 38 : int(ofi.oraPrecision));

    f.setPrecision(ofi.oraScale);
    return f;
}

/*!
  \internal

   Convert qlonglong to the internal Oracle OCINumber format.
  */
QByteArray qMakeOCINumber(const qlonglong& ll, OCIError* err)
{
    QByteArray ba(sizeof(OCINumber), 0);

    OCINumberFromInt(err,
                     &ll,
                     sizeof(qlonglong),
                     OCI_NUMBER_SIGNED,
                     reinterpret_cast<OCINumber*>(ba.data()));
    return ba;
}

/*!
  \internal

   Convert qulonglong to the internal Oracle OCINumber format.
  */
QByteArray qMakeOCINumber(const qulonglong& ull, OCIError* err)
{
    QByteArray ba(sizeof(OCINumber), 0);

    OCINumberFromInt(err,
                     &ull,
                     sizeof(qlonglong),
                     OCI_NUMBER_UNSIGNED,
                     reinterpret_cast<OCINumber*>(ba.data()));
    return ba;
}

qlonglong qMakeLongLong(const char* ociNumber, OCIError* err)
{
    qlonglong qll = 0;
    OCINumberToInt(err, reinterpret_cast<const OCINumber *>(ociNumber), sizeof(qlonglong),
                   OCI_NUMBER_SIGNED, &qll);
    return qll;
}

qulonglong qMakeULongLong(const char* ociNumber, OCIError* err)
{
    qulonglong qull = 0;
    OCINumberToInt(err, reinterpret_cast<const OCINumber *>(ociNumber), sizeof(qulonglong),
                   OCI_NUMBER_UNSIGNED, &qull);
    return qull;
}

class QOCICols
{
public:
    QOCICols(qsizetype size, QOCIResultPrivate* dp);

    int readPiecewise(QVariantList &values, int index = 0);
    int readLOBs(QVariantList &values, int index = 0);
    qsizetype fieldFromDefine(OCIDefine *d) const;
    void getValues(QVariantList &v, int index);
    inline qsizetype size() const { return fieldInf.size(); }
    static bool execBatch(QOCIResultPrivate *d, QVariantList &boundValues, bool arrayBind);

    QSqlRecord rec;

private:
    char* create(int position, int size);
    OCILobLocator ** createLobLocator(int position, OCIEnv* env);
    OraFieldInfo qMakeOraField(const QOCIResultPrivate* p, OCIParam* param) const;

    struct OraFieldInf
    {
        ~OraFieldInf();

        char *data = nullptr;
        int len = 0;
        sb2 ind = 0;
        QMetaType typ;
        ub4 oraType = 0;
        OCIDefine *def = nullptr;
        OCILobLocator *lob = nullptr;
        void *dataPtr = nullptr;
    };

    QList<OraFieldInf> fieldInf;
    const QOCIResultPrivate *const d;
};

QOCICols::OraFieldInf::~OraFieldInf()
{
    delete [] data;
    if (lob) {
        int r = OCIDescriptorFree(lob, OCI_DTYPE_LOB);
        if (r != 0)
            qCWarning(lcOci, "QOCICols: Cannot free LOB descriptor");
    }
    if (dataPtr) {
        switch (typ.id()) {
        case QMetaType::QDate:
        case QMetaType::QTime:
        case QMetaType::QDateTime: {
            int r = OCIDescriptorFree(dataPtr, OCI_DTYPE_TIMESTAMP_TZ);
            if (r != OCI_SUCCESS)
                qCWarning(lcOci, "QOCICols: Cannot free OCIDateTime descriptor");
            break;
        }
        default:
            break;
        }
    }
}

QOCICols::QOCICols(qsizetype size, QOCIResultPrivate* dp)
    : fieldInf(size), d(dp)
{
    ub4 dataSize = 0;
    OCIDefine *dfn = nullptr;
    int r;

    OCIParam *param = nullptr;
    sb4 parmStatus = 0;
    ub4 count = 1;
    int idx = 0;
    parmStatus = OCIParamGet(d->stmtp,
                             OCI_HTYPE_STMT,
                             d->err,
                             reinterpret_cast<void **>(&param),
                             count);

    while (parmStatus == OCI_SUCCESS) {
        OraFieldInfo ofi = qMakeOraField(d, param);
        if (ofi.oraType == SQLT_RDD)
            dataSize = 50;
#ifdef SQLT_INTERVAL_YM
#ifdef SQLT_INTERVAL_DS
        else if (ofi.oraType == SQLT_INTERVAL_YM || ofi.oraType == SQLT_INTERVAL_DS)
            // since we are binding interval datatype as string,
            // we are not interested in the number of bytes but characters.
            dataSize = 50;  // magic number
#endif //SQLT_INTERVAL_DS
#endif //SQLT_INTERVAL_YM
        else if (ofi.oraType == SQLT_NUM || ofi.oraType == SQLT_VNU){
            if (ofi.oraPrecision > 0)
                dataSize = (ofi.oraPrecision + 1) * sizeof(utext);
            else
                dataSize = (38 + 1) * sizeof(utext);
        }
        else
            dataSize = ofi.oraLength;

        fieldInf[idx].typ = ofi.type;
        fieldInf[idx].oraType = ofi.oraType;
        rec.append(qFromOraInf(ofi));

        switch (ofi.type.id()) {
        case QMetaType::QDateTime:
            r = OCIDescriptorAlloc(d->env, (void **)&fieldInf[idx].dataPtr, OCI_DTYPE_TIMESTAMP_TZ, 0, 0);
            if (r != OCI_SUCCESS) {
                qCWarning(lcOci, "QOCICols: Unable to allocate the OCIDateTime descriptor");
                break;
            }
            r = OCIDefineByPos(d->stmtp,
                               &dfn,
                               d->err,
                               count,
                               &fieldInf[idx].dataPtr,
                               sizeof(OCIDateTime *),
                               SQLT_TIMESTAMP_TZ,
                               &(fieldInf[idx].ind),
                               0, 0, OCI_DEFAULT);
            break;
        case QMetaType::Double:
            r = OCIDefineByPos(d->stmtp,
                               &dfn,
                               d->err,
                               count,
                               create(idx, sizeof(double) - 1),
                               sizeof(double),
                               SQLT_FLT,
                               &(fieldInf[idx].ind),
                               0, 0, OCI_DEFAULT);
            break;
        case QMetaType::Int:
            r = OCIDefineByPos(d->stmtp,
                               &dfn,
                               d->err,
                               count,
                               create(idx, sizeof(qint32) - 1),
                               sizeof(qint32),
                               SQLT_INT,
                               &(fieldInf[idx].ind),
                               0, 0, OCI_DEFAULT);
            break;
        case QMetaType::LongLong:
            r = OCIDefineByPos(d->stmtp,
                               &dfn,
                               d->err,
                               count,
                               create(idx, sizeof(OCINumber)),
                               sizeof(OCINumber),
                               SQLT_VNU,
                               &(fieldInf[idx].ind),
                               0, 0, OCI_DEFAULT);
            break;
        case QMetaType::QByteArray:
            // RAW and LONG RAW fields can't be bound to LOB locators
            if (ofi.oraType == SQLT_BIN) {
//                                qDebug("binding SQLT_BIN");
                r = OCIDefineByPos(d->stmtp,
                                   &dfn,
                                   d->err,
                                   count,
                                   create(idx, dataSize),
                                   dataSize,
                                   SQLT_BIN,
                                   &(fieldInf[idx].ind),
                                   0, 0, OCI_DYNAMIC_FETCH);
            } else if (ofi.oraType == SQLT_LBI) {
//                                    qDebug("binding SQLT_LBI");
                r = OCIDefineByPos(d->stmtp,
                                   &dfn,
                                   d->err,
                                   count,
                                   0,
                                   SB4MAXVAL,
                                   SQLT_LBI,
                                   &(fieldInf[idx].ind),
                                   0, 0, OCI_DYNAMIC_FETCH);
            } else if (ofi.oraType == SQLT_CLOB) {
                r = OCIDefineByPos(d->stmtp,
                                   &dfn,
                                   d->err,
                                   count,
                                   createLobLocator(idx, d->env),
                                   -1,
                                   SQLT_CLOB,
                                   &(fieldInf[idx].ind),
                                   0, 0, OCI_DEFAULT);
            } else {
//                 qDebug("binding SQLT_BLOB");
                r = OCIDefineByPos(d->stmtp,
                                   &dfn,
                                   d->err,
                                   count,
                                   createLobLocator(idx, d->env),
                                   -1,
                                   SQLT_BLOB,
                                   &(fieldInf[idx].ind),
                                   0, 0, OCI_DEFAULT);
            }
            break;
        case QMetaType::QString:
            if (ofi.oraType == SQLT_LNG) {
                r = OCIDefineByPos(d->stmtp,
                        &dfn,
                        d->err,
                        count,
                        0,
                        SB4MAXVAL,
                        SQLT_LNG,
                        &(fieldInf[idx].ind),
                        0, 0, OCI_DYNAMIC_FETCH);
            } else {
                dataSize += dataSize + sizeof(QChar);
                //qDebug("OCIDefineByPosStr(%d): %d", count, dataSize);
                r = OCIDefineByPos(d->stmtp,
                        &dfn,
                        d->err,
                        count,
                        create(idx, dataSize),
                        dataSize,
                        SQLT_STR,
                        &(fieldInf[idx].ind),
                        0, 0, OCI_DEFAULT);
                if (r == 0)
                    d->setCharset(dfn, OCI_HTYPE_DEFINE);
            }
           break;
        default:
            // this should make enough space even with character encoding
            dataSize = (dataSize + 1) * sizeof(utext) ;
            //qDebug("OCIDefineByPosDef(%d): %d", count, dataSize);
            r = OCIDefineByPos(d->stmtp,
                                &dfn,
                                d->err,
                                count,
                                create(idx, dataSize),
                                dataSize+1,
                                SQLT_STR,
                                &(fieldInf[idx].ind),
                                0, 0, OCI_DEFAULT);
            break;
        }
        if (r != 0)
            qOraWarning("QOCICols::bind:", d->err);
        fieldInf[idx].def = dfn;
        ++count;
        ++idx;
        parmStatus = OCIParamGet(d->stmtp,
                                  OCI_HTYPE_STMT,
                                  d->err,
                                  reinterpret_cast<void **>(&param),
                                  count);
    }
}

char* QOCICols::create(int position, int size)
{
    char* c = new char[size+1];
    // Oracle may not fill fixed width fields
    memset(c, 0, size+1);
    fieldInf[position].data = c;
    fieldInf[position].len = size;
    return c;
}

OCILobLocator **QOCICols::createLobLocator(int position, OCIEnv* env)
{
    OCILobLocator *& lob = fieldInf[position].lob;
    int r = OCIDescriptorAlloc(env,
                               reinterpret_cast<void **>(&lob),
                               OCI_DTYPE_LOB,
                               0,
                               0);
    if (r != 0) {
        qCWarning(lcOci, "QOCICols: Cannot create LOB locator");
        lob = 0;
    }
    return &lob;
}

int QOCICols::readPiecewise(QVariantList &values, int index)
{
    OCIDefine*     dfn;
    ub4            typep;
    ub1            in_outp;
    ub4            iterp;
    ub4            idxp;
    ub1            piecep;
    sword          status;
    text           col [QOCI_DYNAMIC_CHUNK_SIZE+1];
    int            r = 0;
    bool           nullField;

    do {
        r = OCIStmtGetPieceInfo(d->stmtp, d->err, reinterpret_cast<void **>(&dfn), &typep,
                                 &in_outp, &iterp, &idxp, &piecep);
        if (r != OCI_SUCCESS)
            qOraWarning("OCIResultPrivate::readPiecewise: unable to get piece info:", d->err);
        qsizetype fieldNum = fieldFromDefine(dfn);
        bool isStringField = fieldInf.at(fieldNum).oraType == SQLT_LNG;
        ub4 chunkSize = QOCI_DYNAMIC_CHUNK_SIZE;
        nullField = false;
        r  = OCIStmtSetPieceInfo(dfn, OCI_HTYPE_DEFINE,
                                 d->err, col,
                                 &chunkSize, piecep, NULL, NULL);
        if (r != OCI_SUCCESS)
            qOraWarning("OCIResultPrivate::readPiecewise: unable to set piece info:", d->err);
        status = OCIStmtFetch (d->stmtp, d->err, 1, OCI_FETCH_NEXT, OCI_DEFAULT);
        if (status == -1) {
            sb4 errcode;
            OCIErrorGet(d->err, 1, 0, &errcode, 0, 0,OCI_HTYPE_ERROR);
            switch (errcode) {
            case 1405: /* NULL */
                nullField = true;
                break;
            default:
                qOraWarning("OCIResultPrivate::readPiecewise: unable to fetch next:", d->err);
                break;
            }
        }
        if (status == OCI_NO_DATA)
            break;
        if (nullField || !chunkSize) {
            fieldInf[fieldNum].ind = -1;
        } else {
            if (isStringField) {
                QString str = values.at(fieldNum + index).toString();
                str += QString(reinterpret_cast<const QChar *>(col), chunkSize / 2);
                values[fieldNum + index] = str;
                fieldInf[fieldNum].ind = 0;
            } else {
                QByteArray ba = values.at(fieldNum + index).toByteArray();
                int sz = ba.size();
                ba.resize(sz + chunkSize);
                memcpy(ba.data() + sz, reinterpret_cast<char *>(col), chunkSize);
                values[fieldNum + index] = ba;
                fieldInf[fieldNum].ind = 0;
            }
        }
    } while (status == OCI_SUCCESS_WITH_INFO || status == OCI_NEED_DATA);
    return r;
}

OraFieldInfo QOCICols::qMakeOraField(const QOCIResultPrivate* p, OCIParam* param) const
{
    OraFieldInfo ofi;
    ub2 colType(0);
    text *colName = nullptr;
    ub4 colNameLen(0);
    sb1 colScale(0);
    ub2 colLength(0);
    ub2 colFieldLength(0);
    sb2 colPrecision(0);
    ub1 colIsNull(0);
    int r(0);

    r = OCIAttrGet(param,
                   OCI_DTYPE_PARAM,
                   &colType,
                   0,
                   OCI_ATTR_DATA_TYPE,
                   p->err);
    if (r != 0)
        qOraWarning("qMakeOraField:", p->err);

    r = OCIAttrGet(param,
                   OCI_DTYPE_PARAM,
                   &colName,
                   &colNameLen,
                   OCI_ATTR_NAME,
                   p->err);
    if (r != 0)
        qOraWarning("qMakeOraField:", p->err);

    r = OCIAttrGet(param,
                   OCI_DTYPE_PARAM,
                   &colLength,
                   0,
                   OCI_ATTR_DATA_SIZE, /* in bytes */
                   p->err);
    if (r != 0)
        qOraWarning("qMakeOraField:", p->err);

#ifdef OCI_ATTR_CHAR_SIZE
    r = OCIAttrGet(param,
                   OCI_DTYPE_PARAM,
                   &colFieldLength,
                   0,
                   OCI_ATTR_CHAR_SIZE,
                   p->err);
    if (r != 0)
        qOraWarning("qMakeOraField:", p->err);
#else
    // for Oracle8.
    colFieldLength = colLength;
#endif

    r = OCIAttrGet(param,
                   OCI_DTYPE_PARAM,
                   &colPrecision,
                   0,
                   OCI_ATTR_PRECISION,
                   p->err);
    if (r != 0)
        qOraWarning("qMakeOraField:", p->err);

    r = OCIAttrGet(param,
                   OCI_DTYPE_PARAM,
                   &colScale,
                   0,
                   OCI_ATTR_SCALE,
                   p->err);
    if (r != 0)
        qOraWarning("qMakeOraField:", p->err);
    r = OCIAttrGet(param,
                   OCI_DTYPE_PARAM,
                   &colType,
                   0,
                   OCI_ATTR_DATA_TYPE,
                   p->err);
    if (r != 0)
        qOraWarning("qMakeOraField:", p->err);
    r = OCIAttrGet(param,
                   OCI_DTYPE_PARAM,
                   &colIsNull,
                   0,
                   OCI_ATTR_IS_NULL,
                   p->err);
    if (r != 0)
        qOraWarning("qMakeOraField:", p->err);

    QMetaType type = qDecodeOCIType(colType, p->q_func()->numericalPrecisionPolicy());

    if (type.id() == QMetaType::Int) {
        if ((colLength == 22 && colPrecision == 0 && colScale == 0) || colScale > 0)
            type = QMetaType(QMetaType::QString);
    }

    // bind as double if the precision policy asks for it
    if (((colType == SQLT_FLT) || (colType == SQLT_NUM))
            && (p->q_func()->numericalPrecisionPolicy() == QSql::LowPrecisionDouble)) {
        type = QMetaType(QMetaType::Double);
    }

    // bind as int32 or int64 if the precision policy asks for it
    if ((colType == SQLT_NUM) || (colType == SQLT_VNU) || (colType == SQLT_UIN)
            || (colType == SQLT_INT)) {
        if (p->q_func()->numericalPrecisionPolicy() == QSql::LowPrecisionInt64)
            type = QMetaType(QMetaType::LongLong);
        else if (p->q_func()->numericalPrecisionPolicy() == QSql::LowPrecisionInt32)
            type = QMetaType(QMetaType::Int);
    }

    if (colType == SQLT_BLOB)
        colLength = 0;

    // colNameLen is length in bytes
    ofi.name = QString(reinterpret_cast<const QChar*>(colName), colNameLen / 2);
    ofi.type = type;
    ofi.oraType = colType;
    ofi.oraFieldLength = colFieldLength;
    ofi.oraLength = colLength;
    ofi.oraScale = colScale;
    ofi.oraPrecision = colPrecision;
    ofi.oraIsNull = colIsNull;

    return ofi;
}

struct QOCIBatchColumn
{
    OCIBind* bindh = nullptr;
    ub2 bindAs = 0;
    ub4 maxLen = 0;
    ub4 recordCount = 0;
    char* data  = nullptr;
    ub4* lengths = nullptr;
    sb2* indicators = nullptr;
    ub4 maxarr_len = 0;
    ub4 curelep = 0;
};

struct QOCIBatchCleanupHandler
{
    inline QOCIBatchCleanupHandler(QList<QOCIBatchColumn> &columns)
        : col(columns) {}

    ~QOCIBatchCleanupHandler()
    {
        // deleting storage, length and indicator arrays
        for ( int j = 0; j < col.count(); ++j){
            delete[] col[j].lengths;
            delete[] col[j].indicators;
            delete[] col[j].data;
        }
    }

    QList<QOCIBatchColumn> &col;
};

bool QOCICols::execBatch(QOCIResultPrivate *d, QVariantList &boundValues, bool arrayBind)
{
    qsizetype columnCount = boundValues.count();
    if (boundValues.isEmpty() || columnCount == 0)
        return false;

#ifdef QOCI_DEBUG
    qCDebug(lcOci) << "columnCount:" << columnCount << boundValues;
#endif

    sword r;

    QVarLengthArray<QMetaType> fieldTypes;
    for (qsizetype i = 0; i < columnCount; ++i) {
        QMetaType tp = boundValues.at(i).metaType();
        fieldTypes.append(tp.id() == QMetaType::QVariantList ? boundValues.at(i).toList().value(0).metaType() : tp);
    }
    SizeArray tmpSizes(columnCount);
    QList<QOCIBatchColumn> columns(columnCount);
    QOCIBatchCleanupHandler cleaner(columns);
    TempStorage tmpStorage;

    // figuring out buffer sizes
    for (qsizetype i = 0; i < columnCount; ++i) {

        if (boundValues.at(i).typeId() != QMetaType::QVariantList) {

            // not a list - create a deep-copy of the single value
            QOCIBatchColumn &singleCol = columns[i];
            singleCol.indicators = new sb2[1];
            *singleCol.indicators = QSqlResultPrivate::isVariantNull(boundValues.at(i)) ? -1 : 0;

            r = d->bindValue(d->stmtp, &singleCol.bindh, d->err, i,
                             boundValues.at(i), singleCol.indicators, &tmpSizes[i], tmpStorage);

            if (r != OCI_SUCCESS && r != OCI_SUCCESS_WITH_INFO) {
                qOraWarning("QOCIPrivate::execBatch: unable to bind column:", d->err);
                d->q_func()->setLastError(qMakeError(QCoreApplication::translate("QOCIResult",
                         "Unable to bind column for batch execute"),
                         QSqlError::StatementError, d->err));
                return false;
            }
            continue;
        }

        QOCIBatchColumn &col = columns[i];
        col.recordCount = boundValues.at(i).toList().count();

        col.lengths = new ub4[col.recordCount];
        col.indicators = new sb2[col.recordCount];
        col.maxarr_len = col.recordCount;
        col.curelep = col.recordCount;

        switch (fieldTypes[i].id()) {
            case QMetaType::QTime:
            case QMetaType::QDate:
            case QMetaType::QDateTime:
                col.bindAs = SQLT_TIMESTAMP_TZ;
                col.maxLen = sizeof(OCIDateTime *);
                break;

            case QMetaType::Int:
                col.bindAs = SQLT_INT;
                col.maxLen = sizeof(int);
                break;

            case QMetaType::UInt:
                col.bindAs = SQLT_UIN;
                col.maxLen = sizeof(uint);
                break;

            case QMetaType::LongLong:
                col.bindAs = SQLT_VNU;
                col.maxLen = sizeof(OCINumber);
                break;

            case QMetaType::ULongLong:
                col.bindAs = SQLT_VNU;
                col.maxLen = sizeof(OCINumber);
                break;

            case QMetaType::Double:
                col.bindAs = SQLT_FLT;
                col.maxLen = sizeof(double);
                break;

            case QMetaType::QString: {
                col.bindAs = SQLT_STR;
                for (uint j = 0; j < col.recordCount; ++j) {
                    uint len;
                    if (d->isOutValue(i))
                        len = boundValues.at(i).toList().at(j).toString().capacity() + 1;
                    else
                        len = boundValues.at(i).toList().at(j).toString().length() + 1;
                    if (len > col.maxLen)
                        col.maxLen = len;
                }
                col.maxLen *= sizeof(QChar);
                break; }

            case QMetaType::QByteArray:
            default: {
                if (fieldTypes[i].id() >= QMetaType::User) {
                    col.bindAs = SQLT_RDD;
                    col.maxLen = sizeof(OCIRowid*);
                } else {
                    col.bindAs = SQLT_LBI;
                    for (uint j = 0; j < col.recordCount; ++j) {
                        if (d->isOutValue(i))
                            col.lengths[j] = boundValues.at(i).toList().at(j).toByteArray().capacity();
                        else
                            col.lengths[j] = boundValues.at(i).toList().at(j).toByteArray().size();
                        if (col.lengths[j] > col.maxLen)
                            col.maxLen = col.lengths[j];
                    }
                }
                break;
            }
        }

        col.data = new char[col.maxLen * col.recordCount];
        memset(col.data, 0, col.maxLen * col.recordCount);

        // we may now populate column with data
        for (uint row = 0; row < col.recordCount; ++row) {
            const QVariant val = boundValues.at(i).toList().at(row);

            if (QSqlResultPrivate::isVariantNull(val) && !d->isOutValue(i)) {
                columns[i].indicators[row] = -1;
                columns[i].lengths[row] = 0;
            } else {
                columns[i].indicators[row] = 0;
                char *dataPtr = columns[i].data + (columns[i].maxLen * row);
                switch (fieldTypes[i].id()) {
                    case QMetaType::QTime:
                    case QMetaType::QDate:
                    case QMetaType::QDateTime:{
                        columns[i].lengths[row] = columns[i].maxLen;
                        QOCIDateTime *date = new QOCIDateTime(d->env, d->err, val.toDateTime());
                        *reinterpret_cast<OCIDateTime**>(dataPtr) = date->dateTime;
                        tmpStorage.dateTimes.append(date);
                        break;
                    }
                    case QMetaType::Int:
                        columns[i].lengths[row] = columns[i].maxLen;
                        *reinterpret_cast<int*>(dataPtr) = val.toInt();
                        break;

                    case QMetaType::UInt:
                        columns[i].lengths[row] = columns[i].maxLen;
                        *reinterpret_cast<uint*>(dataPtr) = val.toUInt();
                        break;

                    case QMetaType::LongLong:
                    {
                        columns[i].lengths[row] = columns[i].maxLen;
                        const QByteArray ba = qMakeOCINumber(val.toLongLong(), d->err);
                        Q_ASSERT(ba.size() == columns[i].maxLen);
                        memcpy(dataPtr, ba.constData(), columns[i].maxLen);
                        break;
                    }
                    case QMetaType::ULongLong:
                    {
                        columns[i].lengths[row] = columns[i].maxLen;
                        const QByteArray ba = qMakeOCINumber(val.toULongLong(), d->err);
                        Q_ASSERT(ba.size() == columns[i].maxLen);
                        memcpy(dataPtr, ba.constData(), columns[i].maxLen);
                        break;
                    }
                    case QMetaType::Double:
                         columns[i].lengths[row] = columns[i].maxLen;
                         *reinterpret_cast<double*>(dataPtr) = val.toDouble();
                         break;

                    case QMetaType::QString: {
                        const QString s = val.toString();
                        columns[i].lengths[row] = ub2((s.length() + 1) * sizeof(QChar));
                        memcpy(dataPtr, s.utf16(), columns[i].lengths[row]);
                        break;
                    }
                    case QMetaType::QByteArray:
                    default: {
                        if (fieldTypes[i].id() >= QMetaType::User) {
                            if (val.canConvert<QOCIRowIdPointer>()) {
                                const QOCIRowIdPointer rptr = qvariant_cast<QOCIRowIdPointer>(val);
                                *reinterpret_cast<OCIRowid**>(dataPtr) = rptr->id;
                                columns[i].lengths[row] = 0;
                                break;
                            }
                        } else {
                            const QByteArray ba = val.toByteArray();
                            columns[i].lengths[row] = ba.size();
                            memcpy(dataPtr, ba.constData(), ba.size());
                        }
                        break;
                    }
                }
            }
        }

        QOCIBatchColumn &bindColumn = columns[i];

#ifdef QOCI_DEBUG
            qCDebug(lcOci, "OCIBindByPos2(%p, %p, %p, %d, %p, %d, %d, %p, %p, 0, %d, %p, OCI_DEFAULT)",
            d->stmtp, &bindColumn.bindh, d->err, i + 1, bindColumn.data,
            bindColumn.maxLen, bindColumn.bindAs, bindColumn.indicators, bindColumn.lengths,
            arrayBind ? bindColumn.maxarr_len : 0, arrayBind ? &bindColumn.curelep : 0);

        for (int ii = 0; ii < (int)bindColumn.recordCount; ++ii) {
            qCDebug(lcOci, " record %d: indicator %d, length %d", ii, bindColumn.indicators[ii],
                    bindColumn.lengths[ii]);
        }
#endif


        // binding the column
        r = OCIBindByPos2(
                d->stmtp, &bindColumn.bindh, d->err, i + 1,
                bindColumn.data,
                bindColumn.maxLen,
                bindColumn.bindAs,
                bindColumn.indicators,
                bindColumn.lengths,
                0,
                arrayBind ? bindColumn.maxarr_len : 0,
                arrayBind ? &bindColumn.curelep : 0,
                OCI_DEFAULT);

#ifdef QOCI_DEBUG
        qCDebug(lcOci, "After OCIBindByPos: r = %d, bindh = %p", r, bindColumn.bindh);
#endif

        if (r != OCI_SUCCESS && r != OCI_SUCCESS_WITH_INFO) {
            qOraWarning("QOCIPrivate::execBatch: unable to bind column:", d->err);
            d->q_func()->setLastError(qMakeError(QCoreApplication::translate("QOCIResult",
                     "Unable to bind column for batch execute"),
                     QSqlError::StatementError, d->err));
            return false;
        }

        r = OCIBindArrayOfStruct (
                columns[i].bindh, d->err,
                columns[i].maxLen,
                sizeof(columns[i].indicators[0]),
                sizeof(columns[i].lengths[0]),
                0);

        if (r != OCI_SUCCESS && r != OCI_SUCCESS_WITH_INFO) {
            qOraWarning("QOCIPrivate::execBatch: unable to bind column:", d->err);
            d->q_func()->setLastError(qMakeError(QCoreApplication::translate("QOCIResult",
                     "Unable to bind column for batch execute"),
                     QSqlError::StatementError, d->err));
            return false;
        }
    }

    //finally we can execute
    r = OCIStmtExecute(d->svc, d->stmtp, d->err,
                       arrayBind ? 1 : columns[0].recordCount,
                       0, NULL, NULL,
                       d->transaction ? OCI_DEFAULT : OCI_COMMIT_ON_SUCCESS);

    if (r != OCI_SUCCESS && r != OCI_SUCCESS_WITH_INFO) {
        qOraWarning("QOCIPrivate::execBatch: unable to execute batch statement:", d->err);
        d->q_func()->setLastError(qMakeError(QCoreApplication::translate("QOCIResult",
                        "Unable to execute batch statement"),
                        QSqlError::StatementError, d->err));
        return false;
    }

    // for out parameters we copy data back to value list
    for (qsizetype i = 0; i < columnCount; ++i) {

        if (!d->isOutValue(i))
            continue;

        if (auto tp = boundValues.at(i).metaType(); tp.id() != QMetaType::QVariantList) {
            qOraOutValue(boundValues[i], tmpStorage, d->env, d->err);
            if (*columns[i].indicators == -1)
                boundValues[i] = QVariant(tp);
            continue;
        }

        QVariantList *list = static_cast<QVariantList *>(const_cast<void*>(boundValues.at(i).data()));

        char* data = columns[i].data;
        for (uint r = 0; r < columns[i].recordCount; ++r){

            if (columns[i].indicators[r] == -1) {
                (*list)[r] = QVariant(fieldTypes[i]);
                continue;
            }

            switch(columns[i].bindAs) {

                case SQLT_TIMESTAMP_TZ:
                    (*list)[r] = QOCIDateTime::fromOCIDateTime(d->env, d->err,
                                    *reinterpret_cast<OCIDateTime **>(data + r * columns[i].maxLen));
                    break;
                case SQLT_INT:
                    (*list)[r] =  *reinterpret_cast<int*>(data + r * columns[i].maxLen);
                    break;

                case SQLT_UIN:
                    (*list)[r] =  *reinterpret_cast<uint*>(data + r * columns[i].maxLen);
                    break;

                case SQLT_VNU:
                {
                    switch (boundValues.at(i).typeId()) {
                    case QMetaType::LongLong:
                        (*list)[r] =  qMakeLongLong(data + r * columns[i].maxLen, d->err);
                        break;
                    case QMetaType::ULongLong:
                        (*list)[r] =  qMakeULongLong(data + r * columns[i].maxLen, d->err);
                        break;
                    default:
                        break;
                    }
                    break;
                }

                case SQLT_FLT:
                    (*list)[r] =  *reinterpret_cast<double*>(data + r * columns[i].maxLen);
                    break;

                case SQLT_STR:
                    (*list)[r] =  QString(reinterpret_cast<const QChar *>(data
                                                                + r * columns[i].maxLen));
                    break;

                default:
                    (*list)[r] =  QByteArray(data + r * columns[i].maxLen, columns[i].maxLen);
                break;
            }
        }
    }

    d->q_func()->setSelect(false);
    d->q_func()->setAt(QSql::BeforeFirstRow);
    d->q_func()->setActive(true);

    qDeleteAll(tmpStorage.dateTimes);
    return true;
}

template<class T, int sz>
int qReadLob(T &buf, const QOCIResultPrivate *d, OCILobLocator *lob)
{
    ub1 csfrm;
    ub4 amount;
    int r;

    // Read this from the database, don't assume we know what it is set to
    r = OCILobCharSetForm(d->env, d->err, lob, &csfrm);
    if (r != OCI_SUCCESS) {
        qOraWarning("OCIResultPrivate::readLobs: Couldn't get LOB char set form: ", d->err);
        csfrm = 0;
    }

    // Get the length of the LOB (this is in characters)
    r = OCILobGetLength(d->svc, d->err, lob, &amount);
    if (r == OCI_SUCCESS) {
        if (amount == 0) {
            // Short cut for null LOBs
            buf.resize(0);
            return OCI_SUCCESS;
        }
    } else {
        qOraWarning("OCIResultPrivate::readLobs: Couldn't get LOB length: ", d->err);
        return r;
    }

    // Resize the buffer to hold the LOB contents
    buf.resize(amount);

    // Read the LOB into the buffer
    r = OCILobRead(d->svc,
                   d->err,
                   lob,
                   &amount,
                   1,
                   buf.data(),
                   buf.size() * sz, // this argument is in bytes, not characters
                   0,
                   0,
                   // Extract the data from a CLOB in UTF-16 (ie. what QString uses internally)
                   sz == 1 ? ub2(0) : ub2(QOCIEncoding),
                   csfrm);

    if (r != OCI_SUCCESS)
        qOraWarning("OCIResultPrivate::readLOBs: Cannot read LOB: ", d->err);

    return r;
}

int QOCICols::readLOBs(QVariantList &values, int index)
{
    OCILobLocator *lob;
    int r = OCI_SUCCESS;

    for (qsizetype i = 0; i < size(); ++i) {
        const OraFieldInf &fi = fieldInf.at(i);
        if (fi.ind == -1 || !(lob = fi.lob))
            continue;

        bool isClob = fi.oraType == SQLT_CLOB;
        QVariant var;

        if (isClob) {
            QString str;
            r = qReadLob<QString, sizeof(QChar)>(str, d, lob);
            var = str;
        } else {
            QByteArray buf;
            r = qReadLob<QByteArray, sizeof(char)>(buf, d, lob);
            var = buf;
        }
        if (r == OCI_SUCCESS)
            values[index + i] = var;
        else
            break;
    }
    return r;
}

qsizetype QOCICols::fieldFromDefine(OCIDefine *d) const
{
    for (qsizetype i = 0; i < fieldInf.size(); ++i) {
        if (fieldInf.at(i).def == d)
            return i;
    }
    return -1;
}

void QOCICols::getValues(QVariantList &v, int index)
{
    for (qsizetype i = 0; i < fieldInf.size(); ++i) {
        const OraFieldInf &fld = fieldInf.at(i);

        if (fld.ind == -1) {
            // got a NULL value
            v[index + i] = QVariant(fld.typ);
            continue;
        }

        if (fld.oraType == SQLT_BIN || fld.oraType == SQLT_LBI || fld.oraType == SQLT_LNG)
            continue; // already fetched piecewise

        switch (fld.typ.id()) {
        case QMetaType::QDateTime:
            v[index + i] = QVariant(QOCIDateTime::fromOCIDateTime(d->env, d->err,
                                        reinterpret_cast<OCIDateTime *>(fld.dataPtr)));
            break;
        case QMetaType::Double:
        case QMetaType::Int:
        case QMetaType::LongLong:
            if (d->q_func()->numericalPrecisionPolicy() != QSql::HighPrecision) {
                if ((d->q_func()->numericalPrecisionPolicy() == QSql::LowPrecisionDouble)
                        && (fld.typ.id() == QMetaType::Double)) {
                    v[index + i] = *reinterpret_cast<double *>(fld.data);
                    break;
                } else if ((d->q_func()->numericalPrecisionPolicy() == QSql::LowPrecisionInt64)
                        && (fld.typ.id() == QMetaType::LongLong)) {
                    qint64 qll = 0;
                    int r = OCINumberToInt(d->err, reinterpret_cast<OCINumber *>(fld.data), sizeof(qint64),
                                   OCI_NUMBER_SIGNED, &qll);
                    if (r == OCI_SUCCESS)
                        v[index + i] = qll;
                    else
                        v[index + i] = QVariant();
                    break;
                } else if ((d->q_func()->numericalPrecisionPolicy() == QSql::LowPrecisionInt32)
                        && (fld.typ.id() == QMetaType::Int)) {
                    v[index + i] = *reinterpret_cast<int *>(fld.data);
                    break;
                }
            }
            // else fall through
        case QMetaType::QString:
            v[index + i] = QString(reinterpret_cast<const QChar *>(fld.data));
            break;
        case QMetaType::QByteArray:
            if (fld.len > 0)
                v[index + i] = QByteArray(fld.data, fld.len);
            else
                v[index + i] = QVariant(QMetaType(QMetaType::QByteArray));
            break;
        default:
            qCWarning(lcOci, "QOCICols::value: unknown data type");
            break;
        }
    }
}

QOCIResultPrivate::QOCIResultPrivate(QOCIResult *q, const QOCIDriver *drv)
    : QSqlCachedResultPrivate(q, drv),
      env(drv_d_func()->env),
      svc(drv_d_func()->svc),
      transaction(drv_d_func()->transaction),
      serverVersion(drv_d_func()->serverVersion),
      prefetchRows(drv_d_func()->prefetchRows),
      prefetchMem(drv_d_func()->prefetchMem)
{
    Q_ASSERT(!err);
    int r = OCIHandleAlloc(env,
                           reinterpret_cast<void **>(&err),
                           OCI_HTYPE_ERROR,
                           0, nullptr);
    if (r != OCI_SUCCESS)
        qCWarning(lcOci, "QOCIResult: unable to alloc error handle");
}

QOCIResultPrivate::~QOCIResultPrivate()
{
    delete cols;

    if (stmtp && OCIHandleFree(stmtp, OCI_HTYPE_STMT) != OCI_SUCCESS)
        qCWarning(lcOci, "~QOCIResult: unable to free statement handle");

    if (OCIHandleFree(err, OCI_HTYPE_ERROR) != OCI_SUCCESS)
        qCWarning(lcOci, "~QOCIResult: unable to free error report handle");
}


////////////////////////////////////////////////////////////////////////////

QOCIResult::QOCIResult(const QOCIDriver *db)
    : QSqlCachedResult(*new QOCIResultPrivate(this, db))
{
    isCursor = false;
}

QOCIResult::~QOCIResult()
{
}

QVariant QOCIResult::handle() const
{
    Q_D(const QOCIResult);
    return QVariant::fromValue(d->stmtp);
}

bool QOCIResult::reset (const QString& query)
{
    if (!prepare(query))
        return false;
    return exec();
}

bool QOCIResult::gotoNext(QSqlCachedResult::ValueCache &values, int index)
{
    Q_D(QOCIResult);
    if (at() == QSql::AfterLastRow)
        return false;

    bool piecewise = false;
    int r = OCI_SUCCESS;
    r = OCIStmtFetch(d->stmtp, d->err, 1, OCI_FETCH_NEXT, OCI_DEFAULT);

    if (index < 0) //not interested in values
        return r == OCI_SUCCESS || r == OCI_SUCCESS_WITH_INFO;

    switch (r) {
    case OCI_SUCCESS:
        break;
    case OCI_SUCCESS_WITH_INFO:
        qOraWarning("QOCIResult::gotoNext: SuccessWithInfo: ", d->err);
        r = OCI_SUCCESS; //ignore it
        break;
    case OCI_NO_DATA:
        // end of rowset
        return false;
    case OCI_NEED_DATA:
        piecewise = true;
        r = OCI_SUCCESS;
        break;
    case OCI_ERROR:
        if (qOraErrorNumber(d->err) == 1406) {
            qCWarning(lcOci, "QOCI Warning: data truncated for %ls", qUtf16Printable(lastQuery()));
            r = OCI_SUCCESS; /* ignore it */
            break;
        }
        // fall through
    default:
        qOraWarning("QOCIResult::gotoNext: ", d->err);
        setLastError(qMakeError(QCoreApplication::translate("QOCIResult",
                                "Unable to goto next"),
                                QSqlError::StatementError, d->err));
        break;
    }

    // need to read piecewise before assigning values
    if (r == OCI_SUCCESS && piecewise)
        r = d->cols->readPiecewise(values, index);

    if (r == OCI_SUCCESS)
        d->cols->getValues(values, index);
    if (r == OCI_SUCCESS)
        r = d->cols->readLOBs(values, index);
    if (r != OCI_SUCCESS)
        setAt(QSql::AfterLastRow);
    return r == OCI_SUCCESS || r == OCI_SUCCESS_WITH_INFO;
}

int QOCIResult::size()
{
    return -1;
}

int QOCIResult::numRowsAffected()
{
    Q_D(QOCIResult);
    int rowCount;
    OCIAttrGet(d->stmtp,
                OCI_HTYPE_STMT,
                &rowCount,
                NULL,
                OCI_ATTR_ROW_COUNT,
                d->err);
    return rowCount;
}

bool QOCIResult::internal_prepare()
{
    Q_D(QOCIResult);
    int r = 0;
    QString noStr;
    QSqlResult::prepare(noStr);

    delete d->cols;
    d->cols = nullptr;
    QSqlCachedResult::cleanup();

    if (d->stmtp) {
        r = OCIHandleFree(d->stmtp, OCI_HTYPE_STMT);
        if (r == OCI_SUCCESS)
            d->stmtp = nullptr;
        else
            qOraWarning("QOCIResult::prepare: unable to free statement handle:", d->err);
    }

    r = OCIHandleAlloc(d->env,
                       reinterpret_cast<void **>(&d->stmtp),
                       OCI_HTYPE_STMT,
                       0, nullptr);
    if (r != OCI_SUCCESS) {
        qOraWarning("QOCIResult::prepare: unable to alloc statement:", d->err);
        setLastError(qMakeError(QCoreApplication::translate("QOCIResult",
                     "Unable to alloc statement"), QSqlError::StatementError, d->err));
        return false;
    }
    d->setStatementAttributes();

    return true;
}

bool QOCIResult::prepare(const QString& query)
{
    if (query.isEmpty())
        return false;

    if (!internal_prepare())
        return false;

    int r;
    const OraText *txt = reinterpret_cast<const OraText *>(query.utf16());
    const auto len = ub4(query.length() * sizeof(QChar));
    Q_D(QOCIResult);
    r = OCIStmtPrepare(d->stmtp,
                       d->err,
                       txt,
                       len,
                       OCI_NTV_SYNTAX,
                       OCI_DEFAULT);
    if (r != OCI_SUCCESS) {
        qOraWarning("QOCIResult::prepare: unable to prepare statement:", d->err);
        setLastError(qMakeError(QCoreApplication::translate("QOCIResult",
                                "Unable to prepare statement"), QSqlError::StatementError, d->err));
        return false;
    }
    return true;
}

bool QOCIResult::exec()
{
    Q_D(QOCIResult);
    int r = 0;
    ub2 stmtType=0;
    ub4 iters;
    ub4 mode;
    TempStorage tmpStorage;
    IndicatorArray indicators(boundValueCount());
    SizeArray tmpSizes(boundValueCount());

    r = OCIAttrGet(d->stmtp,
                    OCI_HTYPE_STMT,
                    &stmtType,
                    NULL,
                    OCI_ATTR_STMT_TYPE,
                    d->err);

    if (r != OCI_SUCCESS && r != OCI_SUCCESS_WITH_INFO) {
        qOraWarning("QOCIResult::exec: Unable to get statement type:", d->err);
        setLastError(qMakeError(QCoreApplication::translate("QOCIResult",
                     "Unable to get statement type"), QSqlError::StatementError, d->err));
#ifdef QOCI_DEBUG
        qCDebug(lcOci) << "lastQuery()" << lastQuery();
#endif
        return false;
    }

    iters = stmtType == OCI_STMT_SELECT ? 0 : 1;
    mode = d->transaction ? OCI_DEFAULT : OCI_COMMIT_ON_SUCCESS;

    // bind placeholders
    if (boundValueCount() > 0
        && d->bindValues(boundValues(), indicators, tmpSizes, tmpStorage) != OCI_SUCCESS) {
        qOraWarning("QOCIResult::exec: unable to bind value: ", d->err);
        setLastError(qMakeError(QCoreApplication::translate("QOCIResult", "Unable to bind value"),
                    QSqlError::StatementError, d->err));
#ifdef QOCI_DEBUG
        qCDebug(lcOci) << "lastQuery()" << lastQuery();
#endif
        return false;
    }

    if (!isCursor) {
        // execute
        r = OCIStmtExecute(d->svc,
                           d->stmtp,
                           d->err,
                           iters,
                           0,
                           0,
                           0,
                           mode);
        if (r != OCI_SUCCESS && r != OCI_SUCCESS_WITH_INFO) {
            qOraWarning("QOCIResult::exec: unable to execute statement:", d->err);
            setLastError(qMakeError(QCoreApplication::translate("QOCIResult",
                        "Unable to execute statement"), QSqlError::StatementError, d->err));
    #ifdef QOCI_DEBUG
            qCDebug(lcOci) << "lastQuery()" << lastQuery();
    #endif
            return false;
        }
    }

    if (stmtType == OCI_STMT_SELECT) {
        ub4 parmCount = 0;
        int r = OCIAttrGet(d->stmtp, OCI_HTYPE_STMT, reinterpret_cast<void **>(&parmCount),
                           0, OCI_ATTR_PARAM_COUNT, d->err);
        if (r == 0 && !d->cols)
            d->cols = new QOCICols(parmCount, d);
        setSelect(true);
        QSqlCachedResult::init(parmCount);
    } else { /* non-SELECT */
        setSelect(false);
    }
    setAt(QSql::BeforeFirstRow);
    setActive(true);

    if (hasOutValues())
        d->outValues(boundValues(), indicators, tmpStorage);
    qDeleteAll(tmpStorage.dateTimes);
    return true;
}

QSqlRecord QOCIResult::record() const
{
    Q_D(const QOCIResult);
    QSqlRecord inf;
    if (!isActive() || !isSelect() || !d->cols)
        return inf;
    return d->cols->rec;
}

QVariant QOCIResult::lastInsertId() const
{
    Q_D(const QOCIResult);
    if (isActive()) {
        QOCIRowIdPointer ptr(new QOCIRowId(d->env));

        int r = OCIAttrGet(d->stmtp, OCI_HTYPE_STMT, ptr.constData()->id,
                           0, OCI_ATTR_ROWID, d->err);
        if (r == OCI_SUCCESS)
            return QVariant::fromValue(ptr);
    }
    return QVariant();
}

bool QOCIResult::execBatch(bool arrayBind)
{
    Q_D(QOCIResult);
    QOCICols::execBatch(d, boundValues(), arrayBind);
    resetBindCount();
    return lastError().type() == QSqlError::NoError;
}

void QOCIResult::virtual_hook(int id, void *data)
{
    Q_ASSERT(data);

    QSqlCachedResult::virtual_hook(id, data);
}

bool QOCIResult::fetchNext()
{
    Q_D(QOCIResult);
    if (isForwardOnly())
        d->cache.clear();
    return QSqlCachedResult::fetchNext();
}

////////////////////////////////////////////////////////////////////////////


QOCIDriver::QOCIDriver(QObject* parent)
    : QSqlDriver(*new QOCIDriverPrivate, parent)
{
    Q_D(QOCIDriver);
#ifdef QOCI_THREADED
    const ub4 mode = OCI_UTF16 | OCI_OBJECT | OCI_THREADED;
#else
    const ub4 mode = OCI_UTF16 | OCI_OBJECT;
#endif
    int r = OCIEnvCreate(&d->env,
                         mode,
                         NULL,
                         NULL,
                         NULL,
                         NULL,
                         0,
                         NULL);
    if (r != 0) {
        qCWarning(lcOci, "QOCIDriver: unable to create environment");
        setLastError(qMakeError(tr("Unable to initialize", "QOCIDriver"),
                     QSqlError::ConnectionError, d->err));
        return;
    }

    d->allocErrorHandle();
}

QOCIDriver::QOCIDriver(OCIEnv* env, OCISvcCtx* ctx, QObject* parent)
    : QSqlDriver(*new QOCIDriverPrivate, parent)
{
    Q_D(QOCIDriver);
    d->env = env;
    d->svc = ctx;

    d->allocErrorHandle();

    if (env && ctx) {
        setOpen(true);
        setOpenError(false);
    }
}

QOCIDriver::~QOCIDriver()
{
    Q_D(QOCIDriver);
    if (isOpen())
        close();
    int r = OCIHandleFree(d->err, OCI_HTYPE_ERROR);
    if (r != OCI_SUCCESS)
        qCWarning(lcOci, "Unable to free Error handle: %d", r);
    r = OCIHandleFree(d->env, OCI_HTYPE_ENV);
    if (r != OCI_SUCCESS)
        qCWarning(lcOci, "Unable to free Environment handle: %d", r);
}

bool QOCIDriver::hasFeature(DriverFeature f) const
{
    Q_D(const QOCIDriver);
    switch (f) {
    case Transactions:
    case LastInsertId:
    case BLOB:
    case PreparedQueries:
    case NamedPlaceholders:
    case BatchOperations:
    case LowPrecisionNumbers:
        return true;
    case QuerySize:
    case PositionalPlaceholders:
    case SimpleLocking:
    case EventNotifications:
    case FinishQuery:
    case CancelQuery:
    case MultipleResultSets:
        return false;
    case Unicode:
        return d->serverVersion >= 9;
    }
    return false;
}

static void qParseOpts(const QString &options, QOCIDriverPrivate *d)
{
    const QVector<QStringView> opts(QStringView(options).split(u';', Qt::SkipEmptyParts));
    for (const auto tmp : opts) {
        qsizetype idx;
        if ((idx = tmp.indexOf(u'=')) == -1) {
            qCWarning(lcOci, "QOCIDriver::parseArgs: Invalid parameter: '%ls'",
                      qUtf16Printable(tmp.toString()));
            continue;
        }
        const QStringView opt = tmp.left(idx);
        const QStringView val = tmp.mid(idx + 1).trimmed();
        bool ok;
        if (opt == "OCI_ATTR_PREFETCH_ROWS"_L1) {
            d->prefetchRows = val.toInt(&ok);
            if (!ok)
                d->prefetchRows = -1;
        } else if (opt == "OCI_ATTR_PREFETCH_MEMORY"_L1) {
            d->prefetchMem = val.toInt(&ok);
            if (!ok)
                d->prefetchMem = -1;
        } else if (opt == "OCI_AUTH_MODE"_L1) {
            if (val == "OCI_SYSDBA"_L1) {
                d->authMode = OCI_SYSDBA;
            } else if (val == "OCI_SYSOPER"_L1) {
                d->authMode = OCI_SYSOPER;
            } else if (val != "OCI_DEFAULT"_L1) {
                qCWarning(lcOci, "QOCIDriver::parseArgs: Unsupported value for OCI_AUTH_MODE: '%ls'",
                          qUtf16Printable(val.toString()));
            }
        } else {
            qCWarning(lcOci, "QOCIDriver::parseArgs: Invalid parameter: '%ls'",
                      qUtf16Printable(opt.toString()));
        }
    }
}

bool QOCIDriver::open(const QString & db,
                       const QString & user,
                       const QString & password,
                       const QString & hostname,
                       int port,
                       const QString &opts)
{
    Q_D(QOCIDriver);
    int r;

    if (isOpen())
        close();

    qParseOpts(opts, d);

    // Connect without tnsnames.ora if a hostname is given
    QString connectionString = db;
    if (!hostname.isEmpty())
        connectionString =
        QString::fromLatin1("(DESCRIPTION=(ADDRESS=(PROTOCOL=TCP)(Host=%1)(Port=%2))"
                "(CONNECT_DATA=(SID=%3)))").arg(hostname).arg((port > -1 ? port : 1521)).arg(db);

    Q_ASSERT(!d->srvhp);
    r = OCIHandleAlloc(d->env, reinterpret_cast<void **>(&d->srvhp), OCI_HTYPE_SERVER, 0, nullptr);
    if (r == OCI_SUCCESS) {
        r = OCIServerAttach(d->srvhp, d->err,
                            reinterpret_cast<const OraText *>(connectionString.utf16()),
                            sb4(connectionString.length() * sizeof(QChar)), OCI_DEFAULT);
    }
    Q_ASSERT(!d->svc);
    if (r == OCI_SUCCESS || r == OCI_SUCCESS_WITH_INFO) {
        r = OCIHandleAlloc(d->env, reinterpret_cast<void **>(&d->svc), OCI_HTYPE_SVCCTX,
                           0, nullptr);
    }
    if (r == OCI_SUCCESS)
        r = OCIAttrSet(d->svc, OCI_HTYPE_SVCCTX, d->srvhp, 0, OCI_ATTR_SERVER, d->err);
    Q_ASSERT(!d->authp);
    if (r == OCI_SUCCESS) {
        r = OCIHandleAlloc(d->env, reinterpret_cast<void **>(&d->authp), OCI_HTYPE_SESSION,
                           0, nullptr);
    }
    if (r == OCI_SUCCESS) {
        r = OCIAttrSet(d->authp, OCI_HTYPE_SESSION, const_cast<ushort *>(user.utf16()),
                       ub4(user.length() * sizeof(QChar)), OCI_ATTR_USERNAME, d->err);
    }
    if (r == OCI_SUCCESS) {
        r = OCIAttrSet(d->authp, OCI_HTYPE_SESSION, const_cast<ushort *>(password.utf16()),
                       ub4(password.length() * sizeof(QChar)), OCI_ATTR_PASSWORD, d->err);
    }
    Q_ASSERT(!d->trans);
    if (r == OCI_SUCCESS) {
        r = OCIHandleAlloc(d->env, reinterpret_cast<void **>(&d->trans), OCI_HTYPE_TRANS,
                           0, nullptr);
    }
    if (r == OCI_SUCCESS)
        r = OCIAttrSet(d->svc, OCI_HTYPE_SVCCTX, d->trans, 0, OCI_ATTR_TRANS, d->err);

    if (r == OCI_SUCCESS) {
        if (user.isEmpty() && password.isEmpty())
            r = OCISessionBegin(d->svc, d->err, d->authp, OCI_CRED_EXT, d->authMode);
        else
            r = OCISessionBegin(d->svc, d->err, d->authp, OCI_CRED_RDBMS, d->authMode);
    }
    if (r == OCI_SUCCESS || r == OCI_SUCCESS_WITH_INFO)
        r = OCIAttrSet(d->svc, OCI_HTYPE_SVCCTX, d->authp, 0, OCI_ATTR_SESSION, d->err);

    if (r != OCI_SUCCESS) {
        setLastError(qMakeError(tr("Unable to logon"), QSqlError::ConnectionError, d->err));
        setOpenError(true);
        if (d->trans)
            OCIHandleFree(d->trans, OCI_HTYPE_TRANS);
        d->trans = nullptr;
        if (d->authp)
            OCIHandleFree(d->authp, OCI_HTYPE_SESSION);
        d->authp = nullptr;
        if (d->svc)
            OCIHandleFree(d->svc, OCI_HTYPE_SVCCTX);
        d->svc = nullptr;
        if (d->srvhp)
            OCIHandleFree(d->srvhp, OCI_HTYPE_SERVER);
        d->srvhp = nullptr;
        return false;
    }

    // get server version
    char vertxt[512];
    r = OCIServerVersion(d->svc,
                          d->err,
                          reinterpret_cast<OraText *>(vertxt),
                          sizeof(vertxt),
                          OCI_HTYPE_SVCCTX);
    if (r != 0) {
        qCWarning(lcOci, "QOCIDriver::open: could not get Oracle server version.");
    } else {
        QString versionStr;
        versionStr = QString(reinterpret_cast<const QChar *>(vertxt));
#if QT_CONFIG(regularexpression)
        auto match = QRegularExpression("([0-9]+)\\.[0-9\\.]+[0-9]"_L1).match(versionStr);
        if (match.hasMatch())
            d->serverVersion = match.captured(1).toInt();
#endif
        if (d->serverVersion == 0)
            d->serverVersion = -1;
    }

    setOpen(true);
    setOpenError(false);
    d->user = user;

    return true;
}

void QOCIDriver::close()
{
    Q_D(QOCIDriver);
    if (!isOpen())
        return;

    OCISessionEnd(d->svc, d->err, d->authp, OCI_DEFAULT);
    OCIServerDetach(d->srvhp, d->err, OCI_DEFAULT);
    OCIHandleFree(d->trans, OCI_HTYPE_TRANS);
    d->trans = nullptr;
    OCIHandleFree(d->authp, OCI_HTYPE_SESSION);
    d->authp = nullptr;
    OCIHandleFree(d->svc, OCI_HTYPE_SVCCTX);
    d->svc = nullptr;
    OCIHandleFree(d->srvhp, OCI_HTYPE_SERVER);
    d->srvhp = nullptr;
    setOpen(false);
    setOpenError(false);
}

QSqlResult *QOCIDriver::createResult() const
{
    return new QOCIResult(this);
}

bool QOCIDriver::beginTransaction()
{
    Q_D(QOCIDriver);
    if (!isOpen()) {
        qCWarning(lcOci, "QOCIDriver::beginTransaction: Database not open");
        return false;
    }
    int r = OCITransStart(d->svc,
                          d->err,
                          2,
                          OCI_TRANS_READWRITE);
    if (r == OCI_ERROR) {
        qOraWarning("QOCIDriver::beginTransaction: ", d->err);
        setLastError(qMakeError(QCoreApplication::translate("QOCIDriver",
                                "Unable to begin transaction"), QSqlError::TransactionError, d->err));
        return false;
    }
    d->transaction = true;
    return true;
}

bool QOCIDriver::commitTransaction()
{
    Q_D(QOCIDriver);
    if (!isOpen()) {
        qCWarning(lcOci, "QOCIDriver::commitTransaction: Database not open");
        return false;
    }
    int r = OCITransCommit(d->svc,
                           d->err,
                           0);
    if (r == OCI_ERROR) {
        qOraWarning("QOCIDriver::commitTransaction:", d->err);
        setLastError(qMakeError(QCoreApplication::translate("QOCIDriver",
                                "Unable to commit transaction"), QSqlError::TransactionError, d->err));
        return false;
    }
    d->transaction = false;
    return true;
}

bool QOCIDriver::rollbackTransaction()
{
    Q_D(QOCIDriver);
    if (!isOpen()) {
        qCWarning(lcOci, "QOCIDriver::rollbackTransaction: Database not open");
        return false;
    }
    int r = OCITransRollback(d->svc,
                             d->err,
                             0);
    if (r == OCI_ERROR) {
        qOraWarning("QOCIDriver::rollbackTransaction:", d->err);
        setLastError(qMakeError(QCoreApplication::translate("QOCIDriver",
                                "Unable to rollback transaction"), QSqlError::TransactionError, d->err));
        return false;
    }
    d->transaction = false;
    return true;
}

enum Expression {
    OrExpression,
    AndExpression
};

static QString make_where_clause(const QString &user, Expression e)
{
    static const char sysUsers[][8] = {
        "MDSYS",
        "LBACSYS",
        "SYS",
        "SYSTEM",
        "WKSYS",
        "CTXSYS",
        "WMSYS",
    };
    static const char joinC[][4] = { "or" , "and" };
    static constexpr char16_t bang[] = { u' ', u'!' };

    const QLatin1StringView join(joinC[e]);

    QString result;
    result.reserve(sizeof sysUsers / sizeof *sysUsers *
                   // max-sizeof(owner != <sysuser> and )
                                (9 + sizeof *sysUsers + 5));
    for (const auto &sysUser : sysUsers) {
        const QLatin1StringView l1(sysUser);
        if (l1 != user)
            result += "owner "_L1 + bang[e] + "= '"_L1 + l1 + "' "_L1 + join + u' ';
    }

    result.chop(join.size() + 2); // remove final " <join> "

    return result;
}

QStringList QOCIDriver::tables(QSql::TableType type) const
{
    Q_D(const QOCIDriver);
    QStringList tl;

    QString user = d->user;
    if ( isIdentifierEscaped(user, QSqlDriver::TableName))
        user = stripDelimiters(user, QSqlDriver::TableName);
    else
        user = user.toUpper();

    if (!isOpen())
        return tl;

    QSqlQuery t(createResult());
    t.setForwardOnly(true);
    if (type & QSql::Tables) {
        const auto tableQuery = "select owner, table_name from all_tables where "_L1;
        const QString where = make_where_clause(user, AndExpression);
        t.exec(tableQuery + where);
        while (t.next()) {
            if (t.value(0).toString().toUpper() != user.toUpper())
                tl.append(t.value(0).toString() + u'.' + t.value(1).toString());
            else
                tl.append(t.value(1).toString());
        }

        // list all table synonyms as well
        const auto synonymQuery = "select owner, synonym_name from all_synonyms where "_L1;
        t.exec(synonymQuery + where);
        while (t.next()) {
            if (t.value(0).toString() != d->user)
                tl.append(t.value(0).toString() + u'.' + t.value(1).toString());
            else
                tl.append(t.value(1).toString());
        }
    }
    if (type & QSql::Views) {
        const auto query = "select owner, view_name from all_views where "_L1;
        const QString where = make_where_clause(user, AndExpression);
        t.exec(query + where);
        while (t.next()) {
            if (t.value(0).toString().toUpper() != d->user.toUpper())
                tl.append(t.value(0).toString() + u'.' + t.value(1).toString());
            else
                tl.append(t.value(1).toString());
        }
    }
    if (type & QSql::SystemTables) {
        t.exec("select table_name from dictionary"_L1);
        while (t.next()) {
            tl.append(t.value(0).toString());
        }
        const auto tableQuery = "select owner, table_name from all_tables where "_L1;
        const QString where = make_where_clause(user, OrExpression);
        t.exec(tableQuery + where);
        while (t.next()) {
            if (t.value(0).toString().toUpper() != user.toUpper())
                tl.append(t.value(0).toString() + u'.' + t.value(1).toString());
            else
                tl.append(t.value(1).toString());
        }

        // list all table synonyms as well
        const auto synonymQuery = "select owner, synonym_name from all_synonyms where "_L1;
        t.exec(synonymQuery + where);
        while (t.next()) {
            if (t.value(0).toString() != d->user)
                tl.append(t.value(0).toString() + u'.' + t.value(1).toString());
            else
                tl.append(t.value(1).toString());
        }
    }
    return tl;
}

void qSplitTableAndOwner(const QString & tname, QString * tbl,
                          QString * owner)
{
    qsizetype i = tname.indexOf(u'.'); // prefixed with owner?
    if (i != -1) {
        *tbl = tname.right(tname.length() - i - 1);
        *owner = tname.left(i);
    } else {
        *tbl = tname;
    }
}

QSqlRecord QOCIDriver::record(const QString& tablename) const
{
    Q_D(const QOCIDriver);
    QSqlRecord fil;
    if (!isOpen())
        return fil;

    QSqlQuery t(createResult());
    // using two separate queries for this is A LOT faster than using
    // eg. a sub-query on the sys.synonyms table
    QString stmt("select column_name, data_type, data_length, "
                  "data_precision, data_scale, nullable, data_default%1"
                  "from all_tab_columns a "_L1);
    if (d->serverVersion >= 9)
        stmt = stmt.arg(", char_length "_L1);
    else
        stmt = stmt.arg(" "_L1);
    bool buildRecordInfo = false;
    QString table, owner, tmpStmt;
    qSplitTableAndOwner(tablename, &table, &owner);

    if (isIdentifierEscaped(table, QSqlDriver::TableName))
        table = stripDelimiters(table, QSqlDriver::TableName);
    else
        table = table.toUpper();

    tmpStmt = stmt + "where a.table_name='"_L1 + table + u'\'';
    if (owner.isEmpty()) {
        owner = d->user;
    }

    if (isIdentifierEscaped(owner, QSqlDriver::TableName))
        owner = stripDelimiters(owner, QSqlDriver::TableName);
    else
        owner = owner.toUpper();

    tmpStmt += " and a.owner='"_L1 + owner + u'\'';
    t.setForwardOnly(true);
    t.exec(tmpStmt);
    if (!t.next()) { // try and see if the tablename is a synonym
        stmt = stmt + " join all_synonyms b on a.owner=b.table_owner and a.table_name=b.table_name "
                      "where b.owner='"_L1 + owner + "' and b.synonym_name='"_L1 + table + u'\'';
        t.setForwardOnly(true);
        t.exec(stmt);
        if (t.next())
            buildRecordInfo = true;
    } else {
        buildRecordInfo = true;
    }
    QStringList keywords = QStringList() << "NUMBER"_L1 << "FLOAT"_L1 << "BINARY_FLOAT"_L1
              << "BINARY_DOUBLE"_L1;
    if (buildRecordInfo) {
        do {
            QMetaType ty = qDecodeOCIType(t.value(1).toString(), t.numericalPrecisionPolicy());
            QSqlField f(t.value(0).toString(), ty);
            f.setRequired(t.value(5).toString() == "N"_L1);
            f.setPrecision(t.value(4).toInt());
            if (d->serverVersion >= 9 && (ty.id() == QMetaType::QString) && !t.isNull(3) && !keywords.contains(t.value(1).toString())) {
                // Oracle9: data_length == size in bytes, char_length == amount of characters
                f.setLength(t.value(7).toInt());
            } else {
                f.setLength(t.value(t.isNull(3) ? 2 : 3).toInt());
            }
            f.setDefaultValue(t.value(6));
            fil.append(f);
        } while (t.next());
    }
    return fil;
}

QSqlIndex QOCIDriver::primaryIndex(const QString& tablename) const
{
    Q_D(const QOCIDriver);
    QSqlIndex idx(tablename);
    if (!isOpen())
        return idx;
    QSqlQuery t(createResult());
    QString stmt("select b.column_name, b.index_name, a.table_name, a.owner "
                  "from all_constraints a, all_ind_columns b "
                  "where a.constraint_type='P' "
                  "and b.index_name = a.index_name "
                  "and b.index_owner = a.owner"_L1);

    bool buildIndex = false;
    QString table, owner, tmpStmt;
    qSplitTableAndOwner(tablename, &table, &owner);

    if (isIdentifierEscaped(table, QSqlDriver::TableName))
        table = stripDelimiters(table, QSqlDriver::TableName);
    else
        table = table.toUpper();

    tmpStmt = stmt + " and a.table_name='"_L1 + table + u'\'';
    if (owner.isEmpty()) {
        owner = d->user;
    }

    if (isIdentifierEscaped(owner, QSqlDriver::TableName))
        owner = stripDelimiters(owner, QSqlDriver::TableName);
    else
        owner = owner.toUpper();

    tmpStmt += " and a.owner='"_L1 + owner + u'\'';
    t.setForwardOnly(true);
    t.exec(tmpStmt);

    if (!t.next()) {
        stmt += " and a.table_name=(select tname from sys.synonyms where sname='"_L1
                + table + "' and creator=a.owner)"_L1;
        t.setForwardOnly(true);
        t.exec(stmt);
        if (t.next()) {
            owner = t.value(3).toString();
            buildIndex = true;
        }
    } else {
        buildIndex = true;
    }
    if (buildIndex) {
        QSqlQuery tt(createResult());
        tt.setForwardOnly(true);
        idx.setName(t.value(1).toString());
        do {
            tt.exec("select data_type from all_tab_columns where table_name='"_L1 +
                     t.value(2).toString() + "' and column_name='"_L1 +
                     t.value(0).toString() + "' and owner='"_L1 +
                     owner + u'\'');
            if (!tt.next()) {
                return QSqlIndex();
            }
            QSqlField f(t.value(0).toString(), qDecodeOCIType(tt.value(0).toString(), t.numericalPrecisionPolicy()));
            idx.append(f);
        } while (t.next());
        return idx;
    }
    return QSqlIndex();
}

QString QOCIDriver::formatValue(const QSqlField &field, bool trimStrings) const
{
    switch (field.metaType().id()) {
    case QMetaType::QDateTime: {
        QDateTime datetime = field.value().toDateTime();
        QString datestring;
        if (datetime.isValid()) {
            datestring = "TO_DATE('"_L1 + QString::number(datetime.date().year())
                         + u'-'
                         + QString::number(datetime.date().month()) + u'-'
                         + QString::number(datetime.date().day()) + u' '
                         + QString::number(datetime.time().hour()) + u':'
                         + QString::number(datetime.time().minute()) + u':'
                         + QString::number(datetime.time().second())
                         + "','YYYY-MM-DD HH24:MI:SS')"_L1;
        } else {
            datestring = "NULL"_L1;
        }
        return datestring;
    }
    case QMetaType::QTime: {
        QDateTime datetime = field.value().toDateTime();
        QString datestring;
        if (datetime.isValid()) {
            datestring = "TO_DATE('"_L1
                         + QString::number(datetime.time().hour()) + u':'
                         + QString::number(datetime.time().minute()) + u':'
                         + QString::number(datetime.time().second())
                         + "','HH24:MI:SS')"_L1;
        } else {
            datestring = "NULL"_L1;
        }
        return datestring;
    }
    case QMetaType::QDate: {
        QDate date = field.value().toDate();
        QString datestring;
        if (date.isValid()) {
            datestring = "TO_DATE('"_L1 + QString::number(date.year()) +
                         u'-' +
                         QString::number(date.month()) + u'-' +
                         QString::number(date.day()) + "','YYYY-MM-DD')"_L1;
        } else {
            datestring = "NULL"_L1;
        }
        return datestring;
    }
    default:
        break;
    }
    return QSqlDriver::formatValue(field, trimStrings);
}

QVariant QOCIDriver::handle() const
{
    Q_D(const QOCIDriver);
    return QVariant::fromValue(d->env);
}

QString QOCIDriver::escapeIdentifier(const QString &identifier, IdentifierType type) const
{
    QString res = identifier;
    if (!identifier.isEmpty() && !isIdentifierEscaped(identifier, type)) {
        res.replace(u'"', "\"\""_L1);
        res.replace(u'.', "\".\""_L1);
        res = u'"' + res + u'"';
    }
    return res;
}

int QOCIDriver::maximumIdentifierLength(IdentifierType type) const
{
    Q_D(const QOCIDriver);
    Q_UNUSED(type);
    return d->serverVersion > 12 ? 128 : 30;
}

QT_END_NAMESPACE

#include "moc_qsql_oci_p.cpp"
