/****************************************************************************
**
** Copyright (C) 2018 The Qt Company Ltd.
** Copyright (C) 2018 Intel Corporation.
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

#ifndef QUTFCODEC_P_H
#define QUTFCODEC_P_H

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

#include <QtCore/qstring.h>
#include <QtCore/qlist.h>
#include <QtCore/qendian.h>

#if QT_CONFIG(textcodec)
#include "QtCore/qtextcodec.h"
#endif

#include "private/qstringconverter_p.h"
#include "private/qtextcodec_p.h"

QT_BEGIN_NAMESPACE

#if QT_CONFIG(textcodec)

class QUtf8Codec : public QTextCodec {
public:
    ~QUtf8Codec();

    QByteArray name() const override;
    int mibEnum() const override;

    QString convertToUnicode(const char *, int, ConverterState *) const override;
    QByteArray convertFromUnicode(const QChar *, int, ConverterState *) const override;
    void convertToUnicode(QString *target, const char *, int, ConverterState *) const;
};

class QUtf16Codec : public QTextCodec {
protected:
public:
    QUtf16Codec() { e = DetectEndianness; }
    ~QUtf16Codec();

    QByteArray name() const override;
    QList<QByteArray> aliases() const override;
    int mibEnum() const override;

    QString convertToUnicode(const char *, int, ConverterState *) const override;
    QByteArray convertFromUnicode(const QChar *, int, ConverterState *) const override;

protected:
    DataEndianness e;
};

class QUtf16BECodec : public QUtf16Codec {
public:
    QUtf16BECodec() : QUtf16Codec() { e = BigEndianness; }
    QByteArray name() const override;
    QList<QByteArray> aliases() const override;
    int mibEnum() const override;
};

class QUtf16LECodec : public QUtf16Codec {
public:
    QUtf16LECodec() : QUtf16Codec() { e = LittleEndianness; }
    QByteArray name() const override;
    QList<QByteArray> aliases() const override;
    int mibEnum() const override;
};

class QUtf32Codec : public QTextCodec {
public:
    QUtf32Codec() { e = DetectEndianness; }
    ~QUtf32Codec();

    QByteArray name() const override;
    QList<QByteArray> aliases() const override;
    int mibEnum() const override;

    QString convertToUnicode(const char *, int, ConverterState *) const override;
    QByteArray convertFromUnicode(const QChar *, int, ConverterState *) const override;

protected:
    DataEndianness e;
};

class QUtf32BECodec : public QUtf32Codec {
public:
    QUtf32BECodec() : QUtf32Codec() { e = BigEndianness; }
    QByteArray name() const override;
    QList<QByteArray> aliases() const override;
    int mibEnum() const override;
};

class QUtf32LECodec : public QUtf32Codec {
public:
    QUtf32LECodec() : QUtf32Codec() { e = LittleEndianness; }
    QByteArray name() const override;
    QList<QByteArray> aliases() const override;
    int mibEnum() const override;
};


#endif // textcodec

QT_END_NAMESPACE

#endif // QUTFCODEC_P_H
