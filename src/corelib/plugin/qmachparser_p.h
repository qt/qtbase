/****************************************************************************
**
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

#ifndef QMACHPARSER_P_H
#define QMACHPARSER_P_H

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

#if defined(Q_OF_MACH_O)

QT_BEGIN_NAMESPACE

class QString;
class QLibraryPrivate;

class Q_AUTOTEST_EXPORT QMachOParser
{
public:
    enum { QtMetaDataSection, NoQtSection, NotSuitable };
    static int parse(const char *m_s, ulong fdlen, const QString &library, QString *errorString, qsizetype *pos, qsizetype *sectionlen);
};

QT_END_NAMESPACE

#endif // defined(Q_OF_ELF) && defined(Q_CC_GNU)

#endif // QMACHPARSER_P_H
