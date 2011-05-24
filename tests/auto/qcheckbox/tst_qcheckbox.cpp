/****************************************************************************
**
** Copyright (C) 2011 Nokia Corporation and/or its subsidiary(-ies).
** All rights reserved.
** Contact: Nokia Corporation (qt-info@nokia.com)
**
** This file is part of the test suite of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:LGPL$
** GNU Lesser General Public License Usage
** This file may be used under the terms of the GNU Lesser General Public
** License version 2.1 as published by the Free Software Foundation and
** appearing in the file LICENSE.LGPL included in the packaging of this
** file. Please review the following information to ensure the GNU Lesser
** General Public License version 2.1 requirements will be met:
** http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, Nokia gives you certain additional
** rights. These rights are described in the Nokia Qt LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
** GNU General Public License Usage
** Alternatively, this file may be used under the terms of the GNU General
** Public License version 3.0 as published by the Free Software Foundation
** and appearing in the file LICENSE.GPL included in the packaging of this
** file. Please review the following information to ensure the GNU General
** Public License version 3.0 requirements will be met:
** http://www.gnu.org/copyleft/gpl.html.
**
** Other Usage
** Alternatively, this file may be used in accordance with the terms and
** conditions contained in a signed written agreement between you and Nokia.
**
**
**
**
**
** $QT_END_LICENSE$
**
****************************************************************************/


#include <QtTest/QtTest>


#include "qcheckbox.h"
#include <qapplication.h>
#include <qpixmap.h>
#include <qdatetime.h>
#include <qcheckbox.h>

//TESTED_CLASS=
//TESTED_FILES=

class tst_QCheckBox : public QObject
{
Q_OBJECT

public:
    tst_QCheckBox();
    virtual ~tst_QCheckBox();

public slots:
    void initTestCase();
    void cleanupTestCase();
    void init();
    void cleanup();

private slots:
    void isChecked();
    void setChecked();
    void setNoChange();
    void setTriState();
    void isTriState();
    void text();
    void setText_data();
    void setText();
    void isToggleButton();
    void setDown();
    void isDown();
    void isOn();
    void checkState();
    void autoRepeat();
    void setAutoRepeat();
    void toggle();
    void pressed();
    void released();
    void clicked();
    void toggled();
    void stateChanged();
    void accel();
    void setAccel();
    void group();
    void foregroundRole();
    void minimumSizeHint();

protected slots:
    void onClicked();
    void onToggled( bool on );
    void onPressed();
    void onReleased();
    void onStateChanged( int state );

private:
    uint click_count;
    uint toggle_count;
    uint press_count;
    uint release_count;
    int cur_state;
    uint tmp;
    QCheckBox *testWidget;
    uint tmp2;
};

tst_QCheckBox::tst_QCheckBox()
{
}

tst_QCheckBox::~tst_QCheckBox()
{
}

void tst_QCheckBox::initTestCase()
{
    // Create the test class
    testWidget = new QCheckBox(0);
    testWidget->setObjectName("testObject");
    testWidget->resize( 200, 200 );
    testWidget->show();
}

void tst_QCheckBox::cleanupTestCase()
{
    delete testWidget;
    testWidget = 0;
}

void tst_QCheckBox::init()
{
    testWidget->setTristate( FALSE );
    testWidget->setChecked( FALSE );
    testWidget->setAutoRepeat( FALSE );
}

void tst_QCheckBox::cleanup()
{
    disconnect(testWidget, SIGNAL(pressed()), this, SLOT(onPressed()));
    disconnect(testWidget, SIGNAL(released()), this, SLOT(onReleased()));
    disconnect(testWidget, SIGNAL(clicked()), this, SLOT(onClicked()));
    disconnect(testWidget, SIGNAL(toggled(bool)), this, SLOT(onToggled(bool)));
}

void tst_QCheckBox::onClicked()
{
    click_count++;
}

void tst_QCheckBox::onPressed()
{
    press_count++;
}

void tst_QCheckBox::onReleased()
{
    release_count++;
}

void tst_QCheckBox::onToggled( bool /*on*/ )
{
    toggle_count++;
}

