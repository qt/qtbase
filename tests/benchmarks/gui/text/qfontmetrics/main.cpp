/****************************************************************************
**
** Copyright (C) 2016 The Qt Company Ltd.
** Contact: https://www.qt.io/licensing/
**
** This file is part of the test suite of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:GPL-EXCEPT$
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company. For licensing terms
** and conditions see https://www.qt.io/terms-conditions. For further
** information use the contact form at https://www.qt.io/contact-us.
**
** GNU General Public License Usage
** Alternatively, this file may be used under the terms of the GNU
** General Public License version 3 as published by the Free Software
** Foundation with exceptions as appearing in the file LICENSE.GPL3-EXCEPT
** included in the packaging of this file. Please review the following
** information to ensure the GNU General Public License requirements will
** be met: https://www.gnu.org/licenses/gpl-3.0.html.
**
** $QT_END_LICENSE$
**
****************************************************************************/

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
    Q_UNUSED(fontHeight)
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
