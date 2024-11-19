// Copyright (C) 2022 The Qt Company Ltd.
// Copyright (C) 2022 Mimer Information Technology
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only
#include <qcoreapplication.h>
#include <qvariant.h>
#include <qmetatype.h>
#include <qdatetime.h>
#include <qloggingcategory.h>
#include <qsqlerror.h>
#include <qsqlfield.h>
#include <qsqlindex.h>
#include <qsqlrecord.h>
#include <qsqlquery.h>
#include <qsocketnotifier.h>
#include <qstringlist.h>
#include <qlocale.h>
#if defined(Q_OS_WIN32)
#    include <QtCore/qt_windows.h>
#endif
#include <QtSql/private/qsqlresult_p.h>
#include <QtSql/private/qsqldriver_p.h>
#include "qsql_mimer.h"

#define MIMER_DEFAULT_DATATYPE 1000

Q_DECLARE_OPAQUE_POINTER(MimerSession)
Q_DECLARE_METATYPE(MimerSession)

Q_DECLARE_OPAQUE_POINTER(MimerStatement)
Q_DECLARE_METATYPE(MimerStatement)

QT_BEGIN_NAMESPACE

Q_STATIC_LOGGING_CATEGORY(lcMimer, "qt.sql.mimer")

enum class MimerColumnTypes {
    Binary,
    Clob,
    Blob,
    String,
    Int,
    Numeric,
    Long,
    Float,
    Double,
    Boolean,
    Uuid,
    Date,
    Time,
    Timestamp,
    Unknown
};

using namespace Qt::StringLiterals;

class QMimerSQLResultPrivate;

class QMimerSQLResult final : public QSqlResult
{
    Q_DECLARE_PRIVATE(QMimerSQLResult)
public:
    QMimerSQLResult(const QMimerSQLDriver *db);
    virtual ~QMimerSQLResult() override;
    QVariant handle() const override;
    static constexpr int genericError = -1;
    static constexpr int lobChunkMaxSizeSet = 1048500;
    static constexpr int lobChunkMaxSizeFetch = 65536;
    static constexpr int maxStackStringSize = 200;
    static constexpr int maxTimeStringSize = 18;
    static constexpr int maxDateStringSize = 10;
    static constexpr int maxTimestampStringSize = 29;

private:
    void cleanup();
    bool fetch(int i) override;
    bool fetchFirst() override;
    bool fetchLast() override;
    bool fetchNext() override;
    QVariant data(int i) override;
    bool isNull(int index) override;
    bool reset(const QString &query) override;
    int size() override;
    int numRowsAffected() override;
    QSqlRecord record() const override;
    bool prepare(const QString &query) override;
    bool execBatch(bool arrayBind = false) override;
    bool exec() override;
    qint64 currentRow();
    QVariant lastInsertId() const override;
};

class QMimerSQLDriverPrivate final : public QSqlDriverPrivate
{
    Q_DECLARE_PUBLIC(QMimerSQLDriver)
public:
    QMimerSQLDriverPrivate() : QSqlDriverPrivate(QSqlDriver::MimerSQL), sessionhandle(nullptr) { }
    MimerSession sessionhandle;
    QString dbName;
    QString dbUser;
    void splitTableQualifier(const QString &qualifier, QString *schema, QString *table) const;
};

class QMimerSQLResultPrivate : public QSqlResultPrivate
{
    Q_DECLARE_PUBLIC(QMimerSQLResult)
public:
    Q_DECLARE_SQLDRIVER_PRIVATE(QMimerSQLDriver)
    QMimerSQLResultPrivate(QMimerSQLResult *q, const QMimerSQLDriver *drv)
        : QSqlResultPrivate(q, drv),
          statementhandle(nullptr),
          lobhandle(nullptr),
          rowsAffected(0),
          preparedQuery(false),
          openCursor(false),
          openStatement(false),
          executedStatement(false),
          callWithOut(false),
          execBatch(false),
          currentRow(QSql::BeforeFirstRow)
    {
    }
    MimerStatement statementhandle;
    MimerLob lobhandle;
    int rowsAffected;
    bool preparedQuery;
    bool openCursor;
    bool openStatement;
    bool executedStatement;
    bool callWithOut;
    bool execBatch;
    qint64 currentSize = -1;
    qint64 currentRow; // Only used when forwardOnly()
    QVector<QVariant> batch_vector;
};

static QSqlError qMakeError(const QString &err, const int errCode, QSqlError::ErrorType type,
                            const QMimerSQLDriverPrivate *p)
{
    QString msg;
    if (p) {
        size_t str_len;
        int e_code;
        int rc;
        str_len = (rc = MimerGetError(p->sessionhandle, &e_code, NULL, 0)) + 1;
        if (!MIMER_SUCCEEDED(rc)) {
            msg = QCoreApplication::translate("QMimerSQL", "No Mimer SQL error for code %1")
                          .arg(errCode);
        } else {
            QVarLengthArray<wchar_t> tmp_buff((qsizetype)str_len);
            if (!MIMER_SUCCEEDED(
                        rc = MimerGetError(p->sessionhandle, &e_code, tmp_buff.data(), str_len)))
                msg = QCoreApplication::translate("QMimerSQL", "No Mimer SQL error for code %1")
                              .arg(errCode);
            else
                msg = QString::fromWCharArray(tmp_buff.data());
        }
    } else {
        msg = QCoreApplication::translate("QMimerSQL", "Generic Mimer SQL error");
    }

    return QSqlError("QMIMER: "_L1 + err, msg, type, QString::number(errCode));
}

static QString msgCouldNotGet(const char *type, int column)
{
    //: Data type, column
    return QCoreApplication::translate("QMimerSQLResult",
                                       "Could not get %1, column %2").arg(QLatin1StringView(type)).arg(column);
}

static QString msgCouldNotSet(const char *type, int column)
{
    //: Data type, parameter
    return QCoreApplication::translate("QMimerSQLResult",
                                       "Could not set %1, parameter %2").arg(QLatin1StringView(type)).arg(column);
}

QMimerSQLDriver::QMimerSQLDriver(QObject *parent) : QSqlDriver(*new QMimerSQLDriverPrivate, parent)
{
}

QMimerSQLDriver::QMimerSQLDriver(MimerSession *conn, QObject *parent)
    : QSqlDriver(*new QMimerSQLDriverPrivate, parent)
{
    Q_D(QMimerSQLDriver);
    if (conn)
        d->sessionhandle = *conn;
}

QMimerSQLDriver::~QMimerSQLDriver()
{
    close();
}

QMimerSQLResult::QMimerSQLResult(const QMimerSQLDriver *db)
    : QSqlResult(*new QMimerSQLResultPrivate(this, db))
{
    Q_D(QMimerSQLResult);
    d->preparedQuery = db->hasFeature(QSqlDriver::PreparedQueries);
}

QMimerSQLResult::~QMimerSQLResult()
{
    cleanup();
}

