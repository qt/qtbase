// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#include "qsql_odbc_p.h"
#include <qsqlrecord.h>

#if defined (Q_OS_WIN32)
#include <qt_windows.h>
#endif
#include <qcoreapplication.h>
#include <qdatetime.h>
#include <qlist.h>
#include <qmath.h>
#include <qsqlerror.h>
#include <qsqlfield.h>
#include <qsqlindex.h>
#include <qstringconverter.h>
#include <qstringlist.h>
#include <qvariant.h>
#include <qvarlengtharray.h>
#include <QDebug>
#include <QSqlQuery>
#include <QtSql/private/qsqldriver_p.h>
#include <QtSql/private/qsqlresult_p.h>
#include "private/qtools_p.h"

QT_BEGIN_NAMESPACE

using namespace Qt::StringLiterals;

// non-standard ODBC SQL data type from SQL Server sometimes used instead of SQL_TIME
#ifndef SQL_SS_TIME2
#define SQL_SS_TIME2 (-154)
#endif

// undefine this to prevent initial check of the ODBC driver
#define ODBC_CHECK_DRIVER

static constexpr int COLNAMESIZE = 256;
static constexpr SQLSMALLINT TABLENAMESIZE = 128;
//Map Qt parameter types to ODBC types
static constexpr SQLSMALLINT qParamType[4] = { SQL_PARAM_INPUT, SQL_PARAM_INPUT, SQL_PARAM_OUTPUT, SQL_PARAM_INPUT_OUTPUT };

template<typename C, int SIZE = sizeof(SQLTCHAR)>
inline static QString fromSQLTCHAR(const C &input, qsizetype size = -1)
{
    // Remove any trailing \0 as some drivers misguidedly append one
    qsizetype realsize = qMin(size, input.size());
    if (realsize > 0 && input[realsize - 1] == 0)
        realsize--;
    if constexpr (SIZE == 1)
        return QString::fromUtf8(reinterpret_cast<const char *>(input.constData()), realsize);
    else if constexpr (SIZE == 2)
        return QString::fromUtf16(reinterpret_cast<const char16_t *>(input.constData()), realsize);
    else if constexpr (SIZE == 4)
        return QString::fromUcs4(reinterpret_cast<const char32_t *>(input.constData()), realsize);
    else
        static_assert(QtPrivate::value_dependent_false<SIZE>(),
                      "Don't know how to handle sizeof(SQLTCHAR) != 1/2/4");
}

template<int SIZE = sizeof(SQLTCHAR)>
QStringConverter::Encoding encodingForSqlTChar()
{
    if constexpr (SIZE == 1)
        return QStringConverter::Utf8;
    else if constexpr (SIZE == 2)
        return QStringConverter::Utf16;
    else if constexpr (SIZE == 4)
        return QStringConverter::Utf32;
    else
        static_assert(QtPrivate::value_dependent_false<SIZE>(),
                      "Don't know how to handle sizeof(SQLTCHAR) != 1/2/4");
}

inline static QVarLengthArray<SQLTCHAR> toSQLTCHAR(const QString &input)
{
    QVarLengthArray<SQLTCHAR> result;
    QStringEncoder enc(encodingForSqlTChar());
    result.resize(enc.requiredSpace(input.size()));
    const auto end = enc.appendToBuffer(reinterpret_cast<char *>(result.data()), input);
    result.resize((end - reinterpret_cast<char *>(result.data())) / sizeof(SQLTCHAR));
    return result;
}

class QODBCDriverPrivate : public QSqlDriverPrivate
{
    Q_DECLARE_PUBLIC(QODBCDriver)

public:
    enum DefaultCase {Lower, Mixed, Upper, Sensitive};
    using QSqlDriverPrivate::QSqlDriverPrivate;

    SQLHANDLE hEnv = nullptr;
    SQLHANDLE hDbc = nullptr;

    int disconnectCount = 0;
    int datetimePrecision = 19;
    bool unicode = false;
    bool useSchema = false;
    bool isFreeTDSDriver = false;
    bool hasSQLFetchScroll = true;
    bool hasMultiResultSets = false;

    bool checkDriver() const;
    void checkUnicode();
    void checkDBMS();
    void checkHasSQLFetchScroll();
    void checkHasMultiResults();
    void checkSchemaUsage();
    void checkDateTimePrecision();
    bool setConnectionOptions(const QString& connOpts);
    void splitTableQualifier(const QString &qualifier, QString &catalog,
                             QString &schema, QString &table) const;
    DefaultCase defaultCase() const;
    QString adjustCase(const QString&) const;
    QChar quoteChar();
private:
    bool isQuoteInitialized = false;
    QChar quote = u'"';
};

class QODBCResultPrivate;

class QODBCResult: public QSqlResult
{
    Q_DECLARE_PRIVATE(QODBCResult)

public:
    QODBCResult(const QODBCDriver *db);
    virtual ~QODBCResult();

    bool prepare(const QString &query) override;
    bool exec() override;

    QVariant lastInsertId() const override;
    QVariant handle() const override;

protected:
    bool fetchNext() override;
    bool fetchFirst() override;
    bool fetchLast() override;
    bool fetchPrevious() override;
    bool fetch(int i) override;
    bool reset(const QString &query) override;
    QVariant data(int field) override;
    bool isNull(int field) override;
    int size() override;
    int numRowsAffected() override;
    QSqlRecord record() const override;
    void virtual_hook(int id, void *data) override;
    void detachFromResultSet() override;
    bool nextResult() override;
};

class QODBCResultPrivate: public QSqlResultPrivate
{
    Q_DECLARE_PUBLIC(QODBCResult)

public:
    Q_DECLARE_SQLDRIVER_PRIVATE(QODBCDriver)
    QODBCResultPrivate(QODBCResult *q, const QODBCDriver *db)
        : QSqlResultPrivate(q, db)
    {
        unicode = drv_d_func()->unicode;
        useSchema = drv_d_func()->useSchema;
        disconnectCount = drv_d_func()->disconnectCount;
        hasSQLFetchScroll = drv_d_func()->hasSQLFetchScroll;
    }

    inline void clearValues()
    { fieldCache.fill(QVariant()); fieldCacheIdx = 0; }

    SQLHANDLE dpEnv() const { return drv_d_func() ? drv_d_func()->hEnv : 0;}
    SQLHANDLE dpDbc() const { return drv_d_func() ? drv_d_func()->hDbc : 0;}
    SQLHANDLE hStmt = nullptr;

    QSqlRecord rInf;
    QVariantList fieldCache;
    int fieldCacheIdx = 0;
    int disconnectCount = 0;
    bool hasSQLFetchScroll = true;
    bool unicode = false;
    bool useSchema = false;

    bool isStmtHandleValid() const;
    void updateStmtHandleState();
};

bool QODBCResultPrivate::isStmtHandleValid() const
{
    return drv_d_func() && disconnectCount == drv_d_func()->disconnectCount;
}

void QODBCResultPrivate::updateStmtHandleState()
{
    disconnectCount = drv_d_func() ? drv_d_func()->disconnectCount : 0;
}

struct DiagRecord
{
    QString description;
    QString sqlState;
    QString errorCode;
};
static QList<DiagRecord> qWarnODBCHandle(int handleType, SQLHANDLE handle)
{
    SQLINTEGER nativeCode = 0;
    SQLSMALLINT msgLen = 0;
    SQLSMALLINT i = 1;
    SQLRETURN r = SQL_NO_DATA;
    QVarLengthArray<SQLTCHAR, SQL_SQLSTATE_SIZE + 1> state(SQL_SQLSTATE_SIZE + 1);
    QVarLengthArray<SQLTCHAR, SQL_MAX_MESSAGE_LENGTH + 1> description(SQL_MAX_MESSAGE_LENGTH + 1);
    QList<DiagRecord> result;

    if (!handle)
        return result;
    do {
        r = SQLGetDiagRec(handleType,
                          handle,
                          i,
                          state.data(),
                          &nativeCode,
                          description.data(),
                          description.size(),
                          &msgLen);
        if (msgLen >= description.size()) {
            description.resize(msgLen + 1); // incl. \0 termination
            continue;
        }
        if (r == SQL_SUCCESS || r == SQL_SUCCESS_WITH_INFO) {
            result.push_back({fromSQLTCHAR(description, msgLen),
                              fromSQLTCHAR(state),
                              QString::number(nativeCode)});
        } else if (r == SQL_ERROR || r == SQL_INVALID_HANDLE) {
            break;
        }
        ++i;
    } while (r != SQL_NO_DATA);
    return result;
}

static QList<DiagRecord> qODBCWarn(const SQLHANDLE hStmt,
                                   const SQLHANDLE envHandle = nullptr,
                                   const SQLHANDLE pDbC = nullptr)
{
    QList<DiagRecord> result;
    result.append(qWarnODBCHandle(SQL_HANDLE_ENV, envHandle));
    result.append(qWarnODBCHandle(SQL_HANDLE_DBC, pDbC));
    result.append(qWarnODBCHandle(SQL_HANDLE_STMT, hStmt));
    return result;
}

static QList<DiagRecord> qODBCWarn(const QODBCResultPrivate *odbc)
{
    return qODBCWarn(odbc->hStmt, odbc->dpEnv(), odbc->dpDbc());
}

static QList<DiagRecord> qODBCWarn(const QODBCDriverPrivate *odbc)
{
    return qODBCWarn(nullptr, odbc->hEnv, odbc->hDbc);
}

static DiagRecord combineRecords(const QList<DiagRecord> &records)
{
    const auto add = [](const DiagRecord &a, const DiagRecord &b) {
        return DiagRecord{a.description + u' ' + b.description,
                          a.sqlState + u';' + b.sqlState,
                          a.errorCode + u';' + b.errorCode};
    };
    return std::accumulate(std::next(records.begin()), records.end(), records.front(), add);
}

static QSqlError errorFromDiagRecords(const QString &err,
                                      QSqlError::ErrorType type,
                                      const QList<DiagRecord> &records)
{
    if (records.empty())
        return QSqlError("QODBC: unknown error"_L1, {}, type, {});
    const auto combined = combineRecords(records);
    return QSqlError("QODBC: "_L1 + err, combined.description + ", "_L1 + combined.sqlState, type,
                     combined.errorCode);
}

static QString errorStringFromDiagRecords(const QList<DiagRecord>& records)
{
    const auto combined = combineRecords(records);
    return combined.description;
}

template<class T>
static void qSqlWarning(const QString &message, T &&val)
{
    qWarning() << message << "\tError:" << errorStringFromDiagRecords(qODBCWarn(val));
}

static QSqlError qMakeError(const QString &err,
                            QSqlError::ErrorType type,
                            const QODBCResultPrivate *p)
{
    return errorFromDiagRecords(err, type, qODBCWarn(p));
}

static QSqlError qMakeError(const QString &err,
                            QSqlError::ErrorType type,
                            const QODBCDriverPrivate *p)
{
    return errorFromDiagRecords(err, type, qODBCWarn(p));
}

static QMetaType qDecodeODBCType(SQLSMALLINT sqltype, bool isSigned = true)
{
    int type = QMetaType::UnknownType;
    switch (sqltype) {
    case SQL_DECIMAL:
    case SQL_NUMERIC:
    case SQL_FLOAT: // 24 or 53 bits precision
    case SQL_DOUBLE:// 53 bits
        type = QMetaType::Double;
        break;
    case SQL_REAL:  // 24 bits
        type = QMetaType::Float;
        break;
    case SQL_SMALLINT:
        type = isSigned ? QMetaType::Short : QMetaType::UShort;
        break;
    case SQL_INTEGER:
    case SQL_BIT:
        type = isSigned ? QMetaType::Int : QMetaType::UInt;
        break;
    case SQL_TINYINT:
        type = QMetaType::UInt;
        break;
    case SQL_BIGINT:
        type = isSigned ? QMetaType::LongLong : QMetaType::ULongLong;
        break;
    case SQL_BINARY:
    case SQL_VARBINARY:
    case SQL_LONGVARBINARY:
        type = QMetaType::QByteArray;
        break;
    case SQL_DATE:
    case SQL_TYPE_DATE:
        type = QMetaType::QDate;
        break;
    case SQL_SS_TIME2:
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
        type = QMetaType::QString;
        break;
    case SQL_CHAR:
    case SQL_VARCHAR:
#if (ODBCVER >= 0x0350)
    case SQL_GUID:
#endif
    case SQL_LONGVARCHAR:
        type = QMetaType::QString;
        break;
    default:
        type = QMetaType::QByteArray;
        break;
    }
    return QMetaType(type);
}