// ***************************************************

void tst_QCheckBox::isChecked()
{
    DEPENDS_ON( "setChecked" );
}

void tst_QCheckBox::setChecked()
{
    testWidget->setChecked( TRUE );
    QVERIFY( testWidget->isChecked() );
    QVERIFY( testWidget->isChecked() );
    QVERIFY( testWidget->checkState() == Qt::Checked );

    testWidget->setChecked( FALSE );
    QVERIFY( !testWidget->isChecked() );
    QVERIFY( !testWidget->isChecked() );
    QVERIFY( testWidget->checkState() == Qt::Unchecked );

    testWidget->setChecked( FALSE );
    QTest::keyClick( testWidget, ' ' );
    QVERIFY( testWidget->isChecked() );

    QTest::keyClick( testWidget, ' ' );
    QVERIFY( !testWidget->isChecked() );
}

void tst_QCheckBox::setTriState()
{
    testWidget->setTristate( TRUE );
    QVERIFY( testWidget->isTristate() );
    QVERIFY( testWidget->checkState() == Qt::Unchecked );

    testWidget->setCheckState(Qt::PartiallyChecked);
    QVERIFY( testWidget->checkState() == Qt::PartiallyChecked );

    testWidget->setChecked( TRUE );
    QVERIFY( testWidget->isChecked() );
    QVERIFY( testWidget->checkState() == Qt::Checked );

    testWidget->setChecked( FALSE );
    QVERIFY( !testWidget->isChecked() );
    QVERIFY( testWidget->checkState() == Qt::Unchecked );

    testWidget->setCheckState(Qt::PartiallyChecked);
    QVERIFY( testWidget->checkState() == Qt::PartiallyChecked );

    testWidget->setTristate( FALSE );
    QVERIFY( !testWidget->isTristate() );

    testWidget->setCheckState(Qt::PartiallyChecked);
    QVERIFY( testWidget->checkState() == Qt::PartiallyChecked );

    testWidget->setChecked( TRUE );
    QVERIFY( testWidget->checkState() == Qt::Checked );

    testWidget->setChecked( FALSE );
    QVERIFY( testWidget->checkState() == Qt::Unchecked );
}

void tst_QCheckBox::isTriState()
{
    DEPENDS_ON( "setTriState" );
}

void tst_QCheckBox::setNoChange()
{
    DEPENDS_ON( "setTriState" );
}

void tst_QCheckBox::text()
{
    DEPENDS_ON( "setText" );
}

void tst_QCheckBox::setText_data()
{
    QTest::addColumn<QString>("s1");

#ifdef Q_OS_WIN32
    QTest::newRow( "win32_data0" ) << QString("This is a text");
    QTest::newRow( "win32_data1" ) << QString("A");
    QTest::newRow( "win32_data2" ) << QString("ABCDEFG ");
    QTest::newRow( "win32_data3" ) << QString("Text\nwith a cr-lf");
    QTest::newRow( "win32_data4" ) << QString("");
#else
    QTest::newRow( "data0" ) << QString("This is a text");
    QTest::newRow( "data1" ) << QString("A");
    QTest::newRow( "data2" ) << QString("ABCDEFG ");
    QTest::newRow( "data3" ) << QString("Text\nwith a cr-lf");
    QTest::newRow( "data4" ) << QString("");
#endif
}

void tst_QCheckBox::setText()
{
    QFETCH( QString, s1 );
    testWidget->setText( s1 );
    QCOMPARE( testWidget->text(), s1 );
}

void tst_QCheckBox::setDown()
{
    testWidget->setDown( TRUE );
    QVERIFY( testWidget->isDown() );

    testWidget->setDown( FALSE );
    QVERIFY( !testWidget->isDown() );
}

void tst_QCheckBox::isDown()
{
    DEPENDS_ON( "setDown" );
}

void tst_QCheckBox::isOn()
{
    DEPENDS_ON( "setChecked" );
}

void tst_QCheckBox::checkState()
{
    DEPENDS_ON( "setChecked" );
}

void tst_QCheckBox::autoRepeat()
{
    DEPENDS_ON( "setAutoRepeat" );
}

