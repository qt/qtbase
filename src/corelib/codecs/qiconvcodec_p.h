/****************************************************************************
**
** Copyright (C) 2021 The Qt Company Ltd.
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

#ifndef QICONVCODEC_P_H
#define QICONVCODEC_P_H

//
//  W A R N I N G
//  -------------
//
// This file is not part of the Qt API. It exists purely as an
// implementation detail. This header file may change from version to
// version without notice, or even be removed.
//
// We mean it.
//

#include <QtCore/private/qglobal_p.h>
#include "qtextcodec.h"

QT_REQUIRE_CONFIG(iconv);

#include <iconv.h>

QT_BEGIN_NAMESPACE

class QIconvCodec: public QTextCodec
{
private:
    mutable QTextCodec *utf16Codec;

public:
    QIconvCodec();
    ~QIconvCodec();

    QString convertToUnicode(const char *, int, ConverterState *) const override;
    QByteArray convertFromUnicode(const QChar *, int, ConverterState *) const override;

    QByteArray name() const override;
    int mibEnum() const override;

    void init() const;
    iconv_t createIconv_t(const char *to, const char *from) const;

    class IconvState
    {
    public:
        IconvState(iconv_t x);
        ~IconvState();
        ConverterState internalState;
        char *buffer;
        int bufferLen;
        iconv_t cd;

        char array[8];

        void saveChars(const char *c, int count);
    };
};

QT_END_NAMESPACE

#endif // QICONVCODEC_P_H
