/****************************************************************************
**
** Copyright (C) 2015 The Qt Company Ltd.
** Contact: http://www.qt.io/licensing/
**
** This file is part of the test suite of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:LGPL21$
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company. For licensing terms
** and conditions see http://www.qt.io/terms-conditions. For further
** information use the contact form at http://www.qt.io/contact-us.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 2.1 or version 3 as published by the Free
** Software Foundation and appearing in the file LICENSE.LGPLv21 and
** LICENSE.LGPLv3 included in the packaging of this file. Please review the
** following information to ensure the GNU Lesser General Public License
** requirements will be met: https://www.gnu.org/licenses/lgpl.html and
** http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** As a special exception, The Qt Company gives you certain additional
** rights. These rights are described in The Qt Company LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
** $QT_END_LICENSE$
**
****************************************************************************/


#include <QtTest/QtTest>

#include <qtreewidget.h>
#include <qtreewidgetitemiterator.h>
#include <qapplication.h>
#include <qeventloop.h>
#include <qdebug.h>

class tst_QTreeWidgetItemIterator : public QObject
{
    Q_OBJECT

public:
    tst_QTreeWidgetItemIterator();
    ~tst_QTreeWidgetItemIterator();

public slots:
    void initTestCase();
    void cleanupTestCase();
    void init();
    void cleanup();

private slots:
    void postincrement();
    void preincrement();
    void postdecrement();
    void predecrement();
    void plus_eq_data();
    void plus_eq();
    void minus_eq_data();
    void minus_eq();
    void iteratorflags_data();
    void iteratorflags();
    void updateIfModifiedFromWidget_data();
    void updateIfModifiedFromWidget();
    void constructIteratorWithItem_data();
    void constructIteratorWithItem();
    void updateIteratorAfterDeletedItem_and_ContinueIteration_data();
    void updateIteratorAfterDeletedItem_and_ContinueIteration();
    void initializeIterator();
    void sortingEnabled();
private:
    QTreeWidget *testWidget;
};

tst_QTreeWidgetItemIterator::tst_QTreeWidgetItemIterator(): testWidget(0)
{
}

tst_QTreeWidgetItemIterator::~tst_QTreeWidgetItemIterator()
{
}

void tst_QTreeWidgetItemIterator::initTestCase()
{
    testWidget = new QTreeWidget();
    testWidget->clear();
    testWidget->setColumnCount(2);
    testWidget->show();


    /**
     * These are default:
     *
     *           Qt::ItemIsSelectable
     *          |Qt::ItemIsUserCheckable
     *          |Qt::ItemIsEnabled
     *          |Qt::ItemIsDragEnabled
     *          |Qt::ItemIsDropEnabled
     *
     */
    for (int i=0; i <= 16; ++i) {
        QTreeWidgetItem *top = new QTreeWidgetItem(testWidget);
        top->setText(0, QString("top%1").arg(i));
        switch (i) {
            case 0:  testWidget->setItemHidden(top, true);break;
            case 1:  testWidget->setItemHidden(top, false);break;

            case 2:  testWidget->setItemSelected(top, true);break;
            case 3:  testWidget->setItemSelected(top, false);break;

            case 4:  top->setFlags(Qt::ItemIsEnabled | Qt::ItemIsSelectable);break;
            case 5:  top->setFlags(Qt::ItemIsEnabled);break;

            case 6:  top->setFlags(Qt::ItemIsEnabled | Qt::ItemIsDragEnabled);break;
            case 7:  top->setFlags(Qt::ItemIsEnabled);break;

            case 8:  top->setFlags(Qt::ItemIsEnabled | Qt::ItemIsDropEnabled);break;
            case 9:  top->setFlags(Qt::ItemIsEnabled);break;

            case 10:  top->setFlags(Qt::ItemIsEnabled);break;
            case 11:
                top->setFlags(0);
                break;

            case 12:  top->setFlags(Qt::ItemIsEnabled | Qt::ItemIsEditable);break;
            case 13:  top->setFlags(Qt::ItemIsEnabled);break;

            case 14: top->setCheckState(0, Qt::Checked);break;
            case 15: top->setCheckState(0, Qt::Unchecked);break;
            case 16: top->setCheckState(0, Qt::PartiallyChecked);break;
        }
        for (int j=0; j <= 16; ++j) {
            QTreeWidgetItem *child = new QTreeWidgetItem(top);
            child->setText(0, QString("top%1,child%2").arg(i).arg(j));
            switch (j) {
                case 0:  testWidget->setItemHidden(child, true);break;
                case 1:  testWidget->setItemHidden(child, false);break;

                case 2:  testWidget->setItemSelected(child, true);break;
                case 3:  testWidget->setItemSelected(child, false);break;

                case 4:  child->setFlags(Qt::ItemIsEnabled | Qt::ItemIsSelectable);break;
                case 5:  child->setFlags(Qt::ItemIsEnabled);break;

                case 6:  child->setFlags(Qt::ItemIsEnabled | Qt::ItemIsDragEnabled);break;
                case 7:  child->setFlags(Qt::ItemIsEnabled);break;

                case 8:  child->setFlags(Qt::ItemIsEnabled | Qt::ItemIsDropEnabled);break;
                case 9:  child->setFlags(Qt::ItemIsEnabled);break;

                case 10:  child->setFlags(Qt::ItemIsEnabled);break;
                case 11:  child->setFlags(0);break;

                case 12:  child->setFlags(Qt::ItemIsEnabled | Qt::ItemIsEditable);break;
                case 13:  child->setFlags(Qt::ItemIsEnabled);break;

                case 14: child->setCheckState(0, Qt::Checked);break;
                case 15: child->setCheckState(0, Qt::Unchecked);break;
                case 16: child->setCheckState(0, Qt::PartiallyChecked);break;
            }

        }
    }
}

void tst_QTreeWidgetItemIterator::cleanupTestCase()
{
    testWidget->hide();
    delete testWidget;
}

void tst_QTreeWidgetItemIterator::init()
{
}

void tst_QTreeWidgetItemIterator::cleanup()
{
}

