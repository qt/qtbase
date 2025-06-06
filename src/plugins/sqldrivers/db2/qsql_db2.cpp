// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#include "qsql_db2_p.h"
#include <qcoreapplication.h>
#include <qdatetime.h>
#include <qlist.h>
#include <qsqlerror.h>
#include <qsqlfield.h>
#include <qsqlindex.h>
#include <qsqlrecord.h>
#include <qstringlist.h>
#include <qvarlengtharray.h>
#include <QDebug>
#include <QtSql/private/qsqldriver_p.h>
#include <QtSql/private/qsqlresult_p.h>
#include "private/qtools_p.h"

#if defined(Q_CC_BOR)
// DB2's sqlsystm.h (included through sqlcli1.h) defines the SQL_BIGINT_TYPE
// and SQL_BIGUINT_TYPE to wrong the types for Borland; so do the defines to
// the right type before including the header
#define SQL_BIGINT_TYPE qint64
#define SQL_BIGUINT_TYPE quint64
#endif

#include <sqlcli1.h>

#include <string.h>

QT_BEGIN_NAMESPACE

using namespace Qt::StringLiterals;

static const int COLNAMESIZE = 255;
// Based on what is mentioned in the documentation here:
// https://www.ibm.com/support/knowledgecenter/en/SSEPEK_10.0.0/sqlref/src/tpc/db2z_limits.html
// The limit is 128 bytes for table names
static const SQLSMALLINT TABLENAMESIZE = 128;
static const SQLSMALLINT qParamType[4] = { SQL_PARAM_INPUT, SQL_PARAM_INPUT, SQL_PARAM_OUTPUT, SQL_PARAM_INPUT_OUTPUT };

class QDB2DriverPrivate : public QSqlDriverPrivate
{
    Q_DECLARE_PUBLIC(QDB2Driver)

public:
    QDB2DriverPrivate() : QSqlDriverPrivate(QSqlDriver::DB2) {}
    SQLHANDLE hEnv = 0;
    SQLHANDLE hDbc = 0;
    QString user;

    void qSplitTableQualifier(const QString &qualifier, QString &catalog,
                              QString &schema, QString &table) const;
};

class QDB2ResultPrivate;

class QDB2Result: public QSqlResult
{
    Q_DECLARE_PRIVATE(QDB2Result)

public:
    QDB2Result(const QDB2Driver *drv);
    ~QDB2Result();
    bool prepare(const QString &query) override;
    bool exec() override;
    QVariant handle() const override;

protected:
    QVariant data(int field) override;
    bool reset(const QString &query) override;
    bool fetch(int i) override;
    bool fetchNext() override;
    bool fetchFirst() override;
    bool fetchLast() override;
    bool isNull(int i) override;
    int size() override;
    int numRowsAffected() override;
    QSqlRecord record() const override;
    void virtual_hook(int id, void *data) override;
    void detachFromResultSet() override;
    bool nextResult() override;
};

class QDB2ResultPrivate: public QSqlResultPrivate
{
    Q_DECLARE_PUBLIC(QDB2Result)

public:
    Q_DECLARE_SQLDRIVER_PRIVATE(QDB2Driver)
    QDB2ResultPrivate(QDB2Result *q, const QDB2Driver *drv)
        : QSqlResultPrivate(q, drv),
          hStmt(0)
    {}
    ~QDB2ResultPrivate()
    {
        emptyValueCache();
    }
    void clearValueCache()
    {
        for (int i = 0; i < valueCache.count(); ++i) {
            delete valueCache[i];
            valueCache[i] = NULL;
        }
    }
    void emptyValueCache()
    {
        clearValueCache();
        valueCache.clear();
    }

    SQLHANDLE hStmt;
    QSqlRecord recInf;
    QList<QVariant*> valueCache;
};

static QString qFromTChar(SQLTCHAR* str)
{
    return QString((const QChar *)str);
}

// dangerous!! (but fast). Don't use in functions that
// require out parameters!
static SQLTCHAR* qToTChar(const QString& str)
{
    return (SQLTCHAR*)str.utf16();
}

static QString qWarnDB2Handle(int handleType, SQLHANDLE handle, int *errorCode)
{
    SQLINTEGER nativeCode;
    SQLSMALLINT msgLen;
    SQLRETURN r = SQL_ERROR;
    SQLTCHAR state[SQL_SQLSTATE_SIZE + 1];
    SQLTCHAR description[SQL_MAX_MESSAGE_LENGTH];
    r = SQLGetDiagRec(handleType,
                       handle,
                       1,
                       (SQLTCHAR*) state,
                       &nativeCode,
                       (SQLTCHAR*) description,
                       SQL_MAX_MESSAGE_LENGTH - 1, /* in bytes, not in characters */
                       &msgLen);
    if (r == SQL_SUCCESS || r == SQL_SUCCESS_WITH_INFO) {
        if (errorCode)
            *errorCode = nativeCode;
        return QString(qFromTChar(description));
    }
    return QString();
}

static QString qDB2Warn(const QDB2DriverPrivate* d, QStringList *errorCodes = nullptr)
{
    int errorCode = 0;
    QString error = qWarnDB2Handle(SQL_HANDLE_ENV, d->hEnv, &errorCode);
    if (errorCodes && errorCode != 0) {
        *errorCodes << QString::number(errorCode);
        errorCode = 0;
    }
    if (!error.isEmpty())
        error += u' ';
    error += qWarnDB2Handle(SQL_HANDLE_DBC, d->hDbc, &errorCode);
    if (errorCodes && errorCode != 0)
        *errorCodes << QString::number(errorCode);
    return error;
}

static QString qDB2Warn(const QDB2ResultPrivate* d, QStringList *errorCodes = nullptr)
{
    int errorCode = 0;
    QString error = qDB2Warn(d->drv_d_func());
    if (!error.isEmpty())
        error += u' ';
    error += qWarnDB2Handle(SQL_HANDLE_STMT, d->hStmt, &errorCode);
    if (errorCodes && errorCode != 0)
        *errorCodes << QString::number(errorCode);
    return error;
}

template <typename T>
static void qSqlWarning(const QString &message, const T *d)
{
    qWarning("%s\tError: %s", message.toLocal8Bit().constData(),
                              qDB2Warn(d).toLocal8Bit().constData());
}

template <typename T>
static QSqlError qMakeError(const QString &err, QSqlError::ErrorType type,
                            const T *p)
{
    QStringList errorCodes;
    const QString error = qDB2Warn(p, &errorCodes);
    return QSqlError("QDB2: "_L1 + err, error, type, errorCodes.join(u';'));
}