static MimerColumnTypes mimerMapColumnTypes(int32_t t)
{
    switch (t) {
    case MIMER_BINARY:
    case MIMER_BINARY_VARYING:
        return MimerColumnTypes::Binary;
    case MIMER_BLOB:
    case MIMER_NATIVE_BLOB:
        return MimerColumnTypes::Blob;
    case MIMER_CLOB:
    case MIMER_NCLOB:
    case MIMER_NATIVE_CLOB:
    case MIMER_NATIVE_NCLOB:
        return MimerColumnTypes::Clob;
    case MIMER_DATE:
        return MimerColumnTypes::Date;
    case MIMER_TIME:
        return MimerColumnTypes::Time;
    case MIMER_TIMESTAMP:
        return MimerColumnTypes::Timestamp;
    case MIMER_INTERVAL_DAY:
    case MIMER_INTERVAL_DAY_TO_HOUR:
    case MIMER_INTERVAL_DAY_TO_MINUTE:
    case MIMER_INTERVAL_DAY_TO_SECOND:
    case MIMER_INTERVAL_HOUR:
    case MIMER_INTERVAL_HOUR_TO_MINUTE:
    case MIMER_INTERVAL_HOUR_TO_SECOND:
    case MIMER_INTERVAL_MINUTE:
    case MIMER_INTERVAL_MINUTE_TO_SECOND:
    case MIMER_INTERVAL_MONTH:
    case MIMER_INTERVAL_SECOND:
    case MIMER_INTERVAL_YEAR:
    case MIMER_INTERVAL_YEAR_TO_MONTH:
    case MIMER_NCHAR:
    case MIMER_CHARACTER:
    case MIMER_CHARACTER_VARYING:
    case MIMER_NCHAR_VARYING:
    case MIMER_UTF8:
    case MIMER_DEFAULT_DATATYPE:
        return MimerColumnTypes::String;
    case MIMER_INTEGER:
    case MIMER_DECIMAL:
    case MIMER_FLOAT:
        return MimerColumnTypes::Numeric;
    case MIMER_BOOLEAN:
        return MimerColumnTypes::Boolean;
    case MIMER_T_BIGINT:
    case MIMER_T_UNSIGNED_BIGINT:
    case MIMER_NATIVE_BIGINT_NULLABLE:
    case MIMER_NATIVE_BIGINT:
        return MimerColumnTypes::Long;
    case MIMER_NATIVE_REAL_NULLABLE:
    case MIMER_NATIVE_REAL:
    case MIMER_T_REAL:
        return MimerColumnTypes::Float;
    case MIMER_T_FLOAT:
    case MIMER_NATIVE_DOUBLE_NULLABLE:
    case MIMER_NATIVE_DOUBLE:
    case MIMER_T_DOUBLE:
        return MimerColumnTypes::Double;
    case MIMER_NATIVE_INTEGER:
    case MIMER_NATIVE_INTEGER_NULLABLE:
    case MIMER_NATIVE_SMALLINT_NULLABLE:
    case MIMER_NATIVE_SMALLINT:
    case MIMER_T_INTEGER:
    case MIMER_T_SMALLINT:
        return MimerColumnTypes::Int;
    case MIMER_UUID:
        return MimerColumnTypes::Uuid;
    default:
        qCWarning(lcMimer) << "QMimerSQLDriver::mimerMapColumnTypes: Unknown data type:" << t;
    }
    return MimerColumnTypes::Unknown;
}

static QMetaType::Type qDecodeMSQLType(int32_t t)
{
    switch (t) {
    case MIMER_BINARY:
    case MIMER_BINARY_VARYING:
    case MIMER_BLOB:
    case MIMER_NATIVE_BLOB:
        return QMetaType::QByteArray;
    case MIMER_CLOB:
    case MIMER_NCLOB:
    case MIMER_NATIVE_CLOB:
    case MIMER_NATIVE_NCLOB:
    case MIMER_INTERVAL_DAY:
    case MIMER_DECIMAL:
    case MIMER_INTERVAL_DAY_TO_HOUR:
    case MIMER_INTERVAL_DAY_TO_MINUTE:
    case MIMER_INTERVAL_DAY_TO_SECOND:
    case MIMER_INTERVAL_HOUR:
    case MIMER_INTERVAL_HOUR_TO_MINUTE:
    case MIMER_INTERVAL_HOUR_TO_SECOND:
    case MIMER_INTERVAL_MINUTE:
    case MIMER_INTERVAL_MINUTE_TO_SECOND:
    case MIMER_INTERVAL_MONTH:
    case MIMER_INTERVAL_SECOND:
    case MIMER_INTERVAL_YEAR:
    case MIMER_INTERVAL_YEAR_TO_MONTH:
    case MIMER_NCHAR:
    case MIMER_CHARACTER:
    case MIMER_CHARACTER_VARYING:
    case MIMER_NCHAR_VARYING:
    case MIMER_UTF8:
    case MIMER_DEFAULT_DATATYPE:
    case MIMER_INTEGER:
    case MIMER_FLOAT:
        return QMetaType::QString;
    case MIMER_BOOLEAN:
        return QMetaType::Bool;
    case MIMER_T_BIGINT:
    case MIMER_T_UNSIGNED_BIGINT:
    case MIMER_NATIVE_BIGINT_NULLABLE:
    case MIMER_NATIVE_BIGINT:
        return QMetaType::LongLong;
    case MIMER_NATIVE_REAL_NULLABLE:
    case MIMER_NATIVE_REAL:
    case MIMER_T_REAL:
        return QMetaType::Float;
    case MIMER_T_FLOAT:
    case MIMER_NATIVE_DOUBLE_NULLABLE:
    case MIMER_NATIVE_DOUBLE:
    case MIMER_T_DOUBLE:
        return QMetaType::Double;
    case MIMER_NATIVE_INTEGER_NULLABLE:
    case MIMER_T_INTEGER:
    case MIMER_NATIVE_INTEGER:
        return QMetaType::Int;
    case MIMER_NATIVE_SMALLINT_NULLABLE:
    case MIMER_T_SMALLINT:
        return QMetaType::Int;
    case MIMER_DATE:
        return QMetaType::QDate;
    case MIMER_TIME:
        return QMetaType::QTime;
        break;
    case MIMER_TIMESTAMP:
        return QMetaType::QDateTime;
    case MIMER_UUID:
        return QMetaType::QUuid;
    default:
        qCWarning(lcMimer) << "QMimerSQLDriver::qDecodeMSQLType: Unknown data type:" << t;
        return QMetaType::UnknownType;
    }
}

static int32_t qLookupMimDataType(QStringView s)
{
    if (s == u"BINARY")
        return MIMER_BINARY;
    if (s == u"BINARY VARYING")
        return MIMER_BINARY_VARYING;
    if (s == u"BINARY LARGE OBJECT")
        return MIMER_BLOB;
    if (s == u"CHARACTER LARGE OBJECT")
        return MIMER_CLOB;
    if (s == u"NATIONAL CHAR LARGE OBJECT")
        return MIMER_NCLOB;
    if (s == u"INTERVAL DAY")
        return MIMER_INTERVAL_DAY;
    if (s == u"DECIMAL")
        return MIMER_DECIMAL;
    if (s == u"INTERVAL DAY TO HOUR")
        return MIMER_INTERVAL_DAY_TO_HOUR;
    if (s == u"INTERVAL DAY TO MINUTE")
        return MIMER_INTERVAL_DAY_TO_MINUTE;
    if (s == u"INTERVAL DAY TO SECOND")
        return MIMER_INTERVAL_DAY_TO_SECOND;
    if (s == u"INTERVAL HOUR")
        return MIMER_INTERVAL_HOUR;
    if (s == u"INTERVAL HOUR TO MINUTE")
        return MIMER_INTERVAL_HOUR_TO_MINUTE;
    if (s == u"INTERVAL HOUR TO SECOND")
        return MIMER_INTERVAL_HOUR_TO_SECOND;
    if (s == u"INTERVAL MINUTE")
        return MIMER_INTERVAL_MINUTE;
    if (s == u"INTERVAL MINUTE TO SECOND")
        return MIMER_INTERVAL_MINUTE_TO_SECOND;
    if (s == u"INTERVAL MONTH")
        return MIMER_INTERVAL_MONTH;
    if (s == u"INTERVAL SECOND")
        return MIMER_INTERVAL_SECOND;
    if (s == u"INTERVAL YEAR")
        return MIMER_INTERVAL_YEAR;
    if (s == u"INTERVAL YEAR TO MONTH")
        return MIMER_INTERVAL_YEAR_TO_MONTH;
    if (s == u"NATIONAL CHARACTER")
        return MIMER_NCHAR;
    if (s == u"CHARACTER")
        return MIMER_CHARACTER;
    if (s == u"CHARACTER VARYING")
        return MIMER_CHARACTER_VARYING;
    if (s == u"NATIONAL CHARACTER VARYING")
        return MIMER_NCHAR_VARYING;
    if (s == u"UTF-8")
        return MIMER_UTF8;
    if (s == u"BOOLEAN")
        return MIMER_BOOLEAN;
    if (s == u"BIGINT")
        return MIMER_T_BIGINT;
    if (s == u"REAL")
        return MIMER_T_REAL;
    if (s == u"FLOAT")
        return MIMER_T_FLOAT;
    if (s == u"DOUBLE PRECISION")
        return MIMER_T_DOUBLE;
    if (s == u"INTEGER")
        return MIMER_T_INTEGER;
    if (s == u"SMALLINT")
        return MIMER_T_SMALLINT;
    if (s == u"DATE")
        return MIMER_DATE;
    if (s == u"TIME")
        return MIMER_TIME;
    if (s == u"TIMESTAMP")
        return MIMER_TIMESTAMP;
    if (s == u"BUILTIN.UUID")
        return MIMER_UUID;
    if (s == u"USER-DEFINED")
        return MIMER_DEFAULT_DATATYPE;
    qCWarning(lcMimer) << "QMimerSQLDriver::qLookupMimDataType: Unhandled data type:" << s;
    return MIMER_DEFAULT_DATATYPE;
}