void tst_QTreeWidgetItemIterator::iteratorflags_data()
{
  /*
  // Should preferably test for all these flags (and combinations).....

        All           = 0x00000000,
        Hidden        = 0x00000001,
        NotHidden     = 0x00000002,
        Selected      = 0x00000004,
        Unselected    = 0x00000008,
        Selectable    = 0x00000010,
        NotSelectable = 0x00000020,
        DragEnabled   = 0x00000040,
        DragDisabled  = 0x00000080,
        DropEnabled   = 0x00000100,
        DropDisabled  = 0x00000200,
        HasChildren   = 0x00000400,
        NoChildren    = 0x00000800,
        Checked       = 0x00001000,
        NotChecked    = 0x00002000,
        Enabled       = 0x00004000,
        Disabled      = 0x00008000,
        Editable      = 0x00010000,
        NotEditable   = 0x00020000
*/
    QTest::addColumn<int>("start");
    QTest::addColumn<int>("iteratorflags");
    QTest::addColumn<QStringList>("matches");

    QTest::newRow("Match all") << 0 << (int)QTreeWidgetItemIterator::All
                               << (QStringList()
                                                                << "top0"   << "top0,child0"  << "top0,child1"  << "top0,child2"  << "top0,child3"
                                                                            << "top0,child4"  << "top0,child5"  << "top0,child6"  << "top0,child7"
                                                                            << "top0,child8"  << "top0,child9"  << "top0,child10" << "top0,child11"
                                                                            << "top0,child12" << "top0,child13" << "top0,child14" << "top0,child15"
                                                                            << "top0,child16"
                                                                << "top1"   << "top1,child0"  << "top1,child1"  << "top1,child2"  << "top1,child3"
                                                                            << "top1,child4"  << "top1,child5"  << "top1,child6"  << "top1,child7"
                                                                            << "top1,child8"  << "top1,child9"  << "top1,child10" << "top1,child11"
                                                                            << "top1,child12" << "top1,child13" << "top1,child14" << "top1,child15"
                                                                            << "top1,child16"
                                                                << "top2"   << "top2,child0"  << "top2,child1"  << "top2,child2"  << "top2,child3"
                                                                            << "top2,child4"  << "top2,child5"  << "top2,child6"  << "top2,child7"
                                                                            << "top2,child8"  << "top2,child9"  << "top2,child10" << "top2,child11"
                                                                            << "top2,child12" << "top2,child13" << "top2,child14" << "top2,child15"
                                                                            << "top2,child16"
                                                                << "top3"   << "top3,child0"  << "top3,child1"  << "top3,child2"  << "top3,child3"
                                                                            << "top3,child4"  << "top3,child5"  << "top3,child6"  << "top3,child7"
                                                                            << "top3,child8"  << "top3,child9"  << "top3,child10" << "top3,child11"
                                                                            << "top3,child12" << "top3,child13" << "top3,child14" << "top3,child15"
                                                                            << "top3,child16"
                                                                << "top4"   << "top4,child0"  << "top4,child1"  << "top4,child2"  << "top4,child3"
                                                                            << "top4,child4"  << "top4,child5"  << "top4,child6"  << "top4,child7"
                                                                            << "top4,child8"  << "top4,child9"  << "top4,child10" << "top4,child11"
                                                                            << "top4,child12" << "top4,child13" << "top4,child14" << "top4,child15"
                                                                            << "top4,child16"
                                                                << "top5"   << "top5,child0"  << "top5,child1"  << "top5,child2"  << "top5,child3"
                                                                            << "top5,child4"  << "top5,child5"  << "top5,child6"  << "top5,child7"
                                                                            << "top5,child8"  << "top5,child9"  << "top5,child10" << "top5,child11"
                                                                            << "top5,child12" << "top5,child13" << "top5,child14" << "top5,child15"
                                                                            << "top5,child16"
                                                                << "top6"   << "top6,child0"  << "top6,child1"  << "top6,child2"  << "top6,child3"
                                                                            << "top6,child4"  << "top6,child5"  << "top6,child6"  << "top6,child7"
                                                                            << "top6,child8"  << "top6,child9"  << "top6,child10" << "top6,child11"
                                                                            << "top6,child12" << "top6,child13" << "top6,child14" << "top6,child15"
                                                                            << "top6,child16"
                                                                << "top7"   << "top7,child0"  << "top7,child1"  << "top7,child2"  << "top7,child3"
                                                                            << "top7,child4"  << "top7,child5"  << "top7,child6"  << "top7,child7"
                                                                            << "top7,child8"  << "top7,child9"  << "top7,child10" << "top7,child11"
                                                                            << "top7,child12" << "top7,child13" << "top7,child14" << "top7,child15"
                                                                            << "top7,child16"
                                                                << "top8"   << "top8,child0"  << "top8,child1"  << "top8,child2"  << "top8,child3"
                                                                            << "top8,child4"  << "top8,child5"  << "top8,child6"  << "top8,child7"
                                                                            << "top8,child8"  << "top8,child9"  << "top8,child10" << "top8,child11"
                                                                            << "top8,child12" << "top8,child13" << "top8,child14" << "top8,child15"
                                                                            << "top8,child16"
                                                                << "top9"   << "top9,child0"  << "top9,child1"  << "top9,child2"  << "top9,child3"
                                                                            << "top9,child4"  << "top9,child5"  << "top9,child6"  << "top9,child7"
                                                                            << "top9,child8"  << "top9,child9"  << "top9,child10" << "top9,child11"
                                                                            << "top9,child12" << "top9,child13" << "top9,child14" << "top9,child15"
                                                                            << "top9,child16"
                                                                << "top10"  << "top10,child0"  << "top10,child1"  << "top10,child2"  << "top10,child3"
                                                                            << "top10,child4"  << "top10,child5"  << "top10,child6"  << "top10,child7"
                                                                            << "top10,child8"  << "top10,child9"  << "top10,child10" << "top10,child11"
                                                                            << "top10,child12" << "top10,child13" << "top10,child14" << "top10,child15"
                                                                            << "top10,child16"
                                                                << "top11"  << "top11,child0"  << "top11,child1"  << "top11,child2"  << "top11,child3"
                                                                            << "top11,child4"  << "top11,child5"  << "top11,child6"  << "top11,child7"
                                                                            << "top11,child8"  << "top11,child9"  << "top11,child10" << "top11,child11"
                                                                            << "top11,child12" << "top11,child13" << "top11,child14" << "top11,child15"
                                                                            << "top11,child16"
                                                                << "top12"  << "top12,child0"  << "top12,child1"  << "top12,child2"  << "top12,child3"
                                                                            << "top12,child4"  << "top12,child5"  << "top12,child6"  << "top12,child7"
                                                                            << "top12,child8"  << "top12,child9"  << "top12,child10" << "top12,child11"
                                                                            << "top12,child12" << "top12,child13" << "top12,child14" << "top12,child15"
                                                                            << "top12,child16"
                                                                << "top13"  << "top13,child0"  << "top13,child1"  << "top13,child2"  << "top13,child3"
                                                                            << "top13,child4"  << "top13,child5"  << "top13,child6"  << "top13,child7"
                                                                            << "top13,child8"  << "top13,child9"  << "top13,child10" << "top13,child11"
                                                                            << "top13,child12" << "top13,child13" << "top13,child14" << "top13,child15"
                                                                            << "top13,child16"
                                                                << "top14"  << "top14,child0"  << "top14,child1"  << "top14,child2"  << "top14,child3"
                                                                            << "top14,child4"  << "top14,child5"  << "top14,child6"  << "top14,child7"
                                                                            << "top14,child8"  << "top14,child9"  << "top14,child10" << "top14,child11"
                                                                            << "top14,child12" << "top14,child13" << "top14,child14" << "top14,child15"
                                                                            << "top14,child16"
                                                                << "top15"  << "top15,child0"  << "top15,child1"  << "top15,child2"  << "top15,child3"
                                                                            << "top15,child4"  << "top15,child5"  << "top15,child6"  << "top15,child7"
                                                                            << "top15,child8"  << "top15,child9"  << "top15,child10" << "top15,child11"
                                                                            << "top15,child12" << "top15,child13" << "top15,child14" << "top15,child15"
                                                                            << "top15,child16"
                                                                << "top16"  << "top16,child0"  << "top16,child1"  << "top16,child2"  << "top16,child3"
                                                                            << "top16,child4"  << "top16,child5"  << "top16,child6"  << "top16,child7"
                                                                            << "top16,child8"  << "top16,child9"  << "top16,child10" << "top16,child11"
                                                                            << "top16,child12" << "top16,child13" << "top16,child14" << "top16,child15"
                                                                            << "top16,child16");

    QTest::newRow("Match hidden") << 0 << (int)QTreeWidgetItemIterator::Hidden
                                  << (QStringList()
                                                                << "top0" << "top0,child0"  // fails due to hidden row
                                                                << "top1,child0"
                                                                << "top2,child0"
                                                                << "top3,child0"
                                                                << "top4,child0"
                                                                << "top5,child0"
                                                                << "top6,child0"
                                                                << "top7,child0"
                                                                << "top8,child0"
                                                                << "top9,child0"
                                                                << "top10,child0"
                                                                << "top11,child0"
                                                                << "top12,child0"
                                                                << "top13,child0"
                                                                << "top14,child0"
                                                                << "top15,child0"
                                      << "top16,child0");

    QTest::newRow("Match not hidden") << 0 << (int)QTreeWidgetItemIterator::NotHidden
                                      << (QStringList()
                                                                            << "top0,child1"  << "top0,child2"  << "top0,child3"
                                                                            << "top0,child4"  << "top0,child5"  << "top0,child6"  << "top0,child7"
                                                                            << "top0,child8"  << "top0,child9"  << "top0,child10" << "top0,child11"
                                                                            << "top0,child12" << "top0,child13" << "top0,child14" << "top0,child15"
                                                                            << "top0,child16"
                                                                << "top1"   << "top1,child1"  << "top1,child2"  << "top1,child3"
                                                                            << "top1,child4"  << "top1,child5"  << "top1,child6"  << "top1,child7"
                                                                            << "top1,child8"  << "top1,child9"  << "top1,child10" << "top1,child11"
                                                                            << "top1,child12" << "top1,child13" << "top1,child14" << "top1,child15"
                                                                            << "top1,child16"
                                                                << "top2"   << "top2,child1"  << "top2,child2"  << "top2,child3"
                                                                            << "top2,child4"  << "top2,child5"  << "top2,child6"  << "top2,child7"
                                                                            << "top2,child8"  << "top2,child9"  << "top2,child10" << "top2,child11"
                                                                            << "top2,child12" << "top2,child13" << "top2,child14" << "top2,child15"
                                                                            << "top2,child16"
                                                                << "top3"   << "top3,child1"  << "top3,child2"  << "top3,child3"
                                                                            << "top3,child4"  << "top3,child5"  << "top3,child6"  << "top3,child7"
                                                                            << "top3,child8"  << "top3,child9"  << "top3,child10" << "top3,child11"
                                                                            << "top3,child12" << "top3,child13" << "top3,child14" << "top3,child15"
                                                                            << "top3,child16"
                                                                << "top4"   << "top4,child1"  << "top4,child2"  << "top4,child3"
                                                                            << "top4,child4"  << "top4,child5"  << "top4,child6"  << "top4,child7"
                                                                            << "top4,child8"  << "top4,child9"  << "top4,child10" << "top4,child11"
                                                                            << "top4,child12" << "top4,child13" << "top4,child14" << "top4,child15"
                                                                            << "top4,child16"
                                                                << "top5"   << "top5,child1"  << "top5,child2"  << "top5,child3"
                                                                            << "top5,child4"  << "top5,child5"  << "top5,child6"  << "top5,child7"
                                                                            << "top5,child8"  << "top5,child9"  << "top5,child10" << "top5,child11"
                                                                            << "top5,child12" << "top5,child13" << "top5,child14" << "top5,child15"
                                                                            << "top5,child16"
                                                                << "top6"   << "top6,child1"  << "top6,child2"  << "top6,child3"
                                                                            << "top6,child4"  << "top6,child5"  << "top6,child6"  << "top6,child7"
                                                                            << "top6,child8"  << "top6,child9"  << "top6,child10" << "top6,child11"
                                                                            << "top6,child12" << "top6,child13" << "top6,child14" << "top6,child15"
                                                                            << "top6,child16"
                                                                << "top7"   << "top7,child1"  << "top7,child2"  << "top7,child3"
                                                                            << "top7,child4"  << "top7,child5"  << "top7,child6"  << "top7,child7"
                                                                            << "top7,child8"  << "top7,child9"  << "top7,child10" << "top7,child11"
                                                                            << "top7,child12" << "top7,child13" << "top7,child14" << "top7,child15"
                                                                            << "top7,child16"
                                                                << "top8"   << "top8,child1"  << "top8,child2"  << "top8,child3"
                                                                            << "top8,child4"  << "top8,child5"  << "top8,child6"  << "top8,child7"
                                                                            << "top8,child8"  << "top8,child9"  << "top8,child10" << "top8,child11"
                                                                            << "top8,child12" << "top8,child13" << "top8,child14" << "top8,child15"
                                                                            << "top8,child16"
                                                                << "top9"   << "top9,child1"  << "top9,child2"  << "top9,child3"
                                                                            << "top9,child4"  << "top9,child5"  << "top9,child6"  << "top9,child7"
                                                                            << "top9,child8"  << "top9,child9"  << "top9,child10" << "top9,child11"
                                                                            << "top9,child12" << "top9,child13" << "top9,child14" << "top9,child15"
                                                                            << "top9,child16"
                                                                << "top10"  << "top10,child1"  << "top10,child2"  << "top10,child3"
                                                                            << "top10,child4"  << "top10,child5"  << "top10,child6"  << "top10,child7"
                                                                            << "top10,child8"  << "top10,child9"  << "top10,child10" << "top10,child11"
                                                                            << "top10,child12" << "top10,child13" << "top10,child14" << "top10,child15"
                                                                            << "top10,child16"
                                                                << "top11"  << "top11,child1"  << "top11,child2"  << "top11,child3"
                                                                            << "top11,child4"  << "top11,child5"  << "top11,child6"  << "top11,child7"
                                                                            << "top11,child8"  << "top11,child9"  << "top11,child10" << "top11,child11"
                                                                            << "top11,child12" << "top11,child13" << "top11,child14" << "top11,child15"
                                                                            << "top11,child16"
                                                                << "top12"  << "top12,child1"  << "top12,child2"  << "top12,child3"
                                                                            << "top12,child4"  << "top12,child5"  << "top12,child6"  << "top12,child7"
                                                                            << "top12,child8"  << "top12,child9"  << "top12,child10" << "top12,child11"
                                                                            << "top12,child12" << "top12,child13" << "top12,child14" << "top12,child15"
                                                                            << "top12,child16"
                                                                << "top13"  << "top13,child1"  << "top13,child2"  << "top13,child3"
                                                                            << "top13,child4"  << "top13,child5"  << "top13,child6"  << "top13,child7"
                                                                            << "top13,child8"  << "top13,child9"  << "top13,child10" << "top13,child11"
                                                                            << "top13,child12" << "top13,child13" << "top13,child14" << "top13,child15"
                                                                            << "top13,child16"
                                                                << "top14"  << "top14,child1"  << "top14,child2"  << "top14,child3"
                                                                            << "top14,child4"  << "top14,child5"  << "top14,child6"  << "top14,child7"
                                                                            << "top14,child8"  << "top14,child9"  << "top14,child10" << "top14,child11"
                                                                            << "top14,child12" << "top14,child13" << "top14,child14" << "top14,child15"
                                                                            << "top14,child16"
                                                                << "top15"  << "top15,child1"  << "top15,child2"  << "top15,child3"
                                                                            << "top15,child4"  << "top15,child5"  << "top15,child6"  << "top15,child7"
                                                                            << "top15,child8"  << "top15,child9"  << "top15,child10" << "top15,child11"
                                                                            << "top15,child12" << "top15,child13" << "top15,child14" << "top15,child15"
                                                                            << "top15,child16"
                                                                << "top16"  << "top16,child1"  << "top16,child2"  << "top16,child3"
                                                                            << "top16,child4"  << "top16,child5"  << "top16,child6"  << "top16,child7"
                                                                            << "top16,child8"  << "top16,child9"  << "top16,child10" << "top16,child11"
                                                                            << "top16,child12" << "top16,child13" << "top16,child14" << "top16,child15"
                                                                            << "top16,child16");

    QTest::newRow("Match selected") << 0 << (int)QTreeWidgetItemIterator::Selected
                                    << (QStringList()
                                                                << "top0,child2"
                                                                << "top1,child2"
                                                                << "top2" << "top2,child2"
                                                                << "top3,child2"
                                                                << "top4,child2"
                                                                << "top5,child2"
                                                                << "top6,child2"
                                                                << "top7,child2"
                                                                << "top8,child2"
                                                                << "top9,child2"
                                                                << "top10,child2"
                                                                << "top11,child2"
                                                                << "top12,child2"
                                                                << "top13,child2"
                                                                << "top14,child2"
                                                                << "top15,child2"
                                        << "top16,child2");

    QTest::newRow("Match selectable") << 0 << (int)QTreeWidgetItemIterator::Selectable
                                      << (QStringList()
                                                                << "top0"   << "top0,child0"  << "top0,child1"    << "top0,child2"  << "top0,child3"
                                                                            << "top0,child4"
                                                                                                                  << "top0,child14" << "top0,child15"
                                                                            << "top0,child16"
                                                                << "top1"   << "top1,child0"  << "top1,child1"    << "top1,child2"  << "top1,child3"
                                                                            << "top1,child4"
                                                                                                                  << "top1,child14" << "top1,child15"
                                                                            << "top1,child16"
                                                                << "top2"   << "top2,child0"  << "top2,child1"    << "top2,child2"  << "top2,child3"
                                                                            << "top2,child4"
                                                                                                                  << "top2,child14" << "top2,child15"
                                                                            << "top2,child16"
                                                                << "top3"   << "top3,child0"  << "top3,child1"    << "top3,child2"  << "top3,child3"
                                                                            << "top3,child4"
                                                                                                                  << "top3,child14" << "top3,child15"
                                                                            << "top3,child16"
                                                                << "top4"   << "top4,child0"  << "top4,child1"    << "top4,child2"  << "top4,child3"
                                                                            << "top4,child4"
                                                                                                                  << "top4,child14" << "top4,child15"
                                                                            << "top4,child16"
                                                                /* "top5"*/ << "top5,child0"  << "top5,child1"    << "top5,child2"  << "top5,child3"
                                                                            << "top5,child4"
                                                                                                                  << "top5,child14" << "top5,child15"
                                                                            << "top5,child16"
                                                                /* "top6"*/ << "top6,child0"  << "top6,child1"    << "top6,child2"  << "top6,child3"
                                                                            << "top6,child4"
                                                                                                                  << "top6,child14" << "top6,child15"
                                                                            << "top6,child16"
                                                                /* "top7"*/ << "top7,child0"  << "top7,child1"    << "top7,child2"  << "top7,child3"
                                                                            << "top7,child4"
                                                                                                                  << "top7,child14" << "top7,child15"
                                                                            << "top7,child16"
                                                                /* "top8"*/ << "top8,child0"  << "top8,child1"    << "top8,child2"  << "top8,child3"
                                                                            << "top8,child4"
                                                                                                                  << "top8,child14" << "top8,child15"
                                                                            << "top8,child16"
                                                                /* "top9"*/ << "top9,child0"  << "top9,child1"    << "top9,child2"  << "top9,child3"
                                                                            << "top9,child4"
                                                                                                                  << "top9,child14" << "top9,child15"
                                                                            << "top9,child16"
                                                                /* "top10*/ << "top10,child0"  << "top10,child1"    << "top10,child2"  << "top10,child3"
                                                                            << "top10,child4"
                                                                                                                  << "top10,child14" << "top10,child15"
                                                                            << "top10,child16"
                                                                /* "top11*/ << "top11,child0"  << "top11,child1"    << "top11,child2"  << "top11,child3"
                                                                            << "top11,child4"
                                                                                                                  << "top11,child14" << "top11,child15"
                                                                            << "top11,child16"
                                                                /* "top12*/ << "top12,child0"  << "top12,child1"    << "top12,child2"  << "top12,child3"
                                                                            << "top12,child4"
                                                                                                                  << "top12,child14" << "top12,child15"
                                                                            << "top12,child16"
                                                                /* "top13*/ << "top13,child0"  << "top13,child1"    << "top13,child2"  << "top13,child3"
                                                                            << "top13,child4"
                                                                                                                  << "top13,child14" << "top13,child15"
                                                                            << "top13,child16"
                                                                << "top14"  << "top14,child0"  << "top14,child1"    << "top14,child2"  << "top14,child3"
                                                                            << "top14,child4"
                                                                                                                  << "top14,child14" << "top14,child15"
                                                                            << "top14,child16"
                                                                << "top15"  << "top15,child0"  << "top15,child1"    << "top15,child2"  << "top15,child3"
                                                                            << "top15,child4"
                                                                                                                  << "top15,child14" << "top15,child15"
                                                                            << "top15,child16"
                                                                << "top16"  << "top16,child0"  << "top16,child1"    << "top16,child2"  << "top16,child3"
                                                                            << "top16,child4"
                                                                                                                  << "top16,child14" << "top16,child15"
                                          << "top16,child16");


    QTest::newRow("Match DragEnabled") << 0 << (int)QTreeWidgetItemIterator::DragEnabled
                                       << (QStringList()
                                                                << "top0"   << "top0,child0"  << "top0,child1"    << "top0,child2"  << "top0,child3"
                                                                            << "top0,child6"
                                                                                                                  << "top0,child14" << "top0,child15"
                                                                            << "top0,child16"
                                                                << "top1"   << "top1,child0"  << "top1,child1"    << "top1,child2"  << "top1,child3"
                                                                            << "top1,child6"
                                                                                                                  << "top1,child14" << "top1,child15"
                                                                            << "top1,child16"
                                                                << "top2"   << "top2,child0"  << "top2,child1"    << "top2,child2"  << "top2,child3"
                                                                            << "top2,child6"
                                                                                                                  << "top2,child14" << "top2,child15"
                                                                            << "top2,child16"
                                                                << "top3"   << "top3,child0"  << "top3,child1"    << "top3,child2"  << "top3,child3"
                                                                            << "top3,child6"
                                                                                                                  << "top3,child14" << "top3,child15"
                                                                            << "top3,child16"
                                                                /* "top4"*/ << "top4,child0"  << "top4,child1"    << "top4,child2"  << "top4,child3"
                                                                            << "top4,child6"
                                                                                                                  << "top4,child14" << "top4,child15"
                                                                            << "top4,child16"
                                                                /* "top5"*/ << "top5,child0"  << "top5,child1"    << "top5,child2"  << "top5,child3"
                                                                            << "top5,child6"
                                                                                                                  << "top5,child14" << "top5,child15"
                                                                            << "top5,child16"
                                                                << "top6"   << "top6,child0"  << "top6,child1"    << "top6,child2"  << "top6,child3"
                                                                            << "top6,child6"
                                                                                                                  << "top6,child14" << "top6,child15"
                                                                            << "top6,child16"
                                                                /* "top7"*/ << "top7,child0"  << "top7,child1"    << "top7,child2"  << "top7,child3"
                                                                            << "top7,child6"
                                                                                                                  << "top7,child14" << "top7,child15"
                                                                            << "top7,child16"
                                                                /* "top8"*/ << "top8,child0"  << "top8,child1"    << "top8,child2"  << "top8,child3"
                                                                            << "top8,child6"
                                                                                                                  << "top8,child14" << "top8,child15"
                                                                            << "top8,child16"
                                                                /* "top9"*/ << "top9,child0"  << "top9,child1"    << "top9,child2"  << "top9,child3"
                                                                            << "top9,child6"
                                                                                                                  << "top9,child14" << "top9,child15"
                                                                            << "top9,child16"
                                                                /* "top10*/ << "top10,child0"  << "top10,child1"    << "top10,child2"  << "top10,child3"
                                                                            << "top10,child6"
                                                                                                                  << "top10,child14" << "top10,child15"
                                                                            << "top10,child16"
                                                                /* "top11*/ << "top11,child0"  << "top11,child1"    << "top11,child2"  << "top11,child3"
                                                                            << "top11,child6"
                                                                                                                  << "top11,child14" << "top11,child15"
                                                                            << "top11,child16"
                                                                /* "top12*/ << "top12,child0"  << "top12,child1"    << "top12,child2"  << "top12,child3"
                                                                            << "top12,child6"
                                                                                                                  << "top12,child14" << "top12,child15"
                                                                            << "top12,child16"
                                                                /* "top13*/ << "top13,child0"  << "top13,child1"    << "top13,child2"  << "top13,child3"
                                                                            << "top13,child6"
                                                                                                                  << "top13,child14" << "top13,child15"
                                                                            << "top13,child16"
                                                                << "top14"  << "top14,child0"  << "top14,child1"    << "top14,child2"  << "top14,child3"
                                                                            << "top14,child6"
                                                                                                                  << "top14,child14" << "top14,child15"
                                                                            << "top14,child16"
                                                                << "top15"  << "top15,child0"  << "top15,child1"    << "top15,child2"  << "top15,child3"
                                                                            << "top15,child6"
                                                                                                                  << "top15,child14" << "top15,child15"
                                                                            << "top15,child16"
                                                                << "top16"  << "top16,child0"  << "top16,child1"    << "top16,child2"  << "top16,child3"
                                                                            << "top16,child6"
                                                                                                                  << "top16,child14" << "top16,child15"
                                           << "top16,child16");

    QTest::newRow("Match DragDisabled") << 0 << (int)QTreeWidgetItemIterator::DragDisabled
                                        << (QStringList()

                                                                /* top0  */
                                                                            << "top0,child4" << "top0,child5"   << "top0,child7"    << "top0,child8"
                                                                            << "top0,child9" << "top0,child10"  << "top0,child11"   << "top0,child12"
                                                                            << "top0,child13"
                                                                /* top1  */
                                                                            << "top1,child4" << "top1,child5"   << "top1,child7"    << "top1,child8"
                                                                            << "top1,child9" << "top1,child10"  << "top1,child11"   << "top1,child12"
                                                                            << "top1,child13"
                                                                /* top2  */
                                                                            << "top2,child4" << "top2,child5"   << "top2,child7"    << "top2,child8"
                                                                            << "top2,child9" << "top2,child10"  << "top2,child11"   << "top2,child12"
                                                                            << "top2,child13"
                                                                /* top3  */
                                                                            << "top3,child4" << "top3,child5"   << "top3,child7"    << "top3,child8"
                                                                            << "top3,child9" << "top3,child10"  << "top3,child11"   << "top3,child12"
                                                                            << "top3,child13"
                                                                << "top4"
                                                                            << "top4,child4" << "top4,child5"   << "top4,child7"    << "top4,child8"
                                                                            << "top4,child9" << "top4,child10"  << "top4,child11"   << "top4,child12"
                                                                            << "top4,child13"
                                                                << "top5"
                                                                            << "top5,child4" << "top5,child5"   << "top5,child7"    << "top5,child8"
                                                                            << "top5,child9" << "top5,child10"  << "top5,child11"   << "top5,child12"
                                                                            << "top5,child13"
                                                                /* "top6"*/
                                                                            << "top6,child4" << "top6,child5"   << "top6,child7"    << "top6,child8"
                                                                            << "top6,child9" << "top6,child10"  << "top6,child11"   << "top6,child12"
                                                                            << "top6,child13"
                                                                << "top7"
                                                                            << "top7,child4" << "top7,child5"   << "top7,child7"    << "top7,child8"
                                                                            << "top7,child9" << "top7,child10"  << "top7,child11"   << "top7,child12"
                                                                            << "top7,child13"
                                                                << "top8"
                                                                            << "top8,child4" << "top8,child5"   << "top8,child7"    << "top8,child8"
                                                                            << "top8,child9" << "top8,child10"  << "top8,child11"   << "top8,child12"
                                                                            << "top8,child13"
                                                                << "top9"
                                                                            << "top9,child4" << "top9,child5"   << "top9,child7"    << "top9,child8"
                                                                            << "top9,child9" << "top9,child10"  << "top9,child11"   << "top9,child12"
                                                                            << "top9,child13"
                                                                << "top10"
                                                                            << "top10,child4" << "top10,child5"   << "top10,child7"    << "top10,child8"
                                                                            << "top10,child9" << "top10,child10"  << "top10,child11"   << "top10,child12"
                                                                            << "top10,child13"
                                                                << "top11"
                                                                            << "top11,child4" << "top11,child5"   << "top11,child7"    << "top11,child8"
                                                                            << "top11,child9" << "top11,child10"  << "top11,child11"   << "top11,child12"
                                                                            << "top11,child13"
                                                                << "top12"
                                                                            << "top12,child4" << "top12,child5"   << "top12,child7"    << "top12,child8"
                                                                            << "top12,child9" << "top12,child10"  << "top12,child11"   << "top12,child12"
                                                                            << "top12,child13"
                                                                << "top13"
                                                                            << "top13,child4" << "top13,child5"   << "top13,child7"    << "top13,child8"
                                                                            << "top13,child9" << "top13,child10"  << "top13,child11"   << "top13,child12"
                                                                            << "top13,child13"
                                                                /* top14 */
                                                                            << "top14,child4" << "top14,child5"   << "top14,child7"    << "top14,child8"
                                                                            << "top14,child9" << "top14,child10"  << "top14,child11"   << "top14,child12"
                                                                            << "top14,child13"
                                                                /* top15  */
                                                                            << "top15,child4" << "top15,child5"   << "top15,child7"    << "top15,child8"
                                                                            << "top15,child9" << "top15,child10"  << "top15,child11"   << "top15,child12"
                                                                            << "top15,child13"
                                                                /* top16  */
                                                                            << "top16,child4" << "top16,child5"   << "top16,child7"    << "top16,child8"
                                                                            << "top16,child9" << "top16,child10"  << "top16,child11"   << "top16,child12"
                                                                            << "top16,child13" );


    QTest::newRow("Match DropEnabled") << 0 << (int)QTreeWidgetItemIterator::DropEnabled
                                       << (QStringList()
                                                                << "top0"   << "top0,child0"  << "top0,child1"    << "top0,child2"  << "top0,child3"
                                                                            << "top0,child8"
                                                                                                                  << "top0,child14" << "top0,child15"
                                                                            << "top0,child16"
                                                                << "top1"   << "top1,child0"  << "top1,child1"    << "top1,child2"  << "top1,child3"
                                                                            << "top1,child8"
                                                                                                                  << "top1,child14" << "top1,child15"
                                                                            << "top1,child16"
                                                                << "top2"   << "top2,child0"  << "top2,child1"    << "top2,child2"  << "top2,child3"
                                                                            << "top2,child8"
                                                                                                                  << "top2,child14" << "top2,child15"
                                                                            << "top2,child16"
                                                                << "top3"   << "top3,child0"  << "top3,child1"    << "top3,child2"  << "top3,child3"
                                                                            << "top3,child8"
                                                                                                                  << "top3,child14" << "top3,child15"
                                                                            << "top3,child16"
                                                                /* "top4"*/ << "top4,child0"  << "top4,child1"    << "top4,child2"  << "top4,child3"
                                                                            << "top4,child8"
                                                                                                                  << "top4,child14" << "top4,child15"
                                                                            << "top4,child16"
                                                                /* "top5"*/ << "top5,child0"  << "top5,child1"    << "top5,child2"  << "top5,child3"
                                                                            << "top5,child8"
                                                                                                                  << "top5,child14" << "top5,child15"
                                                                            << "top5,child16"
                                                                /* "top6"*/ << "top6,child0"  << "top6,child1"    << "top6,child2"  << "top6,child3"
                                                                            << "top6,child8"
                                                                                                                  << "top6,child14" << "top6,child15"
                                                                            << "top6,child16"
                                                                /* "top7"*/ << "top7,child0"  << "top7,child1"    << "top7,child2"  << "top7,child3"
                                                                            << "top7,child8"
                                                                                                                  << "top7,child14" << "top7,child15"
                                                                            << "top7,child16"
                                                                << "top8"   << "top8,child0"  << "top8,child1"    << "top8,child2"  << "top8,child3"
                                                                            << "top8,child8"
                                                                                                                  << "top8,child14" << "top8,child15"
                                                                            << "top8,child16"
                                                                /* "top9"*/ << "top9,child0"  << "top9,child1"    << "top9,child2"  << "top9,child3"
                                                                            << "top9,child8"
                                                                                                                  << "top9,child14" << "top9,child15"
                                                                            << "top9,child16"
                                                                /* "top10*/ << "top10,child0"  << "top10,child1"    << "top10,child2"  << "top10,child3"
                                                                            << "top10,child8"
                                                                                                                  << "top10,child14" << "top10,child15"
                                                                            << "top10,child16"
                                                                /* "top11*/ << "top11,child0"  << "top11,child1"    << "top11,child2"  << "top11,child3"
                                                                            << "top11,child8"
                                                                                                                  << "top11,child14" << "top11,child15"
                                                                            << "top11,child16"
                                                                /* "top12*/ << "top12,child0"  << "top12,child1"    << "top12,child2"  << "top12,child3"
                                                                            << "top12,child8"
                                                                                                                  << "top12,child14" << "top12,child15"
                                                                            << "top12,child16"
                                                                /* "top13*/ << "top13,child0"  << "top13,child1"    << "top13,child2"  << "top13,child3"
                                                                            << "top13,child8"
                                                                                                                  << "top13,child14" << "top13,child15"
                                                                            << "top13,child16"
                                                                << "top14"  << "top14,child0"  << "top14,child1"    << "top14,child2"  << "top14,child3"
                                                                            << "top14,child8"
                                                                                                                  << "top14,child14" << "top14,child15"
                                                                            << "top14,child16"
                                                                << "top15"  << "top15,child0"  << "top15,child1"    << "top15,child2"  << "top15,child3"
                                                                            << "top15,child8"
                                                                                                                  << "top15,child14" << "top15,child15"
                                                                            << "top15,child16"
                                                                << "top16"  << "top16,child0"  << "top16,child1"    << "top16,child2"  << "top16,child3"
                                                                            << "top16,child8"
                                                                                                                  << "top16,child14" << "top16,child15"
                                           << "top16,child16");

    QTest::newRow("Match HasChildren") << 0 << (int)QTreeWidgetItemIterator::HasChildren
                                       << (QStringList() << "top0" << "top1" << "top2" << "top3" << "top4" << "top5"
                                           << "top6" << "top7" << "top8" << "top9" << "top10" << "top11" << "top12"
                                           << "top13" << "top14" << "top15" << "top16");

    QTest::newRow("Match Checked") << 0 << (int)QTreeWidgetItemIterator::Checked
                                   << (QStringList()
                                                                            << "top0,child14"  << "top0,child16"
                                                                            << "top1,child14"  << "top1,child16"
                                                                            << "top2,child14"  << "top2,child16"
                                                                            << "top3,child14"  << "top3,child16"
                                                                            << "top4,child14"  << "top4,child16"
                                                                            << "top5,child14"  << "top5,child16"
                                                                            << "top6,child14"  << "top6,child16"
                                                                            << "top7,child14"  << "top7,child16"
                                                                            << "top8,child14"  << "top8,child16"
                                                                            << "top9,child14"  << "top9,child16"
                                                                            << "top10,child14" << "top10,child16"
                                                                            << "top11,child14" << "top11,child16"
                                                                            << "top12,child14" << "top12,child16"
                                                                            << "top13,child14" << "top13,child16"
                                                                << "top14"
                                                                            << "top14,child14" << "top14,child16"
                                                                            << "top15,child14" << "top15,child16"
                                                                << "top16"
                                       << "top16,child14" << "top16,child16");

    QTest::newRow("Match NotChecked") << 0 << (int)QTreeWidgetItemIterator::NotChecked
                                      << (QStringList()
                                                                << "top0"   << "top0,child0"  << "top0,child1"  << "top0,child2"  << "top0,child3"
                                                                            << "top0,child4"  << "top0,child5"  << "top0,child6"  << "top0,child7"
                                                                            << "top0,child8"  << "top0,child9"  << "top0,child10" << "top0,child11"
                                                                            << "top0,child12" << "top0,child13" <<                   "top0,child15"

                                                                << "top1"   << "top1,child0" << "top1,child1"  << "top1,child2"  << "top1,child3"
                                                                            << "top1,child4"  << "top1,child5"  << "top1,child6"  << "top1,child7"
                                                                            << "top1,child8"  << "top1,child9"  << "top1,child10" << "top1,child11"
                                                                            << "top1,child12" << "top1,child13"                   << "top1,child15"

                                                                << "top2"   << "top2,child0" << "top2,child1"  << "top2,child2"  << "top2,child3"
                                                                            << "top2,child4"  << "top2,child5"  << "top2,child6"  << "top2,child7"
                                                                            << "top2,child8"  << "top2,child9"  << "top2,child10" << "top2,child11"
                                                                            << "top2,child12" << "top2,child13"                   << "top2,child15"

                                                                << "top3"   << "top3,child0" << "top3,child1"  << "top3,child2"  << "top3,child3"
                                                                            << "top3,child4"  << "top3,child5"  << "top3,child6"  << "top3,child7"
                                                                            << "top3,child8"  << "top3,child9"  << "top3,child10" << "top3,child11"
                                                                            << "top3,child12" << "top3,child13"                   << "top3,child15"

                                                                << "top4"   << "top4,child0" << "top4,child1"  << "top4,child2"  << "top4,child3"
                                                                            << "top4,child4"  << "top4,child5"  << "top4,child6"  << "top4,child7"
                                                                            << "top4,child8"  << "top4,child9"  << "top4,child10" << "top4,child11"
                                                                            << "top4,child12" << "top4,child13"                   << "top4,child15"

                                                                << "top5"   << "top5,child0" << "top5,child1"  << "top5,child2"  << "top5,child3"
                                                                            << "top5,child4"  << "top5,child5"  << "top5,child6"  << "top5,child7"
                                                                            << "top5,child8"  << "top5,child9"  << "top5,child10" << "top5,child11"
                                                                            << "top5,child12" << "top5,child13"                   << "top5,child15"

                                                                << "top6"   << "top6,child0" << "top6,child1"  << "top6,child2"  << "top6,child3"
                                                                            << "top6,child4"  << "top6,child5"  << "top6,child6"  << "top6,child7"
                                                                            << "top6,child8"  << "top6,child9"  << "top6,child10" << "top6,child11"
                                                                            << "top6,child12" << "top6,child13"                   << "top6,child15"

                                                                << "top7"   << "top7,child0" << "top7,child1"  << "top7,child2"  << "top7,child3"
                                                                            << "top7,child4"  << "top7,child5"  << "top7,child6"  << "top7,child7"
                                                                            << "top7,child8"  << "top7,child9"  << "top7,child10" << "top7,child11"
                                                                            << "top7,child12" << "top7,child13"                   << "top7,child15"

                                                                << "top8"   << "top8,child0" << "top8,child1"  << "top8,child2"  << "top8,child3"
                                                                            << "top8,child4"  << "top8,child5"  << "top8,child6"  << "top8,child7"
                                                                            << "top8,child8"  << "top8,child9"  << "top8,child10" << "top8,child11"
                                                                            << "top8,child12" << "top8,child13"                   << "top8,child15"

                                                                << "top9"   << "top9,child0" << "top9,child1"  << "top9,child2"  << "top9,child3"
                                                                            << "top9,child4"  << "top9,child5"  << "top9,child6"  << "top9,child7"
                                                                            << "top9,child8"  << "top9,child9"  << "top9,child10" << "top9,child11"
                                                                            << "top9,child12" << "top9,child13"                   << "top9,child15"

                                                                << "top10"  << "top10,child0" << "top10,child1"  << "top10,child2"  << "top10,child3"
                                                                            << "top10,child4"  << "top10,child5"  << "top10,child6"  << "top10,child7"
                                                                            << "top10,child8"  << "top10,child9"  << "top10,child10" << "top10,child11"
                                                                            << "top10,child12" << "top10,child13"                   << "top10,child15"

                                                                << "top11"  << "top11,child0" << "top11,child1"  << "top11,child2"  << "top11,child3"
                                                                            << "top11,child4"  << "top11,child5"  << "top11,child6"  << "top11,child7"
                                                                            << "top11,child8"  << "top11,child9"  << "top11,child10" << "top11,child11"
                                                                            << "top11,child12" << "top11,child13"                   << "top11,child15"

                                                                << "top12"  << "top12,child0" << "top12,child1"  << "top12,child2"  << "top12,child3"
                                                                            << "top12,child4"  << "top12,child5"  << "top12,child6"  << "top12,child7"
                                                                            << "top12,child8"  << "top12,child9"  << "top12,child10" << "top12,child11"
                                                                            << "top12,child12" << "top12,child13"                   << "top12,child15"

                                                                << "top13"  << "top13,child0" << "top13,child1"  << "top13,child2"  << "top13,child3"
                                                                            << "top13,child4"  << "top13,child5"  << "top13,child6"  << "top13,child7"
                                                                            << "top13,child8"  << "top13,child9"  << "top13,child10" << "top13,child11"
                                                                            << "top13,child12" << "top13,child13"                   << "top13,child15"

                                                                /* "top14"*/<< "top14,child0" << "top14,child1"  << "top14,child2"  << "top14,child3"
                                                                            << "top14,child4"  << "top14,child5"  << "top14,child6"  << "top14,child7"
                                                                            << "top14,child8"  << "top14,child9"  << "top14,child10" << "top14,child11"
                                                                            << "top14,child12" << "top14,child13"                   << "top14,child15"

                                                                << "top15"  << "top15,child0" << "top15,child1"  << "top15,child2"  << "top15,child3"
                                                                            << "top15,child4"  << "top15,child5"  << "top15,child6"  << "top15,child7"
                                                                            << "top15,child8"  << "top15,child9"  << "top15,child10" << "top15,child11"
                                                                            << "top15,child12" << "top15,child13"                   << "top15,child15"

                                                                /* "top16"*/<< "top16,child0" << "top16,child1"  << "top16,child2"  << "top16,child3"
                                                                            << "top16,child4"  << "top16,child5"  << "top16,child6"  << "top16,child7"
                                                                            << "top16,child8"  << "top16,child9"  << "top16,child10" << "top16,child11"
                                          << "top16,child12" << "top16,child13" << "top16,child15");



    QTest::newRow("Match Disabled") << 0 << (int)QTreeWidgetItemIterator::Disabled
                                    << (QStringList()
                                                                            << "top0,child11"
                                                                            << "top1,child11"
                                                                            << "top2,child11"
                                                                            << "top3,child11"
                                                                            << "top4,child11"
                                                                            << "top5,child11"
                                                                            << "top6,child11"
                                                                            << "top7,child11"
                                                                            << "top8,child11"
                                                                            << "top9,child11"
                                                                            << "top10,child11"
                                                                << "top11"
                                                                            << "top11,child0"
                                                                            << "top11,child1"
                                                                            << "top11,child2"
                                                                            << "top11,child3"
                                                                            << "top11,child4"
                                                                            << "top11,child5"
                                                                            << "top11,child6"
                                                                            << "top11,child7"
                                                                            << "top11,child8"
                                                                            << "top11,child9"
                                                                            << "top11,child10"
                                                                            << "top11,child11"
                                                                            << "top11,child12"
                                                                            << "top11,child13"
                                                                            << "top11,child14"
                                                                            << "top11,child15"
                                                                            << "top11,child16"

                                                                            << "top12,child11"
                                                                            << "top13,child11"
                                                                            << "top14,child11"
                                                                            << "top15,child11"
                                        << "top16,child11");

    QTest::newRow("Match Editable") << 0 << (int)QTreeWidgetItemIterator::Editable
                                    << (QStringList()
                                                                            << "top0,child12"
                                                                            << "top1,child12"
                                                                            << "top2,child12"
                                                                            << "top3,child12"
                                                                            << "top4,child12"
                                                                            << "top5,child12"
                                                                            << "top6,child12"
                                                                            << "top7,child12"
                                                                            << "top8,child12"
                                                                            << "top9,child12"
                                                                            << "top10,child12"
                                                                            << "top11,child12"
                                                                << "top12"
                                                                            << "top12,child12"
                                                                            << "top13,child12"
                                                                            << "top14,child12"
                                                                            << "top15,child12"
                                        << "top16,child12");

    QTest::newRow("Match mutually exclusive Hidden|NotHidden") << 0 << (int)(QTreeWidgetItemIterator::Hidden|QTreeWidgetItemIterator::NotHidden)
                                                               << QStringList();
    QTest::newRow("Match mutually exclusive Selected|Unselected") << 0 << (int)(QTreeWidgetItemIterator::Selected|QTreeWidgetItemIterator::Unselected)
                                                                  << QStringList();
    QTest::newRow("Match mutually exclusive Selectable|NotSelectable") << 0 << (int)(QTreeWidgetItemIterator::Selectable|QTreeWidgetItemIterator::NotSelectable)
                                                                       << QStringList();
    QTest::newRow("Match mutually exclusive DragEnabled|DragDisabled") << 0 << (int)(QTreeWidgetItemIterator::DragEnabled|QTreeWidgetItemIterator::DragDisabled)
                                                                       << QStringList();
    QTest::newRow("Match mutually exclusive DropEnabled|DropDisabled") << 0 << (int)(QTreeWidgetItemIterator::DropEnabled|QTreeWidgetItemIterator::DropDisabled)
                                                                       << QStringList();
    QTest::newRow("Match mutually exclusive HasChildren|NoChildren") << 0 << (int)(QTreeWidgetItemIterator::HasChildren|QTreeWidgetItemIterator::NoChildren)
                                                                     << QStringList();
    QTest::newRow("Match mutually exclusive Checked|NotChecked") << 0 << (int)(QTreeWidgetItemIterator::Checked|QTreeWidgetItemIterator::NotChecked)
                                                                 << QStringList();
    QTest::newRow("Match mutually exclusive Disabled|Enabled") << 0 << (int)(QTreeWidgetItemIterator::Disabled|QTreeWidgetItemIterator::Enabled)
                                                               << QStringList();
    QTest::newRow("Match mutually exclusive Editable|NotEditable") << 0 << (int)(QTreeWidgetItemIterator::Editable|QTreeWidgetItemIterator::NotEditable)
                                                                   << QStringList();
}