static QMetaType qDecodeDB2Type(SQLSMALLINT sqltype)
{
    int type = QMetaType::UnknownType;
    switch (sqltype) {
    case SQL_REAL:
    case SQL_FLOAT:
    case SQL_DOUBLE:
    case SQL_DECIMAL:
    case SQL_NUMERIC:
        type = QMetaType::Double;
        break;
    case SQL_SMALLINT:
    case SQL_INTEGER:
    case SQL_BIT:
    case SQL_TINYINT:
        type = QMetaType::Int;
        break;
    case SQL_BIGINT:
        type = QMetaType::LongLong;
        break;
    case SQL_BLOB:
    case SQL_BINARY:
    case SQL_VARBINARY:
    case SQL_LONGVARBINARY:
    case SQL_CLOB:
    case SQL_DBCLOB:
        type = QMetaType::QByteArray;
        break;
    case SQL_DATE:
    case SQL_TYPE_DATE:
        type = QMetaType::QDate;
        break;
    case SQL_TIME:
    case SQL_TYPE_TIME:
        type = QMetaType::QTime;
        break;
    case SQL_TIMESTAMP:
    case SQL_TYPE_TIMESTAMP:
        type = QMetaType::QDateTime;
        break;
    case SQL_WCHAR:
    case SQL_WVARCHAR:
    case SQL_WLONGVARCHAR:
    case SQL_CHAR:
    case SQL_VARCHAR:
    case SQL_LONGVARCHAR:
        type = QMetaType::QString;
        break;
    default:
        type = QMetaType::QByteArray;
        break;
    }
    return QMetaType(type);
}

static QSqlField qMakeFieldInfo(const QDB2ResultPrivate* d, int i)
{
    SQLSMALLINT colNameLen;
    SQLSMALLINT colType;
    SQLULEN colSize;
    SQLSMALLINT colScale;
    SQLSMALLINT nullable;
    SQLRETURN r = SQL_ERROR;
    SQLTCHAR colName[COLNAMESIZE];
    r = SQLDescribeCol(d->hStmt,
                        i+1,
                        colName,
                        (SQLSMALLINT) COLNAMESIZE,
                        &colNameLen,
                        &colType,
                        &colSize,
                        &colScale,
                        &nullable);

    if (r != SQL_SUCCESS) {
        qSqlWarning(QString::fromLatin1("qMakeFieldInfo: Unable to describe column %1").arg(i), d);
        return QSqlField();
    }
    QSqlField f(qFromTChar(colName), qDecodeDB2Type(colType));
    // nullable can be SQL_NO_NULLS, SQL_NULLABLE or SQL_NULLABLE_UNKNOWN
    if (nullable == SQL_NO_NULLS)
        f.setRequired(true);
    else if (nullable == SQL_NULLABLE)
        f.setRequired(false);
    // else required is unknown
    f.setLength(colSize == 0 ? -1 : int(colSize));
    f.setPrecision(colScale == 0 ? -1 : int(colScale));
    SQLTCHAR tableName[TABLENAMESIZE];
    SQLSMALLINT tableNameLen;
    r = SQLColAttribute(d->hStmt, i + 1, SQL_DESC_BASE_TABLE_NAME, tableName,
                        TABLENAMESIZE, &tableNameLen, 0);
    if (r == SQL_SUCCESS)
        f.setTableName(qFromTChar(tableName));
    return f;
}

static int qGetIntData(SQLHANDLE hStmt, int column, bool& isNull)
{
    SQLINTEGER intbuf;
    isNull = false;
    SQLLEN lengthIndicator = 0;
    SQLRETURN r = SQLGetData(hStmt,
                              column + 1,
                              SQL_C_SLONG,
                              (SQLPOINTER) &intbuf,
                              0,
                              &lengthIndicator);
    if ((r != SQL_SUCCESS && r != SQL_SUCCESS_WITH_INFO) || lengthIndicator == SQL_NULL_DATA) {
        isNull = true;
        return 0;
    }
    return int(intbuf);
}

static double qGetDoubleData(SQLHANDLE hStmt, int column, bool& isNull)
{
    SQLDOUBLE dblbuf;
    isNull = false;
    SQLLEN lengthIndicator = 0;
    SQLRETURN r = SQLGetData(hStmt,
                              column+1,
                              SQL_C_DOUBLE,
                              (SQLPOINTER) &dblbuf,
                              0,
                              &lengthIndicator);
    if ((r != SQL_SUCCESS && r != SQL_SUCCESS_WITH_INFO) || lengthIndicator == SQL_NULL_DATA) {
        isNull = true;
        return 0.0;
    }

    return (double) dblbuf;
}

static SQLBIGINT qGetBigIntData(SQLHANDLE hStmt, int column, bool& isNull)
{
    SQLBIGINT lngbuf = Q_INT64_C(0);
    isNull = false;
    SQLLEN lengthIndicator = 0;
    SQLRETURN r = SQLGetData(hStmt,
                              column+1,
                              SQL_C_SBIGINT,
                              (SQLPOINTER) &lngbuf,
                              0,
                              &lengthIndicator);
    if ((r != SQL_SUCCESS && r != SQL_SUCCESS_WITH_INFO) || lengthIndicator == SQL_NULL_DATA)
        isNull = true;

    return lngbuf;
}

static QString qGetStringData(SQLHANDLE hStmt, int column, int colSize, bool& isNull)
{
    QString     fieldVal;
    SQLRETURN   r = SQL_ERROR;
    SQLLEN      lengthIndicator = 0;

    if (colSize <= 0)
        colSize = 255;
    else if (colSize > 65536) // limit buffer size to 64 KB
        colSize = 65536;
    else
        colSize++; // make sure there is room for more than the 0 termination
    SQLTCHAR* buf = new SQLTCHAR[colSize];

    while (true) {
        r = SQLGetData(hStmt,
                        column + 1,
                        SQL_C_WCHAR,
                        (SQLPOINTER)buf,
                        colSize * sizeof(SQLTCHAR),
                        &lengthIndicator);
        if (r == SQL_SUCCESS || r == SQL_SUCCESS_WITH_INFO) {
            if (lengthIndicator == SQL_NULL_DATA || lengthIndicator == SQL_NO_TOTAL) {
                fieldVal.clear();
                isNull = true;
                break;
            }
            fieldVal += qFromTChar(buf);
        } else if (r == SQL_NO_DATA) {
            break;
        } else {
            qWarning("qGetStringData: Error while fetching data (%d)", r);
            fieldVal.clear();
            break;
        }
    }
    delete[] buf;
    return fieldVal;
}

static QByteArray qGetBinaryData(SQLHANDLE hStmt, int column, SQLLEN& lengthIndicator, bool& isNull)
{
    QByteArray fieldVal;
    SQLSMALLINT colNameLen;
    SQLSMALLINT colType;
    SQLULEN colSize;
    SQLSMALLINT colScale;
    SQLSMALLINT nullable;
    SQLRETURN r = SQL_ERROR;

    SQLTCHAR colName[COLNAMESIZE];
    r = SQLDescribeCol(hStmt,
                        column+1,
                        colName,
                        COLNAMESIZE,
                        &colNameLen,
                        &colType,
                        &colSize,
                        &colScale,
                        &nullable);
    if (r != SQL_SUCCESS)
        qWarning("qGetBinaryData: Unable to describe column %d", column);
    // SQLDescribeCol may return 0 if size cannot be determined
    if (!colSize)
        colSize = 255;
    else if (colSize > 65536) // read the field in 64 KB chunks
        colSize = 65536;
    char * buf = new char[colSize];
    while (true) {
        r = SQLGetData(hStmt,
                        column+1,
                        colType == SQL_DBCLOB ? SQL_C_CHAR : SQL_C_BINARY,
                        (SQLPOINTER) buf,
                        colSize,
                        &lengthIndicator);
        if (r == SQL_SUCCESS || r == SQL_SUCCESS_WITH_INFO) {
            if (lengthIndicator == SQL_NULL_DATA) {
                isNull = true;
                break;
            } else {
                int rSize;
                r == SQL_SUCCESS ? rSize = lengthIndicator : rSize = colSize;
                if (lengthIndicator == SQL_NO_TOTAL) // size cannot be determined
                    rSize = colSize;
                fieldVal.append(QByteArray(buf, rSize));
                if (r == SQL_SUCCESS) // the whole field was read in one chunk
                    break;
            }
        } else {
            break;
        }
    }
    delete [] buf;
    return fieldVal;
}

