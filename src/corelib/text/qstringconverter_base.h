/****************************************************************************
**
** Copyright (C) 2022 The Qt Company Ltd.
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

#ifndef QSTRINGCONVERTER_BASE_H
#define QSTRINGCONVERTER_BASE_H

#if 0
// QStringConverter(Base) class are handled in qstringconverter
#pragma qt_sync_stop_processing
#endif

#include <optional>

#include <QtCore/qglobal.h> // QT_{BEGIN,END}_NAMESPACE
#include <QtCore/qflags.h> // Q_DECLARE_FLAGS

#include <cstring>

QT_BEGIN_NAMESPACE

class QByteArrayView;
class QChar;
class QByteArrayView;
class QStringView;

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
        State(State &&other) noexcept
            : flags(other.flags),
              remainingChars(other.remainingChars),
              invalidChars(other.invalidChars),
              state_data{other.state_data[0], other.state_data[1],
                         other.state_data[2], other.state_data[3]},
              clearFn(other.clearFn)
        { other.clearFn = nullptr; }
        State &operator=(State &&other) noexcept
        {
            clear();
            flags = other.flags;
            remainingChars = other.remainingChars;
            invalidChars = other.invalidChars;
            std::memmove(state_data, other.state_data, sizeof state_data); // self-assignment-safe
            clearFn = other.clearFn;
            other.clearFn = nullptr;
            return *this;
        }
        Q_CORE_EXPORT void clear() noexcept;

        Flags flags;
        int internalState = 0;
        qsizetype remainingChars = 0;
        qsizetype invalidChars = 0;

        union {
            uint state_data[4];
            void *d[2];
        };
        using ClearDataFn = void (*)(State *) noexcept;
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

    constexpr QStringConverter()
        : iface(nullptr)
    {}
    constexpr explicit QStringConverter(Encoding encoding, Flags f)
        : iface(&encodingInterfaces[int(encoding)]), state(f)
    {}
    constexpr explicit QStringConverter(const Interface *i)
        : iface(i)
    {}
    Q_CORE_EXPORT explicit QStringConverter(const char *name, Flags f);


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

QT_END_NAMESPACE

#endif
