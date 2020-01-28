/****************************************************************************
**
** Copyright (C) 2018 Intel Corporation.
** Contact: https://www.qt.io/licensing/
**
** This file is part of the examples of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:BSD$
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company. For licensing terms
** and conditions see https://www.qt.io/terms-conditions. For further
** information use the contact form at https://www.qt.io/contact-us.
**
** BSD License Usage
** Alternatively, you may use this file under the terms of the BSD license
** as follows:
**
** "Redistribution and use in source and binary forms, with or without
** modification, are permitted provided that the following conditions are
** met:
**   * Redistributions of source code must retain the above copyright
**     notice, this list of conditions and the following disclaimer.
**   * Redistributions in binary form must reproduce the above copyright
**     notice, this list of conditions and the following disclaimer in
**     the documentation and/or other materials provided with the
**     distribution.
**   * Neither the name of The Qt Company Ltd nor the names of its
**     contributors may be used to endorse or promote products derived
**     from this software without specific prior written permission.
**
**
** THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
** "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
** LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
** A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
** OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
** SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
** LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
** DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
** THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
** (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
** OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE."
**
** $QT_END_LICENSE$
**
****************************************************************************/

#include "cborconverter.h"

#include <QCborStreamReader>
#include <QCborStreamWriter>
#include <QCborMap>
#include <QCborArray>
#include <QCborValue>
#include <QDataStream>
#include <QFloat16>
#include <QFile>
#include <QMetaType>
#include <QTextStream>

#include <stdio.h>

static CborConverter cborConverter;
static CborDiagnosticDumper cborDiagnosticDumper;

static const char optionHelp[] =
        "convert-float-to-int=yes|no    Write integers instead of floating point, if no\n"
        "                               loss of precision occurs on conversion.\n"
        "float16=yes|always|no          Write using half-precision floating point.\n"
        "                               If 'always', won't check for loss of precision.\n"
        "float32=yes|always|no          Write using single-precision floating point.\n"
        "                               If 'always', won't check for loss of precision.\n"
        "signature=yes|no               Prepend the CBOR signature to the file output.\n"
        ;

static const char diagnosticHelp[] =
        "extended=no|yes                Use extended CBOR diagnostic format.\n"
        "line-wrap=yes|no               Split output into multiple lines.\n"
        ;

QT_BEGIN_NAMESPACE

QDataStream &operator<<(QDataStream &ds, QCborTag tag)
{
    return ds << quint64(tag);
}

QDataStream &operator>>(QDataStream &ds, QCborTag &tag)
{
    quint64 v;
    ds >> v;
    tag = QCborTag(v);
    return ds;
}

QT_END_NAMESPACE

// We can't use QCborValue::toVariant directly because that would destroy
// non-string keys in CBOR maps (QVariantMap can't handle those). Instead, we
// have our own set of converter functions so we can keep the keys properly.

static QVariant convertCborValue(const QCborValue &value);

static QVariant convertCborMap(const QCborMap &map)
{
    VariantOrderedMap result;
    result.reserve(map.size());
    for (auto pair : map)
        result.append({ convertCborValue(pair.first), convertCborValue(pair.second) });
    return QVariant::fromValue(result);
}

static QVariant convertCborArray(const QCborArray &array)
{
    QVariantList result;
    result.reserve(array.size());
    for (auto value : array)
        result.append(convertCborValue(value));
    return result;
}

static QVariant convertCborValue(const QCborValue &value)
{
    if (value.isArray())
        return convertCborArray(value.toArray());
    if (value.isMap())
        return convertCborMap(value.toMap());
    return value.toVariant();
}

enum TrimFloatingPoint { Double, Float, Float16 };
static QCborValue convertFromVariant(const QVariant &v, TrimFloatingPoint fpTrimming)
{
    if (v.userType() == QMetaType::QVariantList) {
        const QVariantList list = v.toList();
        QCborArray array;
        for (const QVariant &v : list)
            array.append(convertFromVariant(v, fpTrimming));

        return array;
    }

    if (v.userType() == qMetaTypeId<VariantOrderedMap>()) {
        const auto m = qvariant_cast<VariantOrderedMap>(v);
        QCborMap map;
        for (const auto &pair : m)
            map.insert(convertFromVariant(pair.first, fpTrimming),
                       convertFromVariant(pair.second, fpTrimming));
        return map;
    }

    if (v.userType() == QMetaType::Double && fpTrimming != Double) {
        float f = float(v.toDouble());
        if (fpTrimming == Float16)
            return float(qfloat16(f));
        return f;
    }

    return QCborValue::fromVariant(v);
}

QString CborDiagnosticDumper::name()
{
    return QStringLiteral("cbor-dump");
}

Converter::Direction CborDiagnosticDumper::directions()
{
    return Out;
}

Converter::Options CborDiagnosticDumper::outputOptions()
{
    return SupportsArbitraryMapKeys;
}

const char *CborDiagnosticDumper::optionsHelp()
{
    return diagnosticHelp;
}

bool CborDiagnosticDumper::probeFile(QIODevice *f)
{
    Q_UNUSED(f);
    return false;
}

QVariant CborDiagnosticDumper::loadFile(QIODevice *f, Converter *&outputConverter)
{
    Q_UNREACHABLE();
    Q_UNUSED(f);
    Q_UNUSED(outputConverter);
    return QVariant();
}