void QDB2DriverPrivate::qSplitTableQualifier(const QString &qualifier, QString &catalog,
                                             QString &schema, QString &table) const
{
    const QList<QStringView> l = QStringView(qualifier).split(u'.');
    switch (l.count()) {
        case 1:
            table = qualifier;
            break;
        case 2:
            schema = l.at(0).toString();
            table = l.at(1).toString();
            break;
        case 3:
            catalog = l.at(0).toString();
            schema = l.at(1).toString();
            table = l.at(2).toString();
            break;
        default:
            qSqlWarning(QString::fromLatin1("QODBCDriver::splitTableQualifier: Unable to split table qualifier '%1'")
                        .arg(qualifier), this);
            break;
    }
}

// creates a QSqlField from a valid hStmt generated
// by SQLColumns. The hStmt has to point to a valid position.
static QSqlField qMakeFieldInfo(const SQLHANDLE hStmt)
{
    bool isNull;
    int type = qGetIntData(hStmt, 4, isNull);
    QSqlField f(qGetStringData(hStmt, 3, -1, isNull), qDecodeDB2Type(type));
    int required = qGetIntData(hStmt, 10, isNull); // nullable-flag
    // required can be SQL_NO_NULLS, SQL_NULLABLE or SQL_NULLABLE_UNKNOWN
    if (required == SQL_NO_NULLS)
        f.setRequired(true);
    else if (required == SQL_NULLABLE)
        f.setRequired(false);
    // else we don't know.
    f.setLength(qGetIntData(hStmt, 6, isNull)); // column size
    f.setPrecision(qGetIntData(hStmt, 8, isNull)); // precision
    return f;
}

static bool qMakeStatement(QDB2ResultPrivate* d, bool forwardOnly, bool setForwardOnly = true)
{
    SQLRETURN r;
    if (!d->hStmt) {
        r = SQLAllocHandle(SQL_HANDLE_STMT,
                            d->drv_d_func()->hDbc,
                            &d->hStmt);
        if (r != SQL_SUCCESS) {
            qSqlWarning("QDB2Result::reset: Unable to allocate statement handle"_L1, d);
            return false;
        }
    } else {
        r = SQLFreeStmt(d->hStmt, SQL_CLOSE);
        if (r != SQL_SUCCESS) {
            qSqlWarning("QDB2Result::reset: Unable to close statement handle"_L1, d);
            return false;
        }
    }

    if (!setForwardOnly)
        return true;

    if (forwardOnly) {
        r = SQLSetStmtAttr(d->hStmt,
                            SQL_ATTR_CURSOR_TYPE,
                            (SQLPOINTER) SQL_CURSOR_FORWARD_ONLY,
                            SQL_IS_UINTEGER);
    } else {
        r = SQLSetStmtAttr(d->hStmt,
                            SQL_ATTR_CURSOR_TYPE,
                            (SQLPOINTER) SQL_CURSOR_STATIC,
                            SQL_IS_UINTEGER);
    }
    if (r != SQL_SUCCESS && r != SQL_SUCCESS_WITH_INFO) {
        qSqlWarning(QString::fromLatin1("QDB2Result::reset: Unable to set %1 attribute.").arg(
                     forwardOnly ? "SQL_CURSOR_FORWARD_ONLY"_L1
                                 : "SQL_CURSOR_STATIC"_L1), d);
        return false;
    }
    return true;
}

QVariant QDB2Result::handle() const
{
    Q_D(const QDB2Result);
    return QVariant(QMetaType::fromType<SQLHANDLE>(), &d->hStmt);
}

/************************************/

QDB2Result::QDB2Result(const QDB2Driver *drv)
    : QSqlResult(*new QDB2ResultPrivate(this, drv))
{
}

QDB2Result::~QDB2Result()
{
    Q_D(const QDB2Result);
    if (d->hStmt) {
        SQLRETURN r = SQLFreeHandle(SQL_HANDLE_STMT, d->hStmt);
        if (r != SQL_SUCCESS)
            qSqlWarning("QDB2Driver: Unable to free statement handle "_L1 + QString::number(r), d);
    }
}

bool QDB2Result::reset (const QString& query)
{
    Q_D(QDB2Result);
    setActive(false);
    setAt(QSql::BeforeFirstRow);
    SQLRETURN r;

    d->recInf.clear();
    d->emptyValueCache();

    if (!qMakeStatement(d, isForwardOnly()))
        return false;

    r = SQLExecDirect(d->hStmt,
                       qToTChar(query),
                       (SQLINTEGER) query.length());
    if (r != SQL_SUCCESS && r != SQL_SUCCESS_WITH_INFO) {
        setLastError(qMakeError(QCoreApplication::translate("QDB2Result",
                                "Unable to execute statement"), QSqlError::StatementError, d));
        return false;
    }
    SQLSMALLINT count = 0;
    r = SQLNumResultCols(d->hStmt, &count);
    if (count) {
        setSelect(true);
        for (int i = 0; i < count; ++i) {
            d->recInf.append(qMakeFieldInfo(d, i));
        }
    } else {
        setSelect(false);
    }
    d->valueCache.resize(count);
    d->valueCache.fill(NULL);
    setActive(true);
    return true;
}

bool QDB2Result::prepare(const QString& query)
{
    Q_D(QDB2Result);
    setActive(false);
    setAt(QSql::BeforeFirstRow);
    SQLRETURN r;

    d->recInf.clear();
    d->emptyValueCache();

    if (!qMakeStatement(d, isForwardOnly()))
        return false;

    r = SQLPrepare(d->hStmt,
                    qToTChar(query),
                    (SQLINTEGER) query.length());

    if (r != SQL_SUCCESS) {
        setLastError(qMakeError(QCoreApplication::translate("QDB2Result",
                     "Unable to prepare statement"), QSqlError::StatementError, d));
        return false;
    }
    return true;
}

