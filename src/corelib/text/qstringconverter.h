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
#if defined(QT_USE_FAST_OPERATOR_PLUS) || defined(QT_USE_QSTRINGBUILDER)
#include <QtCore/qstringbuilder.h>
#endif

#include <optional>

QT_BEGIN_NAMESPACE

// work around a compiler bug in GCC 7
#if (defined(Q_CC_GNU) && __GNUC__ == 7) || defined(Q_QDOC)
#define QSTRINGCONVERTER_CONSTEXPR
#else
#define QSTRINGCONVERTER_CONSTEXPR constexpr
#endif

class QStringConverterBase
{
public:
    enum class Flag {
        Default = 0,
        Stateless = 0x1,
        ConvertInvalidToNull = 0x2,
        WriteBom = 0x4,
        ConvertInitialBom = 0x8
    };
    Q_DECLARE_FLAGS(Flags, Flag)

    struct State {
        constexpr State(Flags f = Flag::Default)
            : flags(f), state_data{0, 0, 0, 0} {}
        ~State() { clear(); }
        State(State &&other)
            : flags(other.flags),
              remainingChars(other.remainingChars),
              invalidChars(other.invalidChars),
              d{other.d[0], other.d[1]},
              clearFn(other.clearFn)
        { other.clearFn = nullptr; }
        State &operator=(State &&other)
        {
            clear();
            flags = other.flags;
            remainingChars = other.remainingChars;
            invalidChars = other.invalidChars;
            d[0] = other.d[0];
            d[1] = other.d[1];
            clearFn = other.clearFn;
            other.clearFn = nullptr;
            return *this;
        }
        Q_CORE_EXPORT void clear();

        Flags flags;
        int internalState = 0;
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
        Latin1,
        System,
        LastEncoding = System
    };
#ifdef Q_QDOC
    // document the flags here
    enum class Flag {
        Default = 0,
        Stateless = 0x1,
        ConvertInvalidToNull = 0x2,
        WriteBom = 0x4,
        ConvertInitialBom = 0x8
    };
    Q_DECLARE_FLAGS(Flags, Flag)
#endif

protected:

    struct Interface
    {
        using DecoderFn = QChar * (*)(QChar *out, QByteArrayView in, State *state);
        using LengthFn = qsizetype (*)(qsizetype inLength);
        using EncoderFn = char * (*)(char *out, QStringView in, State *state);
        const char *name = nullptr;
        DecoderFn toUtf16 = nullptr;
        LengthFn toUtf16Len = nullptr;
        EncoderFn fromUtf16 = nullptr;
        LengthFn fromUtf16Len = nullptr;
    };

    QSTRINGCONVERTER_CONSTEXPR QStringConverter()
        : iface(nullptr)
    {}
    QSTRINGCONVERTER_CONSTEXPR QStringConverter(Encoding encoding, Flags f)
        : iface(&encodingInterfaces[int(encoding)]), state(f)
    {}
    QSTRINGCONVERTER_CONSTEXPR QStringConverter(const Interface *i)
        : iface(i)
    {}
    Q_CORE_EXPORT QStringConverter(const char *name, Flags f);


public:
    bool isValid() const { return iface != nullptr; }

    void resetState()
    {
        state.clear();
    }
    bool hasError() const { return state.invalidChars != 0; }

    const char *name() const
    { return isValid() ? iface->name : nullptr; }

    Q_CORE_EXPORT static std::optional<Encoding> encodingForName(const char *name);
    Q_CORE_EXPORT static const char *nameForEncoding(Encoding e);
    Q_CORE_EXPORT static std::optional<Encoding> encodingForData(QByteArrayView data, char16_t expectedFirstCharacter = 0);
    Q_CORE_EXPORT static std::optional<Encoding> encodingForHtml(QByteArrayView data);

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
    QSTRINGCONVERTER_CONSTEXPR QStringEncoder()
        : QStringConverter()
    {}
    QSTRINGCONVERTER_CONSTEXPR QStringEncoder(Encoding encoding, Flags flags = Flag::Default)
        : QStringConverter(encoding, flags)
    {}
    QStringEncoder(const char *name, Flags flags = Flag::Default)
        : QStringConverter(name, flags)
    {}

#if defined(Q_QDOC)
    QByteArray operator()(const QString &in);
    QByteArray operator()(QStringView in);
    QByteArray encode(const QString &in);
    QByteArray encode(QStringView in);
#else
    template<typename T>
    struct DecodedData
    {
        QStringEncoder *encoder;
        T data;
        operator QByteArray() const { return encoder->encodeAsByteArray(data); }
    };
    Q_WEAK_OVERLOAD
    DecodedData<const QString &> operator()(const QString &str)
    { return DecodedData<const QString &>{this, str}; }
    DecodedData<QStringView> operator()(QStringView in)
    { return DecodedData<QStringView>{this, in}; }
    Q_WEAK_OVERLOAD
    DecodedData<const QString &> encode(const QString &str)
    { return DecodedData<const QString &>{this, str}; }
    DecodedData<QStringView> encode(QStringView in)
    { return DecodedData<QStringView>{this, in}; }
#endif