QVariant QMimerSQLResult::handle() const
{
    Q_D(const QMimerSQLResult);
    return QVariant::fromValue(d->statementhandle);
}

void QMimerSQLResult::cleanup()
{
    Q_D(QMimerSQLResult);
    if (!driver() || !driver()->isOpen()) {
        d->openCursor = false;
        d->openStatement = false;
        return;
    }
    if (d->openCursor) {
        const int32_t err = MimerCloseCursor(d->statementhandle);
        if (!MIMER_SUCCEEDED(err))
            setLastError(qMakeError(
                    QCoreApplication::translate("QMimerSQLResult", "Could not close cursor"), err,
                    QSqlError::StatementError, d->drv_d_func()));
        d->openCursor = false;
    }
    if (d->openStatement) {
        const int32_t err = MimerEndStatement(&d->statementhandle);
        if (!MIMER_SUCCEEDED(err))
            setLastError(qMakeError(
                    QCoreApplication::translate("QMimerSQLResult", "Could not close statement"),
                    err, QSqlError::StatementError, d->drv_d_func()));
        d->openStatement = false;
    }
    d->currentSize = -1;
}

qint64 QMimerSQLResult::currentRow()
{
    Q_D(const QMimerSQLResult);
    return d->currentRow;
}

bool QMimerSQLResult::fetch(int i)
{
    Q_D(const QMimerSQLResult);
    int32_t err = 0;
    if (!isActive() || !isSelect())
        return false;
    if (i == at())
        return true;
    if (i < 0)
        return false;

    if (isForwardOnly() && i < at())
        return false;

    if (isForwardOnly()) {
        bool rc;
        do {
            rc = fetchNext();
        } while (rc && currentRow() < i);
        return rc;
    } else {
        err = MimerFetchScroll(d->statementhandle, MIMER_ABSOLUTE, i + 1);
        if (err == MIMER_NO_DATA)
            return false;
    }
    if (!MIMER_SUCCEEDED(err)) {
        setLastError(
                qMakeError(QCoreApplication::translate("QMimerSQLResult", "Fetch did not succeed"),
                           err, QSqlError::StatementError, d->drv_d_func()));
        return false;
    }
    setAt(MimerCurrentRow(d->statementhandle) - 1);
    return true;
}

bool QMimerSQLResult::fetchFirst()
{
    Q_D(const QMimerSQLResult);
    int32_t err = 0;
    if (!isActive() || !isSelect())
        return false;
    if (isForwardOnly()) {
        if (currentRow() < 0)
            return fetchNext();
        else if (currentRow() == 0)
            setAt(0);
        else
            return false;
    } else {
        err = MimerFetchScroll(d->statementhandle, MIMER_FIRST, 0);
        if (MIMER_SUCCEEDED(err) && err != MIMER_NO_DATA)
            setAt(0);
    }
    if (!MIMER_SUCCEEDED(err)) {
        setLastError(qMakeError(
                QCoreApplication::translate("QMimerSQLResult", "Fetch first did not succeed"), err,
                QSqlError::StatementError, d->drv_d_func()));
        return false;
    }
    if (err == MIMER_NO_DATA)
        return false;
    return true;
}

bool QMimerSQLResult::fetchLast()
{
    Q_D(const QMimerSQLResult);
    int32_t err = 0;
    int row = 0;
    if (!isActive() || !isSelect())
        return false;
    if (isForwardOnly()) {
        bool rc;
        do {
            rc = fetchNext();
        } while (rc);

        return currentRow() >= 0;
    } else {
        err = MimerFetchScroll(d->statementhandle, static_cast<std::int32_t>(MIMER_LAST), 0);
        if (err == MIMER_NO_DATA)
            return false;
        if (MIMER_SUCCEEDED(err)) {
            row = MimerCurrentRow(d->statementhandle) - 1;
        } else {
            setLastError(qMakeError(
                    QCoreApplication::translate("QMimerSQLResult:", "Fetch last did not succeed"),
                    err, QSqlError::StatementError, d->drv_d_func()));
            return false;
        }
    }

    if (row < 0) {
        setAt(QSql::BeforeFirstRow);
        return false;
    } else {
        setAt(row);
        return true;
    }
}

bool QMimerSQLResult::fetchNext()
{
    Q_D(QMimerSQLResult);
    int32_t err = 0;
    if (!isActive() || !isSelect())
        return false;
    if (isForwardOnly())
        err = MimerFetch(d->statementhandle);
    else
        err = MimerFetchScroll(d->statementhandle, MIMER_NEXT, 0);
    if (!MIMER_SUCCEEDED(err)) {
        setLastError(qMakeError(
                QCoreApplication::translate("QMimerSQLResult", "Could not fetch next row"), err,
                QSqlError::StatementError, d->drv_d_func()));
        if (isForwardOnly())
            d->currentRow = QSql::BeforeFirstRow;
        return false;
    }
    if (err == MIMER_NO_DATA)
        return false;
    if (isForwardOnly())
        setAt(++d->currentRow);
    else
        setAt(MimerCurrentRow(d->statementhandle) - 1);
    return true;
}

