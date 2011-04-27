/****************************************************************************
**
** Copyright (C) 2011 Nokia Corporation and/or its subsidiary(-ies).
** All rights reserved.
** Contact: Nokia Corporation (qt-info@nokia.com)
**
** This file is part of the test suite of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:LGPL$
** No Commercial Usage
** This file contains pre-release code and may not be distributed.
** You may use this file in accordance with the terms and conditions
** contained in the Technology Preview License Agreement accompanying
** this package.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 2.1 as published by the Free Software
** Foundation and appearing in the file LICENSE.LGPL included in the
** packaging of this file.  Please review the following information to
** ensure the GNU Lesser General Public License version 2.1 requirements
** will be met: http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, Nokia gives you certain additional
** rights.  These rights are described in the Nokia Qt LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
** If you have questions regarding the use of this file, please contact
** Nokia at qt-info@nokia.com.
**
**
**
**
**
**
**
**
** $QT_END_LICENSE$
**
****************************************************************************/


#include <QtTest/QtTest>
#include <qapplication.h>
#include <qsplitter.h>
#include <qstyle.h>
#include <qfile.h>
#include <qtextstream.h>
#include <qlayout.h>
#include <qabstractscrollarea.h>
#include <qgraphicsview.h>
#include <qmdiarea.h>
#include <qscrollarea.h>
#include <qtextedit.h>
#include <qtreeview.h>
#include <qlabel.h>
#include <qdebug.h> // for file error messages
#include "../../shared/util.h"

#if defined(Q_OS_SYMBIAN)
# define SRCDIR ""
#endif

//TESTED_CLASS=
//TESTED_FILES=

QT_FORWARD_DECLARE_CLASS(QSplitter)
QT_FORWARD_DECLARE_CLASS(QWidget)
class tst_QSplitter : public QObject
{
    Q_OBJECT

public:
    tst_QSplitter();
    virtual ~tst_QSplitter();

public slots:
    void initTestCase();
    void cleanupTestCase();
    void init();
private slots:
    void getSetCheck();
    void sizes(); // bare (as in empty)
    void setSizes3();
    void setSizes3_data();
    void setSizes();
    void setSizes_data();
    void saveAndRestoreState();
    void saveAndRestoreState_data();
    void saveState_data();
    void addWidget();
    void insertWidget();
    void setStretchFactor_data();
    void setStretchFactor();
    void testShowHide_data();
    void testShowHide();
    void testRemoval();
    void rubberBandNotInSplitter();
    void saveAndRestoreStateOfNotYetShownSplitter();

    // task-specific tests below me:
    void task187373_addAbstractScrollAreas();
    void task187373_addAbstractScrollAreas_data();
    void task169702_sizes();
    void taskQTBUG_4101_ensureOneNonCollapsedWidget_data();
    void taskQTBUG_4101_ensureOneNonCollapsedWidget();

private:
    void removeThirdWidget();
    void addThirdWidget();
    QSplitter *splitter;
    QWidget *w1;
    QWidget *w2;
    QWidget *w3;
};

// Testing get/set functions
void tst_QSplitter::getSetCheck()
{
    QSplitter obj1;
    // bool QSplitter::opaqueResize()
    // void QSplitter::setOpaqueResize(bool)
    obj1.setOpaqueResize(false);
    QCOMPARE(false, obj1.opaqueResize());
    obj1.setOpaqueResize(true);
    QCOMPARE(true, obj1.opaqueResize());
}

tst_QSplitter::tst_QSplitter()
    : w1(0), w2(0), w3(0)
{
}

tst_QSplitter::~tst_QSplitter()
{
}

void tst_QSplitter::sizes()
{
    DEPENDS_ON("setSizes");
}

void tst_QSplitter::initTestCase()
{
    splitter = new QSplitter(Qt::Horizontal);
    w1 = new QWidget;
    w2 = new QWidget;
    splitter->addWidget(w1);
    splitter->addWidget(w2);
#if defined(QT3_SUPPORT)
    qApp->setMainWidget(splitter);
#endif
}

void tst_QSplitter::init()
{
    removeThirdWidget();
    w1->show();
    w2->show();
    w1->setMinimumSize(0, 0);
    w2->setMinimumSize(0, 0);
    splitter->setSizes(QList<int>() << 200 << 200);
    qApp->sendPostedEvents();
}

void tst_QSplitter::removeThirdWidget()
{
    delete w3;
    w3 = 0;
    int handleWidth = splitter->style()->pixelMetric(QStyle::PM_SplitterWidth);
    splitter->setFixedSize(400 + handleWidth, 400);
}

void tst_QSplitter::addThirdWidget()
{
    if (!w3) {
        w3 = new QWidget;
        splitter->addWidget(w3);
        int handleWidth = splitter->style()->pixelMetric(QStyle::PM_SplitterWidth);
        splitter->setFixedSize(600 + 2 * handleWidth, 400);
    }
}

void tst_QSplitter::cleanupTestCase()
{
#if defined(QT3_SUPPORT)
    delete qApp->mainWidget();
#endif
}


typedef QList<int> IntList;
Q_DECLARE_METATYPE(IntList)

void tst_QSplitter::setSizes3()
{
    QFETCH(IntList, minimumSizes);
    QFETCH(IntList, splitterSizes);
    QFETCH(IntList, collapsibleStates);
    QFETCH(bool, childrenCollapse);

    QCOMPARE(minimumSizes.size(), splitterSizes.size());
    if (minimumSizes.size() > 2)
        addThirdWidget();
    for (int i = 0; i < minimumSizes.size(); ++i) {
        QWidget *w = splitter->widget(i);
        w->setMinimumWidth(minimumSizes.at(i));
        splitter->setCollapsible(splitter->indexOf(w), collapsibleStates.at(i));
    }
    splitter->setChildrenCollapsible(childrenCollapse);
    splitter->setSizes(splitterSizes);
    QTEST(splitter->sizes(), "expectedSizes");
}

void tst_QSplitter::setSizes3_data()
{
    QTest::addColumn<IntList>("minimumSizes");
    QTest::addColumn<IntList>("splitterSizes");
    QTest::addColumn<IntList>("expectedSizes");
    QTest::addColumn<IntList>("collapsibleStates");
    QTest::addColumn<bool>("childrenCollapse");

    QFile file(SRCDIR "setSizes3.dat");
    if (!file.open(QIODevice::ReadOnly)) {
        qDebug() << "Can't open file, reason:" << file.errorString();
        return;
    }
    QTextStream ts(&file);
    ts.setIntegerBase(10);

    QString dataName;
    IntList minimumSizes;
    IntList splitterSizes;
    IntList expectedSizes;
    IntList collapsibleStates;
    int childrenCollapse;
    while (!ts.atEnd()) {
        int i1, i2, i3;
        minimumSizes.clear();
        splitterSizes.clear();
        expectedSizes.clear();
        collapsibleStates.clear();
        ts >> dataName;
        ts >> i1 >> i2 >> i3;
        minimumSizes << i1 << i2 << i3;
        ts >> i1 >> i2 >> i3;
        splitterSizes << i1 << i2 << i3;
        ts >> i1 >> i2 >> i3;
        expectedSizes << i1 << i2 << i3;
        ts >> i1 >> i2 >> i3;
        collapsibleStates << i1 << i2 << i3;
        ts >> childrenCollapse;
        QTest::newRow(dataName.toLocal8Bit()) << minimumSizes << splitterSizes << expectedSizes << collapsibleStates << bool(childrenCollapse);
        ts.skipWhiteSpace();
    }
}

void tst_QSplitter::setSizes()
{
#if !defined(QT3_SUPPORT)
    QSKIP("No Qt3Support", SkipAll);
#else
    QFETCH(int, minSize1);
    QFETCH(int, minSize2);
    QFETCH(int, splitterSize1);
    QFETCH(int, splitterSize2);
    QFETCH(bool, collapse1);
    QFETCH(bool, collapse2);
    QFETCH(bool, childrencollapse);
    QFETCH(int, resizeMode1);
    QFETCH(int, resizeMode2);
    QList<int> mySizes;
    mySizes << splitterSize1 << splitterSize2;
    w1->setMinimumWidth(minSize1);
    w2->setMinimumWidth(minSize2);
    splitter->setCollapsible(w1, collapse1);
    splitter->setCollapsible(w2, collapse2);
    splitter->setChildrenCollapsible(childrencollapse);
    splitter->setResizeMode(w1, (QSplitter::ResizeMode)resizeMode1);
    splitter->setResizeMode(w2, (QSplitter::ResizeMode)resizeMode2);
    splitter->setSizes(mySizes);
    mySizes = splitter->sizes();
    QTEST(mySizes[0], "expected1");
    QTEST(mySizes[1], "expected2");
#endif
}