template <typename CT>
static QVariant getStringDataImpl(SQLHANDLE hStmt, SQLUSMALLINT column, qsizetype colSize, SQLSMALLINT targetType)
{
    QString fieldVal;
    SQLRETURN r = SQL_ERROR;
    SQLLEN lengthIndicator = 0;
    QVarLengthArray<CT> buf(colSize);
    while (true) {
        r = SQLGetData(hStmt,
                       column + 1,
                       targetType,
                       SQLPOINTER(buf.data()), SQLINTEGER(buf.size() * sizeof(CT)),
                       &lengthIndicator);
        if (r == SQL_SUCCESS || r == SQL_SUCCESS_WITH_INFO) {
            if (lengthIndicator == SQL_NULL_DATA) {
                return {};
            }
            // starting with ODBC Native Client 2012, SQL_NO_TOTAL is returned
            // instead of the length (which sometimes was wrong in older versions)
            // see link for more info: http://msdn.microsoft.com/en-us/library/jj219209.aspx
            // if length indicator equals SQL_NO_TOTAL, indicating that
            // more data can be fetched, but size not known, collect data
            // and fetch next block
            if (lengthIndicator == SQL_NO_TOTAL) {
                fieldVal += fromSQLTCHAR<QVarLengthArray<CT>, sizeof(CT)>(buf, buf.size());
                continue;
            }
            // if SQL_SUCCESS_WITH_INFO is returned, indicating that
            // more data can be fetched, the length indicator does NOT
            // contain the number of bytes returned - it contains the
            // total number of bytes that CAN be fetched
            const qsizetype rSize = (r == SQL_SUCCESS_WITH_INFO)
                    ? buf.size()
                    : qsizetype(lengthIndicator / sizeof(CT));
            fieldVal += fromSQLTCHAR<QVarLengthArray<CT>, sizeof(CT)>(buf, rSize);
            // lengthIndicator does not contain the termination character
            if (lengthIndicator < SQLLEN((buf.size() - 1) * sizeof(CT))) {
                // workaround for Drivermanagers that don't return SQL_NO_DATA
                break;
            }
        } else if (r == SQL_NO_DATA) {
            break;
        } else {
            qSqlWarning("qGetStringData: Error while fetching data:"_L1, hStmt);
            return {};
        }
    }
    return fieldVal;
}

static QVariant qGetStringData(SQLHANDLE hStmt, SQLUSMALLINT column, int colSize, bool unicode)
{
    if (colSize <= 0) {
        colSize = 256;  // default Prealloc size of QVarLengthArray
    } else if (colSize > 65536) { // limit buffer size to 64 KB
        colSize = 65536;
    } else {
        colSize++; // make sure there is room for more than the 0 termination
    }
    return unicode ? getStringDataImpl<SQLTCHAR>(hStmt, column, colSize, SQL_C_TCHAR)
                   : getStringDataImpl<SQLCHAR>(hStmt, column, colSize, SQL_C_CHAR);
}

static QVariant qGetBinaryData(SQLHANDLE hStmt, int column)
{
    QByteArray fieldVal;
    SQLSMALLINT colNameLen;
    SQLSMALLINT colType;
    SQLULEN colSize;
    SQLSMALLINT colScale;
    SQLSMALLINT nullable;
    SQLLEN lengthIndicator = 0;
    SQLRETURN r = SQL_ERROR;

    QVarLengthArray<SQLTCHAR, COLNAMESIZE> colName(COLNAMESIZE);

    r = SQLDescribeCol(hStmt,
                       column + 1,
                       colName.data(), SQLSMALLINT(colName.size()),
                       &colNameLen,
                       &colType,
                       &colSize,
                       &colScale,
                       &nullable);
    if (r != SQL_SUCCESS)
        qWarning() << "qGetBinaryData: Unable to describe column" << column;
    // SQLDescribeCol may return 0 if size cannot be determined
    if (!colSize)
        colSize = 255;
    else if (colSize > 65536) // read the field in 64 KB chunks
        colSize = 65536;
    fieldVal.resize(colSize);
    ulong read = 0;
    while (true) {
        r = SQLGetData(hStmt,
                        column+1,
                        SQL_C_BINARY,
                        const_cast<char *>(fieldVal.constData() + read),
                        colSize,
                        &lengthIndicator);
        if (r != SQL_SUCCESS && r != SQL_SUCCESS_WITH_INFO)
            break;
        if (lengthIndicator == SQL_NULL_DATA)
            return QVariant(QMetaType(QMetaType::QByteArray));
        if (lengthIndicator > SQLLEN(colSize) || lengthIndicator == SQL_NO_TOTAL) {
            read += colSize;
            colSize = 65536;
        } else {
            read += lengthIndicator;
        }
        if (r == SQL_SUCCESS) { // the whole field was read in one chunk
            fieldVal.resize(read);
            break;
        }
        fieldVal.resize(fieldVal.size() + colSize);
    }
    return fieldVal;
}

static QVariant qGetIntData(SQLHANDLE hStmt, int column, bool isSigned = true)
{
    SQLINTEGER intbuf = 0;
    SQLLEN lengthIndicator = 0;
    SQLRETURN r = SQLGetData(hStmt,
                              column+1,
                              isSigned ? SQL_C_SLONG : SQL_C_ULONG,
                              (SQLPOINTER)&intbuf,
                              sizeof(intbuf),
                              &lengthIndicator);
    if (r != SQL_SUCCESS && r != SQL_SUCCESS_WITH_INFO)
        return QVariant();
    if (lengthIndicator == SQL_NULL_DATA)
        return QVariant(QMetaType::fromType<int>());
    if (isSigned)
        return int(intbuf);
    else
        return uint(intbuf);
}

static QVariant qGetDoubleData(SQLHANDLE hStmt, int column)
{
    SQLDOUBLE dblbuf;
    SQLLEN lengthIndicator = 0;
    SQLRETURN r = SQLGetData(hStmt,
                              column+1,
                              SQL_C_DOUBLE,
                              (SQLPOINTER) &dblbuf,
                              0,
                              &lengthIndicator);
    if (r != SQL_SUCCESS && r != SQL_SUCCESS_WITH_INFO) {
        return QVariant();
    }
    if (lengthIndicator == SQL_NULL_DATA)
        return QVariant(QMetaType::fromType<double>());

    return (double) dblbuf;
}


static QVariant qGetBigIntData(SQLHANDLE hStmt, int column, bool isSigned = true)
{
    SQLBIGINT lngbuf = 0;
    SQLLEN lengthIndicator = 0;
    SQLRETURN r = SQLGetData(hStmt,
                              column+1,
                              isSigned ? SQL_C_SBIGINT : SQL_C_UBIGINT,
                              (SQLPOINTER) &lngbuf,
                              sizeof(lngbuf),
                              &lengthIndicator);
    if (r != SQL_SUCCESS && r != SQL_SUCCESS_WITH_INFO)
        return QVariant();
    if (lengthIndicator == SQL_NULL_DATA)
        return QVariant(QMetaType::fromType<qlonglong>());

    if (isSigned)
        return qint64(lngbuf);
    else
        return quint64(lngbuf);
}

static bool isAutoValue(const SQLHANDLE hStmt, int column)
{
    SQLLEN nNumericAttribute = 0; // Check for auto-increment
    const SQLRETURN r = ::SQLColAttribute(hStmt, column + 1, SQL_DESC_AUTO_UNIQUE_VALUE,
                                          0, 0, 0, &nNumericAttribute);
    if (r != SQL_SUCCESS && r != SQL_SUCCESS_WITH_INFO) {
        qSqlWarning(QStringLiteral("qMakeField: Unable to get autovalue attribute for column ")
                    + QString::number(column), hStmt);
        return false;
    }
    return nNumericAttribute != SQL_FALSE;
}

static QSqlField qMakeFieldInfo(const SQLHANDLE hStmt, int i, QString *errorMessage);

// creates a QSqlField from a valid hStmt generated
// by SQLColumns. The hStmt has to point to a valid position.
static QSqlField qMakeFieldInfo(const SQLHANDLE hStmt, const QODBCDriverPrivate* p)
{
    QString fname = qGetStringData(hStmt, 3, -1, p->unicode).toString();
    int type = qGetIntData(hStmt, 4).toInt(); // column type
    QSqlField f(fname, qDecodeODBCType(type, p));
    QVariant var = qGetIntData(hStmt, 6);
    f.setLength(var.isNull() ? -1 : var.toInt()); // column size
    var = qGetIntData(hStmt, 8).toInt();
    f.setPrecision(var.isNull() ? -1 : var.toInt()); // precision
    f.setSqlType(type);
    int required = qGetIntData(hStmt, 10).toInt(); // nullable-flag
    // required can be SQL_NO_NULLS, SQL_NULLABLE or SQL_NULLABLE_UNKNOWN
    if (required == SQL_NO_NULLS)
        f.setRequired(true);
    else if (required == SQL_NULLABLE)
        f.setRequired(false);
    // else we don't know
    return f;
}

static QSqlField qMakeFieldInfo(const QODBCResultPrivate* p, int i )
{
    QString errorMessage;
    const QSqlField result = qMakeFieldInfo(p->hStmt, i, &errorMessage);
    if (!errorMessage.isEmpty())
        qSqlWarning(errorMessage, p);
    return result;
}

static QSqlField qMakeFieldInfo(const SQLHANDLE hStmt, int i, QString *errorMessage)
{
    SQLSMALLINT colNameLen;
    SQLSMALLINT colType;
    SQLULEN colSize;
    SQLSMALLINT colScale;
    SQLSMALLINT nullable;
    SQLRETURN r = SQL_ERROR;
    QVarLengthArray<SQLTCHAR, COLNAMESIZE> colName(COLNAMESIZE);
    errorMessage->clear();
    r = SQLDescribeCol(hStmt,
                        i+1,
                        colName.data(), SQLSMALLINT(colName.size()),
                        &colNameLen,
                        &colType,
                        &colSize,
                        &colScale,
                        &nullable);

    if (r != SQL_SUCCESS) {
        *errorMessage = QStringLiteral("qMakeField: Unable to describe column ") + QString::number(i);
        return QSqlField();
    }

    SQLLEN unsignedFlag = SQL_FALSE;
    r = SQLColAttribute (hStmt,
                         i + 1,
                         SQL_DESC_UNSIGNED,
                         0,
                         0,
                         0,
                         &unsignedFlag);
    if (r != SQL_SUCCESS) {
        qSqlWarning(QStringLiteral("qMakeField: Unable to get column attributes for column ")
                    + QString::number(i), hStmt);
    }

    const QString qColName(fromSQLTCHAR(colName, colNameLen));
    // nullable can be SQL_NO_NULLS, SQL_NULLABLE or SQL_NULLABLE_UNKNOWN
    QMetaType type = qDecodeODBCType(colType, unsignedFlag == SQL_FALSE);
    QSqlField f(qColName, type);
    f.setSqlType(colType);
    f.setLength(colSize == 0 ? -1 : int(colSize));
    f.setPrecision(colScale == 0 ? -1 : int(colScale));
    if (nullable == SQL_NO_NULLS)
        f.setRequired(true);
    else if (nullable == SQL_NULLABLE)
        f.setRequired(false);
    // else we don't know
    f.setAutoValue(isAutoValue(hStmt, i));
    QVarLengthArray<SQLTCHAR, TABLENAMESIZE> tableName(TABLENAMESIZE);
    SQLSMALLINT tableNameLen;
    r = SQLColAttribute(hStmt,
                        i + 1,
                        SQL_DESC_BASE_TABLE_NAME,
                        tableName.data(),
                        SQLSMALLINT(tableName.size() * sizeof(SQLTCHAR)), // SQLColAttribute needs/returns size in bytes
                        &tableNameLen,
                        0);
    if (r == SQL_SUCCESS)
        f.setTableName(fromSQLTCHAR(tableName, tableNameLen / sizeof(SQLTCHAR)));
    return f;
}