bool QDB2Result::exec()
{
    Q_D(QDB2Result);
    QList<QByteArray> tmpStorage; // holds temporary ptrs
    QVarLengthArray<SQLLEN, 32> indicators(boundValues().count());

    memset(indicators.data(), 0, indicators.size() * sizeof(SQLLEN));
    setActive(false);
    setAt(QSql::BeforeFirstRow);
    SQLRETURN r;

    d->recInf.clear();
    d->emptyValueCache();

    if (!qMakeStatement(d, isForwardOnly(), false))
        return false;


    QList<QVariant> &values = boundValues();
    int i;
    for (i = 0; i < values.count(); ++i) {
        // bind parameters - only positional binding allowed
        SQLLEN *ind = &indicators[i];
        if (QSqlResultPrivate::isVariantNull(values.at(i)))
            *ind = SQL_NULL_DATA;
        if (bindValueType(i) & QSql::Out)
            values[i].detach();

        switch (values.at(i).metaType().id()) {
            case QMetaType::QDate: {
                QByteArray ba;
                ba.resize(sizeof(DATE_STRUCT));
                DATE_STRUCT *dt = (DATE_STRUCT *)ba.constData();
                QDate qdt = values.at(i).toDate();
                dt->year = qdt.year();
                dt->month = qdt.month();
                dt->day = qdt.day();
                r = SQLBindParameter(d->hStmt,
                                     i + 1,
                                     qParamType[bindValueType(i) & 3],
                                     SQL_C_DATE,
                                     SQL_DATE,
                                     0,
                                     0,
                                     (void *) dt,
                                     0,
                                     *ind == SQL_NULL_DATA ? ind : NULL);
                tmpStorage.append(ba);
                break; }
            case QMetaType::QTime: {
                QByteArray ba;
                ba.resize(sizeof(TIME_STRUCT));
                TIME_STRUCT *dt = (TIME_STRUCT *)ba.constData();
                QTime qdt = values.at(i).toTime();
                dt->hour = qdt.hour();
                dt->minute = qdt.minute();
                dt->second = qdt.second();
                r = SQLBindParameter(d->hStmt,
                                      i + 1,
                                      qParamType[bindValueType(i) & 3],
                                      SQL_C_TIME,
                                      SQL_TIME,
                                      0,
                                      0,
                                      (void *) dt,
                                      0,
                                      *ind == SQL_NULL_DATA ? ind : NULL);
                tmpStorage.append(ba);
                break; }
            case QMetaType::QDateTime: {
                QByteArray ba;
                ba.resize(sizeof(TIMESTAMP_STRUCT));
                TIMESTAMP_STRUCT * dt = (TIMESTAMP_STRUCT *)ba.constData();
                QDateTime qdt = values.at(i).toDateTime();
                dt->year = qdt.date().year();
                dt->month = qdt.date().month();
                dt->day = qdt.date().day();
                dt->hour = qdt.time().hour();
                dt->minute = qdt.time().minute();
                dt->second = qdt.time().second();
                dt->fraction = qdt.time().msec() * 1000000;
                r = SQLBindParameter(d->hStmt,
                                      i + 1,
                                      qParamType[bindValueType(i) & 3],
                                      SQL_C_TIMESTAMP,
                                      SQL_TIMESTAMP,
                                      0,
                                      0,
                                      (void *) dt,
                                      0,
                                      *ind == SQL_NULL_DATA ? ind : NULL);
                tmpStorage.append(ba);
                break; }
            case QMetaType::Int:
                r = SQLBindParameter(d->hStmt,
                                      i + 1,
                                      qParamType[bindValueType(i) & 3],
                                      SQL_C_SLONG,
                                      SQL_INTEGER,
                                      0,
                                      0,
                                      (void *)values.at(i).constData(),
                                      0,
                                      *ind == SQL_NULL_DATA ? ind : NULL);
                break;
            case QMetaType::Double:
                r = SQLBindParameter(d->hStmt,
                                      i + 1,
                                      qParamType[bindValueType(i) & 3],
                                      SQL_C_DOUBLE,
                                      SQL_DOUBLE,
                                      0,
                                      0,
                                      (void *)values.at(i).constData(),
                                      0,
                                      *ind == SQL_NULL_DATA ? ind : NULL);
                break;
            case QMetaType::QByteArray: {
                int len = values.at(i).toByteArray().size();
                if (*ind != SQL_NULL_DATA)
                    *ind = len;
                r = SQLBindParameter(d->hStmt,
                                      i + 1,
                                      qParamType[bindValueType(i) & 3],
                                      SQL_C_BINARY,
                                      SQL_LONGVARBINARY,
                                      len,
                                      0,
                                      (void *)values.at(i).toByteArray().constData(),
                                      len,
                                      ind);
                break; }
            case QMetaType::QString:
            {
                const QString str(values.at(i).toString());
                if (*ind != SQL_NULL_DATA)
                    *ind = str.length() * sizeof(QChar);
                if (bindValueType(i) & QSql::Out) {
                    QByteArray ba((char *)str.data(), str.capacity() * sizeof(QChar));
                    r = SQLBindParameter(d->hStmt,
                                        i + 1,
                                        qParamType[bindValueType(i) & 3],
                                        SQL_C_WCHAR,
                                        SQL_WVARCHAR,
                                        str.length(),
                                        0,
                                        (void *)ba.constData(),
                                        ba.size(),
                                        ind);
                    tmpStorage.append(ba);
                } else {
                    void *data = (void*)str.utf16();
                    int len = str.length();
                    r = SQLBindParameter(d->hStmt,
                                        i + 1,
                                        qParamType[bindValueType(i) & 3],
                                        SQL_C_WCHAR,
                                        SQL_WVARCHAR,
                                        len,
                                        0,
                                        data,
                                        len * sizeof(QChar),
                                        ind);
                }
                break;
            }
            default: {
                QByteArray ba = values.at(i).toString().toLatin1();
                int len = ba.length() + 1;
                if (*ind != SQL_NULL_DATA)
                    *ind = ba.length();
                r = SQLBindParameter(d->hStmt,
                                      i + 1,
                                      qParamType[bindValueType(i) & 3],
                                      SQL_C_CHAR,
                                      SQL_VARCHAR,
                                      len,
                                      0,
                                      (void *) ba.constData(),
                                      len,
                                      ind);
                tmpStorage.append(ba);
                break; }
        }
        if (r != SQL_SUCCESS) {
            qWarning("QDB2Result::exec: unable to bind variable: %s",
                     qDB2Warn(d).toLocal8Bit().constData());
            setLastError(qMakeError(QCoreApplication::translate("QDB2Result",
                                    "Unable to bind variable"), QSqlError::StatementError, d));
            return false;
        }
    }

    r = SQLExecute(d->hStmt);
    if (r != SQL_SUCCESS && r != SQL_SUCCESS_WITH_INFO) {
        qWarning("QDB2Result::exec: Unable to execute statement: %s",
                 qDB2Warn(d).toLocal8Bit().constData());
        setLastError(qMakeError(QCoreApplication::translate("QDB2Result",
                                "Unable to execute statement"), QSqlError::StatementError, d));
        return false;
    }
    SQLSMALLINT count = 0;
    r = SQLNumResultCols(d->hStmt, &count);
    if (count) {
        setSelect(true);
        for (int i = 0; i < count; ++i) {
            d->recInf.append(qMakeFieldInfo(d, i));
        }
    } else {
        setSelect(false);
    }
    setActive(true);
    d->valueCache.resize(count);
    d->valueCache.fill(NULL);

    //get out parameters
    if (!hasOutValues())
        return true;

    for (i = 0; i < values.count(); ++i) {
        switch (values[i].metaType().id()) {
            case QMetaType::QDate: {
                DATE_STRUCT ds = *((DATE_STRUCT *)tmpStorage.takeFirst().constData());
                values[i] = QVariant(QDate(ds.year, ds.month, ds.day));
                break; }
            case QMetaType::QTime: {
                TIME_STRUCT dt = *((TIME_STRUCT *)tmpStorage.takeFirst().constData());
                values[i] = QVariant(QTime(dt.hour, dt.minute, dt.second));
                break; }
            case QMetaType::QDateTime: {
                TIMESTAMP_STRUCT dt = *((TIMESTAMP_STRUCT *)tmpStorage.takeFirst().constData());
                values[i] = QVariant(QDateTime(QDate(dt.year, dt.month, dt.day),
                              QTime(dt.hour, dt.minute, dt.second, dt.fraction / 1000000)));
                break; }
            case QMetaType::Int:
            case QMetaType::Double:
            case QMetaType::QByteArray:
                break;
            case QMetaType::QString:
                if (bindValueType(i) & QSql::Out)
                    values[i] = QString((const QChar *)tmpStorage.takeFirst().constData());
                break;
            default: {
                values[i] = QString::fromLatin1(tmpStorage.takeFirst().constData());
                break; }
        }
        if (indicators[i] == SQL_NULL_DATA)
            values[i] = QVariant(values[i].metaType());
    }
    return true;
}