void tst_QTreeWidgetItemIterator::iteratorflags()
{
    QFETCH(int, start);
    QFETCH(int, iteratorflags);
    QFETCH(QStringList, matches);

    QTreeWidgetItemIterator it(testWidget, QTreeWidgetItemIterator::IteratorFlags(iteratorflags));
    it+=start;
    int iMatch = 0;
    while (*it && iMatch < matches.count()) {
        QTreeWidgetItem *item = *it;
        QCOMPARE(item->text(0), matches[iMatch]);
        ++it;
        ++iMatch;
    }
    // Make sure the expected result does not contain *more* items than the actual result.
    QCOMPARE(iMatch, matches.size());
}

void tst_QTreeWidgetItemIterator::preincrement()
{
    QTreeWidgetItemIterator it(testWidget, QTreeWidgetItemIterator::All);
    QTreeWidgetItem *item = *(++it);
    // should be the second one
    QCOMPARE(item->text(0), QString("top0,child0"));
}

void tst_QTreeWidgetItemIterator::postincrement()
{
    QTreeWidgetItemIterator it(testWidget, QTreeWidgetItemIterator::All);
    QTreeWidgetItem *item = *(it++);
    // should be the first one
    QCOMPARE(item->text(0), QString("top0"));
}

void tst_QTreeWidgetItemIterator::predecrement()
{
    QTreeWidgetItemIterator it(testWidget, QTreeWidgetItemIterator::All);
    QTreeWidgetItem *item = *(++it);
    // should be the second one
    QCOMPARE(item->text(0), QString("top0,child0"));

    item = *(--it);
    QCOMPARE(item->text(0), QString("top0"));

}