QVariant QMimerSQLResult::data(int i)
{
    Q_D(QMimerSQLResult);
    int32_t err;
    int32_t mType;
    if (d->callWithOut) {
        if (i >= MimerParameterCount(d->statementhandle)) {
            setLastError(qMakeError(
                    QCoreApplication::translate("QMimerSQLResult:", "Column %1 out of range")
                            .arg(i),
                    genericError, QSqlError::StatementError, nullptr));
            return QVariant();
        }
        mType = MimerParameterType(d->statementhandle, static_cast<std::int16_t>(i + 1));
    } else {
        if (i >= MimerColumnCount(d->statementhandle)) {
            setLastError(qMakeError(
                    QCoreApplication::translate("QMimerSQLResult:", "Column %1 out of range")
                            .arg(i),
                    genericError, QSqlError::StatementError, nullptr));
            return QVariant();
        }
        mType = MimerColumnType(d->statementhandle, static_cast<std::int16_t>(i + 1));
    }
    const QMetaType::Type type = qDecodeMSQLType(mType);
    const MimerColumnTypes mimDataType = mimerMapColumnTypes(mType);
    err = MimerIsNull(d->statementhandle, static_cast<std::int16_t>(i + 1));
    if (err > 0) {
        return QVariant(QMetaType(type), nullptr);
    } else {
        switch (mimDataType) {
        case MimerColumnTypes::Date: {
            wchar_t dateString_w[maxDateStringSize + 1];
            err = MimerGetString(d->statementhandle, static_cast<std::int16_t>(i + 1), dateString_w,
                                 sizeof(dateString_w) / sizeof(dateString_w[0]));
            if (!MIMER_SUCCEEDED(err)) {
                setLastError(qMakeError(msgCouldNotGet("date", i),
                                        err, QSqlError::StatementError, d->drv_d_func()));
                return QVariant(QMetaType(type), nullptr);
            }
            return QDate::fromString(QString::fromWCharArray(dateString_w), "yyyy-MM-dd"_L1);
        }
        case MimerColumnTypes::Time: {
            wchar_t timeString_w[maxTimeStringSize + 1];
            err = MimerGetString(d->statementhandle, static_cast<std::int16_t>(i + 1), timeString_w,
                                 sizeof(timeString_w) / sizeof(timeString_w[0]));
            if (!MIMER_SUCCEEDED(err)) {
                setLastError(qMakeError(msgCouldNotGet("time", i),
                                        err, QSqlError::StatementError, d->drv_d_func()));
                return QVariant(QMetaType(type), nullptr);
            }
            QString timeString = QString::fromWCharArray(timeString_w);
            QString timeFormatString = "HH:mm:ss"_L1;
            if (timeString.size() > 8) {
                timeFormatString.append(".zzz"_L1);
                timeString = timeString.left(12);
            }
            return QTime::fromString(timeString, timeFormatString);
        }
        case MimerColumnTypes::Timestamp: {
            wchar_t dateTimeString_w[maxTimestampStringSize + 1];
            err = MimerGetString(d->statementhandle, static_cast<std::int16_t>(i + 1),
                                 dateTimeString_w,
                                 sizeof(dateTimeString_w) / sizeof(dateTimeString_w[0]));
            if (!MIMER_SUCCEEDED(err)) {
                setLastError(
                        qMakeError(msgCouldNotGet("date time", i),
                                   err, QSqlError::StatementError, d->drv_d_func()));
                return QVariant(QMetaType(type), nullptr);
            }
            QString dateTimeString = QString::fromWCharArray(dateTimeString_w);
            QString dateTimeFormatString = "yyyy-MM-dd HH:mm:ss"_L1;
            if (dateTimeString.size() > 19) {
                dateTimeFormatString.append(".zzz"_L1);
                dateTimeString = dateTimeString.left(23);
            }
            return QDateTime::fromString(dateTimeString, dateTimeFormatString);
        }
        case MimerColumnTypes::Int: {
            int resInt;
            err = MimerGetInt32(d->statementhandle, static_cast<std::int16_t>(i + 1), &resInt);
            if (!MIMER_SUCCEEDED(err)) {
                setLastError(qMakeError(msgCouldNotGet("int32", i),
                                        err, QSqlError::StatementError, d->drv_d_func()));
                return QVariant(QMetaType(type), nullptr);
            }
            return resInt;
        }
        case MimerColumnTypes::Long: {
            int64_t resLongLong;
            err = MimerGetInt64(d->statementhandle, static_cast<std::int16_t>(i + 1), &resLongLong);
            if (!MIMER_SUCCEEDED(err)) {
                setLastError(qMakeError(msgCouldNotGet("int64", i),
                                        err, QSqlError::StatementError, d->drv_d_func()));
                return QVariant(QMetaType(type), nullptr);
            }
            return (qlonglong)resLongLong;
        }
        case MimerColumnTypes::Boolean: {
            err = MimerGetBoolean(d->statementhandle, static_cast<std::int16_t>(i + 1));
            if (!MIMER_SUCCEEDED(err)) {
                setLastError(
                        qMakeError(msgCouldNotGet("boolean", i),
                                   err, QSqlError::StatementError, d->drv_d_func()));
                return QVariant(QMetaType(type), nullptr);
            }
            return err == 1;
        }
        case MimerColumnTypes::Float: {
            float resFloat;
            err = MimerGetFloat(d->statementhandle, static_cast<std::int16_t>(i + 1), &resFloat);
            if (!MIMER_SUCCEEDED(err)) {
                setLastError(qMakeError(msgCouldNotGet("float", i),
                                        err, QSqlError::StatementError, d->drv_d_func()));
                return QVariant(QMetaType(type), nullptr);
            }
            return resFloat;
        }
        case MimerColumnTypes::Double: {
            double resDouble;
            err = MimerGetDouble(d->statementhandle, static_cast<std::int16_t>(i + 1), &resDouble);
            if (!MIMER_SUCCEEDED(err)) {
                setLastError(
                        qMakeError(msgCouldNotGet("double", i),
                                   err, QSqlError::StatementError, d->drv_d_func()));
                return QVariant(QMetaType(type), nullptr);
            }
            switch (numericalPrecisionPolicy()) {
            case QSql::LowPrecisionInt32:
                return static_cast<std::int32_t>(resDouble);
            case QSql::LowPrecisionInt64:
                return static_cast<qint64>(resDouble);
            case QSql::LowPrecisionDouble:
                return static_cast<qreal>(resDouble);
            case QSql::HighPrecision:
                return QString::number(resDouble, 'g', 17);
            }
            return QVariant(QMetaType(type), nullptr);
        }
        case MimerColumnTypes::Binary: {
            QByteArray byteArray;
            // Get size
            err = MimerGetBinary(d->statementhandle, static_cast<std::int16_t>(i + 1), NULL, 0);
            if (MIMER_SUCCEEDED(err)) {
                byteArray.resize(err);
                err = MimerGetBinary(d->statementhandle, static_cast<std::int16_t>(i + 1),
                                     byteArray.data(), err);
            }
            if (!MIMER_SUCCEEDED(err)) {
                setLastError(
                        qMakeError(msgCouldNotGet("binary", i),
                                   err, QSqlError::StatementError, d->drv_d_func()));
                return QVariant(QMetaType(type), nullptr);
            }
            return byteArray;
        }
        case MimerColumnTypes::Blob: {
            QByteArray byteArray;
            size_t size;
            err = MimerGetLob(d->statementhandle, static_cast<std::int16_t>(i + 1), &size,
                              &d->lobhandle);
            if (MIMER_SUCCEEDED(err)) {
                constexpr size_t maxSize = lobChunkMaxSizeFetch;
                QVarLengthArray<char> blobchar(lobChunkMaxSizeFetch);
                byteArray.reserve(size);
                size_t left_to_return = size;
                while (left_to_return > 0) {
                    const size_t bytesToReceive =
                            left_to_return <= maxSize ? left_to_return : maxSize;
                    err = MimerGetBlobData(&d->lobhandle, blobchar.data(), bytesToReceive);
                    byteArray.append(QByteArray::fromRawData(blobchar.data(), bytesToReceive));
                    left_to_return -= bytesToReceive;
                    if (!MIMER_SUCCEEDED(err)) {
                        setLastError(qMakeError(msgCouldNotGet("BLOB", i),
                                err, QSqlError::StatementError, d->drv_d_func()));
                        return QVariant(QMetaType(type), nullptr);
                    }
                }
            } else {
                setLastError(qMakeError(msgCouldNotGet("BLOB", i),
                                        err, QSqlError::StatementError, d->drv_d_func()));
                return QVariant(QMetaType(type), nullptr);
            }
            return byteArray;
        }
        case MimerColumnTypes::Numeric:
        case MimerColumnTypes::String: {
            wchar_t resString_w[maxStackStringSize + 1];
            // Get size
            err = MimerGetString(d->statementhandle, static_cast<std::int16_t>(i + 1), resString_w,
                                 0);
            if (MIMER_SUCCEEDED(err)) {
                int size = err;
                if (err <= maxStackStringSize) { // For smaller strings, use a small buffer for
                                                 // efficiency
                    err = MimerGetString(d->statementhandle, static_cast<std::int16_t>(i + 1),
                                         resString_w, maxStackStringSize + 1);
                    if (MIMER_SUCCEEDED(err))
                        return QString::fromWCharArray(resString_w);
                } else { // For larger strings, dynamically allocate memory
                    QVarLengthArray<wchar_t> largeResString_w(size + 1);
                    err = MimerGetString(d->statementhandle, static_cast<std::int16_t>(i + 1),
                                         largeResString_w.data(), size + 1);
                    if (MIMER_SUCCEEDED(err))
                        return QString::fromWCharArray(largeResString_w.data());
                }
            }
            setLastError(qMakeError(msgCouldNotGet(
                        mimDataType == MimerColumnTypes::Numeric ? "numeric" : "string", i),
                        err, QSqlError::StatementError, d->drv_d_func()));
            return QVariant(QMetaType(type), nullptr);
        }
        case MimerColumnTypes::Clob: {
            size_t size;
            err = MimerGetLob(d->statementhandle, static_cast<std::int16_t>(i + 1), &size,
                              &d->lobhandle);
            if (MIMER_SUCCEEDED(err)) {
                constexpr size_t maxSize = lobChunkMaxSizeFetch;
                QVarLengthArray<wchar_t> clobstring_w(lobChunkMaxSizeFetch + 1);

                size_t left_to_return = size;
                QString returnString;
                while (left_to_return > 0) {
                    const size_t bytesToReceive =
                            left_to_return <= maxSize ? left_to_return : maxSize;
                    err = MimerGetNclobData(&d->lobhandle, clobstring_w.data(), bytesToReceive + 1);
                    returnString.append(QString::fromWCharArray(clobstring_w.data()));
                    left_to_return -= bytesToReceive;
                    if (!MIMER_SUCCEEDED(err)) {
                        setLastError(qMakeError(msgCouldNotGet("CLOB", i),
                                err, QSqlError::StatementError, d->drv_d_func()));
                        return QVariant(QMetaType(type), nullptr);
                    }
                }
                return returnString;
            }
            setLastError(qMakeError(msgCouldNotGet("CLOB", i),
                    err, QSqlError::StatementError, d->drv_d_func()));
            return QVariant(QMetaType(type), nullptr);
        }
        case MimerColumnTypes::Uuid: {
            unsigned char uuidChar[16];
            err = MimerGetUUID(d->statementhandle, static_cast<std::int16_t>(i + 1), uuidChar);
            if (!MIMER_SUCCEEDED(err)) {
                setLastError(qMakeError(msgCouldNotGet("UUID", i),
                                        err, QSqlError::StatementError, d->drv_d_func()));
                return QVariant(QMetaType(type), nullptr);
            }
            const auto uuidByteArray = QByteArrayView(reinterpret_cast<char *>(uuidChar), 16);
            return QUuid::fromRfc4122(uuidByteArray);
        }
        case MimerColumnTypes::Unknown:
        default:
            setLastError(qMakeError(
                    QCoreApplication::translate("QMimerSQLResult", "Unknown data type %1").arg(i),
                    genericError, QSqlError::StatementError, nullptr));
        }
        return QVariant(QMetaType(type), nullptr);
    }
}