void CborDiagnosticDumper::saveFile(QIODevice *f, const QVariant &contents, const QStringList &options)
{
    QCborValue::DiagnosticNotationOptions opts = QCborValue::LineWrapped;
    for (const QString &s : options) {
        QStringList pair = s.split('=');
        if (pair.size() == 2) {
            if (pair.first() == "line-wrap") {
                opts &= ~QCborValue::LineWrapped;
                if (pair.last() == "yes") {
                    opts |= QCborValue::LineWrapped;
                    continue;
                } else if (pair.last() == "no") {
                    continue;
                }
            }
            if (pair.first() == "extended") {
                opts &= ~QCborValue::ExtendedFormat;
                if (pair.last() == "yes")
                    opts |= QCborValue::ExtendedFormat;
                continue;
            }
        }

        fprintf(stderr, "Unknown CBOR diagnostic option '%s'. Available options are:\n%s",
                qPrintable(s), diagnosticHelp);
        exit(EXIT_FAILURE);
    }

    QTextStream out(f);
    out << convertFromVariant(contents, Double).toDiagnosticNotation(opts)
        << Qt::endl;
}

CborConverter::CborConverter()
{
    qRegisterMetaType<QCborTag>();
    qRegisterMetaTypeStreamOperators<QCborTag>();
    QMetaType::registerDebugStreamOperator<QCborTag>();
}

QString CborConverter::name()
{
    return "cbor";
}

Converter::Direction CborConverter::directions()
{
    return InOut;
}

Converter::Options CborConverter::outputOptions()
{
    return SupportsArbitraryMapKeys;
}

const char *CborConverter::optionsHelp()
{
    return optionHelp;
}

bool CborConverter::probeFile(QIODevice *f)
{
    if (QFile *file = qobject_cast<QFile *>(f)) {
        if (file->fileName().endsWith(QLatin1String(".cbor")))
            return true;
    }
    return f->isReadable() && f->peek(3) == QByteArray("\xd9\xd9\xf7", 3);
}

QVariant CborConverter::loadFile(QIODevice *f, Converter *&outputConverter)
{
    const char *ptr = nullptr;
    if (auto file = qobject_cast<QFile *>(f))
        ptr = reinterpret_cast<char *>(file->map(0, file->size()));

    QByteArray mapped = QByteArray::fromRawData(ptr, ptr ? f->size() : 0);
    QCborStreamReader reader(mapped);
    if (!ptr)
        reader.setDevice(f);

    if (reader.isTag() && reader.toTag() == QCborKnownTags::Signature)
        reader.next();

    QCborValue contents = QCborValue::fromCbor(reader);
    qint64 offset = reader.currentOffset();
    if (reader.lastError()) {
        fprintf(stderr, "Error loading CBOR contents (byte %lld): %s\n", offset,
                qPrintable(reader.lastError().toString()));
        fprintf(stderr, " bytes: %s\n",
                (ptr ? mapped.mid(offset, 9) : f->read(9)).toHex(' ').constData());
        exit(EXIT_FAILURE);
    } else if (offset < mapped.size() || (!ptr && f->bytesAvailable())) {
        fprintf(stderr, "Warning: bytes remaining at the end of the CBOR stream\n");
    }

    if (outputConverter == nullptr)
        outputConverter = &cborDiagnosticDumper;
    else if (outputConverter == null)
        return QVariant();
    else if (!outputConverter->outputOptions().testFlag(SupportsArbitraryMapKeys))
        return contents.toVariant();
    return convertCborValue(contents);
}

void CborConverter::saveFile(QIODevice *f, const QVariant &contents, const QStringList &options)
{
    bool useSignature = true;
    bool useIntegers = true;
    enum { Yes, No, Always } useFloat16 = Yes, useFloat = Yes;

    for (const QString &s : options) {
        QStringList pair = s.split('=');
        if (pair.size() == 2) {
            if (pair.first() == "convert-float-to-int") {
                if (pair.last() == "yes") {
                    useIntegers = true;
                    continue;
                } else if (pair.last() == "no") {
                    useIntegers = false;
                    continue;
                }
            }

            if (pair.first() == "float16") {
                if (pair.last() == "no") {
                    useFloat16 = No;
                    continue;
                } else if (pair.last() == "yes") {
                    useFloat16 = Yes;
                    continue;
                } else if (pair.last() == "always") {
                    useFloat16 = Always;
                    continue;
                }
            }

            if (pair.first() == "float32") {
                if (pair.last() == "no") {
                    useFloat = No;
                    continue;
                } else if (pair.last() == "yes") {
                    useFloat = Yes;
                    continue;
                } else if (pair.last() == "always") {
                    useFloat = Always;
                    continue;
                }
            }

            if (pair.first() == "signature") {
                if (pair.last() == "yes") {
                    useSignature = true;
                    continue;
                } else if (pair.last() == "no") {
                    useSignature = false;
                    continue;
                }
            }
        }

        fprintf(stderr, "Unknown CBOR format option '%s'. Valid options are:\n%s",
                qPrintable(s), optionHelp);
        exit(EXIT_FAILURE);
    }

    QCborValue v = convertFromVariant(contents,
                                      useFloat16 == Always ? Float16 : useFloat == Always ? Float : Double);
    QCborStreamWriter writer(f);
    if (useSignature)
        writer.append(QCborKnownTags::Signature);

    QCborValue::EncodingOptions opts;
    if (useIntegers)
        opts |= QCborValue::UseIntegers;
    if (useFloat != No)
        opts |= QCborValue::UseFloat;
    if (useFloat16 != No)
        opts |= QCborValue::UseFloat16;
    v.toCbor(writer, opts);
}

