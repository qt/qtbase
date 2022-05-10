// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include <QObject>
#include <QFont>
#include <QFontMetrics>

#include <qtest.h>

//this test benchmarks the once-off (per font configuration) cost
//associated with using QFontMetrics
class tst_QFontMetrics : public QObject
{
    Q_OBJECT
public:
    tst_QFontMetrics() {}
private slots:
    void fontmetrics_create();
    void fontmetrics_create_once_loaded();

    void fontmetrics_height();
    void fontmetrics_height_once_loaded();

private:
    void testQFontMetrics(const QFontMetrics &fm);
};

void tst_QFontMetrics::testQFontMetrics( const QFontMetrics &fm )
{
    int fontHeight = fm.height();
    Q_UNUSED(fontHeight);
}

void tst_QFontMetrics::fontmetrics_create()
{
    QBENCHMARK {
      QFont boldfont = QGuiApplication::font();
      boldfont.setBold( true );
      boldfont.setPointSize(boldfont.pointSize() * 1.5 );
      QFontMetrics bfm( boldfont );
    }
}

void tst_QFontMetrics::fontmetrics_create_once_loaded()
{
    QBENCHMARK {
      QFont boldfont = QGuiApplication::font();
      boldfont.setBold( true );
      boldfont.setPointSize(boldfont.pointSize() * 1.5 );
      QFontMetrics bfm( boldfont );
    }
}

void tst_QFontMetrics::fontmetrics_height()
{
    QFont boldfont = QGuiApplication::font();
    boldfont.setBold( true );
    boldfont.setPointSize(boldfont.pointSize() * 1.5 );
    QFontMetrics bfm( boldfont );

    QBENCHMARK { testQFontMetrics(bfm); }
}

void tst_QFontMetrics::fontmetrics_height_once_loaded()
{
    QFont boldfont = QGuiApplication::font();
    boldfont.setBold( true );
    boldfont.setPointSize(boldfont.pointSize() * 1.5 );
    QFontMetrics bfm( boldfont );
    QBENCHMARK { testQFontMetrics(bfm); }
}

QTEST_MAIN(tst_QFontMetrics)

#include "main.moc"