bool QMimerSQLResult::isNull(int index)
{
    Q_D(const QMimerSQLResult);
    const int32_t rc = MimerIsNull(d->statementhandle, static_cast<std::int16_t>(index + 1));
    if (!MIMER_SUCCEEDED(rc)) {
        setLastError(qMakeError(
                QCoreApplication::translate("QMimerSQLResult", "Could not check null, column %1")
                        .arg(index),
                rc, QSqlError::StatementError, d->drv_d_func()));
        return false;
    }
    return rc != 0;
}

bool QMimerSQLResult::reset(const QString &query)
{
    if (!prepare(query))
        return false;
    return exec();
}

int QMimerSQLResult::size()
{
    Q_D(QMimerSQLResult);
    if (!isActive() || !isSelect() || isForwardOnly())
        return -1;

    if (d->currentSize != -1)
        return d->currentSize;

    const int currentRow = MimerCurrentRow(d->statementhandle);
    MimerFetchScroll(d->statementhandle, static_cast<std::int32_t>(MIMER_LAST), 0);
    int size = MimerCurrentRow(d->statementhandle);
    if (!MIMER_SUCCEEDED(size))
        size = -1;
    MimerFetchScroll(d->statementhandle, MIMER_ABSOLUTE, currentRow);
    d->currentSize = size;
    return size;
}

int QMimerSQLResult::numRowsAffected()
{
    Q_D(const QMimerSQLResult);
    return d->rowsAffected;
}

QSqlRecord QMimerSQLResult::record() const
{
    Q_D(const QMimerSQLResult);
    QSqlRecord rec;
    if (!isActive() || !isSelect() || !driver())
        return rec;
    QSqlField field;
    const int colSize = MimerColumnCount(d->statementhandle);
    for (int i = 0; i < colSize; i++) {
        wchar_t colName_w[100];
        MimerColumnName(d->statementhandle, static_cast<std::int16_t>(i + 1), colName_w,
                        sizeof(colName_w) / sizeof(colName_w[0]));
        field.setName(QString::fromWCharArray(colName_w));
        const int32_t mType = MimerColumnType(d->statementhandle, static_cast<std::int16_t>(i + 1));
        const QMetaType::Type type = qDecodeMSQLType(mType);
        field.setMetaType(QMetaType(type));
        field.setValue(QVariant(field.metaType()));
        // field.setPrecision(); Should be implemented once the Mimer API can give this
        // information.
        // field.setLength(); Should be implemented once the Mimer API can give
        // this information.
        rec.insert(i, field);
    }
    return rec;
}

bool QMimerSQLResult::prepare(const QString &query)
{
    Q_D(QMimerSQLResult);
    int32_t err;
    if (!driver())
        return false;
    if (!d->preparedQuery)
        return QSqlResult::prepare(query);
    if (query.isEmpty())
        return false;
    cleanup();
    const int option = isForwardOnly() ? MIMER_FORWARD_ONLY : MIMER_SCROLLABLE;
    err = MimerBeginStatement8(d->drv_d_func()->sessionhandle, query.toUtf8().constData(), option,
                               &d->statementhandle);
    if (err == MIMER_STATEMENT_CANNOT_BE_PREPARED) {
        err = MimerExecuteStatement8(d->drv_d_func()->sessionhandle, query.toUtf8().constData());
        if (MIMER_SUCCEEDED(err)) {
            d->executedStatement = true;
            d->openCursor = false;
            d->openStatement = false;
            return true;
        }
    }
    if (!MIMER_SUCCEEDED(err)) {
        setLastError(qMakeError(QCoreApplication::translate("QMimerSQLResult",
                                                            "Could not prepare/execute statement"),
                                err, QSqlError::StatementError, d->drv_d_func()));
        return false;
    }
    d->openStatement = true;
    return true;
}

