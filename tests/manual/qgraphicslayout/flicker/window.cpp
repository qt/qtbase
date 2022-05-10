// Copyright (C) 2020 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "window.h"

void SlowWidget::setGeometry(const QRectF &rect)
{
    bool reiterate = false;
    Statistics &stats = *m_stats;
    if (stats.relayoutClicked) {
        ++(stats.setGeometryTracker[this]);
        ++stats.setGeometryCount;
        qDebug() << "setGeometryCount:" << stats.setGeometryCount;
        if (stats.setGeometryTracker.count() == m_window->m_depthSpinBox->value()) {
            ++stats.currentBenchmarkIteration;
            qDebug() << "currentBenchmarkIteration:" << stats.currentBenchmarkIteration;
            if (stats.currentBenchmarkIteration == m_window->m_benchmarkIterationsSpinBox->value()) {
                if (stats.output)
                    stats.output->setText(tr("DONE. Elapsed: %1, setGeometryCount: %2").arg(stats.timer.elapsed()).arg(stats.setGeometryCount));
            } else {
                reiterate = true;
            }
            stats.setGeometryTracker.clear();

        }
    }

    QGraphicsWidget::setGeometry(rect);

    if (reiterate) {
        m_window->doAgain();
        //QTimer::singleShot(0, m_window, SLOT(doAgain()));
    }
}