    qsizetype requiredSpace(qsizetype inputLength) const
    { return iface->fromUtf16Len(inputLength); }
    char *appendToBuffer(char *out, QStringView in)
    { return iface->fromUtf16(out, in, &state); }
private:
    QByteArray encodeAsByteArray(QStringView in)
    {
        QByteArray result(iface->fromUtf16Len(in.size()), Qt::Uninitialized);
        char *out = result.data();
        out = iface->fromUtf16(out, in, &state);
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
    QSTRINGCONVERTER_CONSTEXPR QStringDecoder(Encoding encoding, Flags flags = Flag::Default)
        : QStringConverter(encoding, flags)
    {}
    QSTRINGCONVERTER_CONSTEXPR QStringDecoder()
        : QStringConverter()
    {}
    QStringDecoder(const char *name, Flags f = Flag::Default)
        : QStringConverter(name, f)
    {}

#if defined(Q_QDOC)
    QString operator()(const QByteArray &ba);
    QString operator()(QByteArrayView ba);
    QString decode(const QByteArray &ba);
    QString decode(QByteArrayView ba);
#else
    template<typename T>
    struct EncodedData
    {
        QStringDecoder *decoder;
        T data;
        operator QString() const { return decoder->decodeAsString(data); }
    };
    Q_WEAK_OVERLOAD
    EncodedData<const QByteArray &> operator()(const QByteArray &ba)
    { return EncodedData<const QByteArray &>{this, ba}; }
    EncodedData<QByteArrayView> operator()(QByteArrayView ba)
    { return EncodedData<QByteArrayView>{this, ba}; }
    Q_WEAK_OVERLOAD
    EncodedData<const QByteArray &> decode(const QByteArray &ba)
    { return EncodedData<const QByteArray &>{this, ba}; }
    EncodedData<QByteArrayView> decode(QByteArrayView ba)
    { return EncodedData<QByteArrayView>{this, ba}; }
#endif

    qsizetype requiredSpace(qsizetype inputLength) const
    { return iface->toUtf16Len(inputLength); }
    QChar *appendToBuffer(QChar *out, QByteArrayView ba)
    { return iface->toUtf16(out, ba, &state); }
private:
    QString decodeAsString(QByteArrayView in)
    {
        QString result(iface->toUtf16Len(in.size()), Qt::Uninitialized);
        const QChar *out = iface->toUtf16(result.data(), in, &state);
        result.truncate(out - result.constData());
        return result;
    }
};


#if defined(QT_USE_FAST_OPERATOR_PLUS) || defined(QT_USE_QSTRINGBUILDER)
template <typename T>
struct QConcatenable<QStringDecoder::EncodedData<T>>
        : private QAbstractConcatenable
{
    typedef QChar type;
    typedef QString ConvertTo;
    enum { ExactSize = false };
    static qsizetype size(const QStringDecoder::EncodedData<T> &s) { return s.decoder->requiredSpace(s.data.size()); }
    static inline void appendTo(const QStringDecoder::EncodedData<T> &s, QChar *&out)
    {
        out = s.decoder->appendToBuffer(out, s.data);
    }
};

template <typename T>
struct QConcatenable<QStringEncoder::DecodedData<T>>
        : private QAbstractConcatenable
{
    typedef char type;
    typedef QByteArray ConvertTo;
    enum { ExactSize = false };
    static qsizetype size(const QStringEncoder::DecodedData<T> &s) { return s.encoder->requiredSpace(s.data.size()); }
    static inline void appendTo(const QStringEncoder::DecodedData<T> &s, char *&out)
    {
        out = s.encoder->appendToBuffer(out, s.data);
    }
};

template <typename T>
QString &operator+=(QString &a, const QStringDecoder::EncodedData<T> &b)
{
    qsizetype len = a.size() + QConcatenable<QStringDecoder::EncodedData<T>>::size(b);
    a.reserve(len);
    QChar *it = a.data() + a.size();
    QConcatenable<QStringDecoder::EncodedData<T>>::appendTo(b, it);
    a.resize(qsizetype(it - a.constData())); //may be smaller than len
    return a;
}

template <typename T>
QByteArray &operator+=(QByteArray &a, const QStringEncoder::DecodedData<T> &b)
{
    qsizetype len = a.size() + QConcatenable<QStringEncoder::DecodedData<T>>::size(b);
    a.reserve(len);
    char *it = a.data() + a.size();
    QConcatenable<QStringEncoder::DecodedData<T>>::appendTo(b, it);
    a.resize(qsizetype(it - a.constData())); //may be smaller than len
    return a;
}
#endif

QT_END_NAMESPACE

#undef QSTRINGCONVERTER_CONSTEXPR

#endif