bool QMimerSQLResult::exec()
{
    Q_D(QMimerSQLResult);
    int32_t err;
    if (!driver())
        return false;
    if (!d->preparedQuery)
        return QSqlResult::exec();
    if (d->executedStatement) {
        d->executedStatement = false;
        return true;
    }
    if (d->openCursor) {
        setAt(QSql::BeforeFirstRow);
        err = MimerCloseCursor(d->statementhandle);
        d->openCursor = false;
        d->currentSize = -1;
    }
    QVector<QVariant> &values = boundValues();
    if (d->execBatch)
        values = d->batch_vector;
    int mimParamCount = MimerParameterCount(d->statementhandle);
    if (!MIMER_SUCCEEDED(mimParamCount))
        mimParamCount = 0;
    if (mimParamCount != values.size()) {
        setLastError(qMakeError(
                QCoreApplication::translate("QMimerSQLResult", "Wrong number of parameters"),
                genericError, QSqlError::StatementError, nullptr));
        return false;
    }
    for (int i = 0; i < mimParamCount; i++) {
        if (bindValueType(i) == QSql::Out) {
            d->callWithOut = true;
            continue;
        }
        const QVariant &val = values.at(i);
        if (QSqlResultPrivate::isVariantNull(val) || val.isNull() || val.toString().isNull()) {
            err = MimerSetNull(d->statementhandle, i + 1);
            if (!MIMER_SUCCEEDED(err)) {
                setLastError(
                        qMakeError(msgCouldNotSet("null", i),
                                   err, QSqlError::StatementError, d->drv_d_func()));
                return false;
            }
            continue;
        }

        const int mimParamType = MimerParameterType(d->statementhandle, i + 1);
        const MimerColumnTypes mimDataType = mimerMapColumnTypes(mimParamType);
        switch (mimDataType) {
        case MimerColumnTypes::Int: {
            bool convertOk;
            err = MimerSetInt32(d->statementhandle, i + 1, val.toInt(&convertOk));
            if (!convertOk || !MIMER_SUCCEEDED(err)) {
                setLastError(
                        qMakeError(msgCouldNotSet("int32", i),
                                   convertOk ? err : genericError, QSqlError::StatementError,
                                   convertOk ? d->drv_d_func() : nullptr));
                return false;
            }
            break;
        }
        case MimerColumnTypes::Long: {
            bool convertOk;
            err = MimerSetInt64(d->statementhandle, i + 1, val.toLongLong(&convertOk));
            if (!convertOk || !MIMER_SUCCEEDED(err)) {
                setLastError(
                        qMakeError(msgCouldNotSet("int64", i),
                                   convertOk ? err : genericError, QSqlError::StatementError,
                                   convertOk ? d->drv_d_func() : nullptr));
                return false;
            }
            break;
        }
        case MimerColumnTypes::Float: {
            bool convertOk;
            err = MimerSetFloat(d->statementhandle, i + 1, val.toFloat(&convertOk));
            if (!convertOk || !MIMER_SUCCEEDED(err)) {
                setLastError(
                        qMakeError(msgCouldNotSet("float", i),
                                   convertOk ? err : genericError, QSqlError::StatementError,
                                   convertOk ? d->drv_d_func() : nullptr));
                return false;
            }
            break;
        }
        case MimerColumnTypes::Double: {
            bool convertOk;
            err = MimerSetDouble(d->statementhandle, i + 1, val.toDouble(&convertOk));
            if (!convertOk || !MIMER_SUCCEEDED(err)) {
                setLastError(
                        qMakeError(msgCouldNotSet("double", i),
                                   convertOk ? err : genericError, QSqlError::StatementError,
                                   convertOk ? d->drv_d_func() : nullptr));
                return false;
            }
            break;
        }
        case MimerColumnTypes::Binary: {
            const QByteArray binArr = val.toByteArray();
            size_t size = static_cast<std::size_t>(binArr.size());
            err = MimerSetBinary(d->statementhandle, i + 1, binArr.data(), size);
            if (!MIMER_SUCCEEDED(err)) {
                setLastError(
                        qMakeError(msgCouldNotSet("binary", i),
                                   err, QSqlError::StatementError, d->drv_d_func()));
                return false;
            }
            break;
        }
        case MimerColumnTypes::Boolean: {
            err = MimerSetBoolean(d->statementhandle, i + 1, val.toBool() == true ? 1 : 0);
            if (!MIMER_SUCCEEDED(err)) {
                setLastError(
                        qMakeError(msgCouldNotSet("boolean", i),
                                   err, QSqlError::StatementError, d->drv_d_func()));
                return false;
            }
            break;
        }
        case MimerColumnTypes::Uuid: {
            const QByteArray uuidArray = val.toUuid().toRfc4122();
            const unsigned char *uuid =
                    reinterpret_cast<const unsigned char *>(uuidArray.constData());
            err = MimerSetUUID(d->statementhandle, i + 1, uuid);
            if (!MIMER_SUCCEEDED(err)) {
                setLastError(
                        qMakeError(msgCouldNotSet("UUID", i),
                                   err, QSqlError::StatementError, d->drv_d_func()));
                return false;
            }
            break;
        }
        case MimerColumnTypes::Numeric:
        case MimerColumnTypes::String: {
            QByteArray string_b = val.toString().trimmed().toUtf8();
            const char *string_u = string_b.constData();
            err = MimerSetString8(d->statementhandle, i + 1, string_u);
            if (!MIMER_SUCCEEDED(err)) {
                setLastError(
                        qMakeError(msgCouldNotSet(
                            mimDataType == MimerColumnTypes::Numeric ? "numeric" : "string", i),
                            err, QSqlError::StatementError, d->drv_d_func()));
                return false;
            }
            break;
        }
        case MimerColumnTypes::Date: {
            err = MimerSetString8(d->statementhandle, i + 1, val.toString().toUtf8().constData());
            if (!MIMER_SUCCEEDED(err)) {
                setLastError(
                        qMakeError(msgCouldNotSet("date", i),
                                   err, QSqlError::StatementError, d->drv_d_func()));
                return false;
            }
            break;
        }
        case MimerColumnTypes::Time: {
            QString timeFormatString = "hh:mm:ss"_L1;
            const QTime timeVal = val.toTime();
            if (timeVal.msec() > 0)
                timeFormatString.append(".zzz"_L1);
            err = MimerSetString8(d->statementhandle, i + 1,
                                  timeVal.toString(timeFormatString).toUtf8().constData());
            if (!MIMER_SUCCEEDED(err)) {
                setLastError(
                        qMakeError(msgCouldNotSet("time", i),
                                   err, QSqlError::StatementError, d->drv_d_func()));
                return false;
            }
            break;
        }
        case MimerColumnTypes::Timestamp: {
            QString dateTimeFormatString = "yyyy-MM-dd hh:mm:ss"_L1;
            const QDateTime dateTimeVal = val.toDateTime();
            if (dateTimeVal.time().msec() > 0)
                dateTimeFormatString.append(".zzz"_L1);
            err = MimerSetString8(
                    d->statementhandle, i + 1,
                    val.toDateTime().toString(dateTimeFormatString).toUtf8().constData());
            if (!MIMER_SUCCEEDED(err)) {
                setLastError(qMakeError(msgCouldNotSet("datetime", i),
                        err, QSqlError::StatementError, d->drv_d_func()));
                return false;
            }
            break;
        }
        case MimerColumnTypes::Blob: {
            const QByteArray blobArr = val.toByteArray();
            const char *blobData = blobArr.constData();
            qsizetype size = blobArr.size();
            err = MimerSetLob(d->statementhandle, i + 1, size, &d->lobhandle);
            if (MIMER_SUCCEEDED(err)) {
                qsizetype maxSize = lobChunkMaxSizeSet;
                if (size > maxSize) {
                    qsizetype left_to_send = size;
                    for (qsizetype k = 0; left_to_send > 0; k++) {
                        if (left_to_send <= maxSize) {
                            err = MimerSetBlobData(&d->lobhandle, &blobData[k * maxSize],
                                                   left_to_send);
                            left_to_send = 0;
                        } else {
                            err = MimerSetBlobData(&d->lobhandle, &blobData[k * maxSize], maxSize);
                            left_to_send = left_to_send - maxSize;
                        }
                    }
                    if (!MIMER_SUCCEEDED(err)) {
                        setLastError(
                                qMakeError(msgCouldNotSet("BLOB byte array", i),
                                           err, QSqlError::StatementError, d->drv_d_func()));
                        return false;
                    }
                } else {
                    err = MimerSetBlobData(&d->lobhandle, blobArr, size);
                }
            }
            if (!MIMER_SUCCEEDED(err)) {
                setLastError(qMakeError(msgCouldNotSet("BLOB byte array", i),
                        err, QSqlError::StatementError, d->drv_d_func()));
                return false;
            }
            break;
        }
        case MimerColumnTypes::Clob: {
            QByteArray string_b = val.toString().trimmed().toUtf8();
            const char *string_u = string_b.constData();
            size_t size_c = 1;
            size_t size = 0;
            while (string_u[size++])
                if ((string_u[size] & 0xc0) != 0x80)
                    size_c++;
            err = MimerSetLob(d->statementhandle, i + 1, size_c, &d->lobhandle);
            if (MIMER_SUCCEEDED(err)) {
                constexpr size_t maxSize = lobChunkMaxSizeSet;
                if (size > maxSize) {
                    size_t left_to_send = size;
                    size_t pos = 0;
                    uint step_back = 0;
                    while (left_to_send > 0 && step_back < maxSize) {
                        step_back = 0;
                        if (left_to_send <= maxSize) {
                            err = MimerSetNclobData8(&d->lobhandle, &string_u[pos], left_to_send);
                            left_to_send = 0;
                        } else {
                            // Check that we don't split a multi-byte utf-8 characters
                            while (pos + maxSize - step_back > 0
                                   && (string_u[pos + maxSize - step_back] & 0xc0) == 0x80)
                                step_back++;
                            err = MimerSetNclobData8(&d->lobhandle, &string_u[pos],
                                                     maxSize - step_back);
                            left_to_send = left_to_send - maxSize + step_back;
                            pos += maxSize - step_back;
                        }
                        if (!MIMER_SUCCEEDED(err)) {
                            setLastError(qMakeError(msgCouldNotSet("CLOB", i),
                                    err, QSqlError::StatementError, d->drv_d_func()));
                            return false;
                        }
                    }
                } else {
                    err = MimerSetNclobData8(&d->lobhandle, string_u, size);
                }
            }
            if (!MIMER_SUCCEEDED(err)) {
                setLastError(
                        qMakeError(msgCouldNotSet("CLOB", i),
                                   err, QSqlError::StatementError, d->drv_d_func()));
                return false;
            }
            break;
        }
        case MimerColumnTypes::Unknown:
        default:
            setLastError(qMakeError(
                    QCoreApplication::translate("QMimerSQLResult", "Unknown datatype, parameter %1")
                            .arg(i),
                    genericError, QSqlError::StatementError, nullptr));
            return false;
        }
    }
    if (d->execBatch)
        return true;
    err = MimerExecute(d->statementhandle);
    if (MIMER_SUCCEEDED(err)) {
        d->rowsAffected = err;
        int k = 0;
        for (qsizetype i = 0; i < values.size(); i++) {
            if (bindValueType(i) == QSql::Out || bindValueType(i) == QSql::InOut) {
                bindValue(i, data(k), QSql::In);
                k++;
            }
        }
        d->callWithOut = false;
    }
    setSelect(false);
    if (MIMER_SEQUENCE_ERROR == err) {
        err = MimerOpenCursor(d->statementhandle);
        d->rowsAffected = err;
        d->openCursor = true;
        d->currentRow = QSql::BeforeFirstRow;
        setSelect(true);
    }
    if (!MIMER_SUCCEEDED(err)) {
        setLastError(
                qMakeError(QCoreApplication::translate("QMimerSQLResult",
                                                       "Could not execute statement/open cursor"),
                           err, QSqlError::StatementError, d->drv_d_func()));
        return false;
    }
    setActive(true);
    return true;
}

