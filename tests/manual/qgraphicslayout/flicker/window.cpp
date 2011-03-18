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
                    stats.output->setText(tr("DONE. Elapsed: %1, setGeometryCount: %2").arg(stats.time.elapsed()).arg(stats.setGeometryCount));
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