void tst_QTreeWidgetItemIterator::postdecrement()
{
    QTreeWidgetItemIterator it(testWidget, QTreeWidgetItemIterator::All);
    QTreeWidgetItem *item = *(it++);
    // should be the first one
    QCOMPARE(item->text(0), QString("top0"));

    //Iterator points to second one
    item = *(it--);
    QCOMPARE(item->text(0), QString("top0,child0"));

}

void tst_QTreeWidgetItemIterator::plus_eq_data()
{
    QTest::addColumn<int>("start");
    QTest::addColumn<int>("addition");
    QTest::addColumn<int>("iteratorflags");
    QTest::addColumn<QString>("expecteditem");

    QTest::newRow("+=0") << 0 << 0 << (int)QTreeWidgetItemIterator::All << QString("top0");
    QTest::newRow("+=1") << 0 << 1 << (int)QTreeWidgetItemIterator::All << QString("top0,child0");
    QTest::newRow("+=2") << 0 << 2 << (int)QTreeWidgetItemIterator::All << QString("top0,child1");
    QTest::newRow("+=(-1)") << 1 << -1 << (int)QTreeWidgetItemIterator::All << QString("top0");
    QTest::newRow("+=(-2)") << 3 << -2 << (int)QTreeWidgetItemIterator::All << QString("top0,child0");
}

