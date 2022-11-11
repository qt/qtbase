// Copyright (C) 2016 The Qt Company Ltd.
// Copyright (C) 2016 Intel Corporation.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#ifndef QTEXTSTREAM_P_H
#define QTEXTSTREAM_P_H

//
//  W A R N I N G
//  -------------
//
// This file is not part of the Qt API.  It exists purely as an
// implementation detail.  This header file may change from version to
// version without notice, or even be removed.
//
// We mean it.
//

#include <QtCore/private/qglobal_p.h>
#include <QtCore/qstringconverter.h>
#include <QtCore/qiodevice.h>
#include <QtCore/qlocale.h>
#include "qtextstream.h"

QT_BEGIN_NAMESPACE

#ifndef QT_NO_QOBJECT
class QDeviceClosedNotifier : public QObject
{
    Q_OBJECT
public:
    inline QDeviceClosedNotifier()
    { }

    inline void setupDevice(QTextStream *stream, QIODevice *device)
    {
        disconnect();
        if (device) {
            // Force direct connection here so that QTextStream can be used
            // from multiple threads when the application code is handling
            // synchronization (see also QTBUG-12055).
            connect(device, SIGNAL(aboutToClose()), this, SLOT(flushStream()),
                    Qt::DirectConnection);
        }
        this->stream = stream;
    }

public Q_SLOTS:
    inline void flushStream() { stream->flush(); }

private:
    QTextStream *stream;
};
#endif

class QTextStreamPrivate
{
    Q_DECLARE_PUBLIC(QTextStream)
public:
    // streaming parameters
    class Params
    {
    public:
        void reset();

        int realNumberPrecision;
        int integerBase;
        int fieldWidth;
        QChar padChar;
        QTextStream::FieldAlignment fieldAlignment;
        QTextStream::RealNumberNotation realNumberNotation;
        QTextStream::NumberFlags numberFlags;
    };

    QTextStreamPrivate(QTextStream *q_ptr);
    ~QTextStreamPrivate();
    void reset();

    // device
    QIODevice *device;
#ifndef QT_NO_QOBJECT
    QDeviceClosedNotifier deviceClosedNotifier;
#endif

    // string
    QString *string;
    int stringOffset;
    QIODevice::OpenMode stringOpenMode;

    QStringConverter::Encoding encoding = QStringConverter::Utf8;
    QStringEncoder fromUtf16;
    QStringDecoder toUtf16;
    QStringDecoder savedToUtf16;

    QString writeBuffer;
    QString readBuffer;
    int readBufferOffset;
    int readConverterSavedStateOffset; //the offset between readBufferStartDevicePos and that start of the buffer
    qint64 readBufferStartDevicePos;

    Params params;

    // status
    QTextStream::Status status;
    QLocale locale;
    QTextStream *q_ptr;

    int lastTokenSize;
    bool deleteDevice;
    bool autoDetectUnicode;
    bool hasWrittenData = false;
    bool generateBOM = false;

    // i/o
    enum TokenDelimiter {
        Space,
        NotSpace,
        EndOfLine
    };

    QString read(int maxlen);
    bool scan(const QChar **ptr, int *tokenLength,
              int maxlen, TokenDelimiter delimiter);
    inline const QChar *readPtr() const;
    inline void consumeLastToken();
    inline void consume(int nchars);
    void saveConverterState(qint64 newPos);
    void restoreToSavedConverterState();

    // Return value type for getNumber()
    enum NumberParsingStatus {
        npsOk,
        npsMissingDigit,
        npsInvalidPrefix
    };

    inline bool getChar(QChar *ch);
    inline void ungetChar(QChar ch);
    NumberParsingStatus getNumber(qulonglong *l);
    bool getReal(double *f);

    inline void write(QStringView data) { write(data.begin(), data.size()); }
    inline void write(QChar ch);
    void write(const QChar *data, qsizetype len);
    void write(QLatin1StringView data);
    void writePadding(qsizetype len);
    inline void putString(QStringView string, bool number = false)
    {
        putString(string.constData(), string.size(), number);
    }
    void putString(const QChar *data, qsizetype len, bool number = false);
    void putString(QLatin1StringView data, bool number = false);
    void putString(QUtf8StringView data, bool number = false);
    inline void putChar(QChar ch);
    void putNumber(qulonglong number, bool negative);

    struct PaddingResult {
        int left, right;
    };
    PaddingResult padding(qsizetype len) const;

    // buffers
    bool fillReadBuffer(qint64 maxBytes = -1);
    void resetReadBuffer();
    void flushWriteBuffer();
};

QT_END_NAMESPACE

#endif // QTEXTSTREAM_P_H
