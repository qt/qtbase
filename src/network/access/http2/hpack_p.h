/****************************************************************************
**
** Copyright (C) 2021 The Qt Company Ltd.
** Contact: https://www.qt.io/licensing/
**
** This file is part of the QtNetwork module of the Qt Toolkit.
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

#ifndef HPACK_P_H
#define HPACK_P_H

//
//  W A R N I N G
//  -------------
//
// This file is not part of the Qt API.  It exists for the convenience
// of other Qt classes.  This header file may change from version to
// version without notice, or even be removed.
//
// We mean it.
//

#include "hpacktable_p.h"

#include <QtCore/qglobal.h>

#include <vector>

QT_BEGIN_NAMESPACE

class QByteArray;

namespace HPack
{

using HttpHeader = std::vector<HeaderField>;
HeaderSize header_size(const HttpHeader &header);

class Q_AUTOTEST_EXPORT Encoder
{
public:
    Encoder(quint32 maxTableSize, bool compressStrings);

    quint32 dynamicTableSize() const;

    bool encodeRequest(class BitOStream &outputStream,
                       const HttpHeader &header);
    bool encodeResponse(BitOStream &outputStream,
                        const HttpHeader &header);

    bool encodeSizeUpdate(BitOStream &outputStream,
                          quint32 newSize);

    void setMaxDynamicTableSize(quint32 size);
    void setCompressStrings(bool compress);

private:
    bool encodeRequestPseudoHeaders(BitOStream &outputStream,
                                    const HttpHeader &header);
    bool encodeHeaderField(BitOStream &outputStream,
                           const HeaderField &field);
    bool encodeMethod(BitOStream &outputStream,
                      const HeaderField &field);

    bool encodeResponsePseudoHeaders(BitOStream &outputStream,
                                     const HttpHeader &header);

    bool encodeIndexedField(BitOStream &outputStream, quint32 index) const;


    bool encodeLiteralField(BitOStream &outputStream,
                            const struct BitPattern &fieldType,
                            quint32 nameIndex,
                            const QByteArray &value,
                            bool withCompression);

    bool encodeLiteralField(BitOStream &outputStream,
                            const BitPattern &fieldType,
                            const QByteArray &name,
                            const QByteArray &value,
                            bool withCompression);

    FieldLookupTable lookupTable;
    bool compressStrings;
};

class Q_AUTOTEST_EXPORT Decoder
{
public:
    Decoder(quint32 maxTableSize);

    bool decodeHeaderFields(class BitIStream &inputStream);

    const HttpHeader &decodedHeader() const
    {
        return header;
    }

    quint32 dynamicTableSize() const;

    void setMaxDynamicTableSize(quint32 size);

private:

    bool decodeIndexedField(BitIStream &inputStream);
    bool decodeSizeUpdate(BitIStream &inputStream);
    bool decodeLiteralField(const BitPattern &fieldType,
                            BitIStream &inputStream);

    bool processDecodedField(const BitPattern &fieldType,
                             const QByteArray &name,
                             const QByteArray &value);

    void handleStreamError(BitIStream &inputStream);

    HttpHeader header;
    FieldLookupTable lookupTable;
};

}

QT_END_NAMESPACE

#endif