void tst_QSplitter::setSizes_data()
{
#if defined(QT3_SUPPORT)
    QTest::addColumn<int>("minSize1");
    QTest::addColumn<int>("minSize2");
    QTest::addColumn<int>("splitterSize1");
    QTest::addColumn<int>("splitterSize2");
    QTest::addColumn<int>("expected1");
    QTest::addColumn<int>("expected2");
    QTest::addColumn<bool>("collapse1");
    QTest::addColumn<bool>("collapse2");
    QTest::addColumn<bool>("childrencollapse");
    QTest::addColumn<int>("resizeMode1");
    QTest::addColumn<int>("resizeMode2");
    QTest::newRow("ok00") << 100 << 50 << 100 << 300 << 100 << 300
			 << (bool)true << (bool)true << (bool)true
			 << (int)QSplitter::Auto << (int)QSplitter::Auto;
    QTest::newRow("ok01") << 100 << 100 << 50 << 350 << 100 << 300
			 << (bool)true << (bool)true << (bool)true
			 << (int)QSplitter::Auto << (int)QSplitter::Auto;
    QTest::newRow("ok02") << 100 << 100 << 350 << 50 << 300 << 100
			 << (bool)true << (bool)true << (bool)true
			 << (int)QSplitter::Auto << (int)QSplitter::Auto;
    QTest::newRow("ok03") << 200 << 200 << 350 << 50 << 200 << 200
			 << (bool)true << (bool)true << (bool)true
			 << (int)QSplitter::Auto << (int)QSplitter::Auto;
    QTest::newRow("ok04") << 200 << 200 << 200 << 200 << 200 << 200
			 << (bool)true << (bool)true << (bool)true
			 << (int)QSplitter::Auto << (int)QSplitter::Auto;
    QTest::newRow("ok05") << 200 << 200 << 0 << 350 << 0 << 400
			 << (bool)true << (bool)true << (bool)true
			 << (int)QSplitter::Auto << (int)QSplitter::Auto;
    QTest::newRow("ok06") << 200 << 200 << 350 << 0 << 400 << 0
			 << (bool)true << (bool)true << (bool)true
			 << (int)QSplitter::Auto << (int)QSplitter::Auto;
    QTest::newRow("ok07") << 200 << 200 << 350 << 0 << 200 << 200
			 << (bool)true << (bool)false << (bool)true
			 << (int)QSplitter::Auto << (int)QSplitter::Auto;
    QTest::newRow("ok08") << 200 << 200 << 350 << 0 << 200 << 200
			 << (bool)false << (bool)false << (bool)true
			 << (int)QSplitter::Auto << (int)QSplitter::Auto;
    QTest::newRow("ok09") << 200 << 200 << 0 << 350 << 200 << 200
			 << (bool)false << (bool)true << (bool)true
			 << (int)QSplitter::Auto << (int)QSplitter::Auto;
    QTest::newRow("ok10") << 200 << 200 << 0 << 350 << 200 << 200
			 << (bool)false << (bool)false << (bool)true
			 << (int)QSplitter::Auto << (int)QSplitter::Auto;

    QTest::newRow("ok20") << 100 << 50 << 100 << 300 << 100 << 300
			 << (bool)true << (bool)true << (bool)false
			 << (int)QSplitter::Auto << (int)QSplitter::Auto;
    QTest::newRow("ok21") << 100 << 100 << 50 << 350 << 100 << 300
			 << (bool)true << (bool)true << (bool)false
			 << (int)QSplitter::Auto << (int)QSplitter::Auto;
    QTest::newRow("ok22") << 100 << 100 << 350 << 50 << 300 << 100
			 << (bool)true << (bool)true << (bool)false
			 << (int)QSplitter::Auto << (int)QSplitter::Auto;
    QTest::newRow("ok23") << 200 << 200 << 350 << 50 << 200 << 200
			 << (bool)true << (bool)true << (bool)false
			 << (int)QSplitter::Auto << (int)QSplitter::Auto;
    QTest::newRow("ok24") << 200 << 200 << 200 << 200 << 200 << 200
			 << (bool)true << (bool)true << (bool)false
			 << (int)QSplitter::Auto << (int)QSplitter::Auto;
    QTest::newRow("ok25") << 200 << 200 << 0 << 350 << 0 << 400
			 << (bool)true << (bool)true << (bool)false
			 << (int)QSplitter::Auto << (int)QSplitter::Auto;
    QTest::newRow("ok26") << 200 << 200 << 350 << 0 << 400 << 0
			 << (bool)true << (bool)true << (bool)false
			 << (int)QSplitter::Auto << (int)QSplitter::Auto;
    QTest::newRow("ok27") << 200 << 200 << 350 << 0 << 200 << 200
			 << (bool)true << (bool)false << (bool)false
			 << (int)QSplitter::Auto << (int)QSplitter::Auto;
    QTest::newRow("ok28") << 200 << 200 << 350 << 0 << 200 << 200
			 << (bool)false << (bool)false << (bool)false
			 << (int)QSplitter::Auto << (int)QSplitter::Auto;
    QTest::newRow("ok29") << 200 << 200 << 0 << 350 << 200 << 200
			 << (bool)false << (bool)true << (bool)false
			 << (int)QSplitter::Auto << (int)QSplitter::Auto;
    QTest::newRow("ok30") << 200 << 200 << 0 << 350 << 200 << 200
			 << (bool)false << (bool)false << (bool)false
			 << (int)QSplitter::Auto << (int)QSplitter::Auto;
    QTest::newRow("ok40") << 100 << 50 << 100 << 300 << 100 << 300
			 << (bool)true << (bool)true << (bool)false
			 << (int)QSplitter::Stretch << (int)QSplitter::Auto;
    QTest::newRow("ok41") << 100 << 100 << 50 << 350 << 100 << 300
			 << (bool)true << (bool)true << (bool)false
			 << (int)QSplitter::Stretch << (int)QSplitter::Auto;
    QTest::newRow("ok42") << 100 << 100 << 350 << 50 << 300 << 100
			 << (bool)true << (bool)true << (bool)false
			 << (int)QSplitter::Stretch << (int)QSplitter::Auto;
    QTest::newRow("ok43") << 200 << 200 << 350 << 50 << 200 << 200
			 << (bool)true << (bool)true << (bool)false
			 << (int)QSplitter::Stretch << (int)QSplitter::Auto;
    QTest::newRow("ok44") << 200 << 200 << 200 << 200 << 200 << 200
			 << (bool)true << (bool)true << (bool)false
			 << (int)QSplitter::Stretch << (int)QSplitter::Auto;
    QTest::newRow("ok45") << 200 << 200 << 0 << 350 << 0 << 400
			 << (bool)true << (bool)true << (bool)false
			 << (int)QSplitter::Stretch << (int)QSplitter::Auto;
    QTest::newRow("ok46") << 200 << 200 << 350 << 0 << 400 << 0
			 << (bool)true << (bool)true << (bool)false
			 << (int)QSplitter::Stretch << (int)QSplitter::Auto;
    QTest::newRow("ok47") << 200 << 200 << 350 << 0 << 200 << 200
			 << (bool)true << (bool)false << (bool)false
			 << (int)QSplitter::Stretch << (int)QSplitter::Auto;
    QTest::newRow("ok48") << 200 << 200 << 350 << 0 << 200 << 200
			 << (bool)false << (bool)false << (bool)false
			 << (int)QSplitter::Stretch << (int)QSplitter::Auto;
    QTest::newRow("ok49") << 200 << 200 << 0 << 350 << 200 << 200
			 << (bool)false << (bool)true << (bool)false
			 << (int)QSplitter::Stretch << (int)QSplitter::Auto;
    QTest::newRow("ok50") << 200 << 200 << 0 << 350 << 200 << 200
			 << (bool)false << (bool)false << (bool)false
			 << (int)QSplitter::Stretch << (int)QSplitter::Auto;

    QTest::newRow("ok60") << 100 << 50 << 100 << 300 << 100 << 300
			 << (bool)true << (bool)true << (bool)true
			 << (int)QSplitter::Stretch << (int)QSplitter::Auto;
    QTest::newRow("ok61") << 100 << 100 << 50 << 350 << 100 << 300
			 << (bool)true << (bool)true << (bool)true
			 << (int)QSplitter::Stretch << (int)QSplitter::Auto;
    QTest::newRow("ok62") << 100 << 100 << 350 << 50 << 300 << 100
			 << (bool)true << (bool)true << (bool)true
			 << (int)QSplitter::Stretch << (int)QSplitter::Auto;
    QTest::newRow("ok63") << 200 << 200 << 350 << 50 << 200 << 200
			 << (bool)true << (bool)true << (bool)true
			 << (int)QSplitter::Stretch << (int)QSplitter::Auto;
    QTest::newRow("ok64") << 200 << 200 << 200 << 200 << 200 << 200
			 << (bool)true << (bool)true << (bool)true
			 << (int)QSplitter::Stretch << (int)QSplitter::Auto;
    QTest::newRow("ok65") << 200 << 200 << 0 << 350 << 0 << 400
			 << (bool)true << (bool)true << (bool)true
			 << (int)QSplitter::Stretch << (int)QSplitter::Auto;
    QTest::newRow("ok66") << 200 << 200 << 350 << 0 << 400 << 0
			 << (bool)true << (bool)true << (bool)true
			 << (int)QSplitter::Stretch << (int)QSplitter::Auto;
    QTest::newRow("ok67") << 200 << 200 << 350 << 0 << 200 << 200
			 << (bool)true << (bool)false << (bool)true
			 << (int)QSplitter::Stretch << (int)QSplitter::Auto;
    QTest::newRow("ok68") << 200 << 200 << 350 << 0 << 200 << 200
			 << (bool)false << (bool)false << (bool)true
			 << (int)QSplitter::Stretch << (int)QSplitter::Auto;
    QTest::newRow("ok69") << 200 << 200 << 0 << 350 << 200 << 200
			 << (bool)false << (bool)true << (bool)true
			 << (int)QSplitter::Stretch << (int)QSplitter::Auto;
    QTest::newRow("ok70") << 200 << 200 << 0 << 350 << 200 << 200
			 << (bool)false << (bool)false << (bool)true
			 << (int)QSplitter::Stretch << (int)QSplitter::Auto;

    QTest::newRow("ok80") << 100 << 50 << 100 << 300 << 100 << 300
			 << (bool)true << (bool)true << (bool)true
			 << (int)QSplitter::Auto << (int)QSplitter::Stretch;
    QTest::newRow("ok81") << 100 << 100 << 50 << 350 << 100 << 300
			 << (bool)true << (bool)true << (bool)true
			 << (int)QSplitter::Auto << (int)QSplitter::Stretch;
    QTest::newRow("ok82") << 100 << 100 << 350 << 50 << 300 << 100
			 << (bool)true << (bool)true << (bool)true
			 << (int)QSplitter::Auto << (int)QSplitter::Stretch;
    QTest::newRow("ok83") << 200 << 200 << 350 << 50 << 200 << 200
			 << (bool)true << (bool)true << (bool)true
			 << (int)QSplitter::Auto << (int)QSplitter::Stretch;
    QTest::newRow("ok84") << 200 << 200 << 200 << 200 << 200 << 200
			 << (bool)true << (bool)true << (bool)true
			 << (int)QSplitter::Auto << (int)QSplitter::Stretch;
    QTest::newRow("ok85") << 200 << 200 << 0 << 350 << 0 << 400
			 << (bool)true << (bool)true << (bool)true
			 << (int)QSplitter::Auto << (int)QSplitter::Stretch;
    QTest::newRow("ok86") << 200 << 200 << 350 << 0 << 400 << 0
			 << (bool)true << (bool)true << (bool)true
			 << (int)QSplitter::Auto << (int)QSplitter::Stretch;
    QTest::newRow("ok87") << 200 << 200 << 350 << 0 << 200 << 200
			 << (bool)true << (bool)false << (bool)true
			 << (int)QSplitter::Auto << (int)QSplitter::Stretch;
    QTest::newRow("ok88") << 200 << 200 << 350 << 0 << 200 << 200
			 << (bool)false << (bool)false << (bool)true
			 << (int)QSplitter::Auto << (int)QSplitter::Stretch;
    QTest::newRow("ok89") << 200 << 200 << 0 << 350 << 200 << 200
			 << (bool)false << (bool)true << (bool)true
			 << (int)QSplitter::Auto << (int)QSplitter::Stretch;
    QTest::newRow("ok90") << 200 << 200 << 0 << 350 << 200 << 200
			 << (bool)false << (bool)false << (bool)true
			 << (int)QSplitter::Auto << (int)QSplitter::Stretch;

    QTest::newRow("ok100") << 100 << 50 << 100 << 300 << 100 << 300
			 << (bool)true << (bool)true << (bool)false
			 << (int)QSplitter::Auto << (int)QSplitter::Stretch;
    QTest::newRow("ok101") << 100 << 100 << 50 << 350 << 100 << 300
			 << (bool)true << (bool)true << (bool)false
			 << (int)QSplitter::Auto << (int)QSplitter::Stretch;
    QTest::newRow("ok102") << 100 << 100 << 350 << 50 << 300 << 100
			 << (bool)true << (bool)true << (bool)false
			 << (int)QSplitter::Auto << (int)QSplitter::Stretch;
    QTest::newRow("ok103") << 200 << 200 << 350 << 50 << 200 << 200
			 << (bool)true << (bool)true << (bool)false
			 << (int)QSplitter::Auto << (int)QSplitter::Stretch;
    QTest::newRow("ok104") << 200 << 200 << 200 << 200 << 200 << 200
			 << (bool)true << (bool)true << (bool)false
			 << (int)QSplitter::Auto << (int)QSplitter::Stretch;
    QTest::newRow("ok105") << 200 << 200 << 0 << 350 << 0 << 400
			 << (bool)true << (bool)true << (bool)false
			 << (int)QSplitter::Auto << (int)QSplitter::Stretch;
    QTest::newRow("ok106") << 200 << 200 << 350 << 0 << 400 << 0
			 << (bool)true << (bool)true << (bool)false
			 << (int)QSplitter::Auto << (int)QSplitter::Stretch;
    QTest::newRow("ok107") << 200 << 200 << 350 << 0 << 200 << 200
			 << (bool)true << (bool)false << (bool)false
			 << (int)QSplitter::Auto << (int)QSplitter::Stretch;
    QTest::newRow("ok108") << 200 << 200 << 350 << 0 << 200 << 200
			 << (bool)false << (bool)false << (bool)false
			 << (int)QSplitter::Auto << (int)QSplitter::Stretch;
    QTest::newRow("ok109") << 200 << 200 << 0 << 350 << 200 << 200
			 << (bool)false << (bool)true << (bool)false
			 << (int)QSplitter::Auto << (int)QSplitter::Stretch;
    QTest::newRow("ok110") << 200 << 200 << 0 << 350 << 200 << 200
			 << (bool)false << (bool)false << (bool)false
			 << (int)QSplitter::Auto << (int)QSplitter::Stretch;

    QTest::newRow("ok120") << 100 << 50 << 100 << 300 << 100 << 300
			 << (bool)true << (bool)true << (bool)true
			 << (int)QSplitter::KeepSize << (int)QSplitter::Auto;
    QTest::newRow("ok121") << 100 << 100 << 50 << 350 << 100 << 300
			 << (bool)true << (bool)true << (bool)true
			 << (int)QSplitter::KeepSize << (int)QSplitter::Auto;
    QTest::newRow("ok122") << 100 << 100 << 350 << 50 << 300 << 100
			 << (bool)true << (bool)true << (bool)true
			 << (int)QSplitter::KeepSize << (int)QSplitter::Auto;
    QTest::newRow("ok123") << 200 << 200 << 350 << 50 << 200 << 200
			 << (bool)true << (bool)true << (bool)true
			 << (int)QSplitter::KeepSize << (int)QSplitter::Auto;
    QTest::newRow("ok124") << 200 << 200 << 200 << 200 << 200 << 200
			 << (bool)true << (bool)true << (bool)true
			 << (int)QSplitter::KeepSize << (int)QSplitter::Auto;
    QTest::newRow("ok125") << 200 << 200 << 0 << 350 << 0 << 400
			 << (bool)true << (bool)true << (bool)true
			 << (int)QSplitter::KeepSize << (int)QSplitter::Auto;
    QTest::newRow("ok126") << 200 << 200 << 350 << 0 << 400 << 0
			 << (bool)true << (bool)true << (bool)true
			 << (int)QSplitter::KeepSize << (int)QSplitter::Auto;
    QTest::newRow("ok127") << 200 << 200 << 350 << 0 << 200 << 200
			 << (bool)true << (bool)false << (bool)true
			 << (int)QSplitter::KeepSize << (int)QSplitter::Auto;
    QTest::newRow("ok128") << 200 << 200 << 350 << 0 << 200 << 200
			 << (bool)false << (bool)false << (bool)true
			 << (int)QSplitter::KeepSize << (int)QSplitter::Auto;
    QTest::newRow("ok129") << 200 << 200 << 0 << 350 << 200 << 200
			 << (bool)false << (bool)true << (bool)true
			 << (int)QSplitter::KeepSize << (int)QSplitter::Auto;
    QTest::newRow("ok130") << 200 << 200 << 0 << 350 << 200 << 200
			 << (bool)false << (bool)false << (bool)true
			 << (int)QSplitter::KeepSize << (int)QSplitter::Auto;

    QTest::newRow("ok140") << 100 << 50 << 100 << 300 << 100 << 300
			 << (bool)true << (bool)true << (bool)false
			 << (int)QSplitter::KeepSize << (int)QSplitter::Auto;
    QTest::newRow("ok141") << 100 << 100 << 50 << 350 << 100 << 300
			 << (bool)true << (bool)true << (bool)false
			 << (int)QSplitter::KeepSize << (int)QSplitter::Auto;
    QTest::newRow("ok142") << 100 << 100 << 350 << 50 << 300 << 100
			 << (bool)true << (bool)true << (bool)false
			 << (int)QSplitter::KeepSize << (int)QSplitter::Auto;
    QTest::newRow("ok143") << 200 << 200 << 350 << 50 << 200 << 200
			 << (bool)true << (bool)true << (bool)false
			 << (int)QSplitter::KeepSize << (int)QSplitter::Auto;
    QTest::newRow("ok144") << 200 << 200 << 200 << 200 << 200 << 200
			 << (bool)true << (bool)true << (bool)false
			 << (int)QSplitter::KeepSize << (int)QSplitter::Auto;
    QTest::newRow("ok145") << 200 << 200 << 0 << 350 << 0 << 400
			 << (bool)true << (bool)true << (bool)false
			 << (int)QSplitter::KeepSize << (int)QSplitter::Auto;
    QTest::newRow("ok146") << 200 << 200 << 350 << 0 << 400 << 0
			 << (bool)true << (bool)true << (bool)false
			 << (int)QSplitter::KeepSize << (int)QSplitter::Auto;
    QTest::newRow("ok147") << 200 << 200 << 350 << 0 << 200 << 200
			 << (bool)true << (bool)false << (bool)false
			 << (int)QSplitter::KeepSize << (int)QSplitter::Auto;
    QTest::newRow("ok148") << 200 << 200 << 350 << 0 << 200 << 200
			 << (bool)false << (bool)false << (bool)false
			 << (int)QSplitter::KeepSize << (int)QSplitter::Auto;
    QTest::newRow("ok149") << 200 << 200 << 0 << 350 << 200 << 200
			 << (bool)false << (bool)true << (bool)false
			 << (int)QSplitter::KeepSize << (int)QSplitter::Auto;
    QTest::newRow("ok150") << 200 << 200 << 0 << 350 << 200 << 200
			 << (bool)false << (bool)false << (bool)false
			 << (int)QSplitter::KeepSize << (int)QSplitter::Auto;
    QTest::newRow("ok160") << 100 << 50 << 100 << 300 << 100 << 300
			 << (bool)true << (bool)true << (bool)true
			 << (int)QSplitter::Auto << (int)QSplitter::KeepSize;
    QTest::newRow("ok161") << 100 << 100 << 50 << 350 << 100 << 300
			 << (bool)true << (bool)true << (bool)true
			 << (int)QSplitter::Auto << (int)QSplitter::KeepSize;
    QTest::newRow("ok162") << 100 << 100 << 350 << 50 << 300 << 100
			 << (bool)true << (bool)true << (bool)true
			 << (int)QSplitter::Auto << (int)QSplitter::KeepSize;
    QTest::newRow("ok163") << 200 << 200 << 350 << 50 << 200 << 200
			 << (bool)true << (bool)true << (bool)true
			 << (int)QSplitter::Auto << (int)QSplitter::KeepSize;
    QTest::newRow("ok164") << 200 << 200 << 200 << 200 << 200 << 200
			 << (bool)true << (bool)true << (bool)true
			 << (int)QSplitter::Auto << (int)QSplitter::KeepSize;
    QTest::newRow("ok165") << 200 << 200 << 0 << 350 << 0 << 400
			 << (bool)true << (bool)true << (bool)true
			 << (int)QSplitter::Auto << (int)QSplitter::KeepSize;
    QTest::newRow("ok166") << 200 << 200 << 350 << 0 << 400 << 0
			 << (bool)true << (bool)true << (bool)true
			 << (int)QSplitter::Auto << (int)QSplitter::KeepSize;
    QTest::newRow("ok167") << 200 << 200 << 350 << 0 << 200 << 200
			 << (bool)true << (bool)false << (bool)true
			 << (int)QSplitter::Auto << (int)QSplitter::KeepSize;
    QTest::newRow("ok168") << 200 << 200 << 350 << 0 << 200 << 200
			 << (bool)false << (bool)false << (bool)true
			 << (int)QSplitter::Auto << (int)QSplitter::KeepSize;
    QTest::newRow("ok169") << 200 << 200 << 0 << 350 << 200 << 200
			 << (bool)false << (bool)true << (bool)true
			 << (int)QSplitter::Auto << (int)QSplitter::KeepSize;
    QTest::newRow("ok170") << 200 << 200 << 0 << 350 << 200 << 200
			 << (bool)false << (bool)false << (bool)true
			 << (int)QSplitter::Auto << (int)QSplitter::KeepSize;
    QTest::newRow("ok180") << 100 << 50 << 100 << 300 << 100 << 300
			 << (bool)true << (bool)true << (bool)false
			 << (int)QSplitter::Auto << (int)QSplitter::KeepSize;
    QTest::newRow("ok181") << 100 << 100 << 50 << 350 << 100 << 300
			 << (bool)true << (bool)true << (bool)false
			 << (int)QSplitter::Auto << (int)QSplitter::KeepSize;
    QTest::newRow("ok182") << 100 << 100 << 350 << 50 << 300 << 100
			 << (bool)true << (bool)true << (bool)false
			 << (int)QSplitter::Auto << (int)QSplitter::KeepSize;
    QTest::newRow("ok183") << 200 << 200 << 350 << 50 << 200 << 200
			 << (bool)true << (bool)true << (bool)false
			 << (int)QSplitter::Auto << (int)QSplitter::KeepSize;
    QTest::newRow("ok184") << 200 << 200 << 200 << 200 << 200 << 200
			 << (bool)true << (bool)true << (bool)false
			 << (int)QSplitter::Auto << (int)QSplitter::KeepSize;
    QTest::newRow("ok185") << 200 << 200 << 0 << 350 << 0 << 400
			 << (bool)true << (bool)true << (bool)false
			 << (int)QSplitter::Auto << (int)QSplitter::KeepSize;
    QTest::newRow("ok186") << 200 << 200 << 350 << 0 << 400 << 0
			 << (bool)true << (bool)true << (bool)false
			 << (int)QSplitter::Auto << (int)QSplitter::KeepSize;
    QTest::newRow("ok187") << 200 << 200 << 350 << 0 << 200 << 200
			 << (bool)true << (bool)false << (bool)false
			 << (int)QSplitter::Auto << (int)QSplitter::KeepSize;
    QTest::newRow("ok188") << 200 << 200 << 350 << 0 << 200 << 200
			 << (bool)false << (bool)false << (bool)false
			 << (int)QSplitter::Auto << (int)QSplitter::KeepSize;
    QTest::newRow("ok189") << 200 << 200 << 0 << 350 << 200 << 200
			 << (bool)false << (bool)true << (bool)false
			 << (int)QSplitter::Auto << (int)QSplitter::KeepSize;
    QTest::newRow("ok190") << 200 << 200 << 0 << 350 << 200 << 200
			 << (bool)false << (bool)false << (bool)false
			 << (int)QSplitter::Auto << (int)QSplitter::KeepSize;
    QTest::newRow("ok200") << 100 << 50 << 100 << 300 << 100 << 300
			 << (bool)true << (bool)true << (bool)true
			 << (int)QSplitter::Stretch << (int)QSplitter::Stretch;
    QTest::newRow("ok201") << 100 << 100 << 50 << 350 << 100 << 300
			 << (bool)true << (bool)true << (bool)true
			 << (int)QSplitter::Stretch << (int)QSplitter::Stretch;
    QTest::newRow("ok202") << 100 << 100 << 350 << 50 << 300 << 100
			 << (bool)true << (bool)true << (bool)true
			 << (int)QSplitter::Stretch << (int)QSplitter::Stretch;
    QTest::newRow("ok203") << 200 << 200 << 350 << 50 << 200 << 200
			 << (bool)true << (bool)true << (bool)true
			 << (int)QSplitter::Stretch << (int)QSplitter::Stretch;
    QTest::newRow("ok204") << 200 << 200 << 200 << 200 << 200 << 200
			 << (bool)true << (bool)true << (bool)true
			 << (int)QSplitter::Stretch << (int)QSplitter::Stretch;
    QTest::newRow("ok205") << 200 << 200 << 0 << 350 << 0 << 400
			 << (bool)true << (bool)true << (bool)true
			 << (int)QSplitter::Stretch << (int)QSplitter::Stretch;
    QTest::newRow("ok206") << 200 << 200 << 350 << 0 << 400 << 0
			 << (bool)true << (bool)true << (bool)true
			 << (int)QSplitter::Stretch << (int)QSplitter::Stretch;
    QTest::newRow("ok207") << 200 << 200 << 350 << 0 << 200 << 200
			 << (bool)true << (bool)false << (bool)true
			 << (int)QSplitter::Stretch << (int)QSplitter::Stretch;
    QTest::newRow("ok208") << 200 << 200 << 350 << 0 << 200 << 200
			 << (bool)false << (bool)false << (bool)true
			 << (int)QSplitter::Stretch << (int)QSplitter::Stretch;
    QTest::newRow("ok209") << 200 << 200 << 0 << 350 << 200 << 200
			 << (bool)false << (bool)true << (bool)true
			 << (int)QSplitter::Stretch << (int)QSplitter::Stretch;
    QTest::newRow("ok210") << 200 << 200 << 0 << 350 << 200 << 200
			 << (bool)false << (bool)false << (bool)true
			 << (int)QSplitter::Stretch << (int)QSplitter::Stretch;
    QTest::newRow("ok220") << 100 << 50 << 100 << 300 << 100 << 300
			 << (bool)true << (bool)true << (bool)false
			 << (int)QSplitter::Stretch << (int)QSplitter::Stretch;
    QTest::newRow("ok221") << 100 << 100 << 50 << 350 << 100 << 300
			 << (bool)true << (bool)true << (bool)false
			 << (int)QSplitter::Stretch << (int)QSplitter::Stretch;
    QTest::newRow("ok222") << 100 << 100 << 350 << 50 << 300 << 100
			 << (bool)true << (bool)true << (bool)false
			 << (int)QSplitter::Stretch << (int)QSplitter::Stretch;
    QTest::newRow("ok223") << 200 << 200 << 350 << 50 << 200 << 200
			 << (bool)true << (bool)true << (bool)false
			 << (int)QSplitter::Stretch << (int)QSplitter::Stretch;
    QTest::newRow("ok224") << 200 << 200 << 200 << 200 << 200 << 200
			 << (bool)true << (bool)true << (bool)false
			 << (int)QSplitter::Stretch << (int)QSplitter::Stretch;
    QTest::newRow("ok225") << 200 << 200 << 0 << 350 << 0 << 400
			 << (bool)true << (bool)true << (bool)false
			 << (int)QSplitter::Stretch << (int)QSplitter::Stretch;
    QTest::newRow("ok226") << 200 << 200 << 350 << 0 << 400 << 0
			 << (bool)true << (bool)true << (bool)false
			 << (int)QSplitter::Stretch << (int)QSplitter::Stretch;
    QTest::newRow("ok227") << 200 << 200 << 350 << 0 << 200 << 200
			 << (bool)true << (bool)false << (bool)false
			 << (int)QSplitter::Stretch << (int)QSplitter::Stretch;
    QTest::newRow("ok228") << 200 << 200 << 350 << 0 << 200 << 200
			 << (bool)false << (bool)false << (bool)false
			 << (int)QSplitter::Stretch << (int)QSplitter::Stretch;
    QTest::newRow("ok229") << 200 << 200 << 0 << 350 << 200 << 200
			 << (bool)false << (bool)true << (bool)false
			 << (int)QSplitter::Stretch << (int)QSplitter::Stretch;
    QTest::newRow("ok230") << 200 << 200 << 0 << 350 << 200 << 200
			 << (bool)false << (bool)false << (bool)false
			 << (int)QSplitter::Stretch << (int)QSplitter::Stretch;
    QTest::newRow("ok240") << 100 << 50 << 100 << 300 << 100 << 300
			 << (bool)true << (bool)true << (bool)true
			 << (int)QSplitter::Stretch << (int)QSplitter::KeepSize;
    QTest::newRow("ok241") << 100 << 100 << 50 << 350 << 100 << 300
			 << (bool)true << (bool)true << (bool)true
			 << (int)QSplitter::Stretch << (int)QSplitter::KeepSize;
    QTest::newRow("ok242") << 100 << 100 << 350 << 50 << 300 << 100
			 << (bool)true << (bool)true << (bool)true
			 << (int)QSplitter::Stretch << (int)QSplitter::KeepSize;
    QTest::newRow("ok243") << 200 << 200 << 350 << 50 << 200 << 200
			 << (bool)true << (bool)true << (bool)true
			 << (int)QSplitter::Stretch << (int)QSplitter::KeepSize;
    QTest::newRow("ok244") << 200 << 200 << 200 << 200 << 200 << 200
			 << (bool)true << (bool)true << (bool)true
			 << (int)QSplitter::Stretch << (int)QSplitter::KeepSize;
    QTest::newRow("ok245") << 200 << 200 << 0 << 350 << 0 << 400
			 << (bool)true << (bool)true << (bool)true
			 << (int)QSplitter::Stretch << (int)QSplitter::KeepSize;
    QTest::newRow("ok246") << 200 << 200 << 350 << 0 << 400 << 0
			 << (bool)true << (bool)true << (bool)true
			 << (int)QSplitter::Stretch << (int)QSplitter::KeepSize;
    QTest::newRow("ok247") << 200 << 200 << 350 << 0 << 200 << 200
			 << (bool)true << (bool)false << (bool)true
			 << (int)QSplitter::Stretch << (int)QSplitter::KeepSize;
    QTest::newRow("ok248") << 200 << 200 << 350 << 0 << 200 << 200
			 << (bool)false << (bool)false << (bool)true
			 << (int)QSplitter::Stretch << (int)QSplitter::KeepSize;
    QTest::newRow("ok249") << 200 << 200 << 0 << 350 << 200 << 200
			 << (bool)false << (bool)true << (bool)true
			 << (int)QSplitter::Stretch << (int)QSplitter::KeepSize;
    QTest::newRow("ok250") << 200 << 200 << 0 << 350 << 200 << 200
			 << (bool)false << (bool)false << (bool)true
			 << (int)QSplitter::Stretch << (int)QSplitter::KeepSize;
    QTest::newRow("ok260") << 100 << 50 << 100 << 300 << 100 << 300
			 << (bool)true << (bool)true << (bool)false
			 << (int)QSplitter::Stretch << (int)QSplitter::KeepSize;
    QTest::newRow("ok261") << 100 << 100 << 50 << 350 << 100 << 300
			 << (bool)true << (bool)true << (bool)false
			 << (int)QSplitter::Stretch << (int)QSplitter::KeepSize;
    QTest::newRow("ok262") << 100 << 100 << 350 << 50 << 300 << 100
			 << (bool)true << (bool)true << (bool)false
			 << (int)QSplitter::Stretch << (int)QSplitter::KeepSize;
    QTest::newRow("ok263") << 200 << 200 << 350 << 50 << 200 << 200
			 << (bool)true << (bool)true << (bool)false
			 << (int)QSplitter::Stretch << (int)QSplitter::KeepSize;
    QTest::newRow("ok264") << 200 << 200 << 200 << 200 << 200 << 200
			 << (bool)true << (bool)true << (bool)false
			 << (int)QSplitter::Stretch << (int)QSplitter::KeepSize;
    QTest::newRow("ok265") << 200 << 200 << 0 << 350 << 0 << 400
			 << (bool)true << (bool)true << (bool)false
			 << (int)QSplitter::Stretch << (int)QSplitter::KeepSize;
    QTest::newRow("ok266") << 200 << 200 << 350 << 0 << 400 << 0
			 << (bool)true << (bool)true << (bool)false
			 << (int)QSplitter::Stretch << (int)QSplitter::KeepSize;
    QTest::newRow("ok267") << 200 << 200 << 350 << 0 << 200 << 200
			 << (bool)true << (bool)false << (bool)false
			 << (int)QSplitter::Stretch << (int)QSplitter::KeepSize;
    QTest::newRow("ok268") << 200 << 200 << 350 << 0 << 200 << 200
			 << (bool)false << (bool)false << (bool)false
			 << (int)QSplitter::Stretch << (int)QSplitter::KeepSize;
    QTest::newRow("ok269") << 200 << 200 << 0 << 350 << 200 << 200
			 << (bool)false << (bool)true << (bool)false
			 << (int)QSplitter::Stretch << (int)QSplitter::KeepSize;
    QTest::newRow("ok270") << 200 << 200 << 0 << 350 << 200 << 200
			 << (bool)false << (bool)false << (bool)false
			 << (int)QSplitter::Stretch << (int)QSplitter::KeepSize;
    QTest::newRow("ok280") << 100 << 50 << 100 << 300 << 100 << 300
			 << (bool)true << (bool)true << (bool)true
			 << (int)QSplitter::KeepSize << (int)QSplitter::Stretch;
    QTest::newRow("ok281") << 100 << 100 << 50 << 350 << 100 << 300
			 << (bool)true << (bool)true << (bool)true
			 << (int)QSplitter::KeepSize << (int)QSplitter::Stretch;
    QTest::newRow("ok282") << 100 << 100 << 350 << 50 << 300 << 100
			 << (bool)true << (bool)true << (bool)true
			 << (int)QSplitter::KeepSize << (int)QSplitter::Stretch;
    QTest::newRow("ok283") << 200 << 200 << 350 << 50 << 200 << 200
			 << (bool)true << (bool)true << (bool)true
			 << (int)QSplitter::KeepSize << (int)QSplitter::Stretch;
    QTest::newRow("ok284") << 200 << 200 << 200 << 200 << 200 << 200
			 << (bool)true << (bool)true << (bool)true
			 << (int)QSplitter::KeepSize << (int)QSplitter::Stretch;
    QTest::newRow("ok285") << 200 << 200 << 0 << 350 << 0 << 400
			 << (bool)true << (bool)true << (bool)true
			 << (int)QSplitter::KeepSize << (int)QSplitter::Stretch;
    QTest::newRow("ok286") << 200 << 200 << 350 << 0 << 400 << 0
			 << (bool)true << (bool)true << (bool)true
			 << (int)QSplitter::KeepSize << (int)QSplitter::Stretch;
    QTest::newRow("ok287") << 200 << 200 << 350 << 0 << 200 << 200
			 << (bool)true << (bool)false << (bool)true
			 << (int)QSplitter::KeepSize << (int)QSplitter::Stretch;
    QTest::newRow("ok288") << 200 << 200 << 350 << 0 << 200 << 200
			 << (bool)false << (bool)false << (bool)true
			 << (int)QSplitter::KeepSize << (int)QSplitter::Stretch;
    QTest::newRow("ok289") << 200 << 200 << 0 << 350 << 200 << 200
			 << (bool)false << (bool)true << (bool)true
			 << (int)QSplitter::KeepSize << (int)QSplitter::Stretch;
    QTest::newRow("ok290") << 200 << 200 << 0 << 350 << 200 << 200
			 << (bool)false << (bool)false << (bool)true
			 << (int)QSplitter::KeepSize << (int)QSplitter::Stretch;
    QTest::newRow("ok300") << 100 << 50 << 100 << 300 << 100 << 300
			 << (bool)true << (bool)true << (bool)false
			 << (int)QSplitter::KeepSize << (int)QSplitter::Stretch;
    QTest::newRow("ok301") << 100 << 100 << 50 << 350 << 100 << 300
			 << (bool)true << (bool)true << (bool)false
			 << (int)QSplitter::KeepSize << (int)QSplitter::Stretch;
    QTest::newRow("ok302") << 100 << 100 << 350 << 50 << 300 << 100
			 << (bool)true << (bool)true << (bool)false
			 << (int)QSplitter::KeepSize << (int)QSplitter::Stretch;
    QTest::newRow("ok303") << 200 << 200 << 350 << 50 << 200 << 200
			 << (bool)true << (bool)true << (bool)false
			 << (int)QSplitter::KeepSize << (int)QSplitter::Stretch;
    QTest::newRow("ok304") << 200 << 200 << 200 << 200 << 200 << 200
			 << (bool)true << (bool)true << (bool)false
			 << (int)QSplitter::KeepSize << (int)QSplitter::Stretch;
    QTest::newRow("ok305") << 200 << 200 << 0 << 350 << 0 << 400
			 << (bool)true << (bool)true << (bool)false
			 << (int)QSplitter::KeepSize << (int)QSplitter::Stretch;
    QTest::newRow("ok306") << 200 << 200 << 350 << 0 << 400 << 0
			 << (bool)true << (bool)true << (bool)false
			 << (int)QSplitter::KeepSize << (int)QSplitter::Stretch;
    QTest::newRow("ok307") << 200 << 200 << 350 << 0 << 200 << 200
			 << (bool)true << (bool)false << (bool)false
			 << (int)QSplitter::KeepSize << (int)QSplitter::Stretch;
    QTest::newRow("ok308") << 200 << 200 << 350 << 0 << 200 << 200
			 << (bool)false << (bool)false << (bool)false
			 << (int)QSplitter::KeepSize << (int)QSplitter::Stretch;
    QTest::newRow("ok309") << 200 << 200 << 0 << 350 << 200 << 200
			 << (bool)false << (bool)true << (bool)false
			 << (int)QSplitter::KeepSize << (int)QSplitter::Stretch;
    QTest::newRow("ok310") << 200 << 200 << 0 << 350 << 200 << 200
			 << (bool)false << (bool)false << (bool)false
			 << (int)QSplitter::KeepSize << (int)QSplitter::Stretch;
    QTest::newRow("ok320") << 100 << 50 << 100 << 300 << 100 << 300
			 << (bool)true << (bool)true << (bool)true
			 << (int)QSplitter::KeepSize << (int)QSplitter::KeepSize;
    QTest::newRow("ok321") << 100 << 100 << 50 << 350 << 100 << 300
			 << (bool)true << (bool)true << (bool)true
			 << (int)QSplitter::KeepSize << (int)QSplitter::KeepSize;
    QTest::newRow("ok322") << 100 << 100 << 350 << 50 << 300 << 100
			 << (bool)true << (bool)true << (bool)true
			 << (int)QSplitter::KeepSize << (int)QSplitter::KeepSize;
    QTest::newRow("ok323") << 200 << 200 << 350 << 50 << 200 << 200
			 << (bool)true << (bool)true << (bool)true
			 << (int)QSplitter::KeepSize << (int)QSplitter::KeepSize;
    QTest::newRow("ok324") << 200 << 200 << 200 << 200 << 200 << 200
			 << (bool)true << (bool)true << (bool)true
			 << (int)QSplitter::KeepSize << (int)QSplitter::KeepSize;
    QTest::newRow("ok325") << 200 << 200 << 0 << 350 << 0 << 400
			 << (bool)true << (bool)true << (bool)true
			 << (int)QSplitter::KeepSize << (int)QSplitter::KeepSize;
    QTest::newRow("ok326") << 200 << 200 << 350 << 0 << 400 << 0
			 << (bool)true << (bool)true << (bool)true
			 << (int)QSplitter::KeepSize << (int)QSplitter::KeepSize;
    QTest::newRow("ok327") << 200 << 200 << 350 << 0 << 200 << 200
			 << (bool)true << (bool)false << (bool)true
			 << (int)QSplitter::KeepSize << (int)QSplitter::KeepSize;
    QTest::newRow("ok328") << 200 << 200 << 350 << 0 << 200 << 200
			 << (bool)false << (bool)false << (bool)true
			 << (int)QSplitter::KeepSize << (int)QSplitter::KeepSize;
    QTest::newRow("ok329") << 200 << 200 << 0 << 350 << 200 << 200
			 << (bool)false << (bool)true << (bool)true
			 << (int)QSplitter::KeepSize << (int)QSplitter::KeepSize;
    QTest::newRow("ok330") << 200 << 200 << 0 << 350 << 200 << 200
			 << (bool)false << (bool)false << (bool)true
			 << (int)QSplitter::KeepSize << (int)QSplitter::KeepSize;
    QTest::newRow("ok340") << 100 << 50 << 100 << 300 << 100 << 300
			 << (bool)true << (bool)true << (bool)false
			 << (int)QSplitter::KeepSize << (int)QSplitter::KeepSize;
    QTest::newRow("ok341") << 100 << 100 << 50 << 350 << 100 << 300
			 << (bool)true << (bool)true << (bool)false
			 << (int)QSplitter::KeepSize << (int)QSplitter::KeepSize;
    QTest::newRow("ok342") << 100 << 100 << 350 << 50 << 300 << 100
			 << (bool)true << (bool)true << (bool)false
			 << (int)QSplitter::KeepSize << (int)QSplitter::KeepSize;
    QTest::newRow("ok343") << 200 << 200 << 350 << 50 << 200 << 200
			 << (bool)true << (bool)true << (bool)false
			 << (int)QSplitter::KeepSize << (int)QSplitter::KeepSize;
    QTest::newRow("ok344") << 200 << 200 << 200 << 200 << 200 << 200
			 << (bool)true << (bool)true << (bool)false
			 << (int)QSplitter::KeepSize << (int)QSplitter::KeepSize;
    QTest::newRow("ok345") << 200 << 200 << 0 << 350 << 0 << 400
			 << (bool)true << (bool)true << (bool)false
			 << (int)QSplitter::KeepSize << (int)QSplitter::KeepSize;
    QTest::newRow("ok346") << 200 << 200 << 350 << 0 << 400 << 0
			 << (bool)true << (bool)true << (bool)false
			 << (int)QSplitter::KeepSize << (int)QSplitter::KeepSize;
    QTest::newRow("ok347") << 200 << 200 << 350 << 0 << 200 << 200
			 << (bool)true << (bool)false << (bool)false
			 << (int)QSplitter::KeepSize << (int)QSplitter::KeepSize;
    QTest::newRow("ok348") << 200 << 200 << 350 << 0 << 200 << 200
			 << (bool)false << (bool)false << (bool)false
			 << (int)QSplitter::KeepSize << (int)QSplitter::KeepSize;
    QTest::newRow("ok349") << 200 << 200 << 0 << 350 << 200 << 200
			 << (bool)false << (bool)true << (bool)false
			 << (int)QSplitter::KeepSize << (int)QSplitter::KeepSize;
    QTest::newRow("ok350") << 200 << 200 << 0 << 350 << 200 << 200
			 << (bool)false << (bool)false << (bool)false
			 << (int)QSplitter::KeepSize << (int)QSplitter::KeepSize;
#endif
}

