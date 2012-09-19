/****************************************************************************
**
** Copyright (C) 2012 Thorbjørn Lund Martsum - tmartsum[at]gmail.com
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

#include <QtTest/QtTest>
#include <QtWidgets/QtWidgets>

class BenchQHeaderView : public QObject
{
    Q_OBJECT
public:
    BenchQHeaderView() : QObject() {m_tv = 0; m_hv = 0; m_model = 0;}

protected:
    QTableView *m_tv;
    QHeaderView *m_hv;
    QStandardItemModel *m_model;
    QStandardItemModel m_normalmodel;
    QElapsedTimer t;
    bool m_worst_case;
    void setupTestData();

    bool m_blockSomeSignals;
    bool m_updatesEnabled;
    int m_rowcount;
    int m_colcount;


private slots:
    void init();
    void cleanupTestCase();
    void initTestCase();
    void cleanup();
    void visualIndexAtSpecial_data()   {setupTestData();}
    void visualIndexAt_data()          {setupTestData();}
    void hideShowBench_data()          {setupTestData();}
    void swapSectionsBench_data()      {setupTestData();}
    void moveSectionBench_data()       {setupTestData();}
    void defaultSizeBench_data()       {setupTestData();}
    void removeBench_data()            {setupTestData();}
    void insertBench_data()            {setupTestData();}
    void truncBench_data()             {setupTestData();}

    void visualIndexAtSpecial();
    void visualIndexAt();
    void hideShowBench();
    void swapSectionsBench();
    void moveSectionBench();
    void defaultSizeBench();
    void removeBench();
    void insertBench();
    void truncBench();
};

void BenchQHeaderView::setupTestData()
{
    QTest::addColumn<bool>("worst_case");
    QTest::newRow("Less relevant best case") << false;
    QTest::newRow("__* More important worst case *__") << true;
}

void BenchQHeaderView::initTestCase()
{
    m_tv = new QTableView();
    m_hv = m_tv->verticalHeader();
    m_model = &m_normalmodel;
    m_tv->setModel(m_model);
    m_tv->show();
}

void BenchQHeaderView::cleanupTestCase()
{
    delete m_tv;
    m_tv = 0;
    m_hv = 0;
}

void BenchQHeaderView::init()
{
    QFETCH(bool, worst_case);

    m_blockSomeSignals = true;
    m_updatesEnabled = false;
    m_rowcount = 2500;
    m_colcount = 10;

    m_worst_case = worst_case;
    m_model->clear();
    if (worst_case) {
        for (int  u = 0; u <= m_rowcount; ++u) // ensures fragment in Qt 4.x
            m_model->setRowCount(u);
        m_model->setColumnCount(m_colcount);
        m_hv->swapSections(0, m_rowcount - 1);
        m_hv->hideSection(m_rowcount / 2);
    } else {
        m_model->setColumnCount(m_colcount);
        m_model->setRowCount(m_rowcount);
    }

    QString s;
    for (int i = 0; i < m_model->rowCount(); ++i) {
        m_model->setData(m_model->index(i, 0), QVariant(i));
        s.setNum(i);
        s += ".";
        s += 'a' + (i % 25);
        m_model->setData(m_model->index(i, 1), QVariant(s));
    }
    m_tv->setUpdatesEnabled(m_updatesEnabled);
    m_hv->blockSignals(m_blockSomeSignals);

    const int default_section_size = 25;
    m_hv->setDefaultSectionSize(default_section_size);
}

void BenchQHeaderView::cleanup()
{
    m_tv->setUpdatesEnabled(true);
    m_hv->blockSignals(false);
}

void BenchQHeaderView::visualIndexAtSpecial()
{
    int lookup_pos = m_hv->length() - 50;
    int testnum = 0;

    QBENCHMARK {
            ++testnum;
            m_hv->resizeSection(0, testnum % 47);
            m_hv->visualIndexAt(lookup_pos);
    }
}

void BenchQHeaderView::visualIndexAt()
{
    const int center_pos = m_hv->length() / 2;
    const int maxpos = m_hv->length() - 1;

    QBENCHMARK {
        m_hv->visualIndexAt(0);
        m_hv->visualIndexAt(center_pos);
        m_hv->visualIndexAt(maxpos);
    }
}

void BenchQHeaderView::hideShowBench()
{
    int n = 0;
    bool hide_set = true;

    QBENCHMARK {
        m_hv->setSectionHidden(n, hide_set);
        if (n >= m_hv->count()) {
            n = -1;
            hide_set = !hide_set;
        }
        ++n;
    }
}

void BenchQHeaderView::swapSectionsBench()
{
    int n = 0;
    QBENCHMARK {
        m_hv->swapSections(n, n + 1);
        if (++n >= m_hv->count())
            n = 0;
    }
}

void BenchQHeaderView::moveSectionBench()
{
    QBENCHMARK {
        m_hv->moveSection(0, m_hv->count() - 2);
    }
}

void BenchQHeaderView::defaultSizeBench()
{
    int n = 1;
    QBENCHMARK {
        m_hv->setDefaultSectionSize(n);
        ++n;
    }
}

void BenchQHeaderView::removeBench()
{
    QBENCHMARK {
        m_model->removeRows(0, 1);
        if (m_hv->count() == 0) { // setup a new hard model
            m_model->setRowCount(m_rowcount);
            if (m_worst_case) {
                m_hv->swapSections(0, m_rowcount - 1);
                m_hv->hideSection(m_rowcount / 2);
            }
        }
    }
}

void BenchQHeaderView::insertBench()
{
    QBENCHMARK {
        m_model->insertRows(1, 1);
        if (m_hv->count() == 10000000) { // setup a new hard model
            m_model->setRowCount(m_rowcount);
            if (m_worst_case) {
                m_hv->swapSections(0, m_rowcount - 1);
                m_hv->hideSection(m_rowcount / 2);
            }
        }
    }
}

void BenchQHeaderView::truncBench()
{
    QBENCHMARK {
        m_model->setRowCount(1);
        m_model->setRowCount(m_rowcount);
        if (m_worst_case) {
            m_hv->swapSections(0, m_rowcount - 1);
            m_hv->hideSection(m_rowcount / 2);
        }
    }
}

QTEST_MAIN(BenchQHeaderView)
#include "qheaderviewbench.moc"