void tst_QTreeWidgetItemIterator::plus_eq()
{
    QFETCH(int, start);
    QFETCH(int, addition);
    QFETCH(int, iteratorflags);
    QFETCH(QString, expecteditem);

    QTreeWidgetItemIterator it(testWidget, QTreeWidgetItemIterator::IteratorFlags(iteratorflags));
    it+=start;
    it+=addition;
    QTreeWidgetItem *item = *it;

    QVERIFY(item);
    QCOMPARE(item->text(0), expecteditem);

}

void tst_QTreeWidgetItemIterator::minus_eq_data()
{
    QTest::addColumn<int>("start");
    QTest::addColumn<int>("subtraction");
    QTest::addColumn<int>("iteratorflags");
    QTest::addColumn<QString>("expecteditem");

    QTest::newRow("0-=0") << 0 << 0 << (int)QTreeWidgetItemIterator::All << QString("top0");
    QTest::newRow("2-=1") << 2 << 1 << (int)QTreeWidgetItemIterator::All << QString("top0,child0");
    QTest::newRow("4-=2") << 4 << 2 << (int)QTreeWidgetItemIterator::All << QString("top0,child1");
    QTest::newRow("0-=(-1)") << 0 << -1 << (int)QTreeWidgetItemIterator::All << QString("top0,child0");
    QTest::newRow("0-=(-2)") << 0 << -2 << (int)QTreeWidgetItemIterator::All << QString("top0,child1");
    QTest::newRow("18-=1") << 18 << 1 << (int)QTreeWidgetItemIterator::All << QString("top0,child16");
    QTest::newRow("1-=1") << 1 << 1 << (int)QTreeWidgetItemIterator::All << QString("top0");
}