bool QDB2Result::fetch(int i)
{
    Q_D(QDB2Result);
    if (isForwardOnly() && i < at())
        return false;
    if (i == at())
        return true;
    d->clearValueCache();
    int actualIdx = i + 1;
    if (actualIdx <= 0) {
        setAt(QSql::BeforeFirstRow);
        return false;
    }
    SQLRETURN r;
    if (isForwardOnly()) {
        bool ok = true;
        while (ok && i > at())
            ok = fetchNext();
        return ok;
    } else {
        r = SQLFetchScroll(d->hStmt,
                            SQL_FETCH_ABSOLUTE,
                            actualIdx);
    }
    if (r != SQL_SUCCESS && r != SQL_SUCCESS_WITH_INFO && r != SQL_NO_DATA) {
        setLastError(qMakeError(QCoreApplication::translate("QDB2Result",
                                "Unable to fetch record %1").arg(i), QSqlError::StatementError, d));
        return false;
    }
    else if (r == SQL_NO_DATA)
        return false;
    setAt(i);
    return true;
}

bool QDB2Result::fetchNext()
{
    Q_D(QDB2Result);
    SQLRETURN r;
    d->clearValueCache();
    r = SQLFetchScroll(d->hStmt,
                       SQL_FETCH_NEXT,
                       0);
    if (r != SQL_SUCCESS && r != SQL_SUCCESS_WITH_INFO) {
        if (r != SQL_NO_DATA)
            setLastError(qMakeError(QCoreApplication::translate("QDB2Result",
                                    "Unable to fetch next"), QSqlError::StatementError, d));
        return false;
    }
    setAt(at() + 1);
    return true;
}

bool QDB2Result::fetchFirst()
{
    Q_D(QDB2Result);
    if (isForwardOnly() && at() != QSql::BeforeFirstRow)
        return false;
    if (isForwardOnly())
        return fetchNext();
    d->clearValueCache();
    SQLRETURN r;
    r = SQLFetchScroll(d->hStmt,
                       SQL_FETCH_FIRST,
                       0);
    if (r != SQL_SUCCESS && r != SQL_SUCCESS_WITH_INFO) {
        if (r!= SQL_NO_DATA)
            setLastError(qMakeError(QCoreApplication::translate("QDB2Result", "Unable to fetch first"),
                                    QSqlError::StatementError, d));
        return false;
    }
    setAt(0);
    return true;
}

bool QDB2Result::fetchLast()
{
    Q_D(QDB2Result);
    d->clearValueCache();

    int i = at();
    if (i == QSql::AfterLastRow) {
        if (isForwardOnly()) {
            return false;
        } else {
            if (!fetch(0))
                return false;
            i = at();
        }
    }

    while (fetchNext())
        ++i;

    if (i == QSql::BeforeFirstRow) {
        setAt(QSql::AfterLastRow);
        return false;
    }

    if (!isForwardOnly())
        return fetch(i);

    setAt(i);
    return true;
}


QVariant QDB2Result::data(int field)
{
    Q_D(QDB2Result);
    if (field >= d->recInf.count()) {
        qWarning("QDB2Result::data: column %d out of range", field);
        return QVariant();
    }
    SQLRETURN r = 0;
    SQLLEN lengthIndicator = 0;
    bool isNull = false;
    const QSqlField info = d->recInf.field(field);

    if (!info.isValid() || field >= d->valueCache.size())
        return QVariant();

    if (d->valueCache[field])
        return *d->valueCache[field];


    QVariant *v = nullptr;
    switch (info.metaType().id()) {
        case QMetaType::LongLong:
            v = new QVariant((qint64) qGetBigIntData(d->hStmt, field, isNull));
            break;
        case QMetaType::Int:
            v = new QVariant(qGetIntData(d->hStmt, field, isNull));
            break;
        case QMetaType::QDate: {
            DATE_STRUCT dbuf;
            r = SQLGetData(d->hStmt,
                            field + 1,
                            SQL_C_DATE,
                            (SQLPOINTER) &dbuf,
                            0,
                            &lengthIndicator);
            if ((r == SQL_SUCCESS || r == SQL_SUCCESS_WITH_INFO) && (lengthIndicator != SQL_NULL_DATA)) {
                v = new QVariant(QDate(dbuf.year, dbuf.month, dbuf.day));
            } else {
                v = new QVariant(QDate());
                isNull = true;
            }
            break; }
        case QMetaType::QTime: {
            TIME_STRUCT tbuf;
            r = SQLGetData(d->hStmt,
                            field + 1,
                            SQL_C_TIME,
                            (SQLPOINTER) &tbuf,
                            0,
                            &lengthIndicator);
            if ((r == SQL_SUCCESS || r == SQL_SUCCESS_WITH_INFO) && (lengthIndicator != SQL_NULL_DATA)) {
                v = new QVariant(QTime(tbuf.hour, tbuf.minute, tbuf.second));
            } else {
                v = new QVariant(QTime());
                isNull = true;
            }
            break; }
        case QMetaType::QDateTime: {
            TIMESTAMP_STRUCT dtbuf;
            r = SQLGetData(d->hStmt,
                            field + 1,
                            SQL_C_TIMESTAMP,
                            (SQLPOINTER) &dtbuf,
                            0,
                            &lengthIndicator);
            if ((r == SQL_SUCCESS || r == SQL_SUCCESS_WITH_INFO) && (lengthIndicator != SQL_NULL_DATA)) {
                v = new QVariant(QDateTime(QDate(dtbuf.year, dtbuf.month, dtbuf.day),
                                             QTime(dtbuf.hour, dtbuf.minute, dtbuf.second, dtbuf.fraction / 1000000)));
            } else {
                v = new QVariant(QDateTime());
                isNull = true;
            }
            break; }
        case QMetaType::QByteArray:
            v = new QVariant(qGetBinaryData(d->hStmt, field, lengthIndicator, isNull));
            break;
        case QMetaType::Double:
            {
            switch(numericalPrecisionPolicy()) {
                case QSql::LowPrecisionInt32:
                    v = new QVariant(qGetIntData(d->hStmt, field, isNull));
                    break;
                case QSql::LowPrecisionInt64:
                    v = new QVariant((qint64) qGetBigIntData(d->hStmt, field, isNull));
                    break;
                case QSql::LowPrecisionDouble:
                    v = new QVariant(qGetDoubleData(d->hStmt, field, isNull));
                    break;
                case QSql::HighPrecision:
                default:
                    // length + 1 for the comma
                    v = new QVariant(qGetStringData(d->hStmt, field, info.length() + 1, isNull));
                    break;
            }
            break;
            }
        case QMetaType::QString:
        default:
            v = new QVariant(qGetStringData(d->hStmt, field, info.length(), isNull));
            break;
    }
    if (isNull)
        *v = QVariant(info.metaType());
    d->valueCache[field] = v;
    return *v;
}

