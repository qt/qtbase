/****************************************************************************
**
** Copyright (C) 2013 Digia Plc and/or its subsidiary(-ies).
** Contact: http://www.qt-project.org/legal
**
** This file is part of the test suite of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:LGPL$
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and Digia.  For licensing terms and
** conditions see http://qt.digia.com/licensing.  For further information
** use the contact form at http://qt.digia.com/contact-us.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 2.1 as published by the Free Software
** Foundation and appearing in the file LICENSE.LGPL included in the
** packaging of this file.  Please review the following information to
** ensure the GNU Lesser General Public License version 2.1 requirements
** will be met: http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, Digia gives you certain additional
** rights.  These rights are described in the Digia Qt LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
** GNU General Public License Usage
** Alternatively, this file may be used under the terms of the GNU
** General Public License version 3.0 as published by the Free Software
** Foundation and appearing in the file LICENSE.GPL included in the
** packaging of this file.  Please review the following information to
** ensure the GNU General Public License version 3.0 requirements will be
** met: http://www.gnu.org/copyleft/gpl.html.
**
**
** $QT_END_LICENSE$
**
****************************************************************************/

#include <qtest.h>

#include <QtGui>

#include <private/qpaintengineex_p.h>
#include <private/qpaintbuffer_p.h>

class ReplayWidget : public QWidget
{
    Q_OBJECT
public:
    ReplayWidget(const QString &filename);

    void paintEvent(QPaintEvent *event);
    void resizeEvent(QResizeEvent *event);

    bool done() const { return m_done; }
    qreal result() const { return m_result; }

public slots:
    void updateRect();

public:
    QList<QRegion> updates;
    QPaintBuffer buffer;

    int currentFrame;
    int currentIteration;
    QTime timer;

    QList<uint> visibleUpdates;
    QList<uint> iterationTimes;
    QString filename;

    bool m_done;
    qreal m_result;

    uint m_total;
};

void ReplayWidget::updateRect()
{
    if (!visibleUpdates.isEmpty())
        update(updates.at(visibleUpdates.at(currentFrame)));
}

void ReplayWidget::paintEvent(QPaintEvent *)
{
    if (m_done)
        return;

    QPainter p(this);

    // if partial updates don't work
    // p.setClipRegion(frames.at(currentFrame).updateRegion);

    buffer.draw(&p, visibleUpdates.at(currentFrame));

    ++currentFrame;
    if (currentFrame >= visibleUpdates.size()) {
        currentFrame = 0;
        ++currentIteration;

        uint currentElapsed = timer.isNull() ? 0 : timer.elapsed();
        timer.restart();

        m_total += currentElapsed;

        // warm up for at most 5 iterations or half a second
        if (currentIteration >= 5 || m_total >= 500) {
            iterationTimes << currentElapsed;

            if (iterationTimes.size() >= 5) {
                qreal mean = 0;
                qreal stddev = 0;
                uint min = INT_MAX;

                for (int i = 0; i < iterationTimes.size(); ++i) {
                    mean += iterationTimes.at(i);
                    min = qMin(min, iterationTimes.at(i));
                }

                mean /= qreal(iterationTimes.size());

                for (int i = 0; i < iterationTimes.size(); ++i) {
                    qreal delta = iterationTimes.at(i) - mean;
                    stddev += delta * delta;
                }

                stddev = qSqrt(stddev / iterationTimes.size());

                qSort(iterationTimes.begin(), iterationTimes.end());
                uint median = iterationTimes.at(iterationTimes.size() / 2);

                stddev = 100 * stddev / mean;
                // do 100 iterations, break earlier if we spend more than 5 seconds or have a low std deviation after 2 seconds
                if (iterationTimes.size() >= 100 || m_total >= 5000 || (m_total >= 2000 && stddev < 4)) {
                    printf("%s, iterations: %d, frames: %d, min(ms): %d, median(ms): %d, stddev: %f %%, max(fps): %f\n", qPrintable(filename),
                            iterationTimes.size(), visibleUpdates.size(), min, median, stddev, 1000. * visibleUpdates.size() / min);
                    m_result = min;
                    m_done = true;
                    return;
                }
            }
        }
    }
}

void ReplayWidget::resizeEvent(QResizeEvent * /* event */)
{
    visibleUpdates.clear();

    QRect bounds = rect();
    for (int i = 0; i < updates.size(); ++i) {
        if (updates.at(i).intersects(bounds))
            visibleUpdates << i;
    }

    if (visibleUpdates.size() != updates.size())
        printf("Warning: skipped %d frames due to limited resolution\n", updates.size() - visibleUpdates.size());

}

ReplayWidget::ReplayWidget(const QString &filename_)
    : currentFrame(0)
    , currentIteration(0)
    , filename(filename_)
    , m_done(false)
    , m_result(0)
    , m_total(0)
{
    setWindowTitle(filename);
    QFile file(filename);

    if (!file.open(QIODevice::ReadOnly)) {
        printf("Failed to load input file '%s'\n", qPrintable(filename_));
        return;
    }

    QDataStream in(&file);
    in.setVersion(QDataStream::Qt_4_7);

    char *data;
    uint size;
    in.readBytes(data, size);
    bool isTraceFile = size >= 7 && qstrncmp(data, "qttrace", 7) == 0;
    uint version = 0;
    if (size == 9 && qstrncmp(data, "qttraceV2", 9) == 0) {
        in.setFloatingPointPrecision(QDataStream::SinglePrecision);
        in >> version;
    }

    delete [] data;
    if (!isTraceFile) {
        printf("File '%s' is not a trace file\n", qPrintable(filename_));
        return;
    }

    in >> buffer >> updates;

    resize(buffer.boundingRect().size().toSize());

    setAutoFillBackground(false);
    setAttribute(Qt::WA_NoSystemBackground);
}


class tst_QTraceBench : public QObject
{
    Q_OBJECT

private slots:
    void trace();
    void trace_data();
};

static const QLatin1String prefix(":/traces/");

void tst_QTraceBench::trace_data()
{
    QTest::addColumn<QString>("filename");

    QTest::newRow("basicdrawing") << (prefix + "basicdrawing.trace");
    QTest::newRow("webkit") << (prefix + "webkit.trace");
    QTest::newRow("creator") << (prefix + "creator.trace");
    QTest::newRow("textedit") << (prefix + "textedit.trace");
    QTest::newRow("qmlphoneconcept") << (prefix + "qmlphoneconcept.trace");
    QTest::newRow("qmlsamegame") << (prefix + "qmlsamegame.trace");
}

void tst_QTraceBench::trace()
{
    QFETCH(QString, filename);

    QFile file(filename);
    if (!file.exists()) {
        qWarning() << "Missing file" << filename;
        return;
    }

    ReplayWidget widget(filename);

    if (widget.updates.isEmpty()) {
        qWarning() << "No trace updates" << filename;
        return;
    }

    widget.show();
    QTest::qWaitForWindowShown(&widget);

    while (!widget.done()) {
        widget.updateRect();
        QApplication::processEvents();
    }

    QTest::setBenchmarkResult(widget.result(), QTest::WalltimeMilliseconds);
}

QTEST_MAIN(tst_QTraceBench)
#include "tst_qtracebench.moc"