static size_t qGetODBCVersion(const QString &connOpts)
{
    if (connOpts.contains("SQL_ATTR_ODBC_VERSION=SQL_OV_ODBC3"_L1, Qt::CaseInsensitive))
        return SQL_OV_ODBC3;
    return SQL_OV_ODBC2;
}

QChar QODBCDriverPrivate::quoteChar()
{
    if (!isQuoteInitialized) {
        SQLTCHAR driverResponse[4];
        SQLSMALLINT length;
        int r = SQLGetInfo(hDbc,
                SQL_IDENTIFIER_QUOTE_CHAR,
                &driverResponse,
                sizeof(driverResponse),
                &length);
        if (r == SQL_SUCCESS || r == SQL_SUCCESS_WITH_INFO)
            quote = QChar(driverResponse[0]);
        else
            quote = u'"';
        isQuoteInitialized = true;
    }
    return quote;
}

static SQLRETURN qt_string_SQLSetConnectAttr(SQLHDBC handle, SQLINTEGER attr, const QString &val)
{
    auto encoded = toSQLTCHAR(val);
    return SQLSetConnectAttr(handle, attr,
                             encoded.data(),
                             SQLINTEGER(encoded.size() * sizeof(SQLTCHAR))); // size in bytes
}


bool QODBCDriverPrivate::setConnectionOptions(const QString& connOpts)
{
    // Set any connection attributes
    const QStringList opts(connOpts.split(u';', Qt::SkipEmptyParts));
    SQLRETURN r = SQL_SUCCESS;
    for (int i = 0; i < opts.count(); ++i) {
        const QString tmp(opts.at(i));
        int idx;
        if ((idx = tmp.indexOf(u'=')) == -1) {
            qWarning() << "QODBCDriver::open: Illegal connect option value '" << tmp << '\'';
            continue;
        }
        const QString opt(tmp.left(idx));
        const QString val(tmp.mid(idx + 1).simplified());
        SQLUINTEGER v = 0;

        r = SQL_SUCCESS;
        if (opt.toUpper() == "SQL_ATTR_ACCESS_MODE"_L1) {
            if (val.toUpper() == "SQL_MODE_READ_ONLY"_L1) {
                v = SQL_MODE_READ_ONLY;
            } else if (val.toUpper() == "SQL_MODE_READ_WRITE"_L1) {
                v = SQL_MODE_READ_WRITE;
            } else {
                qWarning() << "QODBCDriver::open: Unknown option value '" << val << '\'';
                continue;
            }
            r = SQLSetConnectAttr(hDbc, SQL_ATTR_ACCESS_MODE, (SQLPOINTER) size_t(v), 0);
        } else if (opt.toUpper() == "SQL_ATTR_CONNECTION_TIMEOUT"_L1) {
            v = val.toUInt();
            r = SQLSetConnectAttr(hDbc, SQL_ATTR_CONNECTION_TIMEOUT, (SQLPOINTER) size_t(v), 0);
        } else if (opt.toUpper() == "SQL_ATTR_LOGIN_TIMEOUT"_L1) {
            v = val.toUInt();
            r = SQLSetConnectAttr(hDbc, SQL_ATTR_LOGIN_TIMEOUT, (SQLPOINTER) size_t(v), 0);
        } else if (opt.toUpper() == "SQL_ATTR_CURRENT_CATALOG"_L1) {
            r = qt_string_SQLSetConnectAttr(hDbc, SQL_ATTR_CURRENT_CATALOG, val);
        } else if (opt.toUpper() == "SQL_ATTR_METADATA_ID"_L1) {
            if (val.toUpper() == "SQL_TRUE"_L1) {
                v = SQL_TRUE;
            } else if (val.toUpper() == "SQL_FALSE"_L1) {
                v = SQL_FALSE;
            } else {
                qWarning() << "QODBCDriver::open: Unknown option value '" << val << '\'';
                continue;
            }
            r = SQLSetConnectAttr(hDbc, SQL_ATTR_METADATA_ID, (SQLPOINTER) size_t(v), 0);
        } else if (opt.toUpper() == "SQL_ATTR_PACKET_SIZE"_L1) {
            v = val.toUInt();
            r = SQLSetConnectAttr(hDbc, SQL_ATTR_PACKET_SIZE, (SQLPOINTER) size_t(v), 0);
        } else if (opt.toUpper() == "SQL_ATTR_TRACEFILE"_L1) {
            r = qt_string_SQLSetConnectAttr(hDbc, SQL_ATTR_TRACEFILE, val);
        } else if (opt.toUpper() == "SQL_ATTR_TRACE"_L1) {
            if (val.toUpper() == "SQL_OPT_TRACE_OFF"_L1) {
                v = SQL_OPT_TRACE_OFF;
            } else if (val.toUpper() == "SQL_OPT_TRACE_ON"_L1) {
                v = SQL_OPT_TRACE_ON;
            } else {
                qWarning() << "QODBCDriver::open: Unknown option value '" << val << '\'';
                continue;
            }
            r = SQLSetConnectAttr(hDbc, SQL_ATTR_TRACE, (SQLPOINTER) size_t(v), 0);
        } else if (opt.toUpper() == "SQL_ATTR_CONNECTION_POOLING"_L1) {
            if (val == "SQL_CP_OFF"_L1)
                v = SQL_CP_OFF;
            else if (val.toUpper() == "SQL_CP_ONE_PER_DRIVER"_L1)
                v = SQL_CP_ONE_PER_DRIVER;
            else if (val.toUpper() == "SQL_CP_ONE_PER_HENV"_L1)
                v = SQL_CP_ONE_PER_HENV;
            else if (val.toUpper() == "SQL_CP_DEFAULT"_L1)
                v = SQL_CP_DEFAULT;
            else {
                qWarning() << "QODBCDriver::open: Unknown option value '" << val << '\'';
                continue;
            }
            r = SQLSetConnectAttr(hDbc, SQL_ATTR_CONNECTION_POOLING, (SQLPOINTER) size_t(v), 0);
        } else if (opt.toUpper() == "SQL_ATTR_CP_MATCH"_L1) {
            if (val.toUpper() == "SQL_CP_STRICT_MATCH"_L1)
                v = SQL_CP_STRICT_MATCH;
            else if (val.toUpper() == "SQL_CP_RELAXED_MATCH"_L1)
                v = SQL_CP_RELAXED_MATCH;
            else if (val.toUpper() == "SQL_CP_MATCH_DEFAULT"_L1)
                v = SQL_CP_MATCH_DEFAULT;
            else {
                qWarning() << "QODBCDriver::open: Unknown option value '" << val << '\'';
                continue;
            }
            r = SQLSetConnectAttr(hDbc, SQL_ATTR_CP_MATCH, (SQLPOINTER) size_t(v), 0);
        } else if (opt.toUpper() == "SQL_ATTR_ODBC_VERSION"_L1) {
            // Already handled in QODBCDriver::open()
            continue;
        } else {
                qWarning() << "QODBCDriver::open: Unknown connection attribute '" << opt << '\'';
        }
        if (r != SQL_SUCCESS && r != SQL_SUCCESS_WITH_INFO)
            qSqlWarning(QString::fromLatin1("QODBCDriver::open: Unable to set connection attribute'%1'").arg(
                        opt), this);
    }
    return true;
}