bool QDB2Result::isNull(int i)
{
    Q_D(const QDB2Result);
    if (i >= d->valueCache.size())
        return true;

    if (d->valueCache[i])
        return d->valueCache[i]->isNull();
    return data(i).isNull();
}

int QDB2Result::numRowsAffected()
{
    Q_D(const QDB2Result);
    SQLLEN affectedRowCount = 0;
    SQLRETURN r = SQLRowCount(d->hStmt, &affectedRowCount);
    if (r == SQL_SUCCESS || r == SQL_SUCCESS_WITH_INFO)
        return affectedRowCount;
    else
        qSqlWarning("QDB2Result::numRowsAffected: Unable to count affected rows"_L1, d);
    return -1;
}

int QDB2Result::size()
{
    return -1;
}

QSqlRecord QDB2Result::record() const
{
    Q_D(const QDB2Result);
    if (isActive())
        return d->recInf;
    return QSqlRecord();
}

bool QDB2Result::nextResult()
{
    Q_D(QDB2Result);
    setActive(false);
    setAt(QSql::BeforeFirstRow);
    d->recInf.clear();
    d->emptyValueCache();
    setSelect(false);

    SQLRETURN r = SQLMoreResults(d->hStmt);
    if (r != SQL_SUCCESS && r != SQL_SUCCESS_WITH_INFO) {
        if (r != SQL_NO_DATA) {
            setLastError(qMakeError(QCoreApplication::translate("QODBCResult",
                "Unable to fetch last"), QSqlError::ConnectionError, d));
        }
        return false;
    }

    SQLSMALLINT fieldCount = 0;
    r = SQLNumResultCols(d->hStmt, &fieldCount);
    setSelect(fieldCount > 0);
    for (int i = 0; i < fieldCount; ++i)
        d->recInf.append(qMakeFieldInfo(d, i));

    d->valueCache.resize(fieldCount);
    d->valueCache.fill(NULL);
    setActive(true);

    return true;
}

void QDB2Result::virtual_hook(int id, void *data)
{
    QSqlResult::virtual_hook(id, data);
}

void QDB2Result::detachFromResultSet()
{
    Q_D(QDB2Result);
    if (d->hStmt)
        SQLCloseCursor(d->hStmt);
}

/************************************/

QDB2Driver::QDB2Driver(QObject* parent)
    : QSqlDriver(*new QDB2DriverPrivate, parent)
{
}

QDB2Driver::QDB2Driver(Qt::HANDLE env, Qt::HANDLE con, QObject* parent)
    : QSqlDriver(*new QDB2DriverPrivate, parent)
{
    Q_D(QDB2Driver);
    d->hEnv = reinterpret_cast<SQLHANDLE>(env);
    d->hDbc = reinterpret_cast<SQLHANDLE>(con);
    if (env && con) {
        setOpen(true);
        setOpenError(false);
    }
}

QDB2Driver::~QDB2Driver()
{
    close();
}

bool QDB2Driver::open(const QString& db, const QString& user, const QString& password, const QString& host, int port,
                       const QString& connOpts)
{
    Q_D(QDB2Driver);
    if (isOpen())
      close();
    SQLRETURN r;
    r = SQLAllocHandle(SQL_HANDLE_ENV,
                        SQL_NULL_HANDLE,
                        &d->hEnv);
    if (r != SQL_SUCCESS && r != SQL_SUCCESS_WITH_INFO) {
        qSqlWarning("QDB2Driver::open: Unable to allocate environment"_L1, d);
        setOpenError(true);
        return false;
    }

    r = SQLAllocHandle(SQL_HANDLE_DBC,
                        d->hEnv,
                        &d->hDbc);
    if (r != SQL_SUCCESS && r != SQL_SUCCESS_WITH_INFO) {
        qSqlWarning("QDB2Driver::open: Unable to allocate connection"_L1, d);
        setOpenError(true);
        return false;
    }

    QString protocol;
    // Set connection attributes
    const QStringList opts(connOpts.split(u';', Qt::SkipEmptyParts));
    for (int i = 0; i < opts.count(); ++i) {
        const QString tmp(opts.at(i));
        int idx;
        if ((idx = tmp.indexOf(u'=')) == -1) {
            qWarning("QDB2Driver::open: Illegal connect option value '%s'",
                     tmp.toLocal8Bit().constData());
            continue;
        }

        const QString opt(tmp.left(idx));
        const QString val(tmp.mid(idx + 1).simplified());

        SQLULEN v = 0;
        r = SQL_SUCCESS;
        if (opt == "SQL_ATTR_ACCESS_MODE"_L1) {
            if (val == "SQL_MODE_READ_ONLY"_L1) {
                v = SQL_MODE_READ_ONLY;
            } else if (val == "SQL_MODE_READ_WRITE"_L1) {
                v = SQL_MODE_READ_WRITE;
            } else {
                qWarning("QDB2Driver::open: Unknown option value '%s'",
                         tmp.toLocal8Bit().constData());
                continue;
            }
            r = SQLSetConnectAttr(d->hDbc, SQL_ATTR_ACCESS_MODE, reinterpret_cast<SQLPOINTER>(v), 0);
        } else if (opt == "SQL_ATTR_LOGIN_TIMEOUT"_L1) {
            v = val.toUInt();
            r = SQLSetConnectAttr(d->hDbc, SQL_ATTR_LOGIN_TIMEOUT, reinterpret_cast<SQLPOINTER>(v), 0);
        } else if (opt.compare("PROTOCOL"_L1, Qt::CaseInsensitive) == 0) {
                        protocol = tmp;
        }
        else {
            qWarning("QDB2Driver::open: Unknown connection attribute '%s'",
                      tmp.toLocal8Bit().constData());
        }
        if (r != SQL_SUCCESS && r != SQL_SUCCESS_WITH_INFO)
            qSqlWarning(QString::fromLatin1("QDB2Driver::open: "
                           "Unable to set connection attribute '%1'").arg(opt), d);
    }

    if (protocol.isEmpty())
        protocol = "PROTOCOL=TCPIP"_L1;

    if (port < 0 )
        port = 50000;

    QString connQStr;
    connQStr =  protocol + ";DATABASE="_L1 + db + ";HOSTNAME="_L1 + host
        + ";PORT="_L1 + QString::number(port) + ";UID="_L1 + user
        + ";PWD="_L1 + password;


    SQLTCHAR connOut[SQL_MAX_OPTION_STRING_LENGTH];
    SQLSMALLINT cb;

    r = SQLDriverConnect(d->hDbc,
                          NULL,
                          qToTChar(connQStr),
                          (SQLSMALLINT) connQStr.length(),
                          connOut,
                          SQL_MAX_OPTION_STRING_LENGTH,
                          &cb,
                          SQL_DRIVER_NOPROMPT);
    if (r != SQL_SUCCESS && r != SQL_SUCCESS_WITH_INFO) {
        setLastError(qMakeError(tr("Unable to connect"),
                                QSqlError::ConnectionError, d));
        setOpenError(true);
        return false;
    }

    d->user = user;
    setOpen(true);
    setOpenError(false);
    return true;
}