void tst_QTreeWidgetItemIterator::minus_eq()
{
    QFETCH(int, start);
    QFETCH(int, subtraction);
    QFETCH(int, iteratorflags);
    QFETCH(QString, expecteditem);

    QTreeWidgetItemIterator it(testWidget, QTreeWidgetItemIterator::IteratorFlags(iteratorflags));
    it+=start;
    it-=subtraction;
    QTreeWidgetItem *item = *it;
    // should be the first one
    QVERIFY(item);
    QCOMPARE(item->text(0), expecteditem);
}

void tst_QTreeWidgetItemIterator::updateIfModifiedFromWidget_data()
{
    QTest::addColumn<int>("topLevelItems");
    QTest::addColumn<int>("childItems");
    QTest::addColumn<int>("grandChildItems");
    QTest::addColumn<int>("iteratorflags");
    QTest::addColumn<int>("removeindex");
    QTest::addColumn<int>("expecteditemindex");
    QTest::addColumn<QString>("expecteditemvalue");
    QTest::addColumn<QString>("expectedUpdatedCurrent");
    QTest::addColumn<int>("expecteditemIsNull");

    QTest::newRow("Remove 3, check 1") << 3 << 3 << 0 << (int)QTreeWidgetItemIterator::All
                << 3 << 1 << QString("top0,child0") << QString("top1") << 0;
    QTest::newRow("Remove 1, check 0") << 3 << 3 << 0 << (int)QTreeWidgetItemIterator::All
                << 1 << 0 << QString("top0") << QString("top0,child1") << 0;
    QTest::newRow("Remove 2, check 2") << 3 << 3 << 0 << (int)QTreeWidgetItemIterator::All
                << 2 << 2 << QString("top0,child2") << QString("top0,child2") << 0;
    QTest::newRow("Remove 0, check 0") << 3 << 3 << 3 << (int)QTreeWidgetItemIterator::All
                << 0 << 0 << QString("top1") << QString("top1") << 0;
    QTest::newRow("Remove top1, check top1") << 3 << 3 << 3 << (int)QTreeWidgetItemIterator::All
                << 13 << 13 << QString("top2") << QString("top2") << 0;
    QTest::newRow("Remove top0, check top1") << 3 << 3 << 3 << (int)QTreeWidgetItemIterator::All
                << 0 << 13 << QString("top1") << QString("top1") << 0;
    QTest::newRow("Remove (top0,child1), check (top0,child1)") << 3 << 3 << 3 << (int)QTreeWidgetItemIterator::All
                << 5 << 5 << QString("top0,child2") << QString("top0,child2") << 0;
    QTest::newRow("Remove (t0,c0) check (t0,c0)") << 3 << 3 << 3 << (int)QTreeWidgetItemIterator::All
                << 1 << 1 << QString("top0,child1") << QString("top0,child1") << 0;
    QTest::newRow("Remove (t0,c1) check (t0,c1)") << 3 << 3 << 3 << (int)QTreeWidgetItemIterator::All
                << 5 << 5 << QString("top0,child2") << QString("top0,child2") << 0;
    QTest::newRow("Remove (t0) check (t0,c1)") << 3 << 3 << 0 << (int)QTreeWidgetItemIterator::All
                << 0 << 4 << QString("top1") << QString("top1") << 0;
    QTest::newRow("Remove (t0) check (t0,c0,g1)") << 3 << 3 << 3 << (int)QTreeWidgetItemIterator::All
                << 0 << 3 << QString("top1") << QString("top1") << 0;
    QTest::newRow("Remove (top2), check if top2 is null") << 3 << 3 << 3 << (int)QTreeWidgetItemIterator::All
                << 2*13 << 2*13 << QString() << QString() << 1;
    QTest::newRow("Remove last item, check if iterator::current returns 0")
                << 3 << 0 << 0 << (int)QTreeWidgetItemIterator::All << 2 << 2 << QString() << QString() << 1;
    QTest::newRow("remove 1, iterator points to 3, should move to 1")
                << 3 << 3 << 3 << (int)QTreeWidgetItemIterator::All << 1 << 3 << QString("top0,child1") << QString("top0,child1") << 0;
}