bool QMimerSQLResult::execBatch(bool arrayBind)
{
    Q_D(QMimerSQLResult);
    Q_UNUSED(arrayBind);
    int32_t err;
    const QVector<QVariant> values = boundValues();

    // Check that we only have input parameters. Currently
    // we can only handle batch operations without output parameters.
    for (qsizetype i = 0; i < values.first().toList().size(); i++)
        if (bindValueType(i) == QSql::Out || bindValueType(i) == QSql::InOut) {
            setLastError(qMakeError(QCoreApplication::translate(
                                            "QMimerSQLResult",
                                            "Only input parameters can be used in batch operations"),
                                    genericError, QSqlError::StatementError, nullptr));
            d->execBatch = false;
            return false;
        }
    d->execBatch = true;
    for (qsizetype i = 0; i < values.first().toList().size(); i++) {
        for (qsizetype j = 0; j < values.size(); j++)
            d->batch_vector.append(values.at(j).toList().at(i));
        exec();
        if (i != (values.at(0).toList().size() - 1)) {
            err = MimerAddBatch(d->statementhandle);
            if (!MIMER_SUCCEEDED(err)) {
                setLastError(qMakeError(
                        //: %1 is the batch number
                        QCoreApplication::translate("QMimerSQLResult", "Could not add batch %1")
                                .arg(i),
                        err, QSqlError::StatementError, d->drv_d_func()));
                d->execBatch = false;
                return false;
            }
        }
        d->batch_vector.clear();
    }
    d->execBatch = false;
    err = MimerExecute(d->statementhandle);
    if (!MIMER_SUCCEEDED(err)) {
        setLastError(qMakeError(
                QCoreApplication::translate("QMimerSQLResult", "Could not execute batch"), err,
                QSqlError::StatementError, d->drv_d_func()));
        return false;
    }
    return true;
}

QVariant QMimerSQLResult::lastInsertId() const
{
    Q_D(const QMimerSQLResult);
    int64_t lastSequence;
    const int32_t err = MimerGetSequenceInt64(d->statementhandle, &lastSequence);
    if (!MIMER_SUCCEEDED(err))
        return QVariant(QMetaType(QMetaType::LongLong), nullptr);
    return QVariant(qint64(lastSequence));
}

bool QMimerSQLDriver::hasFeature(DriverFeature f) const
{
    switch (f) {
    case NamedPlaceholders: // Is true in reality but Qt parses Sql statement...
    case EventNotifications:
    case LowPrecisionNumbers:
    case MultipleResultSets:
    case SimpleLocking:
    case CancelQuery:
        return false;
    case FinishQuery:
    case LastInsertId:
    case Transactions:
    case QuerySize:
    case BLOB:
    case Unicode:
    case PreparedQueries:
    case PositionalPlaceholders:
    case BatchOperations:
        return true;
    }
    return true;
}

bool QMimerSQLDriver::open(const QString &db, const QString &user, const QString &password,
                           const QString &host, int port, const QString &connOpts)
{
    Q_D(QMimerSQLDriver);
    Q_UNUSED(host);
    Q_UNUSED(port);
    Q_UNUSED(connOpts);
    if (isOpen())
        close();
    const int32_t err = MimerBeginSession8(db.toUtf8().constData(), user.toUtf8().constData(),
                                           password.toUtf8().constData(), &d->sessionhandle);
    if (!MIMER_SUCCEEDED(err)) {
        setLastError(qMakeError(
                QCoreApplication::translate("QMimerSQLDriver", "Could not connect to database")
                        + " "_L1 + db,
                err, QSqlError::ConnectionError, nullptr));
        setOpenError(true);
        return false;
    }
    d->dbUser = user;
    d->dbName = db;
    setOpen(true);
    setOpenError(false);
    return true;
}

void QMimerSQLDriver::close()
{
    Q_D(QMimerSQLDriver);
    if (isOpen()) {
        const int end_err = MimerEndSession(&d->sessionhandle);
        if (MIMER_SUCCEEDED(end_err)) {
            setOpen(false);
            setOpenError(false);
        }
    }
}

QSqlResult *QMimerSQLDriver::createResult() const
{
    return new QMimerSQLResult(this);
}