void tst_QSplitter::saveAndRestoreState_data()
{
    saveState_data();
}

void tst_QSplitter::saveAndRestoreState()
{
    QFETCH(IntList, initialSizes);
    splitter->setSizes(initialSizes);
    QApplication::instance()->sendPostedEvents();
    
    QSplitter *splitter2 = new QSplitter(splitter->orientation() == Qt::Horizontal ?
                                         Qt::Vertical : Qt::Horizontal);
    for (int i = 0; i < splitter->count(); ++i) {
        splitter2->addWidget(new QWidget());        
    }
    splitter2->resize(splitter->size());    
    splitter2->setChildrenCollapsible(!splitter->childrenCollapsible());
    splitter2->setOpaqueResize(!splitter->opaqueResize());
    splitter2->setHandleWidth(splitter->handleWidth()+3);
    
    QByteArray ba = splitter->saveState();
    QVERIFY(splitter2->restoreState(ba));

    QCOMPARE(splitter2->orientation(), splitter->orientation());
    QCOMPARE(splitter2->handleWidth(), splitter->handleWidth());
    QCOMPARE(splitter2->opaqueResize(), splitter->opaqueResize());
    QCOMPARE(splitter2->childrenCollapsible(), splitter->childrenCollapsible());

    QList<int> l1 = splitter->sizes();
    QList<int> l2 = splitter2->sizes();
    QCOMPARE(l1.size(), l2.size());
    for (int i = 0; i < splitter->sizes().size(); ++i) {
        QCOMPARE(l2.at(i), l1.at(i));
    }

    // destroy version and magic number
    for (int i = 0; i < ba.size(); ++i) 
        ba[i] = ~ba.at(i);
    QVERIFY(!splitter2->restoreState(ba));
    
    delete splitter2;
}