void QODBCDriverPrivate::splitTableQualifier(const QString &qualifier, QString &catalog,
                                             QString &schema, QString &table) const
{
    if (!useSchema) {
        table = qualifier;
        return;
    }
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

QODBCDriverPrivate::DefaultCase QODBCDriverPrivate::defaultCase() const
{
    DefaultCase ret;
    SQLUSMALLINT casing;
    int r = SQLGetInfo(hDbc,
            SQL_IDENTIFIER_CASE,
            &casing,
            sizeof(casing),
            NULL);
    if ( r != SQL_SUCCESS)
        ret = Mixed;//arbitrary case if driver cannot be queried
    else {
        switch (casing) {
            case (SQL_IC_UPPER):
                ret = Upper;
                break;
            case (SQL_IC_LOWER):
                ret = Lower;
                break;
            case (SQL_IC_SENSITIVE):
                ret = Sensitive;
                break;
            case (SQL_IC_MIXED):
            default:
                ret = Mixed;
                break;
        }
    }
    return ret;
}

/*
   Adjust the casing of an identifier to match what the
   database engine would have done to it.
*/
QString QODBCDriverPrivate::adjustCase(const QString &identifier) const
{
    QString ret = identifier;
    switch(defaultCase()) {
        case (Lower):
            ret = identifier.toLower();
            break;
        case (Upper):
            ret = identifier.toUpper();
            break;
        case(Mixed):
        case(Sensitive):
        default:
            ret = identifier;
    }
    return ret;
}

////////////////////////////////////////////////////////////////////////////

QODBCResult::QODBCResult(const QODBCDriver *db)
    : QSqlResult(*new QODBCResultPrivate(this, db))
{
}

QODBCResult::~QODBCResult()
{
    Q_D(QODBCResult);
    if (d->hStmt && d->isStmtHandleValid() && driver() && driver()->isOpen()) {
        SQLRETURN r = SQLFreeHandle(SQL_HANDLE_STMT, d->hStmt);
        if (r != SQL_SUCCESS)
            qSqlWarning("QODBCDriver: Unable to free statement handle "_L1
                         + QString::number(r), d);
    }
}

bool QODBCResult::reset (const QString& query)
{
    Q_D(QODBCResult);
    setActive(false);
    setAt(QSql::BeforeFirstRow);
    d->rInf.clear();
    d->fieldCache.clear();
    d->fieldCacheIdx = 0;

    // Always reallocate the statement handle - the statement attributes
    // are not reset if SQLFreeStmt() is called which causes some problems.
    SQLRETURN r;
    if (d->hStmt && d->isStmtHandleValid()) {
        r = SQLFreeHandle(SQL_HANDLE_STMT, d->hStmt);
        if (r != SQL_SUCCESS) {
            qSqlWarning("QODBCResult::reset: Unable to free statement handle"_L1, d);
            return false;
        }
    }
    r  = SQLAllocHandle(SQL_HANDLE_STMT,
                         d->dpDbc(),
                         &d->hStmt);
    if (r != SQL_SUCCESS) {
        qSqlWarning("QODBCResult::reset: Unable to allocate statement handle"_L1, d);
        return false;
    }

    d->updateStmtHandleState();

    if (isForwardOnly()) {
        r = SQLSetStmtAttr(d->hStmt,
                            SQL_ATTR_CURSOR_TYPE,
                            (SQLPOINTER)SQL_CURSOR_FORWARD_ONLY,
                            SQL_IS_UINTEGER);
    } else {
        r = SQLSetStmtAttr(d->hStmt,
                            SQL_ATTR_CURSOR_TYPE,
                            (SQLPOINTER)SQL_CURSOR_STATIC,
                            SQL_IS_UINTEGER);
    }
    if (r != SQL_SUCCESS && r != SQL_SUCCESS_WITH_INFO) {
        setLastError(qMakeError(QCoreApplication::translate("QODBCResult",
            "QODBCResult::reset: Unable to set 'SQL_CURSOR_STATIC' as statement attribute. "
            "Please check your ODBC driver configuration"), QSqlError::StatementError, d));
        return false;
    }

    {
        auto encoded = toSQLTCHAR(query);
        r = SQLExecDirect(d->hStmt,
                          encoded.data(),
                          SQLINTEGER(encoded.size()));
    }
    if (r != SQL_SUCCESS && r != SQL_SUCCESS_WITH_INFO && r!= SQL_NO_DATA) {
        setLastError(qMakeError(QCoreApplication::translate("QODBCResult",
                     "Unable to execute statement"), QSqlError::StatementError, d));
        return false;
    }

    SQLULEN isScrollable = 0;
    r = SQLGetStmtAttr(d->hStmt, SQL_ATTR_CURSOR_SCROLLABLE, &isScrollable, SQL_IS_INTEGER, 0);
    if (r == SQL_SUCCESS || r == SQL_SUCCESS_WITH_INFO)
        setForwardOnly(isScrollable == SQL_NONSCROLLABLE);

    SQLSMALLINT count = 0;
    SQLNumResultCols(d->hStmt, &count);
    if (count) {
        setSelect(true);
        for (SQLSMALLINT i = 0; i < count; ++i) {
            d->rInf.append(qMakeFieldInfo(d, i));
        }
        d->fieldCache.resize(count);
    } else {
        setSelect(false);
    }
    setActive(true);

    return true;
}

bool QODBCResult::fetch(int i)
{
    Q_D(QODBCResult);
    if (!driver()->isOpen())
        return false;

    if (isForwardOnly() && i < at())
        return false;
    if (i == at())
        return true;
    d->clearValues();
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
    if (r != SQL_SUCCESS) {
        if (r != SQL_NO_DATA)
            setLastError(qMakeError(QCoreApplication::translate("QODBCResult",
                "Unable to fetch"), QSqlError::ConnectionError, d));
        return false;
    }
    setAt(i);
    return true;
}

bool QODBCResult::fetchNext()
{
    Q_D(QODBCResult);
    SQLRETURN r;
    d->clearValues();

    if (d->hasSQLFetchScroll)
        r = SQLFetchScroll(d->hStmt,
                           SQL_FETCH_NEXT,
                           0);
    else
        r = SQLFetch(d->hStmt);

    if (r != SQL_SUCCESS && r != SQL_SUCCESS_WITH_INFO) {
        if (r != SQL_NO_DATA)
            setLastError(qMakeError(QCoreApplication::translate("QODBCResult",
                "Unable to fetch next"), QSqlError::ConnectionError, d));
        return false;
    }
    setAt(at() + 1);
    return true;
}

bool QODBCResult::fetchFirst()
{
    Q_D(QODBCResult);
    if (isForwardOnly() && at() != QSql::BeforeFirstRow)
        return false;
    SQLRETURN r;
    d->clearValues();
    if (isForwardOnly()) {
        return fetchNext();
    }
    r = SQLFetchScroll(d->hStmt,
                       SQL_FETCH_FIRST,
                       0);
    if (r != SQL_SUCCESS) {
        if (r != SQL_NO_DATA)
            setLastError(qMakeError(QCoreApplication::translate("QODBCResult",
                "Unable to fetch first"), QSqlError::ConnectionError, d));
        return false;
    }
    setAt(0);
    return true;
}

bool QODBCResult::fetchPrevious()
{
    Q_D(QODBCResult);
    if (isForwardOnly())
        return false;
    SQLRETURN r;
    d->clearValues();
    r = SQLFetchScroll(d->hStmt,
                       SQL_FETCH_PRIOR,
                       0);
    if (r != SQL_SUCCESS) {
        if (r != SQL_NO_DATA)
            setLastError(qMakeError(QCoreApplication::translate("QODBCResult",
                "Unable to fetch previous"), QSqlError::ConnectionError, d));
        return false;
    }
    setAt(at() - 1);
    return true;
}

bool QODBCResult::fetchLast()
{
    Q_D(QODBCResult);
    SQLRETURN r;
    d->clearValues();

    if (isForwardOnly()) {
        // cannot seek to last row in forwardOnly mode, so we have to use brute force
        int i = at();
        if (i == QSql::AfterLastRow)
            return false;
        if (i == QSql::BeforeFirstRow)
            i = 0;
        while (fetchNext())
            ++i;
        setAt(i);
        return true;
    }

    r = SQLFetchScroll(d->hStmt,
                       SQL_FETCH_LAST,
                       0);
    if (r != SQL_SUCCESS) {
        if (r != SQL_NO_DATA)
            setLastError(qMakeError(QCoreApplication::translate("QODBCResult",
                "Unable to fetch last"), QSqlError::ConnectionError, d));
        return false;
    }
    SQLULEN currRow = 0;
    r = SQLGetStmtAttr(d->hStmt,
                        SQL_ROW_NUMBER,
                        &currRow,
                        SQL_IS_INTEGER,
                        0);
    if (r != SQL_SUCCESS)
        return false;
    setAt(currRow-1);
    return true;
}

QVariant QODBCResult::data(int field)
{
    Q_D(QODBCResult);
    if (field >= d->rInf.count() || field < 0) {
        qWarning() << "QODBCResult::data: column" << field << "out of range";
        return QVariant();
    }
    if (field < d->fieldCacheIdx)
        return d->fieldCache.at(field);

    SQLRETURN r(0);
    SQLLEN lengthIndicator = 0;

    for (int i = d->fieldCacheIdx; i <= field; ++i) {
        // some servers do not support fetching column n after we already
        // fetched column n+1, so cache all previous columns here
        const QSqlField info = d->rInf.field(i);
        switch (info.metaType().id()) {
        case QMetaType::LongLong:
            d->fieldCache[i] = qGetBigIntData(d->hStmt, i);
        break;
        case QMetaType::ULongLong:
            d->fieldCache[i] = qGetBigIntData(d->hStmt, i, false);
            break;
        case QMetaType::Int:
        case QMetaType::Short:
            d->fieldCache[i] = qGetIntData(d->hStmt, i);
            break;
        case QMetaType::UInt:
        case QMetaType::UShort:
            d->fieldCache[i] = qGetIntData(d->hStmt, i, false);
            break;
        case QMetaType::QDate:
            DATE_STRUCT dbuf;
            r = SQLGetData(d->hStmt,
                            i + 1,
                            SQL_C_DATE,
                            (SQLPOINTER)&dbuf,
                            0,
                            &lengthIndicator);
            if ((r == SQL_SUCCESS || r == SQL_SUCCESS_WITH_INFO) && (lengthIndicator != SQL_NULL_DATA))
                d->fieldCache[i] = QVariant(QDate(dbuf.year, dbuf.month, dbuf.day));
            else
                d->fieldCache[i] = QVariant(QMetaType::fromType<QDate>());
        break;
        case QMetaType::QTime:
            TIME_STRUCT tbuf;
            r = SQLGetData(d->hStmt,
                            i + 1,
                            SQL_C_TIME,
                            (SQLPOINTER)&tbuf,
                            0,
                            &lengthIndicator);
            if ((r == SQL_SUCCESS || r == SQL_SUCCESS_WITH_INFO) && (lengthIndicator != SQL_NULL_DATA))
                d->fieldCache[i] = QVariant(QTime(tbuf.hour, tbuf.minute, tbuf.second));
            else
                d->fieldCache[i] = QVariant(QMetaType::fromType<QTime>());
        break;
        case QMetaType::QDateTime:
            TIMESTAMP_STRUCT dtbuf;
            r = SQLGetData(d->hStmt,
                            i + 1,
                            SQL_C_TIMESTAMP,
                            (SQLPOINTER)&dtbuf,
                            0,
                            &lengthIndicator);
            if ((r == SQL_SUCCESS || r == SQL_SUCCESS_WITH_INFO) && (lengthIndicator != SQL_NULL_DATA))
                d->fieldCache[i] = QVariant(QDateTime(QDate(dtbuf.year, dtbuf.month, dtbuf.day),
                       QTime(dtbuf.hour, dtbuf.minute, dtbuf.second, dtbuf.fraction / 1000000)));
            else
                d->fieldCache[i] = QVariant(QMetaType::fromType<QDateTime>());
            break;
        case QMetaType::QByteArray:
            d->fieldCache[i] = qGetBinaryData(d->hStmt, i);
            break;
        case QMetaType::QString:
            d->fieldCache[i] = qGetStringData(d->hStmt, i, info.length(), d->unicode);
            break;
        case QMetaType::Double:
            switch(numericalPrecisionPolicy()) {
                case QSql::LowPrecisionInt32:
                    d->fieldCache[i] = qGetIntData(d->hStmt, i);
                    break;
                case QSql::LowPrecisionInt64:
                    d->fieldCache[i] = qGetBigIntData(d->hStmt, i);
                    break;
                case QSql::LowPrecisionDouble:
                    d->fieldCache[i] = qGetDoubleData(d->hStmt, i);
                    break;
                case QSql::HighPrecision:
                    const int extra = info.precision() > 0 ? 1 : 0;
                    d->fieldCache[i] = qGetStringData(d->hStmt, i, info.length() + extra, false);
                    break;
            }
            break;
        default:
            d->fieldCache[i] = qGetStringData(d->hStmt, i, info.length(), false);
            break;
        }
        d->fieldCacheIdx = field + 1;
    }
    return d->fieldCache[field];
}

bool QODBCResult::isNull(int field)
{
    Q_D(const QODBCResult);
    if (field < 0 || field >= d->fieldCache.size())
        return true;
    if (field >= d->fieldCacheIdx) {
        // since there is no good way to find out whether the value is NULL
        // without fetching the field we'll fetch it here.
        // (data() also sets the NULL flag)
        data(field);
    }
    return d->fieldCache.at(field).isNull();
}

int QODBCResult::size()
{
    return -1;
}

int QODBCResult::numRowsAffected()
{
    Q_D(QODBCResult);
    SQLLEN affectedRowCount = 0;
    SQLRETURN r = SQLRowCount(d->hStmt, &affectedRowCount);
    if (r == SQL_SUCCESS)
        return affectedRowCount;
    else
        qSqlWarning("QODBCResult::numRowsAffected: Unable to count affected rows"_L1, d);
    return -1;
}

bool QODBCResult::prepare(const QString& query)
{
    Q_D(QODBCResult);
    setActive(false);
    setAt(QSql::BeforeFirstRow);
    SQLRETURN r;

    d->rInf.clear();
    if (d->hStmt && d->isStmtHandleValid()) {
        r = SQLFreeHandle(SQL_HANDLE_STMT, d->hStmt);
        if (r != SQL_SUCCESS) {
            qSqlWarning("QODBCResult::prepare: Unable to close statement"_L1, d);
            return false;
        }
    }
    r  = SQLAllocHandle(SQL_HANDLE_STMT,
                         d->dpDbc(),
                         &d->hStmt);
    if (r != SQL_SUCCESS) {
        qSqlWarning("QODBCResult::prepare: Unable to allocate statement handle"_L1, d);
        return false;
    }

    d->updateStmtHandleState();

    if (isForwardOnly()) {
        r = SQLSetStmtAttr(d->hStmt,
                            SQL_ATTR_CURSOR_TYPE,
                            (SQLPOINTER)SQL_CURSOR_FORWARD_ONLY,
                            SQL_IS_UINTEGER);
    } else {
        r = SQLSetStmtAttr(d->hStmt,
                            SQL_ATTR_CURSOR_TYPE,
                            (SQLPOINTER)SQL_CURSOR_STATIC,
                            SQL_IS_UINTEGER);
    }
    if (r != SQL_SUCCESS && r != SQL_SUCCESS_WITH_INFO) {
        setLastError(qMakeError(QCoreApplication::translate("QODBCResult",
            "QODBCResult::reset: Unable to set 'SQL_CURSOR_STATIC' as statement attribute. "
            "Please check your ODBC driver configuration"), QSqlError::StatementError, d));
        return false;
    }

    {
        auto encoded = toSQLTCHAR(query);
        r = SQLPrepare(d->hStmt,
                       encoded.data(),
                       SQLINTEGER(encoded.size()));
    }

    if (r != SQL_SUCCESS) {
        setLastError(qMakeError(QCoreApplication::translate("QODBCResult",
                     "Unable to prepare statement"), QSqlError::StatementError, d));
        return false;
    }
    return true;
}

bool QODBCResult::exec()
{
    Q_D(QODBCResult);
    setActive(false);
    setAt(QSql::BeforeFirstRow);
    d->rInf.clear();
    d->fieldCache.clear();
    d->fieldCacheIdx = 0;

    if (!d->hStmt) {
        qSqlWarning("QODBCResult::exec: No statement handle available"_L1, d);
        return false;
    }

    if (isSelect())
        SQLCloseCursor(d->hStmt);

    QVariantList &values = boundValues();
    QByteArrayList tmpStorage(values.count(), QByteArray()); // targets for SQLBindParameter()
    QVarLengthArray<SQLLEN, 32> indicators(values.count(), 0);

    // bind parameters - only positional binding allowed
    SQLRETURN r;
    for (qsizetype i = 0; i < values.count(); ++i) {
        if (bindValueType(i) & QSql::Out)
            values[i].detach();
        const QVariant &val = values.at(i);
        SQLLEN *ind = &indicators[i];
        if (QSqlResultPrivate::isVariantNull(val))
            *ind = SQL_NULL_DATA;
        switch (val.typeId()) {
            case QMetaType::QDate: {
                QByteArray &ba = tmpStorage[i];
                ba.resize(sizeof(DATE_STRUCT));
                DATE_STRUCT *dt = (DATE_STRUCT *)const_cast<char *>(ba.constData());
                QDate qdt = val.toDate();
                dt->year = qdt.year();
                dt->month = qdt.month();
                dt->day = qdt.day();
                r = SQLBindParameter(d->hStmt,
                                      i + 1,
                                      qParamType[bindValueType(i) & QSql::InOut],
                                      SQL_C_DATE,
                                      SQL_DATE,
                                      0,
                                      0,
                                      (void *) dt,
                                      0,
                                      *ind == SQL_NULL_DATA ? ind : NULL);
                break; }
            case QMetaType::QTime: {
                QByteArray &ba = tmpStorage[i];
                ba.resize(sizeof(TIME_STRUCT));
                TIME_STRUCT *dt = (TIME_STRUCT *)const_cast<char *>(ba.constData());
                QTime qdt = val.toTime();
                dt->hour = qdt.hour();
                dt->minute = qdt.minute();
                dt->second = qdt.second();
                r = SQLBindParameter(d->hStmt,
                                      i + 1,
                                      qParamType[bindValueType(i) & QSql::InOut],
                                      SQL_C_TIME,
                                      SQL_TIME,
                                      0,
                                      0,
                                      (void *) dt,
                                      0,
                                      *ind == SQL_NULL_DATA ? ind : NULL);
                break; }
            case QMetaType::QDateTime: {
                QByteArray &ba = tmpStorage[i];
                ba.resize(sizeof(TIMESTAMP_STRUCT));
                TIMESTAMP_STRUCT *dt = reinterpret_cast<TIMESTAMP_STRUCT *>(const_cast<char *>(ba.constData()));
                const QDateTime qdt = val.toDateTime();
                const QDate qdate = qdt.date();
                const QTime qtime = qdt.time();
                dt->year = qdate.year();
                dt->month = qdate.month();
                dt->day = qdate.day();
                dt->hour = qtime.hour();
                dt->minute = qtime.minute();
                dt->second = qtime.second();
                // (20 includes a separating period)
                const int precision = d->drv_d_func()->datetimePrecision - 20;
                if (precision <= 0) {
                    dt->fraction = 0;
                } else {
                    dt->fraction = qtime.msec() * 1000000;

                    // (How many leading digits do we want to keep?  With SQL Server 2005, this should be 3: 123000000)
                    int keep = (int)qPow(10.0, 9 - qMin(9, precision));
                    dt->fraction = (dt->fraction / keep) * keep;
                }

                r = SQLBindParameter(d->hStmt,
                                      i + 1,
                                      qParamType[bindValueType(i) & QSql::InOut],
                                      SQL_C_TIMESTAMP,
                                      SQL_TIMESTAMP,
                                      d->drv_d_func()->datetimePrecision,
                                      precision,
                                      (void *) dt,
                                      0,
                                      *ind == SQL_NULL_DATA ? ind : NULL);
                break; }
            case QMetaType::Int:
                r = SQLBindParameter(d->hStmt,
                                      i + 1,
                                      qParamType[bindValueType(i) & QSql::InOut],
                                      SQL_C_SLONG,
                                      SQL_INTEGER,
                                      0,
                                      0,
                                      const_cast<void *>(val.constData()),
                                      0,
                                      *ind == SQL_NULL_DATA ? ind : NULL);
                break;
            case QMetaType::UInt:
                r = SQLBindParameter(d->hStmt,
                                      i + 1,
                                      qParamType[bindValueType(i) & QSql::InOut],
                                      SQL_C_ULONG,
                                      SQL_NUMERIC,
                                      15,
                                      0,
                                      const_cast<void *>(val.constData()),
                                      0,
                                      *ind == SQL_NULL_DATA ? ind : NULL);
                break;
            case QMetaType::Short:
                r = SQLBindParameter(d->hStmt,
                                      i + 1,
                                      qParamType[bindValueType(i) & QSql::InOut],
                                      SQL_C_SSHORT,
                                      SQL_SMALLINT,
                                      0,
                                      0,
                                      const_cast<void *>(val.constData()),
                                      0,
                                      *ind == SQL_NULL_DATA ? ind : NULL);
                break;
            case QMetaType::UShort:
                r = SQLBindParameter(d->hStmt,
                                      i + 1,
                                      qParamType[bindValueType(i) & QSql::InOut],
                                      SQL_C_USHORT,
                                      SQL_NUMERIC,
                                      15,
                                      0,
                                      const_cast<void *>(val.constData()),
                                      0,
                                      *ind == SQL_NULL_DATA ? ind : NULL);
                break;
            case QMetaType::Double:
                r = SQLBindParameter(d->hStmt,
                                      i + 1,
                                      qParamType[bindValueType(i) & QSql::InOut],
                                      SQL_C_DOUBLE,
                                      SQL_DOUBLE,
                                      0,
                                      0,
                                      const_cast<void *>(val.constData()),
                                      0,
                                      *ind == SQL_NULL_DATA ? ind : NULL);
                break;
            case QMetaType::Float:
                r = SQLBindParameter(d->hStmt,
                                      i + 1,
                                      qParamType[bindValueType(i) & QSql::InOut],
                                      SQL_C_FLOAT,
                                      SQL_REAL,
                                      0,
                                      0,
                                      const_cast<void *>(val.constData()),
                                      0,
                                      *ind == SQL_NULL_DATA ? ind : NULL);
                break;
            case QMetaType::LongLong:
                r = SQLBindParameter(d->hStmt,
                                      i + 1,
                                      qParamType[bindValueType(i) & QSql::InOut],
                                      SQL_C_SBIGINT,
                                      SQL_BIGINT,
                                      0,
                                      0,
                                      const_cast<void *>(val.constData()),
                                      0,
                                      *ind == SQL_NULL_DATA ? ind : NULL);
                break;
            case QMetaType::ULongLong:
                r = SQLBindParameter(d->hStmt,
                                      i + 1,
                                      qParamType[bindValueType(i) & QSql::InOut],
                                      SQL_C_UBIGINT,
                                      SQL_BIGINT,
                                      0,
                                      0,
                                      const_cast<void *>(val.constData()),
                                      0,
                                      *ind == SQL_NULL_DATA ? ind : NULL);
                break;
            case QMetaType::QByteArray:
                if (*ind != SQL_NULL_DATA) {
                    *ind = val.toByteArray().size();
                }
                r = SQLBindParameter(d->hStmt,
                                      i + 1,
                                      qParamType[bindValueType(i) & QSql::InOut],
                                      SQL_C_BINARY,
                                      SQL_LONGVARBINARY,
                                      val.toByteArray().size(),
                                      0,
                                      const_cast<char *>(val.toByteArray().constData()),
                                      val.toByteArray().size(),
                                      ind);
                break;
            case QMetaType::Bool:
                r = SQLBindParameter(d->hStmt,
                                      i + 1,
                                      qParamType[bindValueType(i) & QSql::InOut],
                                      SQL_C_BIT,
                                      SQL_BIT,
                                      0,
                                      0,
                                      const_cast<void *>(val.constData()),
                                      0,
                                      *ind == SQL_NULL_DATA ? ind : NULL);
                break;
            case QMetaType::QString:
                if (d->unicode) {
                    QByteArray &ba = tmpStorage[i];
                    {
                        const auto encoded = toSQLTCHAR(val.toString());
                        ba = QByteArray(reinterpret_cast<const char *>(encoded.data()),
                                        encoded.size() * sizeof(SQLTCHAR));
                    }

                    if (*ind != SQL_NULL_DATA)
                        *ind = ba.size();

                    if (bindValueType(i) & QSql::Out) {
                        r = SQLBindParameter(d->hStmt,
                                            i + 1,
                                            qParamType[bindValueType(i) & QSql::InOut],
                                            SQL_C_TCHAR,
                                            ba.size() > 254 ? SQL_WLONGVARCHAR : SQL_WVARCHAR,
                                            0, // god knows... don't change this!
                                            0,
                                            const_cast<char *>(ba.constData()), // don't detach
                                            ba.size(),
                                            ind);
                        break;
                    }
                    r = SQLBindParameter(d->hStmt,
                                          i + 1,
                                          qParamType[bindValueType(i) & QSql::InOut],
                                          SQL_C_TCHAR,
                                          ba.size() > 254 ? SQL_WLONGVARCHAR : SQL_WVARCHAR,
                                          ba.size(),
                                          0,
                                          const_cast<char *>(ba.constData()), // don't detach
                                          ba.size(),
                                          ind);
                    break;
                }
                else
                {
                    QByteArray &str = tmpStorage[i];
                    str = val.toString().toUtf8();
                    if (*ind != SQL_NULL_DATA)
                        *ind = str.length();
                    int strSize = str.length();

                    r = SQLBindParameter(d->hStmt,
                                          i + 1,
                                          qParamType[bindValueType(i) & QSql::InOut],
                                          SQL_C_CHAR,
                                          strSize > 254 ? SQL_LONGVARCHAR : SQL_VARCHAR,
                                          strSize,
                                          0,
                                          const_cast<char *>(str.constData()),
                                          strSize,
                                          ind);
                    break;
                }
            Q_FALLTHROUGH();
            default: {
                QByteArray &ba = tmpStorage[i];
                if (*ind != SQL_NULL_DATA)
                    *ind = ba.size();
                r = SQLBindParameter(d->hStmt,
                                      i + 1,
                                      qParamType[bindValueType(i) & QSql::InOut],
                                      SQL_C_BINARY,
                                      SQL_VARBINARY,
                                      ba.length() + 1,
                                      0,
                                      const_cast<char *>(ba.constData()),
                                      ba.length() + 1,
                                      ind);
                break; }
        }
        if (r != SQL_SUCCESS) {
            qSqlWarning("QODBCResult::exec: unable to bind variable:"_L1, d);
            setLastError(qMakeError(QCoreApplication::translate("QODBCResult",
                         "Unable to bind variable"), QSqlError::StatementError, d));
            return false;
        }
    }
    r = SQLExecute(d->hStmt);
    if (r != SQL_SUCCESS && r != SQL_SUCCESS_WITH_INFO && r != SQL_NO_DATA) {
        qSqlWarning("QODBCResult::exec: Unable to execute statement:"_L1, d);
        setLastError(qMakeError(QCoreApplication::translate("QODBCResult",
                     "Unable to execute statement"), QSqlError::StatementError, d));
        return false;
    }

    SQLULEN isScrollable = 0;
    r = SQLGetStmtAttr(d->hStmt, SQL_ATTR_CURSOR_SCROLLABLE, &isScrollable, SQL_IS_INTEGER, 0);
    if (r == SQL_SUCCESS || r == SQL_SUCCESS_WITH_INFO)
        setForwardOnly(isScrollable == SQL_NONSCROLLABLE);

    SQLSMALLINT count = 0;
    SQLNumResultCols(d->hStmt, &count);
    if (count) {
        setSelect(true);
        for (SQLSMALLINT i = 0; i < count; ++i) {
            d->rInf.append(qMakeFieldInfo(d, i));
        }
        d->fieldCache.resize(count);
    } else {
        setSelect(false);
    }
    setActive(true);


    //get out parameters
    if (!hasOutValues())
        return true;

    for (qsizetype i = 0; i < values.count(); ++i) {
        switch (values.at(i).typeId()) {
            case QMetaType::QDate: {
                DATE_STRUCT ds = *((DATE_STRUCT *)const_cast<char *>(tmpStorage.at(i).constData()));
                values[i] = QVariant(QDate(ds.year, ds.month, ds.day));
                break; }
            case QMetaType::QTime: {
                TIME_STRUCT dt = *((TIME_STRUCT *)const_cast<char *>(tmpStorage.at(i).constData()));
                values[i] = QVariant(QTime(dt.hour, dt.minute, dt.second));
                break; }
            case QMetaType::QDateTime: {
                TIMESTAMP_STRUCT dt = *((TIMESTAMP_STRUCT*)
                                        const_cast<char *>(tmpStorage.at(i).constData()));
                values[i] = QVariant(QDateTime(QDate(dt.year, dt.month, dt.day),
                               QTime(dt.hour, dt.minute, dt.second, dt.fraction / 1000000)));
                break; }
            case QMetaType::Bool:
            case QMetaType::Short:
            case QMetaType::UShort:
            case QMetaType::Int:
            case QMetaType::UInt:
            case QMetaType::Float:
            case QMetaType::Double:
            case QMetaType::QByteArray:
            case QMetaType::LongLong:
            case QMetaType::ULongLong:
                //nothing to do
                break;
            case QMetaType::QString:
                if (d->unicode) {
                    if (bindValueType(i) & QSql::Out) {
                        const QByteArray &bytes = tmpStorage.at(i);
                        const auto strSize = bytes.size() / sizeof(SQLTCHAR);
                        QVarLengthArray<SQLTCHAR> string(strSize);
                        memcpy(string.data(), bytes.data(), strSize * sizeof(SQLTCHAR));
                        values[i] = fromSQLTCHAR(string);
                    }
                    break;
                }
                Q_FALLTHROUGH();
            default: {
                if (bindValueType(i) & QSql::Out)
                    values[i] = tmpStorage.at(i);
                break; }
        }
        if (indicators[i] == SQL_NULL_DATA)
            values[i] = QVariant(values[i].metaType());
    }
    return true;
}

QSqlRecord QODBCResult::record() const
{
    Q_D(const QODBCResult);
    if (!isActive() || !isSelect())
        return QSqlRecord();
    return d->rInf;
}

QVariant QODBCResult::lastInsertId() const
{
    Q_D(const QODBCResult);
    QString sql;

    switch (driver()->dbmsType()) {
    case QSqlDriver::MSSqlServer:
    case QSqlDriver::Sybase:
        sql = "SELECT @@IDENTITY;"_L1;
        break;
    case QSqlDriver::MySqlServer:
        sql = "SELECT LAST_INSERT_ID();"_L1;
        break;
    case QSqlDriver::PostgreSQL:
        sql = "SELECT lastval();"_L1;
        break;
    default:
        break;
    }

    if (!sql.isEmpty()) {
        QSqlQuery qry(driver()->createResult());
        if (qry.exec(sql) && qry.next())
            return qry.value(0);

        qSqlWarning("QODBCResult::lastInsertId: Unable to get lastInsertId"_L1, d);
    } else {
        qSqlWarning("QODBCResult::lastInsertId: not implemented for this DBMS"_L1, d);
    }

    return QVariant();
}

QVariant QODBCResult::handle() const
{
    Q_D(const QODBCResult);
    return QVariant(QMetaType::fromType<SQLHANDLE>(), &d->hStmt);
}

bool QODBCResult::nextResult()
{
    Q_D(QODBCResult);
    setActive(false);
    setAt(QSql::BeforeFirstRow);
    d->rInf.clear();
    d->fieldCache.clear();
    d->fieldCacheIdx = 0;
    setSelect(false);

    SQLRETURN r = SQLMoreResults(d->hStmt);
    if (r != SQL_SUCCESS) {
        if (r == SQL_SUCCESS_WITH_INFO) {
            QString message = errorStringFromDiagRecords(qODBCWarn(d));
            qWarning() << "QODBCResult::nextResult():" << message;
        } else {
            if (r != SQL_NO_DATA)
                setLastError(qMakeError(QCoreApplication::translate("QODBCResult",
                    "Unable to fetch last"), QSqlError::ConnectionError, d));
            return false;
        }
    }

    SQLSMALLINT count = 0;
    SQLNumResultCols(d->hStmt, &count);
    if (count) {
        setSelect(true);
        for (SQLSMALLINT i = 0; i < count; ++i) {
            d->rInf.append(qMakeFieldInfo(d, i));
        }
        d->fieldCache.resize(count);
    } else {
        setSelect(false);
    }
    setActive(true);

    return true;
}

void QODBCResult::virtual_hook(int id, void *data)
{
    QSqlResult::virtual_hook(id, data);
}

void QODBCResult::detachFromResultSet()
{
    Q_D(QODBCResult);
    if (d->hStmt)
        SQLCloseCursor(d->hStmt);
}

////////////////////////////////////////


QODBCDriver::QODBCDriver(QObject *parent)
    : QSqlDriver(*new QODBCDriverPrivate, parent)
{
}

QODBCDriver::QODBCDriver(SQLHANDLE env, SQLHANDLE con, QObject *parent)
    : QSqlDriver(*new QODBCDriverPrivate, parent)
{
    Q_D(QODBCDriver);
    d->hEnv = env;
    d->hDbc = con;
    if (env && con) {
        setOpen(true);
        setOpenError(false);
    }
}

QODBCDriver::~QODBCDriver()
{
    cleanup();
}

bool QODBCDriver::hasFeature(DriverFeature f) const
{
    Q_D(const QODBCDriver);
    switch (f) {
    case Transactions: {
        if (!d->hDbc)
            return false;
        SQLUSMALLINT txn;
        SQLSMALLINT t;
        int r = SQLGetInfo(d->hDbc,
                        (SQLUSMALLINT)SQL_TXN_CAPABLE,
                        &txn,
                        sizeof(txn),
                        &t);
        if (r != SQL_SUCCESS || txn == SQL_TC_NONE)
            return false;
        else
            return true;
    }
    case Unicode:
        return d->unicode;
    case PreparedQueries:
    case PositionalPlaceholders:
    case FinishQuery:
    case LowPrecisionNumbers:
        return true;
    case QuerySize:
    case NamedPlaceholders:
    case BatchOperations:
    case SimpleLocking:
    case EventNotifications:
    case CancelQuery:
        return false;
    case LastInsertId:
        return (d->dbmsType == MSSqlServer)
                || (d->dbmsType == Sybase)
                || (d->dbmsType == MySqlServer)
                || (d->dbmsType == PostgreSQL);
    case MultipleResultSets:
        return d->hasMultiResultSets;
    case BLOB: {
        if (d->dbmsType == MySqlServer)
            return true;
        else
            return false;
    }
    }
    return false;
}

bool QODBCDriver::open(const QString & db,
                        const QString & user,
                        const QString & password,
                        const QString &,
                        int,
                        const QString& connOpts)
{
    Q_D(QODBCDriver);
    if (isOpen())
      close();
    SQLRETURN r;
    r = SQLAllocHandle(SQL_HANDLE_ENV,
                        SQL_NULL_HANDLE,
                        &d->hEnv);
    if (r != SQL_SUCCESS && r != SQL_SUCCESS_WITH_INFO) {
        qSqlWarning("QODBCDriver::open: Unable to allocate environment"_L1, d);
        setOpenError(true);
        return false;
    }
    r = SQLSetEnvAttr(d->hEnv,
                       SQL_ATTR_ODBC_VERSION,
                       (SQLPOINTER)qGetODBCVersion(connOpts),
                       SQL_IS_UINTEGER);
    r = SQLAllocHandle(SQL_HANDLE_DBC,
                        d->hEnv,
                        &d->hDbc);
    if (r != SQL_SUCCESS && r != SQL_SUCCESS_WITH_INFO) {
        qSqlWarning("QODBCDriver::open: Unable to allocate connection"_L1, d);
        setOpenError(true);
        cleanup();
        return false;
    }

    if (!d->setConnectionOptions(connOpts)) {
        cleanup();
        return false;
    }

    // Create the connection string
    QString connQStr;
    // support the "DRIVER={SQL SERVER};SERVER=blah" syntax
    if (db.contains(".dsn"_L1, Qt::CaseInsensitive))
        connQStr = "FILEDSN="_L1 + db;
    else if (db.contains("DRIVER="_L1, Qt::CaseInsensitive)
            || db.contains("SERVER="_L1, Qt::CaseInsensitive))
        connQStr = db;
    else
        connQStr = "DSN="_L1 + db;

    if (!user.isEmpty())
        connQStr += ";UID="_L1 + user;
    if (!password.isEmpty())
        connQStr += ";PWD="_L1 + password;

    SQLSMALLINT cb;
    QVarLengthArray<SQLTCHAR, 1024> connOut(1024);
    {
        auto encoded = toSQLTCHAR(connQStr);
        r = SQLDriverConnect(d->hDbc,
                             nullptr,
                             encoded.data(), SQLSMALLINT(encoded.size()),
                             connOut.data(), SQLSMALLINT(connOut.size()),
                             &cb,
                             /*SQL_DRIVER_NOPROMPT*/0);
    }

    if (r != SQL_SUCCESS && r != SQL_SUCCESS_WITH_INFO) {
        setLastError(qMakeError(tr("Unable to connect"), QSqlError::ConnectionError, d));
        setOpenError(true);
        cleanup();
        return false;
    }

    if (!d->checkDriver()) {
        setLastError(qMakeError(tr("Unable to connect - Driver doesn't support all "
                     "functionality required"), QSqlError::ConnectionError, d));
        setOpenError(true);
        cleanup();
        return false;
    }

    d->checkUnicode();
    d->checkSchemaUsage();
    d->checkDBMS();
    d->checkHasSQLFetchScroll();
    d->checkHasMultiResults();
    d->checkDateTimePrecision();
    setOpen(true);
    setOpenError(false);
    if (d->dbmsType == MSSqlServer) {
        QSqlQuery i(createResult());
        i.exec("SET QUOTED_IDENTIFIER ON"_L1);
    }
    return true;
}

void QODBCDriver::close()
{
    cleanup();
    setOpen(false);
    setOpenError(false);
}

void QODBCDriver::cleanup()
{
    Q_D(QODBCDriver);
    SQLRETURN r;

    if (d->hDbc) {
        // Open statements/descriptors handles are automatically cleaned up by SQLDisconnect
        if (isOpen()) {
            r = SQLDisconnect(d->hDbc);
            if (r != SQL_SUCCESS)
                qSqlWarning("QODBCDriver::disconnect: Unable to disconnect datasource"_L1, d);
            else
                d->disconnectCount++;
        }

        r = SQLFreeHandle(SQL_HANDLE_DBC, d->hDbc);
        if (r != SQL_SUCCESS)
            qSqlWarning("QODBCDriver::cleanup: Unable to free connection handle"_L1, d);
        d->hDbc = 0;
    }

    if (d->hEnv) {
        r = SQLFreeHandle(SQL_HANDLE_ENV, d->hEnv);
        if (r != SQL_SUCCESS)
            qSqlWarning("QODBCDriver::cleanup: Unable to free environment handle"_L1, d);
        d->hEnv = 0;
    }
}

// checks whether the server can return char, varchar and longvarchar
// as two byte unicode characters
void QODBCDriverPrivate::checkUnicode()
{
    SQLRETURN   r;
    SQLUINTEGER fFunc;

    unicode = false;
    r = SQLGetInfo(hDbc,
                    SQL_CONVERT_CHAR,
                    (SQLPOINTER)&fFunc,
                    sizeof(fFunc),
                    NULL);
    if ((r == SQL_SUCCESS || r == SQL_SUCCESS_WITH_INFO) && (fFunc & SQL_CVT_WCHAR)) {
        unicode = true;
        return;
    }

    r = SQLGetInfo(hDbc,
                    SQL_CONVERT_VARCHAR,
                    (SQLPOINTER)&fFunc,
                    sizeof(fFunc),
                    NULL);
    if ((r == SQL_SUCCESS || r == SQL_SUCCESS_WITH_INFO) && (fFunc & SQL_CVT_WVARCHAR)) {
        unicode = true;
        return;
    }

    r = SQLGetInfo(hDbc,
                    SQL_CONVERT_LONGVARCHAR,
                    (SQLPOINTER)&fFunc,
                    sizeof(fFunc),
                    NULL);
    if ((r == SQL_SUCCESS || r == SQL_SUCCESS_WITH_INFO) && (fFunc & SQL_CVT_WLONGVARCHAR)) {
        unicode = true;
        return;
    }
    SQLHANDLE hStmt;
    r = SQLAllocHandle(SQL_HANDLE_STMT,
                                  hDbc,
                                  &hStmt);

    // for databases which do not return something useful in SQLGetInfo and are picky about a
    // 'SELECT' statement without 'FROM' but support VALUE(foo) statement like e.g. DB2 or Oracle
    const auto statements = {
        "select 'test'"_L1,
        "values('test')"_L1,
        "select 'test' from dual"_L1,
    };
    for (const auto &statement : statements) {
        auto encoded = toSQLTCHAR(statement);
        r = SQLExecDirect(hStmt, encoded.data(), SQLINTEGER(encoded.size()));
        if (r == SQL_SUCCESS)
            break;
    }
    if (r == SQL_SUCCESS) {
        r = SQLFetch(hStmt);
        if (r == SQL_SUCCESS) {
            QVarLengthArray<SQLWCHAR, 10> buffer(10);
            r = SQLGetData(hStmt, 1, SQL_C_WCHAR, buffer.data(), buffer.size() * sizeof(SQLWCHAR), NULL);
            if (r == SQL_SUCCESS && fromSQLTCHAR(buffer) == "test"_L1) {
                unicode = true;
            }
        }
    }
    r = SQLFreeHandle(SQL_HANDLE_STMT, hStmt);
}

bool QODBCDriverPrivate::checkDriver() const
{
#ifdef ODBC_CHECK_DRIVER
    static constexpr SQLUSMALLINT reqFunc[] = {
                SQL_API_SQLDESCRIBECOL, SQL_API_SQLGETDATA, SQL_API_SQLCOLUMNS,
                SQL_API_SQLGETSTMTATTR, SQL_API_SQLGETDIAGREC, SQL_API_SQLEXECDIRECT,
                SQL_API_SQLGETINFO, SQL_API_SQLTABLES
    };

    // these functions are optional
    static constexpr SQLUSMALLINT optFunc[] = {
        SQL_API_SQLNUMRESULTCOLS, SQL_API_SQLROWCOUNT
    };

    SQLRETURN r;
    SQLUSMALLINT sup;

    // check the required functions
    for (const SQLUSMALLINT func : reqFunc) {

        r = SQLGetFunctions(hDbc, func, &sup);

        if (r != SQL_SUCCESS) {
            qSqlWarning("QODBCDriver::checkDriver: Cannot get list of supported functions"_L1, this);
            return false;
        }
        if (sup == SQL_FALSE) {
            qWarning () << "QODBCDriver::open: Warning - Driver doesn't support all needed functionality ("
                        << func
                        << ").\nPlease look at the Qt SQL Module Driver documentation for more information.";
            return false;
        }
    }

    // these functions are optional and just generate a warning
    for (const SQLUSMALLINT func : optFunc) {

        r = SQLGetFunctions(hDbc, func, &sup);

        if (r != SQL_SUCCESS) {
            qSqlWarning("QODBCDriver::checkDriver: Cannot get list of supported functions"_L1, this);
            return false;
        }
        if (sup == SQL_FALSE) {
            qWarning() << "QODBCDriver::checkDriver: Warning - Driver doesn't support some non-critical functions ("
                       << func << ')';
            return true;
        }
    }
#endif //ODBC_CHECK_DRIVER

    return true;
}

void QODBCDriverPrivate::checkSchemaUsage()
{
    SQLRETURN   r;
    SQLUINTEGER val;

    r = SQLGetInfo(hDbc,
                   SQL_SCHEMA_USAGE,
                   (SQLPOINTER) &val,
                   sizeof(val),
                   NULL);
    if (r == SQL_SUCCESS || r == SQL_SUCCESS_WITH_INFO)
        useSchema = (val != 0);
}

void QODBCDriverPrivate::checkDBMS()
{
    SQLRETURN   r;
    QVarLengthArray<SQLTCHAR, 200> serverString(200);
    SQLSMALLINT t;

    r = SQLGetInfo(hDbc,
                   SQL_DBMS_NAME,
                   serverString.data(),
                   SQLSMALLINT(serverString.size() * sizeof(SQLTCHAR)),
                   &t);
    if (r == SQL_SUCCESS || r == SQL_SUCCESS_WITH_INFO) {
        const QString serverType = fromSQLTCHAR(serverString, t / sizeof(SQLTCHAR));
        if (serverType.contains("PostgreSQL"_L1, Qt::CaseInsensitive))
            dbmsType = QSqlDriver::PostgreSQL;
        else if (serverType.contains("Oracle"_L1, Qt::CaseInsensitive))
            dbmsType = QSqlDriver::Oracle;
        else if (serverType.contains("MySql"_L1, Qt::CaseInsensitive))
            dbmsType = QSqlDriver::MySqlServer;
        else if (serverType.contains("Microsoft SQL Server"_L1, Qt::CaseInsensitive))
            dbmsType = QSqlDriver::MSSqlServer;
        else if (serverType.contains("Sybase"_L1, Qt::CaseInsensitive))
            dbmsType = QSqlDriver::Sybase;
    }
    r = SQLGetInfo(hDbc,
                   SQL_DRIVER_NAME,
                   serverString.data(),
                   SQLSMALLINT(serverString.size() * sizeof(SQLTCHAR)),
                   &t);
    if (r == SQL_SUCCESS || r == SQL_SUCCESS_WITH_INFO) {
        const QString serverType = fromSQLTCHAR(serverString, t / sizeof(SQLTCHAR));
        isFreeTDSDriver = serverType.contains("tdsodbc"_L1, Qt::CaseInsensitive);
        unicode = unicode && !isFreeTDSDriver;
    }
}

void QODBCDriverPrivate::checkHasSQLFetchScroll()
{
    SQLUSMALLINT sup;
    SQLRETURN r = SQLGetFunctions(hDbc, SQL_API_SQLFETCHSCROLL, &sup);
    if ((r != SQL_SUCCESS && r != SQL_SUCCESS_WITH_INFO) || sup != SQL_TRUE) {
        hasSQLFetchScroll = false;
        qWarning("QODBCDriver::checkHasSQLFetchScroll: Warning - Driver doesn't support scrollable result sets, use forward only mode for queries");
    }
}

void QODBCDriverPrivate::checkHasMultiResults()
{
    QVarLengthArray<SQLTCHAR, 2> driverResponse(2);
    SQLSMALLINT length;
    SQLRETURN r = SQLGetInfo(hDbc,
                             SQL_MULT_RESULT_SETS,
                             driverResponse.data(),
                             SQLSMALLINT(driverResponse.size() * sizeof(SQLTCHAR)),
                             &length);
    if (r == SQL_SUCCESS || r == SQL_SUCCESS_WITH_INFO)
        hasMultiResultSets = fromSQLTCHAR(driverResponse, length / sizeof(SQLTCHAR)).startsWith(u'Y');
}

void QODBCDriverPrivate::checkDateTimePrecision()
{
    SQLINTEGER columnSize;
    SQLHANDLE hStmt;

    SQLRETURN r = SQLAllocHandle(SQL_HANDLE_STMT, hDbc, &hStmt);
    if (r != SQL_SUCCESS) {
        return;
    }

    r = SQLGetTypeInfo(hStmt, SQL_TIMESTAMP);
    if (r == SQL_SUCCESS || r == SQL_SUCCESS_WITH_INFO) {
        r = SQLFetch(hStmt);
        if ( r == SQL_SUCCESS || r == SQL_SUCCESS_WITH_INFO )
        {
            if (SQLGetData(hStmt, 3, SQL_INTEGER, &columnSize, sizeof(columnSize), 0) == SQL_SUCCESS) {
                datetimePrecision = (int)columnSize;
            }
        }
    }
    SQLFreeHandle(SQL_HANDLE_STMT, hStmt);
}

QSqlResult *QODBCDriver::createResult() const
{
    return new QODBCResult(this);
}

bool QODBCDriver::beginTransaction()
{
    Q_D(QODBCDriver);
    if (!isOpen()) {
        qWarning("QODBCDriver::beginTransaction: Database not open");
        return false;
    }
    SQLUINTEGER ac(SQL_AUTOCOMMIT_OFF);
    SQLRETURN r  = SQLSetConnectAttr(d->hDbc,
                                      SQL_ATTR_AUTOCOMMIT,
                                      (SQLPOINTER)size_t(ac),
                                      sizeof(ac));
    if (r != SQL_SUCCESS) {
        setLastError(qMakeError(tr("Unable to disable autocommit"),
                     QSqlError::TransactionError, d));
        return false;
    }
    return true;
}

bool QODBCDriver::commitTransaction()
{
    Q_D(QODBCDriver);
    if (!isOpen()) {
        qWarning("QODBCDriver::commitTransaction: Database not open");
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
    return endTrans();
}

bool QODBCDriver::rollbackTransaction()
{
    Q_D(QODBCDriver);
    if (!isOpen()) {
        qWarning("QODBCDriver::rollbackTransaction: Database not open");
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
    return endTrans();
}

bool QODBCDriver::endTrans()
{
    Q_D(QODBCDriver);
    SQLUINTEGER ac(SQL_AUTOCOMMIT_ON);
    SQLRETURN r  = SQLSetConnectAttr(d->hDbc,
                                      SQL_ATTR_AUTOCOMMIT,
                                      (SQLPOINTER)size_t(ac),
                                      sizeof(ac));
    if (r != SQL_SUCCESS) {
        setLastError(qMakeError(tr("Unable to enable autocommit"), QSqlError::TransactionError, d));
        return false;
    }
    return true;
}

QStringList QODBCDriver::tables(QSql::TableType type) const
{
    Q_D(const QODBCDriver);
    QStringList tl;
    if (!isOpen())
        return tl;
    SQLHANDLE hStmt;

    SQLRETURN r = SQLAllocHandle(SQL_HANDLE_STMT,
                                  d->hDbc,
                                  &hStmt);
    if (r != SQL_SUCCESS) {
        qSqlWarning("QODBCDriver::tables: Unable to allocate handle"_L1, d);
        return tl;
    }
    r = SQLSetStmtAttr(hStmt,
                        SQL_ATTR_CURSOR_TYPE,
                        (SQLPOINTER)SQL_CURSOR_FORWARD_ONLY,
                        SQL_IS_UINTEGER);
    QStringList tableType;
    if (type & QSql::Tables)
        tableType += "TABLE"_L1;
    if (type & QSql::Views)
        tableType += "VIEW"_L1;
    if (type & QSql::SystemTables)
        tableType += "SYSTEM TABLE"_L1;
    if (tableType.isEmpty())
        return tl;

    {
        auto joinedTableTypeString = toSQLTCHAR(tableType.join(u','));

        r = SQLTables(hStmt,
                      nullptr, 0,
                      nullptr, 0,
                      nullptr, 0,
                      joinedTableTypeString.data(), joinedTableTypeString.size());
    }

    if (r != SQL_SUCCESS)
        qSqlWarning("QODBCDriver::tables Unable to execute table list"_L1, d);

    if (d->hasSQLFetchScroll)
        r = SQLFetchScroll(hStmt,
                           SQL_FETCH_NEXT,
                           0);
    else
        r = SQLFetch(hStmt);

    if (r != SQL_SUCCESS && r != SQL_SUCCESS_WITH_INFO && r != SQL_NO_DATA) {
        qSqlWarning("QODBCDriver::tables failed to retrieve table/view list: ("_L1
                            + QString::number(r) + u':',
                    hStmt);
        return QStringList();
    }

    while (r == SQL_SUCCESS) {
        tl.append(qGetStringData(hStmt, 2, -1, d->unicode).toString());

        if (d->hasSQLFetchScroll)
            r = SQLFetchScroll(hStmt,
                               SQL_FETCH_NEXT,
                               0);
        else
            r = SQLFetch(hStmt);
    }

    r = SQLFreeHandle(SQL_HANDLE_STMT, hStmt);
    if (r!= SQL_SUCCESS)
        qSqlWarning("QODBCDriver: Unable to free statement handle"_L1 + QString::number(r), d);
    return tl;
}

QSqlIndex QODBCDriver::primaryIndex(const QString& tablename) const
{
    Q_D(const QODBCDriver);
    QSqlIndex index(tablename);
    if (!isOpen())
        return index;
    bool usingSpecialColumns = false;
    QSqlRecord rec = record(tablename);

    SQLHANDLE hStmt;
    SQLRETURN r = SQLAllocHandle(SQL_HANDLE_STMT,
                                  d->hDbc,
                                  &hStmt);
    if (r != SQL_SUCCESS) {
        qSqlWarning("QODBCDriver::primaryIndex: Unable to list primary key"_L1, d);
        return index;
    }
    QString catalog, schema, table;
    d->splitTableQualifier(tablename, catalog, schema, table);

    if (isIdentifierEscaped(catalog, QSqlDriver::TableName))
        catalog = stripDelimiters(catalog, QSqlDriver::TableName);
    else
        catalog = d->adjustCase(catalog);

    if (isIdentifierEscaped(schema, QSqlDriver::TableName))
        schema = stripDelimiters(schema, QSqlDriver::TableName);
    else
        schema = d->adjustCase(schema);

    if (isIdentifierEscaped(table, QSqlDriver::TableName))
        table = stripDelimiters(table, QSqlDriver::TableName);
    else
        table = d->adjustCase(table);

    r = SQLSetStmtAttr(hStmt,
                        SQL_ATTR_CURSOR_TYPE,
                        (SQLPOINTER)SQL_CURSOR_FORWARD_ONLY,
                        SQL_IS_UINTEGER);
    {
        auto c = toSQLTCHAR(catalog);
        auto s = toSQLTCHAR(schema);
        auto t = toSQLTCHAR(table);
        r = SQLPrimaryKeys(hStmt,
                           catalog.isEmpty() ? nullptr : c.data(), c.size(),
                           schema.isEmpty()  ? nullptr : s.data(), s.size(),
                           t.data(), t.size());
    }

    // if the SQLPrimaryKeys() call does not succeed (e.g the driver
    // does not support it) - try an alternative method to get hold of
    // the primary index (e.g MS Access and FoxPro)
    if (r != SQL_SUCCESS) {
        auto c = toSQLTCHAR(catalog);
        auto s = toSQLTCHAR(schema);
        auto t = toSQLTCHAR(table);
        r = SQLSpecialColumns(hStmt,
                              SQL_BEST_ROWID,
                              catalog.isEmpty() ? nullptr : c.data(), c.size(),
                              schema.isEmpty()  ? nullptr : s.data(), s.size(),
                              t.data(), t.size(),
                              SQL_SCOPE_CURROW,
                              SQL_NULLABLE);

            if (r != SQL_SUCCESS) {
                qSqlWarning("QODBCDriver::primaryIndex: Unable to execute primary key list"_L1, d);
            } else {
                usingSpecialColumns = true;
            }
    }

    if (d->hasSQLFetchScroll)
        r = SQLFetchScroll(hStmt,
                           SQL_FETCH_NEXT,
                           0);
    else
        r = SQLFetch(hStmt);

    int fakeId = 0;
    QString cName, idxName;
    // Store all fields in a StringList because some drivers can't detail fields in this FETCH loop
    while (r == SQL_SUCCESS) {
        if (usingSpecialColumns) {
            cName = qGetStringData(hStmt, 1, -1, d->unicode).toString(); // column name
            idxName = QString::number(fakeId++); // invent a fake index name
        } else {
            cName = qGetStringData(hStmt, 3, -1, d->unicode).toString(); // column name
            idxName = qGetStringData(hStmt, 5, -1, d->unicode).toString(); // pk index name
        }
        index.append(rec.field(cName));
        index.setName(idxName);

        if (d->hasSQLFetchScroll)
            r = SQLFetchScroll(hStmt,
                               SQL_FETCH_NEXT,
                               0);
        else
            r = SQLFetch(hStmt);

    }
    r = SQLFreeHandle(SQL_HANDLE_STMT, hStmt);
    if (r!= SQL_SUCCESS)
        qSqlWarning("QODBCDriver: Unable to free statement handle"_L1 + QString::number(r), d);
    return index;
}

QSqlRecord QODBCDriver::record(const QString& tablename) const
{
    Q_D(const QODBCDriver);
    QSqlRecord fil;
    if (!isOpen())
        return fil;

    SQLHANDLE hStmt;
    QString catalog, schema, table;
    d->splitTableQualifier(tablename, catalog, schema, table);

    if (isIdentifierEscaped(catalog, QSqlDriver::TableName))
        catalog = stripDelimiters(catalog, QSqlDriver::TableName);
    else
        catalog = d->adjustCase(catalog);

    if (isIdentifierEscaped(schema, QSqlDriver::TableName))
        schema = stripDelimiters(schema, QSqlDriver::TableName);
    else
        schema = d->adjustCase(schema);

    if (isIdentifierEscaped(table, QSqlDriver::TableName))
        table = stripDelimiters(table, QSqlDriver::TableName);
    else
        table = d->adjustCase(table);

    SQLRETURN r = SQLAllocHandle(SQL_HANDLE_STMT,
                                  d->hDbc,
                                  &hStmt);
    if (r != SQL_SUCCESS) {
        qSqlWarning("QODBCDriver::record: Unable to allocate handle"_L1, d);
        return fil;
    }
    r = SQLSetStmtAttr(hStmt,
                        SQL_ATTR_CURSOR_TYPE,
                        (SQLPOINTER)SQL_CURSOR_FORWARD_ONLY,
                        SQL_IS_UINTEGER);
    {
        auto c = toSQLTCHAR(catalog);
        auto s = toSQLTCHAR(schema);
        auto t = toSQLTCHAR(table);
        r =  SQLColumns(hStmt,
                        catalog.isEmpty() ? nullptr : c.data(), c.size(),
                        schema.isEmpty()  ? nullptr : s.data(), s.size(),
                        t.data(), t.size(),
                        nullptr,
                        0);
    }
    if (r != SQL_SUCCESS)
        qSqlWarning("QODBCDriver::record: Unable to execute column list"_L1, d);

    if (d->hasSQLFetchScroll)
        r = SQLFetchScroll(hStmt,
                           SQL_FETCH_NEXT,
                           0);
    else
        r = SQLFetch(hStmt);

    // Store all fields in a StringList because some drivers can't detail fields in this FETCH loop
    while (r == SQL_SUCCESS) {

        fil.append(qMakeFieldInfo(hStmt, d));

        if (d->hasSQLFetchScroll)
            r = SQLFetchScroll(hStmt,
                               SQL_FETCH_NEXT,
                               0);
        else
            r = SQLFetch(hStmt);
    }

    r = SQLFreeHandle(SQL_HANDLE_STMT, hStmt);
    if (r!= SQL_SUCCESS)
        qSqlWarning("QODBCDriver: Unable to free statement handle "_L1 + QString::number(r), d);

    return fil;
}

QString QODBCDriver::formatValue(const QSqlField &field,
                                 bool trimStrings) const
{
    QString r;
    if (field.isNull()) {
        r = "NULL"_L1;
    } else if (field.metaType().id() == QMetaType::QDateTime) {
        // Use an escape sequence for the datetime fields
        if (field.value().toDateTime().isValid()){
            QDate dt = field.value().toDateTime().date();
            QTime tm = field.value().toDateTime().time();
            // Dateformat has to be "yyyy-MM-dd hh:mm:ss", with leading zeroes if month or day < 10
            r = "{ ts '"_L1 +
                QString::number(dt.year()) + u'-' +
                QString::number(dt.month()).rightJustified(2, u'0', true) +
                u'-' +
                QString::number(dt.day()).rightJustified(2, u'0', true) +
                u' ' +
                tm.toString() +
                "' }"_L1;
        } else
            r = "NULL"_L1;
    } else if (field.metaType().id() == QMetaType::QByteArray) {
        const QByteArray ba = field.value().toByteArray();
        r.reserve((ba.size() + 1) * 2);
        r = "0x"_L1;
        for (const char c : ba) {
            const uchar s = uchar(c);
            r += QLatin1Char(QtMiscUtils::toHexLower(s >> 4));
            r += QLatin1Char(QtMiscUtils::toHexLower(s & 0x0f));
        }
    } else {
        r = QSqlDriver::formatValue(field, trimStrings);
    }
    return r;
}

QVariant QODBCDriver::handle() const
{
    Q_D(const QODBCDriver);
    return QVariant(QMetaType::fromType<SQLHANDLE>(), &d->hDbc);
}

QString QODBCDriver::escapeIdentifier(const QString &identifier, IdentifierType) const
{
    Q_D(const QODBCDriver);
    QChar quote = const_cast<QODBCDriverPrivate*>(d)->quoteChar();
    QString res = identifier;
    if (!identifier.isEmpty() && !identifier.startsWith(quote) && !identifier.endsWith(quote) ) {
        const QString quoteStr(quote);
        res.replace(quote, quoteStr + quoteStr);
        res.replace(u'.', quoteStr + u'.' + quoteStr);
        res = quote + res + quote;
    }
    return res;
}

bool QODBCDriver::isIdentifierEscaped(const QString &identifier, IdentifierType) const
{
    Q_D(const QODBCDriver);
    QChar quote = const_cast<QODBCDriverPrivate*>(d)->quoteChar();
    return identifier.size() > 2
        && identifier.startsWith(quote) //left delimited
        && identifier.endsWith(quote); //right delimited
}

QT_END_NAMESPACE

#include "moc_qsql_odbc_p.cpp"