void tst_QTreeWidgetItemIterator::updateIfModifiedFromWidget()
{
    QFETCH(int, topLevelItems);
    QFETCH(int, childItems);
    QFETCH(int, grandChildItems);
    QFETCH(int, iteratorflags);
    QFETCH(int, removeindex);
    QFETCH(int, expecteditemindex);
    QFETCH(QString, expecteditemvalue);
    QFETCH(QString, expectedUpdatedCurrent);
    QFETCH(int, expecteditemIsNull);

    QTreeWidget tw;
    tw.clear();
    tw.setColumnCount(2);
    for (int i1=0; i1 < topLevelItems; ++i1) {
        QTreeWidgetItem *top = new QTreeWidgetItem(&tw);
        top->setText(0, QString("top%1").arg(i1));
        for (int i2=0; i2 < childItems; ++i2) {
            QTreeWidgetItem *child = new QTreeWidgetItem(top);
            child->setText(0, QString("top%1,child%2").arg(i1).arg(i2));
            for (int i3=0; i3 < grandChildItems; ++i3) {
                QTreeWidgetItem *grandChild = new QTreeWidgetItem(child);
                grandChild->setText(0, QString("top%1,child%2,grandchild%3").arg(i1).arg(i2).arg(i3));
            }
        }
    }

    QTreeWidgetItemIterator it(&tw, QTreeWidgetItemIterator::IteratorFlags(iteratorflags));
    it+=expecteditemindex;
    QTreeWidgetItem *item = 0;
    QTreeWidgetItemIterator itRemove(&tw, QTreeWidgetItemIterator::IteratorFlags(iteratorflags));
    itRemove+=removeindex;
    item = *itRemove;
    QVERIFY(item);
    delete item;
    item = *it;
    if (expecteditemIsNull) {
        QVERIFY(!item);
    } else {
        QVERIFY(item);
        QCOMPARE(item->text(0), expecteditemvalue);
        item = *itRemove;
        if (expectedUpdatedCurrent.isNull()) {
            QVERIFY(!item);
        } else {
            QCOMPARE(item->text(0), expectedUpdatedCurrent);
        }
    }
}