void tst_QSplitter::saveAndRestoreStateOfNotYetShownSplitter()
{
    QSplitter *spl = new QSplitter;
    QLabel *l1 = new QLabel;
    QLabel *l2 = new QLabel;
    spl->addWidget(l1);
    spl->addWidget(l2);

    QByteArray ba = spl->saveState();
    spl->restoreState(ba);
    spl->show();
    QTest::qWait(500);

    QCOMPARE(l1->geometry().isValid(), true);
    QCOMPARE(l2->geometry().isValid(), true);

    delete spl;
}

void tst_QSplitter::saveState_data()
{
    QTest::addColumn<IntList>("initialSizes");
    QTest::addColumn<bool>("hideWidget1");
    QTest::addColumn<bool>("hideWidget2");
    QTest::addColumn<QByteArray>("finalBa");

    QTest::newRow("ok0") << (IntList() << 200 << 200) << bool(false) << bool(false) << QByteArray("[200,200]");
    QTest::newRow("ok1") << (IntList() << 300 << 100) << bool(false) << bool(false) << QByteArray("[300,100]");
    QTest::newRow("ok2") << (IntList() << 100 << 300) << bool(false) << bool(false) << QByteArray("[100,300]");
    QTest::newRow("ok3") << (IntList() << 200 << 200) << bool(false) << bool(true) << QByteArray("[200,H]");
    QTest::newRow("ok4") << (IntList() << 200 << 200) << bool(true) << bool(false) << QByteArray("[H,200]");
    QTest::newRow("ok5") << (IntList() << 200 << 200) << bool(false) << bool(false) << QByteArray("[200,200]");
    QTest::newRow("ok6") << (IntList() << 200 << 200) << bool(false) << bool(false) << QByteArray("[200,200]");
    QTest::newRow("ok7") << (IntList() << 200 << 200) << bool(false) << bool(false) << QByteArray("[200,200]");
    QTest::newRow("ok8") << (IntList() << 200 << 200) << bool(true) << bool(true) << QByteArray("[H,H]");
}

