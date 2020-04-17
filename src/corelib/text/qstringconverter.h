/****************************************************************************
**
** Copyright (C) 2020 The Qt Company Ltd.
** Contact: https://www.qt.io/licensing/
**
** This file is part of the QtCore module of the Qt Toolkit.
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

#ifndef QSTRINGCONVERTER_H
#define QSTRINGCONVERTER_H

#include <QtCore/qstring.h>

QT_BEGIN_NAMESPACE

// work around a compiler bug in GCC 7
#if defined(Q_CC_GNU) && __GNUC__ == 7
#define QSTRINGCONVERTER_CONSTEXPR
#else
#define QSTRINGCONVERTER_CONSTEXPR constexpr
#endif

class QStringConverterBase
{
public:
    enum Flag {
        DefaultConversion,
        ConvertInvalidToNull = 0x1,
        IgnoreHeader = 0x2,
        Stateless = 0x4
    };
    Q_DECLARE_FLAGS(Flags, Flag)

    struct State {
        constexpr State(Flags f = DefaultConversion)
            : flags(f), state_data{0, 0, 0, 0} {}
        ~State() { clear(); }
        Q_CORE_EXPORT void clear();

        Flags flags;
        qsizetype remainingChars = 0;
        qsizetype invalidChars = 0;

        union {
            uint state_data[4];
            void *d[2];
        };
        using ClearDataFn = void (*)(State *);
        ClearDataFn clearFn = nullptr;
    private:
        Q_DISABLE_COPY(State)
    };
};
Q_DECLARE_OPERATORS_FOR_FLAGS(QStringConverterBase::Flags)

class QStringConverter : public QStringConverterBase
{
public:

    enum Encoding {
        Utf8,
        Utf16,
        Utf16LE,
        Utf16BE,
        Utf32,
        Utf32LE,
        Utf32BE,
        Locale,
        LastEncoding = Locale
    };
protected:

    struct Interface
    {
        // ### FIXME: need a QByteArrayView
        using DecoderFn = QChar * (*)(QChar *out, const char *in, qsizetype length, State *state);
        using LengthFn = qsizetype (*)(qsizetype inLength);
        using EncoderFn = char * (*)(char *out, QStringView in, State *state);
        DecoderFn toUtf16 = nullptr;
        LengthFn toUtf16Len = nullptr;
        EncoderFn fromUtf16 = nullptr;
        LengthFn fromUtf16Len = nullptr;
    };

    QSTRINGCONVERTER_CONSTEXPR QStringConverter(Encoding encoding, Flags f)
        : iface(&encodingInterfaces[int(encoding)]), state(f)
    {}
    QSTRINGCONVERTER_CONSTEXPR QStringConverter(const Interface *i)
        : iface(i)
    {}

public:
    bool isValid() const { return iface != nullptr; }

    void resetState()
    {
        state.clear();
    }
    bool hasError() const { return state.invalidChars != 0; }

protected:
    const Interface *iface;
    State state;
private:
    Q_CORE_EXPORT static const Interface encodingInterfaces[Encoding::LastEncoding + 1];
};

class QStringEncoder : public QStringConverter
{
protected:
    QSTRINGCONVERTER_CONSTEXPR QStringEncoder(const Interface *i)
        : QStringConverter(i)
    {}
public:
    // ### We shouldn't write a BOM by default. Need to resolve this
    // while keeping compat with QTextCodec
    QSTRINGCONVERTER_CONSTEXPR QStringEncoder(Encoding encoding, Flags flags = IgnoreHeader)
        : QStringConverter(encoding, flags)
    {}

    QByteArray operator()(const QChar *in, qsizetype length)
    { return (*this)(QStringView(in, length)); }

    QByteArray operator()(QStringView in)
    {
        QByteArray result(iface->fromUtf16Len(in.size()), Qt::Uninitialized);
        char *out = result.data();
        // ### Fixme: needs to be moved into the conversion methods to honor the other flags
        out = iface->fromUtf16(out, in, state.flags & Stateless ? nullptr : &state);
        result.truncate(out - result.constData());
        return result;
    }
};

class QStringDecoder : public QStringConverter
{
protected:
    QSTRINGCONVERTER_CONSTEXPR QStringDecoder(const Interface *i)
        : QStringConverter(i)
    {}
public:
    QSTRINGCONVERTER_CONSTEXPR QStringDecoder(Encoding encoding, Flags flags = DefaultConversion)
        : QStringConverter(encoding, flags)
    {}

    QString operator()(const char *in, qsizetype length)
    {
        QString result(iface->toUtf16Len(length), Qt::Uninitialized);
        QChar *out  = result.data();
        // ### Fixme: needs to be moved into the conversion methods
        out = iface->toUtf16(out, in, length, state.flags & Stateless ? nullptr : &state);
        result.truncate(out - result.constData());
        return result;
    }
    QString operator()(const QByteArray &ba)
    { return (*this)(ba.constData(), ba.size()); }
    QString operator()(const char *chars)
    { return (*this)(chars, strlen(chars)); }
};

QT_END_NAMESPACE

#undef QSTRINGCONVERTER_CONSTEXPR

#endif