QStringList QMimerSQLDriver::tables(QSql::TableType type) const
{
    QStringList tl;
    if (!isOpen())
        return tl;
    QSqlQuery t(createResult());
    QString sql;
    switch (type) {
    case QSql::Tables: {
        sql = "select table_name from information_schema.tables where "
              "table_type=\'BASE TABLE\' AND table_schema = CURRENT_USER"_L1;
        break;
    }
    case QSql::SystemTables: {
        sql = "select table_name from information_schema.tables where "
              "table_type=\'BASE TABLE\' AND table_schema = \'SYSTEM\'"_L1;
        break;
    }
    case QSql::Views: {
        sql = "select table_name from information_schema.tables where "
              "table_type=\'VIEW\' AND table_schema = CURRENT_USER"_L1;
        break;
    }
    case QSql::AllTables: {
        sql = "select table_name from information_schema.tables where "
              "(table_type=\'VIEW\' or table_type=\'BASE TABLE\')"
              " AND (table_schema = CURRENT_USER OR table_schema =\'SYSTEM\')"_L1;
        break;
    }
    default:
        break;
    }
    if (sql.length() > 0) {
        t.exec(sql);
        while (t.next())
            tl.append(t.value(0).toString());
    }
    return tl;
}

QSqlIndex QMimerSQLDriver::primaryIndex(const QString &tablename) const
{
    Q_D(const QMimerSQLDriver);
    if (!isOpen())
        return QSqlIndex();
    QString table = tablename;
    if (isIdentifierEscaped(table, QSqlDriver::TableName))
        table = stripDelimiters(table, QSqlDriver::TableName);
    QSqlIndex index(tablename);
    QSqlQuery t(createResult());
    QString schema;
    QString qualifiedName = table;
    d->splitTableQualifier(qualifiedName, &schema, &table);
    QString sql =
            "select information_schema.ext_access_paths.column_name,"
            "case when data_type = 'INTERVAL' then 'INTERVAL '|| interval_type "
            "when data_type = 'INTEGER' and numeric_precision > 10 then 'BIGINT' "
            "when data_type = 'INTEGER' and numeric_precision <= 10 AND NUMERIC_PRECISION > 5 "
            "then 'INTEGER' when data_type = 'INTEGER' and numeric_precision <= 5 then 'SMALLINT' "
            "else upper(data_type) end as data_type "
            "from information_schema.ext_access_paths full outer join "
            "information_schema.columns on information_schema.ext_access_paths.column_name = "
            "information_schema.columns.column_name and "
            "information_schema.ext_access_paths.table_name = "
            "information_schema.columns.table_name where "
            "information_schema.ext_access_paths.table_name = \'"_L1;
    sql.append(table)
            .append("\' and index_type = \'PRIMARY KEY\'"_L1);
    if (schema.length() == 0)
        sql.append(" and table_schema = CURRENT_USER"_L1);
    else
        sql.append(" and table_schema = \'"_L1).append(schema).append("\'"_L1);

    if (!t.exec(sql))
        return QSqlIndex();
    int i = 0;
    while (t.next()) {
        QSqlField field(t.value(0).toString(),
                        QMetaType(qDecodeMSQLType(qLookupMimDataType(t.value(1).toString()))),
                        tablename);
        index.insert(i, field);
        index.setName(t.value(0).toString());
        i++;
    }
    return index;
}

QSqlRecord QMimerSQLDriver::record(const QString &tablename) const
{
    Q_D(const QMimerSQLDriver);
    if (!isOpen())
        return QSqlRecord();
    QSqlRecord rec;
    QSqlQuery t(createResult());
    QString qualifiedName = tablename;
    if (isIdentifierEscaped(qualifiedName, QSqlDriver::TableName))
        qualifiedName = stripDelimiters(qualifiedName, QSqlDriver::TableName);
    QString schema, table;
    d->splitTableQualifier(qualifiedName, &schema, &table);

    QString sql =
            "select column_name, case when data_type = 'INTERVAL' then 'INTERVAL '|| interval_type "
            "when data_type = 'INTEGER' and numeric_precision > 10 then 'BIGINT' "
            "when data_type = 'INTEGER' and numeric_precision <= 10 AND numeric_precision > 5 "
            "then 'INTEGER' when data_type = 'INTEGER' and numeric_precision <= 5 then 'SMALLINT' "
            "else UPPER(data_type) end as data_type, case when is_nullable = 'YES' then false else "
            "true end as required, "
            "coalesce(numeric_precision, coalesce(datetime_precision,coalesce(interval_precision, "
            "-1))) as prec from information_schema.columns where table_name = \'"_L1;
    if (schema.length() == 0)
        sql.append(table).append("\' and table_schema = CURRENT_USER"_L1);
    else
        sql.append(table).append("\' and table_schema = \'"_L1).append(schema).append("\'"_L1);
    sql.append(" order by ordinal_position"_L1);
    if (!t.exec(sql))
        return QSqlRecord();

    while (t.next()) {
        QSqlField field(t.value(0).toString(),
                        QMetaType(qDecodeMSQLType(qLookupMimDataType(t.value(1).toString()))),
                        tablename);
        field.setRequired(t.value(3).toBool());
        if (t.value(3).toInt() != -1)
            field.setPrecision(t.value(3).toInt());
        rec.append(field);
    }

    return rec;
}

QVariant QMimerSQLDriver::handle() const
{
    Q_D(const QMimerSQLDriver);
    return QVariant::fromValue(d->sessionhandle);
}

QString QMimerSQLDriver::escapeIdentifier(const QString &identifier, IdentifierType type) const
{
    Q_UNUSED(type);
    QString res = identifier;
    if (!identifier.isEmpty() && !identifier.startsWith(u'"') && !identifier.endsWith(u'"')) {
        res.replace(u'"', "\"\""_L1);
        res = u'"' + res + u'"';
        res.replace(u'.', "\".\""_L1);
    }
    return res;
}

bool QMimerSQLDriver::beginTransaction()
{
    Q_D(const QMimerSQLDriver);
    const int32_t err = MimerBeginTransaction(d->sessionhandle, MIMER_TRANS_READWRITE);
    if (!MIMER_SUCCEEDED(err)) {
        setLastError(qMakeError(
                QCoreApplication::translate("QMimerSQLDriver", "Could not start transaction"), err,
                QSqlError::TransactionError, d));
        return false;
    }
    return true;
}

bool QMimerSQLDriver::commitTransaction()
{
    Q_D(const QMimerSQLDriver);
    const int32_t err = MimerEndTransaction(d->sessionhandle, MIMER_COMMIT);
    if (!MIMER_SUCCEEDED(err)) {
        setLastError(qMakeError(
                QCoreApplication::translate("QMimerSQLDriver", "Could not commit transaction"), err,
                QSqlError::TransactionError, d));
        return false;
    }
    return true;
}

bool QMimerSQLDriver::rollbackTransaction()
{
    Q_D(const QMimerSQLDriver);
    const int32_t err = MimerEndTransaction(d->sessionhandle, MIMER_ROLLBACK);
    if (!MIMER_SUCCEEDED(err)) {
        setLastError(qMakeError(
                QCoreApplication::translate("QMimerSQLDriver", "Could not roll back transaction"),
                err, QSqlError::TransactionError, d));
        return false;
    }
    return true;
}

void QMimerSQLDriverPrivate::splitTableQualifier(const QString &qualifiedName, QString *schema,
                                                 QString *table) const
{
    const QList<QStringView> l = QStringView(qualifiedName).split(u'.');
    int n = l.count();
    if (n > 2) {
        return; // can't possibly be a valid table qualifier
    } else if (n == 1) {
        *schema = QString();
        *table = l.at(0).toString();
    } else {
        *schema = l.at(0).toString();
        *table = l.at(1).toString();
    }
}

QT_END_NAMESPACE

#include "moc_qsql_mimer.cpp"