void QDB2Driver::close()
{
    Q_D(QDB2Driver);
    SQLRETURN r;
    if (d->hDbc) {
        // Open statements/descriptors handles are automatically cleaned up by SQLDisconnect
        if (isOpen()) {
            r = SQLDisconnect(d->hDbc);
            if (r != SQL_SUCCESS)
                qSqlWarning("QDB2Driver::close: Unable to disconnect datasource"_L1, d);
        }
        r = SQLFreeHandle(SQL_HANDLE_DBC, d->hDbc);
        if (r != SQL_SUCCESS)
            qSqlWarning("QDB2Driver::close: Unable to free connection handle"_L1, d);
        d->hDbc = 0;
    }

    if (d->hEnv) {
        r = SQLFreeHandle(SQL_HANDLE_ENV, d->hEnv);
        if (r != SQL_SUCCESS)
            qSqlWarning("QDB2Driver::close: Unable to free environment handle"_L1, d);
        d->hEnv = 0;
    }
    setOpen(false);
    setOpenError(false);
}

QSqlResult *QDB2Driver::createResult() const
{
    return new QDB2Result(this);
}

QSqlRecord QDB2Driver::record(const QString& tableName) const
{
    Q_D(const QDB2Driver);
    QSqlRecord fil;
    if (!isOpen())
        return fil;

    SQLHANDLE hStmt;
    QString catalog, schema, table;
    d->qSplitTableQualifier(tableName, catalog, schema, table);
    if (schema.isEmpty())
        schema = d->user;

    if (isIdentifierEscaped(catalog, QSqlDriver::TableName))
        catalog = stripDelimiters(catalog, QSqlDriver::TableName);
    else
        catalog = catalog.toUpper();

    if (isIdentifierEscaped(schema, QSqlDriver::TableName))
        schema = stripDelimiters(schema, QSqlDriver::TableName);
    else
        schema = schema.toUpper();

    if (isIdentifierEscaped(table, QSqlDriver::TableName))
        table = stripDelimiters(table, QSqlDriver::TableName);
    else
        table = table.toUpper();

    SQLRETURN r = SQLAllocHandle(SQL_HANDLE_STMT,
                                  d->hDbc,
                                  &hStmt);
    if (r != SQL_SUCCESS) {
        qSqlWarning("QDB2Driver::record: Unable to allocate handle"_L1, d);
        return fil;
    }

    r = SQLSetStmtAttr(hStmt,
                        SQL_ATTR_CURSOR_TYPE,
                        (SQLPOINTER) SQL_CURSOR_FORWARD_ONLY,
                        SQL_IS_UINTEGER);


    //Aside: szSchemaName and szTableName parameters of SQLColumns
    //are case sensitive search patterns, so no escaping is used.
    r =  SQLColumns(hStmt,
                     NULL,
                     0,
                     qToTChar(schema),
                     schema.length(),
                     qToTChar(table),
                     table.length(),
                     NULL,
                     0);

    if (r != SQL_SUCCESS)
        qSqlWarning("QDB2Driver::record: Unable to execute column list"_L1, d);
    r = SQLFetchScroll(hStmt,
                        SQL_FETCH_NEXT,
                        0);
    while (r == SQL_SUCCESS) {
        QSqlField fld = qMakeFieldInfo(hStmt);
        fld.setTableName(tableName);
        fil.append(fld);
        r = SQLFetchScroll(hStmt,
                            SQL_FETCH_NEXT,
                            0);
    }

    r = SQLFreeHandle(SQL_HANDLE_STMT, hStmt);
    if (r != SQL_SUCCESS)
        qSqlWarning("QDB2Driver: Unable to free statement handle "_L1
                    + QString::number(r), d);

    return fil;
}

QStringList QDB2Driver::tables(QSql::TableType type) const
{
    Q_D(const QDB2Driver);
    QStringList tl;
    if (!isOpen())
        return tl;

    SQLHANDLE hStmt;

    SQLRETURN r = SQLAllocHandle(SQL_HANDLE_STMT,
                                  d->hDbc,
                                  &hStmt);
    if (r != SQL_SUCCESS && r != SQL_SUCCESS_WITH_INFO) {
        qSqlWarning("QDB2Driver::tables: Unable to allocate handle"_L1, d);
        return tl;
    }
    r = SQLSetStmtAttr(hStmt,
                        SQL_ATTR_CURSOR_TYPE,
                        (SQLPOINTER)SQL_CURSOR_FORWARD_ONLY,
                        SQL_IS_UINTEGER);

    QString tableType;
    if (type & QSql::Tables)
        tableType += "TABLE,"_L1;
    if (type & QSql::Views)
        tableType += "VIEW,"_L1;
    if (type & QSql::SystemTables)
        tableType += "SYSTEM TABLE,"_L1;
    if (tableType.isEmpty())
        return tl;
    tableType.chop(1);

    r = SQLTables(hStmt,
                   NULL,
                   0,
                   NULL,
                   0,
                   NULL,
                   0,
                   qToTChar(tableType),
                   tableType.length());

    if (r != SQL_SUCCESS && r != SQL_SUCCESS_WITH_INFO)
        qSqlWarning("QDB2Driver::tables: Unable to execute table list"_L1, d);
    r = SQLFetchScroll(hStmt,
                        SQL_FETCH_NEXT,
                        0);
    while (r == SQL_SUCCESS) {
        bool isNull;
        QString fieldVal = qGetStringData(hStmt, 2, -1, isNull);
        QString userVal = qGetStringData(hStmt, 1, -1, isNull);
        QString user = d->user;
        if ( isIdentifierEscaped(user, QSqlDriver::TableName))
            user = stripDelimiters(user, QSqlDriver::TableName);
        else
            user = user.toUpper();

        if (userVal != user)
            fieldVal = userVal + u'.' + fieldVal;
        tl.append(fieldVal);
        r = SQLFetchScroll(hStmt,
                            SQL_FETCH_NEXT,
                            0);
    }

    r = SQLFreeHandle(SQL_HANDLE_STMT, hStmt);
    if (r != SQL_SUCCESS)
        qSqlWarning("QDB2Driver::tables: Unable to free statement handle "_L1
                    + QString::number(r), d);
    return tl;
}