void tst_QTreeWidgetItemIterator::updateIteratorAfterDeletedItem_and_ContinueIteration_data()
{
    QTest::addColumn<int>("topLevelItems");
    QTest::addColumn<int>("childItems");
    QTest::addColumn<int>("grandChildItems");       // Populate the tree data
    // we have one iterator pointing to an item in the tree.
    // This iterator will be updated if we delete the item it is pointing to.
    //
    QTest::addColumn<int>("removeindex");                   // The index of the node we want to remove
    QTest::addColumn<int>("iterator_initial_index");        // The new expected index of
    QTest::addColumn<int>("iterator_advance_after_removal");
    QTest::addColumn<QString>("iterator_new_value");        // The new current item value of the iterator
    QTest::newRow("Remove 13, it points to 25, it-=1. We should get top0,child2,grandchild2") << 3 << 3 << 3 << 13 << 25 << -1 << QString("top0,child2,grandchild2");
    QTest::newRow("Remove 0, it points to 12, it+=1. We should get top1,child0") << 3 << 3 << 3 << 0 << 12 << 1 << QString("top1,child0");
    QTest::newRow("Remove 0, it points to 12, it-=1. We should get 0")   << 3 << 3 << 3 << 0 << 12 << -1 << QString();
    QTest::newRow("Remove 0, it points to 1, it+=1. We should get top2") << 4 << 0 << 0 << 0 << 1 << 1 << QString("top2");
    QTest::newRow("Remove 2, it points to 1, it+=0. We should get top1") << 4 << 0 << 0 << 2 << 1 << 0 << QString("top1");
    QTest::newRow("Remove 2, it points to 1, it+=1. We should get top3") << 4 << 0 << 0 << 2 << 1 << 1 << QString("top3");
    QTest::newRow("Remove 1, it points to 2, it+=1. We should get top0,child2") << 3 << 3 << 0 << 1 << 2 << 1 << QString("top0,child2");
    QTest::newRow("Remove 1, it points to 2, it+=0. We should get top0,child1") << 3 << 3 << 0 << 1 << 2 << 0 << QString("top0,child1");
    QTest::newRow("Remove 1, it points to 2, it-=1. We should get top0") << 3 << 3 << 0 << 1 << 2 << -1 << QString("top0");
}

void tst_QTreeWidgetItemIterator::updateIteratorAfterDeletedItem_and_ContinueIteration()
{
    QFETCH(int, topLevelItems);
    QFETCH(int, childItems);
    QFETCH(int, grandChildItems);
    QFETCH(int, removeindex);
    QFETCH(int, iterator_initial_index);
    QFETCH(int, iterator_advance_after_removal);
    QFETCH(QString, iterator_new_value);

    QTreeWidget tw;
    tw.clear();
    tw.setColumnCount(2);
    for (int i1=0; i1 < topLevelItems; ++i1) {
        QTreeWidgetItem *top = new QTreeWidgetItem(&tw);
        top->setText(0, QString("top%1").arg(i1));
        for (int i2=0; i2 < childItems; ++i2) {
            QTreeWidgetItem *child = new QTreeWidgetItem(top);
            child->setText(0, QString("top%1,child%2").arg(i1).arg(i2));
            for (int i3=0; i3 < grandChildItems; ++i3) {
                QTreeWidgetItem *grandChild = new QTreeWidgetItem(child);
                grandChild->setText(0, QString("top%1,child%2,grandchild%3").arg(i1).arg(i2).arg(i3));
            }
        }
    }

    QTreeWidgetItemIterator it(&tw, QTreeWidgetItemIterator::All);
    it += iterator_initial_index;
    QTreeWidgetItem *item = 0;
    QTreeWidgetItemIterator itRemove(&tw, QTreeWidgetItemIterator::All);
    itRemove+=removeindex;
    item = *itRemove;
    QVERIFY(item);
    delete item;
    it+=iterator_advance_after_removal;
    if (iterator_new_value.isNull()) {
        QCOMPARE((*it), (QTreeWidgetItem*)0);
    } else {
        QCOMPARE((*it)->text(0), iterator_new_value);
    }
}

void tst_QTreeWidgetItemIterator::constructIteratorWithItem_data()
{
    QTest::addColumn<int>("indextoitem");
    QTest::addColumn<int>("iteratorflags");
    QTest::addColumn<QString>("expecteditem");

    QTest::newRow("index 0")            << 0 << 0 << QString("top0");
    QTest::newRow("index 1")            << 1 << 0 << QString("top0,child0");
    QTest::newRow("index 2")            << 2 << 0 << QString("top0,child1");
    QTest::newRow("index 30")           << 30 << 0 << QString("top1,child11");
    QTest::newRow("305 (last item)")    << 305 << 0 << QString("top16,child16");
    QTest::newRow("index 0, advance to next matching node") << 0 << (int)QTreeWidgetItemIterator::NotHidden << QString("top0,child1");
}

void tst_QTreeWidgetItemIterator::constructIteratorWithItem()
{
    QFETCH(int, indextoitem);
    QFETCH(int, iteratorflags);
    QFETCH(QString, expecteditem);

    QTreeWidgetItemIterator it(testWidget);
    it+=indextoitem;
    QTreeWidgetItem *item = *it;
    QTreeWidgetItemIterator it2(item, QTreeWidgetItemIterator::IteratorFlags(iteratorflags));
    QTreeWidgetItem *item2 = *it2;

    QVERIFY(item2);
    QCOMPARE(item2->text(0), expecteditem);

}

void tst_QTreeWidgetItemIterator::initializeIterator()
{
    QTreeWidget tw;
    QTreeWidgetItemIterator it(&tw);

    QCOMPARE((*it), static_cast<QTreeWidgetItem*>(0));
}

void tst_QTreeWidgetItemIterator::sortingEnabled()
{
    QTreeWidget *tree = new QTreeWidget;
    tree->setColumnCount(2);
    tree->headerItem()->setText(0, "Id");
    tree->headerItem()->setText(1, "Color");

    tree->setSortingEnabled(true);
    tree->sortByColumn(0, Qt::AscendingOrder);

    QTreeWidgetItem *second = new QTreeWidgetItem;
    second->setText(0, "2");
    second->setText(1, "second");
    QTreeWidgetItem *first = new QTreeWidgetItem;
    first->setText(0, "1");
    first->setText(1, "first");

    tree->addTopLevelItem(second);
    tree->addTopLevelItem(first);

    QTreeWidgetItemIterator it(tree);
    QCOMPARE(*it, first);
    ++it;
    QCOMPARE(*it, second);
}

QTEST_MAIN(tst_QTreeWidgetItemIterator)
#include "tst_qtreewidgetitemiterator.moc"