void tst_QSplitter::addWidget()
{
    QSplitter split;

    // Simple case
    QWidget *widget1 = new QWidget;
    QWidget *widget2 = new QWidget;
    split.addWidget(widget1);
    split.addWidget(widget2);
    QCOMPARE(split.count(), 2);
    QCOMPARE(split.indexOf(widget1), 0);
    QCOMPARE(split.indexOf(widget2), 1);
    QCOMPARE(split.widget(0), widget1);
    QCOMPARE(split.widget(1), widget2);


    // Implicit Add
    QWidget *widget3 = new QWidget(&split);
    QCOMPARE(split.count(), 3);
    QCOMPARE(split.indexOf(widget3), 2);
    QCOMPARE(split.widget(2), widget3);

    // Try and add it again
    split.addWidget(widget3);
    QCOMPARE(split.count(), 3);
    QCOMPARE(split.indexOf(widget3), 2);
    QCOMPARE(split.widget(2), widget3);

    // Add a widget that is already in the splitter
    split.addWidget(widget1);
    QCOMPARE(split.count(), 3);
    QCOMPARE(split.indexOf(widget1), 2);
    QCOMPARE(split.widget(0), widget2);
    QCOMPARE(split.widget(1), widget3);
    QCOMPARE(split.widget(2), widget1);

    // Change a widget's parent
    widget2->setParent(0);
    QCOMPARE(split.count(), 2);
    QCOMPARE(split.indexOf(widget2), -1);


    // Add the widget in again.
    split.addWidget(widget2);
    QCOMPARE(split.count(), 3);
    QCOMPARE(split.indexOf(widget2), 2);
    QCOMPARE(split.widget(0), widget3);
    QCOMPARE(split.widget(1), widget1);
    QCOMPARE(split.widget(2), widget2);

    // Delete a widget
    delete widget1;
    QCOMPARE(split.count(), 2);
    QCOMPARE(split.indexOf(widget1), -1); // Nasty
    QCOMPARE(split.widget(0), widget3);
    QCOMPARE(split.widget(1), widget2);

    delete widget2;
}

