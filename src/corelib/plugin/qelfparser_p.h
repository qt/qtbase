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

#ifndef QELFPARSER_P_H
#define QELFPARSER_P_H

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

#include <qendian.h>
#include <private/qglobal_p.h>

QT_REQUIRE_CONFIG(library);

#if defined (Q_OF_ELF) && defined(Q_CC_GNU)

QT_BEGIN_NAMESPACE

class QString;
class QLibraryPrivate;

typedef quint16  qelfhalf_t;
typedef quint32  qelfword_t;
typedef quintptr qelfoff_t;
typedef quintptr qelfaddr_t;

class QElfParser
{
public:
    enum { QtMetaDataSection, NoQtSection, NotElf, Corrupt };
    enum {ElfLittleEndian = 0, ElfBigEndian = 1};

    struct ElfSectionHeader
    {
        qelfword_t name;
        qelfword_t type;
        qelfoff_t  offset;
        qelfoff_t  size;
    };

    int m_endian;
    int m_bits;
    qelfoff_t m_stringTableFileOffset;

    template <typename T>
    T read(const char *s)
    {
        if (m_endian == ElfBigEndian)
            return qFromBigEndian<T>(s);
        else
            return qFromLittleEndian<T>(s);
    }

    const char *parseSectionHeader(const char* s, ElfSectionHeader *sh);
    int parse(const char *m_s, ulong fdlen, const QString &library, QLibraryPrivate *lib, qsizetype *pos, qsizetype *sectionlen);
};

QT_END_NAMESPACE

#endif // defined(Q_OF_ELF) && defined(Q_CC_GNU)

#endif // QELFPARSER_P_H
