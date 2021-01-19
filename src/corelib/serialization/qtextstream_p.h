/****************************************************************************
**
** Copyright (C) 2021 The Qt Company Ltd.
** Copyright (C) 2016 Intel Corporation.
** Contact: https://www.qt.io/licensing/
**
** This file is part of the QtCore module of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:COMM$
**
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company. For licensing terms
** and conditions see https://www.qt.io/terms-conditions. For further
** information use the contact form at https://www.qt.io/contact-us.
**
** $QT_END_LICENSE$
**
**
**
**
**
**
**
**
**
**
**
**
**
**
**
**
**
**
**
****************************************************************************/

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
#include "qtextstream.h"
#if QT_CONFIG(textcodec)
#include "qtextcodec.h"
#endif

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
        if (device)
            connect(device, SIGNAL(aboutToClose()), this, SLOT(flushStream()));
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

#if QT_CONFIG(textcodec)
    // codec
    QTextCodec *codec;
    QTextCodec::ConverterState readConverterState;
    QTextCodec::ConverterState writeConverterState;
    QTextCodec::ConverterState *readConverterSavedState;
#endif

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
#if QT_CONFIG(textcodec)
    bool autoDetectUnicode;
#endif

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

    inline void write(const QString &data) { write(data.begin(), data.length()); }
    inline void write(QChar ch);
    void write(const QChar *data, int len);
    void write(QLatin1String data);
    void writePadding(int len);
    inline void putString(const QString &ch, bool number = false) { putString(ch.constData(), ch.length(), number); }
    void putString(const QChar *data, int len, bool number = false);
    void putString(QLatin1String data, bool number = false);
    inline void putChar(QChar ch);
    void putNumber(qulonglong number, bool negative);

    struct PaddingResult {
        int left, right;
    };
    PaddingResult padding(int len) const;

    // buffers
    bool fillReadBuffer(qint64 maxBytes = -1);
    void resetReadBuffer();
    void flushWriteBuffer();
};

QT_END_NAMESPACE

#endif // QTEXTSTREAM_P_H