void tst_QSplitter::insertWidget()
{
    QSplitter split;
    QWidget *widget1 = new QWidget;
    QWidget *widget2 = new QWidget;
    QWidget *widget3 = new QWidget;

    split.insertWidget(0, widget1);
    QCOMPARE(split.count(), 1);
    QCOMPARE(split.indexOf(widget1), 0);
    QCOMPARE(split.widget(0), widget1);

    split.insertWidget(0, widget2);
    QCOMPARE(split.count(), 2);
    QCOMPARE(split.indexOf(widget1), 1);
    QCOMPARE(split.indexOf(widget2), 0);
    QCOMPARE(split.widget(0), widget2);
    QCOMPARE(split.widget(1), widget1);

    split.insertWidget(1, widget3);
    QCOMPARE(split.count(), 3);
    QCOMPARE(split.indexOf(widget1), 2);
    QCOMPARE(split.indexOf(widget2), 0);
    QCOMPARE(split.indexOf(widget3), 1);
    QCOMPARE(split.widget(0), widget2);
    QCOMPARE(split.widget(1), widget3);
    QCOMPARE(split.widget(2), widget1);

    delete widget3;
    QCOMPARE(split.count(), 2);
    QCOMPARE(split.indexOf(widget1), 1);
    QCOMPARE(split.indexOf(widget2), 0);
    QCOMPARE(split.widget(0), widget2);
    QCOMPARE(split.widget(1), widget1);

    widget3 = new QWidget;
    split.insertWidget(split.count() + 1, widget3);
    QCOMPARE(split.count(), 3);
    QCOMPARE(split.indexOf(widget1), 1);
    QCOMPARE(split.indexOf(widget2), 0);
    QCOMPARE(split.indexOf(widget3), 2);
    QCOMPARE(split.widget(0), widget2);
    QCOMPARE(split.widget(1), widget1);
    QCOMPARE(split.widget(2), widget3);


    // Try it again,
    split.insertWidget(split.count() + 1, widget3);
    QCOMPARE(split.count(), 3);
    QCOMPARE(split.indexOf(widget1), 1);
    QCOMPARE(split.indexOf(widget2), 0);
    QCOMPARE(split.indexOf(widget3), 2);
    QCOMPARE(split.widget(0), widget2);
    QCOMPARE(split.widget(1), widget1);
    QCOMPARE(split.widget(2), widget3);

    // Try to move widget2 to a bad place
    split.insertWidget(-1, widget2);
    QCOMPARE(split.count(), 3);
    QCOMPARE(split.indexOf(widget1), 0);
    QCOMPARE(split.indexOf(widget2), 2);
    QCOMPARE(split.indexOf(widget3), 1);
    QCOMPARE(split.widget(0), widget1);
    QCOMPARE(split.widget(1), widget3);
    QCOMPARE(split.widget(2), widget2);

    QWidget *widget4 = new QWidget(&split);
    QCOMPARE(split.count(), 4);
    QCOMPARE(split.indexOf(widget1), 0);
    QCOMPARE(split.indexOf(widget2), 2);
    QCOMPARE(split.indexOf(widget3), 1);
    QCOMPARE(split.indexOf(widget4), 3);
    QCOMPARE(split.widget(0), widget1);
    QCOMPARE(split.widget(1), widget3);
    QCOMPARE(split.widget(2), widget2);
    QCOMPARE(split.widget(3), widget4);

    QWidget *widget5 = new QWidget(&split);
    QCOMPARE(split.count(), 5);
    QCOMPARE(split.indexOf(widget1), 0);
    QCOMPARE(split.indexOf(widget2), 2);
    QCOMPARE(split.indexOf(widget3), 1);
    QCOMPARE(split.indexOf(widget4), 3);
    QCOMPARE(split.indexOf(widget5), 4);
    QCOMPARE(split.widget(0), widget1);
    QCOMPARE(split.widget(1), widget3);
    QCOMPARE(split.widget(2), widget2);
    QCOMPARE(split.widget(3), widget4);
    QCOMPARE(split.widget(4), widget5);

    split.insertWidget(2, widget4);
    QCOMPARE(split.count(), 5);
    QCOMPARE(split.indexOf(widget1), 0);
    QCOMPARE(split.indexOf(widget2), 3);
    QCOMPARE(split.indexOf(widget3), 1);
    QCOMPARE(split.indexOf(widget4), 2);
    QCOMPARE(split.indexOf(widget5), 4);
    QCOMPARE(split.widget(0), widget1);
    QCOMPARE(split.widget(1), widget3);
    QCOMPARE(split.widget(2), widget4);
    QCOMPARE(split.widget(3), widget2);
    QCOMPARE(split.widget(4), widget5);

    split.insertWidget(1, widget5);
    QCOMPARE(split.count(), 5);
    QCOMPARE(split.indexOf(widget1), 0);
    QCOMPARE(split.indexOf(widget2), 4);
    QCOMPARE(split.indexOf(widget3), 2);
    QCOMPARE(split.indexOf(widget4), 3);
    QCOMPARE(split.indexOf(widget5), 1);
    QCOMPARE(split.widget(0), widget1);
    QCOMPARE(split.widget(1), widget5);
    QCOMPARE(split.widget(2), widget3);
    QCOMPARE(split.widget(3), widget4);
    QCOMPARE(split.widget(4), widget2);
}

