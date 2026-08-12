// Copyright (C) 2026 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only
// Qt-Security score:significant reason:default
#ifndef QPARSE_QT_TEMPORAL_FORMAT_P_H
#define QPARSE_QT_TEMPORAL_FORMAT_P_H

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

#include "private/qtparsecommon_p.h"
#include "private/qttemporalpattern_p.h"

QT_BEGIN_NAMESPACE

namespace QtParseQtTemporalFormat {

struct ParsedDateTimeFormat : public QtParseCommon::ParsedText
{
    QList<QtTemporalPattern::TemporalField> fields;
};

Q_AUTOTEST_EXPORT
ParsedDateTimeFormat prefix(QStringView pattern, QtTemporalPattern::DateTimeParts form);
}

QT_END_NAMESPACE

#endif // QPARSE_QT_TEMPORAL_FORMAT_P_H
