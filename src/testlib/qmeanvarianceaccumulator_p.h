// Copyright (C) 2025 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#ifndef QMEANVARIANCEACCUMULATOR_P_H
#define QMEANVARIANCEACCUMULATOR_P_H

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

#include <QtTest/private/qbenchmarkmeasurement_p.h>

QT_BEGIN_NAMESPACE

namespace QTestPrivate {

class MeanAndVarianceAccumulator
{

    qsizetype m_count = 0;
    qreal m_mean = 0;
    qreal m_sqdiff = 0;

public:
    MeanAndVarianceAccumulator() = default;
    void update(qreal value)
    {
        const qreal gap = value - m_mean;
        ++m_count;
        m_mean += gap / m_count;
        m_sqdiff += gap * (value - m_mean);
    }
    qsizetype count() const { return m_count; }
    qreal mean() const { return m_mean; }
    qreal variance() const { return m_count > 1 ? m_sqdiff / (m_count - 1) : 0;  }
    qreal total() const { return m_mean * m_count; }
    void reset() { m_mean = 0; m_sqdiff = 0; m_count = 0; }
};

} // namespace QTestPrivate

QT_END_NAMESPACE

#endif // QBENCHMARKTIMEMEASURERS_P_H