void tst_QSplitter::setStretchFactor_data()
{
    QTest::addColumn<int>("orientation");
    QTest::addColumn<int>("widgetIndex");
    QTest::addColumn<int>("stretchFactor");
    QTest::addColumn<int>("expectedHStretch");
    QTest::addColumn<int>("expectedVStretch");

    QTest::newRow("ok01") << int(Qt::Horizontal) << 1 << 2 << 2 << 2;
    QTest::newRow("ok02") << int(Qt::Horizontal) << 2 << 0 << 0 << 0;
    QTest::newRow("ok03") << int(Qt::Horizontal) << 3 << 1 << 1 << 1;
    QTest::newRow("ok04") << int(Qt::Horizontal) << 0 << 7 << 7 << 7;
    QTest::newRow("ok05") << int(Qt::Vertical) << 0 << 0 << 0 << 0;
    QTest::newRow("ok06") << int(Qt::Vertical) << 1 << 1 << 1 << 1;
    QTest::newRow("ok07") << int(Qt::Vertical) << 2 << 2 << 2 << 2;
    QTest::newRow("ok08") << int(Qt::Vertical) << 3 << 5 << 5 << 5;
    QTest::newRow("ok08") << int(Qt::Vertical) << -1 << 5 << 0 << 0;
}

void tst_QSplitter::setStretchFactor()
{
    QFETCH(int, orientation);
    Qt::Orientation orient = Qt::Orientation(orientation);
    QSplitter split(orient);
    QWidget *w = new QWidget;
    split.addWidget(w);
    w = new QWidget;
    split.addWidget(w);
    w = new QWidget;
    split.addWidget(w);
    w = new QWidget;
    split.addWidget(w);

    QFETCH(int, widgetIndex);
    QFETCH(int, stretchFactor);
    w = split.widget(widgetIndex);
    QSizePolicy sp;
    if (w) {
        QCOMPARE(sp.horizontalStretch(), 0);
        QCOMPARE(sp.verticalStretch(), 0);
    }
    split.setStretchFactor(widgetIndex, stretchFactor);
    if (w)
        sp = w->sizePolicy();
    QTEST(sp.horizontalStretch(), "expectedHStretch");
    QTEST(sp.verticalStretch(), "expectedVStretch");
}