void tst_QCheckBox::setAutoRepeat()
{
    // setAutoRepeat has no effect on toggle buttons
    QVERIFY( testWidget->isCheckable() );
}

void tst_QCheckBox::toggle()
{
    bool cur_state;
    cur_state = testWidget->isChecked();
    testWidget->toggle();
    QVERIFY( cur_state != testWidget->isChecked() );

    cur_state = testWidget->isChecked();
    testWidget->toggle();
    QVERIFY( cur_state != testWidget->isChecked() );

    cur_state = testWidget->isChecked();
    testWidget->toggle();
    QVERIFY( cur_state != testWidget->isChecked() );
}

void tst_QCheckBox::pressed()
{
    connect(testWidget, SIGNAL(pressed()), this, SLOT(onPressed()));
    connect(testWidget, SIGNAL(released()), this, SLOT(onReleased()));
    press_count = 0;
    release_count = 0;
    testWidget->setDown(FALSE);
    QVERIFY( !testWidget->isChecked() );

    QTest::keyPress( testWidget, Qt::Key_Space );
    QTest::qWait(100);
    QVERIFY( press_count == 1 );
    QVERIFY( release_count == 0 );
    QVERIFY( !testWidget->isChecked() );

    QTest::keyRelease( testWidget, Qt::Key_Space );
    QTest::qWait(100);
    QVERIFY( press_count == 1 );
    QVERIFY( release_count == 1 );
    QVERIFY( testWidget->isChecked() );
}

void tst_QCheckBox::released()
{
    DEPENDS_ON( "pressed" );
}

void tst_QCheckBox::clicked()
{
    DEPENDS_ON( "pressed" );
}

void tst_QCheckBox::toggled()
{
    connect(testWidget, SIGNAL(toggled(bool)), this, SLOT(onToggled(bool)));
    click_count = 0;
    toggle_count = 0;
    testWidget->toggle();
    QCOMPARE( toggle_count, (uint)1 );

    testWidget->toggle();
    QCOMPARE( toggle_count, (uint)2 );

    testWidget->toggle();
    QCOMPARE( toggle_count, (uint)3 );

    QCOMPARE( click_count, (uint)0 );
}

void tst_QCheckBox::onStateChanged( int state )
{
    cur_state = state;
}

void tst_QCheckBox::stateChanged()
{
    QSignalSpy stateChangedSpy(testWidget, SIGNAL(stateChanged(int)));
    connect(testWidget, SIGNAL(stateChanged(int)), this, SLOT(onStateChanged(int)));
    cur_state = -1;
    testWidget->setChecked( TRUE );
    qApp->processEvents();
    QCOMPARE( cur_state, (int)2 );

    cur_state = -1;
    testWidget->setChecked( FALSE );
    qApp->processEvents();
    QCOMPARE( cur_state, (int)0 );

    cur_state = -1;
    testWidget->setCheckState(Qt::PartiallyChecked);
    qApp->processEvents();
    QCOMPARE( cur_state, (int)1 );

    QCOMPARE(stateChangedSpy.count(), 3);
    testWidget->setCheckState(Qt::PartiallyChecked);
    qApp->processEvents();
    QCOMPARE(stateChangedSpy.count(), 3);
}

void tst_QCheckBox::isToggleButton()
{
    QVERIFY( testWidget->isCheckable() );
}

void tst_QCheckBox::accel()
{
    QSKIP("This test is empty for now", SkipAll);
}

void tst_QCheckBox::setAccel()
{
    QSKIP("This test is empty for now", SkipAll);
}

void tst_QCheckBox::group()
{
    QSKIP("This test is empty for now", SkipAll);
}

void tst_QCheckBox::foregroundRole()
{
    QVERIFY(testWidget->foregroundRole() == QPalette::WindowText);
}

void tst_QCheckBox::minimumSizeHint()
{
    QCheckBox box(tr("CheckBox's sizeHint is the same as it's minimumSizeHint"));
    QCOMPARE(box.sizeHint(), box.minimumSizeHint());
}

QTEST_MAIN(tst_QCheckBox)
#include "tst_qcheckbox.moc"