QSqlIndex QDB2Driver::primaryIndex(const QString& tablename) const
{
    Q_D(const QDB2Driver);
    QSqlIndex index(tablename);
    if (!isOpen())
        return index;
    QSqlRecord rec = record(tablename);

    SQLHANDLE hStmt;
    SQLRETURN r = SQLAllocHandle(SQL_HANDLE_STMT,
                                  d->hDbc,
                                  &hStmt);
    if (r != SQL_SUCCESS) {
        qSqlWarning("QDB2Driver::primaryIndex: Unable to list primary key"_L1, d);
        return index;
    }
    QString catalog, schema, table;
    d->qSplitTableQualifier(tablename, catalog, schema, table);

    if (isIdentifierEscaped(catalog, QSqlDriver::TableName))
        catalog = stripDelimiters(catalog, QSqlDriver::TableName);
    else
        catalog = catalog.toUpper();

    if (isIdentifierEscaped(schema, QSqlDriver::TableName))
        schema = stripDelimiters(schema, QSqlDriver::TableName);
    else
        schema = schema.toUpper();

    if (isIdentifierEscaped(table, QSqlDriver::TableName))
        table = stripDelimiters(table, QSqlDriver::TableName);
    else
        table = table.toUpper();

    r = SQLSetStmtAttr(hStmt,
                        SQL_ATTR_CURSOR_TYPE,
                        (SQLPOINTER)SQL_CURSOR_FORWARD_ONLY,
                        SQL_IS_UINTEGER);

    r = SQLPrimaryKeys(hStmt,
                        NULL,
                        0,
                        qToTChar(schema),
                        schema.length(),
                        qToTChar(table),
                        table.length());
    r = SQLFetchScroll(hStmt,
                        SQL_FETCH_NEXT,
                        0);

    bool isNull;
    QString cName, idxName;
    // Store all fields in a StringList because the driver can't detail fields in this FETCH loop
    while (r == SQL_SUCCESS) {
        cName = qGetStringData(hStmt, 3, -1, isNull); // column name
        idxName = qGetStringData(hStmt, 5, -1, isNull); // pk index name
        index.append(rec.field(cName));
        index.setName(idxName);
        r = SQLFetchScroll(hStmt,
                            SQL_FETCH_NEXT,
                            0);
    }
    r = SQLFreeHandle(SQL_HANDLE_STMT, hStmt);
    if (r!= SQL_SUCCESS)
        qSqlWarning("QDB2Driver: Unable to free statement handle "_L1
                    + QString::number(r), d);
    return index;
}

bool QDB2Driver::hasFeature(DriverFeature f) const
{
    switch (f) {
        case QuerySize:
        case NamedPlaceholders:
        case BatchOperations:
        case LastInsertId:
        case SimpleLocking:
        case EventNotifications:
        case CancelQuery:
            return false;
        case BLOB:
        case Transactions:
        case MultipleResultSets:
        case PreparedQueries:
        case PositionalPlaceholders:
        case LowPrecisionNumbers:
        case FinishQuery:
            return true;
        case Unicode:
            return true;
    }
    return false;
}

bool QDB2Driver::beginTransaction()
{
    if (!isOpen()) {
        qWarning("QDB2Driver::beginTransaction: Database not open");
        return false;
    }
    return setAutoCommit(false);
}

bool QDB2Driver::commitTransaction()
{
    Q_D(QDB2Driver);
    if (!isOpen()) {
        qWarning("QDB2Driver::commitTransaction: Database not open");
        return false;
    }
    SQLRETURN r = SQLEndTran(SQL_HANDLE_DBC,
                              d->hDbc,
                              SQL_COMMIT);
    if (r != SQL_SUCCESS) {
        setLastError(qMakeError(tr("Unable to commit transaction"),
                     QSqlError::TransactionError, d));
        return false;
    }
    return setAutoCommit(true);
}

bool QDB2Driver::rollbackTransaction()
{
    Q_D(QDB2Driver);
    if (!isOpen()) {
        qWarning("QDB2Driver::rollbackTransaction: Database not open");
        return false;
    }
    SQLRETURN r = SQLEndTran(SQL_HANDLE_DBC,
                              d->hDbc,
                              SQL_ROLLBACK);
    if (r != SQL_SUCCESS) {
        setLastError(qMakeError(tr("Unable to rollback transaction"),
                                QSqlError::TransactionError, d));
        return false;
    }
    return setAutoCommit(true);
}

bool QDB2Driver::setAutoCommit(bool autoCommit)
{
    Q_D(QDB2Driver);
    SQLULEN ac = autoCommit ? SQL_AUTOCOMMIT_ON : SQL_AUTOCOMMIT_OFF;
    SQLRETURN r  = SQLSetConnectAttr(d->hDbc,
                                      SQL_ATTR_AUTOCOMMIT,
                                      reinterpret_cast<SQLPOINTER>(ac),
                                      sizeof(ac));
    if (r != SQL_SUCCESS) {
        setLastError(qMakeError(tr("Unable to set autocommit"),
                                QSqlError::TransactionError, d));
        return false;
    }
    return true;
}

QString QDB2Driver::formatValue(const QSqlField &field, bool trimStrings) const
{
    if (field.isNull())
        return "NULL"_L1;

    switch (field.metaType().id()) {
        case QMetaType::QDateTime: {
            // Use an escape sequence for the datetime fields
            if (field.value().toDateTime().isValid()) {
                QDate dt = field.value().toDateTime().date();
                QTime tm = field.value().toDateTime().time();
                // Dateformat has to be "yyyy-MM-dd hh:mm:ss", with leading zeroes if month or day < 10
                return u'\'' + QString::number(dt.year()) + u'-'
                       + QString::number(dt.month()) + u'-'
                       + QString::number(dt.day()) + u'-'
                       + QString::number(tm.hour()) + u'.'
                       + QString::number(tm.minute()).rightJustified(2, u'0', true)
                       + u'.'
                       + QString::number(tm.second()).rightJustified(2, u'0', true)
                       + u'.'
                       + QString::number(tm.msec() * 1000).rightJustified(6, u'0', true)
                       + u'\'';
                } else {
                    return "NULL"_L1;
                }
        }
        case QMetaType::QByteArray: {
            const QByteArray ba = field.value().toByteArray();
            QString r;
            r.reserve(ba.size() * 2 + 9);
            r += "BLOB(X'"_L1;
            for (const char c : ba) {
                const uchar s = uchar(c);
                r += QLatin1Char(QtMiscUtils::toHexLower(s >> 4));
                r += QLatin1Char(QtMiscUtils::toHexLower(s & 0x0f));
            }
            r += "')"_L1;
            return r;
        }
        default:
            return QSqlDriver::formatValue(field, trimStrings);
    }
}

QVariant QDB2Driver::handle() const
{
    Q_D(const QDB2Driver);
    return QVariant(QMetaType::fromType<SQLHANDLE>(), &d->hDbc);
}

QString QDB2Driver::escapeIdentifier(const QString &identifier, IdentifierType) const
{
    QString res = identifier;
    if (!identifier.isEmpty() && !identifier.startsWith(u'"') && !identifier.endsWith(u'"') ) {
        res.replace(u'"', "\"\""_L1);
        res.replace(u'.', "\".\""_L1);
        res = u'"' + res + u'"';
    }
    return res;
}

QT_END_NAMESPACE

#include "moc_qsql_db2_p.cpp"