void tst_QSplitter::testShowHide_data()
{
    QTest::addColumn<bool>("hideWidget1");
    QTest::addColumn<bool>("hideWidget2");
    QTest::addColumn<QList<int> >("finalValues");
    QTest::addColumn<bool>("handleVisible");

    QSplitter *split = new QSplitter(Qt::Horizontal);
    QTest::newRow("hideNone") << false << false << (QList<int>() << 200 << 200) << true;
    QTest::newRow("hide2") << false << true << (QList<int>() << 400 + split->handleWidth() << 0) << false;
    QTest::newRow("hide1") << true << false << (QList<int>() << 0 << 400 + split->handleWidth()) << false;
    QTest::newRow("hideall") << true << true << (QList<int>() << 0 << 0) << false;
    delete split;
}

void tst_QSplitter::testShowHide()
{
    QFETCH(bool, hideWidget1);
    QFETCH(bool, hideWidget2);

    QSplitter *split = new QSplitter(Qt::Horizontal);

    QWidget topLevel;
    QWidget widget(&topLevel);
    widget.resize(400 + split->handleWidth(), 200);
    QVBoxLayout *lay=new QVBoxLayout(&widget);
    lay->setMargin(0);
    lay->setSpacing(0);
    split->addWidget(new QWidget);
    split->addWidget(new QWidget);
    split->setSizes(QList<int>() << 200 << 200);
    lay->addWidget(split);
    widget.setLayout(lay);
    topLevel.show();

    QTest::qWait(100);

    widget.hide();
    split->widget(0)->setHidden(hideWidget1);
    split->widget(1)->setHidden(hideWidget2);
    widget.show();
    QTest::qWait(100);

    QTEST(split->sizes(), "finalValues");
    QTEST(split->handle(1)->isVisible(), "handleVisible");
}

void tst_QSplitter::testRemoval()
{

    // This test relies on the internal structure of QSplitter That is, that
    // there is a handle before every splitter, but sometimes that handle is
    // hidden. But definiately when something is removed the front handle
    // should not be visible.

    QSplitter split;
    split.addWidget(new QWidget);
    split.addWidget(new QWidget);
    split.show();
    QTest::qWait(100);

    QCOMPARE(split.handle(0)->isVisible(), false);
    QSplitterHandle *handle = split.handle(1);
    QCOMPARE(handle->isVisible(), true);

    delete split.widget(0);
    QSplitterHandle *sameHandle = split.handle(0);
    QCOMPARE(handle, sameHandle);
    QCOMPARE(sameHandle->isVisible(), false);
}

class MyFriendlySplitter : public QSplitter
{
public:
    MyFriendlySplitter(QWidget *parent = 0) : QSplitter(parent) {}
    void setRubberBand(int pos) { QSplitter::setRubberBand(pos); }

    friend class tst_QSplitter;
};

void tst_QSplitter::rubberBandNotInSplitter()
{
    MyFriendlySplitter split;
    split.addWidget(new QWidget);
    split.addWidget(new QWidget);
    split.setOpaqueResize(false);
    QCOMPARE(split.count(), 2);
    split.setRubberBand(2);
    QCOMPARE(split.count(), 2);
}

void tst_QSplitter::task187373_addAbstractScrollAreas_data()
{
    QTest::addColumn<QString>("className");
    QTest::addColumn<bool>("addInConstructor");
    QTest::addColumn<bool>("addOutsideConstructor");

    QStringList classNames;
    classNames << QLatin1String("QGraphicsView");
    classNames << QLatin1String("QMdiArea");
    classNames << QLatin1String("QScrollArea");
    classNames << QLatin1String("QTextEdit");
    classNames << QLatin1String("QTreeView");

    foreach (QString className, classNames) {
        QTest::newRow(qPrintable(QString("%1 1").arg(className))) << className << false << true;
        QTest::newRow(qPrintable(QString("%1 2").arg(className))) << className << true << false;
        QTest::newRow(qPrintable(QString("%1 3").arg(className))) << className << true << true;
    }
}

static QAbstractScrollArea *task187373_createScrollArea(
    QSplitter *splitter, const QString &className, bool addInConstructor)
{
    if (className == QLatin1String("QGraphicsView"))
        return new QGraphicsView(addInConstructor ? splitter : 0);
    if (className == QLatin1String("QMdiArea"))
        return new QMdiArea(addInConstructor ? splitter : 0);
    if (className == QLatin1String("QScrollArea"))
        return new QScrollArea(addInConstructor ? splitter : 0);
    if (className == QLatin1String("QTextEdit"))
        return new QTextEdit(addInConstructor ? splitter : 0);
    if (className == QLatin1String("QTreeView"))
        return new QTreeView(addInConstructor ? splitter : 0);
    return 0;
}

void tst_QSplitter::task187373_addAbstractScrollAreas()
{
    QFETCH(QString, className);
    QFETCH(bool, addInConstructor);
    QFETCH(bool, addOutsideConstructor);
    Q_ASSERT(addInConstructor || addOutsideConstructor);

    QSplitter *splitter = new QSplitter;
    splitter->show();
    Q_ASSERT(splitter->isVisible());

    QAbstractScrollArea *w = task187373_createScrollArea(splitter, className, addInConstructor);
    Q_ASSERT(w);
    if (addOutsideConstructor)
        splitter->addWidget(w);

    QTRY_VERIFY(w->isVisible());
    QVERIFY(!w->isHidden());
    QVERIFY(w->viewport()->isVisible());
    QVERIFY(!w->viewport()->isHidden());
}

//! A simple QTextEdit which can switch between two different size states
class MyTextEdit : public QTextEdit 
{
    public:
        MyTextEdit(const QString & text, QWidget* parent = NULL)
            : QTextEdit(text, parent) ,  m_iFactor(1)
        {
            setSizePolicy(QSizePolicy::Maximum, QSizePolicy::Maximum);
        }
        virtual QSize minimumSizeHint () const
        {
            return QSize(200, 200) * m_iFactor;
        }
        virtual QSize sizeHint() const
        {
            return QSize(390, 390) * m_iFactor;
        }
        int m_iFactor;
};

void tst_QSplitter::task169702_sizes()
{
    QWidget topLevel;
    // Create two nested (non-collapsible) splitters
    QSplitter* outerSplitter = new QSplitter(Qt::Vertical, &topLevel);
    outerSplitter->setChildrenCollapsible(false);
    QSplitter* splitter = new QSplitter(Qt::Horizontal, outerSplitter);
    splitter->setChildrenCollapsible(false);

    // populate the outer splitter
    outerSplitter->addWidget(new QTextEdit("Foo"));
    outerSplitter->addWidget(splitter);
    outerSplitter->setStretchFactor(0, 1);
    outerSplitter->setStretchFactor(1, 0);

    // populate the inner splitter
    MyTextEdit* testW = new MyTextEdit("TextEdit with size restriction");
    splitter->addWidget(testW);
    splitter->addWidget(new QTextEdit("Bar"));

    outerSplitter->setGeometry(100, 100, 500, 500);
    topLevel.show();

    QTest::qWait(100);
    testW->m_iFactor++;
    testW->updateGeometry();
    QTest::qWait(500);//100 is too fast for Maemo

    //Make sure the minimimSizeHint is respected
    QCOMPARE(testW->size().height(), testW->minimumSizeHint().height());
}

void tst_QSplitter::taskQTBUG_4101_ensureOneNonCollapsedWidget_data()
{
    QTest::addColumn<bool>("testingHide");

    QTest::newRow("last non collapsed hidden") << true;
    QTest::newRow("last non collapsed deleted") << false;
}

void tst_QSplitter::taskQTBUG_4101_ensureOneNonCollapsedWidget()
{
    QFETCH(bool, testingHide);

    MyFriendlySplitter s;
    QLabel *l;
    for (int i = 0; i < 5; ++i) {
        l = new QLabel(QString("Label ") + QChar('A' + i));
        l->setAlignment(Qt::AlignCenter);
        s.addWidget(l);
        s.moveSplitter(0, i);  // Collapse all the labels except the last one.
    }

    s.show();
    if (testingHide)
        l->hide();
    else
        delete l;
    QTest::qWait(100);
    QVERIFY(s.sizes().at(0) > 0);
}

QTEST_MAIN(tst_QSplitter)
#include "tst_qsplitter.moc"
